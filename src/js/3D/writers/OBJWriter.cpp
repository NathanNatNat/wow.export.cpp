/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "OBJWriter.h"
#include "../../constants.h"
#include "../../generics.h"
#include "../../file-writer.h"

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <charconv>
#include <array>

/**
 * Convert float to string with minimal representation (no trailing zeros),
 * matching JS Number.toString() behavior.
 */
static std::string float_to_string(float val) {
	// Use snprintf with enough precision to round-trip
	std::array<char, 32> buf;
	int len = std::snprintf(buf.data(), buf.size(), "%.17g", static_cast<double>(val));
	if (len <= 0 || static_cast<size_t>(len) >= buf.size())
		return std::to_string(val);
	return std::string(buf.data(), len);
}

OBJWriter::OBJWriter(const std::filesystem::path& out)
	: out(out), name("Mesh"), vertex_offset(0) {}

void OBJWriter::setMaterialLibrary(const std::string& name) {
	mtl = name;
}

void OBJWriter::setName(const std::string& name) {
	this->name = name;
}

void OBJWriter::setVertArray(const std::vector<float>& verts) {
	this->verts = verts;
}

void OBJWriter::setNormalArray(const std::vector<float>& normals) {
	this->normals = normals;
}

void OBJWriter::addUVArray(const std::vector<float>& uv) {
	uvs.push_back(uv);
}

void OBJWriter::addMesh(const std::string& name, const std::vector<uint32_t>& triangles, const std::string& matName) {
	meshes.push_back({ name, triangles, matName, vertex_offset });
}

void OBJWriter::appendGeometry(const std::vector<float>& verts, const std::vector<float>& normals, const std::vector<std::vector<float>>& uvArrays) {
	// calculate current vertex count before appending
	const size_t current_vertex_count = this->verts.size() / 3;
	vertex_offset = current_vertex_count;

	// append vertices
	if (!verts.empty())
		this->verts.insert(this->verts.end(), verts.begin(), verts.end());

	// append normals
	if (!normals.empty())
		this->normals.insert(this->normals.end(), normals.begin(), normals.end());

	// append uvs (match layer count)
	for (size_t i = 0; i < uvArrays.size(); i++) {
		if (i >= uvs.size())
			uvs.emplace_back();

		const auto& uv = uvArrays[i];
		if (!uv.empty())
			uvs[i].insert(uvs[i].end(), uv.begin(), uv.end());
	}
}

void OBJWriter::write(bool overwrite) {
	// If overwriting is disabled, check file existence.
	if (!overwrite && generics::fileExists(out))
		return;

	generics::createDirectory(out.parent_path());
	FileWriter writer(out);

	// Write header.
	writer.writeLine("# Exported using wow.export.cpp v" + std::string(constants::VERSION));
	writer.writeLine("o " + name);

	// Link material library.
	if (!mtl.empty())
		writer.writeLine("mtllib " + mtl);

	// collect used indices (accounting for vertex offsets from appended geometry)
	std::unordered_set<size_t> usedIndices;
	for (const auto& mesh : meshes) {
		const size_t offset = mesh.vertexOffset;
		for (uint32_t index : mesh.triangles)
			usedIndices.insert(static_cast<size_t>(index) + offset);
	}

	std::unordered_map<size_t, size_t> vertMap;
	std::unordered_map<size_t, size_t> normalMap;
	std::unordered_map<size_t, size_t> uvMap;

	// Write verts.
	for (size_t i = 0, j = 0, u = 0, n = verts.size(); i < n; j++, i += 3) {
		if (usedIndices.count(j)) {
			vertMap[j] = u++;
			writer.writeLine("v " + float_to_string(verts[i]) + " " + float_to_string(verts[i + 1]) + " " + float_to_string(verts[i + 2]));
		}
	}

	// Write normals.
	for (size_t i = 0, j = 0, u = 0, n = normals.size(); i < n; j++, i += 3) {
		if (usedIndices.count(j)) {
			normalMap[j] = u++;
			writer.writeLine("vn " + float_to_string(normals[i]) + " " + float_to_string(normals[i + 1]) + " " + float_to_string(normals[i + 2]));
		}
	}

	// Write UVs
	const size_t layerCount = uvs.size();
	const bool hasUV = layerCount > 0;
	if (hasUV) {
		for (size_t uvIndex = 0; uvIndex < layerCount; uvIndex++) {
			const auto& uv = uvs[uvIndex];

			std::string prefix = "vt";

			// Use non-standard properties (vt2, vt3, etc) for additional UV layers.
			if (uvIndex > 0)
				prefix += std::to_string(uvIndex + 1);

			for (size_t i = 0, j = 0, u = 0, n = uv.size(); i < n; j++, i += 2) {
				if (usedIndices.count(j)) {
					// Build the index reference using just the first layer
					// since it will be identical for all other layers.
					if (uvIndex == 0)
						uvMap[j] = u++;

					writer.writeLine(prefix + " " + float_to_string(uv[i]) + " " + float_to_string(uv[i + 1]));
				}
			}
		}
	}

	// Write meshes.
	for (const auto& mesh : meshes) {
		writer.writeLine("g " + mesh.name);
		writer.writeLine("s 1");

		if (!mesh.matName.empty())
			writer.writeLine("usemtl " + mesh.matName);

		const auto& triangles = mesh.triangles;
		const size_t offset = mesh.vertexOffset;

		for (size_t i = 0, n = triangles.size(); i < n; i += 3) {
			const size_t idxA = static_cast<size_t>(triangles[i]) + offset;
			const size_t idxB = static_cast<size_t>(triangles[i + 1]) + offset;
			const size_t idxC = static_cast<size_t>(triangles[i + 2]) + offset;

			const std::string pointA = std::to_string(vertMap[idxA] + 1) + "/" + (hasUV ? std::to_string(uvMap[idxA] + 1) : "") + "/" + std::to_string(normalMap[idxA] + 1);
			const std::string pointB = std::to_string(vertMap[idxB] + 1) + "/" + (hasUV ? std::to_string(uvMap[idxB] + 1) : "") + "/" + std::to_string(normalMap[idxB] + 1);
			const std::string pointC = std::to_string(vertMap[idxC] + 1) + "/" + (hasUV ? std::to_string(uvMap[idxC] + 1) : "") + "/" + std::to_string(normalMap[idxC] + 1);

			writer.writeLine("f " + pointA + " " + pointB + " " + pointC);
		}
	}

	writer.close();
}