/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "wowhead.h"

#include <algorithm>
#include <regex>
#include <string>

namespace wowhead {

static constexpr std::string_view charset = "0zMcmVokRsaqbdrfwihuGINALpTjnyxtgevElBCDFHJKOPQSUWXYZ123456";

// wowhead paperdoll slot index -> our slot id
// wowhead order: head, shoulder, back, chest, shirt, tabard, wrist, hands, waist, legs, feet, main, off
static const std::unordered_map<int, int> WOWHEAD_SLOT_TO_SLOT_ID = {
	{ 1, 1 },   // head
	{ 2, 3 },   // shoulders
	{ 3, 15 },  // back
	{ 4, 5 },   // chest
	{ 5, 4 },   // shirt
	{ 6, 19 },  // tabard
	{ 7, 9 },   // wrists
	{ 8, 10 },  // hands
	{ 9, 6 },   // waist
	{ 10, 7 },  // legs
	{ 11, 8 },  // feet
	{ 12, 16 }, // main-hand
	{ 13, 17 }  // off-hand
};

static int charset_index(char c) {
	auto pos = charset.find(c);
	return pos != std::string_view::npos ? static_cast<int>(pos) : -1;
}

static int decode(std::string_view str) {
	if (str.empty())
		return 0;

	if (str.size() == 1)
		return charset_index(str[0]);

	int result = 0;

	for (size_t i = 0; i < str.size(); i++) {
		// iterate in reverse order
		int value = charset_index(str[str.size() - 1 - i]);
		if (value == -1)
			return 0;

		for (size_t j = 0; j < i; j++)
			value *= 58;

		result += value;
	}

	return result;
}

static std::string decompress_zeros(std::string_view str) {
	std::string result;
	result.reserve(str.size());

	for (size_t i = 0; i < str.size(); i++) {
		if (str[i] == '9' && i + 1 < str.size()) {
			int count = charset_index(str[i + 1]);
			if (count < 0) {
				result += '9';
				result += str[i + 1];
			} else {
				for (int j = 0; j < count; j++)
					result += "08";
			}
			i++; // skip count char
		} else {
			result += str[i];
		}
	}

	return result;
}

static std::string extract_hash_from_url(const std::string& url) {
	static const std::regex pattern("dressing-room#(.+)");
	std::smatch match;
	if (std::regex_search(url, match, pattern))
		return match[1].str();

	return {};
}

static std::string safe_segment(const std::vector<std::string>& segments, size_t index) {
	return index < segments.size() ? segments[index] : std::string{};
}

static char safe_char(const std::string& s, size_t index, char fallback = '0') {
	return index < s.size() ? s[index] : fallback;
}

static std::string safe_substr(const std::string& s, size_t pos) {
	return pos < s.size() ? s.substr(pos) : std::string{};
}

static std::vector<std::string> split(const std::string& str, char delimiter) {
	std::vector<std::string> segments;
	size_t start = 0;
	size_t end;

	while ((end = str.find(delimiter, start)) != std::string::npos) {
		segments.push_back(str.substr(start, end - start));
		start = end + 1;
	}
	segments.push_back(str.substr(start));

	return segments;
}

static ParseResult parse_v15(const std::vector<std::string>& segments, int version) {
	ParseResult result;
	result.version = version;

	result.race = decode(safe_segment(segments, 0));

	// segment 1 is: gender (char 0) + class (char 1) + spec (char 2) + level (rest)
	const std::string combined = safe_segment(segments, 1);
	result.gender = charset_index(safe_char(combined, 0));
	result.class_ = charset_index(safe_char(combined, 1));
	result.spec = charset_index(safe_char(combined, 2));
	result.level = decode(safe_substr(combined, 3));

	// segment 2 is: npcOptions (char 0) + pepe (char 1) + mount (rest)
	const std::string opts = safe_segment(segments, 2);
	result.npc_options = charset_index(safe_char(opts, 0));
	result.pepe = charset_index(safe_char(opts, 1));
	result.mount = decode(safe_substr(opts, 2));

	// find equipment start by looking for first segment with 7 prefix
	int equip_start = -1;
	for (size_t i = 6; i < segments.size(); i++) {
		if (!segments[i].empty() && segments[i][0] == '7') {
			equip_start = static_cast<int>(i);
			break;
		}
	}

	// customization choices are between segment 6 and equipment start
	if (equip_start > 6) {
		for (int i = 6; i < equip_start; i += 2) {
			int choice_id = decode(safe_segment(segments, i + 1));
			if (choice_id != 0)
				result.customizations.push_back(choice_id);
		}
	}

	// parse equipment
	// v15 format: 7X marks equipment start, items follow sequentially
	// 7<slot> markers indicate slot jumps when slots are skipped
	// format: 7X<item>8<bonus>8<item>8<bonus>...87<slot><item>8<bonus>...
	if (equip_start > 0) {
		int seg_idx = equip_start;
		int wh_slot = 1;

		while (seg_idx < static_cast<int>(segments.size()) && wh_slot <= 13) {
			std::string seg = safe_segment(segments, seg_idx);

			// handle 7X prefix (equipment start or slot marker)
			if (seg.size() >= 2 && seg[0] == '7') {
				int marker_val = charset_index(seg[1]);

				// slot markers are 0-indexed, so marker_val 7 = slot 8
				// valid range: 0-12 maps to slots 1-13
				if (marker_val >= 0 && marker_val <= 12) {
					wh_slot = marker_val + 1;
					seg = seg.substr(2);
				} else {
					// invalid slot (like X=50): this is equipment section start marker
					// strip 7X prefix and continue with slot 1
					seg = seg.substr(2);
				}
			}

			// empty segment after stripping, skip
			if (seg.empty()) {
				seg_idx++;
				continue;
			}

			// decode item
			int item_id = decode(seg);

			if (item_id > 0 && wh_slot <= 13) {
				auto it = WOWHEAD_SLOT_TO_SLOT_ID.find(wh_slot);
				if (it != WOWHEAD_SLOT_TO_SLOT_ID.end())
					result.equipment[it->second] = item_id;
			}

			seg_idx++;

			// skip bonus segment if it doesn't have 7 prefix
			if (seg_idx < static_cast<int>(segments.size()) && (segments[seg_idx].empty() || segments[seg_idx][0] != '7'))
				seg_idx++;

			// weapons (slots 12-13) also have enchant segment
			if (wh_slot >= 12 && seg_idx < static_cast<int>(segments.size()) && (segments[seg_idx].empty() || segments[seg_idx][0] != '7'))
				seg_idx++;

			wh_slot++;
		}
	}

	return result;
}

static ParseResult parse_legacy(const std::vector<std::string>& segments, int version) {
	ParseResult result;
	result.version = version;

	// legacy format (v1-v14)
	result.race = decode(safe_segment(segments, 0));

	const std::string seg1 = safe_segment(segments, 1);
	result.gender = charset_index(safe_char(seg1, 0));
	result.class_ = charset_index(safe_char(seg1, 1));
	result.spec = charset_index(safe_char(seg1, 2));
	result.level = decode(safe_substr(seg1, 3));

	const std::string seg2 = safe_segment(segments, 2);
	result.npc_options = charset_index(safe_char(seg2, 0));
	result.pepe = charset_index(safe_char(seg2, 1));
	result.mount = decode(safe_substr(seg2, 2));

	// customization data (segments 3-30)
	for (int i = 3; i <= 30; i++) {
		int val = decode(safe_segment(segments, i));
		if (val != 0)
			result.customizations.push_back(val);
	}

	// equipment starts at segment 31 for legacy versions
	static const std::unordered_map<int, int> slot_map_legacy = {
		{ 31, 1 },   // head
		{ 33, 3 },   // shoulders
		{ 35, 15 },  // back
		{ 37, 5 },   // chest
		{ 38, 10 },  // hands
		{ 40, 6 },   // waist
		{ 42, 7 },   // legs
		{ 44, 8 }    // feet
	};

	for (const auto& [seg_idx, slot_id] : slot_map_legacy) {
		std::string seg = safe_segment(segments, seg_idx);
		size_t start = seg.size() > 4 ? seg.size() - 4 : 0;
		int item_id = decode(std::string_view(seg).substr(start));
		if (item_id > 0)
			result.equipment[slot_id] = item_id;
	}

	return result;
}

// No internal empty-check needed — caller wowhead_parse() validates hash is
// non-empty before calling (throws if extract_hash_from_url returns empty).
// The JS source has the same implicit reliance on the caller (wowhead.js line 64).
static ParseResult wowhead_parse_hash(const std::string& hash) {
	int version = charset_index(hash[0]);
	std::string decompressed = decompress_zeros(std::string_view(hash).substr(1));
	std::vector<std::string> segments = split(decompressed, '8');

	if (version >= 15)
		return parse_v15(segments, version);

	return parse_legacy(segments, version);
}

ParseResult wowhead_parse(const std::string& url) {
	std::string hash = extract_hash_from_url(url);
	if (hash.empty())
		throw std::runtime_error("invalid wowhead url: missing dressing-room hash");

	return wowhead_parse_hash(hash);
}

} // namespace wowhead
