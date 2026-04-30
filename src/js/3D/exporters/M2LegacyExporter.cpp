/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
*/

#include "M2LegacyExporter.h"

#include <algorithm>
#include <cctype>
#include <format>
#include <set>
#include <span>
#include <stdexcept>
#include <string>

#include "../../core.h"
#include "../../log.h"
#include "../../generics.h"
#include "../../buffer.h"
#include "../../casc/blp.h"
#include "../../casc/export-helper.h"
#include "../../mpq/mpq-install.h"
#include "../loaders/M2LegacyLoader.h"
#include "../Texture.h"
#include "../writers/JSONWriter.h"
#include "../writers/OBJWriter.h"
#include "../writers/MTLWriter.h"
#include "../writers/STLWriter.h"
#include "../GeosetMapper.h"

namespace {

std::string toLower(const std::string& s) {
	std::string result = s;
	std::transform(result.begin(), result.end(), result.begin(),
		[](unsigned char c) { return std::tolower(c); });
	return result;
}

/**
 * Resolve a texture path, substituting skin/variant textures where appropriate.
 */
std::string resolveTexturePath(const std::string& fileName, uint32_t textureType, const std::vector<std::string>& skinTextures) {
	std::string texturePath = fileName;

	// check for variant/skin textures
	if (textureType > 0 && !skinTextures.empty()) {
		if (textureType >= 11 && textureType < 14) {
			size_t idx = textureType - 11;
			if (idx < skinTextures.size())
				texturePath = skinTextures[idx];
		} else if (textureType > 1 && textureType < 5) {
			size_t idx = textureType - 2;
			if (idx < skinTextures.size())
				texturePath = skinTextures[idx];
		}
	}

	return texturePath;
}

} // anonymous namespace

M2LegacyExporter::M2LegacyExporter(BufferWrapper data, const std::string& filePath, mpq::MPQInstall* mpq)
	: data(std::move(data))
	, filePath(filePath)
	, mpq(mpq)
{
}

void M2LegacyExporter::setSkinTextures(const std::vector<std::string>& textures) {
	skinTextures = textures;
}

void M2LegacyExporter::setGeosetMask(const std::vector<GeosetMaskEntry>& mask) {
	geosetMask = mask;
}

std::map<std::string, TextureExportInfo> M2LegacyExporter::exportTextures(
	const std::filesystem::path& outDir,
	MTLWriter* mtl,
	casc::ExportHelper* helper)
{
	const auto& config = core::view->config;

	std::map<std::string, TextureExportInfo> validTextures;

	if (!config.value("modelsExportTextures", false))
		return validTextures;

	m2->load().get();

	const bool useAlpha = config.value("modelsExportAlpha", false);
	const bool usePosix = config.value("pathFormat", std::string("")) == "posix";

	std::set<std::string> exportedTextures;

	for (size_t i = 0; i < m2->textures.size(); i++) {
		if (helper && helper->isCancelled())
			return validTextures;

		const auto& texture = m2->textures[i];
		const uint32_t textureType = m2->textureTypes[i];

		std::string texturePath = resolveTexturePath(texture.fileName, textureType, skinTextures);

		if (texturePath.empty())
			continue;

		std::string lowerTexPath = toLower(texturePath);
		if (exportedTextures.count(lowerTexPath))
			continue;

		exportedTextures.insert(lowerTexPath);

		try {
			auto textureData = mpq->getFile(texturePath);
			if (!textureData) {
				logging::write(std::format("Texture not found in MPQ: {}", texturePath));
				continue;
			}

			std::string texFile = std::filesystem::path(texturePath).filename().string();
			texFile = casc::ExportHelper::replaceExtension(texFile, ".png");

			// legacy mpq exports always use flat textures alongside model for compatibility
			std::filesystem::path texPath = outDir / texFile;

			std::string baseName = std::filesystem::path(lowerTexPath).filename().string();
			if (baseName.size() > 4 && baseName.substr(baseName.size() - 4) == ".blp")
				baseName = baseName.substr(0, baseName.size() - 4);
			std::string matName = "mat_" + baseName;
			if (config.value("removePathSpaces", false)) {
				std::erase_if(matName, [](char c) { return std::isspace(static_cast<unsigned char>(c)); });
			}

			const bool fileExisted = generics::fileExists(texPath);

			if (config.value("overwriteFiles", true) || !fileExisted) {
				auto buf = BufferWrapper::from(std::span<const uint8_t>(textureData.value()));
				casc::BLPImage blp(buf);
				blp.saveToPNG(texPath, useAlpha ? 0b1111 : 0b0111);

				logging::write(std::format("Exported legacy M2 texture: {}", texPath.string()));
			} else {
				logging::write(std::format("Skipping M2 texture export {} (file exists, overwrite disabled)", texPath.string()));
			}

			if (usePosix)
				texFile = casc::ExportHelper::win32ToPosix(texFile);

			if (mtl)
				mtl->addMaterial(matName, texFile);

			validTextures[lowerTexPath] = { texFile, texPath, matName };
		} catch (const std::exception& e) {
			logging::write(std::format("Failed to export texture {} for M2: {}", texturePath, e.what()));
		}
	}

	return validTextures;
}

void M2LegacyExporter::exportAsOBJ(
	const std::filesystem::path& out,
	casc::ExportHelper* helper,
	std::vector<FileManifestEntry>* fileManifest)
{
	const auto& config = core::view->config;

	data.seek(0);
	m2 = std::make_unique<M2LegacyLoader>(data);
	m2->load().get();

	auto* skin_ptr = m2->getSkin(0).get();
	if (!skin_ptr)
		throw std::runtime_error("Failed to load legacy skin 0");
	auto& skin = *skin_ptr;

	OBJWriter obj(out);
	auto mtlPath = casc::ExportHelper::replaceExtension(out.string(), ".mtl");
	MTLWriter mtl(mtlPath);

	auto outDir = out.parent_path();
	auto modelName = out.stem().string();
	obj.setName(modelName);

	logging::write(std::format("Exporting legacy M2 model {} as OBJ: {}", modelName, out.string()));

	obj.setVertArray(m2->vertices);
	obj.setNormalArray(m2->normals);
	obj.addUVArray(m2->uv);

	if (config.value("modelsExportUV2", false))
		obj.addUVArray(m2->uv2);

	if (helper)
		helper->setCurrentTaskName(modelName + " textures");

	auto validTextures = exportTextures(outDir, &mtl, helper);

	for (const auto& [texPath, texInfo] : validTextures) {
		if (fileManifest)
			fileManifest->push_back({ "PNG", texInfo.matPath });
	}

	if (helper && helper->isCancelled())
		return;

	// export mesh data
	for (size_t mI = 0, mC = skin.subMeshes.size(); mI < mC; mI++) {
		if (!geosetMask.empty() && (mI >= geosetMask.size() || !geosetMask[mI].checked))
			continue;

		const auto& mesh = skin.subMeshes[mI];
		std::vector<uint32_t> verts(mesh.triangleCount);

		for (uint16_t vI = 0; vI < mesh.triangleCount; vI++)
			verts[vI] = skin.indices[skin.triangles[mesh.triangleStart + vI]];

		std::string matName;
		auto texUnitIt = std::find_if(skin.textureUnits.begin(), skin.textureUnits.end(),
			[mI](const LegacyM2TextureUnit& tex) { return tex.skinSectionIndex == static_cast<uint16_t>(mI); });

		if (texUnitIt != skin.textureUnits.end()) {
			const uint16_t texIndex = m2->textureCombos[texUnitIt->textureComboIndex];
			const auto& texture = m2->textures[texIndex];
			const uint32_t textureType = m2->textureTypes[texIndex];

			// resolve texture path same as exportTextures
			std::string texturePath = resolveTexturePath(texture.fileName, textureType, skinTextures);

			std::string lowerTexPath = toLower(texturePath);
			auto texInfoIt = validTextures.find(lowerTexPath);
			if (!texturePath.empty() && texInfoIt != validTextures.end())
				matName = texInfoIt->second.matName;
		}

		obj.addMesh(geoset_mapper::getGeosetName(static_cast<int>(mI), mesh.submeshID), verts, matName);
	}

	if (!mtl.isEmpty())
		obj.setMaterialLibrary(std::filesystem::path(mtlPath).filename().string());

	obj.write(config.value("overwriteFiles", true));
	if (fileManifest)
		fileManifest->push_back({ "OBJ", out });

	mtl.write(config.value("overwriteFiles", true));
	if (fileManifest)
		fileManifest->push_back({ "MTL", std::filesystem::path(mtlPath) });

	if (config.value("exportM2Meta", false)) {
		if (helper)
			helper->clearCurrentTask();
		if (helper)
			helper->setCurrentTaskName(modelName + ", writing meta data");

		auto jsonPath = casc::ExportHelper::replaceExtension(out.string(), ".json");
		JSONWriter json(jsonPath);

		// clone submesh array with enabled property
		nlohmann::json subMeshes = nlohmann::json::array();
		for (size_t i = 0, n = skin.subMeshes.size(); i < n; i++) {
			bool subMeshEnabled = geosetMask.empty() || (i < geosetMask.size() && geosetMask[i].checked);
			const auto& sm = skin.subMeshes[i];

			nlohmann::json smObj;
			smObj["enabled"] = subMeshEnabled;
			smObj["submeshID"] = sm.submeshID;
			smObj["level"] = sm.level;
			smObj["vertexStart"] = sm.vertexStart;
			smObj["vertexCount"] = sm.vertexCount;
			smObj["triangleStart"] = sm.triangleStart;
			smObj["triangleCount"] = sm.triangleCount;
			smObj["boneCount"] = sm.boneCount;
			smObj["boneStart"] = sm.boneStart;
			smObj["boneInfluences"] = sm.boneInfluences;
			smObj["centerBoneIndex"] = sm.centerBoneIndex;
			smObj["centerPosition"] = sm.centerPosition;
			smObj["sortCenterPosition"] = sm.sortCenterPosition;
			smObj["sortRadius"] = sm.sortRadius;
			subMeshes.push_back(smObj);
		}

		// clone textures array with expanded info
		nlohmann::json textures = nlohmann::json::array();
		for (size_t i = 0, n = m2->textures.size(); i < n; i++) {
			const auto& texture = m2->textures[i];
			const uint32_t textureType = m2->textureTypes[i];

			// resolve texture path same as exportTextures
			std::string texturePath = resolveTexturePath(texture.fileName, textureType, skinTextures);

			nlohmann::json texObj;
			texObj["fileName"] = texturePath;
			texObj["type"] = textureType;

			std::string lowerTexPath = toLower(texturePath);
			auto texInfoIt = validTextures.find(lowerTexPath);
			if (texInfoIt != validTextures.end()) {
				texObj["fileNameExternal"] = texInfoIt->second.matPathRelative;
				texObj["mtlName"] = texInfoIt->second.matName;
			}
			textures.push_back(texObj);
		}

		// materials
		nlohmann::json materialsJson = nlohmann::json::array();
		for (const auto& mat : m2->materials) {
			nlohmann::json matObj;
			matObj["flags"] = mat.flags;
			matObj["blendingMode"] = mat.blendingMode;
			materialsJson.push_back(matObj);
		}

		// bounding boxes
		nlohmann::json bbObj;
		bbObj["min"] = m2->boundingBox.min;
		bbObj["max"] = m2->boundingBox.max;

		nlohmann::json cbObj;
		cbObj["min"] = m2->collisionBox.min;
		cbObj["max"] = m2->collisionBox.max;

		// texture units
		nlohmann::json texUnitsJson = nlohmann::json::array();
		for (const auto& tu : skin.textureUnits) {
			nlohmann::json tuObj;
			tuObj["flags"] = tu.flags;
			tuObj["priority"] = tu.priority;
			tuObj["shaderID"] = tu.shaderID;
			tuObj["skinSectionIndex"] = tu.skinSectionIndex;
			tuObj["flags2"] = tu.flags2;
			tuObj["colorIndex"] = tu.colorIndex;
			tuObj["materialIndex"] = tu.materialIndex;
			tuObj["materialLayer"] = tu.materialLayer;
			tuObj["textureCount"] = tu.textureCount;
			tuObj["textureComboIndex"] = tu.textureComboIndex;
			tuObj["textureCoordComboIndex"] = tu.textureCoordComboIndex;
			tuObj["textureWeightComboIndex"] = tu.textureWeightComboIndex;
			tuObj["textureTransformComboIndex"] = tu.textureTransformComboIndex;
			texUnitsJson.push_back(tuObj);
		}

		json.addProperty("fileType", "m2");
		json.addProperty("filePath", filePath);
		json.addProperty("internalName", m2->name);
		json.addProperty("version", m2->version);
		json.addProperty("flags", m2->flags);
		json.addProperty("textures", textures);
		json.addProperty("textureTypes", m2->textureTypes);
		json.addProperty("materials", materialsJson);
		json.addProperty("textureCombos", m2->textureCombos);
		json.addProperty("transparencyLookup", m2->transparencyLookup);
		json.addProperty("textureTransformsLookup", m2->textureTransformsLookup);
		json.addProperty("boundingBox", bbObj);
		json.addProperty("boundingSphereRadius", m2->boundingSphereRadius);
		json.addProperty("collisionBox", cbObj);
		json.addProperty("collisionSphereRadius", m2->collisionSphereRadius);
		json.addProperty("skin", nlohmann::json{
			{ "subMeshes", subMeshes },
			{ "textureUnits", texUnitsJson }
		});

		json.write(config.value("overwriteFiles", true));
		if (fileManifest)
			fileManifest->push_back({ "META", std::filesystem::path(jsonPath) });
	}
}

void M2LegacyExporter::exportAsSTL(
	const std::filesystem::path& out,
	casc::ExportHelper* helper,
	std::vector<FileManifestEntry>* fileManifest)
{
	const auto& config = core::view->config;

	data.seek(0);
	m2 = std::make_unique<M2LegacyLoader>(data);
	m2->load().get();

	auto* skin_ptr = m2->getSkin(0).get();
	if (!skin_ptr)
		throw std::runtime_error("Failed to load legacy skin 0");
	auto& skin = *skin_ptr;

	STLWriter stl(out);
	auto modelName = out.stem().string();
	stl.setName(modelName);

	logging::write(std::format("Exporting legacy M2 model {} as STL: {}", modelName, out.string()));

	stl.setVertArray(m2->vertices);
	stl.setNormalArray(m2->normals);

	if (helper && helper->isCancelled())
		return;

	for (size_t mI = 0, mC = skin.subMeshes.size(); mI < mC; mI++) {
		if (!geosetMask.empty() && (mI >= geosetMask.size() || !geosetMask[mI].checked))
			continue;

		const auto& mesh = skin.subMeshes[mI];
		std::vector<uint32_t> verts(mesh.triangleCount);

		for (uint16_t vI = 0; vI < mesh.triangleCount; vI++)
			verts[vI] = skin.indices[skin.triangles[mesh.triangleStart + vI]];

		stl.addMesh(geoset_mapper::getGeosetName(static_cast<int>(mI), mesh.submeshID), verts);
	}

	stl.write(config.value("overwriteFiles", true));
	if (fileManifest)
		fileManifest->push_back({ "STL", out });
}

void M2LegacyExporter::exportRaw(
	const std::filesystem::path& out,
	casc::ExportHelper* helper,
	std::vector<FileManifestEntry>* fileManifest)
{
	const auto& config = core::view->config;
	auto outDir = out.parent_path();

	auto manifestPath = casc::ExportHelper::replaceExtension(out.string(), ".manifest.json");
	JSONWriter manifest(manifestPath);

	manifest.addProperty("filePath", filePath);

	// write main m2 file
	data.writeToFile(out);
	if (fileManifest)
		fileManifest->push_back({ "M2", out });

	logging::write(std::format("Exported legacy M2: {}", out.string()));

	// export textures if enabled
	if (config.value("modelsExportTextures", false)) {
		data.seek(0);
		m2 = std::make_unique<M2LegacyLoader>(data);
		m2->load().get();

		nlohmann::json texturesManifest = nlohmann::json::array();
		std::set<std::string> exportedTextures;

		// export embedded textures (type 0 with fileName)
		for (const auto& texture : m2->textures) {
			if (texture.fileName.empty())
				continue;

			const std::string& texturePath = texture.fileName;

			// skip duplicates
			std::string lowerTexPath = toLower(texturePath);
			if (exportedTextures.count(lowerTexPath))
				continue;

			exportedTextures.insert(lowerTexPath);

			try {
				auto textureData = mpq->getFile(texturePath);
				if (!textureData) {
					logging::write(std::format("Texture not found in MPQ: {}", texturePath));
					continue;
				}

				std::filesystem::path texOut;
				if (config.value("enableSharedTextures", false))
					texOut = casc::ExportHelper::getExportPath(texturePath);
				else
					texOut = outDir / std::filesystem::path(texturePath).filename();

				auto buf = BufferWrapper::from(std::span<const uint8_t>(textureData.value()));
				buf.writeToFile(texOut);

				texturesManifest.push_back({
					{ "file", std::filesystem::relative(texOut, outDir).string() },
					{ "path", texturePath },
					{ "type", "embedded" }
				});
				if (fileManifest)
					fileManifest->push_back({ "BLP", texOut });

				logging::write(std::format("Exported legacy M2 texture: {}", texOut.string()));
			} catch (const std::exception& e) {
				logging::write(std::format("Failed to export texture {}: {}", texturePath, e.what()));
			}
		}

		// export skin/variant textures (creature skins, etc)
		if (!skinTextures.empty()) {
			for (const auto& texturePath : skinTextures) {
				if (texturePath.empty())
					continue;

				// skip duplicates
				std::string lowerTexPath = toLower(texturePath);
				if (exportedTextures.count(lowerTexPath))
					continue;

				exportedTextures.insert(lowerTexPath);

				try {
					auto textureData = mpq->getFile(texturePath);
					if (!textureData) {
						logging::write(std::format("Skin texture not found in MPQ: {}", texturePath));
						continue;
					}

					std::filesystem::path texOut;
					if (config.value("enableSharedTextures", false))
						texOut = casc::ExportHelper::getExportPath(texturePath);
					else
						texOut = outDir / std::filesystem::path(texturePath).filename();

					auto buf = BufferWrapper::from(std::span<const uint8_t>(textureData.value()));
					buf.writeToFile(texOut);

					texturesManifest.push_back({
						{ "file", std::filesystem::relative(texOut, outDir).string() },
						{ "path", texturePath },
						{ "type", "skin" }
					});
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
		fileManifest->push_back({ "MANIFEST", std::filesystem::path(manifestPath) });
}
