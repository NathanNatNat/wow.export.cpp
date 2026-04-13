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
#include <any>
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <numbers>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
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
#include <webp/decode.h>

#define NANOSVG_IMPLEMENTATION
#include <nanosvg.h>
#define NANOSVGRAST_IMPLEMENTATION
#include <nanosvgrast.h>

#include "app.h"
#include "js/constants.h"
#include "js/generics.h"
#include "js/core.h"
#include "js/install-type.h"
#include "js/casc/listfile.h"
#include "js/casc/dbd-manifest.h"
#include "js/casc/cdn-resolver.h"
#include "js/log.h"
#include "js/config.h"
#include "js/casc/tact-keys.h"
#include "js/casc/export-helper.h"
#include "js/ui/texture-ribbon.h"
#include "js/3D/Shaders.h"
#include "js/gpu-info.h"
#include "js/modules.h"
#include "js/modules/tab_textures.h"
#include "js/modules/tab_items.h"
#include "js/external-links.h"

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


static GLuint s_logoTexture = 0;
static int s_logoWidth = 0;
static int s_logoHeight = 0;

// Loading screen textures (loading.gif / loading-xmas.gif first frame, gear.svg)
static GLuint s_loadingBgTexture = 0;
static int s_loadingBgWidth = 0;
static int s_loadingBgHeight = 0;
static GLuint s_loadingXmasBgTexture = 0;
static int s_loadingXmasBgWidth = 0;
static int s_loadingXmasBgHeight = 0;
static GLuint s_gearTexture = 0;
static int s_gearTexWidth = 0;
static int s_gearTexHeight = 0;

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
 * Initialize app shell textures (logo).
 * Called once after OpenGL context is ready.
 */
static void initAppShellTextures() {
	std::filesystem::path dataDir = constants::DATA_DIR();

	// Load logo.png (32px display size, but load at full resolution)
	s_logoTexture = loadImageTexture(dataDir / "images" / "logo.png", &s_logoWidth, &s_logoHeight);
	if (!s_logoTexture)
		logging::write("warning: failed to load logo.png for header");
	// Header icons (help, hamburger) now rendered via Font Awesome icon font.

	// Loading screen background images (first frame of GIF via stb_image)
	s_loadingBgTexture = loadImageTexture(dataDir / "images" / "loading.gif",
	                                      &s_loadingBgWidth, &s_loadingBgHeight);
	if (!s_loadingBgTexture)
		logging::write("warning: failed to load loading.gif for loading screen");

	s_loadingXmasBgTexture = loadImageTexture(dataDir / "images" / "loading-xmas.gif",
	                                          &s_loadingXmasBgWidth, &s_loadingXmasBgHeight);
	if (!s_loadingXmasBgTexture)
		logging::write("warning: failed to load loading-xmas.gif for loading screen");

	// Gear icon SVG for loading screen spinner (100px render)
	s_gearTexture = loadSvgTexture(dataDir / "fa-icons" / "gear.svg", 100);
	if (s_gearTexture) {
		s_gearTexWidth = 100;
		s_gearTexHeight = 100;
	} else {
		logging::write("warning: failed to load gear.svg for loading screen");
	}
}

/**
 * Cleanup app shell textures.
 */
static void destroyAppShellTextures() {
	if (s_logoTexture) { glDeleteTextures(1, &s_logoTexture); s_logoTexture = 0; }
	if (s_loadingBgTexture) { glDeleteTextures(1, &s_loadingBgTexture); s_loadingBgTexture = 0; }
	if (s_loadingXmasBgTexture) { glDeleteTextures(1, &s_loadingXmasBgTexture); s_loadingXmasBgTexture = 0; }
	if (s_gearTexture) { glDeleteTextures(1, &s_gearTexture); s_gearTexture = 0; }
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

// Forward declaration so crash handlers can call crash().
static void crash(const std::string& errorCode, const std::string& errorText);

/**
 * Global terminate handler — equivalent to JS:
 *   process.on('uncaughtException', e => crash('ERR_UNHANDLED_EXCEPTION', e.message));
 *
 * Called when a C++ exception escapes all try/catch blocks.
 * The handler must not return; after recording the crash it aborts.
 */
static void terminateHandler() {
	std::string message = "Unknown error";
	if (auto eptr = std::current_exception()) {
		try {
			std::rethrow_exception(eptr);
		} catch (const std::exception& e) {
			message = e.what();
		} catch (...) {
			message = "Non-standard exception";
		}
	}
	crash("ERR_UNHANDLED_EXCEPTION", message);
	// Reset SIGABRT to default before aborting to avoid recursion with our signal handler.
	std::signal(SIGABRT, SIG_DFL);
	std::abort();
}

/**
 * Signal handler for fatal signals (SIGSEGV, SIGABRT, SIGFPE, SIGILL).
 * Best-effort crash capture — not strictly async-signal-safe, but matches
 * the JS intent of catching unhandled errors and routing them to crash().
 */
static void fatalSignalHandler(int sig) {
	const char* sigName = "UNKNOWN";
	switch (sig) {
		case SIGSEGV: sigName = "SIGSEGV (Segmentation fault)"; break;
		case SIGABRT: sigName = "SIGABRT (Abort)"; break;
		case SIGFPE:  sigName = "SIGFPE (Floating-point exception)"; break;
		case SIGILL:  sigName = "SIGILL (Illegal instruction)"; break;
	}
	crash("ERR_FATAL_SIGNAL", std::string("Fatal signal: ") + sigName);
	// Re-raise with default handler for core dump / OS reporting.
	std::signal(sig, SIG_DFL);
	std::raise(sig);
}

#ifdef _WIN32
/**
 * Windows Structured Exception Handler for unhandled SEH exceptions
 * (access violations, stack overflows, etc.).
 * Equivalent to JS: process.on('uncaughtException', ...).
 */
static LONG WINAPI unhandledSEHFilter(EXCEPTION_POINTERS* exInfo) {
	std::string message = std::format("Unhandled SEH exception: 0x{:08X}",
		exInfo->ExceptionRecord->ExceptionCode);
	crash("ERR_UNHANDLED_EXCEPTION", message);
	return EXCEPTION_CONTINUE_SEARCH;
}
#endif

static void crash(const std::string& errorCode, const std::string& errorText) {
	// Prevent a never-ending cycle of depression.
	if (isCrashed)
		return;

	isCrashed = true;

	crashErrorCode = errorCode;
	crashErrorText = errorText;

	// getErrorDump is set as a global function by the log module.
	// This is used to get the contents of the runtime log without depending on the module.
	crashLogDump = getErrorDump();

	// If we can, emit a global event to the application informing of the crash.
	if (core::view)
		core::events.emit("crash");
}


static void renderCrashScreen() {
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::Begin("##CrashScreen", nullptr,
		ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoBringToFrontOnFocus);

	// JS: <h1>Oh no! The kākāpō has exploded...</h1>
	// Per naming convention, user-facing text says "wow.export.cpp".
	ImGui::Text("wow.export.cpp has crashed!");
	ImGui::Separator();

	// Show build version/flavour/ID.
	// JS: setText('#crash-screen-version', 'v' + manifest.version);
	// JS: setText('#crash-screen-flavour', manifest.flavour);
	// JS: setText('#crash-screen-build', manifest.guid);
	ImGui::Text("v%s", std::string(constants::VERSION).c_str());
	ImGui::SameLine();
	ImGui::Text("%s", std::string(constants::FLAVOUR).c_str());
	ImGui::SameLine();
	ImGui::Text("[%s]", std::string(constants::BUILD_GUID).c_str());

	// Display our error code/text.
	ImGui::Spacing();
	ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", crashErrorCode.c_str());
	ImGui::TextWrapped("%s", crashErrorText.c_str());

	// Action buttons matching JS: <div class="form-tray"> with 4 buttons.
	ImGui::Spacing();

	// JS: <input type="button" value="Report Issue" data-external="::ISSUE_TRACKER"/>
	if (ImGui::Button("Report Issue"))
		core::openInExplorer("https://github.com/Kruithne/wow.export/issues");
	ImGui::SameLine();

	// JS: <input type="button" value="Get Help on Discord" data-external="::DISCORD"/>
	if (ImGui::Button("Get Help on Discord"))
		core::openInExplorer("https://discord.gg/kC3EzAYBtf");
	ImGui::SameLine();

	// JS: <input type="button" value="Copy Log to Clipboard" onclick="nw.Clipboard.get().set(...)">
	if (ImGui::Button("Copy Log to Clipboard"))
		ImGui::SetClipboardText(crashLogDump.c_str());
	ImGui::SameLine();

	// JS: <input type="button" value="Restart Application" onclick="chrome.runtime.reload()"/>
	if (ImGui::Button("Restart Application"))
		app::restartApplication();

	// Runtime log displayed as a read-only, selectable textarea.
	// JS: <textarea id="crash-screen-log">No runtime log available.</textarea>
	ImGui::Spacing();
	static std::string crashLogBuffer;
	if (crashLogBuffer.empty())
		crashLogBuffer = crashLogDump.empty() ? "No runtime log available." : crashLogDump;

	ImGui::InputTextMultiline("##CrashLog", &crashLogBuffer[0], crashLogBuffer.size() + 1,
		ImVec2(-FLT_MIN, -FLT_MIN), ImGuiInputTextFlags_ReadOnly);

	ImGui::End();
}

// #container with grid-template-rows: 53px 1fr 73px.

static constexpr float HEADER_HEIGHT = 53.0f;
static constexpr float FOOTER_HEIGHT = 73.0f;
static constexpr float TOAST_HEIGHT   = 30.0f;
static constexpr float NAV_ICON_WIDTH = 45.0f;
static constexpr float NAV_ICON_HEIGHT = 52.0f;

static constexpr ImVec4 COLOR_BG_DARK    = app::theme::BG_DARK;
static constexpr ImVec4 COLOR_BORDER     = app::theme::BORDER;
static constexpr ImVec4 COLOR_FONT_FADED = app::theme::FONT_FADED;
static constexpr ImVec4 COLOR_NAV_ACTIVE = app::theme::NAV_SELECTED;

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
 * Hide the toast bar.
 * @param {boolean} userCancel
 */
static void hideToast(bool userCancel = false) {
	core::hideToast(userCancel);
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

static void renderAppShell() {
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	const ImVec2 vp_pos = viewport->WorkPos;
	const ImVec2 vp_size = viewport->WorkSize;

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

		float cursor_x = 15.0f;
		ImGui::SetCursorPos(ImVec2(cursor_x, (HEADER_HEIGHT - 32.0f) * 0.5f));
		if (s_logoTexture) {
			ImGui::Image(static_cast<ImTextureID>(static_cast<uintptr_t>(s_logoTexture)),
				ImVec2(32.0f, 32.0f));
			if (ImGui::IsItemClicked()) {
				if (core::view) {
					if (core::view->installType == static_cast<int>(install_type::MPQ))
						modules::setActive("legacy_tab_home");
					else
						modules::setActive("tab_home");
				}
			}
			cursor_x += 32.0f + 8.0f;
		}

		ImGui::SetCursorPos(ImVec2(cursor_x, (HEADER_HEIGHT - 25.0f) * 0.5f));
		{
			ImFont* bold = app::theme::getBoldFont();
			ImGui::PushFont(bold, 25.0f);
			ImGui::TextUnformatted("wow.export.cpp");
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

		if (core::view && !core::view->isLoading) {
			// Render nav buttons filtered by installType
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

				ImGui::PushID(btn.module.c_str());

				// Use InvisibleButton for hit testing, then draw icon with correct tint.
				ImGui::SetCursorPos(ImVec2(cursor_x, (HEADER_HEIGHT - NAV_ICON_HEIGHT) * 0.5f));
				ImGui::InvisibleButton("##nav", ImVec2(NAV_ICON_WIDTH, NAV_ICON_HEIGHT));
				bool hovered = ImGui::IsItemHovered();
				bool clicked = ImGui::IsItemClicked();

				// Tint: active = green (#22b549), hover = brightness(2), default = dim white
				ImU32 tint_u32;
				if (is_active)
					tint_u32 = ImGui::ColorConvertFloat4ToU32(COLOR_NAV_ACTIVE);
				else if (hovered)
					tint_u32 = IM_COL32(255, 255, 255, 255); // Full opacity on hover
				else
					tint_u32 = IM_COL32(255, 255, 255, 204); // 0.8 alpha default

				// Try icon font first, fall back to SVG texture for custom icons
				const char* icon_glyph = app::theme::getIconForFilename(btn.icon);
				if (icon_glyph) {
					// Render icon using Font Awesome icon font
					ImVec2 btn_min = ImGui::GetItemRectMin();
					float icon_font_size = NAV_ICON_HEIGHT - 16.0f; // ~36px icon within 52px button
					ImFont* icon_font = app::theme::getIconFont();
					ImVec2 text_size = icon_font->CalcTextSizeA(icon_font_size, FLT_MAX, 0.0f, icon_glyph);
					ImVec2 icon_pos(btn_min.x + (NAV_ICON_WIDTH - text_size.x) * 0.5f,
					                btn_min.y + (NAV_ICON_HEIGHT - text_size.y) * 0.5f);
					ImGui::GetWindowDrawList()->AddText(icon_font, icon_font_size, icon_pos, tint_u32, icon_glyph);
				} else {
					// Fallback: render custom icon via SVG texture
					GLuint icon_tex = getNavIconTexture(btn.icon);
					if (icon_tex) {
						ImVec2 icon_size(NAV_ICON_WIDTH, NAV_ICON_HEIGHT - 8.0f);
						ImVec2 btn_min = ImGui::GetItemRectMin();
						ImVec2 icon_min(btn_min.x, btn_min.y + (NAV_ICON_HEIGHT - icon_size.y) * 0.5f);
						ImVec2 icon_max(icon_min.x + icon_size.x, icon_min.y + icon_size.y);
						ImGui::GetWindowDrawList()->AddImage(
							static_cast<ImTextureID>(static_cast<uintptr_t>(icon_tex)),
							icon_min, icon_max, ImVec2(0, 0), ImVec2(1, 1), tint_u32);
					}
				}

				if (clicked)
					modules::setActive(btn.module);

				// Tooltip label on hover: white text on dark background, to the right of the icon
				//      background: var(--background-dark); height: 52px; }
				if (hovered) {
					ImVec2 tooltip_pos(ImGui::GetItemRectMax().x + 8.0f, ImGui::GetItemRectMin().y);
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

				cursor_x += NAV_ICON_WIDTH;
			}

			// These are positioned from the right edge of the header.

			float right_x = vp_size.x;

			// Hamburger menu icon (rightmost, 15px right margin)
			if (!core::view->isBusy) {
				right_x -= 15.0f + 20.0f;
				ImGui::SetCursorPos(ImVec2(right_x, (HEADER_HEIGHT - 20.0f) * 0.5f));
				{
					ImGui::PushID("##nav-extra");
					ImGui::InvisibleButton("##hamburger", ImVec2(20.0f, 20.0f));
					if (ImGui::IsItemClicked())
						core::view->contextMenus.stateNavExtra = !core::view->contextMenus.stateNavExtra;
					// Render hamburger icon using Font Awesome icon font
					ImFont* icon_font = app::theme::getIconFont();
					float icon_size = 18.0f;
					ImVec2 btn_min = ImGui::GetItemRectMin();
					ImVec2 text_sz = icon_font->CalcTextSizeA(icon_size, FLT_MAX, 0.0f, ICON_FA_BARS);
					ImVec2 icon_pos(btn_min.x + (20.0f - text_sz.x) * 0.5f,
					                btn_min.y + (20.0f - text_sz.y) * 0.5f);
					ImGui::GetWindowDrawList()->AddText(icon_font, icon_size, icon_pos,
						app::theme::FONT_PRIMARY_U32, ICON_FA_BARS);
					ImGui::PopID();
				}

				// Context menu for hamburger button
				if (core::view->contextMenus.stateNavExtra) {
					ImGui::SetNextWindowPos(ImVec2(vp_pos.x + right_x, vp_pos.y + HEADER_HEIGHT));
					if (ImGui::Begin("##MenuExtra", nullptr,
						ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
						ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize |
						ImGuiWindowFlags_NoFocusOnAppearing)) {

						ImGui::PushStyleColor(ImGuiCol_WindowBg, COLOR_BG_DARK);

						const auto& contextOpts = modules::getContextMenuOptions();
						for (const auto& opt : contextOpts) {
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
				right_x -= 10.0f + 20.0f;
				ImGui::SetCursorPos(ImVec2(right_x, (HEADER_HEIGHT - 20.0f) * 0.5f));
				{
					ImGui::PushID("##nav-help");
					ImGui::InvisibleButton("##help", ImVec2(20.0f, 20.0f));
					// Render help icon using Font Awesome icon font
					ImFont* icon_font = app::theme::getIconFont();
					float icon_size = 18.0f;
					ImVec2 btn_min = ImGui::GetItemRectMin();
					ImVec2 text_sz = icon_font->CalcTextSizeA(icon_size, FLT_MAX, 0.0f, ICON_FA_CIRCLE_QUESTION);
					ImVec2 icon_pos(btn_min.x + (20.0f - text_sz.x) * 0.5f,
					                btn_min.y + (20.0f - text_sz.y) * 0.5f);
					ImGui::GetWindowDrawList()->AddText(icon_font, icon_size, icon_pos,
						app::theme::FONT_PRIMARY_U32, ICON_FA_CIRCLE_QUESTION);
					// JS: @click="setActiveModule('tab_help')"
					if (ImGui::IsItemClicked())
						modules::setActive("tab_help");
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

		//       <a data-external="::WEBSITE">Website</a> -
		//       <a data-external="::DISCORD">Discord</a> -
		//       <a data-external="::PATREON">Patreon</a> -
		//       <a data-external="::GITHUB">GitHub</a>
		//     </span>
		// Footer content: centered text (flex column, align-items: center, justify-content: center)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, COLOR_FONT_FADED);

			// Links line — render each link as a clickable item
			struct FooterLink {
				const char* label;
				const char* url;
			};
			static constexpr FooterLink links[] = {
				{ "Website", "https://www.kruithne.net/wow.export/" },
				{ "Discord", "https://discord.gg/kC3EzAYBtf" },
				{ "Patreon", "https://patreon.com/Kruithne" },
				{ "GitHub", "https://github.com/NathanNatNat/wow.export.cpp" }
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

		//      background: url(./images/logo.png) no-repeat center center;
		//      opacity: 0.05; z-index: -5;
		if (s_logoTexture) {
			float logo_w = static_cast<float>(s_logoWidth);
			float logo_h = static_cast<float>(s_logoHeight);
			ImVec2 content_min(vp_pos.x, content_y);
			ImVec2 logo_min(
				content_min.x + (vp_size.x - logo_w) * 0.5f,
				content_min.y + (content_h - logo_h) * 0.5f);
			ImVec2 logo_max(logo_min.x + logo_w, logo_min.y + logo_h);
			// 5% opacity (0.05 * 255 ≈ 13)
			ImU32 watermark_tint = IM_COL32(255, 255, 255, 13);
			ImGui::GetWindowDrawList()->AddImage(
				static_cast<ImTextureID>(static_cast<uintptr_t>(s_logoTexture)),
				logo_min, logo_max, ImVec2(0, 0), ImVec2(1, 1), watermark_tint);
		}

		//       {{ toast.message }}
		//       <span v-for="action in toast.actions" @click="handleToastOptionClick(action)">{{ action.label }}</span>
		//       <div class="close" v-if="toast.closable" @click="hideToast(true)"></div>
		//     </div>
		float toast_h = 0.0f;
		if (core::view && core::view->toast.has_value()) {
			const auto& toast = core::view->toast.value();
			toast_h = TOAST_HEIGHT;

			// Determine background color and icon based on toast type
			ImU32 bg_color = app::theme::TOAST_INFO_U32;
			const char* icon_glyph = ICON_FA_CIRCLE_INFO;
			if (toast.type == "error") {
				bg_color = app::theme::TOAST_ERROR_U32;
				icon_glyph = ICON_FA_TRIANGLE_EXCLAMATION;
			} else if (toast.type == "success") {
				bg_color = app::theme::TOAST_SUCCESS_U32;
				icon_glyph = ICON_FA_CHECK;
			} else if (toast.type == "progress") {
				bg_color = app::theme::TOAST_PROGRESS_U32;
				icon_glyph = ICON_FA_STOPWATCH;
			}

			// Draw the toast bar background (full width, 30px tall)
			ImVec2 toast_min = ImGui::GetCursorScreenPos();
			ImVec2 toast_max(toast_min.x + vp_size.x, toast_min.y + TOAST_HEIGHT);
			ImDrawList* dl = ImGui::GetWindowDrawList();
			dl->AddRectFilled(toast_min, toast_max, bg_color);

			// Icon: 15px, positioned at 10px from left, vertically centered
			constexpr float ICON_SIZE = 15.0f;
			constexpr float ICON_LEFT = 10.0f;
			ImFont* icon_font = app::theme::getIconFont();
			ImVec2 icon_text_size = icon_font->CalcTextSizeA(ICON_SIZE, FLT_MAX, 0.0f, icon_glyph);
			ImVec2 icon_pos(toast_min.x + ICON_LEFT,
			                toast_min.y + (TOAST_HEIGHT - icon_text_size.y) * 0.5f);
			dl->AddText(icon_font, ICON_SIZE, icon_pos, app::theme::FONT_TOAST_U32, icon_glyph);

			// Message text: starts at padding-left: 30px
			constexpr float TEXT_LEFT = 30.0f;
			constexpr float TEXT_FONT_SIZE = 15.0f;
			ImFont* font = ImGui::GetFont();
			ImVec2 msg_size = font->CalcTextSizeA(TEXT_FONT_SIZE, FLT_MAX, 0.0f, toast.message.c_str());
			ImVec2 msg_pos(toast_min.x + TEXT_LEFT,
			               toast_min.y + (TOAST_HEIGHT - msg_size.y) * 0.5f);
			dl->AddText(font, TEXT_FONT_SIZE, msg_pos, app::theme::FONT_TOAST_U32, toast.message.c_str());

			// Action links
			float action_x = msg_pos.x + msg_size.x;
			for (size_t i = 0; i < toast.actions.size(); ++i) {
				const auto& action = toast.actions[i];
				action_x += 5.0f; // left margin
				ImVec2 act_size = font->CalcTextSizeA(TEXT_FONT_SIZE, FLT_MAX, 0.0f, action.label.c_str());
				ImVec2 act_pos(action_x, toast_min.y + (TOAST_HEIGHT - act_size.y) * 0.5f);

				// Invisible button for click detection
				ImGui::SetCursorScreenPos(act_pos);
				std::string act_id = "##toast_action_" + std::to_string(i);
				ImGui::InvisibleButton(act_id.c_str(), act_size);
				if (ImGui::IsItemClicked()) {
					handleToastOptionClick(action.callback);
				}
				if (ImGui::IsItemHovered())
					ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

				// Draw text and underline
				dl->AddText(font, TEXT_FONT_SIZE, act_pos, app::theme::FONT_TOAST_LINK_U32, action.label.c_str());
				dl->AddLine(ImVec2(act_pos.x, act_pos.y + act_size.y),
				            ImVec2(act_pos.x + act_size.x, act_pos.y + act_size.y),
				            app::theme::FONT_TOAST_LINK_U32, 1.0f);

				action_x += act_size.x + 5.0f; // right margin
			}

			// Close button: xmark icon at the right edge
			if (toast.closable) {
				constexpr float CLOSE_WIDTH = 30.0f;
				constexpr float CLOSE_ICON_SIZE = 10.0f;
				ImVec2 close_pos(toast_max.x - CLOSE_WIDTH,
				                 toast_min.y);
				ImGui::SetCursorScreenPos(close_pos);
				ImGui::InvisibleButton("##toast_close", ImVec2(CLOSE_WIDTH, TOAST_HEIGHT));
				if (ImGui::IsItemClicked()) {
					hideToast(true);
				}
				if (ImGui::IsItemHovered())
					ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

				// Draw xmark icon centered in the close area
				const char* close_glyph = ICON_FA_XMARK;
				ImVec2 close_icon_size = icon_font->CalcTextSizeA(CLOSE_ICON_SIZE, FLT_MAX, 0.0f, close_glyph);
				ImVec2 close_icon_pos(
					close_pos.x + (CLOSE_WIDTH - close_icon_size.x) * 0.5f,
					close_pos.y + (TOAST_HEIGHT - close_icon_size.y) * 0.5f);
				dl->AddText(icon_font, CLOSE_ICON_SIZE, close_icon_pos,
				            app::theme::FONT_TOAST_U32, close_glyph);
			}

			// Advance cursor past the toast bar
			ImGui::SetCursorScreenPos(ImVec2(toast_min.x, toast_max.y));
		}

		// Secondary toast: model override filter bar.
		// JS: <div id="toast" v-if="!toast && activeModule && activeModule.__name === 'tab_models' && overrideModelList.length > 0" class="progress">
		//       Filtering models for item: {{ overrideModelName }}
		//       <span @click="removeOverrideModels">Remove</span>
		//       <div class="close" @click="removeOverrideModels"></div>
		//     </div>
		if (core::view && !core::view->toast.has_value()) {
			modules::ModuleDef* cur = modules::getActive();
			if (cur && cur->name == "tab_models" && !core::view->overrideModelList.empty()) {
				toast_h = TOAST_HEIGHT;

				ImU32 bg_color = app::theme::TOAST_PROGRESS_U32;
				const char* icon_glyph = ICON_FA_STOPWATCH;

				ImVec2 toast_min = ImGui::GetCursorScreenPos();
				ImVec2 toast_max(toast_min.x + vp_size.x, toast_min.y + TOAST_HEIGHT);
				ImDrawList* dl = ImGui::GetWindowDrawList();
				dl->AddRectFilled(toast_min, toast_max, bg_color);

				// Icon
				constexpr float ICON_SIZE = 15.0f;
				constexpr float ICON_LEFT = 10.0f;
				ImFont* icon_font = app::theme::getIconFont();
				ImVec2 icon_text_size = icon_font->CalcTextSizeA(ICON_SIZE, FLT_MAX, 0.0f, icon_glyph);
				ImVec2 icon_pos(toast_min.x + ICON_LEFT,
				                toast_min.y + (TOAST_HEIGHT - icon_text_size.y) * 0.5f);
				dl->AddText(icon_font, ICON_SIZE, icon_pos, app::theme::FONT_TOAST_U32, icon_glyph);

				// Message: "Filtering models for item: {overrideModelName}"
				constexpr float TEXT_LEFT = 30.0f;
				constexpr float TEXT_FONT_SIZE = 15.0f;
				ImFont* font = ImGui::GetFont();
				std::string msg = std::format("Filtering models for item: {}", core::view->overrideModelName);
				ImVec2 msg_size = font->CalcTextSizeA(TEXT_FONT_SIZE, FLT_MAX, 0.0f, msg.c_str());
				ImVec2 msg_pos(toast_min.x + TEXT_LEFT,
				               toast_min.y + (TOAST_HEIGHT - msg_size.y) * 0.5f);
				dl->AddText(font, TEXT_FONT_SIZE, msg_pos, app::theme::FONT_TOAST_U32, msg.c_str());

				// "Remove" action link
				float action_x = msg_pos.x + msg_size.x + 5.0f;
				const char* remove_label = "Remove";
				ImVec2 act_size = font->CalcTextSizeA(TEXT_FONT_SIZE, FLT_MAX, 0.0f, remove_label);
				ImVec2 act_pos(action_x, toast_min.y + (TOAST_HEIGHT - act_size.y) * 0.5f);

				ImGui::SetCursorScreenPos(act_pos);
				ImGui::InvisibleButton("##override_remove", act_size);
				if (ImGui::IsItemClicked())
					removeOverrideModels();
				if (ImGui::IsItemHovered())
					ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

				dl->AddText(font, TEXT_FONT_SIZE, act_pos, app::theme::FONT_TOAST_LINK_U32, remove_label);
				dl->AddLine(ImVec2(act_pos.x, act_pos.y + act_size.y),
				            ImVec2(act_pos.x + act_size.x, act_pos.y + act_size.y),
				            app::theme::FONT_TOAST_LINK_U32, 1.0f);

				// Close button (also calls removeOverrideModels)
				constexpr float CLOSE_WIDTH = 30.0f;
				constexpr float CLOSE_ICON_SIZE = 10.0f;
				ImVec2 close_pos(toast_max.x - CLOSE_WIDTH, toast_min.y);
				ImGui::SetCursorScreenPos(close_pos);
				ImGui::InvisibleButton("##override_close", ImVec2(CLOSE_WIDTH, TOAST_HEIGHT));
				if (ImGui::IsItemClicked())
					removeOverrideModels();
				if (ImGui::IsItemHovered())
					ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

				const char* close_glyph = ICON_FA_XMARK;
				ImVec2 close_icon_size = icon_font->CalcTextSizeA(CLOSE_ICON_SIZE, FLT_MAX, 0.0f, close_glyph);
				ImVec2 close_icon_pos(
					close_pos.x + (CLOSE_WIDTH - close_icon_size.x) * 0.5f,
					close_pos.y + (TOAST_HEIGHT - close_icon_size.y) * 0.5f);
				dl->AddText(icon_font, CLOSE_ICON_SIZE, close_icon_pos,
				            app::theme::FONT_TOAST_U32, close_glyph);

				// Advance cursor past the override toast bar
				ImGui::SetCursorScreenPos(ImVec2(toast_min.x, toast_max.y));
			}
		}

		// Render the active module inside the content area
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

	//       <div id="loading-background" :class="{ xmas: isXmas }"></div>
	//       <div id="loading-icon"></div>
	//       <span id="loading-title">{{ loadingTitle }}</span>
	//       <span id="loading-progress">{{ loadingProgress }}</span>
	//       <div id="loading-bar"><div :style="{ width: (loadPct * 100) + '%' }"></div></div>
	//     </div>
	if (core::view && core::view->isLoading) {
		ImGui::SetNextWindowPos(vp_pos);
		ImGui::SetNextWindowSize(vp_size);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, app::theme::BG);
		ImGui::Begin("##LoadingOverlay", nullptr,
			ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
			ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);

		ImDrawList* dl = ImGui::GetWindowDrawList();

		GLuint bg_tex = core::view->isXmas ? s_loadingXmasBgTexture : s_loadingBgTexture;
		if (bg_tex) {
			ImVec2 uv0(0, 0), uv1(1, 1);
			ImU32 tint = IM_COL32(255, 255, 255, 51); // 0.2 * 255 ≈ 51
			dl->AddImage(
				static_cast<ImTextureID>(static_cast<uintptr_t>(bg_tex)),
				vp_pos, ImVec2(vp_pos.x + vp_size.x, vp_pos.y + vp_size.y),
				uv0, uv1, tint);
		}

		// Centered content: gear icon, title, progress text, progress bar
		float center_x = vp_pos.x + vp_size.x * 0.5f;
		float center_y = vp_pos.y + vp_size.y * 0.5f;

		// Calculate total content height for vertical centering:
		// gear(100) + margin(10) + title(25) + progress(20) + margin(15) + bar(15)
		constexpr float GEAR_SIZE = 100.0f;
		constexpr float GEAR_MARGIN_BOTTOM = 10.0f;
		constexpr float TITLE_FONT_SIZE = 25.0f;
		constexpr float PROGRESS_FONT_SIZE = 20.0f;
		constexpr float BAR_MARGIN_TOP = 15.0f;
		constexpr float BAR_WIDTH = 400.0f;
		constexpr float BAR_HEIGHT = 15.0f;

		float total_h = GEAR_SIZE + GEAR_MARGIN_BOTTOM + TITLE_FONT_SIZE + PROGRESS_FONT_SIZE + BAR_MARGIN_TOP + BAR_HEIGHT;
		float start_y = center_y - total_h * 0.5f;

		float gear_x = center_x - GEAR_SIZE * 0.5f;
		float gear_y = start_y;
		if (s_gearTexture) {
			// Compute rotation angle: 360° in 6 seconds, linear
			float time_s = static_cast<float>(ImGui::GetTime());
			float angle_rad = std::fmod(time_s, 6.0f) / 6.0f * 2.0f * std::numbers::pi_v<float>;

			// Rotate four corners of the gear image around its center
			ImVec2 gear_center(gear_x + GEAR_SIZE * 0.5f, gear_y + GEAR_SIZE * 0.5f);
			float cos_a = std::cos(angle_rad);
			float sin_a = std::sin(angle_rad);
			float half = GEAR_SIZE * 0.5f;

			// Corners relative to center: TL, TR, BR, BL
			ImVec2 corners[4];
			float offsets[4][2] = { {-half, -half}, {half, -half}, {half, half}, {-half, half} };
			for (int i = 0; i < 4; i++) {
				float rx = offsets[i][0] * cos_a - offsets[i][1] * sin_a;
				float ry = offsets[i][0] * sin_a + offsets[i][1] * cos_a;
				corners[i] = ImVec2(gear_center.x + rx, gear_center.y + ry);
			}
			ImVec2 uvs[4] = { {0,0}, {1,0}, {1,1}, {0,1} };
			dl->AddImageQuad(
				static_cast<ImTextureID>(static_cast<uintptr_t>(s_gearTexture)),
				corners[0], corners[1], corners[2], corners[3],
				uvs[0], uvs[1], uvs[2], uvs[3],
				IM_COL32(255, 255, 255, 255));
		}

		float text_y = gear_y + GEAR_SIZE + GEAR_MARGIN_BOTTOM;
		{
			ImFont* bold = app::theme::getBoldFont();
			ImVec2 title_size = bold->CalcTextSizeA(TITLE_FONT_SIZE, FLT_MAX, 0.0f,
				core::view->loadingTitle.c_str());
			ImVec2 title_pos(center_x - title_size.x * 0.5f, text_y);
			dl->AddText(bold, TITLE_FONT_SIZE, title_pos,
				app::theme::FONT_PRIMARY_U32, core::view->loadingTitle.c_str());
			text_y += title_size.y;
		}

		{
			ImFont* font = ImGui::GetFont();
			ImVec2 prog_size = font->CalcTextSizeA(PROGRESS_FONT_SIZE, FLT_MAX, 0.0f,
				core::view->loadingProgress.c_str());
			ImVec2 prog_pos(center_x - prog_size.x * 0.5f, text_y);
			dl->AddText(font, PROGRESS_FONT_SIZE, prog_pos,
				app::theme::FONT_PRIMARY_U32, core::view->loadingProgress.c_str());
			text_y += prog_size.y;
		}

		if (core::view->loadPct >= 0.0) {
			float bar_x = center_x - BAR_WIDTH * 0.5f;
			float bar_y = text_y + BAR_MARGIN_TOP;

			dl->AddRectFilled(
				ImVec2(bar_x, bar_y),
				ImVec2(bar_x + BAR_WIDTH, bar_y + BAR_HEIGHT),
				app::theme::LOADING_BAR_BG_U32);

			dl->AddRect(
				ImVec2(bar_x, bar_y),
				ImVec2(bar_x + BAR_WIDTH, bar_y + BAR_HEIGHT),
				app::theme::BORDER_U32, 0.0f, 0, 1.0f);

			float fill_w = static_cast<float>(std::clamp(core::view->loadPct, 0.0, 1.0) * BAR_WIDTH);
			if (fill_w > 0.0f) {
				// Vertical gradient: top color → bottom color
				dl->AddRectFilledMultiColor(
					ImVec2(bar_x, bar_y),
					ImVec2(bar_x + fill_w, bar_y + BAR_HEIGHT),
					app::theme::PROGRESS_BAR_TOP_U32,     // top-left
					app::theme::PROGRESS_BAR_TOP_U32,     // top-right
					app::theme::PROGRESS_BAR_BOTTOM_U32,  // bottom-right
					app::theme::PROGRESS_BAR_BOTTOM_U32); // bottom-left
			}
		}

		ImGui::End();
		ImGui::PopStyleColor();
		ImGui::PopStyleVar(2);
	}

	//       <div id="drop-overlay-icon"></div>
	//       <div id="drop-overlay-text">» {{ fileDropPrompt }} «</div>
	//     </div>
	if (core::view && core::view->fileDropPrompt.is_string()) {
		std::string prompt_text = core::view->fileDropPrompt.get<std::string>();
		if (!prompt_text.empty()) {
			ImGui::SetNextWindowPos(vp_pos);
			ImGui::SetNextWindowSize(vp_size);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			ImGui::PushStyleColor(ImGuiCol_WindowBg, app::theme::BG_TRANS);
			ImGui::Begin("##DropOverlay", nullptr,
				ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
				ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
				ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);

			ImDrawList* dl = ImGui::GetWindowDrawList();
			float center_x = vp_pos.x + vp_size.x * 0.5f;
			float center_y = vp_pos.y + vp_size.y * 0.5f;

			//        background-image: url(./fa-icons/copy.svg) }
			constexpr float ICON_SIZE = 100.0f;
			constexpr float ICON_MARGIN_BOTTOM = 20.0f;
			constexpr float TEXT_FONT_SIZE = 25.0f;

			// Calculate total height for vertical centering: icon(100) + margin(20) + text
			ImFont* font = ImGui::GetFont();
			std::string formatted = std::string("\xc2\xbb ") + prompt_text + " \xc2\xab";
			ImVec2 text_size = font->CalcTextSizeA(TEXT_FONT_SIZE, FLT_MAX, 0.0f, formatted.c_str());
			float total_h = ICON_SIZE + ICON_MARGIN_BOTTOM + text_size.y;
			float start_y = center_y - total_h * 0.5f;

			// Render copy icon using Font Awesome icon font
			ImFont* icon_font = app::theme::getIconFont();
			if (icon_font) {
				ImVec2 icon_text_size = icon_font->CalcTextSizeA(ICON_SIZE, FLT_MAX, 0.0f, ICON_FA_COPY);
				ImVec2 icon_pos(center_x - icon_text_size.x * 0.5f, start_y);
				dl->AddText(icon_font, ICON_SIZE, icon_pos, app::theme::FONT_PRIMARY_U32, ICON_FA_COPY);
			}

			// Render prompt text: » {prompt} «
			ImVec2 text_pos(center_x - text_size.x * 0.5f, start_y + ICON_SIZE + ICON_MARGIN_BOTTOM);
			dl->AddText(font, TEXT_FONT_SIZE, text_pos, app::theme::FONT_PRIMARY_U32, formatted.c_str());

			ImGui::End();
			ImGui::PopStyleColor();
			ImGui::PopStyleVar(2);
		}
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


static void glfw_drop_callback(GLFWwindow* /*window*/, int count, const char** paths) {
	if (count <= 0 || !paths)
		return;

	if (!core::view)
		return;

	// JS: core.view.fileDropPrompt = null;
	core::view->fileDropPrompt = nullptr;

	if (core::view->isBusy)
		return;

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

		// JS: handler.process(include) — passes the entire array of matching files.
		if (!include.empty() && handler->process)
			handler->process(include);
	}
	// JS: ondrop does NOT show an error toast for unrecognized files — it silently returns false.
	// Removed the error toast that was previously here to match JS behavior.
}


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
 * JS equivalent: setAllItemTypes(state) iterates this.itemViewerTypeMask
 * @param {boolean} state
 */
static void setAllItemTypes(bool state) {
	tab_items::setAllItemTypes(state);
}

/**
 * Mark all item qualities to the given state.
 * JS equivalent: setAllItemQualities(state) iterates this.itemViewerQualityMask
 * @param {boolean} state
 */
static void setAllItemQualities(bool state) {
	tab_items::setAllItemQualities(state);
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
 * JS equivalent: click(tag, event, ...params) — checks disabled before emitting.
 * In ImGui, disabled state is handled by BeginDisabled()/EndDisabled() so
 * the disabled parameter is provided for API fidelity but is rarely needed.
 * @param {string} tag
 * @param {bool} disabled  If true, skip the emit (mirrors JS disabled check).
 */
static void click(const std::string& tag, bool disabled = false) {
	if (!disabled)
		core::events.emit("click-" + tag);
}

/**
 * Emit a click event with a typed argument.
 * JS equivalent: click(tag, event, ...params)
 */
static void click(const std::string& tag, bool disabled, const std::any& arg) {
	if (!disabled)
		core::events.emit("click-" + tag, arg);
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
 * Emit an event with a typed argument.
 * JS equivalent: emit(tag, ...params)
 */
static void emit(const std::string& tag, const std::any& arg) {
	core::events.emit(tag, arg);
}

/**
 * Restart the application.
 * JS equivalent: chrome.runtime.reload() — reloads the NW.js app.
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

/**
 * Returns a reference to the ExternalLinks module.
 * JS equivalent: getExternalLink() returns the ExternalLinks class.
 * In C++, callers use ExternalLinks::open(), ExternalLinks::resolve(), etc.
 * directly via the ExternalLinks namespace (external-links.h).
 * This function is provided for API symmetry with the JS version; it returns
 * the STATIC_LINKS map as a convenience.
 * @returns Reference to the static links map.
 */
static const std::unordered_map<std::string, std::string>& getExternalLink() {
	return ExternalLinks::STATIC_LINKS;
}


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

static constexpr float DPI_SCALE_EPSILON = 0.01f;  // tolerance for DPI change detection
static float clampDpiScale(float raw) {
	if (raw <= 0.0f) raw = 1.0f;
	return (std::max)(0.5f, (std::min)(raw, 4.0f));
}


namespace app::theme {

void applyTheme() {
	ImGuiStyle& style = ImGui::GetStyle();

	ImVec4* colors = style.Colors;

	// Window backgrounds
	colors[ImGuiCol_WindowBg]  = BG;
	colors[ImGuiCol_ChildBg]   = BG_ALT;
	colors[ImGuiCol_PopupBg]   = BG_DARK;
	colors[ImGuiCol_MenuBarBg] = BG_DARK;

	// Borders
	colors[ImGuiCol_Border]       = BORDER;
	colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

	// Text
	colors[ImGuiCol_Text]         = FONT_PRIMARY;
	colors[ImGuiCol_TextDisabled] = FONT_FADED;

	// Frame (input fields, checkboxes, sliders)
	colors[ImGuiCol_FrameBg]        = BG_ALT;
	colors[ImGuiCol_FrameBgHovered] = ImVec4(BG_ALT.x * 1.2f, BG_ALT.y * 1.2f, BG_ALT.z * 1.2f, 1.0f);
	colors[ImGuiCol_FrameBgActive]  = ImVec4(BG_ALT.x * 1.4f, BG_ALT.y * 1.4f, BG_ALT.z * 1.4f, 1.0f);

	// Title bar
	colors[ImGuiCol_TitleBg]          = BG_DARK;
	colors[ImGuiCol_TitleBgActive]    = BG_DARK;
	colors[ImGuiCol_TitleBgCollapsed] = BG_DARK;

	// Scrollbar
	colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	colors[ImGuiCol_ScrollbarGrab]        = BORDER;
	colors[ImGuiCol_ScrollbarGrabHovered] = FONT_HIGHLIGHT;
	colors[ImGuiCol_ScrollbarGrabActive]  = FONT_HIGHLIGHT;

	colors[ImGuiCol_Button]        = BUTTON_BASE;
	colors[ImGuiCol_ButtonHovered] = BUTTON_HOVER;
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
	colors[ImGuiCol_TabSelected]        = BUTTON_BASE;
	colors[ImGuiCol_TabSelectedOverline]= NAV_SELECTED;
	colors[ImGuiCol_TabDimmed]          = BG_DARK;
	colors[ImGuiCol_TabDimmedSelected]  = BG_ALT;

	// Check mark
	colors[ImGuiCol_CheckMark] = BUTTON_BASE;

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

	style.WindowRounding    = WINDOW_ROUNDING;  // 0 — sharp corners
	style.FrameRounding     = FRAME_ROUNDING;   // 5px input border-radius
	style.GrabRounding      = FRAME_ROUNDING;
	style.PopupRounding     = POPUP_ROUNDING;   // 0 — sharp popup corners
	style.ScrollbarRounding = SCROLLBAR_ROUNDING; // 5px thumb rounding
	style.TabRounding       = 0.0f;
	style.ChildRounding     = 0.0f;

	style.ScrollbarSize     = SCROLLBAR_SIZE;   // 8px width
	style.FramePadding      = ImVec2(6.0f, 4.0f);
	style.ItemSpacing       = ImVec2(8.0f, 6.0f);
	style.WindowPadding     = ImVec2(8.0f, 8.0f);

	// Button-specific padding is applied via PushStyleVar where needed,

	style.WindowBorderSize  = 1.0f;
	style.FrameBorderSize   = 0.0f;
	style.PopupBorderSize   = 1.0f;

	style.DisabledAlpha     = 0.5f;
}

// Loads Selawik (regular + bold), Gambler, and Font Awesome icon fonts from data/fonts/.
// @font-face { font-family: "Selawik"; font-weight: bold; src: url("fonts/selawkb.woff2"); }
// @font-face { font-family: "Gambler"; src: url("fonts/gmblr.woff2"); }

static ImFont* s_fontBold    = nullptr;
static ImFont* s_fontGambler = nullptr;
static ImFont* s_fontIcon    = nullptr;
static float   s_dpiScale    = 1.0f;   // current DPI scale fonts are built at

// Font Awesome glyph range (static storage, must persist while font is alive).
static const ImWchar s_iconRanges[] = { ICON_FA_MIN, ICON_FA_MAX, 0 };

void loadFonts(float dpiScale) {
	ImGuiIO& io = ImGui::GetIO();

	dpiScale = clampDpiScale(dpiScale);
	s_dpiScale = dpiScale;

	// Scale font pixel sizes by DPI so glyphs are rasterized at native
	// resolution. FontGlobalScale is set to 1/dpiScale by the caller so
	// logical sizes remain the same as on a 1× display.
	const float scaledDefault = DEFAULT_FONT_SIZE * dpiScale;
	const float scaledIcon    = 48.0f * dpiScale;

	std::filesystem::path fontsDir = constants::DATA_DIR() / "fonts";
	std::string regularPath = (fontsDir / "selawk.ttf").string();
	std::string boldPath    = (fontsDir / "selawkb.ttf").string();
	std::string gamblerPath = (fontsDir / "gmblr.ttf").string();
	std::string iconPath    = (fontsDir / "fa-solid-900.ttf").string();

	ImFont* regularFont = io.Fonts->AddFontFromFileTTF(regularPath.c_str(), scaledDefault);
	if (!regularFont) {
		regularFont = io.Fonts->AddFontDefault();
	}

	// Merge Font Awesome icon font into the default (Selawik regular) font.
	// This lets us use icon codepoints inline with regular text.
	{
		ImFontConfig iconCfg;
		iconCfg.MergeMode = true;
		iconCfg.GlyphMinAdvanceX = scaledDefault; // Make icons monospaced
		io.Fonts->AddFontFromFileTTF(iconPath.c_str(), scaledDefault, &iconCfg, s_iconRanges);
	}

	s_fontBold = io.Fonts->AddFontFromFileTTF(boldPath.c_str(), scaledDefault);
	if (!s_fontBold) {
		// Fallback to the regular/default font.
		s_fontBold = regularFont;
	}

	s_fontGambler = io.Fonts->AddFontFromFileTTF(gamblerPath.c_str(), scaledDefault);
	if (!s_fontGambler) {
		s_fontGambler = regularFont;
	}

	// Load a standalone icon font at a larger size for nav icons and header buttons.
	// This is used with CalcTextSizeA/AddText to render icons at various sizes.
	// We load at 48px * dpiScale (larger than any display size) so downscaling stays sharp.
	s_fontIcon = io.Fonts->AddFontFromFileTTF(iconPath.c_str(), scaledIcon, nullptr, s_iconRanges);
	if (!s_fontIcon) {
		s_fontIcon = regularFont;
	}

	// automatically on the first frame — no manual Build() call needed.
}

ImFont* getBoldFont() {
	return s_fontBold ? s_fontBold : ImGui::GetFont();
}

ImFont* getGamblerFont() {
	return s_fontGambler ? s_fontGambler : ImGui::GetFont();
}

ImFont* getIconFont() {
	return s_fontIcon ? s_fontIcon : ImGui::GetFont();
}

void rebuildFontsForScale(float dpiScale) {
	ImGuiIO& io = ImGui::GetIO();

	// Clear the existing atlas and font pointers.
	io.Fonts->Clear();
	s_fontBold    = nullptr;
	s_fontGambler = nullptr;
	s_fontIcon    = nullptr;

	// Reload all fonts at the new DPI scale.
	loadFonts(dpiScale);

	// font atlas GPU texture is rebuilt automatically on the next frame.
}

float getDpiScale() {
	return s_dpiScale;
}

// Maps SVG icon filenames used by nav buttons and context menus to their
// corresponding Font Awesome 6 Solid UTF-8 codepoints.
// and will continue to render via the SVG texture pipeline.

static const std::unordered_map<std::string, const char*> s_iconMapping = {
	{ "arrow-left.svg",                ICON_FA_ARROW_LEFT },
	{ "arrow-right.svg",               ICON_FA_ARROW_RIGHT },
	{ "arrow-rotate-left.svg",         ICON_FA_ARROW_ROTATE_LEFT },
	{ "ban.svg",                       ICON_FA_BAN },
	{ "bug.svg",                       ICON_FA_BUG },
	{ "caret-down.svg",                ICON_FA_CARET_DOWN },
	{ "check.svg",                     ICON_FA_CHECK },
	{ "circle-info.svg",               ICON_FA_CIRCLE_INFO },
	{ "clipboard-list.svg",            ICON_FA_CLIPBOARD_LIST },
	{ "copy.svg",                      ICON_FA_COPY },
	{ "cube.svg",                      ICON_FA_CUBE },
	{ "database.svg",                  ICON_FA_DATABASE },
	{ "export.svg",                    ICON_FA_FILE_EXPORT },
	{ "file-lines.svg",                ICON_FA_FILE_LINES },
	{ "film.svg",                      ICON_FA_FILM },
	{ "fish.svg",                      ICON_FA_FISH },
	{ "font.svg",                      ICON_FA_FONT },
	{ "gear.svg",                      ICON_FA_GEAR },
	{ "help.svg",                      ICON_FA_CIRCLE_QUESTION },
	{ "house.svg",                     ICON_FA_HOUSE },
	{ "image.svg",                     ICON_FA_IMAGE },
	{ "import.svg",                    ICON_FA_FILE_IMPORT },
	{ "line-columns.svg",              ICON_FA_BARS },
	{ "list.svg",                      ICON_FA_LIST },
	{ "map.svg",                       ICON_FA_MAP },
	{ "music.svg",                     ICON_FA_MUSIC },
	{ "palette.svg",                   ICON_FA_PALETTE },
	{ "pause.svg",                     ICON_FA_PAUSE },
	{ "person-solid.svg",              ICON_FA_PERSON },
	{ "play.svg",                      ICON_FA_PLAY },
	{ "save.svg",                      ICON_FA_FLOPPY_DISK },
	{ "search.svg",                    ICON_FA_MAGNIFYING_GLASS },
	{ "sort.svg",                      ICON_FA_SORT },
	{ "sort_down.svg",                 ICON_FA_SORT_DOWN },
	{ "sort_up.svg",                   ICON_FA_SORT_UP },
	{ "timeline.svg",                  ICON_FA_TIMELINE },
	{ "timer.svg",                     ICON_FA_STOPWATCH },
	{ "trash.svg",                     ICON_FA_TRASH },
	{ "triangle-exclamation.svg",      ICON_FA_TRIANGLE_EXCLAMATION },
	{ "triangle-exclamation-white.svg", ICON_FA_TRIANGLE_EXCLAMATION },
	{ "volume.svg",                    ICON_FA_VOLUME_HIGH },
	{ "xmark.svg",                     ICON_FA_XMARK },
};

const char* getIconForFilename(const std::string& svg_filename) {
	auto it = s_iconMapping.find(svg_filename);
	if (it != s_iconMapping.end())
		return it->second;
	return nullptr;
}

GLuint loadSvgTexture(const std::filesystem::path& path, int size) {
	return ::loadSvgTexture(path, size);
}

GLuint loadImageTexture(const std::filesystem::path& path, int* out_w, int* out_h) {
	return ::loadImageTexture(path, out_w, out_h);
}

GLuint loadWebPTexture(const std::filesystem::path& path, int* out_w, int* out_h) {
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

	if (out_w) *out_w = w;
	if (out_h) *out_h = h;
	return tex;
}


// Expansion ID → icon filename mapping.
static const char* s_expansionIconFiles[] = {
	"icon_classic.webp",   // 0: Classic
	"icon_tbc.webp",       // 1: TBC
	"icon_wotlk.webp",     // 2: WotLK
	"icon_cata.webp",      // 3: Cataclysm
	"icon_mop.webp",       // 4: MoP
	"icon_wod.webp",       // 5: WoD
	"icon_legion.webp",    // 6: Legion
	"icon_bfa.webp",       // 7: BfA
	"icon_slands.webp",    // 8: Shadowlands
	"icon_df.webp",        // 9: Dragonflight
	"icon_tww.webp",       // 10: TWW
	"icon_midnight.webp",  // 11: Midnight
	"icon_tlt.webp",       // 12: TLT
};
static constexpr int s_expansionIconCount = static_cast<int>(std::size(s_expansionIconFiles));
static GLuint s_expansionIconTextures[13] = {};
static bool s_expansionIconsLoaded = false;

static void loadExpansionIcons() {
	if (s_expansionIconsLoaded)
		return;
	s_expansionIconsLoaded = true;

	std::filesystem::path iconDir = constants::DATA_DIR() / "images" / "expansion";
	for (int i = 0; i < s_expansionIconCount; ++i) {
		s_expansionIconTextures[i] = loadWebPTexture(iconDir / s_expansionIconFiles[i]);
	}
}

GLuint getExpansionIconTexture(int expansionId) {
	loadExpansionIcons();
	if (expansionId < 0 || expansionId >= s_expansionIconCount)
		return 0;
	return s_expansionIconTextures[expansionId];
}

void renderExpansionFilterButtons(int& selectedFilter, int expansionCount) {
	loadExpansionIcons();

	constexpr float BTN_SIZE = 30.0f;
	constexpr float GAP = 8.0f;

	const ImU32 normalBg   = IM_COL32(0, 0, 0, 71);   // rgb(0 0 0 / 28%)
	const ImU32 activeBg   = IM_COL32(0, 0, 0, 204);   // rgba(0,0,0,0.8)
	const ImU32 hoveredBg  = IM_COL32(0, 0, 0, 204);

	ImGui::Dummy(ImVec2(0, 10.0f));

	// Center the row: total width = (count+1) * BTN_SIZE + count * GAP
	int totalBtns = 1 + expansionCount; // "All" + per-expansion
	float totalWidth = totalBtns * BTN_SIZE + (totalBtns - 1) * GAP;
	float indent = (ImGui::GetContentRegionAvail().x - totalWidth) * 0.5f;
	if (indent > 0.0f)
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + indent);

	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(GAP, 0));

	// "Show All" button — uses a ban/slash icon.
	{
		bool isActive = (selectedFilter == -1);
		ImU32 bg = isActive ? activeBg : normalBg;
		ImGui::PushStyleColor(ImGuiCol_Button,        ImGui::ColorConvertU32ToFloat4(bg));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImGui::ColorConvertU32ToFloat4(hoveredBg));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ImGui::ColorConvertU32ToFloat4(activeBg));

		if (ImGui::Button(ICON_FA_BAN "##exp_all_icon", ImVec2(BTN_SIZE, BTN_SIZE)))
			selectedFilter = -1;

		ImGui::PopStyleColor(3);
	}

	// Per-expansion buttons
	for (int i = 0; i < expansionCount; ++i) {
		ImGui::SameLine();
		bool isActive = (selectedFilter == i);
		ImU32 bg = isActive ? activeBg : normalBg;
		ImGui::PushStyleColor(ImGuiCol_Button,        ImGui::ColorConvertU32ToFloat4(bg));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImGui::ColorConvertU32ToFloat4(hoveredBg));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ImGui::ColorConvertU32ToFloat4(activeBg));

		GLuint tex = getExpansionIconTexture(i);
		if (tex != 0) {
			ImGui::PushID(i);
			if (ImGui::ImageButton("##exp", static_cast<ImTextureID>(static_cast<uintptr_t>(tex)),
			                       ImVec2(24.0f, 24.0f))) {
				selectedFilter = i;
			}
			ImGui::PopID();
		} else {
			// Fallback text button if icon texture failed to load.
			if (ImGui::Button(std::format("E{}##exp_{}", i, i).c_str(), ImVec2(BTN_SIZE, BTN_SIZE)))
				selectedFilter = i;
		}

		ImGui::PopStyleColor(3);
	}

	ImGui::PopStyleVar(3);
}

} // namespace app::theme


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
	// JS: app.js lines 556-563 — dynamically iterates all contextMenu entries.
	if (core::view->activeModule != prevActiveModule) {
		prevActiveModule = core::view->activeModule;
		core::view->contextMenus.resetAll();
	}
}


app::layout::ListTabRegions app::layout::CalcListTabRegions(bool hasSidebar, float colRatio) {
	ListTabRegions r{};
	r.hasSidebar = hasSidebar;

	const ImVec2 avail = ImGui::GetContentRegionAvail();
	const ImVec2 cursor = ImGui::GetCursorPos();

	// Reserve sidebar width from the right edge if present.
	const float sidebarW = hasSidebar ? SIDEBAR_WIDTH : 0.0f;
	const float gridW = avail.x - sidebarW;
	const float gridH = avail.y;

	// Two columns: left = colRatio, right = 1-colRatio
	const float leftColW = gridW * colRatio;
	const float rightColW = gridW * (1.0f - colRatio);

	// Two rows: row 1 = remaining, row 2 = FILTER_BAR_HEIGHT
	const float bottomH = FILTER_BAR_HEIGHT;
	const float topH = gridH - bottomH;

	// .list-container { margin: 20px 10px 0 20px }
	r.listPos = ImVec2(cursor.x + LIST_MARGIN_LEFT, cursor.y + LIST_MARGIN_TOP);
	r.listSize = ImVec2(
		leftColW - LIST_MARGIN_LEFT - LIST_MARGIN_RIGHT,
		topH - LIST_MARGIN_TOP - LIST_MARGIN_BOTTOM
	);

	// .preview-container { margin: 20px 20px 0 10px; grid-row: 1; grid-column: 2 }
	r.previewPos = ImVec2(cursor.x + leftColW + PREVIEW_MARGIN_LEFT, cursor.y + PREVIEW_MARGIN_TOP);
	r.previewSize = ImVec2(
		rightColW - PREVIEW_MARGIN_LEFT - PREVIEW_MARGIN_RIGHT,
		topH - PREVIEW_MARGIN_TOP - PREVIEW_MARGIN_BOTTOM
	);

	// .filter { display: flex; align-items: center; grid-column: 1; grid-row: 2 }
	r.filterPos = ImVec2(cursor.x, cursor.y + topH);
	r.filterSize = ImVec2(leftColW, bottomH);

	// .preview-controls { display: flex; justify-content: flex-end; grid-column: 2; grid-row: 2 }
	r.controlsPos = ImVec2(cursor.x + leftColW, cursor.y + topH);
	r.controlsSize = ImVec2(rightColW, bottomH);

	if (hasSidebar) {
		// .sidebar { grid-column: 3; width: 210px; grid-row: 1/span 2; margin-top: 20px; padding-right: 20px }
		r.sidebarPos = ImVec2(cursor.x + gridW, cursor.y + SIDEBAR_MARGIN_TOP);
		r.sidebarSize = ImVec2(
			SIDEBAR_WIDTH - SIDEBAR_PADDING_RIGHT,
			gridH - SIDEBAR_MARGIN_TOP
		);
	}

	return r;
}

bool app::layout::BeginTab(const char* id) {
	// .tab { position: absolute; top: 0; left: 0; right: 0; bottom: 0 }
	// Fills the entire content region, no borders, no scrollbar by default.
	ImVec2 avail = ImGui::GetContentRegionAvail();
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	bool visible = ImGui::BeginChild(id, avail, ImGuiChildFlags_None,
	    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	ImGui::PopStyleVar();
	return visible;
}

void app::layout::EndTab() {
	ImGui::EndChild();
}

bool app::layout::BeginListContainer(const char* id, const ListTabRegions& regions) {
	ImGui::SetCursorPos(regions.listPos);
	return ImGui::BeginChild(id, regions.listSize, ImGuiChildFlags_None);
}

void app::layout::EndListContainer() {
	ImGui::EndChild();
}

bool app::layout::BeginPreviewContainer(const char* id, const ListTabRegions& regions) {
	ImGui::SetCursorPos(regions.previewPos);
	return ImGui::BeginChild(id, regions.previewSize, ImGuiChildFlags_None);
}

void app::layout::EndPreviewContainer() {
	ImGui::EndChild();
}

bool app::layout::BeginFilterBar(const char* id, const ListTabRegions& regions) {
	ImGui::SetCursorPos(regions.filterPos);

	// Vertically center content within the filter bar.
	// .filter input { margin: 0 10px 0 20px } — apply left/right margins.
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 0.0f));
	bool visible = ImGui::BeginChild(id, regions.filterSize, ImGuiChildFlags_None,
	    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

	// Vertically center: push cursor to (height - item_height) / 2
	if (visible) {
		float itemH = ImGui::GetFrameHeight();
		float padY = (regions.filterSize.y - itemH) * 0.5f;
		if (padY > 0.0f)
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + padY);
	}

	ImGui::PopStyleVar();
	return visible;
}

void app::layout::EndFilterBar() {
	ImGui::EndChild();
}

bool app::layout::BeginPreviewControls(const char* id, const ListTabRegions& regions) {
	ImGui::SetCursorPos(regions.controlsPos);

	// .preview-controls { margin-right: 20px }
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	bool visible = ImGui::BeginChild(id, regions.controlsSize, ImGuiChildFlags_None,
	    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	ImGui::PopStyleVar();

	if (visible) {
		// Vertically center content within the controls bar.
		float itemH = ImGui::GetFrameHeight();
		float padY = (regions.controlsSize.y - itemH) * 0.5f;
		if (padY > 0.0f)
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + padY);
	}

	return visible;
}

void app::layout::EndPreviewControls() {
	ImGui::EndChild();
}

bool app::layout::BeginSidebar(const char* id, const ListTabRegions& regions) {
	if (!regions.hasSidebar)
		return false;

	ImGui::SetCursorPos(regions.sidebarPos);
	return ImGui::BeginChild(id, regions.sidebarSize, ImGuiChildFlags_None);
}

void app::layout::EndSidebar() {
	ImGui::EndChild();
}


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

	// Register crash handlers.
	// JS: process.on('unhandledRejection', e => crash('ERR_UNHANDLED_REJECTION', e.message));
	// JS: process.on('uncaughtException', e => crash('ERR_UNHANDLED_EXCEPTION', e.message));
	std::set_terminate(terminateHandler);
	std::signal(SIGSEGV, fatalSignalHandler);
	std::signal(SIGABRT, fatalSignalHandler);
	std::signal(SIGFPE, fatalSignalHandler);
	std::signal(SIGILL, fatalSignalHandler);
#ifdef _WIN32
	SetUnhandledExceptionFilter(unhandledSEHFilter);
#endif

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
	std::string windowTitle = std::format("wow.export.cpp v{}", constants::VERSION);
	GLFWwindow* window = glfwCreateWindow(1280, 720, windowTitle.c_str(), nullptr, nullptr);
	if (!window) {
		crash("ERR_WINDOW_CREATE", "Failed to create GLFW window");
		glfwTerminate();
		return 1;
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

#ifdef _WIN32
	// Initialize Windows taskbar progress (ITaskbarList3).
	initTaskbarProgress();
	// JS: win.setProgressBar(-1); // Reset taskbar progress in-case it's stuck.
	setTaskbarProgress(window, -1);
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

	// Force all links to open in the users default application.

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	// Apply the app theme.
	app::theme::applyTheme();

	// Query the initial display content scale for high-DPI support.
	// On standard displays this is 1.0; on Retina / 200% displays it is 2.0.
	// backend is not yet initialized at this point.
	float initialDpiScale;
	{
		float xscale = 1.0f, yscale = 1.0f;
		glfwGetWindowContentScale(window, &xscale, &yscale);
		initialDpiScale = clampDpiScale(xscale);
	}

	// Load custom fonts (Selawik regular/bold, Gambler) from data/fonts/.
	// Fonts are rasterized at DEFAULT_FONT_SIZE * dpiScale for crisp rendering.
	// Must be done after CreateContext and before the first NewFrame.
	app::theme::loadFonts(initialDpiScale);

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 460");

	// Load app shell textures (logo, SVG icons) now that OpenGL is ready.
	initAppShellTextures();


	// Initialize Vue equivalent: create AppState and assign to core::view.
	static AppState appState = core::makeNewView();
	core::view = &appState;

	// Interlink error handling for Vue.

	modules::register_components();

	// Dynamic interface scaling thresholds matching the original JS logic.
	//   scale = min(window.innerWidth / 1120, window.innerHeight / 700, 1.0)
	constexpr int SCALE_THRESHOLD_W = 1120;
	constexpr int SCALE_THRESHOLD_H = 700;

	modules::initialize();

	// register static context menu options
	// JS: app.js lines 548-553 — register static context menu options.
	// Note: "Settings" / "Manage Settings" is registered by screen_settings module, not here.
	modules::registerContextMenuOption("runtime-log", "Open Runtime Log", "timeline.svg", []() { logging::openRuntimeLog(); });
	modules::registerContextMenuOption("restart", "Restart wow.export.cpp", "arrow-rotate-left.svg", []() { app::restartApplication(); });
	modules::registerContextMenuOption("reload-style", "Reload Styling", "palette.svg", []() { app::reloadStylesheet(); }, true);
	modules::registerContextMenuOption("reload-shaders", "Reload Shaders", "cube.svg", []() { shaders::reload_all(); }, true);
	modules::registerContextMenuOption("reload-active", "Reload Active Module", "gear.svg", []() { modules::reloadActiveModule(); }, true);
	modules::registerContextMenuOption("reload-all", "Reload All Modules", "gear.svg", []() { modules::reloadAllModules(); }, true);

	// Log some basic information for potential diagnostics.
	logging::write(std::format("wow.export.cpp has started v{}", constants::VERSION));
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
	// JS: window.ondragenter, window.ondrop, window.ondragleave (app.js lines 589-660)
	// GLFW limitation: GLFW only provides a drop callback — there are no drag-enter
	// or drag-leave callbacks. This means the file drop prompt overlay (showing
	// "Export N files as ..." before the user releases) cannot be shown proactively.
	// The JS uses a dropStack counter to track enter/leave events and shows the
	// prompt during drag-over. In the C++ port, the prompt is only cleared on drop.
	// This is a known GLFW platform limitation; implementing drag-enter/leave would
	// require platform-specific native APIs (Win32 OLE D&D, X11/Wayland protocols).
	glfwSetDropCallback(window, glfw_drop_callback);

	loadCacheSize();

	// Load/update BLTE decryption keys.
	casc::tact_keys::load();

	// Auto-updater logic.
	// JS: app.js lines 688-705
	// if (BUILD_RELEASE && !DISABLE_AUTO_UPDATE) {
	//     core.showLoadingScreen(1, 'Checking for updates...');
	//     updater.checkForUpdates().then(updateAvailable => {
	//         if (updateAvailable) updater.applyUpdate();
	//         else { core.hideLoadingScreen(); modules.source_select.setActive(); }
	//     });
	// } else { modules.tab_blender.checkLocalVersion(); }
	//
	// The C++ updater module has not been ported yet. When it is, uncomment and
	// adapt the logic above. For now, we skip directly to source_select.
	// TODO: Port src/js/updater.js to C++ (see TODO_TRACKER.md entry #34+).

	// JS: app.js lines 699, 704 — check Blender add-on version after update check.
	// The tab_blender module has not been ported to C++ yet. When it is, call
	// tab_blender::checkLocalVersion() here. See src/js/modules/tab_blender.js:127-170.
	// TODO: Port tab_blender module to C++ and enable checkLocalVersion() call.

	// Load what's new HTML on app start
	// JS: app.js lines 707-716
	{
		auto whatsNewPath = constants::DATA_DIR() / "whats-new.html";
		try {
			std::ifstream ifs(whatsNewPath, std::ios::in);
			if (ifs.is_open()) {
				std::string html((std::istreambuf_iterator<char>(ifs)),
				                  std::istreambuf_iterator<char>());
				core::view->whatsNewHTML = std::move(html);
			} else {
				logging::write(std::format("failed to load whats-new.html: could not open {}", whatsNewPath.string()));
			}
		} catch (const std::exception& e) {
			logging::write(std::format("failed to load whats-new.html: {}", e.what()));
		}
	}

	// Set source select as the currently active interface screen.
	modules::setActive("source_select");

	// Initialize watch state trackers to current values.
	prevLoadPct = core::view->loadPct;
	prevCasc = static_cast<void*>(core::view->casc);
	prevActiveModule = core::view->activeModule;


	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		try {
			// Drain the main-thread task queue.  Background threads (e.g.
			// CASC loading) post tasks here so that all shared-state
			// mutations happen on the main thread.
			core::drainMainThreadQueue();

			// Debugging reloader.
			if (!BUILD_RELEASE) {
				if (glfwGetKey(window, GLFW_KEY_F5) == GLFW_PRESS) {
					// In NW.js, F5 reloads the app via chrome.runtime.reload().
					app::restartApplication();
				}
			}

			// 1) If the display DPI scale changed (e.g. window moved to a
			//    different monitor), rebuild the font atlas at the new scale.
			// 2) Compute a window-size scale factor that shrinks the UI when
			//    the window is smaller than 1120×700 but never exceeds 1.0.
			// 3) Combine both: FontGlobalScale = windowScale / dpiScale.
			//    The division by dpiScale compensates for the fonts being
			//    rasterized at dpiScale× so that logical sizes stay correct.
			{
				float dpiScale = clampDpiScale(
					ImGui_ImplGlfw_GetContentScaleForWindow(window));

				// Rebuild fonts when DPI changes (moving between monitors).
				if (std::abs(dpiScale - app::theme::getDpiScale()) > DPI_SCALE_EPSILON) {
					app::theme::rebuildFontsForScale(dpiScale);
				}

				// Window-size scale: scale down when smaller than thresholds, never up.
				int win_w, win_h;
				glfwGetWindowSize(window, &win_w, &win_h);
				float scale_w = win_w < SCALE_THRESHOLD_W ? static_cast<float>(win_w) / SCALE_THRESHOLD_W : 1.0f;
				float scale_h = win_h < SCALE_THRESHOLD_H ? static_cast<float>(win_h) / SCALE_THRESHOLD_H : 1.0f;
				float windowScale = (std::min)(scale_w, scale_h);

				// FontGlobalScale = windowScale / dpiScale.
				// Fonts are rasterized at baseSize * dpiScale; dividing by
				// dpiScale brings them back to logical baseSize, then
				// windowScale applies the small-window shrink.
				io.FontGlobalScale = windowScale / dpiScale;
			}

			// Check watchers (loadPct, casc, activeModule)
			checkWatchers(window);

			// Check cache size update timer
			checkCacheSizeUpdate();

			// Start the Dear ImGui frame
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			if (isCrashed) {
				renderCrashScreen();
			} else {
				// Render the app shell (header / content / footer) with the active
				// module rendered inside the content area.
				renderAppShell();
			}
		} catch (const std::exception& e) {
			crash("ERR_UNHANDLED_EXCEPTION", e.what());
		} catch (...) {
			crash("ERR_UNHANDLED_EXCEPTION", "Non-standard exception in main loop");
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