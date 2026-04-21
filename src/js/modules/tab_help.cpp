/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "tab_help.h"
#include "../log.h"
#include "../core.h"
#include "../modules.h"
#include "../components/markdown-content.h"

#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <format>
#include <regex>

#include <imgui.h>

namespace tab_help {

// --- Help article data ---

struct HelpArticle {
	std::vector<std::string> tags;
	std::string title;
	std::string kb_id;
	std::string body;
};

// --- File-local state ---

static std::vector<HelpArticle> help_articles;
static bool help_loaded = false;

static char search_query[256] = "";
static std::vector<const HelpArticle*> filtered_articles;
static const HelpArticle* selected_article = nullptr;
static markdown_content::MarkdownContentState md_state;
// JS: clearTimeout(filter_timeout); filter_timeout = setTimeout(() => {...}, 300)
// Tracks when search_query was last modified, for 300ms debounce.
static double filter_debounce_start = -1.0;

// --- Internal functions ---

static std::string trim(const std::string& s) {
	size_t start = s.find_first_not_of(" \t\r\n");
	size_t end = s.find_last_not_of(" \t\r\n");
	if (start == std::string::npos)
		return "";
	return s.substr(start, end - start + 1);
}

static std::string to_lower(const std::string& s) {
	std::string result = s;
	std::transform(result.begin(), result.end(), result.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return result;
}

static std::vector<std::string> split_whitespace(const std::string& s) {
	std::vector<std::string> tokens;
	std::istringstream iss(s);
	std::string token;
	while (iss >> token)
		tokens.push_back(token);
	return tokens;
}

static void load_help_docs() {
	if (help_loaded)
		return;

	core::showLoadingScreen(1);

	try {
		core::progressLoadingScreen("loading help documents...");

		namespace fs = std::filesystem;
		const fs::path help_dir = "src/help_docs";
		logging::write(std::format("loading help docs from: {}", help_dir.string()));

		if (!fs::exists(help_dir) || !fs::is_directory(help_dir)) {
			logging::write("help_docs directory not found");
			core::hideLoadingScreen();
			return;
		}

		int md_count = 0;
		for (const auto& entry : fs::directory_iterator(help_dir)) {
			if (!entry.is_regular_file())
				continue;

			const std::string filename = entry.path().filename().string();
			if (filename.size() < 3 || filename.substr(filename.size() - 3) != ".md")
				continue;

			md_count++;
			std::ifstream file(entry.path());
			if (!file.is_open())
				continue;

			std::string content((std::istreambuf_iterator<char>(file)),
				std::istreambuf_iterator<char>());
			file.close();

			// Split content into lines.
			std::vector<std::string> lines;
			std::istringstream stream(content);
			std::string line;
			while (std::getline(stream, line))
				lines.push_back(line);

			if (lines.size() < 2)
				continue;

			// First line: tag line starting with '!'.
			std::string tag_line = trim(lines[0]);
			if (tag_line.empty() || tag_line[0] != '!')
				continue;

			std::string tags_str = to_lower(trim(tag_line.substr(1)));
			std::vector<std::string> tags = split_whitespace(tags_str);

			// Second line: title starting with '#'.
			std::string title_line = trim(lines[1]);
			if (title_line.empty() || title_line[0] != '#')
				continue;

			std::string title = trim(title_line.substr(1));

			// Body is everything from line 1 onwards (title + rest).
			std::string body;
			for (size_t i = 1; i < lines.size(); i++) {
				if (i > 1)
					body += '\n';
				body += lines[i];
			}

			// Parse KB ID from title (e.g., "KB001: Some Title").
			std::string kb_id;
			std::string title_text = title;
			std::regex kb_regex(R"(^(KB\d+):\s*(.+))");
			std::smatch kb_match;
			if (std::regex_match(title, kb_match, kb_regex)) {
				kb_id = kb_match[1].str();
				title_text = kb_match[2].str();
			}

			help_articles.push_back({ tags, title_text, kb_id, body });
			logging::write(std::format("loaded help article: {}", title_text));
		}

		logging::write(std::format("found {} markdown files", md_count));
		logging::write(std::format("loaded {} help articles total", help_articles.size()));
		help_loaded = true;
		core::hideLoadingScreen();
	} catch (const std::exception& e) {
		logging::write(std::format("failed to load help documents: {}", e.what()));
		core::hideLoadingScreen();
		core::setToast("error", "failed to load help documents");
	}
}

static std::vector<const HelpArticle*> filter_articles(const std::string& search) {
	std::string trimmed = trim(search);
	if (trimmed.empty()) {
		std::vector<const HelpArticle*> result;
		result.reserve(help_articles.size());
		for (const auto& article : help_articles)
			result.push_back(&article);
		return result;
	}

	std::vector<std::string> keywords = split_whitespace(to_lower(trimmed));

	struct ScoredArticle {
		const HelpArticle* article;
		int matched;
		bool has_default;
	};

	std::vector<ScoredArticle> scored;
	for (const auto& article : help_articles) {
		bool has_default = std::find(article.tags.begin(), article.tags.end(), "default") != article.tags.end();
		int score = 0;

		for (const auto& kw : keywords) {
			std::string kb_lower = to_lower(article.kb_id);
			if (!article.kb_id.empty() && kb_lower == kw)
				score += 3;
			else if (!article.kb_id.empty() && kb_lower.find(kw) != std::string::npos)
				score += 2;

			for (const auto& tag : article.tags) {
				if (tag == kw)
					score += 2;
				else if (tag.find(kw) != std::string::npos)
					score += 1;
			}
		}

		if (score > 0 || has_default)
			scored.push_back({ &article, score, has_default });
	}

	std::sort(scored.begin(), scored.end(),
		[](const ScoredArticle& a, const ScoredArticle& b) { return b.matched < a.matched; });

	std::vector<const HelpArticle*> result;
	result.reserve(scored.size());
	for (const auto& s : scored)
		result.push_back(s.article);
	return result;
}

static void update_filter() {
	filtered_articles = filter_articles(std::string(search_query));
}

// --- Public API ---

void registerTab() {
	modules::register_context_menu_option("tab_help", "Help", "help.svg");
}

void mounted() {
	load_help_docs();

	update_filter();

	// Check for pending KB article from modules::openHelpArticle().
	std::string pending_kb = modules::consumePendingKbId();
	if (!pending_kb.empty()) {
		for (const auto& article : help_articles) {
			if (article.kb_id == pending_kb) {
				selected_article = &article;
				break;
			}
		}
	}

	// Default to KB002 if nothing selected.
	if (!selected_article) {
		for (const auto& article : help_articles) {
			if (article.kb_id == "KB002") {
				selected_article = &article;
				break;
			}
		}
	}
}

void render() {
	// Left panel: article list.
	ImGui::BeginChild("##help-list", ImVec2(300, -ImGui::GetFrameHeightWithSpacing()), true);
	{
		ImGui::Text("Help");
		ImGui::Separator();

		// Search bar.
		// JS: debounced_filter(value) uses clearTimeout/setTimeout(..., 300) for 300ms debounce.
		if (ImGui::InputText("##search", search_query, sizeof(search_query)))
			filter_debounce_start = ImGui::GetTime(); // restart debounce timer on keystroke

		// Apply debounce: update filter 300ms after last keystroke (matches JS setTimeout 300).
		if (filter_debounce_start >= 0.0 && ImGui::GetTime() - filter_debounce_start >= 0.3) {
			update_filter();
			filter_debounce_start = -1.0;
		}

		ImGui::Separator();

		// Article list.
		for (const auto* article : filtered_articles) {
			bool is_selected = (selected_article == article);

			std::string label = article->title;
			if (!article->kb_id.empty())
				label = std::format("[{}] {}", article->kb_id, article->title);

			if (ImGui::Selectable(label.c_str(), is_selected)) {
				selected_article = article;
				// Reset markdown scroll state when selecting a new article.
				md_state = markdown_content::MarkdownContentState{};
			}

			// Show tags as tooltip.
			if (ImGui::IsItemHovered() && !article->tags.empty()) {
				std::string tags_str;
				for (size_t i = 0; i < article->tags.size(); i++) {
					if (i > 0)
						tags_str += ", ";
					tags_str += article->tags[i];
				}
				ImGui::SetTooltip("%s", tags_str.c_str());
			}
		}
	}
	ImGui::EndChild();

	ImGui::SameLine();

	// Right panel: article content rendered as markdown.
	// JS equivalent: <component :is="$components.MarkdownContent" v-if="selected_article" :content="selected_article.body">
	ImGui::BeginChild("##help-content-wrapper", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), true);
	{
		if (selected_article) {
			markdown_content::render("##help-article-md", md_state, selected_article->body,
				// KB link handler: navigate to the referenced help article.
				[](const std::string& kb_id) { modules::openHelpArticle(kb_id); });
		} else {
			ImGui::TextDisabled("Select an article to view");
		}
	}
	ImGui::EndChild();

	// Bottom: Go Back button.
	if (ImGui::Button("Go Back"))
		modules::go_to_landing();
}

} // namespace tab_help
