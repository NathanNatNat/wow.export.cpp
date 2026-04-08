/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "cache-collector.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <random>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <httplib.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

// TODO(conversion): JS uses Node.js crypto for MD5/SHA256 hashing.
// C++ uses OpenSSL via cpp-httplib's dependency or a standalone hash library.
// For now, we use a simple implementation placeholder that will be wired
// to a proper crypto library.
#ifdef _WIN32
#include <windows.h>
#include <wincrypt.h>
#else
// On Linux, use openssl if available, otherwise stub.
// TODO(conversion): Wire to OpenSSL or a lightweight hash library.
#endif

namespace cache_collector {

namespace fs = std::filesystem;

// ── Helper: case-insensitive string ends-with ──────────────────────
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

// ── Helper: convert file_time to milliseconds since epoch ──────────
static int64_t file_time_to_ms(const fs::file_time_type& ft) {
	auto sctp = std::chrono::clock_cast<std::chrono::system_clock>(ft);
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(sctp.time_since_epoch());
	return ms.count();
}

// ── Helper: current time in milliseconds ───────────────────────────
static int64_t now_ms() {
	auto now = std::chrono::system_clock::now();
	return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}

// ── Helper: read entire file into bytes ────────────────────────────
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

// ── Helper: read entire file as string ─────────────────────────────
static std::string read_file_text(const fs::path& file_path) {
	std::ifstream ifs(file_path);
	if (!ifs)
		throw std::runtime_error(std::format("Cannot open file: {}", file_path.string()));

	std::ostringstream ss;
	ss << ifs.rdbuf();
	return ss.str();
}

// ── Helper: write string to file ───────────────────────────────────
static void write_file_text(const fs::path& file_path, const std::string& text) {
	std::ofstream ofs(file_path);
	if (!ofs)
		throw std::runtime_error(std::format("Cannot write file: {}", file_path.string()));

	ofs << text;
}

// ── Helper: hex encode ─────────────────────────────────────────────
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

// ── Helper: generate random hex string ─────────────────────────────
// JS: crypto.randomBytes(16).toString('hex')
static std::string random_hex(size_t num_bytes) {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<int> dist(0, 255);

	std::vector<uint8_t> bytes(num_bytes);
	for (auto& b : bytes)
		b = static_cast<uint8_t>(dist(gen));

	return to_hex(bytes);
}

// ── Helper: simple MD5 hash ────────────────────────────────────────
// TODO(conversion): This uses a minimal MD5 implementation. Should be replaced
// with OpenSSL or a proper crypto library for production use.
// For now, we compute a hash by reading the file in chunks and using
// a platform hash function or a bundled MD5 implementation.

// Minimal MD5 implementation for file hashing.
namespace md5_impl {

struct MD5Context {
	uint32_t state[4];
	uint64_t count;
	uint8_t buffer[64];
};

static constexpr uint32_t S[64] = {
	7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,
	5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,
	4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,
	6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21
};

static constexpr uint32_t K[64] = {
	0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
	0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
	0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
	0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
	0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
	0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
	0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
	0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
	0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
	0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
	0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
	0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
	0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
	0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
	0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
	0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
};

static uint32_t left_rotate(uint32_t x, uint32_t c) {
	return (x << c) | (x >> (32 - c));
}

static void md5_transform(uint32_t state[4], const uint8_t block[64]) {
	uint32_t a = state[0], b = state[1], c = state[2], d = state[3];

	uint32_t M[16];
	for (int i = 0; i < 16; i++)
		M[i] = static_cast<uint32_t>(block[i * 4]) |
		       (static_cast<uint32_t>(block[i * 4 + 1]) << 8) |
		       (static_cast<uint32_t>(block[i * 4 + 2]) << 16) |
		       (static_cast<uint32_t>(block[i * 4 + 3]) << 24);

	for (uint32_t i = 0; i < 64; i++) {
		uint32_t f, g;
		if (i < 16) {
			f = (b & c) | (~b & d);
			g = i;
		} else if (i < 32) {
			f = (d & b) | (~d & c);
			g = (5 * i + 1) % 16;
		} else if (i < 48) {
			f = b ^ c ^ d;
			g = (3 * i + 5) % 16;
		} else {
			f = c ^ (b | ~d);
			g = (7 * i) % 16;
		}

		uint32_t temp = d;
		d = c;
		c = b;
		b = b + left_rotate(a + f + K[i] + M[g], S[i]);
		a = temp;
	}

	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;
}

static void md5_init(MD5Context& ctx) {
	ctx.state[0] = 0x67452301;
	ctx.state[1] = 0xefcdab89;
	ctx.state[2] = 0x98badcfe;
	ctx.state[3] = 0x10325476;
	ctx.count = 0;
	std::memset(ctx.buffer, 0, sizeof(ctx.buffer));
}

static void md5_update(MD5Context& ctx, const uint8_t* data, size_t len) {
	size_t index = static_cast<size_t>(ctx.count % 64);
	ctx.count += len;

	size_t i = 0;
	if (index) {
		size_t part_len = 64 - index;
		if (len >= part_len) {
			std::memcpy(ctx.buffer + index, data, part_len);
			md5_transform(ctx.state, ctx.buffer);
			i = part_len;
		} else {
			std::memcpy(ctx.buffer + index, data, len);
			return;
		}
	}

	for (; i + 63 < len; i += 64)
		md5_transform(ctx.state, data + i);

	if (i < len)
		std::memcpy(ctx.buffer, data + i, len - i);
}

static std::vector<uint8_t> md5_final(MD5Context& ctx) {
	uint8_t padding[64] = { 0x80 };

	uint64_t bits = ctx.count * 8;
	size_t index = static_cast<size_t>(ctx.count % 64);
	size_t pad_len = (index < 56) ? (56 - index) : (120 - index);

	md5_update(ctx, padding, pad_len);

	uint8_t bits_buf[8];
	for (int i = 0; i < 8; i++)
		bits_buf[i] = static_cast<uint8_t>(bits >> (i * 8));

	md5_update(ctx, bits_buf, 8);

	std::vector<uint8_t> digest(16);
	for (int i = 0; i < 4; i++) {
		digest[i * 4]     = static_cast<uint8_t>(ctx.state[i]);
		digest[i * 4 + 1] = static_cast<uint8_t>(ctx.state[i] >> 8);
		digest[i * 4 + 2] = static_cast<uint8_t>(ctx.state[i] >> 16);
		digest[i * 4 + 3] = static_cast<uint8_t>(ctx.state[i] >> 24);
	}

	return digest;
}

} // namespace md5_impl

// ── Helper: simple SHA-256 hash ────────────────────────────────────
namespace sha256_impl {

static constexpr uint32_t k[64] = {
	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
	0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
	0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
	0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
	0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
	0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static uint32_t rotr(uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); }
static uint32_t ch(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (~x & z); }
static uint32_t maj(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (x & z) ^ (y & z); }
static uint32_t sigma0(uint32_t x) { return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22); }
static uint32_t sigma1(uint32_t x) { return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25); }
static uint32_t gamma0(uint32_t x) { return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3); }
static uint32_t gamma1(uint32_t x) { return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10); }

static std::string sha256(const std::vector<uint8_t>& data) {
	uint32_t h[8] = {
		0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
		0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
	};

	// Pre-processing: padding
	uint64_t bit_len = static_cast<uint64_t>(data.size()) * 8;
	std::vector<uint8_t> msg = data;
	msg.push_back(0x80);
	while ((msg.size() % 64) != 56)
		msg.push_back(0x00);

	for (int i = 7; i >= 0; i--)
		msg.push_back(static_cast<uint8_t>(bit_len >> (i * 8)));

	// Process each 512-bit block
	for (size_t offset = 0; offset < msg.size(); offset += 64) {
		uint32_t w[64];
		for (int i = 0; i < 16; i++)
			w[i] = (static_cast<uint32_t>(msg[offset + i * 4]) << 24) |
			       (static_cast<uint32_t>(msg[offset + i * 4 + 1]) << 16) |
			       (static_cast<uint32_t>(msg[offset + i * 4 + 2]) << 8) |
			       static_cast<uint32_t>(msg[offset + i * 4 + 3]);

		for (int i = 16; i < 64; i++)
			w[i] = gamma1(w[i - 2]) + w[i - 7] + gamma0(w[i - 15]) + w[i - 16];

		uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
		uint32_t e = h[4], f = h[5], g = h[6], hh = h[7];

		for (int i = 0; i < 64; i++) {
			uint32_t t1 = hh + sigma1(e) + ch(e, f, g) + k[i] + w[i];
			uint32_t t2 = sigma0(a) + maj(a, b, c);
			hh = g; g = f; f = e; e = d + t1;
			d = c; c = b; b = a; a = t1 + t2;
		}

		h[0] += a; h[1] += b; h[2] += c; h[3] += d;
		h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
	}

	// Produce hex digest
	std::string result;
	result.reserve(64);
	static constexpr char hex_chars[] = "0123456789abcdef";
	for (int i = 0; i < 8; i++) {
		for (int j = 3; j >= 0; j--) {
			uint8_t byte = static_cast<uint8_t>(h[i] >> (j * 8));
			result.push_back(hex_chars[(byte >> 4) & 0x0F]);
			result.push_back(hex_chars[byte & 0x0F]);
		}
	}

	return result;
}

} // namespace sha256_impl

// ── Helper: ISO 8601 timestamp from milliseconds ───────────────────
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

// ── Helper: parse URL into host/port/path ──────────────────────────
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

// ── Helper: trim whitespace ────────────────────────────────────────
static std::string trim(const std::string& s) {
	auto start = s.find_first_not_of(" \t\r\n");
	if (start == std::string::npos)
		return "";
	auto end = s.find_last_not_of(" \t\r\n");
	return s.substr(start, end - start + 1);
}

// ── Helper: split string ───────────────────────────────────────────
static std::vector<std::string> split(const std::string& s, char delimiter) {
	std::vector<std::string> result;
	std::istringstream stream(s);
	std::string token;
	while (std::getline(stream, token, delimiter))
		result.push_back(token);
	return result;
}

// ── Public API ─────────────────────────────────────────────────────

HttpResponse https_request(const std::string& url,
                           const std::string& method,
                           const std::unordered_map<std::string, std::string>& headers,
                           const std::string& body) {
	auto parsed = parse_url(url);

	httplib::Headers hdr;
	for (const auto& [key, value] : headers)
		hdr.emplace(key, value);

	httplib::Result res;

	if (parsed.scheme == "https") {
		httplib::SSLClient cli(parsed.host, parsed.port);
		cli.set_connection_timeout(30);
		cli.set_read_timeout(60);

		if (method == "POST")
			res = cli.Post(parsed.path, hdr, body, "application/octet-stream");
		else
			res = cli.Get(parsed.path, hdr);
	} else {
		httplib::Client cli(parsed.host, parsed.port);
		cli.set_connection_timeout(30);
		cli.set_read_timeout(60);

		if (method == "POST")
			res = cli.Post(parsed.path, hdr, body, "application/octet-stream");
		else
			res = cli.Get(parsed.path, hdr);
	}

	HttpResponse response;
	if (res) {
		response.status = res->status;
		response.ok = (res->status >= 200 && res->status < 300);
		response.data = res->body;
	}

	return response;
}

JsonPostResponse json_post(const std::string& url, const nlohmann::json& payload, const std::string& user_agent) {
	std::string body = payload.dump();

	auto parsed = parse_url(url);

	httplib::Headers hdr;
	hdr.emplace("Content-Type", "application/json");
	hdr.emplace("User-Agent", user_agent);
	hdr.emplace("Content-Length", std::to_string(body.size()));

	httplib::Result res;

	if (parsed.scheme == "https") {
		httplib::SSLClient cli(parsed.host, parsed.port);
		cli.set_connection_timeout(30);
		cli.set_read_timeout(60);
		res = cli.Post(parsed.path, hdr, body, "application/json");
	} else {
		httplib::Client cli(parsed.host, parsed.port);
		cli.set_connection_timeout(30);
		cli.set_read_timeout(60);
		res = cli.Post(parsed.path, hdr, body, "application/json");
	}

	JsonPostResponse response;
	if (res) {
		response.status = res->status;
		response.ok = (res->status >= 200 && res->status < 300);

		if (response.ok) {
			try {
				response.response_json = nlohmann::json::parse(res->body);
			} catch (...) {}
		}
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
	// JS: crypto.randomBytes(16).toString('hex')
	std::string boundary = random_hex(16);

	for (int64_t offset = 0; offset < static_cast<int64_t>(buffer.size()); offset += CHUNK_SIZE) {
		int64_t end = (std::min)(offset + CHUNK_SIZE, static_cast<int64_t>(buffer.size()));
		std::vector<uint8_t> chunk(buffer.begin() + offset, buffer.begin() + end);
		std::vector<uint8_t> body = build_multipart(boundary, chunk, offset);

		auto parsed = parse_url(url);

		std::string content_type = std::format("multipart/form-data; boundary={}", boundary);

		httplib::Headers hdr;
		hdr.emplace("Content-Type", content_type);
		hdr.emplace("Content-Length", std::to_string(body.size()));

		std::string body_str(body.begin(), body.end());
		httplib::Result res;

		if (parsed.scheme == "https") {
			httplib::SSLClient cli(parsed.host, parsed.port);
			cli.set_connection_timeout(30);
			cli.set_read_timeout(60);
			res = cli.Post(parsed.path, hdr, body_str, content_type);
		} else {
			httplib::Client cli(parsed.host, parsed.port);
			cli.set_connection_timeout(30);
			cli.set_read_timeout(60);
			res = cli.Post(parsed.path, hdr, body_str, content_type);
		}

		if (!res || res->status < 200 || res->status >= 300)
			throw std::runtime_error(std::format("upload chunk failed: {}", res ? res->status : 0));
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
	// JS: crypto.createHash('md5') streaming
	std::ifstream ifs(file_path, std::ios::binary);
	if (!ifs)
		throw std::runtime_error(std::format("Cannot open file for hashing: {}", file_path.string()));

	md5_impl::MD5Context ctx;
	md5_impl::md5_init(ctx);

	uint8_t buf[8192];
	while (ifs.read(reinterpret_cast<char*>(buf), sizeof(buf)))
		md5_impl::md5_update(ctx, buf, sizeof(buf));

	if (ifs.gcount() > 0)
		md5_impl::md5_update(ctx, buf, static_cast<size_t>(ifs.gcount()));

	auto digest = md5_impl::md5_final(ctx);
	return to_hex(digest);
}

std::vector<fs::path> find_binaries(const fs::path& flavor_dir) {
	std::vector<fs::path> binaries;

	try {
		for (const auto& entry : fs::directory_iterator(flavor_dir)) {
			if (entry.is_regular_file()) {
				auto name = entry.path().filename().string();
				if (iends_with(name, ".exe")) {
					// JS: entry.name.toLowerCase() !== 'blizzarderror.exe'
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
					if (!iends_with(file_name, ".wdb"))
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
			std::string hash = sha256_impl::sha256(buffer);

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

				// JS: flavor_text.trim().split('\n').pop()?.trim()
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
			// JS: builds.find(b => b.Product === flavor.product)
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
