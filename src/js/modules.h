/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <functional>

/**
 * Module manager — central registry for application modules (tabs/screens).
 *
 * JS equivalent: The modules.cpp file that manages Vue component modules,
 * including registration, activation, nav buttons, context menu options,
 * hot-reloading (dev only), and the component_registry proxy.
 *
 * render(), mounted(), registerTab()/registerScreen() for each module.
 * Components are simply included via headers (no hot-reload in C++).
 *
 * module.exports = { register_components, initialize, set_active, setActive,
 *     go_to_landing, reload_module, reloadActiveModule, reloadAllModules,
 *     registerContextMenuOption, InstallType, [module_name]: module_def }
 */
namespace modules {

/**
 * Navigation button entry (module-registered).
 */
struct NavButton {
	std::string module;
	std::string label;
	std::string icon;
	uint32_t installTypes = 0;
};

/**
 * Context menu option entry.
 */
struct ContextMenuOption {
	std::string id;
	std::string label;
	std::string icon;
	std::function<void()> handler;
	bool dev_only = false;
};

/**
 * Module definition — function pointers for each module's lifecycle.
 */
struct ModuleDef {
	std::string name;
	std::function<void()> render;
	std::function<void()> mounted;
	std::function<void()> registerModule; // registerTab() or registerScreen()

	// initialize() with idempotency guard, error handling
	std::function<void()> initialize;

	// Internal state for initialization guard
	bool _tab_initialized = false;
	bool _tab_initializing = false;

	/**
	 * Activate this module (JS equivalent: proxy get 'setActive').
	 * Calls modules::set_active(this->name).
	 */
	void setActive();

	/**
	 * Reload this module (JS equivalent: proxy get 'reload').
	 * Calls modules::reload_module(this->name).
	 */
	void reload();
};

/**
 * Register a navigation button for a module.
 * JS equivalent: register_nav_button(module_name, label, icon, install_types)
 */
void register_nav_button(const std::string& module_name, const std::string& label,
                         const std::string& icon, uint32_t install_types);

/**
 * Unregister a navigation button for a module.
 * JS equivalent: unregister_nav_button(module_name)
 */
void unregister_nav_button(const std::string& module_name);

/**
 * Register a context menu option.
 * JS equivalent: register_context_menu_option(id, label, icon, action)
 */
void register_context_menu_option(const std::string& id, const std::string& label,
                                  const std::string& icon, std::function<void()> action = nullptr);

/**
 * Unregister a context menu option.
 * JS equivalent: unregister_context_menu_option(id)
 */
void unregister_context_menu_option(const std::string& id);

/**
 * Register a static context menu option (from outside module system).
 * JS equivalent: register_static_context_menu_option(id, label, icon, action, dev_only)
 */
void registerContextMenuOption(const std::string& id, const std::string& label,
                               const std::string& icon, std::function<void()> action,
                               bool dev_only = false);

/**
 * Register Vue components (no-op in C++ — ImGui components are header-included).
 * JS equivalent: register_components(app)
 */
void register_components();

/**
 * Initialize all modules.
 * JS equivalent: initialize(core_instance)
 */
void initialize();

/**
 * Set the active module by key.
 * JS equivalent: set_active(module_key)
 */
void set_active(const std::string& module_key);

/**
 * Set the active module by key (alias).
 * JS equivalent: setActive(module_key)
 */
void setActive(const std::string& module_key);

/**
 * Navigate to the appropriate landing/home page based on install type.
 * JS equivalent: go_to_landing()
 */
void go_to_landing();

/**
 * Reload a specific module (dev-only, no-op in release builds).
 * JS equivalent: reload_module(module_key)
 */
void reload_module(const std::string& module_key);

/**
 * Reload the currently active module (dev-only).
 * JS equivalent: reloadActiveModule()
 */
void reloadActiveModule();

/**
 * Reload all modules (dev-only).
 * JS equivalent: reloadAllModules()
 */
void reloadAllModules();

/**
 * Get a module definition by name (returns nullptr if not found).
 * JS equivalent: modules[module_key] (proxy get)
 */
ModuleDef* get(const std::string& module_key);

/**
 * Get the currently active module (may be nullptr).
 */
ModuleDef* getActive();

/**
 * Get all registered nav buttons (sorted by NAV_BUTTON_ORDER).
 */
const std::vector<NavButton>& getNavButtons();

/**
 * Get all registered context menu options (sorted by CONTEXT_MENU_ORDER).
 */
const std::vector<ContextMenuOption>& getContextMenuOptions();

/**
 * Open a knowledge-base article in the help tab.
 * JS equivalent: modules.tab_help.open_article(kb_id)
 *
 * Sets a pending KB ID and activates the tab_help module. When tab_help
 * mounts, it picks up the pending ID and scrolls to the article.
 */
void openHelpArticle(const std::string& kb_id);

/**
 * Get and clear the pending KB article ID (consumed by tab_help on mount).
 */
std::string consumePendingKbId();

} // namespace modules
