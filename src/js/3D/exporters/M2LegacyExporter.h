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
#include <string>
#include <vector>

#include "../../buffer.h"

class M2LegacyLoader;
class MTLWriter;

namespace casc {
	class ExportHelper;
}

namespace mpq {
	class MPQArchive;
}

/**
 * File manifest entry for tracking exported files.
 */
struct FileManifestEntry {
	std::string type;
	std::filesystem::path file;
};

/**
 * Geoset mask entry for controlling which geosets are exported.
 */
struct GeosetMaskEntry {
	bool checked = false;
};

/**
 * Texture export info returned from exportTextures.
 */
struct TextureExportInfo {
	std::string matPathRelative;
	std::filesystem::path matPath;
	std::string matName;
};

class M2LegacyExporter {
public:
	M2LegacyExporter(BufferWrapper data, const std::string& filePath, mpq::MPQArchive* mpq);

	void setSkinTextures(const std::vector<std::string>& textures);
	void setGeosetMask(const std::vector<GeosetMaskEntry>& mask);

	std::map<std::string, TextureExportInfo> exportTextures(
		const std::filesystem::path& outDir,
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

private:
	BufferWrapper data;
	std::string filePath;
	mpq::MPQArchive* mpq;
	std::unique_ptr<M2LegacyLoader> m2;
	std::vector<std::string> skinTextures;
	std::vector<GeosetMaskEntry> geosetMask;
};
