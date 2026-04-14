/*!
wow.export (https://github.com/Kruithne/wow.export)
Authors: Kruithne <kruithne@gmail.com>
License: MIT
 */
#include "markdown-content.h"

#include <imgui.h>
#include <cmath>
#include <algorithm>
#include <regex>

#include "../../app.h"

namespace markdown_content {

/**
 * Escape HTML entities — not needed for ImGui rendering but kept for
 * functional parity with the JS escapeHtml() method.
 * In ImGui we render text directly, so no escaping is necessary.
 */

/**
 * Parse inline markdown formatting into segments.
 * JS equivalent: parseInline() (lines 204–237)
 *
 * Supports: **bold**, __bold__, *italic*, `code`, [link](url), ![alt](src)
 */
std::vector<InlineSegment> parseInline(const std::string& text) {
std::vector<InlineSegment> segments;

// Process text character by character to find inline formatting
size_t i = 0;
std::string current;

auto flushCurrent = [&]() {
if (!current.empty()) {
segments.push_back({InlineType::Text, current, ""});
current.clear();
}
};

while (i < text.size()) {
// Image: ![alt](src)
if (i + 1 < text.size() && text[i] == '!' && text[i + 1] == '[') {
size_t altEnd = text.find(']', i + 2);
if (altEnd != std::string::npos && altEnd + 1 < text.size() && text[altEnd + 1] == '(') {
size_t srcEnd = text.find(')', altEnd + 2);
if (srcEnd != std::string::npos) {
flushCurrent();
std::string alt = text.substr(i + 2, altEnd - (i + 2));
std::string src = text.substr(altEnd + 2, srcEnd - (altEnd + 2));
// JS: if src starts with './' strip it; if not http/https/data, prefix with help_docs/
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

// Bold: **text** or __text__
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

// Italic: *text* (but not **)
if (text[i] == '*' && (i + 1 >= text.size() || text[i + 1] != '*')) {
size_t end = text.find('*', i + 1);
if (end != std::string::npos) {
flushCurrent();
segments.push_back({InlineType::Italic, text.substr(i + 1, end - (i + 1)), ""});
i = end + 1;
continue;
}
}

// Inline code: `text`
if (text[i] == '`') {
size_t end = text.find('`', i + 1);
if (end != std::string::npos) {
flushCurrent();
segments.push_back({InlineType::Code, text.substr(i + 1, end - (i + 1)), ""});
i = end + 1;
continue;
}
}

// Link: [label](href)
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

/**
 * Parse markdown text into blocks.
 * JS equivalent: parseMarkdown() (lines 143–202)
 */
std::vector<Block> parseMarkdown(const std::string& text) {
std::vector<Block> blocks;
bool in_list = false;
bool in_code = false;
std::vector<std::string> code_block;

// Split text into lines
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
// Code block fence
if (line.starts_with("```")) {
if (in_code) {
std::string combined;
for (size_t i = 0; i < code_block.size(); i++) {
if (i > 0) combined += '\n';
combined += code_block[i];
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

// Headers
if (line.starts_with("### ")) {
if (in_list) { in_list = false; }
blocks.push_back({BlockType::Header3, "", parseInline(line.substr(4))});
} else if (line.starts_with("## ")) {
if (in_list) { in_list = false; }
blocks.push_back({BlockType::Header2, "", parseInline(line.substr(3))});
} else if (line.starts_with("# ")) {
if (in_list) { in_list = false; }
blocks.push_back({BlockType::Header1, "", parseInline(line.substr(2))});
}
// Lists: lines starting with *, -, or + followed by space
else if (line.size() >= 2 && (line[0] == '*' || line[0] == '-' || line[0] == '+') && line[1] == ' ') {
in_list = true;
blocks.push_back({BlockType::ListItem, "", parseInline(line.substr(2))});
} else {
if (in_list) {
in_list = false;
}

// Trim to check for empty lines
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

// Close unclosed code block
if (in_code && !code_block.empty()) {
std::string combined;
for (size_t i = 0; i < code_block.size(); i++) {
if (i > 0) combined += '\n';
combined += code_block[i];
}
blocks.push_back({BlockType::CodeBlock, combined, {}});
}

return blocks;
}


/**
 * Render inline segments using ImGui text functions.
 * Bold uses a placeholder (ImGui doesn't support inline bold natively).
 */
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
// ImGui doesn't have inline bold — render with emphasis color
ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
ImGui::TextUnformatted(seg.text.c_str());
ImGui::PopStyleColor();
break;
case InlineType::Italic:
// ImGui doesn't have inline italic — render with slightly dimmed color
ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.9f, 1.0f));
ImGui::TextUnformatted(seg.text.c_str());
ImGui::PopStyleColor();
break;
case InlineType::Code:
ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.7f, 0.5f, 1.0f));
ImGui::TextUnformatted(seg.text.c_str());
ImGui::PopStyleColor();
break;
case InlineType::Link: {
ImGui::PushStyleColor(ImGuiCol_Text, app::theme::FONT_ALT);
ImGui::TextUnformatted(seg.text.c_str());
ImGui::PopStyleColor();
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
// ImGui can't render inline images from URLs.
// Render as placeholder text showing alt text.
ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
ImGui::TextUnformatted(("[Image: " + seg.text + "]").c_str());
ImGui::PopStyleColor();
break;
}
}
}


/**
 * Render markdown content with custom scrollbar.
 * JS equivalent: template + methods (lines 251–254, 54–141)
 *
 * template: `<div ref="root" class="markdown-content" @wheel="wheelMouse">
 *     <div ref="content" class="markdown-content-inner" v-html="htmlContent"
 *          :style="{ transform: 'translateY(' + (-scroll_pos) + 'px)' }"></div>
 *     <div class="vscroller" ref="scroller" @mousedown="startMouse"
 *          :class="{ using: is_dragging }"
 *          :style="{ top: get_widget_top() + 'px', height: widget_height + 'px' }"><div></div></div>
 * </div>`
 */
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
ImVec2 avail = ImGui::GetContentRegionAvail();
float viewport_height = avail.y;

// Begin clipping region for scrollable content
ImGui::BeginChild("##md_container", avail, false,
                   ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

const ImVec2 containerPos = ImGui::GetCursorScreenPos();
const ImVec2 containerSize = ImGui::GetContentRegionAvail();
viewport_height = containerSize.y;

// Parse and render content
if (!content.empty()) {
std::vector<Block> blocks = parseMarkdown(content);

// Render blocks with scroll offset applied
// JS: style="transform: translateY(-scroll_pos px)"
ImGui::SetCursorPosY(ImGui::GetCursorPosY() - state.scroll_pos);

for (const auto& block : blocks) {
switch (block.type) {
case BlockType::Header1:
ImGui::PushFont(nullptr); // Use default font (no header font available)
ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
ImGui::SetWindowFontScale(1.6f);
renderInlineSegments(block.segments, onKBLink, onExternalLink);
ImGui::SetWindowFontScale(1.0f);
ImGui::PopStyleColor();
ImGui::PopFont();
ImGui::Separator();
break;

case BlockType::Header2:
ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
ImGui::SetWindowFontScale(1.3f);
renderInlineSegments(block.segments, onKBLink, onExternalLink);
ImGui::SetWindowFontScale(1.0f);
ImGui::PopStyleColor();
break;

case BlockType::Header3:
ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
ImGui::SetWindowFontScale(1.1f);
renderInlineSegments(block.segments, onKBLink, onExternalLink);
ImGui::SetWindowFontScale(1.0f);
ImGui::PopStyleColor();
break;

case BlockType::ListItem:
ImGui::Indent(16.0f);
ImGui::TextUnformatted("•");
ImGui::SameLine();
renderInlineSegments(block.segments, onKBLink, onExternalLink);
ImGui::Unindent(16.0f);
break;

case BlockType::CodeBlock:
ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
ImGui::BeginChild("##code", ImVec2(0, 0), true, ImGuiWindowFlags_AlwaysAutoResize);
ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.8f, 1.0f));
ImGui::TextUnformatted(block.raw_text.c_str());
ImGui::PopStyleColor();
ImGui::EndChild();
ImGui::PopStyleColor();
break;

case BlockType::Paragraph:
renderInlineSegments(block.segments, onKBLink, onExternalLink);
break;

case BlockType::LineBreak:
ImGui::Spacing();
break;
}
}

// Get content height for scrollbar calculation
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
if (scrollable_height > 0.0f) {
// get_widget_top() — JS lines 74–90
float widget_range = viewport_height - state.widget_height;
float widget_top = (scrollable_height > 0.0f)
? (state.scroll_pos / scrollable_height) * widget_range
: 0.0f;

// Draw scrollbar track and thumb
ImDrawList* drawList = ImGui::GetWindowDrawList();
const float scrollbar_width = 8.0f;
const float scrollbar_x = containerPos.x + containerSize.x - scrollbar_width - 2.0f;

// Scrollbar thumb
ImVec2 thumbMin(scrollbar_x, containerPos.y + widget_top);
ImVec2 thumbMax(scrollbar_x + scrollbar_width, containerPos.y + widget_top + state.widget_height);

ImU32 thumbColor = state.is_dragging
? IM_COL32(180, 180, 180, 200)
: IM_COL32(120, 120, 120, 150);

drawList->AddRectFilled(thumbMin, thumbMax, thumbColor, 4.0f);

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
