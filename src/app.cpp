/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shobjidl.h>
#else
#include <unistd.h>
#endif

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <stb_image.h>

#define NANOSVG_IMPLEMENTATION
#include <nanosvg.h>
#define NANOSVGRAST_IMPLEMENTATION
#include <nanosvgrast.h>

#include "app.h"
#include "js/constants.h"
#include "js/generics.h"
// const updater = require('./js/updater'); // Removed: updater module deleted
#include "js/core.h"
#include "js/install-type.h"
#include "js/casc/listfile.h"
#include "js/casc/dbd-manifest.h"
#include "js/casc/cdn-resolver.h"
#include "js/log.h"
#include "js/config.h"
#include "js/casc/tact-keys.h"
#include "js/casc/export-helper.h"
// const ExternalLinks = require('./js/external-links'); // Removed: external-links module deleted
#include "js/ui/texture-ribbon.h"
#include "js/3D/Shaders.h"
#include "js/gpu-info.h"
#include "js/modules.h"
#include "js/modules/tab_textures.h"

// BUILD_RELEASE will be set by the bundler during production builds allowing us
// to discern a production build. For debugging builds, process.env.BUILD_RELEASE
// will be undefined. Any code that only runs when BUILD_RELEASE is false will
// be removed as dead-code during compile.
#ifdef NDEBUG
static constexpr bool BUILD_RELEASE = true;
#else
static constexpr bool BUILD_RELEASE = false;
#endif

// check for --disable-auto-update flag
static bool DISABLE_AUTO_UPDATE = false;

/**
 * crash() is used to inform the user that the application has exploded.
 * It is purposely global and primitive as we have no idea what state
 * the application will be in when it is called.
 * @param {string} errorCode
 * @param {string} errorText
 */
static bool isCrashed = false;
static std::string crashErrorCode;
static std::string crashErrorText;
static std::string crashLogDump;

// ── App shell texture state (header logo + SVG icons) ────────────

static GLuint s_logoTexture = 0;
static int s_logoWidth = 0;
static int s_logoHeight = 0;

static GLuint s_helpIconTexture = 0;
static GLuint s_hamburgerIconTexture = 0;

/**
 * Load a PNG/JPEG image from disk into an OpenGL texture.
 * Returns the GL texture ID (0 on failure).
 */
static GLuint loadImageTexture(const std::filesystem::path& path, int* out_w = nullptr, int* out_h = nullptr) {
	int w = 0, h = 0, channels = 0;
	unsigned char* pixels = stbi_load(path.string().c_str(), &w, &h, &channels, 4);
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

	stbi_image_free(pixels);

	if (out_w) *out_w = w;
	if (out_h) *out_h = h;
	return tex;
}

/**
 * Load an SVG file and rasterize it into an OpenGL texture at the given size.
 * Returns the GL texture ID (0 on failure).
 */
static GLuint loadSvgTexture(const std::filesystem::path& path, int size) {
	NSVGimage* image = nsvgParseFromFile(path.string().c_str(), "px", 96.0f);
	if (!image)
		return 0;

	NSVGrasterizer* rast = nsvgCreateRasterizer();
	if (!rast) {
		nsvgDelete(image);
		return 0;
	}

	float scale = static_cast<float>(size) / (std::max)(image->width, image->height);
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

/**
 * Initialize app shell textures (logo + icons).
 * Called once after OpenGL context is ready.
 */
static void initAppShellTextures() {
	std::filesystem::path dataDir = constants::DATA_DIR();

	// Load logo.png (32px display size, but load at full resolution)
	s_logoTexture = loadImageTexture(dataDir / "images" / "logo.png", &s_logoWidth, &s_logoHeight);
	if (!s_logoTexture)
		logging::write("warning: failed to load logo.png for header");

	// Load SVG icons at 20px for the header buttons
	s_helpIconTexture = loadSvgTexture(dataDir / "fa-icons" / "help.svg", 40);
	if (!s_helpIconTexture)
		logging::write("warning: failed to load help.svg for header");

	s_hamburgerIconTexture = loadSvgTexture(dataDir / "fa-icons" / "line-columns.svg", 40);
	if (!s_hamburgerIconTexture)
		logging::write("warning: failed to load line-columns.svg for header");
}

/**
 * Cleanup app shell textures.
 */
static void destroyAppShellTextures() {
	if (s_logoTexture) { glDeleteTextures(1, &s_logoTexture); s_logoTexture = 0; }
	if (s_helpIconTexture) { glDeleteTextures(1, &s_helpIconTexture); s_helpIconTexture = 0; }
	if (s_hamburgerIconTexture) { glDeleteTextures(1, &s_hamburgerIconTexture); s_hamburgerIconTexture = 0; }
}

// Nav icon texture cache (loaded on demand)
static std::unordered_map<std::string, GLuint> s_navIconTextures;

/**
 * Get or load a nav icon SVG texture by filename.
 */
static GLuint getNavIconTexture(const std::string& icon_filename) {
	auto it = s_navIconTextures.find(icon_filename);
	if (it != s_navIconTextures.end())
		return it->second;

	std::filesystem::path path = constants::DATA_DIR() / "fa-icons" / icon_filename;
	GLuint tex = loadSvgTexture(path, 44);
	s_navIconTextures[icon_filename] = tex;
	return tex;
}

static void crash(const std::string& errorCode, const std::string& errorText) {
	// Prevent a never-ending cycle of depression.
	if (isCrashed)
		return;

	isCrashed = true;

	// TODO(conversion): In JS, the crash function replaces the entire document
	// with a crash screen showing version/flavour/build, error code/text, and
	// log dump. In C++/ImGui, we store the crash state and render it in the
	// main loop as a full-window crash overlay.

	crashErrorCode = errorCode;
	crashErrorText = errorText;

	// getErrorDump is set as a global function by the log module.
	// This is used to get the contents of the runtime log without depending on the module.
	crashLogDump = getErrorDump();

	// If we can, emit a global event to the application informing of the crash.
	if (core::view)
		core::events.emit("crash");
}

// ── Crash screen rendering (ImGui equivalent of the <noscript> crash markup) ──

static void renderCrashScreen() {
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::Begin("##CrashScreen", nullptr,
		ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoBringToFrontOnFocus);

	ImGui::Text("wow.export has crashed!");
	ImGui::Separator();

	// Show build version/flavour/ID.
	ImGui::Text("Version: v%s", std::string(constants::VERSION).c_str());

	// Display our error code/text.
	ImGui::Spacing();
	ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Error Code: %s", crashErrorCode.c_str());
	ImGui::TextWrapped("Error Message: %s", crashErrorText.c_str());

	// Show log dump if available.
	if (!crashLogDump.empty()) {
		ImGui::Spacing();
		ImGui::Text("Runtime Log:");
		ImGui::BeginChild("##CrashLog", ImVec2(0, 0), ImGuiChildFlags_Borders);
		ImGui::TextUnformatted(crashLogDump.c_str(), crashLogDump.c_str() + crashLogDump.size());
		ImGui::EndChild();
	}

	ImGui::End();
}

// ── App shell rendering (header / content / footer) ──────────────
// JS equivalent: The Vue template in index.html that defines the
// #container with grid-template-rows: 53px 1fr 73px.

// App shell layout constants from app.css
static constexpr float HEADER_HEIGHT = 53.0f;  // grid-template-rows: 53px
static constexpr float FOOTER_HEIGHT = 73.0f;  // grid-template-rows: 73px
static constexpr float NAV_ICON_WIDTH = 45.0f;  // #nav .option .nav-icon width
static constexpr float NAV_ICON_HEIGHT = 52.0f; // #nav .option .nav-icon height

// CSS color constants — aliases to centralized theme (app.h / app::theme)
static constexpr ImVec4 COLOR_BG_DARK    = app::theme::BG_DARK;
static constexpr ImVec4 COLOR_BORDER     = app::theme::BORDER;
static constexpr ImVec4 COLOR_FONT_FADED = app::theme::FONT_FADED;
static constexpr ImVec4 COLOR_NAV_ACTIVE = app::theme::NAV_SELECTED;

static void renderAppShell() {
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	const ImVec2 vp_pos = viewport->WorkPos;
	const ImVec2 vp_size = viewport->WorkSize;

	// ── Header (53px) ───────────────────────────────────────────
	{
		ImGui::SetNextWindowPos(vp_pos);
		ImGui::SetNextWindowSize(ImVec2(vp_size.x, HEADER_HEIGHT));
		ImGui::PushStyleColor(ImGuiCol_WindowBg, COLOR_BG_DARK);
		ImGui::PushStyleColor(ImGuiCol_Border, COLOR_BORDER);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::Begin("##AppHeader", nullptr,
			ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoFocusOnAppearing);

		ImDrawList* draw = ImGui::GetWindowDrawList();
		// Draw 1px bottom border (border-bottom: 1px solid --border)
		draw->AddLine(
			ImVec2(vp_pos.x, vp_pos.y + HEADER_HEIGHT - 1.0f),
			ImVec2(vp_pos.x + vp_size.x, vp_pos.y + HEADER_HEIGHT - 1.0f),
			ImGui::ColorConvertFloat4ToU32(COLOR_BORDER), 1.0f);

		// ── Logo (#logo): 15px left margin, 32px image, then "wow.export" text ──
		float cursor_x = 15.0f;
		ImGui::SetCursorPos(ImVec2(cursor_x, (HEADER_HEIGHT - 32.0f) * 0.5f));
		if (s_logoTexture) {
			ImGui::Image(static_cast<ImTextureID>(static_cast<uintptr_t>(s_logoTexture)),
				ImVec2(32.0f, 32.0f));
			if (ImGui::IsItemClicked()) {
				// JS: @click="setActiveModule(installType === 1 ? 'legacy_tab_home' : 'tab_home')"
				if (core::view) {
					if (core::view->installType == static_cast<int>(install_type::MPQ))
						modules::setActive("legacy_tab_home");
					else
						modules::setActive("tab_home");
				}
			}
			cursor_x += 32.0f + 8.0f; // 32px image + 8px padding (CSS: padding: 0 0 3px 40px — 40px includes 32px icon + 8px gap)
		}

		// "wow.export" text at 25px bold
		ImGui::SetCursorPos(ImVec2(cursor_x, (HEADER_HEIGHT - 25.0f) * 0.5f));
		// Push bold font at 25px (CSS: #logo span { font-size: 25px; font-weight: 700; })
		{
			ImFont* bold = app::theme::getBoldFont();
			ImGui::PushFont(bold, 25.0f);
			ImGui::TextUnformatted("wow.export");
			if (ImGui::IsItemClicked()) {
				if (core::view) {
					if (core::view->installType == static_cast<int>(install_type::MPQ))
						modules::setActive("legacy_tab_home");
					else
						modules::setActive("tab_home");
				}
			}
			ImGui::PopFont();
		}
		cursor_x = ImGui::GetItemRectMax().x - vp_pos.x + 10.0f;

		// ── Navigation icons (#nav) ─────────────────────────────
		// JS: <div id="nav" v-if="!isLoading">
		if (core::view && !core::view->isLoading) {
			// Render nav buttons filtered by installType
			// JS: <template v-for="btn in modNavButtons">
			//       <div v-if="btn.installTypes & installType" ...>
			const auto& navButtons = modules::getNavButtons();
			for (const auto& btn : navButtons) {
				if (!(btn.installTypes & static_cast<uint32_t>(core::view->installType)))
					continue;

				bool is_active = false;
				if (core::view->activeModule.is_object() &&
					core::view->activeModule.contains("__name") &&
					core::view->activeModule["__name"].get<std::string>() == btn.module) {
					is_active = true;
				}

				// Load icon texture on demand
				GLuint icon_tex = getNavIconTexture(btn.icon);
				if (icon_tex) {
					ImGui::PushID(btn.module.c_str());

					// Use InvisibleButton for hit testing, then draw image with correct tint.
					ImGui::SetCursorPos(ImVec2(cursor_x, (HEADER_HEIGHT - NAV_ICON_HEIGHT) * 0.5f));
					ImGui::InvisibleButton("##nav", ImVec2(NAV_ICON_WIDTH, NAV_ICON_HEIGHT));
					bool hovered = ImGui::IsItemHovered();
					bool clicked = ImGui::IsItemClicked();

					// Tint: active = green (#22b549), hover = brightness(2), default = dim white
					ImVec4 tint = ImVec4(1, 1, 1, 0.8f);
					if (is_active)
						tint = COLOR_NAV_ACTIVE;
					else if (hovered)
						tint = ImVec4(1, 1, 1, 1.0f); // Full opacity on hover (JS: brightness(2) on white SVG)

					// Draw the icon image over the invisible button area
					ImVec2 icon_size(NAV_ICON_WIDTH, NAV_ICON_HEIGHT - 8.0f);
					ImVec2 btn_min = ImGui::GetItemRectMin();
					// Center the icon vertically within the button area
					ImVec2 icon_min(btn_min.x, btn_min.y + (NAV_ICON_HEIGHT - icon_size.y) * 0.5f);
					ImVec2 icon_max(icon_min.x + icon_size.x, icon_min.y + icon_size.y);
					ImGui::GetWindowDrawList()->AddImage(
						static_cast<ImTextureID>(static_cast<uintptr_t>(icon_tex)),
						icon_min, icon_max, ImVec2(0, 0), ImVec2(1, 1),
						ImGui::ColorConvertFloat4ToU32(tint));

					if (clicked)
						modules::setActive(btn.module);

					// Tooltip label on hover: white text on dark background, to the right of the icon
					// JS: .nav-label { position: absolute; left: 100%; color: var(--font-primary);
					//      background: var(--background-dark); height: 52px; }
					if (hovered) {
						ImVec2 tooltip_pos(ImGui::GetItemRectMax().x + 8.0f, btn_min.y);
						ImGui::SetNextWindowPos(tooltip_pos);
						ImGui::PushStyleColor(ImGuiCol_PopupBg, COLOR_BG_DARK);
						ImGui::PushStyleColor(ImGuiCol_Text, app::theme::FONT_PRIMARY);
						ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 0.0f));
						ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
						ImGui::BeginTooltip();
						// Vertically center text within 52px height
						float text_h = ImGui::GetTextLineHeight();
						ImGui::SetCursorPosY((NAV_ICON_HEIGHT - text_h) * 0.5f);
						ImGui::TextUnformatted(btn.label.c_str());
						ImGui::EndTooltip();
						ImGui::PopStyleVar(2);
						ImGui::PopStyleColor(2);
					}

					ImGui::PopID();
				}

				cursor_x += NAV_ICON_WIDTH;
			}

			// ── Right-side icons (help + hamburger menu) ────────
			// These are positioned from the right edge of the header.
			// JS: #nav-help { margin-left: auto; margin-right: 10px; }
			// JS: #nav-extra { margin-right: 15px; }

			float right_x = vp_size.x;

			// Hamburger menu icon (rightmost, 15px right margin)
			// JS: #nav-extra { width: 20px; height: 20px; margin-right: 15px; }
			if (!core::view->isBusy) {
				right_x -= 15.0f + 20.0f;
				ImGui::SetCursorPos(ImVec2(right_x, (HEADER_HEIGHT - 20.0f) * 0.5f));
				if (s_hamburgerIconTexture) {
					ImGui::PushID("##nav-extra");
					ImGui::Image(static_cast<ImTextureID>(static_cast<uintptr_t>(s_hamburgerIconTexture)),
						ImVec2(20.0f, 20.0f));
					if (ImGui::IsItemClicked())
						core::view->contextMenus.stateNavExtra = !core::view->contextMenus.stateNavExtra;
					ImGui::PopID();
				}

				// Context menu for hamburger button
				// JS: <context-menu @close="contextMenus.stateNavExtra = false" :node="contextMenus.stateNavExtra" id="menu-extra">
				if (core::view->contextMenus.stateNavExtra) {
					ImGui::SetNextWindowPos(ImVec2(vp_pos.x + right_x, vp_pos.y + HEADER_HEIGHT));
					if (ImGui::Begin("##MenuExtra", nullptr,
						ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
						ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize |
						ImGuiWindowFlags_NoFocusOnAppearing)) {

						ImGui::PushStyleColor(ImGuiCol_WindowBg, COLOR_BG_DARK);

						const auto& contextOpts = modules::getContextMenuOptions();
						for (const auto& opt : contextOpts) {
							// JS: v-if="!opt.action?.dev_only || isDev"
							if (opt.dev_only && !(core::view->isDev))
								continue;

							if (ImGui::MenuItem(opt.label.c_str())) {
								core::view->contextMenus.stateNavExtra = false;
								if (opt.handler)
									opt.handler();
								else
									modules::setActive(opt.id);
							}
						}

						ImGui::PopStyleColor();
					}
					ImGui::End();

					// Close context menu when clicking outside
					if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) && ImGui::IsMouseClicked(0))
						core::view->contextMenus.stateNavExtra = false;
				}

				// Help icon (left of hamburger, 10px right margin)
				// JS: #nav-help { margin-left: auto; margin-right: 10px; }
				right_x -= 10.0f + 20.0f;
				ImGui::SetCursorPos(ImVec2(right_x, (HEADER_HEIGHT - 20.0f) * 0.5f));
				if (s_helpIconTexture) {
					ImGui::PushID("##nav-help");
					ImGui::Image(static_cast<ImTextureID>(static_cast<uintptr_t>(s_helpIconTexture)),
						ImVec2(20.0f, 20.0f));
					// JS: @click="setActiveModule('tab_help')" — tab_help is removed in C++ version
					// The help icon currently has no action since tab_help was deleted.
					if (ImGui::IsItemHovered()) {
						ImGui::BeginTooltip();
						ImGui::TextUnformatted("Help");
						ImGui::EndTooltip();
					}
					ImGui::PopID();
				}
			}
		}

		ImGui::End();
		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(2);
	}

	// ── Footer (73px) ───────────────────────────────────────────
	{
		float footer_y = vp_pos.y + vp_size.y - FOOTER_HEIGHT;
		ImGui::SetNextWindowPos(ImVec2(vp_pos.x, footer_y));
		ImGui::SetNextWindowSize(ImVec2(vp_size.x, FOOTER_HEIGHT));
		ImGui::PushStyleColor(ImGuiCol_WindowBg, COLOR_BG_DARK);
		ImGui::PushStyleColor(ImGuiCol_Border, COLOR_BORDER);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::Begin("##AppFooter", nullptr,
			ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoFocusOnAppearing);

		ImDrawList* draw = ImGui::GetWindowDrawList();
		// Draw 1px top border (border-top: 1px solid --border)
		draw->AddLine(
			ImVec2(vp_pos.x, footer_y),
			ImVec2(vp_pos.x + vp_size.x, footer_y),
			ImGui::ColorConvertFloat4ToU32(COLOR_BORDER), 1.0f);

		// JS: <span id="footer-links">
		//       <a data-external="::WEBSITE">Website</a> -
		//       <a data-external="::DISCORD">Discord</a> -
		//       <a data-external="::PATREON">Patreon</a> -
		//       <a data-external="::GITHUB">GitHub</a>
		//     </span>
		// Footer content: centered text (flex column, align-items: center, justify-content: center)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, COLOR_FONT_FADED);

			// Links line — render each link as a clickable item
			// JS: data-external="::WEBSITE" etc. → opens via nw.Shell.openExternal()
			struct FooterLink {
				const char* label;
				const char* url;
			};
			static constexpr FooterLink links[] = {
				{ "Website", "https://www.kruithne.net/wow.export/" },
				{ "Discord", "https://discord.gg/kC3EzAYBtf" },
				{ "Patreon", "https://patreon.com/Kruithne" },
				{ "GitHub", "https://github.com/Kruithne/wow.export" }
			};

			// Calculate total width of links line for centering
			float total_w = 0;
			for (int i = 0; i < 4; i++) {
				total_w += ImGui::CalcTextSize(links[i].label).x;
				if (i < 3)
					total_w += ImGui::CalcTextSize(" - ").x;
			}

			float line_h = ImGui::CalcTextSize("A").y;
			float links_y = (FOOTER_HEIGHT - line_h * 2 - 4.0f) * 0.5f;
			float start_x = (vp_size.x - total_w) * 0.5f;
			ImGui::SetCursorPos(ImVec2(start_x, links_y));

			for (int i = 0; i < 4; i++) {
				if (i > 0) {
					ImGui::SameLine(0, 0);
					ImGui::TextUnformatted(" - ");
					ImGui::SameLine(0, 0);
				}
				ImGui::PushStyleColor(ImGuiCol_Text, app::theme::FONT_ALT);
				ImGui::TextUnformatted(links[i].label);
				ImGui::PopStyleColor();
				if (ImGui::IsItemHovered())
					ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
				if (ImGui::IsItemClicked())
					core::openInExplorer(links[i].url);
				ImGui::SameLine(0, 0);
			}
			// End the SameLine sequence
			ImGui::NewLine();

			// JS: <span id="footer-copyright">
			//       World of Warcraft and related trademarks are registered trademarks of
			//       Blizzard Entertainment whom this application is not affiliated with.
			//     </span>
			const char* copyright_text = "World of Warcraft and related trademarks are registered trademarks of Blizzard Entertainment whom this application is not affiliated with.";
			ImVec2 copy_size = ImGui::CalcTextSize(copyright_text);
			ImGui::SetCursorPos(ImVec2((vp_size.x - copy_size.x) * 0.5f, links_y + line_h + 4.0f));
			ImGui::TextUnformatted(copyright_text);

			ImGui::PopStyleColor();
		}

		ImGui::End();
		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(2);
	}

	// ── Content area (between header and footer) ────────────────
	{
		float content_y = vp_pos.y + HEADER_HEIGHT;
		float content_h = vp_size.y - HEADER_HEIGHT - FOOTER_HEIGHT;
		if (content_h < 0) content_h = 0;

		ImGui::SetNextWindowPos(ImVec2(vp_pos.x, content_y));
		ImGui::SetNextWindowSize(ImVec2(vp_size.x, content_h));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::Begin("##AppContent", nullptr,
			ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoFocusOnAppearing);

		// Render the active module inside the content area
		// JS: <div id="module-container" v-if="activeModule">
		//       <keep-alive><component :is="activeModule"></component></keep-alive>
		//     </div>
		modules::ModuleDef* active = modules::getActive();
		if (active && active->render) {
			try {
				active->render();
			} catch (const std::exception& e) {
				crash("ERR_RENDER", e.what());
			}
		}

		ImGui::End();
		ImGui::PopStyleVar(2);
	}
}

static std::string getPlatformName() {
#ifdef _WIN32
	return "win32";
#elif defined(__linux__)
	return "linux";
#else
	return "unknown";
#endif
}

static std::string getArchName() {
#if defined(__x86_64__) || defined(_M_X64)
	return "x64";
#elif defined(__i386__) || defined(_M_IX86)
	return "x86";
#elif defined(__aarch64__) || defined(_M_ARM64)
	return "arm64";
#else
	return "unknown";
#endif
}

static std::string getCPUModel() {
#ifdef _WIN32
	// Read from registry or environment
	char buf[256] = {};
	DWORD bufSize = sizeof(buf);
	HKEY hKey;
	if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
		"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
		0, KEY_READ, &hKey) == ERROR_SUCCESS) {
		RegQueryValueExA(hKey, "ProcessorNameString", nullptr, nullptr,
			reinterpret_cast<LPBYTE>(buf), &bufSize);
		RegCloseKey(hKey);
	}
	std::string model(buf);
	if (model.empty())
		model = "Unknown CPU";
	return model;
#elif defined(__linux__)
	std::ifstream cpuinfo("/proc/cpuinfo");
	std::string line;
	while (std::getline(cpuinfo, line)) {
		if (line.find("model name") == 0) {
			auto pos = line.find(':');
			if (pos != std::string::npos) {
				std::string model = line.substr(pos + 1);
				// Trim leading whitespace
				auto start = model.find_first_not_of(" \t");
				if (start != std::string::npos)
					model = model.substr(start);
				return model;
			}
		}
	}
	return "Unknown CPU";
#else
	return "Unknown CPU";
#endif
}

static int getCPUCoreCount() {
	int count = static_cast<int>(std::thread::hardware_concurrency());
	return count > 0 ? count : 1;
}

static uint64_t getTotalMemory() {
#ifdef _WIN32
	MEMORYSTATUSEX status;
	status.dwLength = sizeof(status);
	GlobalMemoryStatusEx(&status);
	return status.ullTotalPhys;
#elif defined(__linux__)
	long pages = sysconf(_SC_PHYS_PAGES);
	long page_size = sysconf(_SC_PAGE_SIZE);
	if (pages > 0 && page_size > 0)
		return static_cast<uint64_t>(pages) * static_cast<uint64_t>(page_size);
	return 0;
#else
	return 0;
#endif
}

static uint64_t getFreeMemory() {
#ifdef _WIN32
	MEMORYSTATUSEX status;
	status.dwLength = sizeof(status);
	GlobalMemoryStatusEx(&status);
	return status.ullAvailPhys;
#elif defined(__linux__)
	long pages = sysconf(_SC_AVPHYS_PAGES);
	long page_size = sysconf(_SC_PAGE_SIZE);
	if (pages > 0 && page_size > 0)
		return static_cast<uint64_t>(pages) * static_cast<uint64_t>(page_size);
	return 0;
#else
	return 0;
#endif
}

static std::filesystem::path getHomeDir() {
#ifdef _WIN32
	const char* userProfile = std::getenv("USERPROFILE");
	if (userProfile)
		return std::filesystem::path(userProfile);
	const char* homeDrive = std::getenv("HOMEDRIVE");
	const char* homePath = std::getenv("HOMEPATH");
	if (homeDrive && homePath)
		return std::filesystem::path(std::string(homeDrive) + homePath);
	return std::filesystem::path("C:\\");
#else
	const char* home = std::getenv("HOME");
	if (home)
		return std::filesystem::path(home);
	return std::filesystem::path("/tmp");
#endif
}

// ── GLFW drop callback for file drag/drop ────────────────────────

static void glfw_drop_callback(GLFWwindow* /*window*/, int count, const char** paths) {
	if (count <= 0 || !paths)
		return;

	if (!core::view)
		return;

	core::view->fileDropPrompt = nullptr;

	const DropHandler* handler = core::getDropHandler(paths[0]);
	if (handler) {
		// Since dataTransfer.files is a FileList, we need to iterate it the old fashioned way.
		std::vector<std::string> include;
		for (int i = 0; i < count; ++i) {
			std::string check = paths[i];
			std::string lower;
			lower.reserve(check.size());
			for (char c : check)
				lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

			bool match = false;
			for (const auto& ext : handler->ext) {
				if (lower.size() >= ext.size() &&
					lower.compare(lower.size() - ext.size(), ext.size(), ext) == 0) {
					match = true;
					break;
				}
			}

			if (match)
				include.push_back(paths[i]);
		}

		if (!include.empty() && handler->process)
			handler->process(include[0]);
	}
}

// ── Cache size file management ───────────────────────────────────

static int64_t prevCacheSize = -1;
static std::chrono::steady_clock::time_point cacheSizeUpdateScheduledAt;
static bool cacheSizeUpdatePending = false;

static void loadCacheSize() {
	// Load cachesize, a file used to track the overall size of the cache directory
	// without having to calculate the real size before showing to users. Fast and reliable.
	try {
		std::ifstream in(constants::CACHE::SIZE());
		if (in.is_open()) {
			std::string data;
			std::getline(in, data);
			int64_t val = 0;
			try { val = std::stoll(data); } catch (...) { val = 0; }
			core::view->cacheSize = val;
		}
	} catch (...) {
		// Ignore errors — cacheSize stays at 0.
	}

	// Record initial value to prevent needless file write.
	prevCacheSize = core::view->cacheSize;
}

static void checkCacheSizeUpdate() {
	if (!core::view)
		return;

	// Create a watcher programmatically *after* assigning the initial value
	// to prevent a needless file write by triggering itself during init.
	if (core::view->cacheSize != prevCacheSize) {
		prevCacheSize = core::view->cacheSize;

		// We buffer this call by SIZE_UPDATE_DELAY so that we're not writing
		// to the file constantly during heavy cache usage. Postponing until
		// next tick would not help due to async and potential IO/net delay.
		cacheSizeUpdateScheduledAt = std::chrono::steady_clock::now();
		cacheSizeUpdatePending = true;
	}

	if (cacheSizeUpdatePending) {
		auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now() - cacheSizeUpdateScheduledAt).count();
		if (elapsed >= constants::CACHE::SIZE_UPDATE_DELAY) {
			cacheSizeUpdatePending = false;
			try {
				std::ofstream out(constants::CACHE::SIZE());
				out << core::view->cacheSize;
			} catch (...) {
				// Ignore write errors.
			}
		}
	}
}

// ── Vue methods / computed / watch equivalents ───────────────────
// In C++/ImGui, these are called directly from the modules or from
// the main loop. They are defined here to preserve the JS structure.

namespace app {

/**
 * Opens the runtime application log from the application data directory.
 */
static void openRuntimeLog() {
	logging::openRuntimeLog();
}

/**
 * Reloads all stylesheets in the document.
 * JS equivalent: reloads <link> tags. C++ equivalent: re-applies the
 * ImGui theme derived from app.css.
 */
static void reloadStylesheet() {
	app::theme::applyTheme();
}

/**
 * Reload the currently active module.
 */
static void reloadActiveModule() {
	modules::reloadActiveModule();
}

/**
 * Reload all loaded modules.
 */
static void reloadAllModules() {
	modules::reloadAllModules();
}

/**
 * Mark all WMO groups to the given state.
 * @param {boolean} state
 */
static void setAllWMOGroups(bool state) {
	if (!core::view)
		return;
	for (auto& node : core::view->modelViewerWMOGroups)
		node["checked"] = state;
}

/**
 * Mark all decor geosets to the given state.
 * @param {boolean} state
 */
static void setAllDecorGeosets(bool state) {
	if (!core::view)
		return;
	for (auto& node : core::view->decorViewerGeosets)
		node["checked"] = state;
}

/**
 * Mark all decor WMO groups to the given state.
 * @param {boolean} state
 */
static void setAllDecorWMOGroups(bool state) {
	if (!core::view)
		return;
	for (auto& node : core::view->decorViewerWMOGroups)
		node["checked"] = state;
}

/**
 * Mark all creature geosets to the given state.
 * @param {boolean} state
 */
static void setAllCreatureGeosets(bool state) {
	if (!core::view)
		return;
	for (auto& node : core::view->creatureViewerGeosets)
		node["checked"] = state;
}

/**
 * Mark all creature equipment toggles to the given state.
 * @param {boolean} state
 */
static void setAllCreatureEquipment(bool state) {
	if (!core::view)
		return;
	for (auto& node : core::view->creatureViewerEquipment)
		node["checked"] = state;
}

/**
 * Mark all creature WMO groups to the given state.
 * @param {boolean} state
 */
static void setAllCreatureWMOGroups(bool state) {
	if (!core::view)
		return;
	for (auto& node : core::view->creatureViewerWMOGroups)
		node["checked"] = state;
}

/**
 * Toggle UV layer for the main model viewer.
 * @param {string} layerName
 */
static void toggleUVLayer(const std::string& layerName) {
	// In JS: core.events.emit('toggle-uv-layer', layerName);
	core::events.emit("toggle-uv-layer", layerName);
}

/**
 * Mark all geosets to the given state.
 * @param {boolean} state
 * @param {object} geosets
 */
static void setAllGeosets(bool state, std::vector<nlohmann::json>& geosets) {
	for (auto& node : geosets)
		node["checked"] = state;
}

/**
 * Mark all decor category subcategories to the given state.
 * @param {boolean} state
 */
static void setAllDecorCategories(bool state) {
	if (!core::view)
		return;
	for (auto& entry : core::view->decorCategoryMask)
		entry["checked"] = state;
}

/**
 * Mark all subcategories within a category group to the given state.
 * @param {number} category_id
 * @param {boolean} state
 */
static void setDecorCategoryGroup(int category_id, bool state) {
	if (!core::view)
		return;
	for (auto& entry : core::view->decorCategoryMask) {
		if (entry.contains("categoryID") && entry["categoryID"].get<int>() == category_id)
			entry["checked"] = state;
	}
}

/**
 * Mark all item types to the given state.
 * @param {boolean} state
 */
static void setAllItemTypes(bool /*state*/) {
	// TODO(conversion): itemViewerTypeMask is std::vector<int>, not JSON with checked field.
	// The JS version sets entry.checked for each entry. In C++, the mask is handled
	// differently by the item viewer module.
}

/**
 * Mark all item qualities to the given state.
 * @param {boolean} state
 */
static void setAllItemQualities(bool /*state*/) {
	// TODO(conversion): itemViewerQualityMask is std::vector<int>, not JSON with checked field.
	// The JS version sets entry.checked for each entry. In C++, the mask is handled
	// differently by the item viewer module.
}

/**
 * Return a tag for a given product.
 * @param {string} product
 */
static std::string getProductTag(std::string_view product) {
	auto it = std::find_if(constants::PRODUCTS.begin(), constants::PRODUCTS.end(),
		[&](const constants::Product& e) { return e.product == product; });
	return it != constants::PRODUCTS.end() ? std::string(it->tag) : std::string("Unknown");
}

static void setActiveModule(const std::string& module_name) {
	modules::setActive(module_name);
}

static void handleContextMenuClick(const modules::ContextMenuOption& opt) {
	if (opt.handler)
		opt.handler();
	else
		modules::setActive(opt.id);
}

/**
 * Invoked when a toast option is clicked.
 * The tag is passed to our global event emitter.
 * @param {string} tag
 */
static void handleToastOptionClick(const std::function<void()>& func) {
	if (core::view)
		core::view->toast.reset();

	if (func)
		func();
}

/**
 * Invoked when a user cancels a model override filter.
 */
static void removeOverrideModels() {
	if (!core::view)
		return;
	core::view->overrideModelList.clear();
	core::view->overrideModelName.clear();
}

/**
 * Invoked when a user cancels a texture override filter.
 */
static void removeOverrideTextures() {
	if (!core::view)
		return;
	core::view->overrideTextureList.clear();
	core::view->overrideTextureName.clear();
}

/**
 * Invoked when the user manually selects a CDN region.
 * @param {object} region
 */
static void setSelectedCDN(const nlohmann::json& region) {
	if (!core::view)
		return;
	core::view->selectedCDNRegion = region;
	core::view->lockCDNRegion = true;
	core::view->config["sourceSelectUserRegion"] = region.value("tag", "");
	casc::cdn_resolver::startPreResolution(region.value("tag", ""));
}

/**
 * Emit an event using the global event emitter.
 * @param {string} tag
 * @param {object} event
 */
static void click(const std::string& tag) {
	// TODO(conversion): In JS, this checks event.target.classList.contains('disabled')
	// before emitting. In ImGui, disabled state is handled differently by each widget.
	core::events.emit("click-" + tag);
}

/**
 * Pass-through function to emit events from reactive markup.
 * @param {string} tag
 * @param  {...any} params
 */
static void emit(const std::string& tag) {
	core::events.emit(tag);
}

/**
 * Hide the toast bar.
 * @param {boolean} userCancel
 */
static void hideToast(bool userCancel = false) {
	core::hideToast(userCancel);
}

/**
 * Restart the application.
 * JS equivalent: chrome.runtime.reload() — reloads the NW.js app.
 * C++ equivalent: re-exec the current process binary.
 */
void restartApplication() {
#ifdef _WIN32
	wchar_t exe_path[MAX_PATH];
	DWORD len = GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
	if (len > 0 && len < MAX_PATH) {
		STARTUPINFOW si{};
		si.cb = sizeof(si);
		PROCESS_INFORMATION pi{};
		if (CreateProcessW(exe_path, GetCommandLineW(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
			CloseHandle(pi.hThread);
			CloseHandle(pi.hProcess);
		}
	}
	std::exit(0);
#else
	char exe_path[PATH_MAX];
	ssize_t path_len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
	if (path_len > 0) {
		exe_path[path_len] = '\0';
		execl(exe_path, exe_path, nullptr);
	}
	// execl only returns on error — fall through to exit.
	std::exit(0);
#endif
}

/**
 * Invoked when the texture ribbon element on the model viewer
 * fires a resize event.
 */
static void onTextureRibbonResize(int width) {
	texture_ribbon::onResize(width);
}

/**
 * Copy given data as text to the system clipboard.
 * @param {string} data
 */
static void copyToClipboard(const std::string& data) {
	ImGui::SetClipboardText(data.c_str());
}

/**
 * Get the external export path for a given file.
 * @param {string} file
 * @returns {string}
 */
static std::string getExportPath(const std::string& file) {
	return casc::ExportHelper::getExportPath(file);
}

// Removed: getExternalLink() — external-links module deleted
static void getExternalLink() {
	return; // Removed: external-links module deleted
}

// ── Computed property equivalents ────────────────────────────────

/**
 * Return the formatted duration of the selected track on the sound player.
 */
static std::string soundPlayerDurationFormatted() {
	if (!core::view)
		return "00:00";
	return generics::formatPlaybackSeconds(core::view->soundPlayerDuration);
}

/**
 * Return the formatted current seek of the selected track on the sound player.
 */
static std::string soundPlayerSeekFormatted() {
	if (!core::view)
		return "00:00";
	return generics::formatPlaybackSeconds(core::view->soundPlayerSeek * core::view->soundPlayerDuration);
}

/**
 * Returns the maximum amount of pages needed for the texture ribbon.
 * @returns {number}
 */
static int textureRibbonMaxPages() {
	if (!core::view || core::view->textureRibbonSlotCount <= 0)
		return 0;
	return static_cast<int>(std::ceil(
		static_cast<double>(core::view->textureRibbonStack.size()) /
		core::view->textureRibbonSlotCount));
}

/**
 * Returns the texture ribbon stack array subject to paging.
 * @returns {Array}
 */
static std::vector<nlohmann::json> textureRibbonDisplay() {
	if (!core::view)
		return {};
	int startIndex = core::view->textureRibbonPage * core::view->textureRibbonSlotCount;
	int endIndex = startIndex + core::view->textureRibbonSlotCount;
	auto& stack = core::view->textureRibbonStack;
	if (startIndex >= static_cast<int>(stack.size()))
		return {};
	endIndex = (std::min)(endIndex, static_cast<int>(stack.size()));
	return std::vector<nlohmann::json>(
		stack.begin() + startIndex,
		stack.begin() + endIndex);
}

} // namespace app

// ── Centralized ImGui theme from app.css ─────────────────────────

namespace app::theme {

void applyTheme() {
	ImGuiStyle& style = ImGui::GetStyle();

	// ── Colors ───────────────────────────────────────────────────
	ImVec4* colors = style.Colors;

	// Window backgrounds
	colors[ImGuiCol_WindowBg]  = BG;          // --background
	colors[ImGuiCol_ChildBg]   = BG_ALT;      // --background-alt
	colors[ImGuiCol_PopupBg]   = BG_DARK;     // --background-dark
	colors[ImGuiCol_MenuBarBg] = BG_DARK;     // --background-dark

	// Borders
	colors[ImGuiCol_Border]       = BORDER;   // --border
	colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

	// Text
	colors[ImGuiCol_Text]         = FONT_PRIMARY;   // --font-primary: #ffffffcc
	colors[ImGuiCol_TextDisabled] = FONT_FADED;     // --font-faded: #6c757d

	// Frame (input fields, checkboxes, sliders)
	colors[ImGuiCol_FrameBg]        = BG_ALT;
	colors[ImGuiCol_FrameBgHovered] = ImVec4(BG_ALT.x * 1.2f, BG_ALT.y * 1.2f, BG_ALT.z * 1.2f, 1.0f);
	colors[ImGuiCol_FrameBgActive]  = ImVec4(BG_ALT.x * 1.4f, BG_ALT.y * 1.4f, BG_ALT.z * 1.4f, 1.0f);

	// Title bar
	colors[ImGuiCol_TitleBg]          = BG_DARK;
	colors[ImGuiCol_TitleBgActive]    = BG_DARK;
	colors[ImGuiCol_TitleBgCollapsed] = BG_DARK;

	// Scrollbar
	colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.0f, 0.0f, 0.0f, 0.0f); // transparent track
	colors[ImGuiCol_ScrollbarGrab]        = BORDER;         // --border
	colors[ImGuiCol_ScrollbarGrabHovered] = FONT_HIGHLIGHT; // --font-highlight (white on hover)
	colors[ImGuiCol_ScrollbarGrabActive]  = FONT_HIGHLIGHT;

	// Buttons (CSS: --form-button-base / --form-button-hover)
	colors[ImGuiCol_Button]        = BUTTON_BASE;    // --form-button-base: #22b549
	colors[ImGuiCol_ButtonHovered] = BUTTON_HOVER;   // --form-button-hover: #2665d2
	colors[ImGuiCol_ButtonActive]  = BUTTON_HOVER;

	// Headers (collapsible headers, selectable highlights)
	colors[ImGuiCol_Header]        = ImVec4(BUTTON_BASE.x, BUTTON_BASE.y, BUTTON_BASE.z, 0.3f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(BUTTON_BASE.x, BUTTON_BASE.y, BUTTON_BASE.z, 0.5f);
	colors[ImGuiCol_HeaderActive]  = ImVec4(BUTTON_BASE.x, BUTTON_BASE.y, BUTTON_BASE.z, 0.7f);

	// Separator
	colors[ImGuiCol_Separator]        = BORDER;
	colors[ImGuiCol_SeparatorHovered] = FONT_ALT;
	colors[ImGuiCol_SeparatorActive]  = FONT_ALT;

	// Resize grip
	colors[ImGuiCol_ResizeGrip]        = ImVec4(BORDER.x, BORDER.y, BORDER.z, 0.5f);
	colors[ImGuiCol_ResizeGripHovered] = FONT_ALT;
	colors[ImGuiCol_ResizeGripActive]  = FONT_ALT;

	// Tabs
	colors[ImGuiCol_Tab]                = BG_DARK;
	colors[ImGuiCol_TabHovered]         = BUTTON_BASE;
	colors[ImGuiCol_TabSelected]        = BUTTON_BASE;     // --nav-option-selected
	colors[ImGuiCol_TabSelectedOverline]= NAV_SELECTED;
	colors[ImGuiCol_TabDimmed]          = BG_DARK;
	colors[ImGuiCol_TabDimmedSelected]  = BG_ALT;

	// Check mark
	colors[ImGuiCol_CheckMark] = BUTTON_BASE; // green check

	// Slider grab
	colors[ImGuiCol_SliderGrab]       = BUTTON_BASE;
	colors[ImGuiCol_SliderGrabActive] = FONT_HIGHLIGHT;

	// Table
	colors[ImGuiCol_TableHeaderBg]     = BG_DARK;
	colors[ImGuiCol_TableBorderStrong] = BORDER;
	colors[ImGuiCol_TableBorderLight]  = ImVec4(BORDER.x, BORDER.y, BORDER.z, 0.5f);
	colors[ImGuiCol_TableRowBg]        = BG;
	colors[ImGuiCol_TableRowBgAlt]     = BG_DARK;

	// Nav highlight
	colors[ImGuiCol_NavHighlight] = BUTTON_BASE;

	// Text selection
	colors[ImGuiCol_TextSelectedBg] = ImVec4(FONT_ALT.x, FONT_ALT.y, FONT_ALT.z, 0.35f);

	// Drag-drop target
	colors[ImGuiCol_DragDropTarget] = FONT_ALT;

	// ── Rounding / sizing ────────────────────────────────────────
	style.WindowRounding    = WINDOW_ROUNDING;  // 0 — sharp corners
	style.FrameRounding     = FRAME_ROUNDING;   // 5px input border-radius
	style.GrabRounding      = FRAME_ROUNDING;
	style.PopupRounding     = POPUP_ROUNDING;   // 0 — sharp popup corners
	style.ScrollbarRounding = SCROLLBAR_ROUNDING; // 5px thumb rounding
	style.TabRounding       = 0.0f;             // sharp tab edges like CSS
	style.ChildRounding     = 0.0f;

	style.ScrollbarSize     = SCROLLBAR_SIZE;   // 8px width
	style.FramePadding      = ImVec2(6.0f, 4.0f);
	style.ItemSpacing       = ImVec2(8.0f, 6.0f);
	style.WindowPadding     = ImVec2(8.0f, 8.0f);

	// Button-specific padding is applied via PushStyleVar where needed,
	// since ImGui does not have a dedicated button padding style variable.
	// The CSS value is 9px 13px (vertical, horizontal) — see BUTTON_PADDING.

	style.WindowBorderSize  = 1.0f;
	style.FrameBorderSize   = 0.0f;
	style.PopupBorderSize   = 1.0f;
}

// ── Custom font loading ──────────────────────────────────────────
// Loads Selawik (regular + bold) and Gambler TTF fonts from data/fonts/.
// CSS: body { font-family: "Selawik", sans-serif; } — default 16px.
// @font-face { font-family: "Selawik"; font-weight: bold; src: url("fonts/selawkb.woff2"); }
// @font-face { font-family: "Gambler"; src: url("fonts/gmblr.woff2"); }

static ImFont* s_fontBold    = nullptr;
static ImFont* s_fontGambler = nullptr;

void loadFonts() {
	ImGuiIO& io = ImGui::GetIO();

	std::filesystem::path fontsDir = constants::DATA_DIR() / "fonts";
	std::string regularPath = (fontsDir / "selawk.ttf").string();
	std::string boldPath    = (fontsDir / "selawkb.ttf").string();
	std::string gamblerPath = (fontsDir / "gmblr.ttf").string();

	// Load Selawik regular as the default font (16px matches CSS body default).
	ImFont* regularFont = io.Fonts->AddFontFromFileTTF(regularPath.c_str(), DEFAULT_FONT_SIZE);
	if (!regularFont) {
		// Fallback: use ImGui's built-in font if the TTF file is not found.
		regularFont = io.Fonts->AddFontDefault();
	}

	// Load Selawik bold for use with ImGui::PushFont() where JS uses font-weight: bold.
	s_fontBold = io.Fonts->AddFontFromFileTTF(boldPath.c_str(), DEFAULT_FONT_SIZE);
	if (!s_fontBold) {
		// Fallback to the regular/default font.
		s_fontBold = regularFont;
	}

	// Load Gambler font (defined in app.css @font-face but not actively referenced in selectors).
	s_fontGambler = io.Fonts->AddFontFromFileTTF(gamblerPath.c_str(), DEFAULT_FONT_SIZE);
	if (!s_fontGambler) {
		s_fontGambler = regularFont;
	}

	// Build the atlas so fonts are ready for rendering.
	io.Fonts->Build();
}

ImFont* getBoldFont() {
	return s_fontBold ? s_fontBold : ImGui::GetFont();
}

ImFont* getGamblerFont() {
	return s_fontGambler ? s_fontGambler : ImGui::GetFont();
}

} // namespace app::theme

// ── Watch equivalents (change detection in the main loop) ────────

static double prevLoadPct = -1;
static void* prevCasc = nullptr;
static nlohmann::json prevActiveModule;

#ifdef _WIN32
static ITaskbarList3* s_taskbar = nullptr;
static bool s_com_initialized = false;

static void initTaskbarProgress() {
	if (SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED))) {
		s_com_initialized = true;
		CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER,
			IID_ITaskbarList3, reinterpret_cast<void**>(&s_taskbar));
		if (s_taskbar)
			s_taskbar->HrInit();
	}
}

static void setTaskbarProgress(GLFWwindow* window, double val) {
	if (!s_taskbar)
		return;

	HWND hwnd = glfwGetWin32Window(window);
	if (!hwnd)
		return;

	if (val < 0 || val >= 1.0) {
		s_taskbar->SetProgressState(hwnd, TBPF_NOPROGRESS);
	} else {
		s_taskbar->SetProgressState(hwnd, TBPF_NORMAL);
		s_taskbar->SetProgressValue(hwnd, static_cast<ULONGLONG>(val * 10000), 10000);
	}
}
#endif

static void checkWatchers(GLFWwindow* window) {
	if (!core::view)
		return;

	/**
	 * Invoked when the active loading percentage is changed.
	 * @param {float} val
	 */
	if (core::view->loadPct != prevLoadPct) {
		prevLoadPct = core::view->loadPct;
		// JS: win.setProgressBar(val) sets taskbar progress.
#ifdef _WIN32
		setTaskbarProgress(window, prevLoadPct);
#else
		// Linux: no standard taskbar progress API; no-op.
		(void)window;
#endif
	}

	/**
	 * Invoked when the core CASC instance is changed.
	 */
	void* currentCasc = static_cast<void*>(core::view->casc);
	if (currentCasc != prevCasc) {
		prevCasc = currentCasc;
		core::events.emit("casc-source-changed");
	}

	// watch activeModule and close context menus when it changes
	if (core::view->activeModule != prevActiveModule) {
		prevActiveModule = core::view->activeModule;
		auto& contextMenus = core::view->contextMenus;
		contextMenus.stateNavExtra = false;
		contextMenus.stateModelExport = false;
		contextMenus.stateCDNRegion = false;
		contextMenus.nodeTextureRibbon = nullptr;
		contextMenus.nodeItem = nullptr;
		contextMenus.nodeDataTable = nullptr;
		contextMenus.nodeListbox = nullptr;
		contextMenus.nodeMap = nullptr;
		contextMenus.nodeZone = nullptr;
	}
}

// ── Main entry point ─────────────────────────────────────────────

int main(int argc, char* argv[]) {
	// check for --disable-auto-update flag
	for (int i = 1; i < argc; ++i) {
		if (std::string_view(argv[i]) == "--disable-auto-update")
			DISABLE_AUTO_UPDATE = true;
	}

	// Initialize runtime paths (INSTALL_PATH, DATA_DIR, LOG_DIR, etc.)
	constants::init();

	// Initialize logging stream.
	logging::init();

	// ── GLFW / OpenGL / ImGui initialization ─────────────────────

	if (!glfwInit()) {
		crash("ERR_GLFW_INIT", "Failed to initialize GLFW");
		return 1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifndef NDEBUG
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

	// Append the application version to the title bar.
	std::string windowTitle = std::format("wow.export v{}", constants::VERSION);
	GLFWwindow* window = glfwCreateWindow(1280, 720, windowTitle.c_str(), nullptr, nullptr);
	if (!window) {
		crash("ERR_WINDOW_CREATE", "Failed to create GLFW window");
		glfwTerminate();
		return 1;
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // VSync

#ifdef _WIN32
	// Initialize Windows taskbar progress (ITaskbarList3).
	initTaskbarProgress();
#endif

	// Load OpenGL function pointers via GLAD2.
	if (!gladLoadGL(glfwGetProcAddress)) {
		crash("ERR_GLAD_INIT", "Failed to initialize OpenGL loader (GLAD2)");
		glfwDestroyWindow(window);
		glfwTerminate();
		return 1;
	}

	// Prevent files from being dropped onto the window. These are over-written
	// later but we disable here to prevent them working if init fails.
	// (GLFW does not have ondragover; drop callback is set later.)

	// Force all links to open in the users default application.
	// TODO(conversion): In ImGui, external links are opened via platform shell commands
	// rather than DOM click events. Each module handles this directly.
	// ExternalLinks.open(externalElement.getAttribute('data-external')); // Removed: external-links module deleted

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	// Apply the app.css theme to ImGui (replaces StyleColorsDark).
	app::theme::applyTheme();

	// Load custom fonts (Selawik regular/bold, Gambler) from data/fonts/.
	// Must be done after CreateContext and before the first NewFrame.
	app::theme::loadFonts();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 460");

	// Load app shell textures (logo, SVG icons) now that OpenGL is ready.
	initAppShellTextures();

	// ── Application state initialization ─────────────────────────

	// Initialize Vue equivalent: create AppState and assign to core::view.
	static AppState appState = core::makeNewView();
	core::view = &appState;

	// Interlink error handling for Vue.
	// TODO(conversion): In JS, app.config.errorHandler catches Vue render errors.
	// In C++, exceptions during render are caught by the main loop try/catch.

	modules::register_components();

	// dynamic interface scaling for smaller displays
	// TODO(conversion): In ImGui, scaling is handled via ImGui::GetIO().FontGlobalScale
	// and framebuffer DPI. The JS version scales a DOM container; the equivalent
	// in ImGui is adjusting the global font scale based on window size.
	constexpr int SCALE_THRESHOLD_W = 1120;
	constexpr int SCALE_THRESHOLD_H = 700;

	modules::initialize();

	// register static context menu options
	modules::registerContextMenuOption("runtime-log", "Open Runtime Log", "timeline.svg", []() { logging::openRuntimeLog(); });
	modules::registerContextMenuOption("restart", "Restart wow.export", "arrow-rotate-left.svg", []() { app::restartApplication(); });
	modules::registerContextMenuOption("reload-style", "Reload Styling", "palette.svg", []() { app::reloadStylesheet(); }, true);
	modules::registerContextMenuOption("reload-shaders", "Reload Shaders", "cube.svg", []() { shaders::reload_all(); }, true);
	modules::registerContextMenuOption("reload-active", "Reload Active Module", "gear.svg", []() { modules::reloadActiveModule(); }, true);
	modules::registerContextMenuOption("reload-all", "Reload All Modules", "gear.svg", []() { modules::reloadAllModules(); }, true);

	// Log some basic information for potential diagnostics.
	logging::write(std::format("wow.export has started v{}", constants::VERSION));
	logging::write(std::format("Host {} ({}), CPU {} ({} cores), Memory {} / {}",
		getPlatformName(), getArchName(), getCPUModel(), getCPUCoreCount(),
		generics::filesize(static_cast<double>(getFreeMemory())),
		generics::filesize(static_cast<double>(getTotalMemory()))));
	logging::write(std::format("INSTALL_PATH {} DATA_DIR {} LOG_DIR {}",
		constants::INSTALL_PATH().string(),
		constants::DATA_DIR().string(),
		constants::LOG_DIR().string()));

	// log gpu info async to avoid blocking startup
	gpu_info::log_gpu_info();

	// Load configuration.
	config::load();

	// Set-up default export directory if none configured.
	if (core::view->config.contains("exportDirectory") &&
		core::view->config["exportDirectory"].get<std::string>().empty()) {
		std::string defaultExportDir = (getHomeDir() / "wow.export").string();
		core::view->config["exportDirectory"] = defaultExportDir;
		logging::write(std::format("No export directory set, setting to {}", defaultExportDir));
	} else if (!core::view->config.contains("exportDirectory")) {
		std::string defaultExportDir = (getHomeDir() / "wow.export").string();
		core::view->config["exportDirectory"] = defaultExportDir;
		logging::write(std::format("No export directory set, setting to {}", defaultExportDir));
	}

	casc::listfile::preload();
	casc::dbd_manifest::preload();

	// Set-up proper drag/drop handlers.
	glfwSetDropCallback(window, glfw_drop_callback);

	//window.ondragover = e => { e.preventDefault(); return false; };
	//window.ondrop = e => { e.preventDefault(); return false; };

	loadCacheSize();

	// Load/update BLTE decryption keys.
	casc::tact_keys::load();

	// Removed: auto-update check — updater module deleted
	// if (BUILD_RELEASE && !DISABLE_AUTO_UPDATE) {
	// 	core::showLoadingScreen(1, "Checking for updates...");
	// 	updater::checkForUpdates().then(updateAvailable => {
	// 		if (updateAvailable) {
	// 			updater::applyUpdate();
	// 		} else {
	// 			core::hideLoadingScreen();
	// 			modules::setActive("source_select");
	// 		}
	// 	});
	// }

	// Load what's new HTML on app start
	// Removed: whats-new.html is no longer used — home page is a blank placeholder.
	// (async () => {
	// 	try {
	// 		const whats_new_path = BUILD_RELEASE ? './src/whats-new.html' : './src/whats-new.html';
	// 		const html = await fsp.readFile(whats_new_path, 'utf8');
	// 		core.view.whatsNewHTML = html;
	// 	} catch (e) {
	// 		log.write('failed to load whats-new.html: %o', e);
	// 	}
	// })();

	// Set source select as the currently active interface screen.
	modules::setActive("source_select");

	// Initialize watch state trackers to current values.
	prevLoadPct = core::view->loadPct;
	prevCasc = static_cast<void*>(core::view->casc);
	prevActiveModule = core::view->activeModule;

	// ── Main loop ────────────────────────────────────────────────

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		// Debugging reloader.
		if (!BUILD_RELEASE) {
			if (glfwGetKey(window, GLFW_KEY_F5) == GLFW_PRESS) {
				// In NW.js, F5 reloads the app via chrome.runtime.reload().
				// C++ equivalent: re-exec the current process binary.
				app::restartApplication();
			}
		}

		// dynamic interface scaling for smaller displays
		{
			int win_w, win_h;
			glfwGetWindowSize(window, &win_w, &win_h);
			float scale_w = win_w < SCALE_THRESHOLD_W ? static_cast<float>(win_w) / SCALE_THRESHOLD_W : 1.0f;
			float scale_h = win_h < SCALE_THRESHOLD_H ? static_cast<float>(win_h) / SCALE_THRESHOLD_H : 1.0f;
			float scale = (std::min)(scale_w, scale_h);
			io.FontGlobalScale = scale;
		}

		// Check watchers (loadPct, casc, activeModule)
		checkWatchers(window);

		// Check cache size update timer
		checkCacheSizeUpdate();

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		try {
			if (isCrashed) {
				renderCrashScreen();
			} else {
				// Render the app shell (header / content / footer) with the active
				// module rendered inside the content area.
				renderAppShell();
			}
		} catch (const std::exception& e) {
			crash("ERR_RENDER", e.what());
		}

		// Rendering
		ImGui::Render();
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(app::theme::BG_CLEAR_R, app::theme::BG_CLEAR_G, app::theme::BG_CLEAR_B, app::theme::BG_CLEAR_A);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}

	// ── Cleanup ──────────────────────────────────────────────────

	// Release app shell OpenGL textures before context teardown.
	destroyAppShellTextures();
	for (auto& [name, tex] : s_navIconTextures) {
		if (tex) glDeleteTextures(1, &tex);
	}
	s_navIconTextures.clear();

#ifdef _WIN32
	if (s_taskbar) {
		s_taskbar->Release();
		s_taskbar = nullptr;
	}
	if (s_com_initialized)
		CoUninitialize();
#endif

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}