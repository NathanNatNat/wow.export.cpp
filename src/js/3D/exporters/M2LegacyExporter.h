/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
*/
#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

class BufferWrapper;
class M2LegacyLoader;

namespace mpq { class MPQArchive; }
namespace casc { class ExportHelper; }

/**
 * File manifest entry for tracking exported files.
 */
struct FileManifestEntry {
	std::string type;
	std::filesystem::path file;
};

/**
 * Geoset mask entry (controls which meshes are exported).
 */
struct GeosetMaskEntry {
	bool checked = false;
};

/**
 * Exports legacy M2 models (MPQ era) to various output formats.
 *
 * JS equivalent: class M2LegacyExporter — module.exports = M2LegacyExporter
 */
class M2LegacyExporter {
public:
	/**
	 * Construct a new M2LegacyExporter.
	 * @param data     Raw M2 file data.
	 * @param filePath Internal file path for meta output.
	 * @param mpq      MPQ archive to load textures from.
	 */
	M2LegacyExporter(BufferWrapper& data, const std::string& filePath, mpq::MPQArchive* mpq);

	/**
	 * Set the skin/variant textures (creature skins, etc).
	 * @param textures Array of texture paths.
	 */
	void setSkinTextures(std::vector<std::string> textures);

	/**
	 * Set the geoset mask to control which sub-meshes are exported.
	 * @param mask Array of geoset mask entries.
	 */
	void setGeosetMask(std::vector<GeosetMaskEntry> mask);

	/**
	 * Export all textures referenced by the M2 model.
	 * Returns a map of (lowercase texture path → tex info) for valid exports.
	 * @param outDir   Output directory.
	 * @param mtl      Optional MTL writer to register materials into.
	 * @param helper   Optional export helper for cancellation checks.
	 */
	struct TextureInfo {
		std::string matPathRelative;
		std::filesystem::path matPath;
		std::string matName;
	};
	std::unordered_map<std::string, TextureInfo> exportTextures(
		const std::filesystem::path& outDir,
		class MTLWriter* mtl,
		casc::ExportHelper* helper);

	/**
	 * Export the model as a Wavefront OBJ file.
	 * @param out          Output file path.
	 * @param helper       Export helper for progress and cancellation.
	 * @param fileManifest Optional manifest to append exported files to.
	 */
	void exportAsOBJ(const std::filesystem::path& out, casc::ExportHelper* helper, std::vector<FileManifestEntry>* fileManifest);

	/**
	 * Export the model as an STL file.
	 * @param out          Output file path.
	 * @param helper       Export helper for cancellation checks.
	 * @param fileManifest Optional manifest to append exported files to.
	 */
	void exportAsSTL(const std::filesystem::path& out, casc::ExportHelper* helper, std::vector<FileManifestEntry>* fileManifest);

	/**
	 * Export the raw M2 binary along with its referenced textures.
	 * @param out          Output file path.
	 * @param helper       Export helper for cancellation checks.
	 * @param fileManifest Optional manifest to append exported files to.
	 */
	void exportRaw(const std::filesystem::path& out, casc::ExportHelper* helper, std::vector<FileManifestEntry>* fileManifest);

private:
	BufferWrapper& data;
	std::string filePath;
	mpq::MPQArchive* mpq;
	std::unique_ptr<M2LegacyLoader> m2;
	std::optional<std::vector<std::string>> skinTextures;
	std::optional<std::vector<GeosetMaskEntry>> geosetMask;
};
