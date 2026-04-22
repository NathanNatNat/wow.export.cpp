/*!
wow.export (https://github.com/Kruithne/wow.export)
Authors: Kruithne <kruithne@gmail.com>
License: MIT
 */
#include "markdown-content.h"

#include <imgui.h>
#include <cmath>
#include <algorithm>

#include "../../app.h"

namespace markdown_content {

// JS parseInline() calls escapeHtml() first (line 205) before applying regex patterns.
// C++ parses raw text directly because ImGui renders natively — no HTML escaping needed.
// Functionally identical for all practical markdown that does not contain raw HTML entities.
std::vector<InlineSegment> parseInline(const std::string& text) {
	std::vector<InlineSegment> segments;

	size_t i = 0;
	std::string current;

	auto flushCurrent = [&]() {
		if (!current.empty()) {
			segments.push_back({InlineType::Text, current, ""});
			current.clear();
		}
	};

	while (i < text.size()) {
		// Image: ![alt](src) — JS lines 208–216
		if (i + 1 < text.size() && text[i] == '!' && text[i + 1] == '[') {
			size_t altEnd = text.find(']', i + 2);
			if (altEnd != std::string::npos && altEnd + 1 < text.size() && text[altEnd + 1] == '(') {
				size_t srcEnd = text.find(')', altEnd + 2);
				if (srcEnd != std::string::npos) {
					flushCurrent();
					std::string alt = text.substr(i + 2, altEnd - (i + 2));
					std::string src = text.substr(altEnd + 2, srcEnd - (altEnd + 2));
					if (src.starts_with("./"))
						src = src.substr(2);
					if (!src.starts_with("http://") && !src.starts_with("https://") && !src.starts_with("data:"))
						src = "help_docs/" + src;
					segments.push_back({InlineType::Image, alt, src});
					i = srcEnd + 1;
					continue;
				}
			}
		}

		// Bold: **text** or __text__ — JS lines 219–220
		if (i + 1 < text.size() && ((text[i] == '*' && text[i + 1] == '*') || (text[i] == '_' && text[i + 1] == '_'))) {
			char marker = text[i];
			std::string endMarker(2, marker);
			size_t end = text.find(endMarker, i + 2);
			if (end != std::string::npos) {
				flushCurrent();
				segments.push_back({InlineType::Bold, text.substr(i + 2, end - (i + 2)), ""});
				i = end + 2;
				continue;
			}
		}

		// Italic: *text* (but not **) — JS line 223
		if (text[i] == '*' && (i + 1 >= text.size() || text[i + 1] != '*')) {
			size_t end = text.find('*', i + 1);
			if (end != std::string::npos) {
				flushCurrent();
				segments.push_back({InlineType::Italic, text.substr(i + 1, end - (i + 1)), ""});
				i = end + 1;
				continue;
			}
		}

		// Inline code: `text` — JS line 226
		if (text[i] == '`') {
			size_t end = text.find('`', i + 1);
			if (end != std::string::npos) {
				flushCurrent();
				segments.push_back({InlineType::Code, text.substr(i + 1, end - (i + 1)), ""});
				i = end + 1;
				continue;
			}
		}

		// Link: [label](href) — JS lines 229–233
		if (text[i] == '[') {
			size_t labelEnd = text.find(']', i + 1);
			if (labelEnd != std::string::npos && labelEnd + 1 < text.size() && text[labelEnd + 1] == '(') {
				size_t hrefEnd = text.find(')', labelEnd + 2);
				if (hrefEnd != std::string::npos) {
					flushCurrent();
					std::string label = text.substr(i + 1, labelEnd - (i + 1));
					std::string href = text.substr(labelEnd + 2, hrefEnd - (labelEnd + 2));
					segments.push_back({InlineType::Link, label, href});
					i = hrefEnd + 1;
					continue;
				}
			}
		}

		current += text[i];
		i++;
	}

	flushCurrent();
	return segments;
}

// JS equivalent: parseMarkdown() (lines 143–202)
std::vector<Block> parseMarkdown(const std::string& text) {
	std::vector<Block> blocks;
	bool in_list = false;
	bool in_code = false;
	std::vector<std::string> code_block;

	std::vector<std::string> lines;
	size_t start = 0;
	while (start < text.size()) {
		size_t end = text.find('\n', start);
		if (end == std::string::npos) {
			lines.push_back(text.substr(start));
			break;
		}
		lines.push_back(text.substr(start, end - start));
		start = end + 1;
	}

	for (const auto& line : lines) {
		if (line.starts_with("```")) {
			if (in_code) {
				std::string combined;
				for (size_t j = 0; j < code_block.size(); j++) {
					if (j > 0) combined += '\n';
					combined += code_block[j];
				}
				blocks.push_back({BlockType::CodeBlock, combined, {}});
				code_block.clear();
				in_code = false;
			} else {
				in_code = true;
			}
			continue;
		}

		if (in_code) {
			code_block.push_back(line);
			continue;
		}

		if (line.starts_with("### ")) {
			if (in_list) { in_list = false; }
			blocks.push_back({BlockType::Header3, "", parseInline(line.substr(4))});
		} else if (line.starts_with("## ")) {
			if (in_list) { in_list = false; }
			blocks.push_back({BlockType::Header2, "", parseInline(line.substr(3))});
		} else if (line.starts_with("# ")) {
			if (in_list) { in_list = false; }
			blocks.push_back({BlockType::Header1, "", parseInline(line.substr(2))});
		} else if (line.size() >= 2 && (line[0] == '*' || line[0] == '-' || line[0] == '+') && line[1] == ' ') {
			in_list = true;
			blocks.push_back({BlockType::ListItem, "", parseInline(line.substr(2))});
		} else {
			if (in_list) {
				in_list = false;
			}

			std::string trimmed = line;
			while (!trimmed.empty() && (trimmed.front() == ' ' || trimmed.front() == '\t'))
				trimmed.erase(trimmed.begin());
			while (!trimmed.empty() && (trimmed.back() == ' ' || trimmed.back() == '\t'))
				trimmed.pop_back();

			if (!trimmed.empty()) {
				blocks.push_back({BlockType::Paragraph, "", parseInline(line)});
			} else {
				blocks.push_back({BlockType::LineBreak, "", {}});
			}
		}
	}

	if (in_code && !code_block.empty()) {
		std::string combined;
		for (size_t j = 0; j < code_block.size(); j++) {
			if (j > 0) combined += '\n';
			combined += code_block[j];
		}
		blocks.push_back({BlockType::CodeBlock, combined, {}});
	}

	return blocks;
}


static void renderInlineSegments(const std::vector<InlineSegment>& segments,
                                  const std::function<void(const std::string&)>& onKBLink,
                                  const std::function<void(const std::string&)>& onExternalLink) {
	for (size_t i = 0; i < segments.size(); i++) {
		const auto& seg = segments[i];

		if (i > 0)
			ImGui::SameLine(0.0f, 0.0f);

		switch (seg.type) {
		case InlineType::Text:
			ImGui::TextUnformatted(seg.text.c_str());
			break;

		case InlineType::Bold:
			// CSS: .markdown-content-inner strong { font-weight: bold; }
			// Use the loaded bold font face for proper bold rendering.
			ImGui::PushFont(app::theme::getBoldFont());
			ImGui::TextUnformatted(seg.text.c_str());
			ImGui::PopFont();
			break;

		case InlineType::Italic:
			// CSS: .markdown-content-inner em { font-style: italic; }
			// ImGui has no inline italic without a separate italic font face; render with default color.
			ImGui::TextUnformatted(seg.text.c_str());
			break;

		case InlineType::Code: {
			// CSS: .markdown-content-inner code {
			//   background: rgba(0,0,0,0.3); padding: 2px 6px; border-radius: 3px; font-family: monospace;
			// }
			// Color inherits from parent (white). No separate monospace font is loaded.
			const float pad_y = 2.0f;
			ImVec2 cursor = ImGui::GetCursorScreenPos();
			ImVec2 text_size = ImGui::CalcTextSize(seg.text.c_str());
			ImGui::GetWindowDrawList()->AddRectFilled(
				ImVec2(cursor.x, cursor.y - pad_y),
				ImVec2(cursor.x + text_size.x, cursor.y + text_size.y + pad_y),
				IM_COL32(0, 0, 0, 77), 3.0f);
			ImGui::TextUnformatted(seg.text.c_str());
			break;
		}

		case InlineType::Link: {
			// CSS: .markdown-content-inner a { color: var(--font-highlight); text-decoration: underline; }
			ImGui::PushStyleColor(ImGuiCol_Text, app::theme::FONT_HIGHLIGHT);
			ImGui::TextUnformatted(seg.text.c_str());
			ImGui::PopStyleColor();
			// Draw underline manually (CSS text-decoration: underline)
			{
				ImVec2 rmin = ImGui::GetItemRectMin();
				ImVec2 rmax = ImGui::GetItemRectMax();
				ImGui::GetWindowDrawList()->AddLine(
					ImVec2(rmin.x, rmax.y),
					ImVec2(rmax.x, rmax.y),
					app::theme::FONT_HIGHLIGHT_U32, 1.0f);
			}
			if (ImGui::IsItemHovered()) {
				ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
				if (ImGui::IsItemClicked()) {
					if (seg.url.starts_with("::KB") && onKBLink) {
						onKBLink(seg.url.substr(2));
					} else if (onExternalLink) {
						onExternalLink(seg.url);
					}
				}
			}
			break;
		}

		case InlineType::Image:
			// ImGui cannot render inline images without pre-loading as GL textures.
			// CSS: .markdown-content-inner img { border: 1px solid ...; border-radius: 4px; max-width: 100%; }
			// Rendered as alt-text placeholder — known limitation (TODO #115).
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
			ImGui::TextUnformatted(("[Image: " + seg.text + "]").c_str());
			ImGui::PopStyleColor();
			break;
		}
	}
}


void render(const char* id, MarkdownContentState& state,
            const std::string& content,
            const std::function<void(const std::string&)>& onKBLink,
            const std::function<void(const std::string&)>& onExternalLink) {
	ImGui::PushID(id);

	// Watch: content change → reset scroll_pos (JS lines 47–51)
	if (content != state.prev_content) {
		state.prev_content = content;
		state.scroll_pos = 0.0f;
	}

	// <div ref="root" class="markdown-content">
	// CSS: .markdown-content { background: rgb(0 0 0 / 22%); border-radius: 10px; padding: 20px; font-size: 20px; }
	ImVec2 avail = ImGui::GetContentRegionAvail();
	float viewport_height = avail.y;

	{
		ImVec2 pos = ImGui::GetCursorScreenPos();
		ImDrawList* dl = ImGui::GetWindowDrawList();
		dl->AddRectFilled(pos, ImVec2(pos.x + avail.x, pos.y + avail.y),
		                  IM_COL32(0, 0, 0, 56), 10.0f);
	}

	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20.0f);
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20.0f);
	ImVec2 innerAvail(avail.x - 40.0f, avail.y - 40.0f);
	ImGui::BeginChild("##md_container", innerAvail, false,
	                   ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

	const ImVec2 containerPos = ImGui::GetCursorScreenPos();
	const ImVec2 containerSize = ImGui::GetContentRegionAvail();
	viewport_height = containerSize.y;

	if (!content.empty()) {
		std::vector<Block> blocks = parseMarkdown(content);

		ImGui::SetCursorPosY(ImGui::GetCursorPosY() - state.scroll_pos);

		// CSS: .markdown-content { font-size: 20px; } — scale up from DEFAULT_FONT_SIZE (16px)
		const float base_scale = 20.0f / app::theme::DEFAULT_FONT_SIZE;
		ImGui::SetWindowFontScale(base_scale);

		for (const auto& block : blocks) {
			switch (block.type) {
			case BlockType::Header1:
				// CSS: .markdown-content-inner h1 { font-size: 1.8em; font-weight: bold; }
				// No border-bottom in CSS — Separator() is not added here.
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
				ImGui::SetWindowFontScale(1.8f * base_scale);
				renderInlineSegments(block.segments, onKBLink, onExternalLink);
				ImGui::SetWindowFontScale(base_scale);
				ImGui::PopStyleColor();
				break;

			case BlockType::Header2:
				// CSS: .markdown-content-inner h2 { font-size: 1.5em; font-weight: bold; }
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
				ImGui::SetWindowFontScale(1.5f * base_scale);
				renderInlineSegments(block.segments, onKBLink, onExternalLink);
				ImGui::SetWindowFontScale(base_scale);
				ImGui::PopStyleColor();
				break;

			case BlockType::Header3:
				// CSS: .markdown-content-inner h3 { font-size: 1.2em; font-weight: bold; }
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
				ImGui::SetWindowFontScale(1.2f * base_scale);
				renderInlineSegments(block.segments, onKBLink, onExternalLink);
				ImGui::SetWindowFontScale(base_scale);
				ImGui::PopStyleColor();
				break;

			case BlockType::ListItem:
				// CSS: .markdown-content-inner ul { padding-left: 2em; list-style-type: disc; }
				// 2em at 20px base = 40px indent. Use GetFontSize() (= 20px at base_scale) × 2.
				ImGui::Indent(2.0f * ImGui::GetFontSize());
				ImGui::TextUnformatted("\xe2\x80\xa2"); // U+2022 BULLET (disc)
				ImGui::SameLine();
				renderInlineSegments(block.segments, onKBLink, onExternalLink);
				ImGui::Unindent(2.0f * ImGui::GetFontSize());
				break;

			case BlockType::CodeBlock:
				// CSS: .markdown-content-inner pre { background: rgba(0,0,0,0.3); padding: 10px; border-radius: 5px; }
				{
					ImVec2 codePos = ImGui::GetCursorScreenPos();
					ImVec2 textSize = ImGui::CalcTextSize(block.raw_text.c_str(), nullptr, false, ImGui::GetContentRegionAvail().x - 20.0f);
					ImVec2 bgMin = codePos;
					ImVec2 bgMax(codePos.x + ImGui::GetContentRegionAvail().x, codePos.y + textSize.y + 20.0f);
					ImGui::GetWindowDrawList()->AddRectFilled(bgMin, bgMax, IM_COL32(0, 0, 0, 77), 5.0f);
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.0f);
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.8f, 1.0f));
					ImGui::TextUnformatted(block.raw_text.c_str());
					ImGui::PopStyleColor();
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.0f);
				}
				break;

			case BlockType::Paragraph:
				renderInlineSegments(block.segments, onKBLink, onExternalLink);
				break;

			case BlockType::LineBreak:
				ImGui::Spacing();
				break;
			}
		}

		ImGui::SetWindowFontScale(1.0f);

		float content_height = ImGui::GetCursorPosY() + state.scroll_pos;
		float scrollable_height = std::max(0.0f, content_height - viewport_height);

		// update_scrollbar() — JS lines 59–72
		if (content_height > 0.0f) {
			float ratio = viewport_height / content_height;
			state.widget_height = std::max(30.0f, viewport_height * ratio);
		}
		state.scroll_pos = std::clamp(state.scroll_pos, 0.0f, scrollable_height);

		// @wheel="wheelMouse" — JS lines 126–141
		ImGuiIO& io = ImGui::GetIO();
		if (ImGui::IsWindowHovered() && io.MouseWheel != 0.0f) {
			float delta = io.MouseWheel < 0 ? 1.0f : -1.0f;
			state.scroll_pos += delta * 30.0f;
			state.scroll_pos = std::clamp(state.scroll_pos, 0.0f, scrollable_height);
		}

		// Scrollbar drag handling — JS lines 92–123
		if (state.is_dragging) {
			if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
				// moveMouse — JS lines 98–119
				if (scrollable_height > 0.0f) {
					float delta_y = io.MousePos.y - state.drag_start_y;
					float widget_top = state.drag_start_top + delta_y;
					float widget_range = viewport_height - state.widget_height;
					float clamped_top = std::clamp(widget_top, 0.0f, widget_range);
					state.scroll_pos = (clamped_top / widget_range) * scrollable_height;
				}
			} else {
				// stopMouse — JS line 122–124
				state.is_dragging = false;
			}
		}

		// Render scrollbar widget — JS template line 253
		// CSS: .markdown-content .vscroller { right: 3px; width: 8px; opacity: 0.7; }
		// CSS: .vscroller > div { background: var(--border); border: 1px solid var(--border); border-radius: 5px; }
		// CSS: .vscroller:hover > div, .vscroller.using > div { background: var(--font-highlight); }
		if (scrollable_height > 0.0f) {
			float widget_range = viewport_height - state.widget_height;
			float widget_top = (scrollable_height > 0.0f)
				? (state.scroll_pos / scrollable_height) * widget_range
				: 0.0f;

			ImDrawList* drawList = ImGui::GetWindowDrawList();
			const float scrollbar_width = 8.0f;
			// right: 3px from the container edge
			const float scrollbar_x = containerPos.x + containerSize.x - scrollbar_width - 3.0f;

			ImVec2 thumbMin(scrollbar_x, containerPos.y + widget_top);
			ImVec2 thumbMax(scrollbar_x + scrollbar_width, containerPos.y + widget_top + state.widget_height);

			// opacity: 0.7 on .vscroller → alpha = 255 × 0.7 ≈ 179
			bool thumb_hovered = ImGui::IsMouseHoveringRect(thumbMin, thumbMax);
			const ImU32 thumbFill = (state.is_dragging || thumb_hovered)
				? IM_COL32(255, 255, 255, 179)   // var(--font-highlight) × 0.7 opacity
				: IM_COL32(108, 117, 125, 179);  // var(--border) × 0.7 opacity
			// border always remains var(--border) (hover rule only changes background)
			const ImU32 thumbBorder = IM_COL32(108, 117, 125, 179);

			drawList->AddRectFilled(thumbMin, thumbMax, thumbFill, 5.0f);
			drawList->AddRect(thumbMin, thumbMax, thumbBorder, 5.0f, 0, 1.0f);

			// startMouse — JS lines 92–96
			if (ImGui::IsMouseHoveringRect(thumbMin, thumbMax) && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
				state.drag_start_y = io.MousePos.y;
				state.drag_start_top = widget_top;
				state.is_dragging = true;
			}
		}
	}

	ImGui::EndChild();
	ImGui::PopID();
}

} // namespace markdown_content
