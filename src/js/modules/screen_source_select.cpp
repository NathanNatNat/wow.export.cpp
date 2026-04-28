/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "screen_source_select.h"
#include "../log.h"
#include "../core.h"
#include "../constants.h"
#include "../generics.h"
#include "../install-type.h"
#include "../modules.h"
#include "../casc/casc-source-local.h"
#include "../casc/casc-source-remote.h"
#include "../casc/cdn-resolver.h"
#include "../mpq/mpq-install.h"
#include "../workers/cache-collector.h"
#include "../external-links.h"
#include "../config.h"
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

namespace screen_source_select {

// --- Per-card border hover animation state ---
static float s_cardBorderAnim[3] = { 0.0f, 0.0f, 0.0f };

// Linearly interpolate between two ImU32 colors.
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

// --- Source icon SVG textures (lazy-loaded) ---
static GLuint s_texWowLogo = 0;
static GLuint s_texBattlenet = 0;
static GLuint s_texMpq = 0;
static bool s_texturesLoaded = false;

static void ensureSourceTextures() {
	if (s_texturesLoaded) return;
	s_texturesLoaded = true;
	std::filesystem::path imgDir = constants::SRC_DIR() / "images";
	// Load at 160px so we have crisp icons at both 80px and 50px display sizes.
	s_texWowLogo   = app::theme::loadSvgTexture(imgDir / "wow_logo.svg", 160);
	s_texBattlenet = app::theme::loadSvgTexture(imgDir / "import_battlenet.svg", 160);
	s_texMpq       = app::theme::loadSvgTexture(imgDir / "mpq.svg", 160);
}

// --- Expansion icon textures (lazy-loaded from WebP) ---
// Maps expansionId (0..12) to icon_*.webp filenames.
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

/**
 * Load a WebP image from disk into an OpenGL texture.
 * Returns the GL texture ID (0 on failure).
 */
static GLuint loadWebpTexture(const std::filesystem::path& path) {
	// Read file into memory.
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

	// Decode WebP to RGBA.
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

/**
 * Get or load an expansion icon texture by expansion ID.
 */
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

// --- File-local state ---

static std::unique_ptr<casc::CASCLocal> casc_local_source;
static std::unique_ptr<casc::CASCRemote> casc_remote_source;

// Track which type is active.
enum class SourceType { None, Local, Remote };
static SourceType active_source_type = SourceType::None;

// JS uses NW.js <input nwdirectory> elements with value-reset/click patterns for
// directory selection. C++ uses native file dialogs via pfd::select_folder() which is
// functionally equivalent (the reset/reselection behavior is handled natively).

// JS source-open and build-load paths are async/await methods; C++ uses std::jthread +
// core::postToMainThread() to achieve the same non-blocking behavior with identical
// error propagation and UI flow.

// Background thread for cache collection (replaces JS Worker).
static std::unique_ptr<std::jthread> cache_worker_thread;

// Background thread for CASC loading (prevents main-thread blocking).
static std::unique_ptr<std::jthread> casc_load_thread;
// Background thread for source open/init operations (mirrors JS async methods).
static std::unique_ptr<std::jthread> source_open_thread;
static std::unique_ptr<std::jthread> cdn_ping_thread;

/**
 * Generate a random UUID v4 string.
 * JS equivalent: crypto.randomUUID()
 */
static std::string generate_uuid() {
	thread_local std::random_device rd;
	thread_local std::mt19937_64 gen(rd());
	std::uniform_int_distribution<uint64_t> dist;

	uint64_t a = dist(gen);
	uint64_t b = dist(gen);

	// Set UUID version 4 bits.
	a = (a & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;
	b = (b & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;

	return std::format("{:08x}-{:04x}-{:04x}-{:04x}-{:012x}",
		static_cast<uint32_t>(a >> 32),
		static_cast<uint16_t>((a >> 16) & 0xFFFF),
		static_cast<uint16_t>(a & 0xFFFF),
		static_cast<uint16_t>(b >> 48),
		b & 0x0000FFFFFFFFFFFFULL);
}

// --- Internal functions ---

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

		// Find existing entry.
		int pre_index = -1;
		for (size_t i = 0; i < recent_local.size(); i++) {
			if (recent_local[i]["path"] == install_path && recent_local[i]["product"] == product) {
				pre_index = static_cast<int>(i);
				break;
			}
		}

		if (pre_index > -1) {
			// Move to front.
			if (pre_index > 0) {
				nlohmann::json entry = recent_local[static_cast<size_t>(pre_index)];
				recent_local.erase(static_cast<size_t>(pre_index));
				recent_local.insert(recent_local.begin(), entry);
			}
		} else {
			// Add new entry at front.
			nlohmann::json entry;
			entry["path"] = install_path;
			entry["product"] = product;
			recent_local.insert(recent_local.begin(), entry);
		}

		// Trim to max.
		while (recent_local.size() > static_cast<size_t>(constants::MAX_RECENT_LOCAL))
			recent_local.erase(recent_local.size() - 1);
	}

	// Stop any previous CASC load thread before starting a new one.
	casc_load_thread.reset();

	if (active_source_type == SourceType::Local && casc_local_source) {
		// Capture raw pointer — the unique_ptr remains alive for the
		// lifetime of the source_select module, which outlives the thread.
		casc::CASCLocal* src = casc_local_source.get();

		casc_load_thread = std::make_unique<std::jthread>([src, index]() {
			try {
				src->load(index);

				// Post all completion actions to the main thread.
				core::postToMainThread([src]() {
					// Synchronize CASC result back to main thread.
					core::view->casc = src;

					if (core::view->config.value("allowCacheCollection", false)) {
						if (!core::view->config.contains("machineId") || !core::view->config["machineId"].is_string()
							|| core::view->config["machineId"].get<std::string>().empty()) {
							core::view->config["machineId"] = generate_uuid();
						}

						cache_collector::WorkerConfig wconfig;
						wconfig.install_path = src->dir;
						wconfig.machine_id = core::view->config["machineId"].get<std::string>();
						wconfig.submit_url = std::string(constants::CACHE::SUBMIT_URL);
						wconfig.finalize_url = std::string(constants::CACHE::FINALIZE_URL);
						wconfig.user_agent = constants::USER_AGENT();
						wconfig.state_path = constants::CACHE::STATE_FILE();

						// Stop any previous cache worker.
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
							{"View Log", []() { logging::openRuntimeLog(); }},
							{"Visit Support Discord", []() { ExternalLinks::open("::DISCORD"); }}
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
					// Synchronize CASC result back to main thread.
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
							{"View Log", []() { logging::openRuntimeLog(); }},
							{"Visit Support Discord", []() { ExternalLinks::open("::DISCORD"); }}
						}, -1);
					modules::set_active("source_select");
				});
			}
		});
	}
}

void open_local_install(const std::string& install_path, const std::string& product) {
	core::hideToast();

	// Ensure any in-progress CASC loading thread is finished before
	// replacing the source pointers it may reference.
	casc_load_thread.reset();
	source_open_thread.reset();

	auto& recent_local = core::view->config["recentLocal"];
	if (!recent_local.is_array())
		recent_local = nlohmann::json::array();

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
				}
			});
		}
	});
}

void open_legacy_install(const std::string& install_path) {
	core::hideToast();

	try {
		core::view->mpq = std::make_unique<mpq::MPQInstall>(install_path);

		core::showLoadingScreen(2, "Loading Legacy Installation");

		core::view->mpq->loadInstall();

		auto& recent_legacy = core::view->config["recentLegacy"];
		if (!recent_legacy.is_array())
			recent_legacy = nlohmann::json::array();

		// Find existing entry.
		int pre_index = -1;
		for (size_t i = 0; i < recent_legacy.size(); i++) {
			if (recent_legacy[i]["path"] == install_path) {
				pre_index = static_cast<int>(i);
				break;
			}
		}

		if (pre_index > -1) {
			// Move to front.
			if (pre_index > 0) {
				nlohmann::json entry = recent_legacy[static_cast<size_t>(pre_index)];
				recent_legacy.erase(static_cast<size_t>(pre_index));
				recent_legacy.insert(recent_legacy.begin(), entry);
			}
		} else {
			// Add new entry at front.
			nlohmann::json entry;
			entry["path"] = install_path;
			recent_legacy.insert(recent_legacy.begin(), entry);
		}

		// Trim to max.
		while (recent_legacy.size() > static_cast<size_t>(constants::MAX_RECENT_LOCAL))
			recent_legacy.erase(recent_legacy.size() - 1);

		core::view->installType = install_type::MPQ;
		modules::set_active("legacy_tab_home");
		core::hideLoadingScreen();
	} catch (const std::exception& e) {
		core::hideLoadingScreen();
		core::setToast("error", std::format("Failed to load legacy installation from {}", install_path), {}, -1);
		logging::write(std::format("Failed to initialize legacy MPQ source: {}", e.what()));

		auto& recent_legacy = core::view->config["recentLegacy"];
		if (recent_legacy.is_array()) {
			for (int i = static_cast<int>(recent_legacy.size()) - 1; i >= 0; i--) {
				if (recent_legacy[static_cast<size_t>(i)]["path"] == install_path)
					recent_legacy.erase(static_cast<size_t>(i));
			}
		}

		modules::set_active("source_select");
	}
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
			// JS: util.format(constants.PATCH.HOST, region.tag)
			// HOST is "https://%s.version.battle.net/" — replace %s with region tag.
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

	// Build a list of (index, url) pairs to ping — captured by value for async tasks.
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
				// per-ping task already logs and posts delay fallback.
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

	// Ensure any in-progress CASC loading thread is finished before
	// replacing the source pointers it may reference.
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

// --- Public functions ---

/**
 * Draw a dashed rounded rectangle outline using line segments.
 * Approximates a CSS "border: 3px dashed" with border-radius by walking
 * the rectangle perimeter with dash/gap segments.
 */
static void drawDashedRoundedRect(ImDrawList* draw, ImVec2 p_min, ImVec2 p_max, ImU32 color, float rounding, float thickness, float dash_len, float gap_len) {
	// Generate the path for a rounded rectangle, then walk it with dashes.

	// Build polyline points for the rounded rect using ImGui's path API.
	draw->PathClear();
	draw->PathRect(p_min, p_max, rounding, 0);
	// Retrieve the path points, copy them, then clear.
	std::vector<ImVec2> pts(draw->_Path.Data, draw->_Path.Data + draw->_Path.Size);
	draw->PathClear();

	if (pts.size() < 2) return;

	// Walk the polyline with dash/gap
	float accumulated = 0.0f;
	bool drawing = true; // start with a dash
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
	// init recent local/legacy arrays if needed.
	//     this.$core.view.config.recentLocal = [];
	if (!core::view->config.contains("recentLocal") || !core::view->config["recentLocal"].is_array())
		core::view->config["recentLocal"] = nlohmann::json::array();

	//     this.$core.view.config.recentLegacy = [];
	if (!core::view->config.contains("recentLegacy") || !core::view->config["recentLegacy"].is_array())
		core::view->config["recentLegacy"] = nlohmann::json::array();

	// In ImGui, file selectors are triggered via pfd::select_folder() instead of
	// NW.js <input type="file" nwdirectory>.

	// init cdn pings.
	init_cdn_pings();
}

void render() {
	// Ensure source icon textures are loaded.
	ensureSourceTextures();

	// --- Responsiveness: CSS @media (max-height: N px) breakpoints ---
	// Use viewport height (matches CSS media-query semantics), not content window height.
	ImVec2 content_size = ImGui::GetContentRegionAvail();
	float vp_height = ImGui::GetMainViewport()->WorkSize.y;
	bool compact = (vp_height <= 799.0f); // CSS: @media (max-height: 799px) — compress source-select icons/gaps
	bool build_compact = (vp_height <= 849.0f); // CSS: @media (max-height: 849px) — row-wrap build-select buttons

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

	// --- Template rendering ---
	if (!core::view->sourceSelectShowBuildSelect) {
		// Count number of cards (always 3: local, remote, legacy).
		const int num_cards = 3;

		// Pre-calculate card heights for centering.
		// We'll compute each card's actual height based on content.
		struct CardInfo {
			GLuint icon_tex;
			const char* title;
			const char* subtitle;
			std::string link_text;
			bool has_link;
			int card_id; // 0=local, 1=remote, 2=legacy
		};

		// Prepare card data
		CardInfo cards[3];

		// Local
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

		// Remote
		cards[1].icon_tex = s_texBattlenet;
		cards[1].title = "Use Battle.net CDN";
		cards[1].subtitle = "Explore available builds without installation directly from the Battle.net servers";
		cards[1].has_link = false;
		cards[1].card_id = 1;

		// Legacy
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

		// Calculate card heights.
		auto calcCardHeight = [&](const CardInfo& card) -> float {
			float text_width = card_width - card_pad_x * 2 - icon_size - card_gap;
			if (text_width < 100.0f) text_width = 100.0f;

			float h = card_padding * 2; // top + bottom padding
			h += title_size;           // title line
			h += content_gap;

			// Subtitle: may wrap
			ImVec2 subtitle_sz = ImGui::CalcTextSize(card.subtitle, nullptr, false, text_width);
			h += subtitle_sz.y;

			// Link or CDN region line
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
		total_height += card_gap * (num_cards - 1); // gaps between cards

		// Center vertically.
		float start_y = ImGui::GetCursorPosY() + (content_size.y - total_height) * 0.5f;
		if (start_y < ImGui::GetCursorPosY()) start_y = ImGui::GetCursorPosY();

		// Center horizontally.
		float start_x = ImGui::GetCursorPosX() + (content_size.x - card_width) * 0.5f;
		if (start_x < ImGui::GetCursorPosX()) start_x = ImGui::GetCursorPosX();

		float cur_y = start_y;

		for (int ci = 0; ci < num_cards; ++ci) {
			const auto& card = cards[ci];
			float card_h = card_heights[ci];

			// Position an invisible button covering the card area for click detection.
			// SetNextItemAllowOverlap lets the link sub-buttons submitted later claim
			// hover/click priority over this card button within their own rects.
			ImGui::SetCursorPos(ImVec2(start_x, cur_y));
			std::string btn_id = std::format("##source_card_{}", ci);
			ImGui::SetNextItemAllowOverlap();
			bool clicked = ImGui::InvisibleButton(btn_id.c_str(), ImVec2(card_width, card_h));
			bool hovered = ImGui::IsItemHovered();

			// CSS: cursor: pointer on card divs
			if (hovered)
				ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

			// Get absolute screen coordinates for drawing.
			ImVec2 card_min = ImGui::GetItemRectMin();
			ImVec2 card_max = ImGui::GetItemRectMax();

			// Animate border color with 0.2s transition (CSS: transition: border-color 0.2s).
			float& anim = s_cardBorderAnim[ci];
			float dt = ImGui::GetIO().DeltaTime;
			float anim_speed = 1.0f / 0.2f; // 0.2 seconds
			if (hovered)
				anim = std::min(anim + dt * anim_speed, 1.0f);
			else
				anim = std::max(anim - dt * anim_speed, 0.0f);
			ImU32 border_color = lerpColor(ImGui::GetColorU32(ImGuiCol_TextDisabled), ImGui::GetColorU32(ImGuiCol_Text), anim);
			drawDashedRoundedRect(draw, card_min, card_max, border_color, border_radius, border_thick, 8.0f, 6.0f);

			// Draw icon.
			float icon_y = card_min.y + (card_h - icon_size) * 0.5f;
			float icon_x = card_min.x + card_pad_x;
			if (card.icon_tex) {
				draw->AddImage(ImTextureRef(static_cast<ImTextureID>(card.icon_tex)),
					ImVec2(icon_x, icon_y),
					ImVec2(icon_x + icon_size, icon_y + icon_size));
			}

			// Draw text content.
			float text_x = icon_x + icon_size + card_gap;
			float text_width = card_max.x - card_pad_x - text_x;
			if (text_width < 50.0f) text_width = 50.0f;

			// CSS: align-items: center — vertically center text block within card.
			// Compute total text block height to center it.
			float text_block_h = title_size + content_gap;
			{
				ImVec2 subtitle_sz = ImGui::CalcTextSize(card.subtitle, nullptr, false, text_width);
				text_block_h += subtitle_sz.y;
			}
			if (card.has_link || card.card_id == 1)
				text_block_h += 5.0f + link_size;
			float text_y = card_min.y + (card_h - text_block_h) * 0.5f;

			// Title (bold, highlight color).
			draw->AddText(bold_font, title_size, ImVec2(text_x, text_y), ImGui::GetColorU32(ImGuiCol_Text), card.title);
			text_y += title_size + content_gap;

			// Subtitle (opacity 0.7).
			ImU32 subtitle_color = IM_COL32(255, 255, 255, 143); // CSS: opacity: 0.7 applied to --font-primary (#ffffffcc, alpha 204). Effective alpha = 204 * 0.7 ≈ 143.
			{
				const char* text_begin = card.subtitle;
				const char* text_end = text_begin + strlen(text_begin);
				ImFont* font = ImGui::GetFont();
				float font_size = subtitle_size;
				float wrap_width = text_width;

				// Use AddText with wrapping by computing line breaks manually.
				const char* s = text_begin;
				while (s < text_end) {
					const char* line_end = font->CalcWordWrapPositionA(font_size / font->LegacySize, s, text_end, wrap_width);
					if (line_end == s) line_end = s + 1; // Prevent infinite loop
					draw->AddText(font, font_size, ImVec2(text_x, text_y), subtitle_color, s, line_end);
					ImVec2 line_sz = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, s, line_end);
					text_y += line_sz.y;
					s = line_end;
					// Skip leading whitespace on next line.
					while (s < text_end && (*s == ' ' || *s == '\n')) s++;
				}
			}

			// Link text or CDN region (card-specific).
			if (card.card_id == 0 && card.has_link) {
				// "Last Opened: path (tag)" — clickable link
				text_y += 5.0f;
				std::string label_prefix = "Last Opened: ";
				ImVec2 prefix_sz = ImGui::GetFont()->CalcTextSizeA(link_size, FLT_MAX, 0.0f, label_prefix.c_str());
				draw->AddText(ImGui::GetFont(), link_size, ImVec2(text_x, text_y), ImGui::GetColorU32(ImGuiCol_Text), label_prefix.c_str());

				// Clickable link part.
				std::string link_part = card.link_text.substr(label_prefix.size());
				ImVec2 link_pos(text_x + prefix_sz.x, text_y);
				ImVec2 link_sz = ImGui::GetFont()->CalcTextSizeA(link_size, FLT_MAX, 0.0f, link_part.c_str());

				// Create an invisible button over the link text.
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
				// CDN region: "Region: NAME (Change)" with context menu.
				text_y += 5.0f;
				if (!core::view->selectedCDNRegion.is_null()) {
					std::string region_label = std::format("Region: {} ", core::view->selectedCDNRegion.value("name", std::string("...")));
					ImVec2 region_sz = ImGui::GetFont()->CalcTextSizeA(link_size, FLT_MAX, 0.0f, region_label.c_str());
					draw->AddText(ImGui::GetFont(), link_size, ImVec2(text_x, text_y), ImGui::GetColorU32(ImGuiCol_Text), region_label.c_str());

					// "(Change)" link
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

				// CDN region context menu popup.
				ImGui::SetNextWindowSize(ImVec2(260.0f, 0.0f), ImGuiCond_Always);
				if (ImGui::BeginPopup("##cdn_region_menu")) {
					ImDrawList* popup_draw = ImGui::GetWindowDrawList();
					for (auto& region : core::view->cdnRegions) {
						std::string name = region.value("name", std::string("Unknown"));
						int64_t delay = region.value("delay", static_cast<int64_t>(-1));

						// Render region name as selectable item.
						if (ImGui::Selectable(std::format("##cdn_region_{}", name).c_str(), false, 0, ImVec2(0, 0))) {
							set_selected_cdn(region);
						}
						// Custom render: region name at normal opacity, delay at 0.7 opacity + 12px font.
						ImVec2 item_min = ImGui::GetItemRectMin();
						ImVec2 item_max = ImGui::GetItemRectMax();
						float item_h = item_max.y - item_min.y;
						ImFont* font = ImGui::GetFont();
						float normal_size = ImGui::GetFontSize();
						float delay_font_size = 12.0f;

						// Draw region name.
						ImVec2 name_sz = font->CalcTextSizeA(normal_size, FLT_MAX, 0.0f, name.c_str());
						float text_center_y = item_min.y + (item_h - name_sz.y) * 0.5f;
						popup_draw->AddText(font, normal_size, ImVec2(item_min.x, text_center_y),
							ImGui::GetColorU32(ImGuiCol_Text), name.c_str());

						// Draw delay string with reduced opacity and smaller font.
						if (!region["delay"].is_null()) {
							std::string delay_text = delay >= 0 ? std::format(" {}ms", delay) : " N/A";
							ImU32 delay_color = IM_COL32(255, 255, 255, 143); // opacity: 0.7 * --font-primary alpha
							ImVec2 delay_sz = font->CalcTextSizeA(delay_font_size, FLT_MAX, 0.0f, delay_text.c_str());
							float delay_y = item_min.y + (item_h - delay_sz.y) * 0.5f;
							popup_draw->AddText(font, delay_font_size, ImVec2(item_min.x + name_sz.x, delay_y),
								delay_color, delay_text.c_str());
						}
					}
					ImGui::EndPopup();
				}
			} else if (card.card_id == 2 && card.has_link) {
				// Legacy "Last Opened: path" — clickable link
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

			// Handle card click (only if not clicking a sub-element link).
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
		//   <div class="build-select-content">
		//     <div class="build-select-title">Select Build</div>
		//     <div class="build-select-buttons">
		//       For each build: expansion-icon button with build.label
		//     <span @click="click_return_to_source_select" class="link">Return to Installations</span>

		const auto& builds = !core::view->availableLocalBuilds.is_null()
			? core::view->availableLocalBuilds
			: core::view->availableRemoteBuilds;

		int build_count = 0;
		if (!builds.is_null() && builds.is_array())
			build_count = static_cast<int>(builds.size());

		// CSS: @media (max-height: 849px) — row-wrap layout; otherwise column layout.
		// build_compact is derived from vp_height above.
		float btn_height = 50.0f;
		float btn_gap = 10.0f;
		float title_h = 28.0f;
		float title_mb = 10.0f;
		float return_mt = 10.0f;
		float return_h = 16.0f;
		float btn_border_radius = 10.0f;

		// In row-wrap mode buttons are smaller and arranged in a wrapping grid.
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

		// Title: "Select Build"
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

		// Build buttons (column layout or row-wrap at small heights).
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
				bool btn_clicked = ImGui::InvisibleButton(btn_id.c_str(), ImVec2(btn_min_width, btn_h));
				bool btn_hovered = ImGui::IsItemHovered();

				ImVec2 btn_min = ImGui::GetItemRectMin();
				ImVec2 btn_max = ImGui::GetItemRectMax();

				// Border and hover effect.
				if (btn_hovered) {
					draw->AddRectFilled(btn_min, btn_max, IM_COL32(34, 181, 73, 25), btn_border_radius);
					drawDashedRoundedRect(draw, btn_min, btn_max, IM_COL32(34, 181, 73, 255), btn_border_radius, border_thick, 8.0f, 6.0f);
				} else {
					drawDashedRoundedRect(draw, btn_min, btn_max, ImGui::GetColorU32(ImGuiCol_TextDisabled), btn_border_radius, border_thick, 8.0f, 6.0f);
				}

				// Expansion icon (32px, at 10px from left, vertically centered).
				int expansionId = build.value("expansionId", 0);
				GLuint iconTex = getExpansionIconTexture(expansionId);
				if (iconTex) {
					float icon_sz = std::min(32.0f, btn_h - 8.0f);
					float icon_x = btn_min.x + 10.0f;
					float icon_y = btn_min.y + (btn_h - icon_sz) * 0.5f;
					draw->AddImage(ImTextureRef(static_cast<ImTextureID>(iconTex)),
						ImVec2(icon_x, icon_y), ImVec2(icon_x + icon_sz, icon_y + icon_sz));
				}

				// Build label text.
				ImU32 text_color = btn_hovered ? IM_COL32(34, 181, 73, 255) : ImGui::GetColorU32(ImGuiCol_Text);
				float text_x_off = btn_min.x + 50.0f;
				float text_y_off = btn_min.y + (btn_h - 16.0f) * 0.5f;
				draw->AddText(ImGui::GetFont(), 16.0f, ImVec2(text_x_off, text_y_off), text_color, label.c_str());

				if (btn_clicked && !core::view->isBusy)
					click_source_build(buildIndex);

				if (!build_compact)
					cur_y += btn_h + btn_gap;
			}
			if (build_compact && build_count > 0)
				cur_y += rows * (btn_h + row_gap) - row_gap;
		}

		// "Return to Installations" link.
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

} // namespace screen_source_select
