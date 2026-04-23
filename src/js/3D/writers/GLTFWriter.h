/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>, Marlamin <marlamin@marlamin.com>
	License: MIT
 */
#pragma once

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <filesystem>

#include "../../buffer.h"

struct GLTFAnimTrack {
	std::vector<std::vector<uint32_t>> timestamps;
	std::vector<std::vector<std::vector<float>>> values;
	int interpolation = 0;
};

struct GLTFBone {
	int parentBone = -1;
	std::vector<float> pivot;
	uint32_t boneID = 0;
	uint32_t boneNameCRC = 0;
	GLTFAnimTrack translation;
	GLTFAnimTrack rotation;
	GLTFAnimTrack scale;
};

struct GLTFAnimation {
	uint32_t id = 0;
	uint32_t variationIndex = 0;
	uint32_t duration = 0;
};

struct GLTFMesh {
	std::string name;
	std::vector<uint32_t> triangles;
	std::string matName;
};

struct GLTFTextureEntry {
	std::string matPathRelative;
	std::string matName;
};

/**
 * Equipment model data for glTF export.
 * @param name Equipment name for mesh naming
 * @param vertices Vertex positions
 * @param normals Vertex normals
 * @param uv UV coordinates
 * @param uv2 Secondary UV coordinates (optional)
 * @param boneIndices Bone indices (remapped to char skeleton)
 * @param boneWeights Bone weights
 * @param meshes Array of mesh data
 * @param attachment_bone Bone index for attachment (non-skinned equipment), -1 if not set
 */
struct GLTFEquipmentModel {
	std::string name;
	std::vector<float> vertices;
	std::vector<float> normals;
	std::vector<float> uv;
	std::vector<float> uv2;
	std::vector<uint8_t> boneIndices;
	std::vector<uint8_t> boneWeights;
	std::vector<GLTFMesh> meshes;
	int attachment_bone = -1;
};

class GLTFWriter {
public:
	/**
	 * Construct a new GLTF writer instance.
	 * @param out Output file path
	 * @param name Model name
	 */
	GLTFWriter(const std::filesystem::path& out, const std::string& name);

	/**
	 * Set the texture map used for this writer.
	 * @param textures Texture map (string key: numeric ID as string or "data-N" for data textures)
	 */
	void setTextureMap(const std::map<std::string, GLTFTextureEntry>& textures);

	/**
	 * Set the texture buffers for embedding in GLB.
	 * @param texture_buffers Texture buffer map (same key scheme as texture map)
	 */
	void setTextureBuffers(std::map<std::string, BufferWrapper> texture_buffers);

	/**
	 * Add a single texture buffer for embedding in GLB.
	 * @param key Texture key (numeric ID as string or "data-N" for data textures)
	 * @param buffer PNG data buffer
	 */
	void addTextureBuffer(std::string key, BufferWrapper buffer);

	/**
	 * Set the bones array for this writer.
	 * @param bones Bone data
	 */
	void setBonesArray(const std::vector<GLTFBone>& bones);

	/**
	 * Set the vertices array for this writer.
	 * @param vertices Vertex positions
	 */
	void setVerticesArray(const std::vector<float>& vertices);

	/**
	 * Set the normals array for this writer.
	 * @param normals Normal vectors
	 */
	void setNormalArray(const std::vector<float>& normals);

	/**
	 * Add a UV array for this writer.
	 * @param uvs UV coordinates
	 */
	void addUVArray(const std::vector<float>& uvs);

	/**
	 * Set the bone weights array for this writer.
	 * @param boneWeights Bone weights
	 */
	void setBoneWeightArray(const std::vector<uint8_t>& boneWeights);

	/**
	 * Set the bone indicies array for this writer.
	 * @param boneIndices Bone indices
	 */
	void setBoneIndexArray(const std::vector<uint8_t>& boneIndices);

	/**
	 * Set the animations array for this writer.
	 * @param animations Animation data
	 */
	void setAnimations(const std::vector<GLTFAnimation>& animations);

	/**
	 * Add a mesh to this writer.
	 * @param name Mesh name
	 * @param triangles Triangle indices
	 * @param matName Material name
	 */
	void addMesh(const std::string& name, const std::vector<uint32_t>& triangles, const std::string& matName);

	/**
	 * Add an equipment model to be exported alongside the main model.
	 * Equipment shares the main model's skeleton via bone index remapping.
	 * @param equip Equipment data
	 */
	void addEquipmentModel(const GLTFEquipmentModel& equip);

	void write(bool overwrite = true, const std::string& format = "gltf");

private:
	std::filesystem::path out;
	std::string name;

	std::vector<float> vertices;
	std::vector<float> normals;
	std::vector<std::vector<float>> uvs;
	std::vector<uint8_t> boneWeights;
	std::vector<uint8_t> boneIndices;
	std::vector<GLTFBone> bones;
	std::vector<float> inverseBindMatrices;
	std::vector<GLTFAnimation> animations;

	std::map<std::string, GLTFTextureEntry> textures;
	std::map<std::string, BufferWrapper> texture_buffers;
	std::vector<GLTFMesh> meshes;

	// equipment models to append
	std::vector<GLTFEquipmentModel> equipment_models;
};
