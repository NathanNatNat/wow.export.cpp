/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "modules.h"
#include "log.h"
#include "install-type.h"
#include "constants.h"
#include "core.h"

#include <algorithm>
#include <format>
#include <map>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

// Module headers
#include "modules/screen_source_select.h"
#include "modules/screen_settings.h"
#include "modules/tab_home.h"
#include "modules/tab_maps.h"
#include "modules/tab_zones.h"
#include "modules/tab_data.h"
#include "modules/tab_raw.h"
#include "modules/tab_install.h"
#include "modules/tab_text.h"
#include "modules/tab_fonts.h"
#include "modules/tab_videos.h"
#include "modules/tab_models.h"
#include "modules/tab_creatures.h"
#include "modules/tab_decor.h"
#include "modules/tab_audio.h"
#include "modules/tab_items.h"
#include "modules/tab_item_sets.h"
#include "modules/tab_characters.h"
#include "modules/tab_textures.h"
#include "modules/legacy_tab_home.h"
#include "modules/legacy_tab_audio.h"
#include "modules/legacy_tab_textures.h"
#include "modules/legacy_tab_fonts.h"
#include "modules/legacy_tab_files.h"
#include "modules/legacy_tab_data.h"
#include "modules/tab_models_legacy.h"

namespace modules {

// --- File-local state ---

static std::map<std::string, ModuleDef> module_registry;

static std::map<std::string, NavButton> nav_button_map;

static std::map<std::string, ContextMenuOption> context_menu_map;

static ModuleDef* active_module = nullptr;

// Cached sorted vectors for external access
static std::vector<NavButton> sorted_nav_buttons;
static std::vector<ContextMenuOption> sorted_context_menu_options;

#ifdef NDEBUG
static constexpr bool IS_BUNDLED = true;
#else
static constexpr bool IS_BUNDLED = false;
#endif

// components that should not be reloaded. in an ideal world we would support hot-reloading
// these but it was too much effort at the time, so c'est la vie


static void update_nav_buttons() {
	const auto& order = constants::NAV_BUTTON_ORDER;
	std::vector<NavButton> buttons;
	buttons.reserve(nav_button_map.size());

	for (const auto& [name, button] : nav_button_map)
		buttons.push_back(button);

	std::sort(buttons.begin(), buttons.end(), [&order](const NavButton& a, const NavButton& b) {
		int idx_a = -1, idx_b = -1;

		for (size_t i = 0; i < order.size(); i++) {
			if (order[i] == a.module)
				idx_a = static_cast<int>(i);
			if (order[i] == b.module)
				idx_b = static_cast<int>(i);
		}

		if (idx_a == -1 && idx_b == -1)
			return false;

		if (idx_a == -1)
			return false; // a goes after b

		if (idx_b == -1)
			return true;  // a goes before b

		return idx_a < idx_b;
	});

	sorted_nav_buttons = std::move(buttons);

	if (core::view) {
		core::view->modNavButtons.clear();
		for (const auto& btn : sorted_nav_buttons) {
			nlohmann::json j;
			j["module"] = btn.module;
			j["label"] = btn.label;
			j["icon"] = btn.icon;
			j["installTypes"] = btn.installTypes;
			core::view->modNavButtons.push_back(j);
		}
	}
}

static void update_context_menu_options() {
	const auto& order = constants::CONTEXT_MENU_ORDER;
	std::vector<ContextMenuOption> options;
	options.reserve(context_menu_map.size());

	for (const auto& [id, option] : context_menu_map)
		options.push_back(option);

	std::sort(options.begin(), options.end(), [&order](const ContextMenuOption& a, const ContextMenuOption& b) {
		int idx_a = -1, idx_b = -1;

		for (size_t i = 0; i < order.size(); i++) {
			if (order[i] == a.id)
				idx_a = static_cast<int>(i);
			if (order[i] == b.id)
				idx_b = static_cast<int>(i);
		}

		// items not in order go to end
		if (idx_a == -1 && idx_b == -1)
			return false;

		if (idx_a == -1)
			return false;

		if (idx_b == -1)
			return true;

		return idx_a < idx_b;
	});

	sorted_context_menu_options = std::move(options);

	if (core::view) {
		core::view->modContextMenuOptions.clear();
		for (const auto& opt : sorted_context_menu_options) {
			nlohmann::json j;
			j["id"] = opt.id;
			j["label"] = opt.label;
			j["icon"] = opt.icon;
			j["dev_only"] = opt.dev_only;
			core::view->modContextMenuOptions.push_back(j);
		}
	}
}

// error handling, register() call) is handled during initialize().
static void wrap_module(ModuleDef& mod) {
	std::string display_label = mod.name;

	if (mod.registerModule) {
		// The registerModule function for tabs calls register_nav_button internally.
		// We capture the display_label for error messages.
		mod.registerModule();
	}

	// wrap initialize() with idempotency guard, error handling, and activated() retry
	if (mod.initialize) {
		auto original_initialize = mod.initialize;

		mod.initialize = [&mod, original_initialize, display_label]() {
			if (mod._tab_initialized || mod._tab_initializing)
				return;

			mod._tab_initializing = true;

			try {
				original_initialize();
				mod._tab_initialized = true;
			} catch (const std::exception& error) {
				core::hideLoadingScreen();
				logging::write(std::format("Failed to initialize {} tab: {}", display_label, error.what()));
				core::setToast("error", std::format("Failed to initialize {} tab. Check the log for details.", display_label),
				               { {"View Log", []() { logging::openRuntimeLog(); }} }, -1);
				go_to_landing();
			}

			mod._tab_initializing = false;
		};
	}
}

// --- Public API ---

void register_nav_button(const std::string& module_name, const std::string& label,
                         const std::string& icon, uint32_t install_types) {
	NavButton button;
	button.module = module_name;
	button.label = label;
	button.icon = icon;
	button.installTypes = install_types;

	nav_button_map[module_name] = button;
	update_nav_buttons();
	logging::write(std::format("registered nav button for module: {}", module_name));
}

void unregister_nav_button(const std::string& module_name) {
	if (nav_button_map.erase(module_name)) {
		update_nav_buttons();
		logging::write(std::format("unregistered nav button for module: {}", module_name));
	}
}

void register_context_menu_option(const std::string& id, const std::string& label,
                                  const std::string& icon, std::function<void()> action) {
	ContextMenuOption option;
	option.id = id;
	option.label = label;
	option.icon = icon;
	option.handler = action;

	context_menu_map[id] = option;
	update_context_menu_options();
	logging::write(std::format("registered context menu option: {}", id));
}

void unregister_context_menu_option(const std::string& id) {
	if (context_menu_map.erase(id)) {
		update_context_menu_options();
		logging::write(std::format("unregistered context menu option: {}", id));
	}
}

void registerContextMenuOption(const std::string& id, const std::string& label,
                               const std::string& icon, std::function<void()> action,
                               bool dev_only) {
	ContextMenuOption option;
	option.id = id;
	option.label = label;
	option.icon = icon;
	option.handler = action;
	option.dev_only = dev_only;

	context_menu_map[id] = option;
	update_context_menu_options();
	logging::write(std::format("registered context menu option: {}", id));
}

void register_components() {
	// No dynamic component registration is needed.
	logging::write("components loaded (C++: statically linked via headers)");
}

void initialize() {
	logging::write("initializing modules");

	//         modules[name] = wrap_module(name, Vue.markRaw(module_def));

	// Register all modules with their function pointers.
	// The order matches the JS MODULES object.

	// their module headers have not been created yet. They need to be ported.

	auto add_module = [](const std::string& name,
	                     std::function<void()> render_fn,
	                     std::function<void()> mounted_fn,
	                     std::function<void()> register_fn,
	                     std::function<void()> initialize_fn = nullptr) {
		ModuleDef mod;
		mod.name = name;
		mod.render = render_fn;
		mod.mounted = mounted_fn;
		mod.registerModule = register_fn;
		mod.initialize = initialize_fn ? initialize_fn : mounted_fn;
		module_registry[name] = std::move(mod);
	};

	add_module("source_select",
		[]() { screen_source_select::render(); },
		[]() { screen_source_select::mounted(); },
		nullptr);

	add_module("settings",
		[]() { screen_settings::render(); },
		[]() { screen_settings::mounted(); },
		[]() { screen_settings::registerScreen(); });

	add_module("tab_home",
		[]() { tab_home::render(); },
		nullptr,
		nullptr);

	add_module("tab_maps",
		[]() { tab_maps::render(); },
		[]() { tab_maps::mounted(); },
		[]() { tab_maps::registerTab(); });

	add_module("tab_zones",
		[]() { tab_zones::render(); },
		[]() { tab_zones::mounted(); },
		[]() { tab_zones::registerTab(); });

	add_module("tab_data",
		[]() { tab_data::render(); },
		[]() { tab_data::mounted(); },
		[]() { tab_data::registerTab(); });

	add_module("tab_raw",
		[]() { tab_raw::render(); },
		[]() { tab_raw::mounted(); },
		[]() { tab_raw::registerTab(); });

	add_module("tab_install",
		[]() { tab_install::render(); },
		[]() { tab_install::mounted(); },
		[]() { tab_install::registerTab(); });

	add_module("tab_text",
		[]() { tab_text::render(); },
		[]() { tab_text::mounted(); },
		[]() { tab_text::registerTab(); });

	add_module("tab_fonts",
		[]() { tab_fonts::render(); },
		[]() { tab_fonts::mounted(); },
		[]() { tab_fonts::registerTab(); });

	add_module("tab_videos",
		[]() { tab_videos::render(); },
		[]() { tab_videos::mounted(); },
		[]() { tab_videos::registerTab(); });

	add_module("tab_models",
		[]() { tab_models::render(); },
		[]() { tab_models::mounted(); },
		[]() { tab_models::registerTab(); });

	add_module("tab_creatures",
		[]() { tab_creatures::render(); },
		[]() { tab_creatures::mounted(); },
		[]() { tab_creatures::registerTab(); });

	add_module("tab_decor",
		[]() { tab_decor::render(); },
		[]() { tab_decor::mounted(); },
		[]() { tab_decor::registerTab(); });

	add_module("tab_audio",
		[]() { tab_audio::render(); },
		[]() { tab_audio::mounted(); },
		[]() { tab_audio::registerTab(); });

	add_module("tab_items",
		[]() { tab_items::render(); },
		[]() { tab_items::mounted(); },
		[]() { tab_items::registerTab(); });

	add_module("tab_item_sets",
		[]() { tab_item_sets::render(); },
		[]() { tab_item_sets::mounted(); },
		[]() { tab_item_sets::registerTab(); });

	add_module("tab_characters",
		[]() { tab_characters::render(); },
		[]() { tab_characters::mounted(); },
		[]() { tab_characters::registerTab(); });

	add_module("tab_textures",
		[]() { tab_textures::render(); },
		[]() { tab_textures::mounted(); },
		[]() { tab_textures::registerTab(); });

	// has not been created yet. It needs to be ported.

	// has not been created yet. It needs to be ported.

	// has not been created yet. It needs to be ported.

	add_module("legacy_tab_home",
		[]() { legacy_tab_home::render(); },
		nullptr,
		nullptr);

	add_module("legacy_tab_audio",
		[]() { legacy_tab_audio::render(); },
		[]() { legacy_tab_audio::mounted(); },
		[]() { legacy_tab_audio::registerTab(); });

	add_module("legacy_tab_textures",
		[]() { legacy_tab_textures::render(); },
		[]() { legacy_tab_textures::mounted(); },
		[]() { legacy_tab_textures::registerTab(); });

	add_module("legacy_tab_fonts",
		[]() { legacy_tab_fonts::render(); },
		[]() { legacy_tab_fonts::mounted(); },
		[]() { legacy_tab_fonts::registerTab(); });

	add_module("legacy_tab_files",
		[]() { legacy_tab_files::render(); },
		[]() { legacy_tab_files::mounted(); },
		[]() { legacy_tab_files::registerTab(); });

	add_module("legacy_tab_data",
		[]() { legacy_tab_data::render(); },
		[]() { legacy_tab_data::mounted(); },
		[]() { legacy_tab_data::registerTab(); });

	add_module("tab_models_legacy",
		[]() { tab_models_legacy::render(); },
		[]() { tab_models_legacy::mounted(); },
		[]() { tab_models_legacy::registerTab(); });

	// Wrap all modules (call register, set up initialize guard)
	for (auto& [name, mod] : module_registry)
		wrap_module(mod);

	// Build the module name list for logging
	std::string module_names;
	for (const auto& [name, mod] : module_registry) {
		if (!module_names.empty())
			module_names += ", ";
		module_names += name;
	}

	logging::write(std::format("modules loaded: {}", module_names));
}

void set_active(const std::string& module_key) {
	if (active_module) {
		if (core::view)
			core::view->activeModule = nullptr;
		active_module = nullptr;
	}

	if (!module_key.empty()) {
		auto it = module_registry.find(module_key);
		if (it != module_registry.end()) {
			active_module = &it->second;

			if (core::view) {
				nlohmann::json mod_json;
				mod_json["__name"] = module_key;
				core::view->activeModule = mod_json;
			}

			logging::write(std::format("set active module: {}", module_key));

			// on first activation. The wrapped initialize has an idempotency guard,
			// so it's safe to call on every activation.
			if (it->second.initialize)
				it->second.initialize();
		}
	}
}

void setActive(const std::string& module_key) {
	set_active(module_key);
}

void go_to_landing() {
	if (core::view->installType == 0)
		set_active("source_select");
	else if (core::view->installType == static_cast<int>(install_type::MPQ))
		set_active("legacy_tab_home");
	else
		set_active("tab_home");
}

void reload_module(const std::string& module_key) {
	if (IS_BUNDLED) {
		logging::write(std::format("cannot reload module {}: not available in production", module_key));
		return;
	}

	auto it = module_registry.find(module_key);
	if (it == module_registry.end()) {
		logging::write(std::format("cannot reload module {}: not found", module_key));
		return;
	}

	bool was_active = (active_module == &it->second);

	if (was_active) {
		if (core::view)
			core::view->activeModule = nullptr;
		active_module = nullptr;
	}

	// invalidate component cache so they're re-required on next access
	// preserve excluded components (stateful 3D viewers)
	// Components are statically linked. This is a no-op.

	unregister_nav_button(module_key);
	unregister_context_menu_option(module_key);

	// Re-wrapping the existing module definition instead.
	it->second._tab_initialized = false;
	it->second._tab_initializing = false;
	wrap_module(it->second);

	logging::write(std::format("reloaded module: {}", module_key));

	if (was_active)
		set_active(module_key);
}

void reloadActiveModule() {
	if (IS_BUNDLED) {
		logging::write("cannot reload active module: not available in production");
		return;
	}

	if (!active_module) {
		logging::write("no active module to reload");
		return;
	}

	std::string module_key = active_module->name;
	reload_module(module_key);
}

void reloadAllModules() {
	if (IS_BUNDLED) {
		logging::write("cannot reload modules: not available in production");
		return;
	}

	std::string active_module_key = active_module ? active_module->name : "";

	if (core::view)
		core::view->activeModule = nullptr;
	active_module = nullptr;

	// Collect module keys
	std::vector<std::string> module_keys;
	for (const auto& [name, mod] : module_registry)
		module_keys.push_back(name);

	for (const auto& module_key : module_keys) {
		unregister_nav_button(module_key);
		unregister_context_menu_option(module_key);
	}

	// Re-wrapping all existing module definitions instead.
	for (auto& [name, mod] : module_registry) {
		mod._tab_initialized = false;
		mod._tab_initializing = false;
		wrap_module(mod);
	}

	// Build the module name list for logging
	std::string module_names;
	for (const auto& [name, mod] : module_registry) {
		if (!module_names.empty())
			module_names += ", ";
		module_names += name;
	}

	logging::write(std::format("reloaded all modules: {}", module_names));

	if (!active_module_key.empty() && module_registry.contains(active_module_key))
		set_active(active_module_key);
}

ModuleDef* get(const std::string& module_key) {
	auto it = module_registry.find(module_key);
	if (it != module_registry.end())
		return &it->second;
	return nullptr;
}

ModuleDef* getActive() {
	return active_module;
}

const std::vector<NavButton>& getNavButtons() {
	return sorted_nav_buttons;
}

const std::vector<ContextMenuOption>& getContextMenuOptions() {
	return sorted_context_menu_options;
}

} // namespace modules