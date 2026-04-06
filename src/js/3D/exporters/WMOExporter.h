/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>, Marlamin <marlamin@marlamin.com>
	License: MIT
 */
#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "../../buffer.h"

class WMOLoader;
class MTLWriter;

namespace casc {
	class ExportHelper;
	class CASC;
}

/**
 * Group mask entry for controlling which WMO groups are exported.
 */
struct WMOExportGroupMask {
	bool checked = false;
	uint32_t groupIndex = 0;
};

/**
 * Doodad set mask entry for controlling which doodad sets are exported.
 */
struct WMOExportDoodadSetMask {
	bool checked = false;
};

/**
 * File manifest entry for tracking exported WMO files.
 */
struct WMOExportFileManifest {
	std::string type;
	uint32_t fileDataID = 0;
	std::filesystem::path file;
};

/**
 * Texture export info per material texture.
 */
struct WMOExportTextureInfo {
	std::string matPathRelative;
	std::filesystem::path matPath;
	std::string matName;
};

/**
 * Result of texture export containing all maps.
 */
struct WMOExportTextureResult {
	std::map<uint32_t, WMOExportTextureInfo> textureMap;
	std::map<int, std::string> materialMap;
	std::map<uint32_t, BufferWrapper> texture_buffers;
	std::vector<std::filesystem::path> files_to_cleanup;
};

class WMOExporter {
public:
	/**
	 * Construct a new WMOExporter instance.
	 * @param data
	 * @param fileDataID
	 * @param casc CASC source for file loading
	 */
	WMOExporter(BufferWrapper data, uint32_t fileDataID, casc::CASC* casc);

	/**
	 * Set the mask used for group control.
	 * @param mask
	 */
	void setGroupMask(std::vector<WMOExportGroupMask> mask);

	/**
	 * Set the mask used for doodad set control.
	 * @param mask
	 */
	void setDoodadSetMask(std::vector<WMOExportDoodadSetMask> mask);

	/**
	 * Export textures for this WMO.
	 * @param out Output path
	 * @param mtl MTL writer (optional)
	 * @param helper Export helper
	 * @param raw Export as raw BLP
	 * @param glbMode GLB mode returns buffers instead of writing files
	 * @returns Texture result with maps
	 */
	WMOExportTextureResult exportTextures(const std::filesystem::path& out, MTLWriter* mtl,
		casc::ExportHelper* helper, bool raw = false, bool glbMode = false);

	/**
	 * Export the WMO model as a GLTF file.
	 * @param out
	 * @param helper
	 * @param format
	 */
	void exportAsGLTF(const std::filesystem::path& out, casc::ExportHelper* helper, const std::string& format = "gltf");

	/**
	 * Export the WMO model as a WaveFront OBJ.
	 * @param out
	 * @param helper
	 * @param fileManifest
	 * @param split_groups
	 */
	void exportAsOBJ(const std::filesystem::path& out, casc::ExportHelper* helper,
		std::vector<WMOExportFileManifest>* fileManifest, bool split_groups = false);

	/**
	 * Export the WMO model as an STL file.
	 * @param out
	 * @param helper
	 * @param fileManifest
	 */
	void exportAsSTL(const std::filesystem::path& out, casc::ExportHelper* helper,
		std::vector<WMOExportFileManifest>* fileManifest);

	/**
	 * Export each WMO group as separate OBJ file.
	 * @param out
	 * @param helper
	 * @param fileManifest
	 */
	void exportGroupsAsSeparateOBJ(const std::filesystem::path& out, casc::ExportHelper* helper,
		std::vector<WMOExportFileManifest>* fileManifest);

	/**
	 * Export the WMO as raw files.
	 * @param out
	 * @param helper
	 * @param fileManifest
	 */
	void exportRaw(const std::filesystem::path& out, casc::ExportHelper* helper,
		std::vector<WMOExportFileManifest>* fileManifest);

	/**
	 * Clear the WMO exporting cache.
	 */
	static void clearCache();

	/**
	 * Load the WMO data. Must be called before export methods.
	 * Exposes WMO loading for callers that need access to doodad set names.
	 */
	void loadWMO();

	/**
	 * Get the doodad set names from the loaded WMO.
	 * Must call loadWMO() first.
	 * @returns Vector of doodad set name strings.
	 */
	std::vector<std::string> getDoodadSetNames() const;

private:
	BufferWrapper data;
	std::unique_ptr<WMOLoader> wmo;
	uint32_t fileDataID;
	casc::CASC* casc;
	std::vector<WMOExportGroupMask> groupMask;
	std::vector<WMOExportDoodadSetMask> doodadSetMask;
};
