/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
*/
#include "M2LegacyExporter.h"

#include "../../core.h"
#include "../../log.h"
#include "../../generics.h"
#include "../../buffer.h"
#include "../../casc/export-helper.h"
#include "../../casc/blp.h"
#include "../loaders/M2LegacyLoader.h"
#include "../Texture.h"
#include "../writers/JSONWriter.h"
#include "../writers/OBJWriter.h"
#include "../writers/MTLWriter.h"
#include "../writers/STLWriter.h"
#include "../GeosetMapper.h"
#include "../../mpq/mpq.h"

#include <filesystem>
#include <format>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <algorithm>

namespace fs = std::filesystem;

M2LegacyExporter::M2LegacyExporter(BufferWrapper& data, const std::string& filePath, mpq::MPQArchive* mpq)
	: data(data)
	, filePath(filePath)
	, mpq(mpq)
	, m2(nullptr)
{
}

void M2LegacyExporter::setSkinTextures(std::vector<std::string> textures) {
	skinTextures = std::move(textures);
}

void M2LegacyExporter::setGeosetMask(std::vector<GeosetMaskEntry> mask) {
	geosetMask = std::move(mask);
}

std::unordered_map<std::string, M2LegacyExporter::TextureInfo> M2LegacyExporter::exportTextures(
	const fs::path& outDir,
	MTLWriter* mtl,
	casc::ExportHelper* helper)
{
	const auto& config = core::view->config;
	std::unordered_map<std::string, TextureInfo> validTextures;

	if (!config.value("modelsExportTextures", false))
		return validTextures;

	m2->load();

	const bool useAlpha = config.value("modelsExportAlpha", false);
	const bool usePosix = config.value("pathFormat", std::string("")) == "posix";

	std::unordered_set<std::string> exportedTextures;

	for (size_t i = 0; i < m2->textures.size(); i++) {
		if (helper && helper->isCancelled())
			return validTextures;

		const Texture& texture = m2->textures[i];
		const uint32_t textureType = m2->textureTypes[i];

		std::string texturePath = texture.fileName;

		// check for variant/skin textures
		if (textureType > 0 && skinTextures) {
			if (textureType >= 11 && textureType < 14)
				texturePath = (*skinTextures)[textureType - 11];
			else if (textureType > 1 && textureType < 5)
				texturePath = (*skinTextures)[textureType - 2];
		}

		if (texturePath.empty())
			continue;

		std::string texLower = texturePath;
		std::transform(texLower.begin(), texLower.end(), texLower.begin(), ::tolower);

		if (exportedTextures.count(texLower))
			continue;

		exportedTextures.insert(texLower);

		try {
			auto textureData = mpq->extractFile(texturePath);
			if (!textureData) {
				logging::write(std::format("Texture not found in MPQ: {}", texturePath));
				continue;
			}

			std::string texFile = fs::path(texturePath).filename().string();
			texFile = casc::ExportHelper::replaceExtension(texFile, ".png");

			// legacy mpq exports always use flat textures alongside model for compatibility
			fs::path texPath = outDir / texFile;

			std::string matName = "mat_" + fs::path(texLower).stem().string();
			if (config.value("removePathSpaces", false)) {
				matName.erase(std::remove_if(matName.begin(), matName.end(),
					[](char c) { return std::isspace(static_cast<unsigned char>(c)); }), matName.end());
			}

			const bool fileExisted = generics::fileExists(texPath);

			if (config.value("overwriteFiles", true) || !fileExisted) {
				BufferWrapper buf = BufferWrapper::from(std::span<const uint8_t>(textureData->data(), textureData->size()));
				casc::BLPImage blp(std::move(buf));
				blp.saveToPNG(texPath, useAlpha ? 0b1111 : 0b0111);

				logging::write(std::format("Exported legacy M2 texture: {}", texPath.string()));
			} else {
				logging::write(std::format("Skipping M2 texture export {} (file exists, overwrite disabled)", texPath.string()));
			}

			std::string texFileStr = texFile;
			if (usePosix)
				texFileStr = casc::ExportHelper::win32ToPosix(texFileStr);

			if (mtl)
				mtl->addMaterial(matName, texFileStr);

			validTextures[texLower] = { texFileStr, texPath, matName };
		} catch (const std::exception& e) {
			logging::write(std::format("Failed to export texture {} for M2: {}", texturePath, e.what()));
		}
	}

	return validTextures;
}

void M2LegacyExporter::exportAsOBJ(const fs::path& out, casc::ExportHelper* helper, std::vector<FileManifestEntry>* fileManifest) {
	const auto& config = core::view->config;

	m2 = std::make_unique<M2LegacyLoader>(data);
	m2->load();

	LegacyM2Skin& skin = m2->getSkin(0);

	const fs::path mtlPath = casc::ExportHelper::replaceExtension(out.string(), ".mtl");
	OBJWriter obj(out);
	MTLWriter mtl(mtlPath);

	const fs::path outDir = out.parent_path();
	const std::string modelName = out.stem().string();
	obj.setName(modelName);

	logging::write(std::format("Exporting legacy M2 model {} as OBJ: {}", modelName, out.string()));

	obj.setVertArray(m2->vertices);
	obj.setNormalArray(m2->normals);
	obj.addUVArray(m2->uv);

	if (config.value("modelsExportUV2", false))
		obj.addUVArray(m2->uv2);

	if (helper)
		helper->setCurrentTaskName(modelName + " textures");

	const auto validTextures = exportTextures(outDir, &mtl, helper);

	if (fileManifest) {
		for (const auto& [texPath, texInfo] : validTextures)
			fileManifest->push_back({ "PNG", texInfo.matPath });
	}

	if (helper && helper->isCancelled())
		return;

	// export mesh data
	for (size_t mI = 0, mC = skin.subMeshes.size(); mI < mC; mI++) {
		if (geosetMask && (mI >= geosetMask->size() || !(*geosetMask)[mI].checked))
			continue;

		const LegacyM2SubMesh& mesh = skin.subMeshes[mI];
		std::vector<uint32_t> verts(mesh.triangleCount);

		for (uint16_t vI = 0; vI < mesh.triangleCount; vI++)
			verts[vI] = skin.indices[skin.triangles[mesh.triangleStart + vI]];

		std::string matName;
		const LegacyM2TextureUnit* texUnit = nullptr;
		for (const auto& tu : skin.textureUnits) {
			if (tu.skinSectionIndex == static_cast<uint16_t>(mI)) {
				texUnit = &tu;
				break;
			}
		}

		if (texUnit) {
			const uint16_t texIndex = m2->textureCombos[texUnit->textureComboIndex];
			const Texture& texture = m2->textures[texIndex];
			const uint32_t textureType = m2->textureTypes[texIndex];

			// resolve texture path same as exportTextures
			std::string texturePath = texture.fileName;
			if (textureType > 0 && skinTextures) {
				if (textureType >= 11 && textureType < 14)
					texturePath = (*skinTextures)[textureType - 11];
				else if (textureType > 1 && textureType < 5)
					texturePath = (*skinTextures)[textureType - 2];
			}

			if (!texturePath.empty()) {
				std::string texLower = texturePath;
				std::transform(texLower.begin(), texLower.end(), texLower.begin(), ::tolower);
				auto it = validTextures.find(texLower);
				if (it != validTextures.end())
					matName = it->second.matName;
			}
		}

		obj.addMesh(geoset_mapper::getGeosetName(static_cast<int>(mI), mesh.submeshID), verts, matName);
	}

	if (!mtl.isEmpty())
		obj.setMaterialLibrary(mtlPath.filename().string());

	obj.write(config.value("overwriteFiles", true));
	if (fileManifest)
		fileManifest->push_back({ "OBJ", out });

	mtl.write(config.value("overwriteFiles", true));
	if (fileManifest)
		fileManifest->push_back({ "MTL", mtlPath });

	if (config.value("exportM2Meta", false)) {
		if (helper)
			helper->clearCurrentTask();
		if (helper)
			helper->setCurrentTaskName(modelName + ", writing meta data");

		const fs::path jsonPath = casc::ExportHelper::replaceExtension(out.string(), ".json");
		JSONWriter json(jsonPath);

		// clone submesh array with enabled property
		nlohmann::json subMeshesJson = nlohmann::json::array();
		for (size_t i = 0; i < skin.subMeshes.size(); i++) {
			const bool subMeshEnabled = !geosetMask || (i < geosetMask->size() && (*geosetMask)[i].checked);
			const LegacyM2SubMesh& sm = skin.subMeshes[i];
			nlohmann::json smObj;
			smObj["enabled"]          = subMeshEnabled;
			smObj["submeshID"]        = sm.submeshID;
			smObj["level"]            = sm.level;
			smObj["vertexStart"]      = sm.vertexStart;
			smObj["vertexCount"]      = sm.vertexCount;
			smObj["triangleStart"]    = sm.triangleStart;
			smObj["triangleCount"]    = sm.triangleCount;
			smObj["boneCount"]        = sm.boneCount;
			smObj["boneStart"]        = sm.boneStart;
			smObj["boneInfluences"]   = sm.boneInfluences;
			smObj["centerBoneIndex"]  = sm.centerBoneIndex;
			smObj["centerPosition"]   = sm.centerPosition;
			subMeshesJson.push_back(std::move(smObj));
		}

		// clone textures array with expanded info
		nlohmann::json texturesJson = nlohmann::json::array();
		for (size_t i = 0; i < m2->textures.size(); i++) {
			const Texture& texture = m2->textures[i];
			const uint32_t textureType = m2->textureTypes[i];

			// resolve texture path same as exportTextures
			std::string texturePath = texture.fileName;
			if (textureType > 0 && skinTextures) {
				if (textureType >= 11 && textureType < 14)
					texturePath = (*skinTextures)[textureType - 11];
				else if (textureType > 1 && textureType < 5)
					texturePath = (*skinTextures)[textureType - 2];
			}

			nlohmann::json texEntry;
			texEntry["fileName"] = texturePath;
			texEntry["type"] = textureType;

			if (!texturePath.empty()) {
				std::string texLower = texturePath;
				std::transform(texLower.begin(), texLower.end(), texLower.begin(), ::tolower);
				auto it = validTextures.find(texLower);
				if (it != validTextures.end()) {
					texEntry["fileNameExternal"] = it->second.matPathRelative;
					texEntry["mtlName"] = it->second.matName;
				} else {
					texEntry["fileNameExternal"] = nullptr;
					texEntry["mtlName"] = nullptr;
				}
			} else {
				texEntry["fileNameExternal"] = nullptr;
				texEntry["mtlName"] = nullptr;
			}

			texturesJson.push_back(std::move(texEntry));
		}

		// textureUnits for skin
		nlohmann::json textureUnitsJson = nlohmann::json::array();
		for (const auto& tu : skin.textureUnits) {
			nlohmann::json tuObj;
			tuObj["flags"]                      = tu.flags;
			tuObj["priority"]                   = tu.priority;
			tuObj["shaderID"]                   = tu.shaderID;
			tuObj["skinSectionIndex"]           = tu.skinSectionIndex;
			tuObj["flags2"]                     = tu.flags2;
			tuObj["colorIndex"]                 = tu.colorIndex;
			tuObj["materialIndex"]              = tu.materialIndex;
			tuObj["materialLayer"]              = tu.materialLayer;
			tuObj["textureCount"]               = tu.textureCount;
			tuObj["textureComboIndex"]          = tu.textureComboIndex;
			tuObj["textureCoordComboIndex"]     = tu.textureCoordComboIndex;
			tuObj["textureWeightComboIndex"]    = tu.textureWeightComboIndex;
			tuObj["textureTransformComboIndex"] = tu.textureTransformComboIndex;
			textureUnitsJson.push_back(std::move(tuObj));
		}

		// materials
		nlohmann::json materialsJson = nlohmann::json::array();
		for (const auto& mat : m2->materials) {
			nlohmann::json matObj;
			matObj["flags"] = mat.flags;
			matObj["blendingMode"] = mat.blendingMode;
			materialsJson.push_back(std::move(matObj));
		}

		// bounding box
		nlohmann::json boundingBoxJson;
		boundingBoxJson["min"] = m2->boundingBox.min;
		boundingBoxJson["max"] = m2->boundingBox.max;

		nlohmann::json collisionBoxJson;
		collisionBoxJson["min"] = m2->collisionBox.min;
		collisionBoxJson["max"] = m2->collisionBox.max;

		json.addProperty("fileType", "m2");
		json.addProperty("filePath", filePath);
		json.addProperty("internalName", m2->name);
		json.addProperty("version", m2->version);
		json.addProperty("flags", m2->flags);
		json.addProperty("textures", texturesJson);
		json.addProperty("textureTypes", m2->textureTypes);
		json.addProperty("materials", materialsJson);
		json.addProperty("textureCombos", m2->textureCombos);
		json.addProperty("transparencyLookup", m2->transparencyLookup);
		json.addProperty("textureTransformsLookup", m2->textureTransformsLookup);
		json.addProperty("boundingBox", boundingBoxJson);
		json.addProperty("boundingSphereRadius", m2->boundingSphereRadius);
		json.addProperty("collisionBox", collisionBoxJson);
		json.addProperty("collisionSphereRadius", m2->collisionSphereRadius);
		json.addProperty("skin", nlohmann::json{
			{ "subMeshes", subMeshesJson },
			{ "textureUnits", textureUnitsJson }
		});

		json.write(config.value("overwriteFiles", true));
		if (fileManifest)
			fileManifest->push_back({ "META", jsonPath });
	}
}

void M2LegacyExporter::exportAsSTL(const fs::path& out, casc::ExportHelper* helper, std::vector<FileManifestEntry>* fileManifest) {
	const auto& config = core::view->config;

	m2 = std::make_unique<M2LegacyLoader>(data);
	m2->load();

	LegacyM2Skin& skin = m2->getSkin(0);

	STLWriter stl(out);
	const std::string modelName = out.stem().string();
	stl.setName(modelName);

	logging::write(std::format("Exporting legacy M2 model {} as STL: {}", modelName, out.string()));

	stl.setVertArray(m2->vertices);
	stl.setNormalArray(m2->normals);

	if (helper && helper->isCancelled())
		return;

	for (size_t mI = 0, mC = skin.subMeshes.size(); mI < mC; mI++) {
		if (geosetMask && (mI >= geosetMask->size() || !(*geosetMask)[mI].checked))
			continue;

		const LegacyM2SubMesh& mesh = skin.subMeshes[mI];
		std::vector<uint32_t> verts(mesh.triangleCount);

		for (uint16_t vI = 0; vI < mesh.triangleCount; vI++)
			verts[vI] = skin.indices[skin.triangles[mesh.triangleStart + vI]];

		stl.addMesh(geoset_mapper::getGeosetName(static_cast<int>(mI), mesh.submeshID), verts);
	}

	stl.write(config.value("overwriteFiles", true));
	if (fileManifest)
		fileManifest->push_back({ "STL", out });
}

void M2LegacyExporter::exportRaw(const fs::path& out, casc::ExportHelper* helper, std::vector<FileManifestEntry>* fileManifest) {
	const auto& config = core::view->config;
	const fs::path outDir = out.parent_path();

	const fs::path manifestFile = casc::ExportHelper::replaceExtension(out.string(), ".manifest.json");
	JSONWriter manifest(manifestFile);

	manifest.addProperty("filePath", filePath);

	// write main m2 file
	data.writeToFile(out);
	if (fileManifest)
		fileManifest->push_back({ "M2", out });

	logging::write(std::format("Exported legacy M2: {}", out.string()));

	// export textures if enabled
	if (config.value("modelsExportTextures", false)) {
		m2 = std::make_unique<M2LegacyLoader>(data);
		m2->load();

		nlohmann::json texturesManifest = nlohmann::json::array();
		std::unordered_set<std::string> exportedTextures;

		// export embedded textures (type 0 with fileName)
		for (const auto& texture : m2->textures) {
			if (texture.fileName.empty())
				continue;

			const std::string& texturePath = texture.fileName;

			std::string texLower = texturePath;
			std::transform(texLower.begin(), texLower.end(), texLower.begin(), ::tolower);

			// skip duplicates
			if (exportedTextures.count(texLower))
				continue;

			exportedTextures.insert(texLower);

			try {
				auto textureData = mpq->extractFile(texturePath);
				if (!textureData) {
					logging::write(std::format("Texture not found in MPQ: {}", texturePath));
					continue;
				}

				fs::path texOut;
				if (config.value("enableSharedTextures", false))
					texOut = casc::ExportHelper::getExportPath(texturePath);
				else
					texOut = outDir / fs::path(texturePath).filename();

				BufferWrapper buf = BufferWrapper::from(std::span<const uint8_t>(textureData->data(), textureData->size()));
				buf.writeToFile(texOut);

				const std::string relPath = fs::relative(texOut, outDir).string();
				texturesManifest.push_back({ { "file", relPath }, { "path", texturePath }, { "type", "embedded" } });
				if (fileManifest)
					fileManifest->push_back({ "BLP", texOut });

				logging::write(std::format("Exported legacy M2 texture: {}", texOut.string()));
			} catch (const std::exception& e) {
				logging::write(std::format("Failed to export texture {}: {}", texturePath, e.what()));
			}
		}

		// export skin/variant textures (creature skins, etc)
		if (skinTextures && !skinTextures->empty()) {
			for (const auto& texturePath : *skinTextures) {
				if (texturePath.empty())
					continue;

				std::string texLower = texturePath;
				std::transform(texLower.begin(), texLower.end(), texLower.begin(), ::tolower);

				// skip duplicates
				if (exportedTextures.count(texLower))
					continue;

				exportedTextures.insert(texLower);

				try {
					auto textureData = mpq->extractFile(texturePath);
					if (!textureData) {
						logging::write(std::format("Skin texture not found in MPQ: {}", texturePath));
						continue;
					}

					fs::path texOut;
					if (config.value("enableSharedTextures", false))
						texOut = casc::ExportHelper::getExportPath(texturePath);
					else
						texOut = outDir / fs::path(texturePath).filename();

					BufferWrapper buf = BufferWrapper::from(std::span<const uint8_t>(textureData->data(), textureData->size()));
					buf.writeToFile(texOut);

					const std::string relPath = fs::relative(texOut, outDir).string();
					texturesManifest.push_back({ { "file", relPath }, { "path", texturePath }, { "type", "skin" } });
					if (fileManifest)
						fileManifest->push_back({ "BLP", texOut });

					logging::write(std::format("Exported legacy M2 skin texture: {}", texOut.string()));
				} catch (const std::exception& e) {
					logging::write(std::format("Failed to export skin texture {}: {}", texturePath, e.what()));
				}
			}
		}

		manifest.addProperty("textures", texturesManifest);
	}

	manifest.write();
	if (fileManifest)
		fileManifest->push_back({ "MANIFEST", manifestFile });
}
