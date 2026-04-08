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
#include "../casc/casc-source-local.h"
#include "../casc/casc-source-remote.h"
#include "../casc/cdn-resolver.h"
#include "../mpq/mpq-install.h"
#include "../components/file-field.h"

#include <cstring>
#include <format>
#include <algorithm>
#include <future>
#include <memory>
#include <thread>

#include <imgui.h>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

namespace screen_source_select {

// --- File-local state ---

// JS: let casc_source = null;
static std::unique_ptr<casc::CASCLocal> casc_local_source;
static std::unique_ptr<casc::CASCRemote> casc_remote_source;

// Track which type is active.
enum class SourceType { None, Local, Remote };
static SourceType active_source_type = SourceType::None;

// JS: let local_selector = null;
// JS: let legacy_selector = null;
// TODO(conversion): In ImGui, file selectors are triggered via file_field::openDirectoryDialog().

// --- Internal functions ---

// JS: methods.get_product_tag(product)
static std::string get_product_tag(const std::string& product) {
	for (const auto& entry : constants::PRODUCTS) {
		if (entry.product == product)
			return std::string(entry.tag);
	}
	return "Unknown";
}

// JS: methods.set_selected_cdn(region)
static void set_selected_cdn(const nlohmann::json& region) {
	core::view->selectedCDNRegion = region;
	core::view->lockCDNRegion = true;
	core::view->config["sourceSelectUserRegion"] = region["tag"];
	casc::cdn_resolver::startPreResolution(region["tag"].get<std::string>());
}

// JS: methods.load_install(index)
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

	try {
		if (active_source_type == SourceType::Local && casc_local_source) {
			casc_local_source->load(index);

			// JS: if (casc_source instanceof CASCLocal && this.$core.view.config.allowCacheCollection) { ... }
			// JS: Worker for cache-collector.
			// TODO(conversion): Cache collection worker will be wired when background worker system is integrated.
			// The JS code creates a Worker thread for cache collection with workerData:
			//   install_path, machine_id, submit_url, finalize_url, user_agent, state_path.
			// In C++ this would use std::jthread with equivalent parameters.

			core::view->installType = install_type::CASC;
			// JS: this.$modules.tab_home.setActive();
			// TODO(conversion): Module activation will be wired when the module system is integrated.
		} else if (active_source_type == SourceType::Remote && casc_remote_source) {
			casc_remote_source->load(index);

			core::view->installType = install_type::CASC;
			// JS: this.$modules.tab_home.setActive();
			// TODO(conversion): Module activation will be wired when the module system is integrated.
		}
	} catch (const std::exception& e) {
		logging::write(std::format("Failed to load CASC: {}", e.what()));
		nlohmann::json toast_options;
		toast_options["View Log"] = true;
		// JS: 'Visit Support Discord': () => ExternalLinks.open('::DISCORD') // Removed: external-links module deleted
		core::setToast("error", "Unable to initialize CASC. Try repairing your game installation, or seek support.", toast_options, -1);
		// JS: this.$modules.source_select.setActive();
		// TODO(conversion): Module activation will be wired when the module system is integrated.
	}
}

// JS: methods.open_local_install(install_path, product)
void open_local_install(const std::string& install_path, const std::string& product) {
	core::hideToast();

	auto& recent_local = core::view->config["recentLocal"];
	if (!recent_local.is_array())
		recent_local = nlohmann::json::array();

	try {
		casc_local_source = std::make_unique<casc::CASCLocal>(install_path);
		casc_remote_source.reset();
		active_source_type = SourceType::Local;

		casc_local_source->init();

		if (!product.empty()) {
			// Find the build index matching the product.
			int build_index = -1;
			for (size_t i = 0; i < casc_local_source->builds.size(); i++) {
				if (casc_local_source->builds[i].count("Product") &&
					casc_local_source->builds[i].at("Product") == product) {
					build_index = static_cast<int>(i);
					break;
				}
			}
			load_install(build_index);
		} else {
			core::view->availableLocalBuilds = nlohmann::json::array();
			auto product_list = casc_local_source->getProductList();
			for (const auto& entry : product_list) {
				nlohmann::json obj;
				obj["label"] = entry.label;
				obj["expansionId"] = entry.expansionId;
				obj["buildIndex"] = entry.buildIndex;
				core::view->availableLocalBuilds.push_back(obj);
			}
			core::view->sourceSelectShowBuildSelect = true;
		}
	} catch (const std::exception& e) {
		core::setToast("error", std::format("It looks like {} is not a valid World of Warcraft installation.", install_path), nullptr, -1);
		logging::write(std::format("Failed to initialize local CASC source: {}", e.what()));

		// Remove matching entries from recent list.
		for (int i = static_cast<int>(recent_local.size()) - 1; i >= 0; i--) {
			const auto& entry = recent_local[static_cast<size_t>(i)];
			if (entry["path"] == install_path && (product.empty() || entry.value("product", "") == product))
				recent_local.erase(static_cast<size_t>(i));
		}
	}
}

// JS: methods.open_legacy_install(install_path)
void open_legacy_install(const std::string& install_path) {
	core::hideToast();

	try {
		// JS: this.$core.view.mpq = new MPQInstall(install_path);
		// TODO(conversion): MPQ source will be wired when AppState.mpq is integrated.
		// core::view->mpq = std::make_unique<mpq::MPQInstall>(install_path);

		core::showLoadingScreen(2, "Loading Legacy Installation");

		// JS: await this.$core.view.mpq.loadInstall();
		// TODO(conversion): MPQ loadInstall will be wired when AppState.mpq is integrated.
		// core::view->mpq->loadInstall();

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
		// JS: this.$modules.legacy_tab_home.setActive();
		// TODO(conversion): Module activation will be wired when the module system is integrated.
		core::hideLoadingScreen();
	} catch (const std::exception& e) {
		core::hideLoadingScreen();
		core::setToast("error", std::format("Failed to load legacy installation from {}", install_path), nullptr, -1);
		logging::write(std::format("Failed to initialize legacy MPQ source: {}", e.what()));

		auto& recent_legacy = core::view->config["recentLegacy"];
		if (recent_legacy.is_array()) {
			for (int i = static_cast<int>(recent_legacy.size()) - 1; i >= 0; i--) {
				if (recent_legacy[static_cast<size_t>(i)]["path"] == install_path)
					recent_legacy.erase(static_cast<size_t>(i));
			}
		}

		// JS: this.$modules.source_select.setActive();
		// TODO(conversion): Module activation will be wired when the module system is integrated.
	}
}

// JS: methods.init_cdn_pings()
static void init_cdn_pings() {
	auto& regions = core::view->cdnRegions;
	std::string user_region = core::view->config.value("sourceSelectUserRegion", std::string{});

	if (!user_region.empty())
		core::view->lockCDNRegion = true;

	for (const auto& region : constants::PATCH::REGIONS) {
		std::string cdn_url;
		if (region.tag == "cn") {
			cdn_url = std::string(constants::PATCH::HOST_CHINA);
		} else {
			// JS: cdn_url = util.format(constants.PATCH.HOST, region.tag);
			// HOST is "https://%s.version.battle.net/" — insert region tag.
			cdn_url = std::format("https://{}.version.battle.net/", std::string(region.tag));
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

	// Ping all regions asynchronously.
	// JS: Promise.all(pings).then(() => { ... auto-select fastest region ... });
	// TODO(conversion): CDN ping will be wired when async tasks/main-loop integration is complete.
	// For each region, call generics::ping(cdn_url) in a background thread,
	// update node["delay"] with the result, and refresh core::view->cdnRegions.
	// After all pings, if !lockCDNRegion, select the fastest region.
}

// JS: methods.click_source_local()
static void click_source_local() {
	if (core::view->isBusy)
		return;

	// JS: local_selector.value = ''; local_selector.click();
	// In C++, open a native directory picker dialog.
	std::string selected = file_field::openDirectoryDialog();
	if (!selected.empty())
		open_local_install(selected);
}

// JS: methods.click_source_local_recent(entry)
static void click_source_local_recent(const nlohmann::json& entry) {
	if (core::view->isBusy)
		return;

	open_local_install(entry["path"].get<std::string>(), entry.value("product", ""));
}

// JS: methods.click_source_remote()
static void click_source_remote() {
	if (core::view->isBusy)
		return;

	BusyLock _lock = core::create_busy_lock();
	std::string tag = core::view->selectedCDNRegion.value("tag", "us");

	try {
		casc_remote_source = std::make_unique<casc::CASCRemote>(tag);
		casc_local_source.reset();
		active_source_type = SourceType::Remote;

		casc_remote_source->init();

		if (casc_remote_source->builds.empty())
			throw std::runtime_error("No builds available.");

		core::view->availableRemoteBuilds = nlohmann::json::array();
		auto product_list = casc_remote_source->getProductList();
		for (const auto& entry : product_list) {
			nlohmann::json obj;
			obj["label"] = entry.label;
			obj["expansionId"] = entry.expansionId;
			obj["buildIndex"] = entry.buildIndex;
			core::view->availableRemoteBuilds.push_back(obj);
		}
		core::view->sourceSelectShowBuildSelect = true;
	} catch (const std::exception& e) {
		std::string upper_tag = tag;
		std::transform(upper_tag.begin(), upper_tag.end(), upper_tag.begin(), ::toupper);
		core::setToast("error", std::format("There was an error connecting to Blizzard's {} CDN, try another region!", upper_tag), nullptr, -1);
		logging::write(std::format("Failed to initialize remote CASC source: {}", e.what()));
	}
}

// JS: methods.click_source_legacy()
static void click_source_legacy() {
	if (core::view->isBusy)
		return;

	// JS: legacy_selector.value = ''; legacy_selector.click();
	// In C++, open a native directory picker dialog.
	std::string selected = file_field::openDirectoryDialog();
	if (!selected.empty())
		open_legacy_install(selected);
}

// JS: methods.click_source_legacy_recent(entry)
static void click_source_legacy_recent(const nlohmann::json& entry) {
	if (core::view->isBusy)
		return;

	open_legacy_install(entry["path"].get<std::string>());
}

// JS: methods.click_source_build(index)
static void click_source_build(int index) {
	if (core::view->isBusy)
		return;

	load_install(index);
}

// JS: methods.click_return_to_source_select()
static void click_return_to_source_select() {
	core::view->availableLocalBuilds = nullptr;
	core::view->availableRemoteBuilds = nullptr;
	core::view->sourceSelectShowBuildSelect = false;
}

// --- Public functions ---

// JS: mounted()
void mounted() {
	// init recent local/legacy arrays if needed.
	// JS: if (!Array.isArray(this.$core.view.config.recentLocal))
	//     this.$core.view.config.recentLocal = [];
	if (!core::view->config.contains("recentLocal") || !core::view->config["recentLocal"].is_array())
		core::view->config["recentLocal"] = nlohmann::json::array();

	// JS: if (!Array.isArray(this.$core.view.config.recentLegacy))
	//     this.$core.view.config.recentLegacy = [];
	if (!core::view->config.contains("recentLegacy") || !core::view->config["recentLegacy"].is_array())
		core::view->config["recentLegacy"] = nlohmann::json::array();

	// JS: create file selectors
	// TODO(conversion): In ImGui, file selectors are triggered via file_field::openDirectoryDialog()
	// instead of NW.js <input type="file" nwdirectory>.

	// init cdn pings.
	init_cdn_pings();
}

void render() {
	// --- Template rendering ---
	// JS: <div id="source-select" v-if="!$core.view.sourceSelectShowBuildSelect">
	if (!core::view->sourceSelectShowBuildSelect) {
		// JS: <div id="source-local" @click="click_source_local">
		//   Source icon, title "Open Local Installation (Recommended)",
		//   subtitle, recent local link
		// TODO(conversion): Full ImGui rendering will be implemented when UI integration is complete.

		// JS: <div id="source-remote" @click="click_source_remote">
		//   Source icon, title "Use Battle.net CDN",
		//   subtitle, CDN region selector with context menu

		// JS: <div id="source-legacy" @click="click_source_legacy">
		//   Source icon, title "Open Legacy Installation",
		//   subtitle, recent legacy link
	} else {
		// JS: <div id="build-select">
		//   <div class="build-select-content">
		//     <div class="build-select-title">Select Build</div>
		//     <div class="build-select-buttons">
		//       For each build in availableLocalBuilds || availableRemoteBuilds:
		//         expansion-icon button with build.label
		//     <span @click="click_return_to_source_select" class="link">Return to Installations</span>
		// TODO(conversion): Full ImGui rendering will be implemented when UI integration is complete.
	}
}

} // namespace screen_source_select
