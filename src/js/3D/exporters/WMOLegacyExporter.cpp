/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
*/

#include "WMOLegacyExporter.h"
#include "M2LegacyExporter.h"

#include <algorithm>
#include <cctype>
#include <format>
#include <iomanip>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "../../core.h"
#include "../../log.h"
#include "../../generics.h"
#include "../../buffer.h"
#include "../../casc/blp.h"
#include "../../casc/export-helper.h"
#include "../../mpq/mpq-install.h"
#include "../loaders/M2LegacyLoader.h"
#include "../loaders/WMOLegacyLoader.h"
#include "../Texture.h"
#include "../writers/JSONWriter.h"
#include "../writers/OBJWriter.h"
#include "../writers/MTLWriter.h"
#include "../writers/STLWriter.h"
#include "../writers/CSVWriter.h"

namespace {

std::unordered_set<std::string> doodadCache;

std::string toLower(const std::string& s) {
	std::string result = s;
	std::transform(result.begin(), result.end(), result.begin(),
		[](unsigned char c) { return std::tolower(c); });
	return result;
}

} // anonymous namespace

WMOLegacyExporter::WMOLegacyExporter(BufferWrapper data, const std::string& filePath, mpq::MPQInstall* mpq)
	: data(std::move(data))
	, filePath(filePath)
	, mpq(mpq)
{
	// extract mpq prefix from filepath (e.g. "wmo.MPQ" from "wmo.MPQ\world\...")
	std::string normalizedPath = filePath;
	std::replace(normalizedPath.begin(), normalizedPath.end(), '/', '\\');
	auto firstSep = normalizedPath.find('\\');
	if (firstSep != std::string::npos && firstSep > 0) {
		std::string prefix = normalizedPath.substr(0, firstSep);
		std::string lowerPrefix = toLower(prefix);
		if (lowerPrefix.size() >= 4 && lowerPrefix.substr(lowerPrefix.size() - 4) == ".mpq")
			mpqPrefix = prefix;
	}
}

void WMOLegacyExporter::setGroupMask(const std::vector<WMOGroupMaskEntry>& mask) {
	groupMask = mask;
}

void WMOLegacyExporter::setDoodadSetMask(const std::vector<WMODoodadSetMaskEntry>& mask) {
	doodadSetMask = mask;
}

WMOTextureExportResult WMOLegacyExporter::exportTextures(
	const std::filesystem::path& out,
	MTLWriter* mtl,
	casc::ExportHelper* helper)
{
	const auto& config = core::view->config;
	const auto outDir = out.parent_path();

	WMOTextureExportResult result;

	if (!config.value("modelsExportTextures", false))
		return result;

	wmo->load();

	const bool useAlpha = config.value("modelsExportAlpha", false);
	const bool usePosix = config.value("pathFormat", std::string("")) == "posix";
	const size_t materialCount = wmo->materials.size();

	if (helper)
		helper->setCurrentTaskMax(static_cast<int>(materialCount));

	for (size_t i = 0; i < materialCount; i++) {
		if (helper && helper->isCancelled())
			return result;

		const auto& material = wmo->materials[i];
		if (helper)
			helper->setCurrentTaskValue(static_cast<int>(i));

		const uint32_t materialTextures[] = { material.texture1, material.texture2, material.texture3 };

		for (const auto materialTexture : materialTextures) {
			// Do not skip materialTexture == 0 here — MOTX offset 0 is a valid texture entry.
			auto texNameIt = wmo->textureNames.find(materialTexture);
			if (texNameIt == wmo->textureNames.end() || texNameIt->second.empty())
				continue;

			const std::string& texturePath = texNameIt->second;

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

				std::string matName = "mat_" + std::filesystem::path(toLower(texturePath)).stem().string();
				if (config.value("removePathSpaces", false)) {
					std::erase_if(matName, [](char c) { return std::isspace(static_cast<unsigned char>(c)); });
				}

				const bool fileExisted = generics::fileExists(texPath);

				if (config.value("overwriteFiles", true) || !fileExisted) {
					auto buf = BufferWrapper::from(std::span<const uint8_t>(textureData.value()));
					casc::BLPImage blp(buf);
					blp.saveToPNG(texPath, useAlpha ? 0b1111 : 0b0111);

					logging::write(std::format("Exported legacy WMO texture: {}", texPath.string()));
				} else {
					logging::write(std::format("Skipping WMO texture export {} (file exists, overwrite disabled)", texPath.string()));
				}

				if (usePosix)
					texFile = casc::ExportHelper::win32ToPosix(texFile);

				if (mtl)
					mtl->addMaterial(matName, texFile);

				result.textureMap[materialTexture] = { texFile, texPath, matName };

				if (result.materialMap.find(static_cast<int>(i)) == result.materialMap.end())
					result.materialMap[static_cast<int>(i)] = matName;
			} catch (const std::exception& e) {
				logging::write(std::format("Failed to export texture {} for WMO: {}", texturePath, e.what()));
			}
		}
	}

	return result;
}

void WMOLegacyExporter::exportAsOBJ(
	const std::filesystem::path& out,
	casc::ExportHelper* helper,
	std::vector<FileManifestEntry>* fileManifest)
{
	const auto& config = core::view->config;

	OBJWriter obj(out);
	auto mtlPath = casc::ExportHelper::replaceExtension(out.string(), ".mtl");
	MTLWriter mtl(mtlPath);

	const auto wmoName = out.stem().string();
	obj.setName(wmoName);

	logging::write(std::format("Exporting legacy WMO model {} as OBJ: {}", wmoName, out.string()));

	data.seek(0);
	wmo = std::make_unique<WMOLegacyLoader>(data, filePath, false);
	wmo->load();

	const auto outDir = out.parent_path();

	if (helper)
		helper->setCurrentTaskName(wmoName + " textures");

	auto texMaps = exportTextures(out, &mtl, helper);

	if (helper && helper->isCancelled())
		return;

	const auto& materialMap = texMaps.materialMap;
	const auto& textureMap = texMaps.textureMap;

	for (const auto& [texOffset, texInfo] : textureMap) {
		if (fileManifest)
			fileManifest->push_back({ "PNG", texInfo.matPath });
	}

	std::vector<WMOLegacyLoader*> groups;
	size_t nInd = 0;
	size_t maxLayerCount = 0;

	std::set<uint32_t> mask;
	bool hasMask = !groupMask.empty();
	if (hasMask) {
		for (const auto& group : groupMask) {
			if (group.checked)
				mask.insert(group.groupIndex);
		}
	}

	if (helper)
		helper->setCurrentTaskName(wmoName + " groups");
	if (helper)
		helper->setCurrentTaskMax(static_cast<int>(wmo->groupCount));

	for (uint32_t i = 0, n = wmo->groupCount; i < n; i++) {
		if (helper && helper->isCancelled())
			return;

		if (helper)
			helper->setCurrentTaskValue(static_cast<int>(i));

		auto& group = wmo->getGroup(i);

		if (group.renderBatches.empty())
			continue;

		if (hasMask && mask.find(i) == mask.end())
			continue;

		nInd += group.vertices.size() / 3;
		maxLayerCount = std::max(group.uvs.size(), maxLayerCount);

		groups.push_back(&group);
	}

	if (!config.value("modelsExportUV2", false))
		maxLayerCount = std::min(maxLayerCount, static_cast<size_t>(1));

	std::vector<float> vertsArray(nInd * 3);
	std::vector<float> normalsArray(nInd * 3);
	std::vector<std::vector<float>> uvArrays(maxLayerCount);

	for (size_t i = 0; i < maxLayerCount; i++)
		uvArrays[i].resize(nInd * 2, 0.0f);

	size_t indOfs = 0;
	for (size_t gIdx = 0; gIdx < groups.size(); gIdx++) {
		const auto* group = groups[gIdx];
		const size_t indCount = group->vertices.size() / 3;

		const size_t vertOfs = indOfs * 3;
		const auto& groupVerts = group->vertices;
		for (size_t i = 0, n = groupVerts.size(); i < n; i++)
			vertsArray[vertOfs + i] = groupVerts[i];

		const auto& groupNormals = group->normals;
		for (size_t i = 0, n = groupNormals.size(); i < n; i++)
			normalsArray[vertOfs + i] = groupNormals[i];

		const size_t uvsOfs = indOfs * 2;
		const auto& groupUVs = group->uvs;
		const size_t uvCount = indCount * 2;

		for (size_t i = 0; i < maxLayerCount; i++) {
			if (i < groupUVs.size()) {
				const auto& uv = groupUVs[i];
				for (size_t j = 0; j < uvCount; j++)
					uvArrays[i][uvsOfs + j] = (j < uv.size()) ? uv[j] : 0.0f;
			}
			// else: already initialized to 0.0f
		}

		auto groupNameIt = wmo->groupNames.find(group->nameOfs);
		std::string groupName = (groupNameIt != wmo->groupNames.end())
			? groupNameIt->second
			: ("group_" + std::to_string(gIdx));

		for (size_t bI = 0, bC = group->renderBatches.size(); bI < bC; bI++) {
			const auto& batch = group->renderBatches[bI];
			std::vector<uint32_t> indices(batch.numFaces);

			for (uint16_t i = 0; i < batch.numFaces; i++)
				indices[i] = group->indices[batch.firstFace + i] + static_cast<uint32_t>(indOfs);

			const int matID = ((batch.flags & 2) == 2) ? static_cast<int>(batch.possibleBox2[2]) : static_cast<int>(batch.materialID);
			auto matIt = materialMap.find(matID);
			std::string matName = (matIt != materialMap.end()) ? matIt->second : "";
			obj.addMesh(groupName + std::to_string(bI), indices, matName);
		}

		indOfs += indCount;
	}

	obj.setVertArray(vertsArray);
	obj.setNormalArray(normalsArray);

	for (const auto& arr : uvArrays)
		obj.addUVArray(arr);

	// doodad placement csv
	auto csvPath = casc::ExportHelper::replaceExtension(out.string(), "_ModelPlacementInformation.csv");
	if (config.value("overwriteFiles", true) || !generics::fileExists(csvPath)) {
		const bool useAbsolute = config.value("enableAbsoluteCSVPaths", false);
		const bool usePosix = config.value("pathFormat", std::string("")) == "posix";
		CSVWriter csv(csvPath);
		csv.addField({"ModelFile", "PositionX", "PositionY", "PositionZ", "RotationW", "RotationX", "RotationY", "RotationZ", "ScaleFactor", "DoodadSet"});

		const auto& doodadSets = wmo->doodadSets;
		for (size_t i = 0, n = doodadSets.size(); i < n; i++) {
			if (i >= doodadSetMask.size() || !doodadSetMask[i].checked)
				continue;

			const auto& set = doodadSets[i];
			const uint32_t count = set.doodadCount;
			logging::write(std::format("Exporting legacy WMO doodad set {} with {} doodads...", set.name, count));

			if (helper)
				helper->setCurrentTaskName(wmoName + ", doodad set " + set.name);
			if (helper)
				helper->setCurrentTaskMax(static_cast<int>(count));

			for (uint32_t j = 0; j < count; j++) {
				if (helper && helper->isCancelled())
					return;

				if (helper)
					helper->setCurrentTaskValue(static_cast<int>(j));

				const uint32_t doodadIdx = set.firstInstanceIndex + j;
				if (doodadIdx >= wmo->doodads.size())
					continue;

				const auto& doodad = wmo->doodads[doodadIdx];

				auto doodadNameIt = wmo->doodadNames.find(doodad.offset);
				if (doodadNameIt == wmo->doodadNames.end())
					continue;

				const std::string& fileName = doodadNameIt->second;
				if (fileName.empty())
					continue;

				try {
					std::string objFileName = casc::ExportHelper::replaceExtension(fileName, ".obj");

					// prepend mpq prefix for consistent export paths
					std::string prefixedObjFileName = !mpqPrefix.empty()
						? (std::filesystem::path(mpqPrefix) / objFileName).string()
						: objFileName;
					std::string prefixedFileName = !mpqPrefix.empty()
						? (std::filesystem::path(mpqPrefix) / fileName).string()
						: fileName;

					std::string m2Path;
					if (config.value("enableSharedChildren", false))
						m2Path = casc::ExportHelper::getExportPath(prefixedObjFileName);
					else
						m2Path = casc::ExportHelper::replaceFile(out.string(), objFileName);

					if (doodadCache.find(toLower(fileName)) == doodadCache.end()) {
						auto m2Data = mpq->getFile(fileName);
						if (m2Data) {
							auto buf = BufferWrapper::from(std::span<const uint8_t>(m2Data.value()));
							M2LegacyExporter m2Export(std::move(buf), prefixedFileName, mpq);
							m2Export.exportAsOBJ(std::filesystem::path(m2Path), helper, nullptr);

							if (helper && helper->isCancelled())
								return;

							doodadCache.insert(toLower(fileName));
						}
					}

					std::string modelPath = std::filesystem::relative(
						std::filesystem::path(m2Path), outDir).string();

					if (useAbsolute)
						modelPath = std::filesystem::weakly_canonical(
							std::filesystem::path(outDir) / modelPath).string();

					if (usePosix)
						modelPath = casc::ExportHelper::win32ToPosix(modelPath);

					csv.addRow({
						{ "ModelFile", modelPath },
						{ "PositionX", std::format("{:g}", doodad.position[0]) },
						{ "PositionY", std::format("{:g}", doodad.position[1]) },
						{ "PositionZ", std::format("{:g}", doodad.position[2]) },
						{ "RotationW", std::format("{:g}", doodad.rotation[3]) },
						{ "RotationX", std::format("{:g}", doodad.rotation[0]) },
						{ "RotationY", std::format("{:g}", doodad.rotation[1]) },
						{ "RotationZ", std::format("{:g}", doodad.rotation[2]) },
						{ "ScaleFactor", std::format("{:g}", doodad.scale) },
						{ "DoodadSet", set.name }
					});
				} catch (const std::exception& e) {
					logging::write(std::format("Failed to export doodad {} for {}: {}", fileName, set.name, e.what()));
				}
			}
		}

		csv.write();
		if (fileManifest)
			fileManifest->push_back({ "PLACEMENT", std::filesystem::path(csvPath) });
	} else {
		logging::write(std::format("Skipping model placement export {} (file exists, overwrite disabled)", csvPath));
	}

	if (!mtl.isEmpty())
		obj.setMaterialLibrary(std::filesystem::path(mtlPath).filename().string());

	obj.write(config.value("overwriteFiles", true));
	if (fileManifest)
		fileManifest->push_back({ "OBJ", out });

	mtl.write(config.value("overwriteFiles", true));
	if (fileManifest)
		fileManifest->push_back({ "MTL", std::filesystem::path(mtlPath) });

	if (config.value("exportWMOMeta", false)) {
		if (helper)
			helper->clearCurrentTask();
		if (helper)
			helper->setCurrentTaskName(wmoName + ", writing meta data");

		auto jsonPath = casc::ExportHelper::replaceExtension(out.string(), ".json");
		JSONWriter json(jsonPath);
		json.addProperty("fileType", "wmo");
		json.addProperty("filePath", filePath);
		json.addProperty("version", wmo->version);
		json.addProperty("counts", nlohmann::json{
			{ "material", wmo->materialCount },
			{ "group", wmo->groupCount },
			{ "portal", wmo->portalCount },
			{ "light", wmo->lightCount },
			{ "model", wmo->modelCount },
			{ "doodad", wmo->doodadCount },
			{ "set", wmo->setCount }
		});

		json.addProperty("ambientColor", wmo->ambientColor);
		json.addProperty("wmoID", wmo->wmoID);
		json.addProperty("boundingBox1", nlohmann::json(wmo->boundingBox1));
		json.addProperty("boundingBox2", nlohmann::json(wmo->boundingBox2));
		json.addProperty("flags", wmo->flags);

		// groupNames: Object.values(wmo.groupNames)
		nlohmann::json groupNamesArr = nlohmann::json::array();
		for (const auto& [key, val] : wmo->groupNames)
			groupNamesArr.push_back(val);
		json.addProperty("groupNames", groupNamesArr);

		// groupInfo
		nlohmann::json groupInfoArr = nlohmann::json::array();
		for (const auto& gi : wmo->groupInfo) {
			nlohmann::json giObj;
			giObj["flags"] = gi.flags;
			giObj["boundingBox1"] = gi.boundingBox1;
			giObj["boundingBox2"] = gi.boundingBox2;
			giObj["nameIndex"] = gi.nameIndex;
			groupInfoArr.push_back(giObj);
		}
		json.addProperty("groupInfo", groupInfoArr);

		// materials
		nlohmann::json materialsArr = nlohmann::json::array();
		for (const auto& mat : wmo->materials) {
			nlohmann::json matObj;
			matObj["flags"] = mat.flags;
			matObj["shader"] = mat.shader;
			matObj["blendMode"] = mat.blendMode;
			matObj["texture1"] = mat.texture1;
			matObj["color1"] = mat.color1;
			matObj["color1b"] = mat.color1b;
			matObj["texture2"] = mat.texture2;
			matObj["color2"] = mat.color2;
			matObj["groupType"] = mat.groupType;
			matObj["texture3"] = mat.texture3;
			matObj["color3"] = mat.color3;
			matObj["flags3"] = mat.flags3;
			matObj["runtimeData"] = mat.runtimeData;
			materialsArr.push_back(matObj);
		}
		json.addProperty("materials", materialsArr);

		// doodadSets
		nlohmann::json doodadSetsArr = nlohmann::json::array();
		for (const auto& ds : wmo->doodadSets) {
			nlohmann::json dsObj;
			dsObj["name"] = ds.name;
			dsObj["firstInstanceIndex"] = ds.firstInstanceIndex;
			dsObj["doodadCount"] = ds.doodadCount;
			dsObj["unused"] = ds.unused;
			doodadSetsArr.push_back(dsObj);
		}
		json.addProperty("doodadSets", doodadSetsArr);

		// doodads
		nlohmann::json doodadsArr = nlohmann::json::array();
		for (const auto& dd : wmo->doodads) {
			nlohmann::json ddObj;
			ddObj["offset"] = dd.offset;
			ddObj["flags"] = dd.flags;
			ddObj["position"] = dd.position;
			ddObj["rotation"] = dd.rotation;
			ddObj["scale"] = dd.scale;
			ddObj["color"] = dd.color;
			doodadsArr.push_back(ddObj);
		}
		json.addProperty("doodads", doodadsArr);

		json.write(config.value("overwriteFiles", true));
		if (fileManifest)
			fileManifest->push_back({ "META", std::filesystem::path(jsonPath) });
	}
}

void WMOLegacyExporter::exportAsSTL(
	const std::filesystem::path& out,
	casc::ExportHelper* helper,
	std::vector<FileManifestEntry>* fileManifest)
{
	const auto& config = core::view->config;
	STLWriter stl(out);

	const auto wmoName = out.stem().string();
	stl.setName(wmoName);

	logging::write(std::format("Exporting legacy WMO model {} as STL: {}", wmoName, out.string()));

	data.seek(0);
	wmo = std::make_unique<WMOLegacyLoader>(data, filePath, false);
	wmo->load();

	std::vector<WMOLegacyLoader*> groupPtrs;
	size_t nInd = 0;

	std::set<uint32_t> mask;
	bool hasMask = !groupMask.empty();
	if (hasMask) {
		for (const auto& group : groupMask) {
			if (group.checked)
				mask.insert(group.groupIndex);
		}
	}

	if (helper)
		helper->setCurrentTaskName(wmoName + " groups");
	if (helper)
		helper->setCurrentTaskMax(static_cast<int>(wmo->groupCount));

	for (uint32_t i = 0, n = wmo->groupCount; i < n; i++) {
		if (helper && helper->isCancelled())
			return;

		if (helper)
			helper->setCurrentTaskValue(static_cast<int>(i));

		auto& group = wmo->getGroup(i);

		if (group.renderBatches.empty())
			continue;

		if (hasMask && mask.find(i) == mask.end())
			continue;

		nInd += group.vertices.size() / 3;
		groupPtrs.push_back(&group);
	}

	std::vector<float> vertsArray(nInd * 3);
	std::vector<float> normalsArray(nInd * 3);

	size_t indOfs = 0;
	for (size_t gIdx = 0; gIdx < groupPtrs.size(); gIdx++) {
		const auto* group = groupPtrs[gIdx];
		const size_t indCount = group->vertices.size() / 3;

		const size_t vertOfs = indOfs * 3;
		const auto& groupVerts = group->vertices;
		for (size_t i = 0, n = groupVerts.size(); i < n; i++)
			vertsArray[vertOfs + i] = groupVerts[i];

		const auto& groupNormals = group->normals;
		for (size_t i = 0, n = groupNormals.size(); i < n; i++)
			normalsArray[vertOfs + i] = groupNormals[i];

		auto groupNameIt = wmo->groupNames.find(group->nameOfs);
		std::string groupName = (groupNameIt != wmo->groupNames.end())
			? groupNameIt->second
			: ("group_" + std::to_string(gIdx));

		for (size_t bI = 0, bC = group->renderBatches.size(); bI < bC; bI++) {
			const auto& batch = group->renderBatches[bI];
			std::vector<uint32_t> indices(batch.numFaces);

			for (uint16_t i = 0; i < batch.numFaces; i++)
				indices[i] = group->indices[batch.firstFace + i] + static_cast<uint32_t>(indOfs);

			stl.addMesh(groupName + std::to_string(bI), indices);
		}

		indOfs += indCount;
	}

	stl.setVertArray(vertsArray);
	stl.setNormalArray(normalsArray);

	stl.write(config.value("overwriteFiles", true));
	if (fileManifest)
		fileManifest->push_back({ "STL", out });
}

void WMOLegacyExporter::exportRaw(
	const std::filesystem::path& out,
	casc::ExportHelper* helper,
	std::vector<FileManifestEntry>* fileManifest)
{
	const auto& config = core::view->config;
	const auto outDir = out.parent_path();

	auto manifestFile = casc::ExportHelper::replaceExtension(out.string(), ".manifest.json");
	JSONWriter manifest(manifestFile);

	manifest.addProperty("filePath", filePath);

	// write main wmo root file
	data.writeToFile(out);
	if (fileManifest)
		fileManifest->push_back({ "WMO", out });

	logging::write(std::format("Exported legacy WMO root: {}", out.string()));

	// load wmo for parsing related files
	data.seek(0);
	wmo = std::make_unique<WMOLegacyLoader>(data, filePath, true);
	wmo->load();

	// export textures
	if (config.value("modelsExportTextures", false)) {
		nlohmann::json texturesManifest = nlohmann::json::array();
		std::set<std::string> exportedTextures;

		const auto& textureNames = wmo->textureNames;

		for (const auto& [key, texturePath] : textureNames) {
			if (texturePath.empty())
				continue;

			// skip duplicates
			if (exportedTextures.count(toLower(texturePath)))
				continue;

			exportedTextures.insert(toLower(texturePath));

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
					{ "path", texturePath }
				});
				if (fileManifest)
					fileManifest->push_back({ "BLP", texOut });

				logging::write(std::format("Exported legacy WMO texture: {}", texOut.string()));
			} catch (const std::exception& e) {
				logging::write(std::format("Failed to export WMO texture {}: {}", texturePath, e.what()));
			}
		}

		manifest.addProperty("textures", texturesManifest);
	}

	// export wmo group files
	if (config.value("modelsExportWMOGroups", false) && wmo->groupCount > 0) {
		nlohmann::json groupsManifest = nlohmann::json::array();

		for (uint32_t i = 0; i < wmo->groupCount; i++) {
			if (helper && helper->isCancelled())
				return;

			// Replace .wmo with _NNN.wmo
			std::ostringstream oss;
			oss << std::setfill('0') << std::setw(3) << i;
			std::string groupFileName = filePath;
			auto wmoPos = groupFileName.find(".wmo");
			if (wmoPos != std::string::npos)
				groupFileName = groupFileName.substr(0, wmoPos) + "_" + oss.str() + ".wmo";

			try {
				auto groupData = mpq->getFile(groupFileName);
				if (!groupData) {
					logging::write(std::format("WMO group file not found: {}", groupFileName));
					continue;
				}

				std::filesystem::path groupOut;
				if (config.value("enableSharedChildren", false))
					groupOut = casc::ExportHelper::getExportPath(groupFileName);
				else
					groupOut = outDir / std::filesystem::path(groupFileName).filename();

				auto buf = BufferWrapper::from(std::span<const uint8_t>(groupData.value()));
				buf.writeToFile(groupOut);

				groupsManifest.push_back({
					{ "file", std::filesystem::relative(groupOut, outDir).string() },
					{ "path", groupFileName },
					{ "index", i }
				});
				if (fileManifest)
					fileManifest->push_back({ "WMO_GROUP", groupOut });

				logging::write(std::format("Exported legacy WMO group: {}", groupOut.string()));
			} catch (const std::exception& e) {
				logging::write(std::format("Failed to export WMO group {}: {}", groupFileName, e.what()));
			}
		}

		manifest.addProperty("groups", groupsManifest);
	}

	manifest.write();
	if (fileManifest)
		fileManifest->push_back({ "MANIFEST", std::filesystem::path(manifestFile) });
}

void WMOLegacyExporter::clearCache() {
	doodadCache.clear();
}
