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

class M2Loader;
class MTLWriter;
class GLTFWriter;
class OBJWriter;
class STLWriter;
class M2RendererGL;

namespace casc {
	class ExportHelper;
	class CASC;
}

/**
 * Geoset mask entry for controlling which geosets are exported.
 */
struct M2ExportGeosetMask {
	bool checked = false;
};

/**
 * File manifest entry for tracking exported files (with fileDataID).
 */
struct M2ExportFileManifest {
	std::string type;
	uint32_t fileDataID = 0;
	std::filesystem::path file;
};

/**
 * Texture export info returned from exportTextures.
 */
struct M2TextureExportInfo {
	std::string matName;
	std::string matPathRelative;
	std::filesystem::path matPath;
};

/**
 * Result from exportTextures in GLB mode.
 */
struct M2ExportTextureResult {
	std::map<std::string, M2TextureExportInfo> validTextures;
	std::map<std::string, BufferWrapper> texture_buffers;
	std::vector<std::filesystem::path> files_to_cleanup;
};

/**
 * Equipment model for OBJ/STL export (pre-baked geometry).
 */
struct EquipmentModel {
	int slot_id = 0;
	int item_id = 0;
	M2RendererGL* renderer = nullptr;
	std::vector<float> vertices;
	std::vector<float> normals;
	std::vector<float> uv;
	std::vector<float> uv2;
	std::vector<uint32_t> textures;
};

/**
 * Equipment model for GLTF export (with bone data for rigging).
 */
struct EquipmentModelGLTF {
	int slot_id = 0;
	int item_id = 0;
	M2RendererGL* renderer = nullptr;
	std::vector<float> vertices;
	std::vector<float> normals;
	std::vector<float> uv;
	std::vector<float> uv2;
	std::vector<uint8_t> boneIndices;
	std::vector<uint8_t> boneWeights;
	std::vector<uint32_t> textures;
	bool is_collection_style = false;
};

class M2Exporter {
public:
	/**
	 * Construct a new M2Exporter instance.
	 * @param data
	 * @param variantTextures
	 * @param fileDataID
	 * @param casc CASC source for file loading
	 */
	M2Exporter(BufferWrapper data, std::vector<uint32_t> variantTextures, uint32_t fileDataID, casc::CASC* casc);

	/**
	 * Set the mask array used for geoset control.
	 * @param mask
	 */
	void setGeosetMask(std::vector<M2ExportGeosetMask> mask);

	/**
	 * Set posed geometry to use instead of bind pose
	 * @param vertices
	 * @param normals
	 */
	void setPosedGeometry(std::vector<float> vertices, std::vector<float> normals);

	/**
	 * Export additional texture from canvas
	 * @param textureType Texture type ID used as key
	 * @param pngData PNG image data buffer
	 */
	void addURITexture(uint32_t textureType, BufferWrapper pngData);

	/**
	 * Set equipment models to export alongside the main model (for OBJ/STL).
	 * @param equipment
	 */
	void setEquipmentModels(std::vector<EquipmentModel> equipment);

	/**
	 * Set equipment models for GLTF export (with bone data for rigging).
	 * @param equipment
	 */
	void setEquipmentModelsGLTF(std::vector<EquipmentModelGLTF> equipment);

	/**
	 * Export the textures for this M2 model.
	 * @param out Output directory
	 * @param raw Export as raw BLP
	 * @param mtl MTL writer
	 * @param helper Export helper
	 * @param fullTexPaths Use full texture paths for material names
	 * @param glbMode GLB mode returns buffers instead of writing files
	 * @returns Texture result with maps
	 */
	M2ExportTextureResult exportTextures(const std::filesystem::path& out, bool raw, MTLWriter* mtl,
		casc::ExportHelper* helper, bool fullTexPaths = false, bool glbMode = false);

	void exportAsGLTF(const std::filesystem::path& out, casc::ExportHelper* helper, const std::string& format = "gltf");

	/**
	 * Export the M2 model as a WaveFront OBJ.
	 * @param out
	 * @param exportCollision
	 * @param helper
	 * @param fileManifest
	 */
	void exportAsOBJ(const std::filesystem::path& out, bool exportCollision,
		casc::ExportHelper* helper, std::vector<M2ExportFileManifest>* fileManifest);

	/**
	 * Export the M2 model as an STL file.
	 * @param out
	 * @param exportCollision
	 * @param helper
	 * @param fileManifest
	 */
	void exportAsSTL(const std::filesystem::path& out, bool exportCollision,
		casc::ExportHelper* helper, std::vector<M2ExportFileManifest>* fileManifest);

	/**
	 * Export the model as a raw M2 file, including related files
	 * such as textures, bones, animations, etc.
	 * @param out
	 * @param helper
	 * @param fileManifest
	 */
	void exportRaw(const std::filesystem::path& out, casc::ExportHelper* helper,
		std::vector<M2ExportFileManifest>* fileManifest);

private:
	/**
	 * Add equipment model to GLTF writer.
	 */
	void _addEquipmentToGLTF(GLTFWriter& gltf, const EquipmentModelGLTF& equip,
		std::map<std::string, M2TextureExportInfo>& textureMap,
		const std::filesystem::path& outDir, const std::string& format, casc::ExportHelper* helper);

	/**
	 * Export equipment model geometry and textures to OBJ/MTL.
	 */
	void _exportEquipmentToOBJ(OBJWriter& obj, MTLWriter& mtl, const std::filesystem::path& outDir,
		const EquipmentModel& equip, std::map<std::string, M2TextureExportInfo>& validTextures,
		casc::ExportHelper* helper, std::vector<M2ExportFileManifest>* fileManifest);

	/**
	 * Export equipment model geometry to STL.
	 */
	void _exportEquipmentToSTL(STLWriter& stl, const EquipmentModel& equip, casc::ExportHelper* helper);

	BufferWrapper data;
	std::unique_ptr<M2Loader> m2;
	uint32_t fileDataID;
	casc::CASC* casc;
	std::vector<uint32_t> variantTextures;
	std::map<uint32_t, BufferWrapper> dataTextures;
	std::vector<M2ExportGeosetMask> geosetMask;
	std::vector<float> posedVertices;
	std::vector<float> posedNormals;
	std::vector<EquipmentModel> equipmentModels;
	std::vector<EquipmentModelGLTF> equipmentModelsGLTF;
};
