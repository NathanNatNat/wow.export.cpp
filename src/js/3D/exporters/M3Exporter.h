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
#include <vector>

#include "../../buffer.h"

class M3Loader;
class MTLWriter;

namespace casc {
	class ExportHelper;
}

/**
 * File manifest entry for tracking exported M3 files.
 */
struct M3ExportFileManifest {
	std::string type;
	uint32_t fileDataID = 0;
	std::filesystem::path file;
};

/**
 * Geoset mask entry for M3 export.
 */
struct M3ExportGeosetMask {
	bool checked = false;
};

class M3Exporter {
public:
	/**
	 * Construct a new M3Exporter instance.
	 * @param data
	 * @param variantTextures
	 * @param fileDataID
	 */
	M3Exporter(BufferWrapper data, std::vector<uint32_t> variantTextures, uint32_t fileDataID);

	/**
	 * Set the mask array used for geoset control.
	 * @param mask
	 */
	void setGeosetMask(std::vector<M3ExportGeosetMask> mask);

	/**
	 * Export additional texture from canvas
	 */
	void addURITexture(const std::string& out, BufferWrapper pngData);

	/**
	 * Export the textures for this M2 model.
	 * @param out
	 * @param raw
	 * @param mtl
	 * @param helper
	 * @param fullTexPaths
	 * @returns Texture map
	 */
	std::map<uint32_t, std::string> exportTextures(const std::filesystem::path& out, bool raw = false,
		MTLWriter* mtl = nullptr, casc::ExportHelper* helper = nullptr, bool fullTexPaths = false);

	void exportAsGLTF(const std::filesystem::path& out, casc::ExportHelper* helper, const std::string& format = "gltf");

	/**
	 * Export the M3 model as a WaveFront OBJ.
	 * @param out
	 * @param exportCollision
	 * @param helper
	 * @param fileManifest
	 */
	void exportAsOBJ(const std::filesystem::path& out, bool exportCollision,
		casc::ExportHelper* helper, std::vector<M3ExportFileManifest>* fileManifest);

	/**
	 * Export the M3 model as an STL file.
	 * @param out
	 * @param exportCollision
	 * @param helper
	 * @param fileManifest
	 */
	void exportAsSTL(const std::filesystem::path& out, bool exportCollision,
		casc::ExportHelper* helper, std::vector<M3ExportFileManifest>* fileManifest);

	/**
	 * Export the model as a raw M3 file, including related files
	 * such as textures, bones, animations, etc.
	 * @param out
	 * @param helper
	 * @param fileManifest
	 */
	void exportRaw(const std::filesystem::path& out, casc::ExportHelper* helper,
		std::vector<M3ExportFileManifest>* fileManifest);

private:
	BufferWrapper data;
	std::unique_ptr<M3Loader> m3;
	uint32_t fileDataID;
	std::vector<uint32_t> variantTextures;
	std::map<std::string, BufferWrapper> dataTextures;
	std::vector<M3ExportGeosetMask> geosetMask;
};
