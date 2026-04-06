/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <string>
#include <vector>
#include <array>
#include <filesystem>

class STLWriter {
public:
	/**
	 * Construct a new STLWriter instance.
	 * @param out Output path to write to.
	 */
	STLWriter(const std::filesystem::path& out);

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
	 * Add a mesh to this writer.
	 * @param name Mesh name.
	 * @param triangles Index array.
	 */
	void addMesh(const std::string& name, const std::vector<uint32_t>& triangles);

	/**
	 * Append additional geometry from another model.
	 * @param verts Vertex array (x,y,z triplets).
	 * @param normals Normal array (x,y,z triplets).
	 */
	void appendGeometry(const std::vector<float>& verts, const std::vector<float>& normals);

	/**
	 * Calculate normal from three vertices using cross product.
	 */
	std::array<float, 3> calculate_normal(
		float v1x, float v1y, float v1z,
		float v2x, float v2y, float v2z,
		float v3x, float v3y, float v3z) const;

	/**
	 * Write the STL file in binary format.
	 * @param overwrite Whether to overwrite existing files.
	 */
	void write(bool overwrite = true);

private:
	struct Mesh {
		std::string name;
		std::vector<uint32_t> triangles;
		size_t vertexOffset;
	};

	std::filesystem::path out;

	std::vector<float> verts;
	std::vector<float> normals;
	std::vector<Mesh> meshes;
	std::string name;

	// track vertex offsets for appending additional models
	size_t vertex_offset;
};
