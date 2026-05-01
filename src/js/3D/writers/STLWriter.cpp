/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "STLWriter.h"
#include "../../constants.h"
#include "../../generics.h"
#include "../../buffer.h"

#include <cmath>
#include <cstring>

STLWriter::STLWriter(const std::filesystem::path& out)
	: out(out), name("Mesh"), vertex_offset(0) {}

void STLWriter::setName(const std::string& name) {
	this->name = name;
}

void STLWriter::setVertArray(const std::vector<float>& verts) {
	this->verts = verts;
}

void STLWriter::setNormalArray(const std::vector<float>& normals) {
	this->normals = normals;
}

void STLWriter::addMesh(const std::string& name, const std::vector<uint32_t>& triangles) {
	meshes.push_back({ name, triangles, vertex_offset });
}

void STLWriter::appendGeometry(const std::vector<float>& verts, const std::vector<float>& normals) {
	// calculate current vertex count before appending
	const size_t current_vertex_count = this->verts.size() / 3;
	vertex_offset = current_vertex_count;

	// append vertices
	if (!verts.empty())
		this->verts.insert(this->verts.end(), verts.begin(), verts.end());

	// append normals
	if (!normals.empty())
		this->normals.insert(this->normals.end(), normals.begin(), normals.end());
}

std::array<float, 3> STLWriter::calculate_normal(
	float v1x, float v1y, float v1z,
	float v2x, float v2y, float v2z,
	float v3x, float v3y, float v3z) const
{
	// edge vectors
	const float ux = v2x - v1x;
	const float uy = v2y - v1y;
	const float uz = v2z - v1z;

	const float vx = v3x - v1x;
	const float vy = v3y - v1y;
	const float vz = v3z - v1z;

	// cross product
	float nx = uy * vz - uz * vy;
	float ny = uz * vx - ux * vz;
	float nz = ux * vy - uy * vx;

	// normalize
	const float len = std::sqrt(nx * nx + ny * ny + nz * nz);
	if (len > 0) {
		nx /= len;
		ny /= len;
		nz /= len;
	}

	return { nx, ny, nz };
}

void STLWriter::write(bool overwrite) {
	if (!overwrite && generics::fileExists(out))
		return;

	generics::createDirectory(out.parent_path());

	// count total triangles
	uint32_t triangle_count = 0;
	for (const auto& mesh : meshes)
		triangle_count += static_cast<uint32_t>(mesh.triangles.size() / 3);

	// binary stl: 80 byte header + 4 byte count + 50 bytes per triangle
	const size_t buffer_size = 80 + 4 + (static_cast<size_t>(triangle_count) * 50);
	BufferWrapper buffer = BufferWrapper::alloc(buffer_size, true);

	// write header (80 bytes)
	const std::string header = "Exported using wow.export.cpp v" + std::string(constants::VERSION);
	const size_t header_len = std::min(header.size(), static_cast<size_t>(80));
	buffer.writeBuffer(std::span<const uint8_t>(
		reinterpret_cast<const uint8_t*>(header.data()), header_len));

	// pad remaining header with zeros
	for (size_t i = header_len; i < 80; i++)
		buffer.writeUInt8(0);

	// write triangle count
	buffer.writeUInt32LE(triangle_count);

	const bool has_normals = !normals.empty();

	// write each triangle
	// coordinate transform: wow uses Y-up, stl expects Z-up
	// swap Y and Z components for both vertices and normals
	for (const auto& mesh : meshes) {
		const auto& triangles = mesh.triangles;
		const size_t offset = mesh.vertexOffset;

		for (size_t i = 0, n = triangles.size(); i < n; i += 3) {
			const size_t i0 = static_cast<size_t>(triangles[i]) + offset;
			const size_t i1 = static_cast<size_t>(triangles[i + 1]) + offset;
			const size_t i2 = static_cast<size_t>(triangles[i + 2]) + offset;

			// vertex positions (swap y/z for coordinate system conversion)
			const size_t v0_idx = i0 * 3;
			const size_t v1_idx = i1 * 3;
			const size_t v2_idx = i2 * 3;

			const float v0x = verts[v0_idx];
			const float v0y = verts[v0_idx + 2];
			const float v0z = verts[v0_idx + 1];

			const float v1x = verts[v1_idx];
			const float v1y = verts[v1_idx + 2];
			const float v1z = verts[v1_idx + 1];

			const float v2x = verts[v2_idx];
			const float v2y = verts[v2_idx + 2];
			const float v2z = verts[v2_idx + 1];

			// calculate face normal (or use vertex normals averaged)
			float nx, ny, nz;
			if (has_normals) {
				// average vertex normals for face normal (swap y/z)
				const float n0x = normals[v0_idx];
				const float n0y = normals[v0_idx + 2];
				const float n0z = normals[v0_idx + 1];

				const float n1x = normals[v1_idx];
				const float n1y = normals[v1_idx + 2];
				const float n1z = normals[v1_idx + 1];

				const float n2x = normals[v2_idx];
				const float n2y = normals[v2_idx + 2];
				const float n2z = normals[v2_idx + 1];

				nx = (n0x + n1x + n2x) / 3.0f;
				ny = (n0y + n1y + n2y) / 3.0f;
				nz = (n0z + n1z + n2z) / 3.0f;

				// normalize
				const float len = std::sqrt(nx * nx + ny * ny + nz * nz);
				if (len > 0) {
					nx /= len;
					ny /= len;
					nz /= len;
				}
			} else {
				// calculate from vertices (already transformed)
				auto [cnx, cny, cnz] = calculate_normal(v0x, v0y, v0z, v1x, v1y, v1z, v2x, v2y, v2z);
				nx = cnx;
				ny = cny;
				nz = cnz;
			}

			// write normal (3 floats = 12 bytes)
			buffer.writeFloatLE(nx);
			buffer.writeFloatLE(ny);
			buffer.writeFloatLE(nz);

			// write vertex 1 (3 floats = 12 bytes)
			buffer.writeFloatLE(v0x);
			buffer.writeFloatLE(v0y);
			buffer.writeFloatLE(v0z);

			// write vertex 2 (3 floats = 12 bytes)
			buffer.writeFloatLE(v1x);
			buffer.writeFloatLE(v1y);
			buffer.writeFloatLE(v1z);

			// write vertex 3 (3 floats = 12 bytes)
			buffer.writeFloatLE(v2x);
			buffer.writeFloatLE(v2y);
			buffer.writeFloatLE(v2z);

			// write attribute byte count (2 bytes, always 0)
			buffer.writeUInt16LE(0);
		}
	}

	buffer.writeToFile(out);
}
