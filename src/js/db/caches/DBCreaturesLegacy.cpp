/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
*/

#include "DBCreaturesLegacy.h"

#include "../../log.h"
#include "../../buffer.h"
#include "../DBCReader.h"

#include <format>
#include <algorithm>
#include <regex>
#include <filesystem>

namespace db::caches::DBCreaturesLegacy {

static uint32_t fieldToUint32(const db::FieldValue& val) {
	if (auto* p = std::get_if<int64_t>(&val))
		return static_cast<uint32_t>(*p);
	if (auto* p = std::get_if<uint64_t>(&val))
		return static_cast<uint32_t>(*p);
	if (auto* p = std::get_if<float>(&val))
		return static_cast<uint32_t>(*p);
	return 0;
}

static std::string fieldToString(const db::FieldValue& val) {
	if (auto* p = std::get_if<std::string>(&val))
		return *p;
	return "";
}

static std::vector<std::string> fieldToStringVec(const db::FieldValue& val) {
	if (auto* p = std::get_if<std::vector<std::string>>(&val))
		return *p;
	return {};
}

// JS: const creatureDisplays = new Map(); // model_path (lowercase) -> array of display info
static std::unordered_map<std::string, std::vector<LegacyCreatureDisplay>> creatureDisplays;
// JS: let isInitialized = false;
static bool isInitialized = false;

// Helper: normalize path (lowercase, forward slashes, .mdl/.mdx to .m2)
static std::string normalizePath(const std::string& path) {
	std::string normalized = path;
	std::transform(normalized.begin(), normalized.end(), normalized.begin(),
		[](unsigned char c) { return std::tolower(c); });
	std::replace(normalized.begin(), normalized.end(), '\\', '/');

	// Convert .mdl/.mdx to .m2
	if (normalized.size() >= 4) {
		std::string ext = normalized.substr(normalized.size() - 4);
		if (ext == ".mdl" || ext == ".mdx")
			normalized = normalized.substr(0, normalized.size() - 4) + ".m2";
	}

	return normalized;
}

// Helper: get directory part of path
static std::string getDirectory(const std::string& path) {
	auto pos = path.find_last_of('/');
	if (pos != std::string::npos)
		return path.substr(0, pos);
	return "";
}

/**
 * Initialize legacy creature display data from DBC files.
 * @param getFile Function to get file data from MPQ.
 * @param build_id Build identifier string.
 */
void initializeCreatureData(std::function<std::vector<uint8_t>(const std::string&)> getFile, const std::string& build_id) {
	if (isInitialized)
		return;

	logging::write("Loading legacy creature textures from DBC...");

	try {
		// load CreatureModelData.dbc
		// JS: const model_data_raw = mpq.getFile('DBFilesClient\\CreatureModelData.dbc');
		auto model_data_raw = getFile("DBFilesClient\\CreatureModelData.dbc");
		if (model_data_raw.empty()) {
			logging::write("CreatureModelData.dbc not found in MPQ");
			return;
		}

		// JS: const model_data_reader = new DBCReader('CreatureModelData.dbc', build_id);
		db::DBCReader model_data_reader("CreatureModelData.dbc", build_id);
		// JS: await model_data_reader.parse(new BufferWrapper(Buffer.from(model_data_raw)));
		BufferWrapper model_buf = BufferWrapper::from(model_data_raw);
		model_data_reader.parse(model_buf);

		// build map of modelID -> model filepath
		// JS: const model_id_to_path = new Map();
		std::unordered_map<uint32_t, std::string> model_id_to_path;

		auto model_rows = model_data_reader.getAllRows();
		for (const auto& [id, row] : model_rows) {
			// CreatureModelData has ModelPath field (string)
			// JS: const model_path = row.ModelName || row.ModelPath || row.field_2;
			std::string model_path;
			auto it = row.find("ModelName");
			if (it != row.end())
				model_path = fieldToString(it->second);
			if (model_path.empty()) {
				it = row.find("ModelPath");
				if (it != row.end())
					model_path = fieldToString(it->second);
			}
			if (model_path.empty()) {
				it = row.find("field_2");
				if (it != row.end())
					model_path = fieldToString(it->second);
			}

			if (!model_path.empty()) {
				// normalize: lowercase, convert .mdl/.mdx to .m2
				std::string normalized = normalizePath(model_path);
				model_id_to_path.emplace(id, std::move(normalized));
			}
		}

		logging::write(std::format("Loaded {} creature models from CreatureModelData.dbc", model_id_to_path.size()));

		// load CreatureDisplayInfo.dbc
		// JS: const display_info_raw = mpq.getFile('DBFilesClient\\CreatureDisplayInfo.dbc');
		auto display_info_raw = getFile("DBFilesClient\\CreatureDisplayInfo.dbc");
		if (display_info_raw.empty()) {
			logging::write("CreatureDisplayInfo.dbc not found in MPQ");
			return;
		}

		// JS: const display_info_reader = new DBCReader('CreatureDisplayInfo.dbc', build_id);
		db::DBCReader display_info_reader("CreatureDisplayInfo.dbc", build_id);
		BufferWrapper display_buf = BufferWrapper::from(display_info_raw);
		display_info_reader.parse(display_buf);

		auto display_rows = display_info_reader.getAllRows();

		for (const auto& [display_id, row] : display_rows) {
			// JS: const model_id = row.ModelID ?? row.field_1;
			uint32_t model_id = 0;
			auto mit = row.find("ModelID");
			if (mit != row.end())
				model_id = fieldToUint32(mit->second);
			if (model_id == 0) {
				mit = row.find("field_1");
				if (mit != row.end())
					model_id = fieldToUint32(mit->second);
			}

			auto pathIt = model_id_to_path.find(model_id);
			if (pathIt == model_id_to_path.end())
				continue;

			const std::string& model_path = pathIt->second;

			// get texture variation strings (3 slots)
			// JS: const tex1 = row.TextureVariation?.[0] ?? row.Skin1 ?? row.field_6 ?? '';
			std::string tex1, tex2, tex3;

			auto tvIt = row.find("TextureVariation");
			if (tvIt != row.end()) {
				auto texVec = fieldToStringVec(tvIt->second);
				if (texVec.size() > 0) tex1 = texVec[0];
				if (texVec.size() > 1) tex2 = texVec[1];
				if (texVec.size() > 2) tex3 = texVec[2];
			}

			// Fallback field names
			if (tex1.empty()) {
				auto it2 = row.find("Skin1");
				if (it2 != row.end()) tex1 = fieldToString(it2->second);
			}
			if (tex1.empty()) {
				auto it2 = row.find("field_6");
				if (it2 != row.end()) tex1 = fieldToString(it2->second);
			}
			if (tex2.empty()) {
				auto it2 = row.find("Skin2");
				if (it2 != row.end()) tex2 = fieldToString(it2->second);
			}
			if (tex2.empty()) {
				auto it2 = row.find("field_7");
				if (it2 != row.end()) tex2 = fieldToString(it2->second);
			}
			if (tex3.empty()) {
				auto it2 = row.find("Skin3");
				if (it2 != row.end()) tex3 = fieldToString(it2->second);
			}
			if (tex3.empty()) {
				auto it2 = row.find("field_8");
				if (it2 != row.end()) tex3 = fieldToString(it2->second);
			}

			// skip if no textures
			if (tex1.empty() && tex2.empty() && tex3.empty())
				continue;

			// JS: const model_dir = path.dirname(model_path).replace(/\\/g, '/');
			std::string model_dir = getDirectory(model_path);

			LegacyCreatureDisplay display;
			display.id = display_id;

			// build full texture paths
			if (!tex1.empty())
				display.textures.push_back(model_dir + "/" + tex1 + ".blp");
			if (!tex2.empty())
				display.textures.push_back(model_dir + "/" + tex2 + ".blp");
			if (!tex3.empty())
				display.textures.push_back(model_dir + "/" + tex3 + ".blp");

			// add to map by model path
			creatureDisplays[model_path].push_back(std::move(display));
		}

		logging::write(std::format("Loaded skin variations for {} creature models", creatureDisplays.size()));
		isInitialized = true;
	} catch (const std::exception& e) {
		logging::write(std::format("Failed to load legacy creature data: {}", e.what()));
	}
}

/**
 * Get all available creature display variations for a model path.
 * @param model_path The M2 model path (can include MPQ prefix).
 * @returns Pointer to vector of displays, or nullptr if not found.
 */
const std::vector<LegacyCreatureDisplay>* getCreatureDisplaysByPath(const std::string& model_path) {
	// normalize path: lowercase, forward slashes, strip MPQ prefix
	std::string normalized = normalizePath(model_path);

	// strip MPQ archive prefix if present (e.g., "data/model.mpq/creature/...")
	// JS: const mpq_match = normalized.match(/\.mpq[\/\\](.+)/i);
	static const std::regex mpq_regex(R"(\.mpq[/\\](.+))", std::regex::icase);
	std::smatch match;
	if (std::regex_search(normalized, match, mpq_regex))
		normalized = match[1].str();

	// convert .mdx to .m2 (already done in normalizePath but be safe)
	if (normalized.size() >= 4) {
		std::string ext = normalized.substr(normalized.size() - 4);
		if (ext == ".mdx")
			normalized = normalized.substr(0, normalized.size() - 4) + ".m2";
	}

	auto it = creatureDisplays.find(normalized);
	if (it != creatureDisplays.end())
		return &it->second;
	return nullptr;
}

/**
 * Reset the cache (for when switching MPQ installs).
 */
void reset() {
	creatureDisplays.clear();
	isInitialized = false;
}

} // namespace db::caches::DBCreaturesLegacy
