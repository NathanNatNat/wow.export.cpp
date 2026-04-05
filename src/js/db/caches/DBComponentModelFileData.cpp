/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
*/

#include "DBComponentModelFileData.h"

#include "../../log.h"
#include "../../casc/db2.h"
#include "../WDCReader.h"

#include <format>
#include <unordered_map>
#include <mutex>

namespace db::caches::DBComponentModelFileData {

static uint32_t fieldToUint32(const db::FieldValue& val) {
	if (auto* p = std::get_if<int64_t>(&val))
		return static_cast<uint32_t>(*p);
	if (auto* p = std::get_if<uint64_t>(&val))
		return static_cast<uint32_t>(*p);
	if (auto* p = std::get_if<float>(&val))
		return static_cast<uint32_t>(*p);
	return 0;
}

// JS: const GENDER_ANY = 2;
static constexpr uint32_t GENDER_ANY = 2;

// JS: const file_data_to_info = new Map();
static std::unordered_map<uint32_t, ComponentModelInfo> file_data_to_info;

// JS: let is_initialized = false;
static bool is_initialized = false;
// JS: let init_promise = null;
static bool is_initializing = false;

// JS: const initialize = async () => { ... }
void initialize() {
	if (is_initialized)
		return;

	if (is_initializing)
		return;

	is_initializing = true;

	logging::write("Loading ComponentModelFileData...");

	// JS: for (const [id, row] of await db2.ComponentModelFileData.getAllRows())
	auto allRows = casc::db2::preloadTable("ComponentModelFileData").getAllRows();
	for (const auto& [id, row] : allRows) {
		ComponentModelInfo info;
		info.raceID = fieldToUint32(row.at("RaceID"));
		info.genderIndex = fieldToUint32(row.at("GenderIndex"));
		info.classID = fieldToUint32(row.at("ClassID"));
		info.positionIndex = fieldToUint32(row.at("PositionIndex"));
		file_data_to_info.emplace(id, info);
	}

	logging::write(std::format("Loaded ComponentModelFileData for {} models", file_data_to_info.size()));
	is_initialized = true;
	is_initializing = false;
}

// JS: const getModelForRaceGender = (file_data_ids, race_id, gender_index, fallback_race_id = 0) => { ... }
std::optional<uint32_t> getModelForRaceGender(const std::vector<uint32_t>& file_data_ids, uint32_t race_id, uint32_t gender_index, uint32_t fallback_race_id) {
	if (file_data_ids.empty())
		return std::nullopt;

	// if only one option, return it
	if (file_data_ids.size() == 1)
		return file_data_ids[0];

	// try exact race + gender match
	for (const auto fdid : file_data_ids) {
		auto it = file_data_to_info.find(fdid);
		if (it != file_data_to_info.end() && it->second.raceID == race_id && it->second.genderIndex == gender_index)
			return fdid;
	}

	// try race + any gender
	for (const auto fdid : file_data_ids) {
		auto it = file_data_to_info.find(fdid);
		if (it != file_data_to_info.end() && it->second.raceID == race_id && it->second.genderIndex == GENDER_ANY)
			return fdid;
	}

	// try fallback race if provided
	if (fallback_race_id > 0) {
		for (const auto fdid : file_data_ids) {
			auto it = file_data_to_info.find(fdid);
			if (it != file_data_to_info.end() && it->second.raceID == fallback_race_id && (it->second.genderIndex == gender_index || it->second.genderIndex == GENDER_ANY))
				return fdid;
		}
	}

	// try race=0 (any race)
	for (const auto fdid : file_data_ids) {
		auto it = file_data_to_info.find(fdid);
		if (it != file_data_to_info.end() && it->second.raceID == 0)
			return fdid;
	}

	// fallback to first
	return file_data_ids[0];
}

// JS: const getModelsForRaceGenderByPosition = (file_data_ids, race_id, gender_index) => { ... }
PositionResult getModelsForRaceGenderByPosition(const std::vector<uint32_t>& file_data_ids, uint32_t race_id, uint32_t gender_index) {
	PositionResult result;

	if (file_data_ids.empty())
		return result;

	// group candidates by positionIndex, filtering by race/gender
	struct Candidate {
		uint32_t fdid;
		const ComponentModelInfo* info;
	};

	std::vector<Candidate> by_position_0;
	std::vector<Candidate> by_position_1;

	for (const auto fdid : file_data_ids) {
		auto it = file_data_to_info.find(fdid);
		if (it == file_data_to_info.end() || (it->second.positionIndex != 0 && it->second.positionIndex != 1))
			continue;

		if (it->second.positionIndex == 0)
			by_position_0.push_back({fdid, &it->second});
		else
			by_position_1.push_back({fdid, &it->second});
	}

	// helper to find best match from a list of candidates
	auto find_best = [race_id, gender_index](const std::vector<Candidate>& candidates) -> std::optional<uint32_t> {
		// exact race + gender
		for (const auto& c : candidates) {
			if (c.info->raceID == race_id && c.info->genderIndex == gender_index)
				return c.fdid;
		}

		// race + any gender
		for (const auto& c : candidates) {
			if (c.info->raceID == race_id && c.info->genderIndex == GENDER_ANY)
				return c.fdid;
		}

		// any race
		for (const auto& c : candidates) {
			if (c.info->raceID == 0)
				return c.fdid;
		}

		// fallback to first
		if (!candidates.empty())
			return candidates[0].fdid;
		return std::nullopt;
	};

	result.left = find_best(by_position_0);
	result.right = find_best(by_position_1);

	return result;
}

// JS: const hasEntry = (file_data_id) => file_data_to_info.has(file_data_id);
bool hasEntry(uint32_t file_data_id) {
	return file_data_to_info.contains(file_data_id);
}

// JS: const getInfo = (file_data_id) => file_data_to_info.get(file_data_id) || null;
const ComponentModelInfo* getInfo(uint32_t file_data_id) {
	auto it = file_data_to_info.find(file_data_id);
	if (it != file_data_to_info.end())
		return &it->second;
	return nullptr;
}

} // namespace db::caches::DBComponentModelFileData
