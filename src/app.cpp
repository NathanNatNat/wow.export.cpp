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
#include <dwmapi.h>
#include <shobjidl.h>
#include <shellapi.h>
#include <ole2.h>
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
#include "js/casc/build-cache.h"
#include "js/casc/export-helper.h"
#include "js/ui/texture-ribbon.h"
#include "js/3D/Shaders.h"
#include "js/gpu-info.h"
#include "js/modules.h"
#include "js/modules/tab_textures.h"
#include "js/modules/tab_items.h"
#include "js/modules/tab_blender.h"

#ifdef BUILD_RELEASE
static constexpr bool IS_RELEASE_BUILD = true;
#else
static constexpr bool IS_RELEASE_BUILD = false;
#endif

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

static GLuint s_loadingBgTexture = 0;
static int s_loadingBgWidth = 0;
static int s_loadingBgHeight = 0;
static GLuint s_loadingXmasBgTexture = 0;
static int s_loadingXmasBgWidth = 0;
static int s_loadingXmasBgHeight = 0;
static GLuint s_gearTexture = 0;
static int s_gearTexWidth = 0;
static int s_gearTexHeight = 0;

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

static void initAppShellTextures() {
	std::filesystem::path srcDir = constants::SRC_DIR();

	s_logoTexture = loadImageTexture(srcDir / "images" / "logo.png", &s_logoWidth, &s_logoHeight);
	if (!s_logoTexture)
		logging::write("warning: failed to load logo.png for header");

	s_loadingBgTexture = loadImageTexture(srcDir / "images" / "loading.gif",
	                                      &s_loadingBgWidth, &s_loadingBgHeight);
	if (!s_loadingBgTexture)
		logging::write("warning: failed to load loading.gif for loading screen");

	s_loadingXmasBgTexture = loadImageTexture(srcDir / "images" / "loading-xmas.gif",
	                                          &s_loadingXmasBgWidth, &s_loadingXmasBgHeight);
	if (!s_loadingXmasBgTexture)
		logging::write("warning: failed to load loading-xmas.gif for loading screen");

	s_gearTexture = loadSvgTexture(srcDir / "fa-icons" / "gear.svg", 100);
	if (s_gearTexture) {
		s_gearTexWidth = 100;
		s_gearTexHeight = 100;
	} else {
		logging::write("warning: failed to load gear.svg for loading screen");
	}
}

static void destroyAppShellTextures() {
	if (s_logoTexture) { glDeleteTextures(1, &s_logoTexture); s_logoTexture = 0; }
	if (s_loadingBgTexture) { glDeleteTextures(1, &s_loadingBgTexture); s_loadingBgTexture = 0; }
	if (s_loadingXmasBgTexture) { glDeleteTextures(1, &s_loadingXmasBgTexture); s_loadingXmasBgTexture = 0; }
	if (s_gearTexture) { glDeleteTextures(1, &s_gearTexture); s_gearTexture = 0; }
}

static std::unordered_map<std::string, GLuint> s_navIconTextures;

static GLuint getNavIconTexture(const std::string& icon_filename) {
	auto it = s_navIconTextures.find(icon_filename);
	if (it != s_navIconTextures.end())
		return it->second;

	std::filesystem::path path = constants::SRC_DIR() / "fa-icons" / icon_filename;
	GLuint tex = loadSvgTexture(path, 44);
	s_navIconTextures[icon_filename] = tex;
	return tex;
}

static void crash(const std::string& errorCode, const std::string& errorText);

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
	std::signal(SIGABRT, SIG_DFL);
	std::abort();
}

static void fatalSignalHandler(int sig) {
	const char* sigName = "UNKNOWN";
	switch (sig) {
		case SIGSEGV: sigName = "SIGSEGV (Segmentation fault)"; break;
		case SIGABRT: sigName = "SIGABRT (Abort)"; break;
		case SIGFPE:  sigName = "SIGFPE (Floating-point exception)"; break;
		case SIGILL:  sigName = "SIGILL (Illegal instruction)"; break;
	}
	crash("ERR_FATAL_SIGNAL", std::string("Fatal signal: ") + sigName);
	std::signal(sig, SIG_DFL);
	std::raise(sig);
}

#ifdef _WIN32
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
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(50.0f, 50.0f));
	ImGui::Begin("##CrashScreen", nullptr,
		ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoBringToFrontOnFocus);
	ImGui::PopStyleVar();

	if (s_logoTexture) {
		float logo_w = static_cast<float>(s_logoWidth);
		float logo_h = static_cast<float>(s_logoHeight);
		ImVec2 wp = viewport->WorkPos;
		ImVec2 ws = viewport->WorkSize;
		ImVec2 logo_min(
			wp.x + (ws.x - logo_w) * 0.5f,
			wp.y + (ws.y - logo_h) * 0.5f);
		ImVec2 logo_max(logo_min.x + logo_w, logo_min.y + logo_h);
		ImU32 watermark_tint = IM_COL32(255, 255, 255, 13);
		ImGui::GetWindowDrawList()->AddImage(
			static_cast<ImTextureID>(static_cast<uintptr_t>(s_logoTexture)),
			logo_min, logo_max, ImVec2(0, 0), ImVec2(1, 1), watermark_tint);
	}

	{
		ImFont* icon_font = app::theme::getIconFont();
		if (icon_font) {
			const float iconSize = 24.0f;
			ImVec2 cursorPos = ImGui::GetCursorScreenPos();
			ImGui::GetWindowDrawList()->AddText(icon_font, iconSize,
				ImVec2(cursorPos.x, cursorPos.y + 2.0f),
				ImGui::GetColorU32(ImGuiCol_Text), ICON_FA_TRIANGLE_EXCLAMATION);
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 50.0f);
		}
	}
	ImGui::Text("Oh no! The k\xc4\x81k\xc4\x81p\xc5\x8d has exploded...");
	ImGui::Separator();

	ImGui::TextDisabled("v%s", std::string(constants::VERSION).c_str());
	ImGui::SameLine();
	ImGui::TextDisabled("%s", std::string(constants::FLAVOUR).c_str());
	ImGui::SameLine();
	ImGui::TextDisabled("[%s]", constants::BUILD_GUID.data());

	ImGui::Dummy(ImVec2(0.0f, 20.0f));
	{
		ImFont* bold = app::theme::getBoldFont();
		if (bold) ImGui::PushFont(bold);
		ImGui::TextUnformatted(crashErrorCode.c_str());
		if (bold) ImGui::PopFont();
	}
	ImGui::SameLine(0.0f, 5.0f);
	ImGui::TextWrapped("%s", crashErrorText.c_str());
	ImGui::Dummy(ImVec2(0.0f, 20.0f));

	if (ImGui::Button("Copy Log to Clipboard"))
		ImGui::SetClipboardText(crashLogDump.c_str());
	ImGui::SameLine();

	if (ImGui::Button("Restart Application"))
		app::restartApplication();

	ImGui::Spacing();
	static std::string crashLogBuffer;
	if (crashLogBuffer.empty())
		crashLogBuffer = crashLogDump.empty() ? "No runtime log available." : crashLogDump;

	ImGui::InputTextMultiline("##CrashLog", &crashLogBuffer[0], crashLogBuffer.size() + 1,
		ImVec2(-FLT_MIN, -FLT_MIN), ImGuiInputTextFlags_ReadOnly);

	ImGui::End();
}

static constexpr float HEADER_HEIGHT = 53.0f;
static constexpr float FOOTER_HEIGHT = 73.0f;
static constexpr float TOAST_HEIGHT   = 30.0f;
static constexpr float NAV_ICON_WIDTH = 45.0f;
static constexpr float NAV_ICON_HEIGHT = 52.0f;


static void handleToastOptionClick(const std::function<void()>& func) {
	if (core::view)
		core::view->toast.reset();

	if (func)
		func();
}

static void hideToast(bool userCancel = false) {
	core::hideToast(userCancel);
}

static void removeOverrideModels() {
	if (!core::view)
		return;
	core::view->overrideModelList.clear();
	core::view->overrideModelName.clear();
}

static void removeOverrideTextures() {
	if (!core::view)
		return;
	core::view->overrideTextureList.clear();
	core::view->overrideTextureName.clear();
}

static void renderToastBar(
	float bar_width,
	const ImVec4& bg_color,
	const char* icon_glyph,
	const std::string& message,
	const std::vector<std::pair<std::string, std::function<void()>>>& actions,
	bool closable,
	const std::function<void()>& on_close)
{
	ImGui::PushStyleColor(ImGuiCol_ChildBg, bg_color);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 4.0f));
	ImGui::BeginChild("##toast", ImVec2(bar_width, TOAST_HEIGHT),
		ImGuiChildFlags_None,
		ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

	ImGui::PushFont(app::theme::getIconFont());
	ImGui::TextUnformatted(icon_glyph);
	ImGui::PopFont();
	ImGui::SameLine(0.0f, 6.0f);
	ImGui::TextUnformatted(message.c_str());

	for (size_t i = 0; i < actions.size(); ++i) {
		ImGui::PushID(static_cast<int>(i));
		ImGui::SameLine(0.0f, 8.0f);
		if (ImGui::SmallButton(actions[i].first.c_str()))
			actions[i].second();
		ImGui::PopID();
	}

	if (closable) {
		ImGui::SameLine(bar_width - 32.0f, 0.0f);
		ImGui::PushFont(app::theme::getIconFont());
		if (ImGui::SmallButton(ICON_FA_XMARK "##close"))
			on_close();
		ImGui::PopFont();
	}

	ImGui::EndChild();
	ImGui::PopStyleVar();
	ImGui::PopStyleColor();
}

static void renderAppShell() {
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	const ImVec2 vp_pos = viewport->WorkPos;
	const ImVec2 vp_size = viewport->WorkSize;

	{
		ImGui::SetNextWindowPos(vp_pos);
		ImGui::SetNextWindowSize(ImVec2(vp_size.x, HEADER_HEIGHT));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::Begin("##AppHeader", nullptr,
			ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoFocusOnAppearing);

		ImDrawList* draw = ImGui::GetWindowDrawList();
		draw->AddLine(
			ImVec2(vp_pos.x, vp_pos.y + HEADER_HEIGHT - 1.0f),
			ImVec2(vp_pos.x + vp_size.x, vp_pos.y + HEADER_HEIGHT - 1.0f),
			ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_Separator)), 1.0f);

		if (core::view && core::view->toast.has_value()) {
			ImDrawList* fg = ImGui::GetForegroundDrawList();
			const float shadow_y = vp_pos.y + HEADER_HEIGHT;
			const float x0 = vp_pos.x, x1 = vp_pos.x + vp_size.x;
			static constexpr ImU32 shadow_alphas[] = { 80, 55, 35, 18, 7 };
			for (int i = 0; i < 5; ++i) {
				fg->AddRectFilled(
					ImVec2(x0, shadow_y + i),
					ImVec2(x1, shadow_y + i + 1.0f),
					IM_COL32(0, 0, 0, shadow_alphas[i]));
			}
		}

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
			const auto& navButtons = modules::getNavButtons();
			for (const auto& btn : navButtons) {
				if (!(btn.installTypes & static_cast<uint32_t>(core::view->installType)))
					continue;

				const bool is_active = core::view->activeModule.is_object() &&
					core::view->activeModule.contains("__name") &&
					core::view->activeModule["__name"].get<std::string>() == btn.module;

				ImGui::PushID(btn.module.c_str());
				ImGui::SetCursorPos(ImVec2(cursor_x, (HEADER_HEIGHT - NAV_ICON_HEIGHT) * 0.5f));

				if (is_active)
					ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));

				bool clicked = false;
				const char* icon_glyph = app::theme::getIconForFilename(btn.icon);
				if (icon_glyph) {
					ImGui::PushFont(app::theme::getIconFont());
					clicked = ImGui::Button(icon_glyph, ImVec2(NAV_ICON_WIDTH, NAV_ICON_HEIGHT));
					ImGui::PopFont();
				} else {
					GLuint icon_tex = getNavIconTexture(btn.icon);
					if (icon_tex) {
						ImVec4 tint = is_active ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
						clicked = ImGui::ImageButton("##nav",
							static_cast<ImTextureID>(static_cast<uintptr_t>(icon_tex)),
							ImVec2(NAV_ICON_WIDTH - 8.0f, NAV_ICON_HEIGHT - 16.0f),
							ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), tint);
					}
				}

				if (is_active)
					ImGui::PopStyleColor();

				if (clicked)
					modules::setActive(btn.module);

				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("%s", btn.label.c_str());

				ImGui::PopID();
				cursor_x += NAV_ICON_WIDTH;
			}

			float right_x = vp_size.x;

			if (!core::view->isBusy) {
				right_x -= 15.0f + 20.0f;
				ImGui::SetCursorPos(ImVec2(right_x, (HEADER_HEIGHT - 20.0f) * 0.5f));
				ImGui::PushID("##nav-extra");

				GLuint hamburger_tex = getNavIconTexture("line-columns.svg");
				bool hamburger_clicked = false;
				if (hamburger_tex) {
					hamburger_clicked = ImGui::ImageButton("##hamburger",
						static_cast<ImTextureID>(static_cast<uintptr_t>(hamburger_tex)),
						ImVec2(18.0f, 18.0f));
				} else {
					ImGui::PushFont(app::theme::getIconFont());
					hamburger_clicked = ImGui::Button(ICON_FA_BARS, ImVec2(20.0f, 20.0f));
					ImGui::PopFont();
				}

				if (hamburger_clicked)
					ImGui::OpenPopup("##MenuExtraPopup");

				ImGui::SetNextWindowPos(ImVec2(vp_pos.x + right_x + 20.0f, vp_pos.y + HEADER_HEIGHT), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
				if (ImGui::BeginPopup("##MenuExtraPopup", ImGuiWindowFlags_AlwaysAutoResize)) {
					const auto& contextOpts = modules::getContextMenuOptions();
					for (const auto& opt : contextOpts) {
						if (opt.dev_only && !(core::view->isDev))
							continue;
						if (ImGui::MenuItem(opt.label.c_str())) {
							if (opt.handler)
								opt.handler();
							else
								modules::setActive(opt.id);
						}
					}
					ImGui::EndPopup();
				}
				ImGui::PopID();

				right_x -= 10.0f + 20.0f;
				ImGui::SetCursorPos(ImVec2(right_x, (HEADER_HEIGHT - 20.0f) * 0.5f));
				ImGui::PushID("##nav-help");

				GLuint help_tex = getNavIconTexture("help.svg");
				if (help_tex) {
					ImGui::ImageButton("##help",
						static_cast<ImTextureID>(static_cast<uintptr_t>(help_tex)),
						ImVec2(18.0f, 18.0f));
				} else {
					ImGui::PushFont(app::theme::getIconFont());
					ImGui::Button(ICON_FA_CIRCLE_QUESTION, ImVec2(20.0f, 20.0f));
					ImGui::PopFont();
				}

				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Help");

				ImGui::PopID();
			}
		}

		ImGui::End();
		ImGui::PopStyleVar(2);
	}

	{
		float footer_y = vp_pos.y + vp_size.y - FOOTER_HEIGHT;
		ImGui::SetNextWindowPos(ImVec2(vp_pos.x, footer_y));
		ImGui::SetNextWindowSize(ImVec2(vp_size.x, FOOTER_HEIGHT));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::Begin("##AppFooter", nullptr,
			ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoFocusOnAppearing);

		ImDrawList* draw = ImGui::GetWindowDrawList();
		draw->AddLine(
			ImVec2(vp_pos.x, footer_y),
			ImVec2(vp_pos.x + vp_size.x, footer_y),
			ImGui::GetColorU32(ImGuiCol_Separator), 1.0f);

		{
			const char* copyright_text = "World of Warcraft and related trademarks are registered trademarks of Blizzard Entertainment whom this application is not affiliated with.";
			ImVec2 copy_size = ImGui::CalcTextSize(copyright_text);
			float line_h = ImGui::CalcTextSize("A").y;
			float copy_y = (FOOTER_HEIGHT - line_h) * 0.5f;
			ImGui::SetCursorPos(ImVec2((vp_size.x - copy_size.x) * 0.5f, copy_y));
			ImGui::TextDisabled("%s", copyright_text);
		}

		ImGui::End();
		ImGui::PopStyleVar(2);
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

		if (s_logoTexture) {
			float logo_w = static_cast<float>(s_logoWidth);
			float logo_h = static_cast<float>(s_logoHeight);
			ImVec2 content_min(vp_pos.x, content_y);
			ImVec2 logo_min(
				content_min.x + (vp_size.x - logo_w) * 0.5f,
				content_min.y + (content_h - logo_h) * 0.5f);
			ImVec2 logo_max(logo_min.x + logo_w, logo_min.y + logo_h);
			ImU32 watermark_tint = IM_COL32(255, 255, 255, 13);
			ImGui::GetWindowDrawList()->AddImage(
				static_cast<ImTextureID>(static_cast<uintptr_t>(s_logoTexture)),
				logo_min, logo_max, ImVec2(0, 0), ImVec2(1, 1), watermark_tint);
		}

		float toast_h = 0.0f;
		if (core::view && core::view->toast.has_value()) {
			const auto& toast = core::view->toast.value();
			toast_h = TOAST_HEIGHT;

			ImVec4 bg_color;
			const char* icon_glyph;
			if      (toast.type == "error")    { bg_color = ImVec4(0.60f, 0.18f, 0.18f, 1.0f); icon_glyph = ICON_FA_TRIANGLE_EXCLAMATION; }
			else if (toast.type == "success")  { bg_color = ImVec4(0.18f, 0.52f, 0.18f, 1.0f); icon_glyph = ICON_FA_CHECK; }
			else if (toast.type == "progress") { bg_color = ImVec4(0.52f, 0.40f, 0.10f, 1.0f); icon_glyph = ICON_FA_STOPWATCH; }
			else                               { bg_color = ImVec4(0.18f, 0.36f, 0.60f, 1.0f); icon_glyph = ICON_FA_CIRCLE_INFO; }

			std::vector<std::pair<std::string, std::function<void()>>> acts;
			for (const auto& act : toast.actions)
				acts.emplace_back(act.label, [cb = act.callback]() { handleToastOptionClick(cb); });

			renderToastBar(vp_size.x, bg_color, icon_glyph, toast.message, acts,
			               toast.closable, []() { hideToast(true); });
		}

		if (core::view && !core::view->toast.has_value()) {
			modules::ModuleDef* cur = modules::getActive();
			if (cur && cur->name == "tab_models" && !core::view->overrideModelList.empty()) {
				toast_h = TOAST_HEIGHT;
				std::string msg = std::format("Filtering models for item: {}", core::view->overrideModelName);
				std::vector<std::pair<std::string, std::function<void()>>> acts;
				acts.emplace_back("Remove", removeOverrideModels);
				renderToastBar(vp_size.x, ImVec4(0.52f, 0.40f, 0.10f, 1.0f), ICON_FA_STOPWATCH,
				               msg, acts, true, removeOverrideModels);
			}
		}

		if (core::view && !core::view->toast.has_value() && toast_h == 0.0f) {
			modules::ModuleDef* cur = modules::getActive();
			if (cur && cur->name == "tab_textures" && !core::view->overrideTextureList.empty()) {
				toast_h = TOAST_HEIGHT;
				std::string msg = std::format("Filtering textures for item: {}", core::view->overrideTextureName);
				std::vector<std::pair<std::string, std::function<void()>>> acts;
				acts.emplace_back("Remove", removeOverrideTextures);
				renderToastBar(vp_size.x, ImVec4(0.52f, 0.40f, 0.10f, 1.0f), ICON_FA_STOPWATCH,
				               msg, acts, true, removeOverrideTextures);
			}
		}

		modules::ModuleDef* active = modules::getActive();
		if (active && active->render) {
			try {
				active->render();
			} catch (const std::exception& e) {
				crash("ERR_VUE", e.what());
			}
		}

		ImGui::End();
		ImGui::PopStyleVar(2);
	}

	if (core::view && core::view->isLoading) {
		ImGui::SetNextWindowPos(vp_pos);
		ImGui::SetNextWindowSize(vp_size);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::Begin("##LoadingOverlay", nullptr,
			ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
			ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);

		ImDrawList* dl = ImGui::GetWindowDrawList();

		GLuint bg_tex = core::view->isXmas ? s_loadingXmasBgTexture : s_loadingBgTexture;
		if (bg_tex) {
			ImVec2 uv0(0, 0), uv1(1, 1);
			ImU32 tint = IM_COL32(255, 255, 255, 51);
			dl->AddImage(
				static_cast<ImTextureID>(static_cast<uintptr_t>(bg_tex)),
				vp_pos, ImVec2(vp_pos.x + vp_size.x, vp_pos.y + vp_size.y),
				uv0, uv1, tint);
		}

		float center_x = vp_pos.x + vp_size.x * 0.5f;
		float center_y = vp_pos.y + vp_size.y * 0.5f;

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
			float time_s = static_cast<float>(ImGui::GetTime());
			float angle_rad = std::fmod(time_s, 6.0f) / 6.0f * 2.0f * std::numbers::pi_v<float>;

			ImVec2 gear_center(gear_x + GEAR_SIZE * 0.5f, gear_y + GEAR_SIZE * 0.5f);
			float cos_a = std::cos(angle_rad);
			float sin_a = std::sin(angle_rad);
			float half = GEAR_SIZE * 0.5f;

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
			ImGui::PushFont(bold, TITLE_FONT_SIZE);
			ImVec2 title_size = ImGui::CalcTextSize(core::view->loadingTitle.c_str());
			ImGui::SetCursorScreenPos(ImVec2(center_x - title_size.x * 0.5f, text_y));
			ImGui::TextUnformatted(core::view->loadingTitle.c_str());
			text_y += title_size.y;
			ImGui::PopFont();
		}

		{
			ImGui::PushFont(nullptr, PROGRESS_FONT_SIZE);
			ImVec2 prog_size = ImGui::CalcTextSize(core::view->loadingProgress.c_str());
			ImGui::SetCursorScreenPos(ImVec2(center_x - prog_size.x * 0.5f, text_y));
			ImGui::TextUnformatted(core::view->loadingProgress.c_str());
			text_y += prog_size.y;
			ImGui::PopFont();
		}

		if (core::view->loadPct >= 0.0) {
			float bar_x = center_x - BAR_WIDTH * 0.5f;
			float bar_y = text_y + BAR_MARGIN_TOP;

			dl->AddRectFilled(
				ImVec2(bar_x, bar_y),
				ImVec2(bar_x + BAR_WIDTH, bar_y + BAR_HEIGHT),
				IM_COL32(0, 0, 0, 56));

			dl->AddRect(
				ImVec2(bar_x, bar_y),
				ImVec2(bar_x + BAR_WIDTH, bar_y + BAR_HEIGHT),
				ImGui::GetColorU32(ImGuiCol_Separator), 0.0f, 0, 1.0f);

			float fill_w = static_cast<float>(std::clamp(core::view->loadPct, 0.0, 1.0) * BAR_WIDTH);
			if (fill_w > 0.0f) {
				dl->AddRectFilledMultiColor(
					ImVec2(bar_x, bar_y),
					ImVec2(bar_x + fill_w, bar_y + BAR_HEIGHT),
					IM_COL32(87, 175, 226, 255),
					IM_COL32(87, 175, 226, 255),
					IM_COL32(53, 117, 154, 255),
					IM_COL32(53, 117, 154, 255));
			}
		}

		ImGui::End();
		ImGui::PopStyleVar(2);
	}

	if (core::view && core::view->fileDropPrompt.is_string()) {
		std::string prompt_text = core::view->fileDropPrompt.get<std::string>();
		if (!prompt_text.empty()) {
			ImGui::SetNextWindowPos(vp_pos);
			ImGui::SetNextWindowSize(vp_size);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			ImGui::Begin("##DropOverlay", nullptr,
				ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
				ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
				ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);

			float center_x = vp_pos.x + vp_size.x * 0.5f;
			float center_y = vp_pos.y + vp_size.y * 0.5f;

			constexpr float ICON_SIZE = 100.0f;
			constexpr float ICON_MARGIN_BOTTOM = 20.0f;
			constexpr float TEXT_FONT_SIZE = 25.0f;

			std::string formatted = std::string("\xc2\xbb ") + prompt_text + " \xc2\xab";

			ImVec2 text_size;
			{
				ImGui::PushFont(nullptr, TEXT_FONT_SIZE);
				text_size = ImGui::CalcTextSize(formatted.c_str());
				ImGui::PopFont();
			}

			float total_h = ICON_SIZE + ICON_MARGIN_BOTTOM + text_size.y;
			float start_y = center_y - total_h * 0.5f;

			ImFont* icon_font = app::theme::getIconFont();
			if (icon_font) {
				ImGui::PushFont(icon_font, ICON_SIZE);
				ImVec2 icon_sz = ImGui::CalcTextSize(ICON_FA_COPY);
				ImGui::SetCursorScreenPos(ImVec2(center_x - icon_sz.x * 0.5f, start_y));
				ImGui::TextUnformatted(ICON_FA_COPY);
				ImGui::PopFont();
			}

			ImGui::PushFont(nullptr, TEXT_FONT_SIZE);
			ImGui::SetCursorScreenPos(ImVec2(center_x - text_size.x * 0.5f, start_y + ICON_SIZE + ICON_MARGIN_BOTTOM));
			ImGui::TextUnformatted(formatted.c_str());
			ImGui::PopFont();

			ImGui::End();
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

		if (!include.empty() && handler->process)
			handler->process(include);
	}
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
	} catch (...) {}

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
			} catch (...) {}
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
 * @param {boolean} state
 */
static void setAllItemTypes(bool state) {
	tab_items::setAllItemTypes(state);
}

/**
 * Mark all item qualities to the given state.
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
 * Invoked when the user manually selects a CDN region.
 * @param {object} region
 */
static void setSelectedCDN(const nlohmann::json& region) {
	if (!core::view)
		return;
	core::view->selectedCDNRegion = region;
	core::view->lockCDNRegion = true;
	core::view->config["sourceSelectUserRegion"] = region.value("tag", "");
	config::save();
	casc::cdn_resolver::startPreResolution(region.value("tag", ""));
}

/**
 * Emit an event using the global event emitter.
 * @param {string} tag
 */
static void click(const std::string& tag, bool disabled = false) {
	if (!disabled)
		core::events.emit("click-" + tag);
}

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

static void emit(const std::string& tag, const std::any& arg) {
	core::events.emit(tag, arg);
}

/**
 * Restart the application.
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

/**
 * Switches to the textures tab and filters for the given file.
 * @param {number} fileDataID
 */
static void goToTexture(uint32_t fileDataID) {
	tab_textures::goToTexture(fileDataID);
}

}

static constexpr float DPI_SCALE_EPSILON = 0.01f;
static float clampDpiScale(float raw) {
	if (raw <= 0.0f) raw = 1.0f;
	return (std::max)(0.5f, (std::min)(raw, 4.0f));
}


namespace app::theme {



static ImFont* s_fontBold    = nullptr;
static ImFont* s_fontGambler = nullptr;
static ImFont* s_fontIcon    = nullptr;
static float   s_dpiScale    = 1.0f;

static const ImWchar s_iconRanges[] = { ICON_FA_MIN, ICON_FA_MAX, 0 };

void loadFonts(float dpiScale) {
	ImGuiIO& io = ImGui::GetIO();

	dpiScale = clampDpiScale(dpiScale);
	s_dpiScale = dpiScale;

	const float scaledDefault = DEFAULT_FONT_SIZE * dpiScale;
	const float scaledIcon    = 48.0f * dpiScale;

	std::filesystem::path fontsDir = constants::SRC_DIR() / "fonts";
	std::string regularPath = (fontsDir / "selawk.ttf").string();
	std::string boldPath    = (fontsDir / "selawkb.ttf").string();
	std::string gamblerPath = (fontsDir / "gmblr.ttf").string();
	std::string iconPath    = (fontsDir / "fa-solid-900.ttf").string();

	ImFont* regularFont = io.Fonts->AddFontFromFileTTF(regularPath.c_str(), scaledDefault);
	if (!regularFont) {
		regularFont = io.Fonts->AddFontDefault();
	}

	{
		ImFontConfig iconCfg;
		iconCfg.MergeMode = true;
		iconCfg.GlyphMinAdvanceX = scaledDefault;
		io.Fonts->AddFontFromFileTTF(iconPath.c_str(), scaledDefault, &iconCfg, s_iconRanges);
	}

	s_fontBold = io.Fonts->AddFontFromFileTTF(boldPath.c_str(), scaledDefault);
	if (!s_fontBold) {
		s_fontBold = regularFont;
	}

	s_fontGambler = io.Fonts->AddFontFromFileTTF(gamblerPath.c_str(), scaledDefault);
	if (!s_fontGambler) {
		s_fontGambler = regularFont;
	}

	s_fontIcon = io.Fonts->AddFontFromFileTTF(iconPath.c_str(), scaledIcon, nullptr, s_iconRanges);
	if (!s_fontIcon) {
		s_fontIcon = regularFont;
	}

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

	io.Fonts->Clear();
	s_fontBold    = nullptr;
	s_fontGambler = nullptr;
	s_fontIcon    = nullptr;

	loadFonts(dpiScale);

}

float getDpiScale() {
	return s_dpiScale;
}


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


static const char* s_expansionIconFiles[] = {
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
};
static constexpr int s_expansionIconCount = static_cast<int>(std::size(s_expansionIconFiles));
static GLuint s_expansionIconTextures[13] = {};
static bool s_expansionIconsLoaded = false;

static void loadExpansionIcons() {
	if (s_expansionIconsLoaded)
		return;
	s_expansionIconsLoaded = true;

	std::filesystem::path iconDir = constants::SRC_DIR() / "images" / "expansion";
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

	ImGui::Dummy(ImVec2(0, 10.0f));

	int totalBtns = 1 + expansionCount;
	float totalWidth = totalBtns * BTN_SIZE + (totalBtns - 1) * GAP;
	float indent = (ImGui::GetContentRegionAvail().x - totalWidth) * 0.5f;
	if (indent > 0.0f)
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + indent);

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(GAP, 0));

	{
		bool isActive = (selectedFilter == -1);
		if (isActive) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
		if (ImGui::Button(ICON_FA_BAN "##exp_all_icon", ImVec2(BTN_SIZE, BTN_SIZE)))
			selectedFilter = -1;
		if (isActive) ImGui::PopStyleColor();
	}

	for (int i = 0; i < expansionCount; ++i) {
		ImGui::SameLine();
		bool isActive = (selectedFilter == i);
		if (isActive) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));

		GLuint tex = getExpansionIconTexture(i);
		if (tex != 0) {
			ImGui::PushID(i);
			if (ImGui::ImageButton("##exp", static_cast<ImTextureID>(static_cast<uintptr_t>(tex)),
			                       ImVec2(24.0f, 24.0f))) {
				selectedFilter = i;
			}
			ImGui::PopID();
		} else {
			if (ImGui::Button(std::format("E{}##exp_{}", i, i).c_str(), ImVec2(BTN_SIZE, BTN_SIZE)))
				selectedFilter = i;
		}

		if (isActive) ImGui::PopStyleColor();
	}

	ImGui::PopStyleVar(2);
}

}


static double prevLoadPct = -1;
static void* prevCasc = nullptr;
static nlohmann::json prevActiveModule;

#ifdef _WIN32
static ITaskbarList3* s_taskbar = nullptr;

static void initTaskbarProgress() {
	CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER,
		IID_ITaskbarList3, reinterpret_cast<void**>(&s_taskbar));
	if (s_taskbar)
		s_taskbar->HrInit();
}

static void setTaskbarProgress(GLFWwindow* window, double val) {
	if (!s_taskbar)
		return;

	HWND hwnd = glfwGetWin32Window(window);
	if (!hwnd)
		return;

	if (val < 0) {
		s_taskbar->SetProgressState(hwnd, TBPF_NOPROGRESS);
	} else {
		s_taskbar->SetProgressState(hwnd, TBPF_NORMAL);
		s_taskbar->SetProgressValue(hwnd, static_cast<ULONGLONG>(val * 10000), 10000);
	}
}

static std::vector<std::string> extractFilePathsFromDataObject(IDataObject* pDataObj) {
	std::vector<std::string> paths;
	FORMATETC fmt = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	STGMEDIUM stg = {};
	if (FAILED(pDataObj->GetData(&fmt, &stg)))
		return paths;

	HDROP hDrop = static_cast<HDROP>(GlobalLock(stg.hGlobal));
	if (hDrop) {
		UINT count = DragQueryFileW(hDrop, 0xFFFFFFFF, nullptr, 0);
		paths.reserve(count);
		for (UINT i = 0; i < count; ++i) {
			UINT len = DragQueryFileW(hDrop, i, nullptr, 0);
			if (len == 0)
				continue;
			std::wstring wpath(len, L'\0');
			DragQueryFileW(hDrop, i, wpath.data(), len + 1);
			int utf8len = WideCharToMultiByte(CP_UTF8, 0, wpath.c_str(), -1, nullptr, 0, nullptr, nullptr);
			if (utf8len > 0) {
				std::string utf8(utf8len - 1, '\0');
				WideCharToMultiByte(CP_UTF8, 0, wpath.c_str(), -1, utf8.data(), utf8len, nullptr, nullptr);
				paths.push_back(std::move(utf8));
			}
		}
		GlobalUnlock(stg.hGlobal);
	}
	ReleaseStgMedium(&stg);
	return paths;
}

class WinDropTarget : public IDropTarget {
	LONG m_refCount = 1;
	int m_dropStack = 0;

public:
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
		if (riid == IID_IUnknown || riid == IID_IDropTarget) {
			*ppv = static_cast<IDropTarget*>(this);
			AddRef();
			return S_OK;
		}
		*ppv = nullptr;
		return E_NOINTERFACE;
	}

	ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&m_refCount); }
	ULONG STDMETHODCALLTYPE Release() override {
		LONG ref = InterlockedDecrement(&m_refCount);
		if (ref == 0)
			delete this;
		return ref;
	}

	HRESULT STDMETHODCALLTYPE DragEnter(IDataObject* pDataObj, DWORD /*grfKeyState*/,
		POINTL /*pt*/, DWORD* pdwEffect) override {
		if (!core::view) {
			*pdwEffect = DROPEFFECT_NONE;
			return S_OK;
		}

		if (core::view->isBusy) {
			*pdwEffect = DROPEFFECT_NONE;
			return S_OK;
		}

		m_dropStack++;

		// We're already showing a prompt, don't re-process it.
		if (!core::view->fileDropPrompt.is_null()) {
			*pdwEffect = DROPEFFECT_COPY;
			return S_OK;
		}

		auto files = extractFilePathsFromDataObject(pDataObj);
		if (!files.empty()) {
			const DropHandler* handler = core::getDropHandler(files[0]);
			if (handler) {
				int count = 0;
				for (const auto& file : files) {
					std::string lower = file;
					std::transform(lower.begin(), lower.end(), lower.begin(),
						[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
					for (const auto& ext : handler->ext) {
						if (lower.size() >= ext.size() &&
							lower.compare(lower.size() - ext.size(), ext.size(), ext) == 0) {
							count++;
							break;
						}
					}
				}
				if (count > 0 && handler->prompt)
					core::view->fileDropPrompt = handler->prompt(count);
			} else {
				core::view->fileDropPrompt = "That file cannot be converted.";
			}
		}

		*pdwEffect = DROPEFFECT_COPY;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE DragOver(DWORD /*grfKeyState*/, POINTL /*pt*/,
		DWORD* pdwEffect) override {
		*pdwEffect = DROPEFFECT_COPY;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE DragLeave() override {
		m_dropStack--;
		if (m_dropStack == 0 && core::view)
			core::view->fileDropPrompt = nullptr;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE Drop(IDataObject* pDataObj, DWORD /*grfKeyState*/,
		POINTL /*pt*/, DWORD* pdwEffect) override {
		m_dropStack = 0;

		if (core::view)
			core::view->fileDropPrompt = nullptr;

		if (!core::view || core::view->isBusy) {
			*pdwEffect = DROPEFFECT_NONE;
			return S_OK;
		}

		auto files = extractFilePathsFromDataObject(pDataObj);
		if (!files.empty()) {
			const DropHandler* handler = core::getDropHandler(files[0]);
			if (handler) {
				std::vector<std::string> include;
				for (const auto& file : files) {
					std::string lower = file;
					std::transform(lower.begin(), lower.end(), lower.begin(),
						[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
					for (const auto& ext : handler->ext) {
						if (lower.size() >= ext.size() &&
							lower.compare(lower.size() - ext.size(), ext.size(), ext) == 0) {
							include.push_back(file);
							break;
						}
					}
				}
				if (!include.empty() && handler->process)
					handler->process(include);
			}
		}

		*pdwEffect = DROPEFFECT_COPY;
		return S_OK;
	}
};

static WinDropTarget* s_dropTarget = nullptr;
static bool s_ole_initialized = false;
#endif

static void checkWatchers(GLFWwindow* window) {
	if (!core::view)
		return;

	config::checkForChanges();

	/**
	 * Invoked when the active loading percentage is changed.
	 * @param {float} val
	 */
	if (core::view->loadPct != prevLoadPct) {
		prevLoadPct = core::view->loadPct;
#ifdef _WIN32
		setTaskbarProgress(window, prevLoadPct);
#else
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
		core::view->contextMenus.resetAll();
	}
}


app::layout::ListTabRegions app::layout::CalcListTabRegions(bool hasSidebar, float colRatio) {
	ListTabRegions r{};
	r.hasSidebar = hasSidebar;

	const ImVec2 avail = ImGui::GetContentRegionAvail();
	const ImVec2 cursor = ImGui::GetCursorPos();

	const float sidebarW = hasSidebar ? SIDEBAR_WIDTH : 0.0f;
	const float gridW = avail.x - sidebarW;
	const float gridH = avail.y;

	const float leftColW = gridW * colRatio;
	const float rightColW = gridW * (1.0f - colRatio);

	const float bottomH = FILTER_BAR_HEIGHT;
	const float statusBarH = 27.0f;
	const float topH = gridH - bottomH - statusBarH;

	r.listPos = ImVec2(cursor.x + LIST_MARGIN_LEFT, cursor.y + LIST_MARGIN_TOP);
	r.listSize = ImVec2(
		leftColW - LIST_MARGIN_LEFT - LIST_MARGIN_RIGHT,
		topH - LIST_MARGIN_TOP - LIST_MARGIN_BOTTOM
	);

	r.statusBarPos = ImVec2(cursor.x + LIST_MARGIN_LEFT, cursor.y + topH);
	r.statusBarSize = ImVec2(leftColW - LIST_MARGIN_LEFT - LIST_MARGIN_RIGHT, statusBarH);

	r.previewPos = ImVec2(cursor.x + leftColW + PREVIEW_MARGIN_LEFT, cursor.y + PREVIEW_MARGIN_TOP);
	r.previewSize = ImVec2(
		rightColW - PREVIEW_MARGIN_LEFT - PREVIEW_MARGIN_RIGHT,
		topH - PREVIEW_MARGIN_TOP - PREVIEW_MARGIN_BOTTOM
	);

	r.filterPos = ImVec2(cursor.x, cursor.y + topH + statusBarH);
	r.filterSize = ImVec2(leftColW, bottomH);

	r.controlsPos = ImVec2(cursor.x + leftColW, cursor.y + topH + statusBarH);
	r.controlsSize = ImVec2(rightColW, bottomH);

	if (hasSidebar) {
		r.sidebarPos = ImVec2(cursor.x + gridW, cursor.y + SIDEBAR_MARGIN_TOP);
		r.sidebarSize = ImVec2(
			SIDEBAR_WIDTH - SIDEBAR_PADDING_RIGHT,
			gridH - SIDEBAR_MARGIN_TOP
		);
	}

	return r;
}

bool app::layout::BeginTab(const char* id) {
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
	return ImGui::BeginChild(id, regions.listSize, ImGuiChildFlags_None,
		ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
}

void app::layout::EndListContainer() {
	ImGui::EndChild();
}

bool app::layout::BeginStatusBar(const char* id, const ListTabRegions& regions) {
	ImGui::SetCursorPos(regions.statusBarPos);
	return ImGui::BeginChild(id, regions.statusBarSize, ImGuiChildFlags_None,
		ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
}

void app::layout::EndStatusBar() {
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

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 0.0f));
	bool visible = ImGui::BeginChild(id, regions.filterSize, ImGuiChildFlags_None,
	    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

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

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	bool visible = ImGui::BeginChild(id, regions.controlsSize, ImGuiChildFlags_None,
	    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	ImGui::PopStyleVar();

	if (visible) {
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
	constants::init();

	logging::init();

	// Register crash handlers.
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
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifndef NDEBUG
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif
	glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

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
	if (SUCCEEDED(OleInitialize(nullptr)))
		s_ole_initialized = true;

	{
		HWND hwnd = glfwGetWin32Window(window);
		BOOL useDarkMode = TRUE;
		::DwmSetWindowAttribute(hwnd, 20, &useDarkMode, sizeof(useDarkMode));

		COLORREF captionColor = RGB(44, 49, 54);
		::DwmSetWindowAttribute(hwnd, 35, &captionColor, sizeof(captionColor));
	}

	initTaskbarProgress();
	setTaskbarProgress(window, -1);

	// Prevent files from being dropped onto the window. These are over-written
	// later but we disable here to prevent them working if init fails.
	{
		HWND hwnd = glfwGetWin32Window(window);
		DragAcceptFiles(hwnd, FALSE);
		s_dropTarget = new WinDropTarget();
		RegisterDragDrop(hwnd, s_dropTarget);
	}
#endif

	if (!gladLoadGL(glfwGetProcAddress)) {
		crash("ERR_GLAD_INIT", "Failed to initialize OpenGL loader (GLAD2)");
		glfwDestroyWindow(window);
		glfwTerminate();
		return 1;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	ImGui::StyleColorsDark();

	float initialDpiScale;
	{
		float xscale = 1.0f, yscale = 1.0f;
		glfwGetWindowContentScale(window, &xscale, &yscale);
		initialDpiScale = clampDpiScale(xscale);
	}

	app::theme::loadFonts(initialDpiScale);

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 430");

	initAppShellTextures();


	static AppState appState = core::makeNewView();
	core::view = &appState;

	// Interlink error handling for Vue.
	modules::register_components();

	constexpr int SCALE_THRESHOLD_W = 1120;
	constexpr int SCALE_THRESHOLD_H = 700;

	modules::initialize();

	// register static context menu options
	modules::registerContextMenuOption("runtime-log", "Open Runtime Log", "timeline.svg", []() { logging::openRuntimeLog(); });
	modules::registerContextMenuOption("restart", "Restart wow.export.cpp", "arrow-rotate-left.svg", []() { app::restartApplication(); });
	modules::registerContextMenuOption("reload-shaders", "Reload Shaders", "cube.svg", []() { shaders::reload_all(); }, true);
	modules::registerContextMenuOption("reload-active", "Reload Active Module", "gear.svg", []() { modules::reloadActiveModule(); }, true);
	modules::registerContextMenuOption("reload-all", "Reload All Modules", "gear.svg", []() { modules::reloadAllModules(); }, true);

	// Log some basic information for potential diagnostics.
	logging::write(std::format("wow.export.cpp has started v{} {} [{}]",
		constants::VERSION, constants::FLAVOUR, constants::BUILD_GUID));
	logging::write(std::format("Host {} ({}), CPU {} ({} cores), Memory {} / {}",
		getPlatformName(), getArchName(), getCPUModel(), getCPUCoreCount(),
		generics::filesize(static_cast<double>(getFreeMemory())),
		generics::filesize(static_cast<double>(getTotalMemory()))));
	logging::write(std::format("INSTALL_PATH {} DATA_PATH {}",
		constants::INSTALL_PATH().string(),
		constants::DATA_DIR().string()));

	// log gpu info async to avoid blocking startup
	gpu_info::log_gpu_info();

	// Load configuration.
	config::load();

	// Set-up default export directory if none configured.
	if (core::view->config.contains("exportDirectory") &&
		core::view->config["exportDirectory"].get<std::string>().empty()) {
		std::string defaultExportDir = (getHomeDir() / "wow.export").string();
		core::view->config["exportDirectory"] = defaultExportDir;
		config::save();
		logging::write(std::format("No export directory set, setting to {}", defaultExportDir));
	} else if (!core::view->config.contains("exportDirectory")) {
		std::string defaultExportDir = (getHomeDir() / "wow.export").string();
		core::view->config["exportDirectory"] = defaultExportDir;
		config::save();
		logging::write(std::format("No export directory set, setting to {}", defaultExportDir));
	}

	casc::listfile::preloadAsync();
	casc::dbd_manifest::preload();

#ifndef _WIN32
	glfwSetDropCallback(window, glfw_drop_callback);
#endif

	loadCacheSize();

	casc::initBuildCacheSystem();

	casc::registerBuildCacheEvents();

	casc::tact_keys::loadBackground();

	tab_blender::checkLocalVersion();

	// Set source select as the currently active interface screen.
	modules::setActive("source_select");

	{
		std::filesystem::path whatsNewPath = constants::SRC_DIR() / "whats-new.html";
		std::ifstream whatsNewFile(whatsNewPath);
		if (whatsNewFile.is_open()) {
			std::string html((std::istreambuf_iterator<char>(whatsNewFile)),
			                  std::istreambuf_iterator<char>());
			core::view->whatsNewHTML = std::move(html);
		} else {
			logging::write(std::format("failed to load whats-new.html: {}", whatsNewPath.string()));
		}
	}

	prevLoadPct = core::view->loadPct;
	prevCasc = static_cast<void*>(core::view->casc);
	prevActiveModule = core::view->activeModule;


	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		try {
			core::drainMainThreadQueue();

			// Debugging reloader.
			if (!IS_RELEASE_BUILD) {
				static bool f5WasPressed = false;
				bool f5IsPressed = glfwGetKey(window, GLFW_KEY_F5) == GLFW_PRESS;
				if (!f5IsPressed && f5WasPressed) {
					app::restartApplication();
				}
				f5WasPressed = f5IsPressed;
			}

			{
				float dpiScale = clampDpiScale(
					ImGui_ImplGlfw_GetContentScaleForWindow(window));

				if (std::abs(dpiScale - app::theme::getDpiScale()) > DPI_SCALE_EPSILON) {
					app::theme::rebuildFontsForScale(dpiScale);
				}
			}

			checkWatchers(window);

			checkCacheSizeUpdate();

			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();

			{
				float dpiScale = app::theme::getDpiScale();

				int win_w, win_h;
				glfwGetWindowSize(window, &win_w, &win_h);
				int fb_w, fb_h;
				glfwGetFramebufferSize(window, &fb_w, &fb_h);

				float css_w = static_cast<float>(win_w);
				float css_h = static_cast<float>(win_h);

				float logical_w = (std::max)(css_w, static_cast<float>(SCALE_THRESHOLD_W));
				float logical_h = (std::max)(css_h, static_cast<float>(SCALE_THRESHOLD_H));

				io.DisplaySize = ImVec2(logical_w, logical_h);

				float fb_scale_x = (fb_w > 0) ? static_cast<float>(fb_w) / logical_w : 1.0f;
				float fb_scale_y = (fb_h > 0) ? static_cast<float>(fb_h) / logical_h : 1.0f;
				io.DisplayFramebufferScale = ImVec2(fb_scale_x, fb_scale_y);

				if (io.MousePos.x != -FLT_MAX && win_w > 0 && win_h > 0) {
					io.MousePos.x *= (logical_w / static_cast<float>(win_w));
					io.MousePos.y *= (logical_h / static_cast<float>(win_h));
				}

				io.FontGlobalScale = 1.5f / dpiScale;
			}

			ImGui::NewFrame();

			if (isCrashed) {
				renderCrashScreen();
			} else {
				renderAppShell();
			}
		} catch (const std::exception& e) {
			crash("ERR_UNHANDLED_EXCEPTION", e.what());
		} catch (...) {
			crash("ERR_UNHANDLED_EXCEPTION", "Non-standard exception in main loop");
		}

		ImGui::Render();
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(0.06f, 0.06f, 0.06f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}


	destroyAppShellTextures();
	for (auto& [name, tex] : s_navIconTextures) {
		if (tex) glDeleteTextures(1, &tex);
	}
	s_navIconTextures.clear();

#ifdef _WIN32
	{
		HWND hwnd = glfwGetWin32Window(window);
		if (hwnd)
			RevokeDragDrop(hwnd);
	}
	if (s_dropTarget) {
		s_dropTarget->Release();
		s_dropTarget = nullptr;
	}
	if (s_taskbar) {
		s_taskbar->Release();
		s_taskbar = nullptr;
	}
	if (s_ole_initialized)
		OleUninitialize();
#endif

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
