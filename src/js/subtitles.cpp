/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "subtitles.h"
#include "casc/casc-source.h"

#include <cmath>
#include <regex>
#include <sstream>
#include <iomanip>
#include <optional>
#include <stdexcept>
#include <vector>

namespace subtitles {

static constexpr int FRAMES_PER_SECOND = 24;

static std::optional<int> parse_sbt_int_js(std::string_view sv) {
	std::size_t i = 0;
	while (i < sv.size() && (sv[i] == ' ' || sv[i] == '\t' || sv[i] == '\r' || sv[i] == '\n'))
		++i;

	bool negative = false;
	if (i < sv.size() && (sv[i] == '+' || sv[i] == '-')) {
		negative = (sv[i] == '-');
		++i;
	}

	if (i >= sv.size() || sv[i] < '0' || sv[i] > '9')
		return std::nullopt;

	int value = 0;
	while (i < sv.size() && sv[i] >= '0' && sv[i] <= '9') {
		value = value * 10 + (sv[i] - '0');
		++i;
	}

	return negative ? -value : value;
}

static int64_t parse_sbt_timestamp(std::string_view timestamp) {
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

	auto hours = parse_sbt_int_js(parts[0]);
	auto minutes = parse_sbt_int_js(parts[1]);
	auto seconds = parse_sbt_int_js(parts[2]);
	auto frames = parse_sbt_int_js(parts[3]);

	if (!hours.has_value() || !minutes.has_value() || !seconds.has_value() || !frames.has_value())
		return 0;

	int64_t total_ms = static_cast<int64_t>(hours.value() * 3600 + minutes.value() * 60 + seconds.value()) * 1000;
	int64_t frame_ms = std::llround((static_cast<double>(frames.value()) / FRAMES_PER_SECOND) * 1000.0);

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

static std::string format_srt_timestamp(int64_t ms) {
	int64_t hours = ms / 3600000;
	ms %= 3600000;

	int64_t minutes = ms / 60000;
	ms %= 60000;

	int64_t seconds = ms / 1000;
	int64_t millis = ms % 1000;

	return pad2(hours) + ":" + pad2(minutes) + ":" + pad2(seconds) + "," + pad3(millis);
}

static std::string format_vtt_timestamp(int64_t ms) {
	int64_t hours = ms / 3600000;
	ms %= 3600000;

	int64_t minutes = ms / 60000;
	ms %= 60000;

	int64_t seconds = ms / 1000;
	int64_t millis = ms % 1000;

	return pad2(hours) + ":" + pad2(minutes) + ":" + pad2(seconds) + "." + pad3(millis);
}

static int64_t parse_srt_timestamp(std::string_view timestamp) {
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

static std::vector<std::string> split_lines(std::string_view text) {
	std::vector<std::string> lines;
	std::size_t start = 0;

	for (std::size_t i = 0; i < text.size(); i++) {
		if (text[i] == '\n') {
			std::size_t end = i;
			if (end > start && text[end - 1] == '\r')
				end--;
			lines.emplace_back(text.substr(start, end - start));
			start = i + 1;
		}
	}

	if (start <= text.size())
		lines.emplace_back(text.substr(start));

	return lines;
}

static std::string trim(std::string_view sv) {
	std::size_t start = 0;
	while (start < sv.size() && (sv[start] == ' ' || sv[start] == '\t' || sv[start] == '\r' || sv[start] == '\n'))
		start++;

	std::size_t end = sv.size();
	while (end > start && (sv[end - 1] == ' ' || sv[end - 1] == '\t' || sv[end - 1] == '\r' || sv[end - 1] == '\n'))
		end--;

	return std::string(sv.substr(start, end - start));
}

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

static std::string sbt_to_srt(std::string_view sbt) {
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
	std::size_t total_size = 0;
	for (const auto& line : srt_lines)
		total_size += line.size() + 1;
	result.reserve(total_size);

	for (std::size_t i = 0; i < srt_lines.size(); i++) {
		if (i > 0)
			result += '\n';
		result += srt_lines[i];
	}

	return result;
}

static std::string srt_to_vtt(std::string_view srt) {
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
	std::size_t total_size = 0;
	for (const auto& line : vtt_lines)
		total_size += line.size() + 1;
	result.reserve(total_size);

	for (std::size_t i = 0; i < vtt_lines.size(); i++) {
		if (i > 0)
			result += '\n';
		result += vtt_lines[i];
	}

	return result;
}

std::string get_subtitles_vtt_from_text(std::string_view text, SubtitleFormat format) {
	std::string str(text);

	if (str.rfind("\xEF\xBB\xBF", 0) == 0)
		str = str.substr(3);

	std::string srt;
	if (format == SubtitleFormat::SBT)
		srt = sbt_to_srt(str);
	else
		srt = str;

	return srt_to_vtt(srt);
}

std::string get_subtitles_vtt(casc::CASC* casc, uint32_t file_data_id, SubtitleFormat format) {
	if (casc == nullptr)
		throw std::runtime_error("casc is null");

	BufferWrapper data = casc->getVirtualFileByID(file_data_id);
	std::string text = data.readString();
	return get_subtitles_vtt_from_text(text, format);
}

}
