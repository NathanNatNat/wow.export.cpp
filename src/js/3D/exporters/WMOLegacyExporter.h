/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
*/
#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "../../buffer.h"

class WMOLegacyLoader;
class MTLWriter;

namespace casc {
	class ExportHelper;
}

namespace mpq {
	class MPQInstall;
}

struct FileManifestEntry;

/**
 * Group mask entry for controlling which WMO groups are exported.
 */
struct WMOGroupMaskEntry {
	bool checked = false;
	uint32_t groupIndex = 0;
};

/**
 * Doodad set mask entry for controlling which doodad sets are exported.
 */
struct WMODoodadSetMaskEntry {
	bool checked = false;
};

/**
 * Texture export result returned from exportTextures.
 */
struct WMOTextureExportInfo {
	std::string matPathRelative;
	std::filesystem::path matPath;
	std::string matName;
};

/**
 * Result of texture export containing both maps.
 */
struct WMOTextureExportResult {
	std::map<uint32_t, WMOTextureExportInfo> textureMap;
	std::map<int, std::string> materialMap;
};

class WMOLegacyExporter {
public:
	WMOLegacyExporter(BufferWrapper data, const std::string& filePath, mpq::MPQInstall* mpq);

	void setGroupMask(const std::vector<WMOGroupMaskEntry>& mask);
	void setDoodadSetMask(const std::vector<WMODoodadSetMaskEntry>& mask);

	WMOTextureExportResult exportTextures(
		const std::filesystem::path& out,
		MTLWriter* mtl,
		casc::ExportHelper* helper
	);

	void exportAsOBJ(
		const std::filesystem::path& out,
		casc::ExportHelper* helper,
		std::vector<FileManifestEntry>* fileManifest
	);

	void exportAsSTL(
		const std::filesystem::path& out,
		casc::ExportHelper* helper,
		std::vector<FileManifestEntry>* fileManifest
	);

	void exportRaw(
		const std::filesystem::path& out,
		casc::ExportHelper* helper,
		std::vector<FileManifestEntry>* fileManifest
	);

	static void clearCache();

private:
	BufferWrapper data;
	std::string filePath;
	mpq::MPQInstall* mpq;
	std::unique_ptr<WMOLegacyLoader> wmo;
	std::optional<std::vector<WMOGroupMaskEntry>> groupMask;
	std::optional<std::vector<WMODoodadSetMaskEntry>> doodadSetMask;

	// extract mpq prefix from filepath (e.g. "wmo.MPQ" from "wmo.MPQ\world\...")
	std::string mpqPrefix;
};
