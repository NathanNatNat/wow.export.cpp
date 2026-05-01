/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <string>
#include <vector>
#include <filesystem>

class OBJWriter {
public:
	/**
	 * Construct a new OBJWriter instance.
	 * @param out Output path to write to.
	 */
	OBJWriter(const std::filesystem::path& out);

	/**
	 * Set the name of the material library.
	 * @param name Material library file name.
	 */
	void setMaterialLibrary(const std::string& name);

	/**
	 * Set the name of this model.
	 * @param name Model name.
	 */
	void setName(const std::string& name);

	/**
	 * Set the vertex array for this writer.
	 * @param verts Vertex array (x,y,z triplets).
	 */
	void setVertArray(const std::vector<float>& verts);

	/**
	 * Set the normals array for this writer.
	 * @param normals Normal array (x,y,z triplets).
	 */
	void setNormalArray(const std::vector<float>& normals);

	/**
	 * Add a UV array for this writer.
	 * @param uv UV array (u,v pairs).
	 */
	void addUVArray(const std::vector<float>& uv);

	/**
	 * Set the vertex color array (RGBA floats, 4 per vertex).
	 * @param colors Color array (r,g,b,a quads).
	 */
	void setColorArray(const std::vector<float>& colors);

	/**
	 * Add a mesh to this writer.
	 * @param name Mesh name.
	 * @param triangles Index array.
	 * @param matName Material name.
	 */
	void addMesh(const std::string& name, const std::vector<uint32_t>& triangles, const std::string& matName = "");

	/**
	 * Append additional geometry from another model.
	 * Call this after setting base model data and adding its meshes.
	 * @param verts Vertex array (x,y,z triplets).
	 * @param normals Normal array (x,y,z triplets).
	 * @param uvArrays Array of UV arrays.
	 */
	void appendGeometry(const std::vector<float>& verts, const std::vector<float>& normals, const std::vector<std::vector<float>>& uvArrays);

	/**
	 * Write the OBJ file (and associated MTLs).
	 * @param overwrite Whether to overwrite existing files.
	 */
	void write(bool overwrite = true);

private:
	struct Mesh {
		std::string name;
		std::vector<uint32_t> triangles;
		std::string matName;
		size_t vertexOffset;
	};

	std::filesystem::path out;

	std::vector<float> verts;
	std::vector<float> normals;
	std::vector<std::vector<float>> uvs;
	std::vector<float> colors;

	std::vector<Mesh> meshes;
	std::string name;
	std::string mtl;

	// track vertex offsets for appending additional models
	size_t vertex_offset;

public:
	bool flip_uvs = false;
};
