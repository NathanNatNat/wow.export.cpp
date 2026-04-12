/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "wowhead.h"

#include <algorithm>
#include <string>
#include <vector>

namespace {

constexpr std::string_view CHARSET = "0zMcmVokRsaqbdrfwihuGINALpTjnyxtgevElBCDFHJKOPQSUWXYZ123456";

const std::unordered_map<int, int> WOWHEAD_SLOT_TO_SLOT_ID = {
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

int charset_index_of(char c) {
	auto pos = CHARSET.find(c);
	return pos != std::string_view::npos ? static_cast<int>(pos) : -1;
}

std::string_view seg_at(const std::vector<std::string>& segments, size_t i) {
	if (i < segments.size())
		return segments[i];
	return {};
}

char char_at(std::string_view str, size_t i, char default_char = '0') {
	return i < str.size() ? str[i] : default_char;
}

std::string_view substr_safe(std::string_view str, size_t pos) {
	return pos < str.size() ? str.substr(pos) : std::string_view{};
}

int decode(std::string_view str) {
	if (str.empty())
		return 0;

	if (str.size() == 1)
		return charset_index_of(str[0]);

	int result = 0;

	for (size_t i = 0; i < str.size(); i++) {
		size_t rev = str.size() - 1 - i;
		int value = charset_index_of(str[rev]);
		if (value == -1)
			return 0;

		for (size_t j = 0; j < i; j++)
			value *= 58;

		result += value;
	}

	return result;
}

std::string decompress_zeros(std::string_view str) {
	std::string result;
	result.reserve(str.size());

	for (size_t i = 0; i < str.size(); i++) {
		if (str[i] == '9' && i + 1 < str.size()) {
			int count = charset_index_of(str[i + 1]);
			if (count < 0) {
				result += str[i];
			} else {
				for (int j = 0; j < count; j++)
					result += "08";
				i++;
			}
		} else {
			result += str[i];
		}
	}

	return result;
}

std::string extract_hash_from_url(std::string_view url) {
	const std::string_view marker = "dressing-room#";
	auto pos = url.find(marker);
	if (pos == std::string_view::npos)
		return {};
	return std::string(url.substr(pos + marker.size()));
}

std::vector<std::string> split(const std::string& str, char delim) {
	std::vector<std::string> parts;
	size_t start = 0;
	for (size_t i = 0; i <= str.size(); i++) {
		if (i == str.size() || str[i] == delim) {
			parts.push_back(str.substr(start, i - start));
			start = i + 1;
		}
	}
	return parts;
}

bool starts_with(std::string_view str, char c) {
	return !str.empty() && str[0] == c;
}

WowheadResult parse_v15(const std::vector<std::string>& segments, int version) {
	WowheadResult r;
	r.version = version;
	r.race = decode(seg_at(segments, 0));

	auto combined = seg_at(segments, 1);
	r.gender = charset_index_of(char_at(combined, 0));
	r.clazz = charset_index_of(char_at(combined, 1));
	r.spec = charset_index_of(char_at(combined, 2));
	r.level = decode(substr_safe(combined, 3));

	auto opts = seg_at(segments, 2);
	r.npc_options = charset_index_of(char_at(opts, 0));
	r.pepe = charset_index_of(char_at(opts, 1));
	r.mount = decode(substr_safe(opts, 2));

	int equip_start = -1;
	for (size_t i = 6; i < segments.size(); i++) {
		if (!segments[i].empty() && starts_with(segments[i], '7')) {
			equip_start = static_cast<int>(i);
			break;
		}
	}

	if (equip_start > 6) {
		for (int i = 6; i < equip_start; i += 2) {
			int choice_id = decode(seg_at(segments, i + 1));
			if (choice_id != 0)
				r.customizations.push_back(choice_id);
		}
	}

	if (equip_start > 0) {
		int seg_idx = equip_start;
		int wh_slot = 1;

		while (seg_idx < static_cast<int>(segments.size()) && wh_slot <= 13) {
			std::string_view seg = seg_at(segments, seg_idx);
			std::string seg_owned;

			if (seg.size() >= 2 && seg[0] == '7') {
				char marker_char = seg[1];
				int marker_val = charset_index_of(marker_char);

				if (marker_val >= 0 && marker_val <= 12) {
					wh_slot = marker_val + 1;
					seg_owned = std::string(seg.substr(2));
				} else {
					seg_owned = std::string(seg.substr(2));
				}
				seg = seg_owned;
			}

			if (seg.empty()) {
				seg_idx++;
				continue;
			}

			int item_id = decode(seg);

			if (item_id > 0 && wh_slot <= 13) {
				auto it = WOWHEAD_SLOT_TO_SLOT_ID.find(wh_slot);
				if (it != WOWHEAD_SLOT_TO_SLOT_ID.end())
					r.equipment[it->second] = item_id;
			}

			seg_idx++;

			if (seg_idx < static_cast<int>(segments.size()) && !starts_with(segments[seg_idx], '7'))
				seg_idx++;

			if (wh_slot >= 12 && seg_idx < static_cast<int>(segments.size()) && !starts_with(segments[seg_idx], '7'))
				seg_idx++;

			wh_slot++;
		}
	}

	return r;
}

WowheadResult parse_legacy(const std::vector<std::string>& segments, int version) {
	WowheadResult r;
	r.version = version;
	r.race = decode(seg_at(segments, 0));

	auto seg1 = seg_at(segments, 1);
	r.gender = charset_index_of(char_at(seg1, 0));
	r.clazz = charset_index_of(char_at(seg1, 1));
	r.spec = charset_index_of(char_at(seg1, 2));
	r.level = decode(substr_safe(seg1, 3));

	auto seg2 = seg_at(segments, 2);
	r.npc_options = charset_index_of(char_at(seg2, 0));
	r.pepe = charset_index_of(char_at(seg2, 1));
	r.mount = decode(substr_safe(seg2, 2));

	for (int i = 3; i <= 30; i++) {
		int val = decode(seg_at(segments, i));
		if (val != 0)
			r.customizations.push_back(val);
	}

	static const std::unordered_map<int, int> SLOT_MAP_LEGACY = {
		{ 31, 1 },   // head
		{ 33, 3 },   // shoulders
		{ 35, 15 },  // back
		{ 37, 5 },   // chest
		{ 38, 10 },  // hands
		{ 40, 6 },   // waist
		{ 42, 7 },   // legs
		{ 44, 8 }    // feet
	};

	for (const auto& [seg_idx, slot_id] : SLOT_MAP_LEGACY) {
		auto seg = seg_at(segments, seg_idx);
		size_t start = seg.size() > 4 ? seg.size() - 4 : 0;
		int item_id = decode(seg.substr(start));
		if (item_id > 0)
			r.equipment[slot_id] = item_id;
	}

	return r;
}

WowheadResult wowhead_parse_hash(const std::string& hash) {
	if (hash.empty())
		return {};

	int version = charset_index_of(hash[0]);
	std::string decompressed = decompress_zeros(std::string_view(hash).substr(1));
	std::vector<std::string> segments = split(decompressed, '8');

	if (version >= 15)
		return parse_v15(segments, version);

	return parse_legacy(segments, version);
}

} // anonymous namespace

WowheadResult wowhead_parse(std::string_view url) {
	std::string hash = extract_hash_from_url(url);
	if (hash.empty())
		throw std::runtime_error("invalid wowhead url: missing dressing-room hash");

	return wowhead_parse_hash(hash);
}
