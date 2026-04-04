/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "subtitles.h"

#include <cmath>
#include <regex>
#include <sstream>
#include <iomanip>
#include <optional>
#include <vector>

namespace subtitles {

static constexpr int FRAMES_PER_SECOND = 24;

int64_t parse_sbt_timestamp(std::string_view timestamp) {
	// Split by ':'
	std::vector<std::string_view> parts;
	std::size_t start = 0;
	for (std::size_t i = 0; i <= timestamp.size(); i++) {
		if (i == timestamp.size() || timestamp[i] == ':') {
			parts.push_back(timestamp.substr(start, i - start));
			start = i + 1;
		}
	}

	if (parts.size() != 4)
		return 0;

	auto parse_int = [](std::string_view sv) -> int {
		int result = 0;
		for (char ch : sv) {
			if (ch >= '0' && ch <= '9')
				result = result * 10 + (ch - '0');
		}
		return result;
	};

	int hours = parse_int(parts[0]);
	int minutes = parse_int(parts[1]);
	int seconds = parse_int(parts[2]);
	int frames = parse_int(parts[3]);

	int64_t total_ms = static_cast<int64_t>(hours * 3600 + minutes * 60 + seconds) * 1000;
	int64_t frame_ms = std::llround((static_cast<double>(frames) / FRAMES_PER_SECOND) * 1000.0);

	return total_ms + frame_ms;
}

static std::string pad2(int64_t n) {
	std::ostringstream oss;
	oss << std::setw(2) << std::setfill('0') << n;
	return oss.str();
}

static std::string pad3(int64_t n) {
	std::ostringstream oss;
	oss << std::setw(3) << std::setfill('0') << n;
	return oss.str();
}

std::string format_srt_timestamp(int64_t ms) {
	int64_t hours = ms / 3600000;
	ms %= 3600000;

	int64_t minutes = ms / 60000;
	ms %= 60000;

	int64_t seconds = ms / 1000;
	int64_t millis = ms % 1000;

	return pad2(hours) + ":" + pad2(minutes) + ":" + pad2(seconds) + "," + pad3(millis);
}

std::string format_vtt_timestamp(int64_t ms) {
	int64_t hours = ms / 3600000;
	ms %= 3600000;

	int64_t minutes = ms / 60000;
	ms %= 60000;

	int64_t seconds = ms / 1000;
	int64_t millis = ms % 1000;

	return pad2(hours) + ":" + pad2(minutes) + ":" + pad2(seconds) + "." + pad3(millis);
}

int64_t parse_srt_timestamp(std::string_view timestamp) {
	// Match pattern: HH:MM:SS,mmm
	std::regex re(R"((\d{2}):(\d{2}):(\d{2}),(\d{3}))");
	std::string ts_str(timestamp);
	std::smatch match;

	if (!std::regex_search(ts_str, match, re))
		return 0;

	int hours = std::stoi(match[1].str());
	int minutes = std::stoi(match[2].str());
	int seconds = std::stoi(match[3].str());
	int millis = std::stoi(match[4].str());

	return static_cast<int64_t>(hours * 3600 + minutes * 60 + seconds) * 1000 + millis;
}

/**
 * Split a string by \r?\n into lines.
 */
static std::vector<std::string> split_lines(std::string_view text) {
	std::vector<std::string> lines;
	std::size_t start = 0;

	for (std::size_t i = 0; i < text.size(); i++) {
		if (text[i] == '\n') {
			std::size_t end = i;
			// Strip \r before \n
			if (end > start && text[end - 1] == '\r')
				end--;
			lines.emplace_back(text.substr(start, end - start));
			start = i + 1;
		}
	}

	// Last line (no trailing newline)
	if (start <= text.size())
		lines.emplace_back(text.substr(start));

	return lines;
}

/**
 * Trim whitespace from both ends of a string.
 */
static std::string trim(std::string_view sv) {
	std::size_t start = 0;
	while (start < sv.size() && (sv[start] == ' ' || sv[start] == '\t' || sv[start] == '\r' || sv[start] == '\n'))
		start++;

	std::size_t end = sv.size();
	while (end > start && (sv[end - 1] == ' ' || sv[end - 1] == '\t' || sv[end - 1] == '\r' || sv[end - 1] == '\n'))
		end--;

	return std::string(sv.substr(start, end - start));
}

/**
 * Check if a string consists entirely of digits.
 */
static bool is_all_digits(const std::string& s) {
	if (s.empty())
		return false;

	for (char ch : s) {
		if (ch < '0' || ch > '9')
			return false;
	}

	return true;
}

struct SubtitleEntry {
	int64_t start;
	int64_t end;
	std::vector<std::string> text;
};

std::string sbt_to_srt(std::string_view sbt) {
	auto lines = split_lines(sbt);

	std::vector<SubtitleEntry> entries;
	std::optional<SubtitleEntry> current_entry;

	// timestamp line: 00:00:14:12 - 00:00:17:08
	std::regex timestamp_re(R"(^(\d{2}:\d{2}:\d{2}:\d{2})\s*-\s*(\d{2}:\d{2}:\d{2}:\d{2}))");

	for (const auto& line : lines) {
		std::string trimmed = trim(line);

		if (trimmed.empty()) {
			if (current_entry.has_value() && !current_entry->text.empty()) {
				entries.push_back(std::move(*current_entry));
				current_entry.reset();
			}
			continue;
		}

		std::smatch timestamp_match;
		if (std::regex_search(trimmed, timestamp_match, timestamp_re)) {
			if (current_entry.has_value() && !current_entry->text.empty())
				entries.push_back(std::move(*current_entry));

			current_entry = SubtitleEntry{
				parse_sbt_timestamp(timestamp_match[1].str()),
				parse_sbt_timestamp(timestamp_match[2].str()),
				{}
			};
			continue;
		}

		if (current_entry.has_value())
			current_entry->text.push_back(trimmed);
	}

	if (current_entry.has_value() && !current_entry->text.empty())
		entries.push_back(std::move(*current_entry));

	std::vector<std::string> srt_lines;
	for (std::size_t i = 0; i < entries.size(); i++) {
		const auto& entry = entries[i];
		srt_lines.push_back(std::to_string(i + 1));
		srt_lines.push_back(format_srt_timestamp(entry.start) + " --> " + format_srt_timestamp(entry.end));
		for (const auto& text : entry.text)
			srt_lines.push_back(text);
		srt_lines.emplace_back("");
	}

	std::string result;
	for (std::size_t i = 0; i < srt_lines.size(); i++) {
		if (i > 0)
			result += '\n';
		result += srt_lines[i];
	}

	return result;
}

std::string srt_to_vtt(std::string_view srt) {
	auto lines = split_lines(srt);

	std::vector<SubtitleEntry> entries;
	std::optional<SubtitleEntry> current_entry;

	// timestamp line: 00:00:02,433 --> 00:00:06,067
	std::regex timestamp_re(R"(^(\d{2}:\d{2}:\d{2},\d{3})\s*-->\s*(\d{2}:\d{2}:\d{2},\d{3}))");

	for (std::size_t i = 0; i < lines.size(); i++) {
		const auto& line = lines[i];
		std::string trimmed = trim(line);

		if (trimmed.empty()) {
			if (current_entry.has_value() && !current_entry->text.empty()) {
				entries.push_back(std::move(*current_entry));
				current_entry.reset();
			}
			continue;
		}

		// skip sequence numbers
		if (is_all_digits(trimmed) && !current_entry.has_value())
			continue;

		std::smatch timestamp_match;
		if (std::regex_search(trimmed, timestamp_match, timestamp_re)) {
			if (current_entry.has_value() && !current_entry->text.empty())
				entries.push_back(std::move(*current_entry));

			current_entry = SubtitleEntry{
				parse_srt_timestamp(timestamp_match[1].str()),
				parse_srt_timestamp(timestamp_match[2].str()),
				{}
			};
			continue;
		}

		if (current_entry.has_value())
			current_entry->text.push_back(trimmed);
	}

	if (current_entry.has_value() && !current_entry->text.empty())
		entries.push_back(std::move(*current_entry));

	std::vector<std::string> vtt_lines;
	vtt_lines.emplace_back("WEBVTT");
	vtt_lines.emplace_back("");

	for (const auto& entry : entries) {
		vtt_lines.push_back(format_vtt_timestamp(entry.start) + " --> " + format_vtt_timestamp(entry.end));
		for (const auto& text : entry.text)
			vtt_lines.push_back(text);
		vtt_lines.emplace_back("");
	}

	std::string result;
	for (std::size_t i = 0; i < vtt_lines.size(); i++) {
		if (i > 0)
			result += '\n';
		result += vtt_lines[i];
	}

	return result;
}

std::string get_subtitles_vtt(std::string_view text, SubtitleFormat format) {
	std::string str(text);

	// strip BOM if present (UTF-8 BOM: EF BB BF)
	if (str.size() >= 3 &&
		static_cast<unsigned char>(str[0]) == 0xEF &&
		static_cast<unsigned char>(str[1]) == 0xBB &&
		static_cast<unsigned char>(str[2]) == 0xBF) {
		str = str.substr(3);
	}

	std::string srt;
	if (format == SubtitleFormat::SBT)
		srt = sbt_to_srt(str);
	else
		srt = str;

	return srt_to_vtt(srt);
}

} // namespace subtitles
