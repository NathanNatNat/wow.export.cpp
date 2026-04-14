/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <string>
#include <functional>
#include <vector>

/**
 * Markdown content component (ImGui immediate-mode equivalent).
 *
 * JS equivalent: Vue component with props: ['content'],
 * Renders markdown content with a custom scrollbar.
 *
 * In JS, markdown is parsed to HTML and rendered via v-html.
 * In C++/ImGui, markdown is parsed and rendered using ImGui text primitives
 * (TextWrapped, BulletText, etc.) since ImGui does not support HTML rendering.
 *
 * Supports: headers (h1–h3), bold, italic, inline code, code blocks,
 * unordered lists, links, images (as text placeholders), and paragraphs.
 */
namespace markdown_content {

/**
 * Persistent state for a single MarkdownContent widget instance.
 * Equivalent to the Vue component's data() reactive state.
 */
struct MarkdownContentState {
	float scroll_pos = 0.0f;
	float widget_height = 0.0f;
	bool is_dragging = false;
	float drag_start_y = 0.0f;
	float drag_start_top = 0.0f;
	std::string prev_content;  // Track content changes to reset scroll
};

/**
 * Parsed inline segment for rendering.
 * Each segment has a type and text content.
 */
enum class InlineType {
	Text,
	Bold,
	Italic,
	Code,
	Link,
	Image
};

struct InlineSegment {
	InlineType type = InlineType::Text;
	std::string text;
	std::string url;  // For links and images
};

/**
 * Parsed block element for rendering.
 */
enum class BlockType {
	Paragraph,
	Header1,
	Header2,
	Header3,
	ListItem,
	CodeBlock,
	LineBreak
};

struct Block {
	BlockType type = BlockType::Paragraph;
	std::string raw_text;  // For code blocks (no inline parsing)
	std::vector<InlineSegment> segments;  // For parsed inline content
};

/**
 * Parse markdown text into a list of blocks for rendering.
 * @param text Raw markdown text.
 * @return Vector of parsed blocks.
 */
std::vector<Block> parseMarkdown(const std::string& text);

/**
 * Parse inline formatting (bold, italic, code, links, images) into segments.
 * @param text Text to parse for inline formatting.
 * @return Vector of inline segments.
 */
std::vector<InlineSegment> parseInline(const std::string& text);

/**
 * Render a markdown content widget using ImGui.
 *
 * @param id              Unique ImGui ID string for this widget instance.
 * @param state           Persistent state across frames.
 * @param content         Raw markdown text to render.
 * @param onKBLink        Callback for knowledge-base links (::KB prefix).
 * @param onExternalLink  Callback for external links.
 */
void render(const char* id, MarkdownContentState& state,
            const std::string& content,
            const std::function<void(const std::string&)>& onKBLink = nullptr,
            const std::function<void(const std::string&)>& onExternalLink = nullptr);

} // namespace markdown_content
