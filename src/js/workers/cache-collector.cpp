/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "cache-collector.h"
#include "../generics.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <regex>
#include <sstream>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include <httplib.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

namespace cache_collector {

namespace fs = std::filesystem;

static bool iends_with(const std::string& str, std::string_view suffix) {
	if (str.size() < suffix.size())
		return false;

	auto str_it = str.rbegin();
	auto suf_it = suffix.rbegin();
	for (; suf_it != suffix.rend(); ++str_it, ++suf_it) {
		if (std::tolower(static_cast<unsigned char>(*str_it)) != std::tolower(static_cast<unsigned char>(*suf_it)))
			return false;
	}
	return true;
}

static int64_t file_time_to_ms(const fs::file_time_type& ft) {
	auto sctp = std::chrono::clock_cast<std::chrono::system_clock>(ft);
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(sctp.time_since_epoch());
	return ms.count();
}

static int64_t now_ms() {
	auto now = std::chrono::system_clock::now();
	return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}

static std::vector<uint8_t> read_file_bytes(const fs::path& file_path) {
	std::ifstream ifs(file_path, std::ios::binary | std::ios::ate);
	if (!ifs)
		throw std::runtime_error(std::format("Cannot open file: {}", file_path.string()));

	auto size = ifs.tellg();
	ifs.seekg(0, std::ios::beg);

	std::vector<uint8_t> buffer(static_cast<size_t>(size));
	if (size > 0)
		ifs.read(reinterpret_cast<char*>(buffer.data()), size);

	return buffer;
}

static std::string read_file_text(const fs::path& file_path) {
	std::ifstream ifs(file_path);
	if (!ifs)
		throw std::runtime_error(std::format("Cannot open file: {}", file_path.string()));

	std::ostringstream ss;
	ss << ifs.rdbuf();
	return ss.str();
}

static void write_file_text(const fs::path& file_path, const std::string& text) {
	std::ofstream ofs(file_path);
	if (!ofs)
		throw std::runtime_error(std::format("Cannot write file: {}", file_path.string()));

	ofs << text;
}

static std::string to_hex(const std::vector<uint8_t>& data) {
	static constexpr char hex_chars[] = "0123456789abcdef";
	std::string result;
	result.reserve(data.size() * 2);
	for (uint8_t b : data) {
		result.push_back(hex_chars[(b >> 4) & 0x0F]);
		result.push_back(hex_chars[b & 0x0F]);
	}
	return result;
}

static std::vector<uint8_t> secure_random_bytes(size_t num_bytes) {
	std::vector<uint8_t> bytes(num_bytes);
	if (RAND_bytes(bytes.data(), static_cast<int>(num_bytes)) != 1)
		throw std::runtime_error("Failed to generate secure random bytes");
	return bytes;
}

static std::string evp_hash_hex(std::span<const uint8_t> data, const EVP_MD* md) {
	EVP_MD_CTX* ctx = EVP_MD_CTX_new();
	if (!ctx)
		throw std::runtime_error("EVP_MD_CTX_new failed");
	if (EVP_DigestInit_ex(ctx, md, nullptr) != 1 ||
		EVP_DigestUpdate(ctx, data.data(), data.size()) != 1) {
		EVP_MD_CTX_free(ctx);
		throw std::runtime_error("Hash computation failed");
	}
	std::vector<uint8_t> digest(EVP_MD_size(md));
	unsigned int digest_len = 0;
	if (EVP_DigestFinal_ex(ctx, digest.data(), &digest_len) != 1) {
		EVP_MD_CTX_free(ctx);
		throw std::runtime_error("Hash finalization failed");
	}
	EVP_MD_CTX_free(ctx);
	digest.resize(digest_len);
	return to_hex(digest);
}

static std::string random_hex(size_t num_bytes) {
	return to_hex(secure_random_bytes(num_bytes));
}



static std::string ms_to_iso8601(int64_t ms) {
	auto tp = std::chrono::system_clock::time_point(std::chrono::milliseconds(ms));
	auto time_t_val = std::chrono::system_clock::to_time_t(tp);
	std::tm tm_val{};
#ifdef _WIN32
	gmtime_s(&tm_val, &time_t_val);
#else
	gmtime_r(&time_t_val, &tm_val);
#endif
	char buf[64];
	std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tm_val);

	auto ms_part = ms % 1000;
	return std::format("{}.{:03d}Z", buf, ms_part);
}

struct ParsedURL {
	std::string scheme;
	std::string host;
	int port;
	std::string path;
};

static ParsedURL parse_url(const std::string& url) {
	ParsedURL result;

	// Extract scheme
	auto scheme_end = url.find("://");
	if (scheme_end != std::string::npos) {
		result.scheme = url.substr(0, scheme_end);
		auto rest = url.substr(scheme_end + 3);

		// Extract host:port/path
		auto path_start = rest.find('/');
		std::string host_port;
		if (path_start != std::string::npos) {
			host_port = rest.substr(0, path_start);
			result.path = rest.substr(path_start);
		} else {
			host_port = rest;
			result.path = "/";
		}

		auto colon = host_port.find(':');
		if (colon != std::string::npos) {
			result.host = host_port.substr(0, colon);
			result.port = std::stoi(host_port.substr(colon + 1));
		} else {
			result.host = host_port;
			result.port = (result.scheme == "https") ? 443 : 80;
		}
	}

	return result;
}

static std::string trim(const std::string& s) {
	auto start = s.find_first_not_of(" \t\r\n");
	if (start == std::string::npos)
		return "";
	auto end = s.find_last_not_of(" \t\r\n");
	return s.substr(start, end - start + 1);
}

static std::vector<std::string> split(const std::string& s, char delimiter) {
	std::vector<std::string> result;
	std::istringstream stream(s);
	std::string token;
	while (std::getline(stream, token, delimiter))
		result.push_back(token);
	return result;
}


HttpResponse https_request(const std::string& url,
                           const std::string& method,
                           const std::unordered_map<std::string, std::string>& headers,
                           const std::vector<uint8_t>& body) {
	auto parsed = parse_url(url);

	// Extract Content-Type from headers (if present) so it can be passed as
	// the dedicated content_type argument to httplib::Post(). The JS code
	// passes all headers including Content-Type directly to the request.
	httplib::Headers hdr;
	std::string content_type;
	for (const auto& [key, value] : headers) {
		if (key == "Content-Type")
			content_type = value;
		else
			hdr.emplace(key, value);
	}

	httplib::Result res;

	if (parsed.scheme == "https") {
		httplib::SSLClient cli(parsed.host, parsed.port);

		if (method == "POST")
			res = cli.Post(parsed.path, hdr,
				reinterpret_cast<const char*>(body.data()), body.size(), content_type);
		else
			res = cli.Get(parsed.path, hdr);
	} else {
		httplib::Client cli(parsed.host, parsed.port);

		if (method == "POST")
			res = cli.Post(parsed.path, hdr,
				reinterpret_cast<const char*>(body.data()), body.size(), content_type);
		else
			res = cli.Get(parsed.path, hdr);
	}

	// JS: req.on('error', reject) — reject the promise on connection errors.
	if (!res)
		throw std::runtime_error(httplib::to_string(res.error()));

	HttpResponse response;
	response.status = res->status;
	response.ok = (res->status >= 200 && res->status < 300);
	response.data = res->body;

	return response;
}

JsonPostResponse json_post(const std::string& url, const nlohmann::json& payload, const std::string& user_agent) {
	std::string body = payload.dump();
	std::vector<uint8_t> bodyBytes(body.begin(), body.end());

	// JS: json_post calls https_request() with method/headers/body.
	std::unordered_map<std::string, std::string> headers;
	headers["Content-Type"] = "application/json";
	headers["User-Agent"] = user_agent;
	headers["Content-Length"] = std::to_string(body.size());

	auto res = https_request(url, "POST", headers, bodyBytes);

	JsonPostResponse response;
	response.status = res.status;
	response.ok = res.ok;

	if (response.ok) {
		try {
			response.response_json = nlohmann::json::parse(res.data);
		} catch (...) {}
	}

	return response;
}

std::vector<uint8_t> build_multipart(const std::string& boundary, const std::vector<uint8_t>& file_buf, int64_t offset) {
	std::vector<uint8_t> result;

	// file field
	std::string file_header = std::format(
		"--{}\r\n"
		"Content-Disposition: form-data; name=\"file\"; filename=\"chunk.bin\"\r\n"
		"Content-Type: application/octet-stream\r\n\r\n",
		boundary
	);
	result.insert(result.end(), file_header.begin(), file_header.end());
	result.insert(result.end(), file_buf.begin(), file_buf.end());

	std::string crlf = "\r\n";
	result.insert(result.end(), crlf.begin(), crlf.end());

	// offset field
	std::string offset_str = std::to_string(offset);
	std::string offset_part = std::format(
		"--{}\r\n"
		"Content-Disposition: form-data; name=\"offset\"\r\n\r\n"
		"{}\r\n",
		boundary, offset_str
	);
	result.insert(result.end(), offset_part.begin(), offset_part.end());

	// closing boundary
	std::string closing = std::format("--{}--\r\n", boundary);
	result.insert(result.end(), closing.begin(), closing.end());

	return result;
}

void upload_chunks(const std::string& url, const std::vector<uint8_t>& buffer) {
	std::string boundary = random_hex(16);

	for (int64_t offset = 0; offset < static_cast<int64_t>(buffer.size()); offset += CHUNK_SIZE) {
		int64_t end = (std::min)(offset + CHUNK_SIZE, static_cast<int64_t>(buffer.size()));
		std::vector<uint8_t> chunk(buffer.begin() + offset, buffer.begin() + end);
		std::vector<uint8_t> body_bytes = build_multipart(boundary, chunk, offset);

		std::string content_type = std::format("multipart/form-data; boundary={}", boundary);
		// JS: upload_chunks calls https_request() with method/headers/body.
		std::unordered_map<std::string, std::string> headers;
		headers["Content-Type"] = content_type;
		headers["Content-Length"] = std::to_string(body_bytes.size());

		auto res = https_request(url, "POST", headers, body_bytes);

		if (!res.ok)
			throw std::runtime_error(std::format("upload chunk failed: {}", res.status));
	}
}

std::vector<std::unordered_map<std::string, std::string>> parse_build_info(const std::string& text) {
	auto lines = split(text, '\n');

	// filter empty lines
	std::erase_if(lines, [](const std::string& line) { return line.empty(); });

	if (lines.size() < 2)
		return {};

	// Parse headers: each header is "Name!Type:Size" — we only want the Name part
	auto header_parts = split(lines[0], '|');
	std::vector<std::string> headers;
	headers.reserve(header_parts.size());
	for (const auto& h : header_parts) {
		auto bang_pos = h.find('!');
		if (bang_pos != std::string::npos)
			headers.push_back(h.substr(0, bang_pos));
		else
			headers.push_back(h);
	}

	std::vector<std::unordered_map<std::string, std::string>> rows;

	for (size_t i = 1; i < lines.size(); i++) {
		auto values = split(lines[i], '|');
		std::unordered_map<std::string, std::string> row;
		for (size_t j = 0; j < headers.size(); j++) {
			if (j < values.size())
				row[headers[j]] = trim(values[j]);
			else
				row[headers[j]] = "";
		}
		rows.push_back(std::move(row));
	}

	return rows;
}

std::string hash_file(const fs::path& file_path) {
	const std::vector<uint8_t> bytes = read_file_bytes(file_path);
	return evp_hash_hex(bytes, EVP_md5());
}

std::vector<fs::path> find_binaries(const fs::path& flavor_dir) {
	std::vector<fs::path> binaries;

	try {
		for (const auto& entry : fs::directory_iterator(flavor_dir)) {
			if (entry.is_regular_file()) {
				auto name = entry.path().filename().string();
				if (iends_with(name, ".exe")) {
					std::string lower_name = name;
					std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
					               [](unsigned char c) { return std::tolower(c); });
					if (lower_name != "blizzarderror.exe")
						binaries.push_back(entry.path());
				}
			}

			if (entry.is_directory()) {
				auto dir_name = entry.path().filename().string();
				if (dir_name.size() >= BINARY_APP_DIR.size() &&
				    dir_name.substr(dir_name.size() - BINARY_APP_DIR.size()) == BINARY_APP_DIR) {
					auto app_binaries = scan_app_bundle(entry.path());
					binaries.insert(binaries.end(), app_binaries.begin(), app_binaries.end());
				}
			}
		}
	} catch (...) {}

	return binaries;
}

std::vector<fs::path> scan_app_bundle(const fs::path& app_dir) {
	std::vector<fs::path> binaries;
	auto macos_dir = app_dir / "Contents" / "MacOS";

	try {
		for (const auto& entry : fs::directory_iterator(macos_dir)) {
			if (entry.is_regular_file())
				binaries.push_back(entry.path());
		}
	} catch (...) {}

	return binaries;
}

std::vector<CacheFileEntry> scan_wdb(const fs::path& flavor_dir) {
	auto wdb_root = flavor_dir / "Cache" / "WDB";
	std::vector<CacheFileEntry> wdb_files;

	std::error_code ec;
	if (!fs::is_directory(wdb_root, ec))
		return wdb_files;

	try {
		for (const auto& locale_entry : fs::directory_iterator(wdb_root)) {
			if (!locale_entry.is_directory())
				continue;

			auto locale_name = locale_entry.path().filename().string();

			try {
				for (const auto& file_entry : fs::directory_iterator(locale_entry.path())) {
					auto file_name = file_entry.path().filename().string();
					if (!file_name.ends_with(".wdb"))
						continue;

					try {
						auto file_size = static_cast<int64_t>(fs::file_size(file_entry.path()));
						if (file_size > WDB_MIN_SIZE) {
							CacheFileEntry entry;
							entry.name = file_name;
							entry.locale = locale_name;
							entry.size = file_size;
							entry.path = file_entry.path();
							entry.modified_at = file_time_to_ms(fs::last_write_time(file_entry.path()));
							wdb_files.push_back(std::move(entry));
						}
					} catch (...) {}
				}
			} catch (...) {
				continue;
			}
		}
	} catch (...) {}

	return wdb_files;
}

std::vector<CacheFileEntry> scan_adb(const fs::path& flavor_dir) {
	auto adb_root = flavor_dir / "Cache" / "ADB";
	std::vector<CacheFileEntry> adb_files;

	std::error_code ec;
	if (!fs::is_directory(adb_root, ec))
		return adb_files;

	try {
		for (const auto& locale_entry : fs::directory_iterator(adb_root)) {
			if (!locale_entry.is_directory())
				continue;

			auto file_path = locale_entry.path() / "DBCache.bin";
			try {
				auto file_size = static_cast<int64_t>(fs::file_size(file_path));
				if (file_size > WDB_MIN_SIZE) {
					CacheFileEntry entry;
					entry.name = "DBCache.bin";
					entry.locale = locale_entry.path().filename().string();
					entry.size = file_size;
					entry.path = file_path;
					entry.modified_at = file_time_to_ms(fs::last_write_time(file_path));
					adb_files.push_back(std::move(entry));
				}
			} catch (...) {}
		}
	} catch (...) {}

	return adb_files;
}

nlohmann::json load_state(const fs::path& state_path) {
	try {
		auto text = read_file_text(state_path);
		return nlohmann::json::parse(text);
	} catch (...) {
		return nlohmann::json::object();
	}
}

void save_state(const fs::path& state_path, const nlohmann::json& state) {
	write_file_text(state_path, state.dump());
}

void upload_flavor(const FlavorResult& result, nlohmann::json& state, const WorkerConfig& config, const LogCallback& log) {
	if (result.cache_files.empty())
		return;

	std::string flavor_key = std::format("{}|{}|{}", result.product, result.patch, result.build_number);
	nlohmann::json prev_hashes = state.contains(flavor_key) ? state[flavor_key] : nlohmann::json::object();

	std::unordered_map<std::string, std::vector<uint8_t>> file_buffers;
	std::unordered_map<std::string, std::string> file_hashes;
	nlohmann::json submit_files = nlohmann::json::array();

	int64_t now = now_ms();

	for (const auto& wdb : result.cache_files) {
		try {
			if (now - wdb.modified_at > MAX_AGE_MS) {
				log(std::format("skipping {} ({}): file too old", wdb.name, wdb.locale));
				continue;
			}

			auto buffer = read_file_bytes(wdb.path);

			if (buffer.empty()) {
				log(std::format("skipping {} ({}): empty file", wdb.name, wdb.locale));
				continue;
			}

			std::string key = std::format("{}/{}", wdb.locale, wdb.name);
			std::string hash = evp_hash_hex(buffer, EVP_sha256());

			file_hashes[key] = hash;

			if (prev_hashes.contains(key) && prev_hashes[key].get<std::string>() == hash)
				continue;

			file_buffers[key] = std::move(buffer);
			submit_files.push_back({
				{"name", wdb.name},
				{"locale", wdb.locale},
				{"size", file_buffers[key].size()},
				{"modified_at", ms_to_iso8601(wdb.modified_at)}
			});
		} catch (const std::exception& e) {
			log(std::format("failed to read {}: {}", wdb.path.string(), e.what()));
		}
	}

	if (submit_files.empty()) {
		log(std::format("all files unchanged for {}, skipping", result.product));
		return;
	}

	nlohmann::json submit_payload = {
		{"machine_id", config.machine_id},
		{"product", result.product},
		{"patch", result.patch},
		{"build_number", [&]() -> int { try { return std::stoi(result.build_number); } catch (...) { return 0; } }()},
		{"build_key", result.build_key},
		{"cdn_key", result.cdn_key},
		{"binary_hashes", result.binary_hashes},
		{"files", submit_files}
	};

	auto submit_res = json_post(config.submit_url, submit_payload, config.user_agent);

	if (!submit_res.ok) {
		log(std::format("submit failed ({}) for {}", submit_res.status, result.product));
		return;
	}

	std::string submission_id = submit_res.response_json.value("submission_id", "");
	auto upload_urls = submit_res.response_json.value("upload_urls", nlohmann::json::object());
	log(std::format("submission {} created for {} ({} files)", submission_id, result.product, submit_files.size()));

	nlohmann::json checksums = nlohmann::json::object();

	for (const auto& [key, buffer] : file_buffers) {
		if (!upload_urls.contains(key)) {
			log(std::format("no upload URL for {}", key));
			continue;
		}

		std::string upload_url = upload_urls[key].template get<std::string>();

		try {
			upload_chunks(upload_url, buffer);
			checksums[key] = file_hashes[key];
		} catch (const std::exception& e) {
			log(std::format("upload failed for {}: {}", key, e.what()));
		}
	}

	nlohmann::json finalize_payload = {
		{"submission_id", submission_id},
		{"checksums", checksums}
	};

	auto finalize_res = json_post(config.finalize_url, finalize_payload, config.user_agent);

	if (finalize_res.ok) {
		log(std::format("submission {} finalized", submission_id));

		// update state with hashes of successfully uploaded files
		nlohmann::json new_hashes = prev_hashes;
		for (auto& [key, value] : checksums.items())
			new_hashes[key] = value;

		state[flavor_key] = new_hashes;
	} else {
		log(std::format("finalize failed ({}) for {}", finalize_res.status, submission_id));
	}
}

void collect(const WorkerConfig& config, const LogCallback& log) {
	try {
		nlohmann::json state = load_state(config.state_path);

		auto build_info_path = config.install_path / ".build.info";
		std::string build_info_text = read_file_text(build_info_path);
		auto builds = parse_build_info(build_info_text);

		if (builds.empty())
			return;

		// Scan root for flavor directories (directories named _*_)
		struct FlavorDir {
			std::string dir;
			std::string product;
		};
		std::vector<FlavorDir> flavor_dirs;

		for (const auto& entry : fs::directory_iterator(config.install_path)) {
			if (!entry.is_directory())
				continue;

			auto dir_name = entry.path().filename().string();
			if (!dir_name.starts_with('_') || !dir_name.ends_with('_'))
				continue;

			auto flavor_info_path = entry.path() / ".flavor.info";
			try {
				std::string flavor_text = read_file_text(flavor_info_path);
				flavor_text = trim(flavor_text);

				auto flavor_lines = split(flavor_text, '\n');
				std::string product;
				if (!flavor_lines.empty()) {
					product = trim(flavor_lines.back());
				}

				if (!product.empty())
					flavor_dirs.push_back({dir_name, product});
			} catch (...) {
				continue;
			}
		}

		for (const auto& flavor : flavor_dirs) {
			const std::unordered_map<std::string, std::string>* build_row = nullptr;
			for (const auto& b : builds) {
				auto it = b.find("Product");
				if (it != b.end() && it->second == flavor.product) {
					build_row = &b;
					break;
				}
			}

			if (!build_row)
				continue;

			auto flavor_path = config.install_path / flavor.dir;

			auto binary_paths = find_binaries(flavor_path);
			std::unordered_map<std::string, std::string> binary_hashes;
			for (const auto& bin_path : binary_paths) {
				try {
					binary_hashes[bin_path.filename().string()] = hash_file(bin_path);
				} catch (...) {}
			}

			auto wdb_files = scan_wdb(flavor_path);
			auto adb_files = scan_adb(flavor_path);
			std::vector<CacheFileEntry> cache_files;
			cache_files.insert(cache_files.end(), wdb_files.begin(), wdb_files.end());
			cache_files.insert(cache_files.end(), adb_files.begin(), adb_files.end());

			if (cache_files.empty())
				continue;

			// Extract version info
			auto version_it = build_row->find("Version");
			std::string version = (version_it != build_row->end()) ? version_it->second : "";
			std::regex version_re("^(.+)\\.(\\d+)$");
			std::smatch version_match;
			std::string patch, build_number;
			if (std::regex_match(version, version_match, version_re)) {
				patch = version_match[1].str();
				build_number = version_match[2].str();
			} else {
				patch = version;
				build_number = "";
			}

			auto build_key_it = build_row->find("Build Key");
			auto cdn_key_it = build_row->find("CDN Key");

			try {
				FlavorResult flavor_result;
				flavor_result.product = flavor.product;
				flavor_result.patch = patch;
				flavor_result.build_number = build_number;
				flavor_result.build_key = (build_key_it != build_row->end()) ? build_key_it->second : "";
				flavor_result.cdn_key = (cdn_key_it != build_row->end()) ? cdn_key_it->second : "";
				flavor_result.binary_hashes = binary_hashes;
				flavor_result.cache_files = std::move(cache_files);

				upload_flavor(flavor_result, state, config, log);
			} catch (const std::exception& e) {
				log(std::format("error for {}: {}", flavor.product, e.what()));
			}
		}

		save_state(config.state_path, state);
	} catch (const std::exception& err) {
		log(std::format("fatal: {}", err.what()));
	}
}

} // namespace cache_collector
