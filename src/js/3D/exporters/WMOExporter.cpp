/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>, Marlamin <marlamin@marlamin.com>
	License: MIT
 */
#include "WMOExporter.h"

#include <algorithm>
#include <format>
#include <set>
#include <unordered_set>

#include "../../core.h"
#include "../../log.h"
#include "../../generics.h"
#include "../../constants.h"
#include "../../casc/listfile.h"
#include "../../casc/blp.h"
#include "../../casc/casc-source.h"
#include "../../casc/export-helper.h"
#include "../loaders/WMOLoader.h"
#include "../loaders/M2Loader.h"
#include "../loaders/M3Loader.h"
#include "../Skin.h"
#include "../Texture.h"
#include "../writers/OBJWriter.h"
#include "../writers/MTLWriter.h"
#include "../writers/STLWriter.h"
#include "../writers/CSVWriter.h"
#include "../writers/GLTFWriter.h"
#include "../writers/JSONWriter.h"
#include "../WMOShaderMapper.h"
#include "M2Exporter.h"
#include "M3Exporter.h"

namespace {
	std::unordered_set<uint32_t> doodadCache;

	// -----------------------------------------------------------------------
	// nlohmann::json serialization helpers for WMO structs (meta export)
	// -----------------------------------------------------------------------
	nlohmann::json fogEntryToJson(const WMOFogEntry& e) {
		return { {"end", e.end}, {"startScalar", e.startScalar}, {"color", e.color} };
	}

	nlohmann::json fogToJson(const WMOFog& f) {
		nlohmann::json j;
		j["flags"] = f.flags;
		j["position"] = f.position;
		j["radiusSmall"] = f.radiusSmall;
		j["radiusLarge"] = f.radiusLarge;
		j["fog"] = fogEntryToJson(f.fog);
		j["underwaterFog"] = fogEntryToJson(f.underwaterFog);
		return j;
	}

	nlohmann::json materialToJson(const WMOMaterial& m) {
		nlohmann::json j;
		j["flags"] = m.flags;
		j["shader"] = m.shader;
		j["blendMode"] = m.blendMode;
		j["texture1"] = m.texture1;
		j["color1"] = m.color1;
		j["color1b"] = m.color1b;
		j["texture2"] = m.texture2;
		j["color2"] = m.color2;
		j["groupType"] = m.groupType;
		j["texture3"] = m.texture3;
		j["color3"] = m.color3;
		j["flags3"] = m.flags3;
		j["runtimeData"] = m.runtimeData;
		return j;
	}

	nlohmann::json portalInfoToJson(const WMOPortalInfo& p) {
		return { {"startVertex", p.startVertex}, {"count", p.count}, {"plane", p.plane} };
	}

	nlohmann::json portalRefToJson(const WMOPortalRef& p) {
		return { {"portalIndex", p.portalIndex}, {"groupIndex", p.groupIndex}, {"side", p.side} };
	}

	nlohmann::json groupInfoToJson(const WMOGroupInfo& g) {
		nlohmann::json j;
		j["flags"] = g.flags;
		j["boundingBox1"] = g.boundingBox1;
		j["boundingBox2"] = g.boundingBox2;
		j["nameIndex"] = g.nameIndex;
		return j;
	}

	nlohmann::json doodadSetToJson(const WMODoodadSet& d) {
		nlohmann::json j;
		j["name"] = d.name;
		j["firstInstanceIndex"] = d.firstInstanceIndex;
		j["doodadCount"] = d.doodadCount;
		j["unused"] = d.unused;
		return j;
	}

	nlohmann::json doodadToJson(const WMODoodad& d) {
		nlohmann::json j;
		j["offset"] = d.offset;
		j["flags"] = d.flags;
		j["position"] = d.position;
		j["rotation"] = d.rotation;
		j["scale"] = d.scale;
		j["color"] = d.color;
		return j;
	}

	nlohmann::json liquidVertexToJson(const WMOLiquidVertex& v) {
		return { {"data", v.data}, {"height", v.height} };
	}

	nlohmann::json liquidToJson(const WMOLiquid& l) {
		nlohmann::json j;
		j["vertX"] = l.vertX;
		j["vertY"] = l.vertY;
		j["tileX"] = l.tileX;
		j["tileY"] = l.tileY;
		nlohmann::json verts = nlohmann::json::array();
		for (const auto& v : l.vertices)
			verts.push_back(liquidVertexToJson(v));
		j["vertices"] = verts;
		j["tiles"] = l.tiles;
		j["corner"] = l.corner;
		j["materialID"] = l.materialID;
		return j;
	}

	nlohmann::json renderBatchToJson(const WMORenderBatch& b) {
		nlohmann::json j;
		j["possibleBox1"] = b.possibleBox1;
		j["possibleBox2"] = b.possibleBox2;
		j["firstFace"] = b.firstFace;
		j["numFaces"] = b.numFaces;
		j["firstVertex"] = b.firstVertex;
		j["lastVertex"] = b.lastVertex;
		j["flags"] = b.flags;
		j["materialID"] = b.materialID;
		return j;
	}

	nlohmann::json materialInfoToJson(const WMOMaterialInfo& m) {
		return { {"flags", m.flags}, {"materialID", m.materialID} };
	}

	// Build group mask set from user-facing mask
	std::set<uint32_t> buildGroupMaskSet(const std::vector<WMOExportGroupMask>& groupMask) {
		std::set<uint32_t> mask;
		for (const auto& group : groupMask) {
			if (group.checked)
				mask.insert(group.groupIndex);
		}
		return mask;
	}

	std::string removeSpaces(const std::string& str) {
		std::string result;
		result.reserve(str.size());
		for (char c : str) {
			if (!std::isspace(static_cast<unsigned char>(c)))
				result += c;
		}
		return result;
	}
} // anonymous namespace

/**
 * Construct a new WMOExporter instance.
 * @param data
 * @param fileDataID
 * @param casc CASC source for file loading
 */
WMOExporter::WMOExporter(BufferWrapper data, uint32_t fileDataID, casc::CASC* casc)
	: data(std::move(data)), fileDataID(fileDataID), casc(casc)
{
	wmo = std::make_unique<WMOLoader>(this->data, fileDataID);
}

/**
 * Set the mask used for group control.
 * @param mask 
 */
void WMOExporter::setGroupMask(std::vector<WMOExportGroupMask> mask) {
	groupMask = std::move(mask);
}

/**
 * Set the mask used for doodad set control.
 */
void WMOExporter::setDoodadSetMask(std::vector<WMOExportDoodadSetMask> mask) {
	doodadSetMask = std::move(mask);
}

/**
 * Export textures for this WMO.
 * @param out
 * @param mtl
 * @param helper
 * @param raw
 * @param glbMode
 * @returns {{ textureMap, materialMap, texture_buffers, files_to_cleanup }}
 */
WMOExportTextureResult WMOExporter::exportTextures(const std::filesystem::path& out, MTLWriter* mtl,
	casc::ExportHelper* helper, bool raw, bool glbMode)
{
	const auto& config = core::view->config;

	WMOExportTextureResult result;

	if (!config.value("modelsExportTextures", false))
		return result;

	// Ensure the WMO is loaded before reading materials.
	wmo->load();

	const bool useAlpha = config.value("modelsExportAlpha", false);
	const bool usePosix = config.value("pathFormat", std::string("")) == "posix";
	const bool isClassic = !wmo->textureNames.empty();
	const size_t materialCount = wmo->materials.size();

	helper->setCurrentTaskMax(static_cast<int>(materialCount));

	for (size_t i = 0; i < materialCount; i++) {
		if (helper->isCancelled())
			return result;

		const auto& material = wmo->materials[i];
		helper->setCurrentTaskValue(static_cast<int>(i));

		std::vector<uint32_t> materialTextures = { material.texture1, material.texture2, material.texture3 };

		// Variable that purely exists to not handle the first texture as the main one for shader23
		bool dontUseFirstTexture = false;

		if (material.shader == 23) {
			materialTextures.push_back(material.flags3);
			materialTextures.push_back(material.color3);
			if (material.runtimeData.size() > 0) materialTextures.push_back(material.runtimeData[0]);
			if (material.runtimeData.size() > 1) materialTextures.push_back(material.runtimeData[1]);
			if (material.runtimeData.size() > 2) materialTextures.push_back(material.runtimeData[2]);
			if (material.runtimeData.size() > 3) materialTextures.push_back(material.runtimeData[3]);

			dontUseFirstTexture = true;
		}

		for (const auto materialTexture : materialTextures) {
			// Skip unused material slots.
			if (materialTexture == 0)
				continue;

			uint32_t texFileDataID = 0;
			std::string fileName;

			if (isClassic) {
				// Classic, lookup fileDataID using file name.
				auto it = wmo->textureNames.find(materialTexture);
				if (it != wmo->textureNames.end())
					fileName = it->second;
				auto fid = casc::listfile::getByFilename(fileName);
				texFileDataID = fid.value_or(0);

				// Remove all whitespace from exported textures due to MTL incompatibility.
				if (config.value("removePathSpaces", false))
					fileName = removeSpaces(fileName);
			} else {
				// Retail, use fileDataID directly.
				texFileDataID = materialTexture;
			}

			// Skip unknown/missing files.
			if (texFileDataID == 0)
				continue;

			try {
				std::string texFile = std::to_string(texFileDataID) + (raw ? ".blp" : ".png");
				std::filesystem::path texPath = out.parent_path() / texFile;

				// Default MTL name to the file ID (prefixed for Maya).
				std::string matName = "mat_" + std::to_string(texFileDataID);

				// Attempt to get the file name if we don't already have it.
				if (fileName.empty())
					fileName = casc::listfile::getByID(texFileDataID);

				// If we have a valid file name, use it for the material name.
				if (!fileName.empty()) {
					std::filesystem::path fnPath(fileName);
					std::string baseName = fnPath.stem().string();
					// Convert to lowercase
					std::transform(baseName.begin(), baseName.end(), baseName.begin(),
						[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
					matName = "mat_" + baseName;

					// Remove spaces from material name for MTL compatibility.
					if (config.value("removePathSpaces", false))
						matName = removeSpaces(matName);
				}

				// Map texture files relative to shared directory.
				if (config.value("enableSharedTextures", false)) {
					if (!fileName.empty()) {
						// Replace BLP extension with PNG.
						if (!raw)
							fileName = casc::ExportHelper::replaceExtension(fileName, ".png");
					} else {
						// Handle unknown files.
						fileName = casc::listfile::formatUnknownFile(texFileDataID, raw ? ".blp" : ".png");
					}

					texPath = casc::ExportHelper::getExportPath(fileName);
					texFile = std::filesystem::relative(texPath, out.parent_path()).string();
				}

				const bool file_existed = generics::fileExists(texPath);

				if (glbMode && !raw) {
					// glb mode: convert to PNG buffer without writing
					auto fileData = casc->getVirtualFileByID(texFileDataID);
					casc::BLPImage blp(fileData);
					auto png_buffer = blp.toPNG(useAlpha ? 0b1111 : 0b0111);
					result.texture_buffers[texFileDataID] = std::move(png_buffer);
					logging::write(std::format("Buffering WMO texture {} for GLB embedding", texFileDataID));

					if (!file_existed)
						result.files_to_cleanup.push_back(texPath);
				} else if (config.value("overwriteFiles", true) || !file_existed) {
					auto fileData = casc->getVirtualFileByID(texFileDataID);

					logging::write(std::format("Exporting WMO texture {} -> {}", texFileDataID, texPath.string()));
					if (raw) {
						fileData.writeToFile(texPath);
					} else {
						casc::BLPImage blp(fileData);
						blp.saveToPNG(texPath, useAlpha ? 0b1111 : 0b0111);
					}
				} else {
					logging::write(std::format("Skipping WMO texture export {} (file exists, overwrite disabled)", texPath.string()));
				}

				if (usePosix)
					texFile = casc::ExportHelper::win32ToPosix(texFile);

				if (mtl)
					mtl->addMaterial(matName, texFile);
				result.textureMap[texFileDataID] = { texFile, texPath, matName };

				// MTL only supports one texture per material, only link the first unless we only want the second one (e.g. for shader 23).
				if (result.materialMap.find(static_cast<int>(i)) == result.materialMap.end() && !dontUseFirstTexture)
					result.materialMap[static_cast<int>(i)] = matName;

				// Unset skip here so we always pick the next texture in line
				dontUseFirstTexture = false;
			} catch (const std::exception& e) {
				logging::write(std::format("Failed to export texture {} for WMO: {}", texFileDataID, e.what()));
			}
		}
	}

	return result;
}

/**
 * Export the WMO model as a GLTF file.
 * @param out 
 * @param helper 
 */
void WMOExporter::exportAsGLTF(const std::filesystem::path& out, casc::ExportHelper* helper, const std::string& format) {
	const std::string ext = (format == "glb") ? ".glb" : ".gltf";
	const std::string outFile = casc::ExportHelper::replaceExtension(out.string(), ext);

	// Skip export if file exists and overwriting is disabled.
	if (!core::view->config.value("overwriteFiles", true) && generics::fileExists(outFile)) {
		logging::write(std::format("Skipping {} export of {} (already exists, overwrite disabled)", format, outFile));
		return;
	}

	const std::string wmo_name = std::filesystem::path(outFile).stem().string();
	GLTFWriter gltf(out.string(), wmo_name);

	logging::write(std::format("Exporting WMO model {} as {}: {}", wmo_name, format, outFile));

	wmo->load();

	helper->setCurrentTaskName(wmo_name + " textures");
	auto texMaps = exportTextures(out, nullptr, helper, false, format == "glb");

	if (helper->isCancelled())
		return;

	// Build GLTF texture lookup
	std::map<uint32_t, GLTFTextureEntry> gltfTextureMap;
	for (const auto& [fid, texInfo] : texMaps.textureMap) {
		gltfTextureMap[fid] = { texInfo.matPathRelative, texInfo.matName };
	}

	gltf.setTextureMap(gltfTextureMap);
	if (format == "glb")
		gltf.setTextureBuffers(texMaps.texture_buffers);

	std::vector<WMOLoader*> groups;
	size_t nInd = 0;

	std::set<uint32_t> mask;
	bool hasMask = false;

	// Map our user-facing group mask to a WMO mask.
	if (!groupMask.empty()) {
		hasMask = true;
		mask = buildGroupMaskSet(groupMask);
	}

	// Iterate over the groups once to calculate the total size of our
	// vertex/normal/uv arrays allowing for pre-allocation.
	for (uint32_t i = 0; i < wmo->groupCount; i++) {
		auto& group = wmo->getGroup(i);

		// Skip empty groups.
		if (group.renderBatches.empty())
			continue;

		// Skip masked groups.
		if (hasMask && mask.find(i) == mask.end())
			continue;

		// 3 vertices per indices.
		nInd += group.vertices.size() / 3;

		// Store the valid groups for quicker iteration later.
		groups.push_back(&group);
	}

	std::vector<float> vertices(nInd * 3);
	std::vector<float> normals(nInd * 3);

	std::vector<std::vector<float>> uv_maps;

	// Iterate over groups again and fill the allocated arrays.
	size_t indOfs = 0;
	for (const auto* group : groups) {
		const size_t indCount = group->vertices.size() / 3;

		const size_t vertOfs = indOfs * 3;
		for (size_t vi = 0; vi < group->vertices.size(); vi++)
			vertices[vertOfs + vi] = group->vertices[vi];

		// Normal and vertices should match, so reuse vertOfs here.
		for (size_t ni = 0; ni < group->normals.size(); ni++)
			normals[vertOfs + ni] = group->normals[ni];

		const size_t uv_ofs = indOfs * 2;

		if (!group->uvs.empty()) {
			for (size_t ui = 0; ui < group->uvs.size(); ui++) {
				if (ui >= uv_maps.size())
					uv_maps.resize(ui + 1, std::vector<float>(nInd * 2, 0.0f));

				const auto& uv = group->uvs[ui];
				for (size_t uvi = 0; uvi < uv.size(); uvi++)
					uv_maps[ui][uv_ofs + uvi] = uv[uvi];
			}
		} else {
			// No UVs available for the mesh, zero fill.
			if (uv_maps.empty())
				uv_maps.resize(1, std::vector<float>(nInd * 2, 0.0f));
			// Already zero-filled by default
		}

		std::string groupName;
		auto nameIt = wmo->groupNames.find(group->nameOfs);
		if (nameIt != wmo->groupNames.end())
			groupName = nameIt->second;

		// Load all render batches into the mesh.
		for (size_t bI = 0; bI < group->renderBatches.size(); bI++) {
			const auto& batch = group->renderBatches[bI];
			std::vector<uint32_t> indices(batch.numFaces);

			for (uint16_t fi = 0; fi < batch.numFaces; fi++)
				indices[fi] = group->indices[batch.firstFace + fi] + static_cast<uint32_t>(indOfs);

			const int matID = ((batch.flags & 2) == 2) ? batch.possibleBox2[2] : batch.materialID;
			std::string matName;
			auto matIt = texMaps.materialMap.find(matID);
			if (matIt != texMaps.materialMap.end())
				matName = matIt->second;
			gltf.addMesh(groupName + std::to_string(bI), indices, matName);
		}

		indOfs += indCount;
	}

	gltf.setVerticesArray(vertices);
	gltf.setNormalArray(normals);
	
	for (const auto& uv_map : uv_maps)
		gltf.addUVArray(uv_map);

	// TODO: Add support for exporting doodads inside a GLTF WMO.

	gltf.write(core::view->config.value("overwriteFiles", true), format);
}

/**
 * Export the WMO model as a WaveFront OBJ.
 * @param out
 * @param helper
 * @param fileManifest
 */
void WMOExporter::exportAsOBJ(const std::filesystem::path& out, casc::ExportHelper* helper,
	std::vector<WMOExportFileManifest>* fileManifest, bool split_groups)
{
	if (split_groups) {
		exportGroupsAsSeparateOBJ(out, helper, fileManifest);
		return;
	}

	const auto& config = core::view->config;
	OBJWriter obj(out);
	const std::filesystem::path mtlPath(casc::ExportHelper::replaceExtension(out.string(), ".mtl"));
	MTLWriter mtl(mtlPath);

	const std::string wmoName = out.stem().string();
	obj.setName(wmoName);

	logging::write(std::format("Exporting WMO model {} as OBJ: {}", wmoName, out.string()));

	wmo->load();

	helper->setCurrentTaskName(wmoName + " textures");

	auto texMaps = exportTextures(out, &mtl, helper);

	if (helper->isCancelled())
		return;
		
	const auto& materialMap = texMaps.materialMap;
	const auto& textureMap = texMaps.textureMap;

	for (const auto& [texFileDataID, texInfo] : textureMap) {
		if (fileManifest)
			fileManifest->push_back({ "PNG", texFileDataID, texInfo.matPath });
	}

	std::vector<WMOLoader*> validGroups;
	size_t nInd = 0;
	size_t maxLayerCount = 0;

	std::set<uint32_t> mask;
	bool hasMask = false;

	// Map our user-facing group mask to a WMO mask.
	if (!groupMask.empty()) {
		hasMask = true;
		mask = buildGroupMaskSet(groupMask);
	}

	helper->setCurrentTaskName(wmoName + " groups");
	helper->setCurrentTaskMax(static_cast<int>(wmo->groupCount));

	// Iterate over the groups once to calculate the total size of our
	// vertex/normal/uv arrays allowing for pre-allocation.
	for (uint32_t i = 0; i < wmo->groupCount; i++) {
		// Abort if the export has been cancelled.
		if (helper->isCancelled())
			return;

		helper->setCurrentTaskValue(static_cast<int>(i));

		auto& group = wmo->getGroup(i);

		// Skip empty groups.
		if (group.renderBatches.empty())
			continue;

		// Skip masked groups.
		if (hasMask && mask.find(i) == mask.end())
			continue;

		// 3 verts per indices.
		nInd += group.vertices.size() / 3;

		// UV counts vary between groups, allocate for the max.
		maxLayerCount = std::max(group.uvs.size(), maxLayerCount);

		// Store the valid groups for quicker iteration later.
		validGroups.push_back(&group);
	}

	// Restrict to first UV layer if additional UV layers are not enabled.
	if (!config.value("modelsExportUV2", false))
		maxLayerCount = std::min(maxLayerCount, static_cast<size_t>(1));

	std::vector<float> vertsArray(nInd * 3);
	std::vector<float> normalsArray(nInd * 3);
	std::vector<std::vector<float>> uvArrays(maxLayerCount);

	// Create all necessary UV layer arrays.
	for (size_t i = 0; i < maxLayerCount; i++)
		uvArrays[i].resize(nInd * 2, 0.0f);

	// Iterate over groups again and fill the allocated arrays.
	size_t indOfs = 0;
	for (const auto* group : validGroups) {
		const size_t indCount = group->vertices.size() / 3;

		const size_t vertOfs = indOfs * 3;
		for (size_t vi = 0; vi < group->vertices.size(); vi++)
			vertsArray[vertOfs + vi] = group->vertices[vi];

		// Normals and vertices should match, so re-use vertOfs here.
		for (size_t ni = 0; ni < group->normals.size(); ni++)
			normalsArray[vertOfs + ni] = group->normals[ni];

		const size_t uvsOfs = indOfs * 2;
		const size_t uvCount = indCount * 2;

		// Write to all UV layers, even if we have no data.
		for (size_t li = 0; li < maxLayerCount; li++) {
			for (size_t j = 0; j < uvCount; j++) {
				float val = 0.0f;
				if (li < group->uvs.size() && j < group->uvs[li].size())
					val = group->uvs[li][j];
				uvArrays[li][uvsOfs + j] = val;
			}
		}

		std::string groupName;
		auto nameIt = wmo->groupNames.find(group->nameOfs);
		if (nameIt != wmo->groupNames.end())
			groupName = nameIt->second;

		// Load all render batches into the mesh.
		for (size_t bI = 0; bI < group->renderBatches.size(); bI++) {
			const auto& batch = group->renderBatches[bI];
			std::vector<uint32_t> indices(batch.numFaces);

			for (uint16_t fi = 0; fi < batch.numFaces; fi++)
				indices[fi] = group->indices[batch.firstFace + fi] + static_cast<uint32_t>(indOfs);

			const int matID = ((batch.flags & 2) == 2) ? batch.possibleBox2[2] : batch.materialID;
			std::string matName;
			auto matIt = materialMap.find(matID);
			if (matIt != materialMap.end())
				matName = matIt->second;
			obj.addMesh(groupName + std::to_string(bI), indices, matName);
		}

		indOfs += indCount;
	}

	obj.setVertArray(vertsArray);
	obj.setNormalArray(normalsArray);

	for (const auto& arr : uvArrays)
		obj.addUVArray(arr);

	const std::string csvPathStr = casc::ExportHelper::replaceExtension(out.string(), "_ModelPlacementInformation.csv");
	const std::filesystem::path csvPath(csvPathStr);
	if (config.value("overwriteFiles", true) || !generics::fileExists(csvPath)) {
		const bool useAbsolute = config.value("enableAbsoluteCSVPaths", false);
		const bool usePosix = config.value("pathFormat", std::string("")) == "posix";
		const std::filesystem::path outDir = out.parent_path();
		CSVWriter csv(csvPath);
		csv.addField({"ModelFile", "PositionX", "PositionY", "PositionZ", "RotationW", "RotationX", "RotationY", "RotationZ", "ScaleFactor", "DoodadSet", "FileDataID"});

		// Doodad sets.
		const auto& doodadSets = wmo->doodadSets;
		for (size_t i = 0; i < doodadSets.size(); i++) {
			// Skip disabled doodad sets.
			if (i >= doodadSetMask.size() || !doodadSetMask[i].checked)
				continue;

			const auto& set = doodadSets[i];
			const uint32_t count = set.doodadCount;
			logging::write(std::format("Exporting WMO doodad set {} with {} doodads...", set.name, count));

			helper->setCurrentTaskName(wmoName + ", doodad set " + set.name);
			helper->setCurrentTaskMax(static_cast<int>(count));

			for (uint32_t j = 0; j < count; j++) {
				// Abort if the export has been cancelled.
				if (helper->isCancelled())
					return;

				helper->setCurrentTaskValue(static_cast<int>(j));

				const auto& doodad = wmo->doodads[set.firstInstanceIndex + j];
				uint32_t doodadFileDataID = 0;
				std::string doodadFileName;
	
				if (!wmo->fileDataIDs.empty()) {
					// Retail, use fileDataID and lookup the filename.
					doodadFileDataID = wmo->fileDataIDs[doodad.offset];
					doodadFileName = casc::listfile::getByID(doodadFileDataID);
				} else {
					// Classic, use fileName and lookup the fileDataID.
					auto dnIt = wmo->doodadNames.find(doodad.offset);
					if (dnIt != wmo->doodadNames.end())
						doodadFileName = dnIt->second;
					doodadFileDataID = casc::listfile::getByFilename(doodadFileName).value_or(0);
				}
	
				if (doodadFileDataID > 0) {
					try {
						if (!doodadFileName.empty()) {
							// Replace M2 extension with OBJ.
							doodadFileName = casc::ExportHelper::replaceExtension(doodadFileName, ".obj");
						} else {
							// Handle unknown files.
							doodadFileName = casc::listfile::formatUnknownFile(doodadFileDataID, ".obj");
						}

						std::string m2Path;
						if (config.value("enableSharedChildren", false))
							m2Path = casc::ExportHelper::getExportPath(doodadFileName);
						else
							m2Path = casc::ExportHelper::replaceFile(out.string(), doodadFileName);

						// Only export doodads that are not already exported.
						if (doodadCache.find(doodadFileDataID) == doodadCache.end()) {
							auto doodadData = casc->getVirtualFileByID(doodadFileDataID);
							const uint32_t modelMagic = doodadData.readUInt32LE();
							doodadData.seek(0);
							if (modelMagic == constants::MAGIC::MD21) {
								M2Exporter m2Export(std::move(doodadData), {}, doodadFileDataID, casc);
								m2Export.exportAsOBJ(m2Path, config.value("modelsExportCollision", false), helper, nullptr);
							} else if (modelMagic == constants::MAGIC::M3DT) {
								M3Exporter m3Export(std::move(doodadData), {}, doodadFileDataID);
								m3Export.exportAsOBJ(m2Path, config.value("modelsExportCollision", false), helper, nullptr);
							}
							
							// Abort if the export has been cancelled.
							if (helper->isCancelled())
								return;

							doodadCache.insert(doodadFileDataID);
						}

						std::string modelPath = std::filesystem::relative(m2Path, outDir).string();

						if (useAbsolute)
							modelPath = std::filesystem::absolute(std::filesystem::path(outDir) / modelPath).string();

						if (usePosix)
							modelPath = casc::ExportHelper::win32ToPosix(modelPath);

						csv.addRow({
							{ "ModelFile", modelPath },
							{ "PositionX", std::to_string(doodad.position[0]) },
							{ "PositionY", std::to_string(doodad.position[1]) },
							{ "PositionZ", std::to_string(doodad.position[2]) },
							{ "RotationW", std::to_string(doodad.rotation[3]) },
							{ "RotationX", std::to_string(doodad.rotation[0]) },
							{ "RotationY", std::to_string(doodad.rotation[1]) },
							{ "RotationZ", std::to_string(doodad.rotation[2]) },
							{ "ScaleFactor", std::to_string(doodad.scale) },
							{ "DoodadSet", set.name },
							{ "FileDataID", std::to_string(doodadFileDataID) },
						});
					} catch (const std::exception& e) {
						logging::write(std::format("Failed to load doodad {} for {}: {}", doodadFileDataID, set.name, e.what()));
					}
				}
			}
		}

		csv.write();
		if (fileManifest)
			fileManifest->push_back({ "PLACEMENT", wmo->fileDataID, csvPath });
	} else {
		logging::write(std::format("Skipping model placement export {} (file exists, overwrite disabled)", csvPath.string()));
	}

	if (!mtl.isEmpty())
		obj.setMaterialLibrary(mtlPath.filename().string());

	obj.write(config.value("overwriteFiles", true));
	if (fileManifest)
		fileManifest->push_back({ "OBJ", wmo->fileDataID, out });

	mtl.write(config.value("overwriteFiles", true));
	if (fileManifest)
		fileManifest->push_back({ "MTL", wmo->fileDataID, mtlPath });

	if (config.value("exportWMOMeta", false)) {
		helper->clearCurrentTask();
		helper->setCurrentTaskName(wmoName + ", writing meta data");

		const std::string jsonPathStr = casc::ExportHelper::replaceExtension(out.string(), ".json");
		JSONWriter json(jsonPathStr);
		json.addProperty("fileType", "wmo");
		json.addProperty("fileDataID", wmo->fileDataID);
		json.addProperty("fileName", wmo->fileName);
		json.addProperty("version", wmo->version);
		json.addProperty("counts", nlohmann::json{
			{"material", wmo->materialCount},
			{"group", wmo->groupCount},
			{"portal", wmo->portalCount},
			{"light", wmo->lightCount},
			{"model", wmo->modelCount},
			{"doodad", wmo->doodadCount},
			{"set", wmo->setCount},
			{"lod", wmo->lodCount}
		});
		
		json.addProperty("portalVertices", nlohmann::json(wmo->portalVertices));

		{
			nlohmann::json piArr = nlohmann::json::array();
			for (const auto& p : wmo->portalInfo)
				piArr.push_back(portalInfoToJson(p));
			json.addProperty("portalInfo", piArr);
		}

		{
			nlohmann::json prArr = nlohmann::json::array();
			for (const auto& p : wmo->mopr)
				prArr.push_back(portalRefToJson(p));
			json.addProperty("portalMapObjectRef", prArr);
		}

		json.addProperty("ambientColor", wmo->ambientColor);
		json.addProperty("wmoID", wmo->wmoID);
		json.addProperty("boundingBox1", nlohmann::json(wmo->boundingBox1));
		json.addProperty("boundingBox2", nlohmann::json(wmo->boundingBox2));

		{
			nlohmann::json fogArr = nlohmann::json::array();
			for (const auto& f : wmo->fogs)
				fogArr.push_back(fogToJson(f));
			json.addProperty("fog", fogArr);
		}

		json.addProperty("flags", wmo->flags);

		{
			nlohmann::json groupsArr = nlohmann::json::array();
			for (size_t gi = 0; gi < wmo->groups.size(); gi++) {
				const auto* grp = wmo->groups[gi];
				nlohmann::json gj;

				auto gnIt = wmo->groupNames.find(grp->nameOfs);
				gj["groupName"] = (gnIt != wmo->groupNames.end()) ? gnIt->second : "";

				auto gdIt = wmo->groupNames.find(grp->descOfs);
				gj["groupDescription"] = (gdIt != wmo->groupNames.end()) ? gdIt->second : "";

				gj["enabled"] = !hasMask || mask.find(static_cast<uint32_t>(gi)) != mask.end();
				gj["version"] = grp->version;
				gj["flags"] = grp->groupFlags;
				gj["ambientColor"] = grp->ambientColor;
				gj["boundingBox1"] = grp->boundingBox1;
				gj["boundingBox2"] = grp->boundingBox2;
				gj["numPortals"] = grp->numPortals;
				gj["numBatchesA"] = grp->numBatchesA;
				gj["numBatchesB"] = grp->numBatchesB;
				gj["numBatchesC"] = grp->numBatchesC;
				gj["liquidType"] = grp->liquidType;
				gj["groupID"] = grp->groupID;

				{
					nlohmann::json miArr = nlohmann::json::array();
					for (const auto& mi : grp->materialInfo)
						miArr.push_back(materialInfoToJson(mi));
					gj["materialInfo"] = miArr;
				}
				{
					nlohmann::json rbArr = nlohmann::json::array();
					for (const auto& rb : grp->renderBatches)
						rbArr.push_back(renderBatchToJson(rb));
					gj["renderBatches"] = rbArr;
				}
				gj["vertexColours"] = nlohmann::json(grp->vertexColours);
				gj["liquid"] = liquidToJson(grp->liquid);

				groupsArr.push_back(gj);
			}
			json.addProperty("groups", groupsArr);
		}

		{
			nlohmann::json gnArr = nlohmann::json::array();
			for (const auto& [offset, name] : wmo->groupNames)
				gnArr.push_back(name);
			json.addProperty("groupNames", gnArr);
		}

		{
			nlohmann::json giArr = nlohmann::json::array();
			for (const auto& gi : wmo->groupInfo)
				giArr.push_back(groupInfoToJson(gi));
			json.addProperty("groupInfo", giArr);
		}

		// Create a textures array and push every unique fileDataID from the
		// material stack, expanded with file name/path data for external QoL.
		{
			nlohmann::json textures = nlohmann::json::array();
			std::set<uint32_t> textureCache;
			for (const auto& material : wmo->materials) {
				std::vector<uint32_t> materialTextures = { material.texture1, material.texture2, material.texture3 };

				// Look up vertex/pixel shader from the shader map
				int vertexShader = -1;
				int pixelShader = -1;
				auto shaderIt = wmo_shader_mapper::WMOShaderMap.find(static_cast<int>(material.shader));
				if (shaderIt != wmo_shader_mapper::WMOShaderMap.end()) {
					vertexShader = static_cast<int>(shaderIt->second.VertexShader);
					pixelShader = static_cast<int>(shaderIt->second.PixelShader);
				}

				if (pixelShader == 19) {
					materialTextures.push_back(material.color2);
					materialTextures.push_back(material.flags3);
					if (!material.runtimeData.empty())
						materialTextures.push_back(material.runtimeData[0]);
				} else if (pixelShader == 20) {
					materialTextures.push_back(material.color3);
					for (const auto rtdTexture : material.runtimeData)
						materialTextures.push_back(rtdTexture);
				}

				for (const auto materialTexture : materialTextures) {
					if (materialTexture == 0 || textureCache.count(materialTexture))
						continue;

					textureCache.insert(materialTexture);

					nlohmann::json texEntry;
					texEntry["fileDataID"] = materialTexture;
					std::string internalName = casc::listfile::getByID(materialTexture);
					texEntry["fileNameInternal"] = internalName.empty() ? nlohmann::json(nullptr) : nlohmann::json(internalName);

					auto texIt = textureMap.find(materialTexture);
					if (texIt != textureMap.end()) {
						texEntry["fileNameExternal"] = texIt->second.matPathRelative;
						texEntry["mtlName"] = texIt->second.matName;
					} else {
						texEntry["fileNameExternal"] = nullptr;
						texEntry["mtlName"] = nullptr;
					}
					textures.push_back(texEntry);
				}
			}
			json.addProperty("textures", textures);
		}

		{
			nlohmann::json matsArr = nlohmann::json::array();
			for (const auto& mat : wmo->materials) {
				auto matJson = materialToJson(mat);
				// Add vertex/pixel shader from shader map
				auto shaderIt = wmo_shader_mapper::WMOShaderMap.find(static_cast<int>(mat.shader));
				if (shaderIt != wmo_shader_mapper::WMOShaderMap.end()) {
					matJson["vertexShader"] = static_cast<int>(shaderIt->second.VertexShader);
					matJson["pixelShader"] = static_cast<int>(shaderIt->second.PixelShader);
				}
				matsArr.push_back(matJson);
			}
			json.addProperty("materials", matsArr);
		}

		{
			nlohmann::json dsArr = nlohmann::json::array();
			for (const auto& ds : wmo->doodadSets)
				dsArr.push_back(doodadSetToJson(ds));
			json.addProperty("doodadSets", dsArr);
		}

		json.addProperty("fileDataIDs", nlohmann::json(wmo->fileDataIDs));

		{
			nlohmann::json ddArr = nlohmann::json::array();
			for (const auto& dd : wmo->doodads)
				ddArr.push_back(doodadToJson(dd));
			json.addProperty("doodads", ddArr);
		}

		json.addProperty("groupIDs", nlohmann::json(wmo->groupIDs));

		json.write(config.value("overwriteFiles", true));
		if (fileManifest)
			fileManifest->push_back({ "META", wmo->fileDataID, jsonPathStr });
	}
}

/**
 * Export the WMO model as an STL file.
 * @param out
 * @param helper
 * @param fileManifest
 */
void WMOExporter::exportAsSTL(const std::filesystem::path& out, casc::ExportHelper* helper,
	std::vector<WMOExportFileManifest>* fileManifest)
{
	const auto& config = core::view->config;
	STLWriter stl(out);

	const std::string wmoName = out.stem().string();
	stl.setName(wmoName);

	logging::write(std::format("Exporting WMO model {} as STL: {}", wmoName, out.string()));

	wmo->load();

	std::vector<WMOLoader*> validGroups;
	size_t nInd = 0;

	std::set<uint32_t> mask;
	bool hasMask = false;

	// map our user-facing group mask to a wmo mask
	if (!groupMask.empty()) {
		hasMask = true;
		mask = buildGroupMaskSet(groupMask);
	}

	helper->setCurrentTaskName(wmoName + " groups");
	helper->setCurrentTaskMax(static_cast<int>(wmo->groupCount));

	// iterate over the groups once to calculate the total size
	for (uint32_t i = 0; i < wmo->groupCount; i++) {
		if (helper->isCancelled())
			return;

		helper->setCurrentTaskValue(static_cast<int>(i));

		auto& group = wmo->getGroup(i);

		// skip empty groups
		if (group.renderBatches.empty())
			continue;

		// skip masked groups
		if (hasMask && mask.find(i) == mask.end())
			continue;

		// 3 verts per indices
		nInd += group.vertices.size() / 3;

		// store the valid groups for quicker iteration later
		validGroups.push_back(&group);
	}

	std::vector<float> vertsArray(nInd * 3);
	std::vector<float> normalsArray(nInd * 3);

	// iterate over groups again and fill the allocated arrays
	size_t indOfs = 0;
	for (const auto* group : validGroups) {
		const size_t indCount = group->vertices.size() / 3;

		const size_t vertOfs = indOfs * 3;
		for (size_t vi = 0; vi < group->vertices.size(); vi++)
			vertsArray[vertOfs + vi] = group->vertices[vi];

		// normals and vertices should match, so re-use vertOfs here
		for (size_t ni = 0; ni < group->normals.size(); ni++)
			normalsArray[vertOfs + ni] = group->normals[ni];

		std::string groupName;
		auto nameIt = wmo->groupNames.find(group->nameOfs);
		if (nameIt != wmo->groupNames.end())
			groupName = nameIt->second;

		// load all render batches into the mesh
		for (size_t bI = 0; bI < group->renderBatches.size(); bI++) {
			const auto& batch = group->renderBatches[bI];
			std::vector<uint32_t> indices(batch.numFaces);

			for (uint16_t fi = 0; fi < batch.numFaces; fi++)
				indices[fi] = group->indices[batch.firstFace + fi] + static_cast<uint32_t>(indOfs);

			stl.addMesh(groupName + std::to_string(bI), indices);
		}

		indOfs += indCount;
	}

	stl.setVertArray(vertsArray);
	stl.setNormalArray(normalsArray);

	stl.write(config.value("overwriteFiles", true));
	if (fileManifest)
		fileManifest->push_back({ "STL", wmo->fileDataID, out });
}

/**
 * export each wmo group as separate obj file
 * @param out
 * @param helper
 * @param fileManifest
 */
void WMOExporter::exportGroupsAsSeparateOBJ(const std::filesystem::path& out, casc::ExportHelper* helper,
	std::vector<WMOExportFileManifest>* fileManifest)
{
	const auto& config = core::view->config;

	wmo->load();

	const std::string wmoName = out.stem().string();
	const std::filesystem::path outDir = out.parent_path();

	logging::write(std::format("exporting wmo model {} as split obj: {}", wmoName, out.string()));

	// export textures once, shared across all groups
	helper->setCurrentTaskName(wmoName + " textures");

	const std::filesystem::path sharedMTLPath(casc::ExportHelper::replaceExtension(out.string(), ".mtl"));
	MTLWriter sharedMTL(sharedMTLPath);
	auto texMaps = exportTextures(out, &sharedMTL, helper);

	if (helper->isCancelled())
		return;

	const auto& textureMap = texMaps.textureMap;
	const auto& materialMap = texMaps.materialMap;

	for (const auto& [texFileDataID, texInfo] : textureMap) {
		if (fileManifest)
			fileManifest->push_back({ "PNG", texFileDataID, texInfo.matPath });
	}

	// build group mask
	std::set<uint32_t> mask;
	bool hasMask = false;
	if (!groupMask.empty()) {
		hasMask = true;
		mask = buildGroupMaskSet(groupMask);
	}

	helper->setCurrentTaskName(wmoName + " groups");
	helper->setCurrentTaskMax(static_cast<int>(wmo->groupCount));

	// export each group separately
	for (uint32_t i = 0; i < wmo->groupCount; i++) {
		if (helper->isCancelled())
			return;

		helper->setCurrentTaskValue(static_cast<int>(i));

		auto& group = wmo->getGroup(i);

		// skip empty groups
		if (group.renderBatches.empty())
			continue;

		// skip masked groups
		if (hasMask && mask.find(i) == mask.end())
			continue;

		std::string groupName;
		auto nameIt = wmo->groupNames.find(group.nameOfs);
		if (nameIt != wmo->groupNames.end())
			groupName = nameIt->second;
		const std::string groupFileName = wmoName + "_" + groupName + ".obj";
		const std::filesystem::path groupOut = outDir / groupFileName;

		OBJWriter obj(groupOut);
		obj.setName(groupFileName);

		logging::write(std::format("exporting wmo group {}: {}", groupName, groupOut.string()));

		// prepare arrays for this group
		const size_t indCount = group.vertices.size() / 3;
		std::vector<float> vertsArray(group.vertices.begin(), group.vertices.end());
		std::vector<float> normalsArray(group.normals.begin(), group.normals.end());

		// handle uv layers
		const size_t maxLayerCount = config.value("modelsExportUV2", false)
			? group.uvs.size()
			: std::min(group.uvs.size(), static_cast<size_t>(1));
		const size_t uvCount = indCount * 2;

		std::vector<std::vector<float>> uvArrays(maxLayerCount);
		for (size_t j = 0; j < maxLayerCount; j++) {
			uvArrays[j].resize(uvCount, 0.0f);
			if (j < group.uvs.size()) {
				const auto& uv = group.uvs[j];
				for (size_t k = 0; k < uvCount && k < uv.size(); k++)
					uvArrays[j][k] = uv[k];
			}
		}

		obj.setVertArray(vertsArray);
		obj.setNormalArray(normalsArray);

		for (const auto& arr : uvArrays)
			obj.addUVArray(arr);

		// add render batches
		for (size_t bI = 0; bI < group.renderBatches.size(); bI++) {
			const auto& batch = group.renderBatches[bI];
			std::vector<uint32_t> indices(batch.numFaces);

			for (uint16_t j = 0; j < batch.numFaces; j++)
				indices[j] = group.indices[batch.firstFace + j];

			const int matID = ((batch.flags & 2) == 2) ? batch.possibleBox2[2] : batch.materialID;
			std::string matName;
			auto matIt = materialMap.find(matID);
			if (matIt != materialMap.end())
				matName = matIt->second;
			obj.addMesh(groupName + std::to_string(bI), indices, matName);
		}

		if (!sharedMTL.isEmpty())
			obj.setMaterialLibrary(sharedMTLPath.filename().string());

		obj.write(config.value("overwriteFiles", true));
		if (fileManifest)
			fileManifest->push_back({ "OBJ", wmo->fileDataID, groupOut });
	}

	// write shared mtl
	sharedMTL.write(config.value("overwriteFiles", true));
	if (fileManifest)
		fileManifest->push_back({ "MTL", wmo->fileDataID, sharedMTLPath });

	// export doodad placement csv (shared across all groups)
	const std::string csvPathStr = casc::ExportHelper::replaceExtension(out.string(), "_ModelPlacementInformation.csv");
	const std::filesystem::path csvPath(csvPathStr);
	if (config.value("overwriteFiles", true) || !generics::fileExists(csvPath)) {
		const bool useAbsolute = config.value("enableAbsoluteCSVPaths", false);
		const bool usePosix = config.value("pathFormat", std::string("")) == "posix";
		CSVWriter csv(csvPath);
		csv.addField({"ModelFile", "PositionX", "PositionY", "PositionZ", "RotationW", "RotationX", "RotationY", "RotationZ", "ScaleFactor", "DoodadSet", "FileDataID"});

		// doodad sets
		const auto& doodadSets = wmo->doodadSets;
		for (size_t i = 0; i < doodadSets.size(); i++) {
			if (i >= doodadSetMask.size() || !doodadSetMask[i].checked)
				continue;

			const auto& set = doodadSets[i];
			const uint32_t count = set.doodadCount;
			logging::write(std::format("exporting wmo doodad set {} with {} doodads...", set.name, count));

			helper->setCurrentTaskName(wmoName + ", doodad set " + set.name);
			helper->setCurrentTaskMax(static_cast<int>(count));

			for (uint32_t j = 0; j < count; j++) {
				if (helper->isCancelled())
					return;

				helper->setCurrentTaskValue(static_cast<int>(j));

				const auto& doodad = wmo->doodads[set.firstInstanceIndex + j];
				uint32_t doodadFileDataID = 0;
				std::string doodadFileName;

				if (!wmo->fileDataIDs.empty()) {
					doodadFileDataID = wmo->fileDataIDs[doodad.offset];
					doodadFileName = casc::listfile::getByID(doodadFileDataID);
				} else {
					auto dnIt = wmo->doodadNames.find(doodad.offset);
					if (dnIt != wmo->doodadNames.end())
						doodadFileName = dnIt->second;
					doodadFileDataID = casc::listfile::getByFilename(doodadFileName).value_or(0);
				}

				if (doodadFileDataID > 0) {
					try {
						if (!doodadFileName.empty()) {
							doodadFileName = casc::ExportHelper::replaceExtension(doodadFileName, ".obj");
						} else {
							doodadFileName = casc::listfile::formatUnknownFile(doodadFileDataID, ".obj");
						}

						std::string m2Path;
						if (config.value("enableSharedChildren", false))
							m2Path = casc::ExportHelper::getExportPath(doodadFileName);
						else
							m2Path = casc::ExportHelper::replaceFile(out.string(), doodadFileName);

						if (doodadCache.find(doodadFileDataID) == doodadCache.end()) {
							auto doodadData = casc->getVirtualFileByID(doodadFileDataID);
							const uint32_t modelMagic = doodadData.readUInt32LE();
							doodadData.seek(0);
							if (modelMagic == constants::MAGIC::MD21) {
								M2Exporter m2Export(std::move(doodadData), {}, doodadFileDataID, casc);
								m2Export.exportAsOBJ(m2Path, config.value("modelsExportCollision", false), helper, nullptr);
							} else if (modelMagic == constants::MAGIC::M3DT) {
								M3Exporter m3Export(std::move(doodadData), {}, doodadFileDataID);
								m3Export.exportAsOBJ(m2Path, config.value("modelsExportCollision", false), helper, nullptr);
							}

							if (helper->isCancelled())
								return;

							doodadCache.insert(doodadFileDataID);
						}

						std::string modelPath = std::filesystem::relative(m2Path, outDir).string();

						if (useAbsolute)
							modelPath = std::filesystem::absolute(std::filesystem::path(outDir) / modelPath).string();

						if (usePosix)
							modelPath = casc::ExportHelper::win32ToPosix(modelPath);

						csv.addRow({
							{ "ModelFile", modelPath },
							{ "PositionX", std::to_string(doodad.position[0]) },
							{ "PositionY", std::to_string(doodad.position[1]) },
							{ "PositionZ", std::to_string(doodad.position[2]) },
							{ "RotationW", std::to_string(doodad.rotation[3]) },
							{ "RotationX", std::to_string(doodad.rotation[0]) },
							{ "RotationY", std::to_string(doodad.rotation[1]) },
							{ "RotationZ", std::to_string(doodad.rotation[2]) },
							{ "ScaleFactor", std::to_string(doodad.scale) },
							{ "DoodadSet", set.name },
							{ "FileDataID", std::to_string(doodadFileDataID) },
						});
					} catch (const std::exception& e) {
						logging::write(std::format("failed to load doodad {} for {}: {}", doodadFileDataID, set.name, e.what()));
					}
				}
			}
		}

		csv.write();
		if (fileManifest)
			fileManifest->push_back({ "PLACEMENT", wmo->fileDataID, csvPath });
	} else {
		logging::write(std::format("skipping model placement export {} (file exists, overwrite disabled)", csvPath.string()));
	}

	// export meta if enabled
	if (config.value("exportWMOMeta", false)) {
		helper->clearCurrentTask();
		helper->setCurrentTaskName(wmoName + ", writing meta data");

		const std::string jsonPathStr = casc::ExportHelper::replaceExtension(out.string(), ".json");
		JSONWriter json(jsonPathStr);
		json.addProperty("fileType", "wmo");
		json.addProperty("fileDataID", wmo->fileDataID);
		json.addProperty("fileName", wmo->fileName);
		json.addProperty("version", wmo->version);
		json.addProperty("counts", nlohmann::json{
			{"material", wmo->materialCount},
			{"group", wmo->groupCount},
			{"portal", wmo->portalCount},
			{"light", wmo->lightCount},
			{"model", wmo->modelCount},
			{"doodad", wmo->doodadCount},
			{"set", wmo->setCount},
			{"lod", wmo->lodCount}
		});

		json.addProperty("portalVertices", nlohmann::json(wmo->portalVertices));

		{
			nlohmann::json piArr = nlohmann::json::array();
			for (const auto& p : wmo->portalInfo)
				piArr.push_back(portalInfoToJson(p));
			json.addProperty("portalInfo", piArr);
		}

		{
			nlohmann::json prArr = nlohmann::json::array();
			for (const auto& p : wmo->mopr)
				prArr.push_back(portalRefToJson(p));
			json.addProperty("portalMapObjectRef", prArr);
		}

		json.addProperty("ambientColor", wmo->ambientColor);
		json.addProperty("wmoID", wmo->wmoID);
		json.addProperty("boundingBox1", nlohmann::json(wmo->boundingBox1));
		json.addProperty("boundingBox2", nlohmann::json(wmo->boundingBox2));

		{
			nlohmann::json fogArr = nlohmann::json::array();
			for (const auto& f : wmo->fogs)
				fogArr.push_back(fogToJson(f));
			json.addProperty("fog", fogArr);
		}

		json.addProperty("flags", wmo->flags);

		{
			nlohmann::json groupsArr = nlohmann::json::array();
			for (size_t gi = 0; gi < wmo->groups.size(); gi++) {
				const auto* grp = wmo->groups[gi];
				nlohmann::json gj;

				auto gnIt = wmo->groupNames.find(grp->nameOfs);
				gj["groupName"] = (gnIt != wmo->groupNames.end()) ? gnIt->second : "";

				auto gdIt = wmo->groupNames.find(grp->descOfs);
				gj["groupDescription"] = (gdIt != wmo->groupNames.end()) ? gdIt->second : "";

				gj["enabled"] = !hasMask || mask.find(static_cast<uint32_t>(gi)) != mask.end();
				gj["version"] = grp->version;
				gj["flags"] = grp->groupFlags;
				gj["ambientColor"] = grp->ambientColor;
				gj["boundingBox1"] = grp->boundingBox1;
				gj["boundingBox2"] = grp->boundingBox2;
				gj["numPortals"] = grp->numPortals;
				gj["numBatchesA"] = grp->numBatchesA;
				gj["numBatchesB"] = grp->numBatchesB;
				gj["numBatchesC"] = grp->numBatchesC;
				gj["liquidType"] = grp->liquidType;
				gj["groupID"] = grp->groupID;

				{
					nlohmann::json miArr = nlohmann::json::array();
					for (const auto& mi : grp->materialInfo)
						miArr.push_back(materialInfoToJson(mi));
					gj["materialInfo"] = miArr;
				}
				{
					nlohmann::json rbArr = nlohmann::json::array();
					for (const auto& rb : grp->renderBatches)
						rbArr.push_back(renderBatchToJson(rb));
					gj["renderBatches"] = rbArr;
				}
				gj["vertexColours"] = nlohmann::json(grp->vertexColours);
				gj["liquid"] = liquidToJson(grp->liquid);

				groupsArr.push_back(gj);
			}
			json.addProperty("groups", groupsArr);
		}

		{
			nlohmann::json gnArr = nlohmann::json::array();
			for (const auto& [offset, name] : wmo->groupNames)
				gnArr.push_back(name);
			json.addProperty("groupNames", gnArr);
		}

		{
			nlohmann::json giArr = nlohmann::json::array();
			for (const auto& gi : wmo->groupInfo)
				giArr.push_back(groupInfoToJson(gi));
			json.addProperty("groupInfo", giArr);
		}

		{
			nlohmann::json textures = nlohmann::json::array();
			std::set<uint32_t> textureCache;
			for (const auto& material : wmo->materials) {
				std::vector<uint32_t> materialTextures = { material.texture1, material.texture2, material.texture3 };

				if (material.shader == 23) {
					materialTextures.push_back(material.color3);
					materialTextures.push_back(material.flags3);
					if (material.runtimeData.size() > 0) materialTextures.push_back(material.runtimeData[0]);
					if (material.runtimeData.size() > 1) materialTextures.push_back(material.runtimeData[1]);
					if (material.runtimeData.size() > 2) materialTextures.push_back(material.runtimeData[2]);
					if (material.runtimeData.size() > 3) materialTextures.push_back(material.runtimeData[3]);
				}

				for (const auto materialTexture : materialTextures) {
					if (materialTexture == 0 || textureCache.count(materialTexture))
						continue;

					textureCache.insert(materialTexture);

					nlohmann::json texEntry;
					texEntry["fileDataID"] = materialTexture;
					std::string internalName = casc::listfile::getByID(materialTexture);
					texEntry["fileNameInternal"] = internalName.empty() ? nlohmann::json(nullptr) : nlohmann::json(internalName);

					auto texIt = textureMap.find(materialTexture);
					if (texIt != textureMap.end()) {
						texEntry["fileNameExternal"] = texIt->second.matPathRelative;
						texEntry["mtlName"] = texIt->second.matName;
					} else {
						texEntry["fileNameExternal"] = nullptr;
						texEntry["mtlName"] = nullptr;
					}
					textures.push_back(texEntry);
				}
			}
			json.addProperty("textures", textures);
		}

		{
			nlohmann::json matsArr = nlohmann::json::array();
			for (const auto& mat : wmo->materials)
				matsArr.push_back(materialToJson(mat));
			json.addProperty("materials", matsArr);
		}

		{
			nlohmann::json dsArr = nlohmann::json::array();
			for (const auto& ds : wmo->doodadSets)
				dsArr.push_back(doodadSetToJson(ds));
			json.addProperty("doodadSets", dsArr);
		}

		json.addProperty("fileDataIDs", nlohmann::json(wmo->fileDataIDs));

		{
			nlohmann::json ddArr = nlohmann::json::array();
			for (const auto& dd : wmo->doodads)
				ddArr.push_back(doodadToJson(dd));
			json.addProperty("doodads", ddArr);
		}

		json.addProperty("groupIDs", nlohmann::json(wmo->groupIDs));

		json.write(config.value("overwriteFiles", true));
		if (fileManifest)
			fileManifest->push_back({ "META", wmo->fileDataID, jsonPathStr });
	}
}

/**
 *
 * @param out
 * @param helper
 * @param fileManifest
 */
void WMOExporter::exportRaw(const std::filesystem::path& out, casc::ExportHelper* helper,
	std::vector<WMOExportFileManifest>* fileManifest)
{
	const auto& config = core::view->config;

	const std::string manifestFileStr = casc::ExportHelper::replaceExtension(out.string(), ".manifest.json");
	JSONWriter manifest(manifestFileStr);

	manifest.addProperty("fileDataID", wmo->fileDataID);

	// Write the raw WMO file with no conversion.
	if (data.byteLength() == 0) {
		auto wmoData = casc->getVirtualFileByID(wmo->fileDataID);
		wmoData.writeToFile(out);
	} else {
		data.seek(0);
		data.writeToFile(out);
	}
	
	if (fileManifest)
		fileManifest->push_back({ "WMO", wmo->fileDataID, out });

	wmo->load();

	// Export raw textures.
	auto textures = exportTextures(out, nullptr, helper, true);
	nlohmann::json texturesManifest = nlohmann::json::array();
	for (const auto& [texFileDataID, texInfo] : textures.textureMap) {
		if (fileManifest)
			fileManifest->push_back({ "BLP", texFileDataID, texInfo.matPath });
		texturesManifest.push_back({ {"fileDataID", texFileDataID}, {"file", std::filesystem::relative(texInfo.matPath, out).string()} });
	}

	manifest.addProperty("textures", texturesManifest);

	if (config.value("modelsExportWMOGroups", false)) {
		nlohmann::json groupManifest = nlohmann::json::array();
		const std::string wmoFileName = wmo->fileName;

		const uint32_t lodCount = wmo->groupIDs.empty() ? 0 : static_cast<uint32_t>(wmo->groupIDs.size()) / wmo->groupCount;

		uint32_t groupOffset = 0;
		for (uint32_t lodIndex = 0; lodIndex < lodCount; lodIndex++) {
			for (uint32_t groupIndex = 0; groupIndex < wmo->groupCount; groupIndex++) {
				// Abort if the export has been cancelled.
				if (helper->isCancelled())
					return;
	
				std::string groupName;
				std::string paddedIndex = std::to_string(groupIndex);
				while (paddedIndex.size() < 3)
					paddedIndex = "0" + paddedIndex;

				if (lodIndex > 0)
					groupName = casc::ExportHelper::replaceExtension(wmoFileName, "_" + paddedIndex + "_lod" + std::to_string(lodIndex) + ".wmo");
				else
					groupName = casc::ExportHelper::replaceExtension(wmoFileName, "_" + paddedIndex + ".wmo");
				
				uint32_t groupFileDataID = 0;
				if (groupOffset < wmo->groupIDs.size())
					groupFileDataID = wmo->groupIDs[groupOffset];
				if (groupFileDataID == 0) {
					auto fid = casc::listfile::getByFilename(groupName);
					groupFileDataID = fid.value_or(0);
				}
				groupOffset++;

				if (groupFileDataID == 0)
					continue;

				auto groupData = casc->getVirtualFileByID(groupFileDataID);
				
				std::filesystem::path groupFile;
				if (config.value("enableSharedChildren", false))
					groupFile = casc::ExportHelper::getExportPath(groupName);
				else
					groupFile = out.parent_path() / std::filesystem::path(groupName).filename();
	
				groupData.writeToFile(groupFile);
	
				if (fileManifest)
					fileManifest->push_back({ "WMO_GROUP", groupFileDataID, groupFile });
				groupManifest.push_back({ {"fileDataID", groupFileDataID}, {"file", std::filesystem::relative(groupFile, out).string()} });
			}
		}

		manifest.addProperty("groups", groupManifest);
	}

	// Doodad sets.
	const auto& doodadSets = wmo->doodadSets;
	for (size_t i = 0; i < doodadSets.size(); i++) {
		const auto& set = doodadSets[i];
		const uint32_t count = set.doodadCount;
		logging::write(std::format("Exporting WMO doodad set {} with {} doodads...", set.name, count));

		helper->setCurrentTaskName("Doodad set " + set.name);
		helper->setCurrentTaskMax(static_cast<int>(count));

		for (uint32_t j = 0; j < count; j++) {
			// Abort if the export has been cancelled.
			if (helper->isCancelled())
				return;

			helper->setCurrentTaskValue(static_cast<int>(j));

			const auto& doodad = wmo->doodads[set.firstInstanceIndex + j];
			uint32_t doodadFileDataID = 0;
			std::string doodadFileName;

			if (!wmo->fileDataIDs.empty()) {
				// Retail, use fileDataID and lookup the filename.
				doodadFileDataID = wmo->fileDataIDs[doodad.offset];
				doodadFileName = casc::listfile::getByID(doodadFileDataID);
			} else {
				// Classic, use fileName and lookup the fileDataID.
				auto dnIt = wmo->doodadNames.find(doodad.offset);
				if (dnIt != wmo->doodadNames.end())
					doodadFileName = dnIt->second;
				doodadFileDataID = casc::listfile::getByFilename(doodadFileName).value_or(0);
			}

			if (doodadFileDataID > 0) {
				try {
					if (doodadFileName.empty()) {
						// Handle unknown files.
						doodadFileName = casc::listfile::formatUnknownFile(doodadFileDataID, ".m2");
					}

					std::string m2Path;
					if (config.value("enableSharedChildren", false))
						m2Path = casc::ExportHelper::getExportPath(doodadFileName);
					else
						m2Path = casc::ExportHelper::replaceFile(out.string(), doodadFileName);

					// Only export doodads that are not already exported.
					if (doodadCache.find(doodadFileDataID) == doodadCache.end()) {
						
						auto doodadData = casc->getVirtualFileByID(doodadFileDataID);
						const uint32_t modelMagic = doodadData.readUInt32LE();
						doodadData.seek(0);
						if (modelMagic == constants::MAGIC::MD21) {
							M2Exporter m2Export(std::move(doodadData), {}, doodadFileDataID, casc);
							m2Export.exportRaw(m2Path, helper, nullptr);
						} else if (modelMagic == constants::MAGIC::M3DT) {
							M3Exporter m3Export(std::move(doodadData), {}, doodadFileDataID);
							m3Export.exportRaw(m2Path, helper, nullptr);
						}

						// Abort if the export has been cancelled.
						if (helper->isCancelled())
							return;

						doodadCache.insert(doodadFileDataID);
					}
				} catch (const std::exception& e) {
					logging::write(std::format("Failed to load doodad {} for {}: {}", doodadFileDataID, set.name, e.what()));
				}
			}
		}
	}

	manifest.write();
}

/**
 * Clear the WMO exporting cache.
 */
void WMOExporter::clearCache() {
	doodadCache.clear();
}

void WMOExporter::loadWMO() {
	wmo->load();
}

std::vector<std::string> WMOExporter::getDoodadSetNames() const {
	std::vector<std::string> names;
	for (const auto& s : wmo->doodadSets)
		names.push_back(s.name);
	return names;
}
