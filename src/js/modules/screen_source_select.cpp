
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "screen_source_select.h"
#include "../log.h"
#include "../core.h"
#include "../config.h"
#include "../constants.h"
#include "../generics.h"
#include "../install-type.h"
#include "../modules.h"
#include "../casc/casc-source-local.h"
#include "../casc/casc-source-remote.h"
#include "../casc/cdn-resolver.h"
#include "../mpq/mpq-install.h"
#include "../workers/cache-collector.h"
#include "../../app.h"

#include <atomic>
#include <cstring>
#include <format>
#include <algorithm>
#include <future>
#include <limits>
#include <memory>
#include <mutex>
#include <random>
#include <thread>
#include <cmath>
#include <filesystem>
#include <fstream>

#include <imgui.h>
#include <imgui_internal.h>
#include <portable-file-dialogs.h>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <webp/decode.h>
#include <nanosvg.h>
#include <nanosvgrast.h>

namespace screen_source_select {

static float s_cardBorderAnim[3] = { 0.0f, 0.0f, 0.0f };

static ImU32 lerpColor(ImU32 a, ImU32 b, float t) {
	int ra = (a >> IM_COL32_R_SHIFT) & 0xFF, ga = (a >> IM_COL32_G_SHIFT) & 0xFF;
	int ba2 = (a >> IM_COL32_B_SHIFT) & 0xFF, aa = (a >> IM_COL32_A_SHIFT) & 0xFF;
	int rb = (b >> IM_COL32_R_SHIFT) & 0xFF, gb = (b >> IM_COL32_G_SHIFT) & 0xFF;
	int bb = (b >> IM_COL32_B_SHIFT) & 0xFF, ab = (b >> IM_COL32_A_SHIFT) & 0xFF;
	return IM_COL32(
		static_cast<int>(ra + (rb - ra) * t),
		static_cast<int>(ga + (gb - ga) * t),
		static_cast<int>(ba2 + (bb - ba2) * t),
		static_cast<int>(aa + (ab - aa) * t)
	);
}

static GLuint s_texWowLogo = 0;
static GLuint s_texBattlenet = 0;
static GLuint s_texMpq = 0;
static bool s_texturesLoaded = false;

static GLuint loadSvgTexture(const std::filesystem::path& path, int size) {
	NSVGimage* image = nsvgParseFromFile(path.string().c_str(), "px", 96.0f);
	if (!image)
		return 0;

	NSVGrasterizer* rast = nsvgCreateRasterizer();
	if (!rast) {
		nsvgDelete(image);
		return 0;
	}

	float scale = static_cast<float>(size) / std::max(image->width, image->height);
	int w = static_cast<int>(image->width * scale);
	int h = static_cast<int>(image->height * scale);
	if (w <= 0 || h <= 0) {
		nsvgDeleteRasterizer(rast);
		nsvgDelete(image);
		return 0;
	}

	std::vector<unsigned char> pixels(static_cast<size_t>(w) * h * 4, 0);
	nsvgRasterize(rast, image, 0, 0, scale, pixels.data(), w, h, w * 4);

	GLuint tex = 0;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
	glBindTexture(GL_TEXTURE_2D, 0);

	nsvgDeleteRasterizer(rast);
	nsvgDelete(image);
	return tex;
}

static void ensureSourceTextures() {
	if (s_texturesLoaded) return;
	s_texturesLoaded = true;
	std::filesystem::path imgDir = constants::SRC_DIR() / "images";
	s_texWowLogo   = loadSvgTexture(imgDir / "wow_logo.svg", 160);
	s_texBattlenet = loadSvgTexture(imgDir / "import_battlenet.svg", 160);
	s_texMpq       = loadSvgTexture(imgDir / "mpq.svg", 160);
}

static constexpr std::array<const char*, 13> EXPANSION_ICON_FILES = {{
	"icon_classic.webp",
	"icon_tbc.webp",
	"icon_wotlk.webp",
	"icon_cata.webp",
	"icon_mop.webp",
	"icon_wod.webp",
	"icon_legion.webp",
	"icon_bfa.webp",
	"icon_slands.webp",
	"icon_df.webp",
	"icon_tww.webp",
	"icon_midnight.webp",
	"icon_tlt.webp",
}};

static std::unordered_map<int, GLuint> s_expansionIconTextures;

static GLuint loadWebpTexture(const std::filesystem::path& path) {
	std::ifstream file(path, std::ios::binary | std::ios::ate);
	if (!file.is_open())
		return 0;

	auto fileSize = file.tellg();
	if (fileSize <= 0)
		return 0;

	std::vector<uint8_t> fileData(static_cast<size_t>(fileSize));
	file.seekg(0);
	file.read(reinterpret_cast<char*>(fileData.data()), fileSize);
	file.close();

	int w = 0, h = 0;
	uint8_t* pixels = WebPDecodeRGBA(fileData.data(), fileData.size(), &w, &h);
	if (!pixels)
		return 0;

	GLuint tex = 0;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	glBindTexture(GL_TEXTURE_2D, 0);

	WebPFree(pixels);
	return tex;
}

static GLuint getExpansionIconTexture(int expansionId) {
	if (expansionId < 0 || expansionId >= static_cast<int>(EXPANSION_ICON_FILES.size()))
		return 0;

	auto it = s_expansionIconTextures.find(expansionId);
	if (it != s_expansionIconTextures.end())
		return it->second;

	std::filesystem::path path = constants::SRC_DIR() / "images" / "expansion" / EXPANSION_ICON_FILES[static_cast<size_t>(expansionId)];
	GLuint tex = loadWebpTexture(path);
	s_expansionIconTextures[expansionId] = tex;
	if (!tex)
		logging::write(std::format("warning: failed to load expansion icon for expansion {}: {}", expansionId, path.string()));
	return tex;
}

static std::unique_ptr<casc::CASCLocal> casc_local_source;
static std::unique_ptr<casc::CASCRemote> casc_remote_source;

enum class SourceType { None, Local, Remote };
static SourceType active_source_type = SourceType::None;

static std::unique_ptr<std::jthread> cache_worker_thread;

static std::unique_ptr<std::jthread> casc_load_thread;
static std::unique_ptr<std::jthread> source_open_thread;
static std::unique_ptr<std::jthread> cdn_ping_thread;

static std::string generate_uuid() {
	thread_local std::random_device rd;
	thread_local std::mt19937_64 gen(rd());
	std::uniform_int_distribution<uint64_t> dist;

	uint64_t a = dist(gen);
	uint64_t b = dist(gen);

	a = (a & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;
	b = (b & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;

	return std::format("{:08x}-{:04x}-{:04x}-{:04x}-{:012x}",
		static_cast<uint32_t>(a >> 32),
		static_cast<uint16_t>((a >> 16) & 0xFFFF),
		static_cast<uint16_t>(a & 0xFFFF),
		static_cast<uint16_t>(b >> 48),
		b & 0x0000FFFFFFFFFFFFULL);
}

static std::string get_product_tag(const std::string& product) {
	for (const auto& entry : constants::PRODUCTS) {
		if (entry.product == product)
			return std::string(entry.tag);
	}
	return "Unknown";
}

static void set_selected_cdn(const nlohmann::json& region) {
	core::view->selectedCDNRegion = region;
	core::view->lockCDNRegion = true;
	core::view->config["sourceSelectUserRegion"] = region["tag"];
	config::save();
	casc::cdn_resolver::startPreResolution(region["tag"].get<std::string>());
}

void load_install(int index) {
	core::view->availableLocalBuilds = nullptr;
	core::view->availableRemoteBuilds = nullptr;

	if (active_source_type == SourceType::Local && casc_local_source) {
		auto& recent_local = core::view->config["recentLocal"];
		if (!recent_local.is_array())
			recent_local = nlohmann::json::array();

		const std::string& install_path = casc_local_source->dir;
		const auto& build = casc_local_source->builds[static_cast<size_t>(index)];
		std::string product = build.at("Product");

		int pre_index = -1;
		for (size_t i = 0; i < recent_local.size(); i++) {
			if (recent_local[i]["path"] == install_path && recent_local[i]["product"] == product) {
				pre_index = static_cast<int>(i);
				break;
			}
		}

		if (pre_index > -1) {
			if (pre_index > 0) {
				nlohmann::json entry = recent_local[static_cast<size_t>(pre_index)];
				recent_local.erase(static_cast<size_t>(pre_index));
				recent_local.insert(recent_local.begin(), entry);
			}
		} else {
			nlohmann::json entry;
			entry["path"] = install_path;
			entry["product"] = product;
			recent_local.insert(recent_local.begin(), entry);
		}

		while (recent_local.size() > static_cast<size_t>(constants::MAX_RECENT_LOCAL))
			recent_local.erase(recent_local.size() - 1);

		config::save();
	}

	casc_load_thread.reset();

	if (active_source_type == SourceType::Local && casc_local_source) {
		casc::CASCLocal* src = casc_local_source.get();

		casc_load_thread = std::make_unique<std::jthread>([src, index]() {
			try {
				src->load(index);

				core::postToMainThread([src]() {
					core::view->casc = src;

					if (core::view->config.value("allowCacheCollection", false)) {
						if (!core::view->config.contains("machineId") || !core::view->config["machineId"].is_string()
							|| core::view->config["machineId"].get<std::string>().empty()) {
							core::view->config["machineId"] = generate_uuid();
							config::save();
						}

						cache_collector::WorkerConfig wconfig;
						wconfig.install_path = src->dir;
						wconfig.machine_id = core::view->config["machineId"].get<std::string>();
						wconfig.submit_url = std::string(constants::CACHE::SUBMIT_URL);
						wconfig.finalize_url = std::string(constants::CACHE::FINALIZE_URL);
						wconfig.user_agent = constants::USER_AGENT();
						wconfig.state_path = constants::CACHE::STATE_FILE();

						cache_worker_thread.reset();

						cache_worker_thread = std::make_unique<std::jthread>([cfg = std::move(wconfig)]() {
							try {
								cache_collector::collect(cfg, [](const std::string& msg) {
									logging::write(std::format("cache-collector: {}", msg));
								});
							} catch (const std::exception& err) {
								logging::write(std::format("cache-collector error: {}", err.what()));
							}
						});
					}

					core::view->installType = install_type::CASC;
					modules::set_active("tab_home");
				});
			} catch (const std::exception& e) {
				std::string errMsg = e.what();
				core::postToMainThread([errMsg = std::move(errMsg)]() {
					logging::write(std::format("Failed to load CASC: {}", errMsg));
					core::setToast("error", "Unable to initialize CASC. Try repairing your game installation, or seek support.",
						{
							{"View Log", []() { logging::openRuntimeLog(); }}
						}, -1);
					modules::set_active("source_select");
				});
			}
		});
	} else if (active_source_type == SourceType::Remote && casc_remote_source) {
		casc::CASCRemote* src = casc_remote_source.get();

		casc_load_thread = std::make_unique<std::jthread>([src, index]() {
			try {
				src->load(index);

				core::postToMainThread([src]() {
					core::view->casc = src;
					core::view->installType = install_type::CASC;
					modules::set_active("tab_home");
				});
			} catch (const std::exception& e) {
				std::string errMsg = e.what();
				core::postToMainThread([errMsg = std::move(errMsg)]() {
					logging::write(std::format("Failed to load CASC: {}", errMsg));
					core::setToast("error", "Unable to initialize CASC. Try repairing your game installation, or seek support.",
						{
							{"View Log", []() { logging::openRuntimeLog(); }}
						}, -1);
					modules::set_active("source_select");
				});
			}
		});
	}
}

void open_local_install(const std::string& install_path, const std::string& product) {
	core::hideToast();

	casc_load_thread.reset();
	source_open_thread.reset();

	auto& recent_local = core::view->config["recentLocal"];
	if (!recent_local.is_array()) {
		recent_local = nlohmann::json::array();
		config::save();
	}

	casc_local_source = std::make_unique<casc::CASCLocal>(install_path);
	casc_remote_source.reset();
	active_source_type = SourceType::Local;
	casc::CASCLocal* src = casc_local_source.get();

	source_open_thread = std::make_unique<std::jthread>([src, install_path, product]() {
		try {
			src->init();

			if (!product.empty()) {
				int build_index = -1;
				for (size_t i = 0; i < src->builds.size(); i++) {
					if (src->builds[i].count("Product") && src->builds[i].at("Product") == product) {
						build_index = static_cast<int>(i);
						break;
					}
				}

				core::postToMainThread([src, build_index]() {
					if (active_source_type == SourceType::Local && casc_local_source && casc_local_source.get() == src)
						load_install(build_index);
				});
				return;
			}

			auto product_list = src->getProductList();
			core::postToMainThread([src, product_list = std::move(product_list)]() mutable {
				if (!(active_source_type == SourceType::Local && casc_local_source && casc_local_source.get() == src))
					return;

				core::view->availableLocalBuilds = nlohmann::json::array();
				for (const auto& entry : product_list) {
					nlohmann::json obj;
					obj["label"] = entry.label;
					obj["expansionId"] = entry.expansionId;
					obj["buildIndex"] = entry.buildIndex;
					core::view->availableLocalBuilds.push_back(obj);
				}
				core::view->sourceSelectShowBuildSelect = true;
			});
		} catch (const std::exception& e) {
			std::string err = e.what();
			core::postToMainThread([install_path, product, err = std::move(err)]() {
				core::setToast("error", std::format("It looks like {} is not a valid World of Warcraft installation.", install_path), {}, -1);
				logging::write(std::format("Failed to initialize local CASC source: {}", err));

				auto& recent_local = core::view->config["recentLocal"];
				if (recent_local.is_array()) {
					for (int i = static_cast<int>(recent_local.size()) - 1; i >= 0; i--) {
						const auto& entry = recent_local[static_cast<size_t>(i)];
						if (entry["path"] == install_path && (product.empty() || entry.value("product", "") == product))
							recent_local.erase(static_cast<size_t>(i));
					}
					config::save();
				}
			});
		}
	});
}

void open_legacy_install(const std::string& install_path) {
	core::hideToast();

	source_open_thread.reset();

	core::view->mpq = std::make_unique<mpq::MPQInstall>(install_path);

	core::showLoadingScreen(2, "Loading Legacy Installation");

	mpq::MPQInstall* src = core::view->mpq.get();

	source_open_thread = std::make_unique<std::jthread>([src, install_path]() {
		try {
			src->loadInstall();

			core::postToMainThread([src, install_path]() {
				if (!core::view->mpq || core::view->mpq.get() != src)
					return;

				auto& recent_legacy = core::view->config["recentLegacy"];
				if (!recent_legacy.is_array()) {
					recent_legacy = nlohmann::json::array();
					config::save();
				}

				int pre_index = -1;
				for (size_t i = 0; i < recent_legacy.size(); i++) {
					if (recent_legacy[i]["path"] == install_path) {
						pre_index = static_cast<int>(i);
						break;
					}
				}

				if (pre_index > -1) {
					if (pre_index > 0) {
						nlohmann::json entry = recent_legacy[static_cast<size_t>(pre_index)];
						recent_legacy.erase(static_cast<size_t>(pre_index));
						recent_legacy.insert(recent_legacy.begin(), entry);
					}
				} else {
					nlohmann::json entry;
					entry["path"] = install_path;
					recent_legacy.insert(recent_legacy.begin(), entry);
				}

				while (recent_legacy.size() > static_cast<size_t>(constants::MAX_RECENT_LOCAL))
					recent_legacy.erase(recent_legacy.size() - 1);

				config::save();

				core::view->installType = install_type::MPQ;
				modules::set_active("legacy_tab_home");
				core::hideLoadingScreen();
			});
		} catch (const std::exception& e) {
			std::string err = e.what();
			core::postToMainThread([install_path, err = std::move(err)]() {
				core::hideLoadingScreen();
				core::setToast("error", std::format("Failed to load legacy installation from {}", install_path), {}, -1);
				logging::write(std::format("Failed to initialize legacy MPQ source: {}", err));

				auto& recent_legacy = core::view->config["recentLegacy"];
				if (recent_legacy.is_array()) {
					for (int i = static_cast<int>(recent_legacy.size()) - 1; i >= 0; i--) {
						if (recent_legacy[static_cast<size_t>(i)]["path"] == install_path)
							recent_legacy.erase(static_cast<size_t>(i));
					}
					config::save();
				}

				modules::set_active("source_select");
			});
		}
	});
}

static void init_cdn_pings() {
	auto& regions = core::view->cdnRegions;
	regions = nlohmann::json::array();
	std::string user_region;
	auto it = core::view->config.find("sourceSelectUserRegion");
	if (it != core::view->config.end() && it->is_string())
		user_region = it->get<std::string>();

	if (!user_region.empty())
		core::view->lockCDNRegion = true;

	for (const auto& region : constants::PATCH::REGIONS) {
		std::string cdn_url;
		if (region.tag == "cn") {
			cdn_url = std::string(constants::PATCH::HOST_CHINA);
		} else {
			std::string host_template(constants::PATCH::HOST);
			auto pos = host_template.find("%s");
			if (pos != std::string::npos)
				cdn_url = host_template.substr(0, pos) + std::string(region.tag) + host_template.substr(pos + 2);
			else
				cdn_url = host_template;
		}

		nlohmann::json node;
		node["tag"] = std::string(region.tag);
		node["name"] = std::string(region.name);
		node["url"] = cdn_url;
		node["delay"] = nullptr;
		regions.push_back(node);

		if (std::string(region.tag) == user_region ||
			(user_region.empty() && region.tag == constants::PATCH::DEFAULT_REGION)) {
			core::view->selectedCDNRegion = node;
			casc::cdn_resolver::startPreResolution(std::string(region.tag));
		}
	}

	struct PingTarget { size_t index; std::string url; };
	std::vector<PingTarget> targets;
	for (size_t i = 0; i < regions.size(); i++)
		targets.push_back({i, regions[i]["url"].get<std::string>()});

	cdn_ping_thread.reset();
	cdn_ping_thread = std::make_unique<std::jthread>([targets = std::move(targets)]() {
		std::vector<std::future<void>> ping_tasks;
		ping_tasks.reserve(targets.size());

		for (const auto& target : targets) {
			ping_tasks.emplace_back(std::async(std::launch::async, [target]() {
				int64_t delay = -1;
				try {
					delay = generics::ping(target.url);
				} catch (const std::exception& e) {
					logging::write(std::format("Failed ping to {}: {}", target.url, e.what()));
				}

				core::postToMainThread([index = target.index, delay]() {
					auto& regions = core::view->cdnRegions;
					if (index < regions.size())
						regions[index]["delay"] = delay;
				});
			}));
		}

		for (auto& task : ping_tasks) {
			try {
				task.get();
			} catch (...) {
			}
		}

		core::postToMainThread([]() {
			if (core::view->lockCDNRegion)
				return;

			auto& regions = core::view->cdnRegions;
			nlohmann::json selected_region = core::view->selectedCDNRegion;

			for (const auto& region : regions) {
				if (!region.contains("delay") || region["delay"].is_null() || !region["delay"].is_number())
					continue;

				const int64_t delay = region["delay"].get<int64_t>();
				if (delay < 0)
					continue;

				if (selected_region.contains("delay") && selected_region["delay"].is_number() &&
					delay < selected_region["delay"].get<int64_t>()) {
					core::view->selectedCDNRegion = region;
					casc::cdn_resolver::startPreResolution(region["tag"].get<std::string>());
					selected_region = region;
				}
			}
		});
	});
}

static void click_source_local() {
	if (core::view->isBusy)
		return;

	std::string selected = pfd::select_folder("Select Directory").result();
	if (!selected.empty())
		open_local_install(selected);
}

static void click_source_local_recent(const nlohmann::json& entry) {
	if (core::view->isBusy)
		return;

	open_local_install(entry["path"].get<std::string>(), entry.value("product", ""));
}

static void click_source_remote() {
	if (core::view->isBusy)
		return;

	casc_load_thread.reset();
	source_open_thread.reset();

	std::string tag = core::view->selectedCDNRegion.value("tag", "us");
	auto busy_lock = std::make_shared<BusyLock>(core::create_busy_lock());

	casc_remote_source = std::make_unique<casc::CASCRemote>(tag);
	casc_local_source.reset();
	active_source_type = SourceType::Remote;
	casc::CASCRemote* src = casc_remote_source.get();

	source_open_thread = std::make_unique<std::jthread>([src, tag, busy_lock = std::move(busy_lock)]() mutable {
		try {
			src->init();

			auto product_list = src->getProductList();
			if (product_list.empty())
				throw std::runtime_error("No builds available.");

			core::postToMainThread([src, product_list = std::move(product_list), busy_lock = std::move(busy_lock)]() mutable {
				if (!(active_source_type == SourceType::Remote && casc_remote_source && casc_remote_source.get() == src)) {
					busy_lock.reset();
					return;
				}

				core::view->availableRemoteBuilds = nlohmann::json::array();
				for (const auto& entry : product_list) {
					nlohmann::json obj;
					obj["label"] = entry.label;
					obj["expansionId"] = entry.expansionId;
					obj["buildIndex"] = entry.buildIndex;
					core::view->availableRemoteBuilds.push_back(obj);
				}
				core::view->sourceSelectShowBuildSelect = true;
				busy_lock.reset();
			});
		} catch (const std::exception& e) {
			std::string err = e.what();
			core::postToMainThread([tag, err = std::move(err), busy_lock = std::move(busy_lock)]() mutable {
				std::string upper_tag = tag;
				std::transform(upper_tag.begin(), upper_tag.end(), upper_tag.begin(), ::toupper);
				core::setToast("error", std::format("There was an error connecting to Blizzard's {} CDN, try another region!", upper_tag), {}, -1);
				logging::write(std::format("Failed to initialize remote CASC source: {}", err));
				busy_lock.reset();
			});
		}
	});
}

static void click_source_legacy() {
	if (core::view->isBusy)
		return;

	std::string selected = pfd::select_folder("Select Directory").result();
	if (!selected.empty())
		open_legacy_install(selected);
}

static void click_source_legacy_recent(const nlohmann::json& entry) {
	if (core::view->isBusy)
		return;

	open_legacy_install(entry["path"].get<std::string>());
}

static void click_source_build(int index) {
	if (core::view->isBusy)
		return;

	load_install(index);
}

static void click_return_to_source_select() {
	core::view->availableLocalBuilds = nullptr;
	core::view->availableRemoteBuilds = nullptr;
	core::view->sourceSelectShowBuildSelect = false;
}

static void drawDashedRoundedRect(ImDrawList* draw, ImVec2 p_min, ImVec2 p_max, ImU32 color, float rounding, float thickness, float dash_len, float gap_len) {
	draw->PathClear();
	draw->PathRect(p_min, p_max, rounding, 0);
	std::vector<ImVec2> pts(draw->_Path.Data, draw->_Path.Data + draw->_Path.Size);
	draw->PathClear();

	if (pts.size() < 2) return;

	float accumulated = 0.0f;
	bool drawing = true;
	for (size_t i = 0; i < pts.size(); ++i) {
		size_t next = (i + 1) % pts.size();
		ImVec2 a = pts[i];
		ImVec2 b = pts[next];
		float dx = b.x - a.x;
		float dy = b.y - a.y;
		float seg_len = std::sqrt(dx * dx + dy * dy);
		if (seg_len < 0.001f) continue;

		float seg_offset = 0.0f;
		while (seg_offset < seg_len) {
			float remain = drawing ? (dash_len - accumulated) : (gap_len - accumulated);
			float advance = std::min(remain, seg_len - seg_offset);
			if (drawing) {
				float t0 = seg_offset / seg_len;
				float t1 = (seg_offset + advance) / seg_len;
				ImVec2 from(a.x + dx * t0, a.y + dy * t0);
				ImVec2 to(a.x + dx * t1, a.y + dy * t1);
				draw->AddLine(from, to, color, thickness);
			}
			accumulated += advance;
			seg_offset += advance;
			float target = drawing ? dash_len : gap_len;
			if (accumulated >= target - 0.01f) {
				drawing = !drawing;
				accumulated = 0.0f;
			}
		}
	}
}

void mounted() {
	bool config_changed = false;
	if (!core::view->config.contains("recentLocal") || !core::view->config["recentLocal"].is_array()) {
		core::view->config["recentLocal"] = nlohmann::json::array();
		config_changed = true;
	}

	if (!core::view->config.contains("recentLegacy") || !core::view->config["recentLegacy"].is_array()) {
		core::view->config["recentLegacy"] = nlohmann::json::array();
		config_changed = true;
	}

	if (config_changed)
		config::save();

	init_cdn_pings();
}

void render() {
	ensureSourceTextures();

	ImVec2 content_size = ImGui::GetContentRegionAvail();
	float vp_height = ImGui::GetMainViewport()->WorkSize.y;
	bool compact = (vp_height <= 799.0f);
	bool build_compact = (vp_height <= 849.0f);

	float card_width    = 700.0f;
	float card_min_h    = compact ? 60.0f : 120.0f;
	float card_padding  = compact ? 15.0f : 30.0f;
	float card_pad_x    = compact ? 20.0f : 30.0f;
	float card_gap      = compact ? 15.0f : 30.0f;
	float icon_size     = compact ? 50.0f : 80.0f;
	float title_size    = compact ? 18.0f : 22.0f;
	float subtitle_size = compact ? 14.0f : 16.0f;
	float link_size     = compact ? 13.0f : 15.0f;
	float content_gap   = compact ? 4.0f : 8.0f;
	float border_radius = 15.0f;
	float border_thick  = 3.0f;

	ImFont* bold_font = app::theme::getBoldFont();
	ImDrawList* draw = ImGui::GetWindowDrawList();

	if (!core::view->sourceSelectShowBuildSelect) {
		const int num_cards = 3;

		struct CardInfo {
			GLuint icon_tex;
			const char* title;
			const char* subtitle;
			std::string link_text;
			bool has_link;
			int card_id;
		};

		CardInfo cards[3];

		cards[0].icon_tex = s_texWowLogo;
		cards[0].title = "Open Local Installation (Recommended)";
		cards[0].subtitle = "Explore a locally installed World of Warcraft installation on your machine";
		cards[0].has_link = false;
		cards[0].card_id = 0;
		if (core::view->config.contains("recentLocal") && core::view->config["recentLocal"].is_array()
			&& !core::view->config["recentLocal"].empty()) {
			const auto& recent = core::view->config["recentLocal"][0];
			std::string path = recent.value("path", std::string(""));
			std::string product = recent.value("product", std::string(""));
			std::string tag = get_product_tag(product);
			cards[0].link_text = std::format("Last Opened: {} ({})", path, tag);
			cards[0].has_link = true;
		}

		cards[1].icon_tex = s_texBattlenet;
		cards[1].title = "Use Battle.net CDN";
		cards[1].subtitle = "Explore available builds without installation directly from the Battle.net servers";
		cards[1].has_link = false;
		cards[1].card_id = 1;

		cards[2].icon_tex = s_texMpq;
		cards[2].title = "Open Legacy Installation";
		cards[2].subtitle = "Explore a legacy MPQ-based installation on your machine";
		cards[2].has_link = false;
		cards[2].card_id = 2;
		if (core::view->config.contains("recentLegacy") && core::view->config["recentLegacy"].is_array()
			&& !core::view->config["recentLegacy"].empty()) {
			const auto& recent = core::view->config["recentLegacy"][0];
			std::string path = recent.value("path", std::string(""));
			cards[2].link_text = std::format("Last Opened: {}", path);
			cards[2].has_link = true;
		}

		auto calcCardHeight = [&](const CardInfo& card) -> float {
			float text_width = card_width - card_pad_x * 2 - icon_size - card_gap;
			if (text_width < 100.0f) text_width = 100.0f;

			float h = card_padding * 2;
			h += title_size;
			h += content_gap;

			ImVec2 subtitle_sz = ImGui::CalcTextSize(card.subtitle, nullptr, false, text_width);
			h += subtitle_sz.y;

			if (card.has_link || card.card_id == 1) {
				h += 5.0f;
				h += link_size;
			}

			return std::max(h, std::max(card_min_h, card_padding * 2 + icon_size));
		};

		float total_height = 0.0f;
		float card_heights[3];
		for (int i = 0; i < num_cards; ++i) {
			card_heights[i] = calcCardHeight(cards[i]);
			total_height += card_heights[i];
		}
		total_height += card_gap * (num_cards - 1);

		float start_y = ImGui::GetCursorPosY() + (content_size.y - total_height) * 0.5f;
		if (start_y < ImGui::GetCursorPosY()) start_y = ImGui::GetCursorPosY();

		float start_x = ImGui::GetCursorPosX() + (content_size.x - card_width) * 0.5f;
		if (start_x < ImGui::GetCursorPosX()) start_x = ImGui::GetCursorPosX();

		float cur_y = start_y;

		for (int ci = 0; ci < num_cards; ++ci) {
			const auto& card = cards[ci];
			float card_h = card_heights[ci];

			ImGui::SetCursorPos(ImVec2(start_x, cur_y));
			std::string btn_id = std::format("##source_card_{}", ci);
			ImGui::SetNextItemAllowOverlap();
			bool clicked = ImGui::InvisibleButton(btn_id.c_str(), ImVec2(card_width, card_h));
			bool hovered = ImGui::IsItemHovered();

			if (hovered)
				ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

			ImVec2 card_min = ImGui::GetItemRectMin();
			ImVec2 card_max = ImGui::GetItemRectMax();

			float& anim = s_cardBorderAnim[ci];
			float dt = ImGui::GetIO().DeltaTime;
			float anim_speed = 1.0f / 0.2f;
			if (hovered)
				anim = std::min(anim + dt * anim_speed, 1.0f);
			else
				anim = std::max(anim - dt * anim_speed, 0.0f);
			ImU32 border_color = lerpColor(ImGui::GetColorU32(ImGuiCol_TextDisabled), ImGui::GetColorU32(ImGuiCol_Text), anim);
			drawDashedRoundedRect(draw, card_min, card_max, border_color, border_radius, border_thick, 8.0f, 6.0f);

			float icon_y = card_min.y + (card_h - icon_size) * 0.5f;
			float icon_x = card_min.x + card_pad_x;
			if (card.icon_tex) {
				draw->AddImage(ImTextureRef(static_cast<ImTextureID>(card.icon_tex)),
					ImVec2(icon_x, icon_y),
					ImVec2(icon_x + icon_size, icon_y + icon_size));
			}

			float text_x = icon_x + icon_size + card_gap;
			float text_width = card_max.x - card_pad_x - text_x;
			if (text_width < 50.0f) text_width = 50.0f;

			float text_block_h = title_size + content_gap;
			{
				ImVec2 subtitle_sz = ImGui::CalcTextSize(card.subtitle, nullptr, false, text_width);
				text_block_h += subtitle_sz.y;
			}
			if (card.has_link || card.card_id == 1)
				text_block_h += 5.0f + link_size;
			float text_y = card_min.y + (card_h - text_block_h) * 0.5f;

			ImGui::SetCursorScreenPos(ImVec2(text_x, text_y));
			ImGui::PushFont(bold_font, title_size);
			ImGui::TextUnformatted(card.title);
			ImGui::PopFont();
			text_y += title_size + content_gap;

			{
				ImGui::SetCursorScreenPos(ImVec2(text_x, text_y));
				ImGui::PushFont(nullptr, subtitle_size);
				ImGui::PushTextWrapPos(text_x + text_width);
				ImGui::TextUnformatted(card.subtitle);
				ImGui::PopTextWrapPos();
				ImGui::PopFont();

				ImVec2 subtitle_extent = ImGui::GetItemRectSize();
				text_y += subtitle_extent.y;
			}

			if (card.card_id == 0 && card.has_link) {
				text_y += 5.0f;
				std::string label_prefix = "Last Opened: ";
				ImVec2 prefix_sz = ImGui::GetFont()->CalcTextSizeA(link_size, FLT_MAX, 0.0f, label_prefix.c_str());
				draw->AddText(ImGui::GetFont(), link_size, ImVec2(text_x, text_y), ImGui::GetColorU32(ImGuiCol_Text), label_prefix.c_str());

				std::string link_part = card.link_text.substr(label_prefix.size());
				ImVec2 link_pos(text_x + prefix_sz.x, text_y);
				ImVec2 link_sz = ImGui::GetFont()->CalcTextSizeA(link_size, FLT_MAX, 0.0f, link_part.c_str());

				ImGui::SetCursorScreenPos(link_pos);
				std::string link_id = std::format("##recent_local_link_{}", ci);
				if (ImGui::InvisibleButton(link_id.c_str(), link_sz)) {
					if (core::view->config.contains("recentLocal") && core::view->config["recentLocal"].is_array()
						&& !core::view->config["recentLocal"].empty()) {
						click_source_local_recent(core::view->config["recentLocal"][0]);
					}
				}
				bool link_hovered = ImGui::IsItemHovered();
				ImU32 link_color = link_hovered ? IM_COL32(159, 241, 161, 255) : IM_COL32(87, 175, 226, 255);
				draw->AddText(ImGui::GetFont(), link_size, link_pos, link_color, link_part.c_str());
			} else if (card.card_id == 1) {
				text_y += 5.0f;
				if (!core::view->selectedCDNRegion.is_null()) {
					std::string region_label = std::format("Region: {} ", core::view->selectedCDNRegion.value("name", std::string("...")));
					ImVec2 region_sz = ImGui::GetFont()->CalcTextSizeA(link_size, FLT_MAX, 0.0f, region_label.c_str());
					draw->AddText(ImGui::GetFont(), link_size, ImVec2(text_x, text_y), ImGui::GetColorU32(ImGuiCol_Text), region_label.c_str());

					const char* change_text = "(Change)";
					ImVec2 change_sz = ImGui::GetFont()->CalcTextSizeA(link_size, FLT_MAX, 0.0f, change_text);
					ImVec2 change_pos(text_x + region_sz.x, text_y);

					ImGui::SetCursorScreenPos(change_pos);
					if (ImGui::InvisibleButton("##cdn_change_link", change_sz)) {
						ImGui::OpenPopup("##cdn_region_menu");
					}
					bool change_hovered = ImGui::IsItemHovered();
					ImU32 change_color = change_hovered ? IM_COL32(159, 241, 161, 255) : IM_COL32(87, 175, 226, 255);
					draw->AddText(ImGui::GetFont(), link_size, change_pos, change_color, change_text);
				}

				ImGui::SetNextWindowSize(ImVec2(260.0f, 0.0f), ImGuiCond_Always);
				if (ImGui::BeginPopup("##cdn_region_menu")) {
					ImDrawList* popup_draw = ImGui::GetWindowDrawList();
					for (auto& region : core::view->cdnRegions) {
						std::string name = region.value("name", std::string("Unknown"));
						int64_t delay = region.value("delay", static_cast<int64_t>(-1));

						if (ImGui::Selectable(std::format("##cdn_region_{}", name).c_str(), false, 0, ImVec2(0, 0))) {
							set_selected_cdn(region);
						}
						ImVec2 item_min = ImGui::GetItemRectMin();
						ImVec2 item_max = ImGui::GetItemRectMax();
						float item_h = item_max.y - item_min.y;
						ImFont* font = ImGui::GetFont();
						float normal_size = ImGui::GetFontSize();
						float delay_font_size = 12.0f;

						ImVec2 name_sz = font->CalcTextSizeA(normal_size, FLT_MAX, 0.0f, name.c_str());
						float text_center_y = item_min.y + (item_h - name_sz.y) * 0.5f;
						popup_draw->AddText(font, normal_size, ImVec2(item_min.x, text_center_y),
							ImGui::GetColorU32(ImGuiCol_Text), name.c_str());

						if (!region["delay"].is_null()) {
							std::string delay_text = delay >= 0 ? std::format(" {}ms", delay) : " N/A";
							ImU32 delay_color = IM_COL32(255, 255, 255, 143);
							ImVec2 delay_sz = font->CalcTextSizeA(delay_font_size, FLT_MAX, 0.0f, delay_text.c_str());
							float delay_y = item_min.y + (item_h - delay_sz.y) * 0.5f;
							popup_draw->AddText(font, delay_font_size, ImVec2(item_min.x + name_sz.x, delay_y),
								delay_color, delay_text.c_str());
						}
					}
					ImGui::EndPopup();
				}
			} else if (card.card_id == 2 && card.has_link) {
				text_y += 5.0f;
				std::string label_prefix = "Last Opened: ";
				ImVec2 prefix_sz = ImGui::GetFont()->CalcTextSizeA(link_size, FLT_MAX, 0.0f, label_prefix.c_str());
				draw->AddText(ImGui::GetFont(), link_size, ImVec2(text_x, text_y), ImGui::GetColorU32(ImGuiCol_Text), label_prefix.c_str());

				std::string link_part = card.link_text.substr(label_prefix.size());
				ImVec2 link_pos(text_x + prefix_sz.x, text_y);
				ImVec2 link_sz = ImGui::GetFont()->CalcTextSizeA(link_size, FLT_MAX, 0.0f, link_part.c_str());

				ImGui::SetCursorScreenPos(link_pos);
				std::string link_id = std::format("##recent_legacy_link_{}", ci);
				if (ImGui::InvisibleButton(link_id.c_str(), link_sz)) {
					if (core::view->config.contains("recentLegacy") && core::view->config["recentLegacy"].is_array()
						&& !core::view->config["recentLegacy"].empty()) {
						click_source_legacy_recent(core::view->config["recentLegacy"][0]);
					}
				}
				bool link_hovered_l = ImGui::IsItemHovered();
				ImU32 link_color_l = link_hovered_l ? IM_COL32(159, 241, 161, 255) : IM_COL32(87, 175, 226, 255);
				draw->AddText(ImGui::GetFont(), link_size, link_pos, link_color_l, link_part.c_str());
			}

			if (clicked) {
				switch (card.card_id) {
					case 0: click_source_local(); break;
					case 1: click_source_remote(); break;
					case 2: click_source_legacy(); break;
				}
			}

			cur_y += card_h + card_gap;
		}
	} else {
		const auto& builds = !core::view->availableLocalBuilds.is_null()
			? core::view->availableLocalBuilds
			: core::view->availableRemoteBuilds;

		int build_count = 0;
		if (!builds.is_null() && builds.is_array())
			build_count = static_cast<int>(builds.size());

		float btn_height = 50.0f;
		float btn_gap = 10.0f;
		float title_h = 28.0f;
		float title_mb = 10.0f;
		float return_mt = 10.0f;
		float return_h = 16.0f;
		float btn_border_radius = 10.0f;

		float btn_min_width = build_compact ? 200.0f : 450.0f;
		float btn_h = build_compact ? 40.0f : btn_height;
		float row_gap = btn_gap;
		int cols = build_compact ? std::max(1, static_cast<int>(content_size.x / (btn_min_width + btn_gap))) : 1;
		int rows = build_compact && build_count > 0 ? (build_count + cols - 1) / cols : build_count;

		float grid_width = build_compact ? (cols * btn_min_width + (cols - 1) * btn_gap) : btn_min_width;
		float total_h = title_h + title_mb + rows * btn_h + (rows > 0 ? (rows - 1) * row_gap : 0) + return_mt + return_h;
		float start_y = ImGui::GetCursorPosY() + (content_size.y - total_h) * 0.5f;
		if (start_y < ImGui::GetCursorPosY()) start_y = ImGui::GetCursorPosY();
		float start_x = ImGui::GetCursorPosX() + (content_size.x - grid_width) * 0.5f;
		if (start_x < ImGui::GetCursorPosX()) start_x = ImGui::GetCursorPosX();

		float cur_y = start_y;

		ImGui::SetCursorPos(ImVec2(start_x, cur_y));
		ImGui::Dummy(ImVec2(grid_width, title_h));
		{
			const char* title_text = "Select Build";
			ImVec2 title_sz = bold_font->CalcTextSizeA(title_h, FLT_MAX, 0.0f, title_text);
			ImVec2 rect_min = ImGui::GetItemRectMin();
			float title_cx = rect_min.x + (grid_width - title_sz.x) * 0.5f;
			draw->AddText(bold_font, title_h, ImVec2(title_cx, rect_min.y), ImGui::GetColorU32(ImGuiCol_Text), title_text);
		}
		cur_y += title_h + title_mb;

		if (!builds.is_null() && builds.is_array()) {
			for (size_t i = 0; i < builds.size(); ++i) {
				const auto& build = builds[i];
				std::string label = build.value("label", std::format("Build {}", i));
				int buildIndex = build.value("buildIndex", static_cast<int>(i));

				float cell_x, cell_y;
				if (build_compact) {
					int col = static_cast<int>(i) % cols;
					int row = static_cast<int>(i) / cols;
					cell_x = start_x + col * (btn_min_width + btn_gap);
					cell_y = cur_y + row * (btn_h + row_gap);
				} else {
					cell_x = start_x;
					cell_y = cur_y;
				}

				ImGui::SetCursorPos(ImVec2(cell_x, cell_y));
				std::string btn_id = std::format("##build_btn_{}", i);
				const bool build_busy = core::view->isBusy;
				if (build_busy) ImGui::BeginDisabled();
				ImGui::SetNextItemAllowOverlap();
				bool btn_clicked = ImGui::InvisibleButton(btn_id.c_str(), ImVec2(btn_min_width, btn_h));
				bool btn_hovered = ImGui::IsItemHovered();

				ImVec2 btn_min = ImGui::GetItemRectMin();
				ImVec2 btn_max = ImGui::GetItemRectMax();

				ImU32 border_color = btn_hovered
					? IM_COL32(34, 181, 73, 255)
					: ImGui::GetColorU32(ImGuiCol_TextDisabled);
				drawDashedRoundedRect(draw, btn_min, btn_max, border_color, btn_border_radius, border_thick, 8.0f, 6.0f);

				int expansionId = build.value("expansionId", 0);
				GLuint iconTex = getExpansionIconTexture(expansionId);
				if (iconTex) {
					float icon_sz = std::min(32.0f, btn_h - 8.0f);
					float icon_x = btn_min.x + 10.0f;
					float icon_y = btn_min.y + (btn_h - icon_sz) * 0.5f;
					ImGui::SetCursorScreenPos(ImVec2(icon_x, icon_y));
					ImGui::Image(static_cast<ImTextureID>(static_cast<uintptr_t>(iconTex)), ImVec2(icon_sz, icon_sz));
				}

				float text_x_off = btn_min.x + 50.0f;
				float text_y_off = btn_min.y + (btn_h - 16.0f) * 0.5f;
				ImGui::SetCursorScreenPos(ImVec2(text_x_off, text_y_off));
				ImGui::PushFont(nullptr, 16.0f);
				ImGui::TextUnformatted(label.c_str());
				ImGui::PopFont();

				if (build_busy) ImGui::EndDisabled();

				if (btn_clicked && !core::view->isBusy)
					click_source_build(buildIndex);

				if (!build_compact)
					cur_y += btn_h + btn_gap;
			}
			if (build_compact && build_count > 0)
				cur_y += rows * (btn_h + row_gap) - row_gap;
		}

		cur_y += return_mt;
		{
			const char* return_text = "Return to Installations";
			ImVec2 return_sz = ImGui::GetFont()->CalcTextSizeA(return_h, FLT_MAX, 0.0f, return_text);
			float return_cx = start_x + (grid_width - return_sz.x) * 0.5f;

			ImGui::SetCursorPos(ImVec2(return_cx, cur_y));
			if (ImGui::InvisibleButton("##return_to_source", return_sz)) {
				click_return_to_source_select();
			}
			bool return_hovered = ImGui::IsItemHovered();
			ImU32 return_color = return_hovered ? ImGui::GetColorU32(ImGuiCol_Text) : IM_COL32(87, 175, 226, 255);
			draw->AddText(ImGui::GetFont(), return_h, ImGui::GetItemRectMin(), return_color, return_text);
		}
	}
}

}
