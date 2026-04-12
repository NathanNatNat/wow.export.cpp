/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "tab_home.h"
#include "../modules.h"
#include "../core.h"
#include "../constants.h"
#include "../install-type.h"
#include "../../app.h"

#include <imgui.h>
#include <glad/gl.h>
#include <webp/decode.h>
#include <format>
#include <fstream>
#include <filesystem>

namespace tab_home {


// Home background texture (home-background.webp) for the What's New panel.
static GLuint s_bgTexture = 0;
static int s_bgWidth = 0;
static int s_bgHeight = 0;
static bool s_bgLoaded = false;

// SVG icon textures for help button watermarks (loaded on demand).
static GLuint s_discordIconTex = 0;
static GLuint s_githubIconTex = 0;
static GLuint s_patreonIconTex = 0;
static bool s_helpIconsLoaded = false;

static constexpr const char* URL_DISCORD = "https://discord.gg/kC3EzAYBtf";
static constexpr const char* URL_GITHUB  = "https://github.com/Kruithne/wow.export";
static constexpr const char* URL_PATREON = "https://patreon.com/Kruithne";


static void loadBackgroundTexture() {
	if (s_bgLoaded)
		return;
	s_bgLoaded = true;

	std::filesystem::path path = constants::DATA_DIR() / "images" / "home-background.webp";
	std::ifstream file(path, std::ios::binary | std::ios::ate);
	if (!file.is_open())
		return;

	auto fileSize = file.tellg();
	if (fileSize == std::streampos(-1) || fileSize == std::streampos(0))
		return;

	std::vector<uint8_t> fileData(static_cast<size_t>(fileSize));
	file.seekg(0);
	file.read(reinterpret_cast<char*>(fileData.data()), fileSize);
	file.close();

	int w = 0, h = 0;
	uint8_t* pixels = WebPDecodeRGBA(fileData.data(), fileData.size(), &w, &h);
	if (!pixels)
		return;

	glGenTextures(1, &s_bgTexture);
	glBindTexture(GL_TEXTURE_2D, s_bgTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	glBindTexture(GL_TEXTURE_2D, 0);

	WebPFree(pixels);
	s_bgWidth = w;
	s_bgHeight = h;
}

static void loadHelpIcons() {
	if (s_helpIconsLoaded)
		return;
	s_helpIconsLoaded = true;

	std::filesystem::path faDir = constants::DATA_DIR() / "fa-icons";
	s_discordIconTex = app::theme::loadSvgTexture(faDir / "discord.svg", 80);
	s_githubIconTex  = app::theme::loadSvgTexture(faDir / "github.svg", 80);
	s_patreonIconTex = app::theme::loadSvgTexture(faDir / "patreon.svg", 80);
}

static void renderHelpButton(const char* id, const char* title, const char* subtitle,
                              const char* url, GLuint iconTex) {
	constexpr float CARD_WIDTH = 300.0f;
	constexpr float CARD_HEIGHT = 65.0f;
	constexpr float PADDING = 20.0f;
	constexpr float ROUNDING = 10.0f;

	ImVec2 cursor = ImGui::GetCursorScreenPos();
	ImVec2 cardMin = cursor;
	ImVec2 cardMax(cursor.x + CARD_WIDTH, cursor.y + CARD_HEIGHT + PADDING * 2);

	ImGui::PushID(id);
	ImGui::InvisibleButton("##card", ImVec2(CARD_WIDTH, CARD_HEIGHT + PADDING * 2));
	bool hovered = ImGui::IsItemHovered();
	bool clicked = ImGui::IsItemClicked();
	ImGui::PopID();

	ImDrawList* draw = ImGui::GetWindowDrawList();

	ImU32 bgCol = IM_COL32(0, 0, 0, 20);
	draw->AddRectFilled(cardMin, cardMax, bgCol, ROUNDING);

	ImU32 borderCol = hovered ? app::theme::NAV_SELECTED_U32 : app::theme::BORDER_U32;
	draw->AddRect(cardMin, cardMax, borderCol, ROUNDING, 0, 1.0f);

	// Icon watermark at 20% opacity on right side
	if (iconTex) {
		float iconSize = 80.0f;
		ImVec2 iconMin(cardMax.x - iconSize - 10.0f, cardMin.y + (cardMax.y - cardMin.y - iconSize) * 0.5f);
		ImVec2 iconMax(iconMin.x + iconSize, iconMin.y + iconSize);
		ImU32 iconTint = IM_COL32(255, 255, 255, 51); // 20% opacity
		draw->AddImage(
			static_cast<ImTextureID>(static_cast<uintptr_t>(iconTex)),
			iconMin, iconMax, ImVec2(0, 0), ImVec2(1, 1), iconTint);
	}

	// Text content
	ImU32 textCol = hovered ? app::theme::NAV_SELECTED_U32 : app::theme::FONT_PRIMARY_U32;
	ImU32 subtitleCol = hovered ? app::theme::NAV_SELECTED_U32 : app::theme::FONT_FADED_U32;

	ImFont* boldFont = app::theme::getBoldFont();
	float titleSize = 16.0f;
	float subtitleSize = 14.0f;
	float textX = cardMin.x + PADDING;
	float totalTextH = titleSize + 4.0f + subtitleSize;
	float textY = cardMin.y + (cardMax.y - cardMin.y - totalTextH) * 0.5f;

	draw->AddText(boldFont, titleSize, ImVec2(textX, textY), textCol, title);
	draw->AddText(ImGui::GetFont(), subtitleSize, ImVec2(textX, textY + titleSize + 4.0f),
	              subtitleCol, subtitle);

	// Click opens external link
	if (clicked)
		core::openInExplorer(url);
}

/**
 * Render a single navigation card.
 * Each card is ~100x100px with a Font Awesome icon centered above a label.
 */
static void renderNavCard(const modules::NavButton& btn, float cardSize) {
	ImVec2 cursor = ImGui::GetCursorScreenPos();
	ImVec2 cardMin = cursor;
	ImVec2 cardMax(cursor.x + cardSize, cursor.y + cardSize);

	ImGui::PushID(btn.module.c_str());
	ImGui::InvisibleButton("##navcard", ImVec2(cardSize, cardSize));
	bool hovered = ImGui::IsItemHovered();
	bool clicked = ImGui::IsItemClicked();
	ImGui::PopID();

	ImDrawList* draw = ImGui::GetWindowDrawList();

	// Background
	ImU32 bgCol = IM_COL32(0, 0, 0, 20);
	draw->AddRectFilled(cardMin, cardMax, bgCol, 8.0f);

	// Border: default = subtle, hover = green
	ImU32 borderCol = hovered ? app::theme::NAV_SELECTED_U32 : IM_COL32(108, 117, 125, 80);
	draw->AddRect(cardMin, cardMax, borderCol, 8.0f, 0, 1.0f);

	// Icon (using Font Awesome icon font)
	const char* icon_glyph = app::theme::getIconForFilename(btn.icon);
	ImU32 iconCol = hovered ? app::theme::NAV_SELECTED_U32 : app::theme::FONT_PRIMARY_U32;
	float iconFontSize = 28.0f;

	if (icon_glyph) {
		ImFont* iconFont = app::theme::getIconFont();
		ImVec2 textSize = iconFont->CalcTextSizeA(iconFontSize, FLT_MAX, 0.0f, icon_glyph);
		float iconX = cardMin.x + (cardSize - textSize.x) * 0.5f;
		float iconY = cardMin.y + cardSize * 0.2f;
		draw->AddText(iconFont, iconFontSize, ImVec2(iconX, iconY), iconCol, icon_glyph);
	}

	// Label text centered below icon
	ImU32 labelCol = hovered ? app::theme::NAV_SELECTED_U32 : app::theme::FONT_PRIMARY_U32;
	float labelSize = 12.0f;
	ImVec2 labelTextSize = ImGui::GetFont()->CalcTextSizeA(labelSize, cardSize - 8.0f, cardSize - 8.0f, btn.label.c_str());
	float labelX = cardMin.x + (cardSize - labelTextSize.x) * 0.5f;
	float labelY = cardMin.y + cardSize * 0.6f;
	draw->AddText(ImGui::GetFont(), labelSize, ImVec2(labelX, labelY), labelCol,
	              btn.label.c_str(), nullptr, cardSize - 8.0f);

	if (clicked)
		modules::setActive(btn.module);
}

/**
 * Render the home tab layout (shared between tab_home and legacy_tab_home).
 *
 * JS template structure:
 *   <div class="tab" id="tab-home">     (or id="legacy-tab-home")
 *     <HomeShowcase />                   (→ replaced with title + nav cards)
 *     <div id="home-changes">...</div>   (→ What's New panel)
 *     <div id="home-help-buttons">...</div> (→ 3 help cards)
 *   </div>
 *
 * CSS grid: grid-template-columns: 1fr 1fr; grid-template-rows: auto 1fr auto auto;
 *           padding: 50px; gap: 0 50px;
 */
void renderHomeLayout() {
	// Ensure textures are loaded.
	loadBackgroundTexture();
	loadHelpIcons();

	const ImVec2 avail = ImGui::GetContentRegionAvail();
	const float PADDING = 50.0f;
	const float COL_GAP = 50.0f;

	// Usable area after padding.
	float usableW = avail.x - PADDING * 2;
	float usableH = avail.y - PADDING * 2;
	if (usableW < 100 || usableH < 100)
		return;

	float colW = (usableW - COL_GAP) * 0.5f;

	ImGuiViewport* viewport = ImGui::GetMainViewport();
	bool showHelpButtons = viewport->Size.y >= 900.0f;

	// Calculate row heights.
	// Row 1 (auto): title height
	float titleRowH = 40.0f; // "wow.export vX.X.X" header
	// Row 4 (auto): help buttons height (or 0 if hidden)
	float helpRowH = showHelpButtons ? (65.0f + 40.0f + 20.0f) : 0.0f; // card + padding*2 + gap
	// Row 3 (auto): showcase links (empty in C++ — no showcase)
	float linksRowH = 0.0f;
	// Row 2 (1fr): remaining space
	float mainRowH = usableH - titleRowH - linksRowH - helpRowH;
	if (mainRowH < 50.0f)
		mainRowH = 50.0f;

	// Set cursor to padded origin.
	ImVec2 origin = ImGui::GetCursorScreenPos();
	float startX = origin.x + PADDING;
	float startY = origin.y + PADDING;

	// ═════════════════════════════════════════════════════════════
	// Row 1, Column 1: Title heading
	// Tracker: "wow.export vX.X.X" title at large bold size, subtitle text below
	// ═════════════════════════════════════════════════════════════
	{
		ImFont* boldFont = app::theme::getBoldFont();
		static const std::string title = std::format("wow.export v{}", constants::VERSION);
		float titleFontSize = 28.0f;
		ImGui::GetWindowDrawList()->AddText(boldFont, titleFontSize,
			ImVec2(startX, startY), app::theme::FONT_HIGHLIGHT_U32, title.c_str());
	}

	// ═════════════════════════════════════════════════════════════
	// Row 2, Column 1: Navigation cards (replaces HomeShowcase)
	//      var(--border); border-radius: 10px; }
	// ═════════════════════════════════════════════════════════════
	{
		float navAreaY = startY + titleRowH;
		float navAreaH = mainRowH;

		// Draw border around the navigation card area (matching showcase border)
		ImVec2 areaMin(startX, navAreaY);
		ImVec2 areaMax(startX + colW, navAreaY + navAreaH);
		ImGui::GetWindowDrawList()->AddRectFilled(areaMin, areaMax, IM_COL32(0, 0, 0, 20), 10.0f);
		ImGui::GetWindowDrawList()->AddRect(areaMin, areaMax, app::theme::BORDER_U32, 10.0f, 0, 1.0f);

		// Get nav buttons filtered by current install type.
		const auto& navButtons = modules::getNavButtons();
		uint32_t installType = static_cast<uint32_t>(core::view->installType);

		// Collect visible buttons.
		std::vector<const modules::NavButton*> visibleButtons;
		for (const auto& btn : navButtons) {
			if (btn.installTypes & installType)
				visibleButtons.push_back(&btn);
		}

		// Layout: 4-column grid of ~100x100px cards with 10px gap.
		constexpr int COLS = 4;
		constexpr float CARD_GAP = 10.0f;
		constexpr float AREA_PAD = 20.0f;

		float innerW = colW - AREA_PAD * 2;
		float cardSize = (innerW - CARD_GAP * (COLS - 1)) / static_cast<float>(COLS);
		if (cardSize < 40.0f) cardSize = 40.0f;
		if (cardSize > 100.0f) cardSize = 100.0f;

		float gridStartX = areaMin.x + AREA_PAD;
		float gridStartY = areaMin.y + AREA_PAD;

		for (size_t i = 0; i < visibleButtons.size(); i++) {
			int col = static_cast<int>(i) % COLS;
			int row = static_cast<int>(i) / COLS;

			float cx = gridStartX + col * (cardSize + CARD_GAP);
			float cy = gridStartY + row * (cardSize + CARD_GAP);

			// Only render if within the area bounds
			if (cy + cardSize > areaMax.y - AREA_PAD)
				break;

			ImGui::SetCursorScreenPos(ImVec2(cx, cy));
			renderNavCard(*visibleButtons[i], cardSize);
		}
	}

	// ═════════════════════════════════════════════════════════════
	// Row 2, Column 2: "What's New" panel (#home-changes)
	//      border: 1px solid var(--border); border-radius: 10px;
	//      background: #0000002e url(./images/home-background.webp) no-repeat center / cover; }
	// ═════════════════════════════════════════════════════════════
	{
		float changesX = startX + colW + COL_GAP;
		float changesY = startY + titleRowH;
		float changesW = colW;
		float changesH = mainRowH;

		ImVec2 panelMin(changesX, changesY);
		ImVec2 panelMax(changesX + changesW, changesY + changesH);

		ImDrawList* draw = ImGui::GetWindowDrawList();

		// Background image (cover)
		if (s_bgTexture) {
			draw->AddImageRounded(
				static_cast<ImTextureID>(static_cast<uintptr_t>(s_bgTexture)),
				panelMin, panelMax, ImVec2(0, 0), ImVec2(1, 1),
				IM_COL32(255, 255, 255, 255), 10.0f);
		}

		// Dark overlay: #0000002e
		draw->AddRectFilled(panelMin, panelMax, IM_COL32(0, 0, 0, 46), 10.0f);

		// Border
		draw->AddRect(panelMin, panelMax, app::theme::BORDER_U32, 10.0f, 0, 1.0f);

		// Content: whatsNewHTML inside a scrolling child window.
		float contentPadX = 50.0f;
		float contentPadY = 20.0f;

		ImGui::SetCursorScreenPos(ImVec2(panelMin.x + 1, panelMin.y + 1));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(contentPadX, contentPadY));
		ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(0, 0, 0, 0));

		if (ImGui::BeginChild("##home-changes", ImVec2(changesW - 2, changesH - 2), ImGuiChildFlags_None,
		                      ImGuiWindowFlags_NoBackground)) {
			if (!core::view->whatsNewHTML.empty()) {
				ImGui::TextWrapped("%s", core::view->whatsNewHTML.c_str());
			} else {
				ImGui::PushStyleColor(ImGuiCol_Text, app::theme::FONT_FADED);
				ImGui::TextWrapped("No changelog available.");
				ImGui::PopStyleColor();
			}
		}
		ImGui::EndChild();

		ImGui::PopStyleColor();
		ImGui::PopStyleVar();
	}

	// ═════════════════════════════════════════════════════════════
	// Row 4: Help buttons (#home-help-buttons) — full width
	// @media (max-height: 899px) { #home-help-buttons { display: none; } }
	// ═════════════════════════════════════════════════════════════
	if (showHelpButtons) {
		float helpY = startY + titleRowH + mainRowH + 20.0f;
		float totalHelpW = 300.0f * 3 + 20.0f * 2;
		float helpStartX = startX + (usableW - totalHelpW) * 0.5f;

		//     <span>Join our Discord community for support!</span></div>
		ImGui::SetCursorScreenPos(ImVec2(helpStartX, helpY));
		renderHelpButton("discord", "Stuck? Need Help?",
		                 "Join our Discord community for support!",
		                 URL_DISCORD, s_discordIconTex);

		//     <span>wow.export is open-source, tinkerers are welcome!</span></div>
		ImGui::SetCursorScreenPos(ImVec2(helpStartX + 300.0f + 20.0f, helpY));
		renderHelpButton("github", "Gnomish Heritage?",
		                 "wow.export is open-source, tinkerers are welcome!",
		                 URL_GITHUB, s_githubIconTex);

		//     <span>Support development of wow.export through Patreon!</span></div>
		ImGui::SetCursorScreenPos(ImVec2(helpStartX + (300.0f + 20.0f) * 2, helpY));
		renderHelpButton("patreon", "Support Us!",
		                 "Support development of wow.export through Patreon!",
		                 URL_PATREON, s_patreonIconTex);
	}
}

/**
 * Render the home tab widget using ImGui.
 * JS template:
 *   <div class="tab" id="tab-home">
 *     <HomeShowcase />
 *     <div id="home-changes"><div v-html="$core.view.whatsNewHTML"></div></div>
 *     <div id="home-help-buttons"> ... </div>
 *   </div>
 */
void render() {
	renderHomeLayout();
}

void navigate(const char* module_name) {
	modules::set_active(module_name);
}

} // namespace tab_home
