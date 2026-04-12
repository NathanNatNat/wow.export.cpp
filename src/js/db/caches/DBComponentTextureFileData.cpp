/*!
wow.export (https://github.com/Kruithne/wow.export)
Authors: Kruithne <kruithne@gmail.com>, Marlamin <marlamin@marlamin.com>
License: MIT
*/

#include "DBComponentTextureFileData.h"

#include "../../log.h"
#include "../../casc/db2.h"
#include "../WDCReader.h"

#include <format>
#include <unordered_map>

namespace db::caches::DBComponentTextureFileData {

static uint32_t fieldToUint32(const db::FieldValue& val) {
if (auto* p = std::get_if<int64_t>(&val))
return static_cast<uint32_t>(*p);
if (auto* p = std::get_if<uint64_t>(&val))
return static_cast<uint32_t>(*p);
if (auto* p = std::get_if<float>(&val))
return static_cast<uint32_t>(*p);
return 0;
}

static constexpr uint32_t GENDER_ANY = 2;

static std::unordered_map<uint32_t, ComponentTextureInfo> file_data_to_info;

static bool is_initialized = false;
static bool is_initializing = false;

void initialize() {
if (is_initialized)
return;

if (is_initializing)
return;

is_initializing = true;

logging::write("Loading ComponentTextureFileData...");

auto allRows = casc::db2::preloadTable("ComponentTextureFileData").getAllRows();
for (const auto& [id, row] : allRows) {
ComponentTextureInfo info;
info.raceID = fieldToUint32(row.at("RaceID"));
info.genderIndex = fieldToUint32(row.at("GenderIndex"));
info.classID = fieldToUint32(row.at("ClassID"));
file_data_to_info.emplace(id, info);
}

logging::write(std::format("Loaded ComponentTextureFileData for {} textures", file_data_to_info.size()));
is_initialized = true;
is_initializing = false;
}

std::optional<uint32_t> getTextureForRaceGender(const std::vector<uint32_t>& file_data_ids, uint32_t race_id, uint32_t gender_index, uint32_t fallback_race_id) {
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

// try race=0 (any race) with specific gender
for (const auto fdid : file_data_ids) {
auto it = file_data_to_info.find(fdid);
if (it != file_data_to_info.end() && it->second.raceID == 0 && it->second.genderIndex == gender_index)
return fdid;
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

bool hasEntry(uint32_t file_data_id) {
return file_data_to_info.contains(file_data_id);
}

const ComponentTextureInfo* getInfo(uint32_t file_data_id) {
auto it = file_data_to_info.find(file_data_id);
if (it != file_data_to_info.end())
return &it->second;
return nullptr;
}

} // namespace db::caches::DBComponentTextureFileData
