/*!
wow.export (https://github.com/Kruithne/wow.export)
Authors: Kruithne <kruithne@gmail.com>, Marlamin <marlamin@marlamin.com>
License: MIT
 */

#include "M2Exporter.h"

#include <algorithm>
#include <cctype>
#include <format>
#include <set>
#include <string>

#include "../../core.h"
#include "../../log.h"
#include "../../generics.h"
#include "../../buffer.h"
#include "../../casc/blp.h"
#include "../../casc/export-helper.h"
#include "../../casc/listfile.h"
#include "../../casc/casc-source.h"
#include "../../wow/EquipmentSlots.h"
#include "../loaders/M2Loader.h"
#include "../loaders/SKELLoader.h"
#include "../Skin.h"
#include "../Texture.h"
#include "../writers/JSONWriter.h"
#include "../writers/OBJWriter.h"
#include "../writers/MTLWriter.h"
#include "../writers/STLWriter.h"
#include "../writers/GLTFWriter.h"
#include "../GeosetMapper.h"
#include "../renderers/M2RendererGL.h"

namespace {

/**
 * Convert M2Track (using M2Value variants) to GLTFAnimTrack (concrete types).
 * Timestamps are expected to hold uint32_t, values hold vector<float>.
 */
GLTFAnimTrack convertM2TrackToGLTFAnimTrack(const M2Track& track) {
	GLTFAnimTrack result;
	result.interpolation = track.interpolation;

	// timestamps: M2Value (uint32_t) -> uint32_t
	result.timestamps.resize(track.timestamps.size());
	for (size_t i = 0; i < track.timestamps.size(); i++) {
		result.timestamps[i].reserve(track.timestamps[i].size());
		for (const auto& val : track.timestamps[i])
			result.timestamps[i].push_back(std::get<uint32_t>(val));
	}

	// values: M2Value (vector<float>) -> vector<float>
	result.values.resize(track.values.size());
	for (size_t i = 0; i < track.values.size(); i++) {
		result.values[i].reserve(track.values[i].size());
		for (const auto& val : track.values[i])
			result.values[i].push_back(std::get<std::vector<float>>(val));
	}

	return result;
}

/**
 * Convert a bone struct (M2Bone or SKELBone) to GLTFBone.
 */
template <typename BoneT>
GLTFBone convertBoneToGLTF(const BoneT& b) {
	GLTFBone gb;
	gb.parentBone = b.parentBone;
	gb.pivot = b.pivot;
	gb.boneID = b.boneID;
	gb.boneNameCRC = b.boneNameCRC;
	gb.translation = convertM2TrackToGLTFAnimTrack(b.translation);
	gb.rotation = convertM2TrackToGLTFAnimTrack(b.rotation);
	gb.scale = convertM2TrackToGLTFAnimTrack(b.scale);
	return gb;
}

/**
 * Convert SKELBone array to GLTFBone array.
 */
std::vector<GLTFBone> convertSkelBonesToGLTF(const std::vector<SKELBone>& bones) {
	std::vector<GLTFBone> gltfBones;
	gltfBones.reserve(bones.size());
	for (const auto& b : bones)
		gltfBones.push_back(convertBoneToGLTF(b));
	return gltfBones;
}

/**
 * Convert M2Bone array to GLTFBone array.
 */
std::vector<GLTFBone> convertM2BonesToGLTF(const std::vector<M2Bone>& bones) {
	std::vector<GLTFBone> gltfBones;
	gltfBones.reserve(bones.size());
	for (const auto& b : bones)
		gltfBones.push_back(convertBoneToGLTF(b));
	return gltfBones;
}

/**
 * Convert SKELAnimation array to GLTFAnimation array.
 */
std::vector<GLTFAnimation> convertSkelAnimsToGLTF(const std::vector<SKELAnimation>& anims) {
	std::vector<GLTFAnimation> gltfAnims;
	gltfAnims.reserve(anims.size());
	for (const auto& a : anims)
		gltfAnims.push_back({ a.id, a.variationIndex, a.duration });
	return gltfAnims;
}

/**
 * Convert M2Animation array to GLTFAnimation array.
 */
std::vector<GLTFAnimation> convertM2AnimsToGLTF(const std::vector<M2Animation>& anims) {
	std::vector<GLTFAnimation> gltfAnims;
	gltfAnims.reserve(anims.size());
	for (const auto& a : anims)
		gltfAnims.push_back({ a.id, a.variationIndex, a.duration });
	return gltfAnims;
}

// --- JSON serialization helpers ---

nlohmann::json m2ValueToJson(const M2Value& v) {
	nlohmann::json j;
	std::visit([&j](const auto& val) { j = val; }, v);
	return j;
}

nlohmann::json m2TrackToJson(const M2Track& t) {
	nlohmann::json ts = nlohmann::json::array();
	for (const auto& inner : t.timestamps) {
		nlohmann::json arr = nlohmann::json::array();
		for (const auto& v : inner)
			arr.push_back(m2ValueToJson(v));
		ts.push_back(arr);
	}

	nlohmann::json vs = nlohmann::json::array();
	for (const auto& inner : t.values) {
		nlohmann::json arr = nlohmann::json::array();
		for (const auto& v : inner)
			arr.push_back(m2ValueToJson(v));
		vs.push_back(arr);
	}

	return {
		{"globalSeq", t.globalSeq},
		{"interpolation", t.interpolation},
		{"timestamps", ts},
		{"values", vs}
	};
}

template <typename BoneT>
nlohmann::json boneToJson(const BoneT& b) {
	return {
		{"boneID", b.boneID}, {"flags", b.flags},
		{"parentBone", b.parentBone}, {"subMeshID", b.subMeshID},
		{"boneNameCRC", b.boneNameCRC},
		{"translation", m2TrackToJson(b.translation)},
		{"rotation", m2TrackToJson(b.rotation)},
		{"scale", m2TrackToJson(b.scale)},
		{"pivot", b.pivot}
	};
}

template <typename BoneVec>
nlohmann::json bonesToJson(const BoneVec& bones) {
	nlohmann::json arr = nlohmann::json::array();
	for (const auto& b : bones)
		arr.push_back(boneToJson(b));
	return arr;
}

nlohmann::json uint8VecToJsonArray(const std::vector<uint8_t>& vec) {
	nlohmann::json arr = nlohmann::json::array();
	for (auto v : vec)
		arr.push_back(static_cast<unsigned>(v));
	return arr;
}

nlohmann::json caabbToJson(const CAaBB& bb) {
	return {{"min", bb.min}, {"max", bb.max}};
}

nlohmann::json m2MaterialsToJson(const std::vector<M2Material>& mats) {
	nlohmann::json arr = nlohmann::json::array();
	for (const auto& m : mats)
		arr.push_back(nlohmann::json{{"flags", m.flags}, {"blendingMode", m.blendingMode}});
	return arr;
}

nlohmann::json m2AnimFileIDsToJson(const std::vector<M2AnimFileEntry>& entries) {
	nlohmann::json arr = nlohmann::json::array();
	for (const auto& e : entries)
		arr.push_back(nlohmann::json{{"animID", e.animID}, {"subAnimID", e.subAnimID}, {"fileDataID", e.fileDataID}});
	return arr;
}

nlohmann::json m2ColorsToJson(const std::vector<M2Color>& colors) {
	nlohmann::json arr = nlohmann::json::array();
	for (const auto& c : colors)
		arr.push_back(nlohmann::json{{"color", m2TrackToJson(c.color)}, {"alpha", m2TrackToJson(c.alpha)}});
	return arr;
}

nlohmann::json m2TracksToJson(const std::vector<M2Track>& tracks) {
	nlohmann::json arr = nlohmann::json::array();
	for (const auto& t : tracks)
		arr.push_back(m2TrackToJson(t));
	return arr;
}

nlohmann::json m2TextureTransformsToJson(const std::vector<M2TextureTransform>& transforms) {
	nlohmann::json arr = nlohmann::json::array();
	for (const auto& t : transforms)
		arr.push_back(nlohmann::json{
			{"translation", m2TrackToJson(t.translation)},
			{"rotation", m2TrackToJson(t.rotation)},
			{"scaling", m2TrackToJson(t.scaling)}
		});
	return arr;
}

} // anonymous namespace

M2Exporter::M2Exporter(BufferWrapper data, std::vector<uint32_t> variantTextures, uint32_t fileDataID, casc::CASC* casc)
: data(std::move(data))
, fileDataID(fileDataID)
, casc(casc)
, variantTextures(std::move(variantTextures))
{
m2 = std::make_unique<M2Loader>(this->data);
}

/**
 * Set the mask array used for geoset control.
 * @param mask
 */
void M2Exporter::setGeosetMask(std::vector<M2ExportGeosetMask> mask) {
geosetMask = std::move(mask);
}

/**
 * Set posed geometry to use instead of bind pose
 * @param vertices
 * @param normals
 */
void M2Exporter::setPosedGeometry(std::vector<float> vertices, std::vector<float> normals) {
posedVertices = std::move(vertices);
posedNormals = std::move(normals);
}

/**
 * Export additional texture from canvas
 */
void M2Exporter::addURITexture(uint32_t textureType, BufferWrapper pngData) {
dataTextures.emplace(textureType, std::move(pngData));
}

/**
 * Set equipment models to export alongside the main model (for OBJ/STL).
 * @param equipment
 */
void M2Exporter::setEquipmentModels(std::vector<EquipmentModel> equipment) {
equipmentModels = std::move(equipment);
}

/**
 * Set equipment models for GLTF export (with bone data for rigging).
 * @param equipment
 */
void M2Exporter::setEquipmentModelsGLTF(std::vector<EquipmentModelGLTF> equipment) {
equipmentModelsGLTF = std::move(equipment);
}

/**
 * Export the textures for this M2 model (for GLB mode, returns buffers instead of writing).
 * @param out
 * @param raw
 * @param mtl
 * @param helper
 * @param fullTexPaths
 * @param glbMode
 * @returns Texture result
 */
M2ExportTextureResult M2Exporter::exportTextures(const std::filesystem::path& out, bool raw, MTLWriter* mtl,
casc::ExportHelper* helper, bool fullTexPaths, bool glbMode)
{
const auto& config = core::view->config;
// JS validTextures is a Map keyed by numeric fileDataID for regular textures and "data-N" strings
// for data textures. C++ uses string keys throughout (regular keys are std::to_string(fileDataID))
// for type uniformity; downstream consumers stringify their lookups accordingly.
M2ExportTextureResult result;

if (!config.value("modelsExportTextures", false))
return result;

m2->load().get();

const bool useAlpha = config.value("modelsExportAlpha", false);
const bool usePosix = config.value("pathFormat", std::string("")) == "posix";

uint32_t textureIndex = 0;

// Export data textures first.
for (const auto& [textureName, dataTexture] : dataTextures) {
try {
std::string texFile = "data-" + std::to_string(textureName) + ".png";
auto texPath = out / texFile;
const std::string matName = "mat_" + std::to_string(textureName);
// In JS, dataTexture is a base64 data URI that gets decoded via
// BufferWrapper.fromBase64(). In C++, addURITexture stores pre-decoded PNG data directly.
auto dataCopy = dataTexture;

if (glbMode) {
result.texture_buffers.emplace("data-" + std::to_string(textureName), dataCopy);
logging::write(std::format("Buffering data texture {} for GLB embedding", textureName));
} else if (config.value("overwriteFiles", true) || !generics::fileExists(texPath)) {
logging::write(std::format("Exporting data texture {} -> {}", textureName, texPath.string()));
dataCopy.writeToFile(texPath);
} else {
logging::write(std::format("Skipping data texture export {} (file exists, overwrite disabled)", texPath.string()));
}

if (usePosix)
texFile = casc::ExportHelper::win32ToPosix(texFile);

if (mtl)
mtl->addMaterial(matName, texFile);

result.validTextures["data-" + std::to_string(textureName)] = {
fullTexPaths ? texFile : matName,
texFile,
texPath
};
} catch (const std::exception& e) {
logging::write(std::format("Failed to export data texture {} for M2: {}", textureName, e.what()));
}

textureIndex++;
}

textureIndex = 0;
for (auto& texture : m2->textures) {
// Abort if the export has been cancelled.
if (helper && helper->isCancelled())
return result;

const uint32_t textureType = m2->textureTypes[textureIndex];
uint32_t texFileDataID = texture.fileDataID;
bool isDataTexture = false;

//TODO: Use m2.materials[texUnit.materialIndex].flags & 0x4 to determine if it's double sided

if (textureType > 0) {
uint32_t targetFileDataID = 0;

if (dataTextures.count(textureType)) {
// Not a fileDataID, but a data texture. JS backward-patches
// texture.fileDataID = 'data-' + textureType; C++ Texture::fileDataID is uint32_t
// so consumers locate data textures via the validTextures "data-N" key directly.
isDataTexture = true;
} else if (textureType >= 11 && textureType < 14) {
// Creature textures.
uint32_t idx = textureType - 11;
if (idx < variantTextures.size())
targetFileDataID = variantTextures[idx];
} else if (textureType > 1 && textureType < 5) {
uint32_t idx = textureType - 2;
if (idx < variantTextures.size())
targetFileDataID = variantTextures[idx];
}

if (!isDataTexture) {
texFileDataID = targetFileDataID;

// Backward patch the variant texture into the M2 instance so that
// the MTL exports with the correct texture once we swap it here.
texture.fileDataID = targetFileDataID;
}
}

if (!isDataTexture && texFileDataID > 0) {
try {
std::string texFile = std::to_string(texFileDataID) + (raw ? ".blp" : ".png");
auto texPath = out / texFile;

// Default MTL name to the file ID (prefixed for Maya).
std::string matName = "mat_" + std::to_string(texFileDataID);
std::string fileName = casc::listfile::getByID(texFileDataID).value_or("");

if (!fileName.empty()) {
std::string baseName = std::filesystem::path(fileName).stem().string();
std::transform(baseName.begin(), baseName.end(), baseName.begin(),
[](unsigned char c) { return std::tolower(c); });
matName = "mat_" + baseName;

// Remove spaces from material name for MTL compatibility.
if (config.value("removePathSpaces", false))
std::erase_if(matName, [](char c) { return std::isspace(static_cast<unsigned char>(c)); });
}

// Map texture files relative to its own path.
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
texFile = std::filesystem::relative(texPath, out).string();
}

const bool file_existed = generics::fileExists(texPath);

if (glbMode && !raw) {
// glb mode: convert to PNG buffer without writing
auto fileData = casc->getVirtualFileByID(texFileDataID);
casc::BLPImage blp(fileData);
auto png_buffer = blp.toPNG(useAlpha ? 0b1111 : 0b0111);
result.texture_buffers.emplace(std::to_string(texFileDataID), std::move(png_buffer));
logging::write(std::format("Buffering M2 texture {} for GLB embedding", texFileDataID));

if (!file_existed)
result.files_to_cleanup.push_back(texPath);
} else if (config.value("overwriteFiles", true) || !file_existed) {
auto fileData = casc->getVirtualFileByID(texFileDataID);
logging::write(std::format("Exporting M2 texture {} -> {}", texFileDataID, texPath.string()));

if (raw) {
// write raw BLP files
fileData.writeToFile(texPath);
} else {
// convert BLP to PNG
casc::BLPImage blp(fileData);
blp.saveToPNG(texPath, useAlpha ? 0b1111 : 0b0111);
}
} else {
logging::write(std::format("Skipping M2 texture export {} (file exists, overwrite disabled)", texPath.string()));
}

if (usePosix)
texFile = casc::ExportHelper::win32ToPosix(texFile);

if (mtl)
mtl->addMaterial(matName, texFile);

result.validTextures[std::to_string(texFileDataID)] = {
fullTexPaths ? texFile : matName,
texFile,
texPath
};
} catch (const std::exception& e) {
logging::write(std::format("Failed to export texture {} for M2: {}", texFileDataID, e.what()));
}
}

textureIndex++;
}

return result;
}

void M2Exporter::exportAsGLTF(const std::filesystem::path& out, casc::ExportHelper* helper, const std::string& format) {
const std::string ext = format == "glb" ? ".glb" : ".gltf";
const auto outGLTF = casc::ExportHelper::replaceExtension(out.string(), ext);
const auto outDir = out.parent_path();

// Skip export if file exists and overwriting is disabled.
if (!core::view->config.value("overwriteFiles", true) && generics::fileExists(outGLTF)) {
std::string formatUpper = format;
std::transform(formatUpper.begin(), formatUpper.end(), formatUpper.begin(), ::toupper);
logging::write(std::format("Skipping {} export of {} (already exists, overwrite disabled)", formatUpper, outGLTF));
return;
}

m2->load().get();
auto& skin = *m2->getSkin(0).get();

const auto model_name = std::filesystem::path(outGLTF).stem().string();
GLTFWriter gltf(out, model_name);
{
std::string formatUpper = format;
std::transform(formatUpper.begin(), formatUpper.end(), formatUpper.begin(), ::toupper);
logging::write(std::format("Exporting M2 model {} as {}: {}", model_name, formatUpper, outGLTF));
}

if (m2->skeletonFileID) {
auto skel_file = casc->getVirtualFileByID(m2->skeletonFileID);
SKELLoader skel(skel_file);

skel.load();

if (skel.parent_skel_file_id > 0) {
auto parent_skel_file = casc->getVirtualFileByID(skel.parent_skel_file_id);
SKELLoader parent_skel(parent_skel_file);
parent_skel.load();

if (core::view->config.value("modelsExportAnimations", false)) {
// load each skeleton's .anim files independently
// each skeleton's bone offsets match its own .anim data
parent_skel.loadAnims();
skel.loadAnims();

// identify which animations come from the child skeleton
std::set<std::string> child_anim_keys;
for (const auto& entry : skel.animFileIDs) {
if (entry.fileDataID > 0)
child_anim_keys.insert(std::to_string(entry.animID) + "-" + std::to_string(entry.subAnimID));
}

// copy child bone animation data into parent bones for child-specific animations
const size_t bone_count = std::min(parent_skel.bones.size(), skel.bones.size());
for (size_t i = 0; i < parent_skel.animations.size(); i++) {
const auto& anim = parent_skel.animations[i];
std::string animKey = std::to_string(anim.id) + "-" + std::to_string(anim.variationIndex);
if (!child_anim_keys.count(animKey))
continue;

// find matching animation index in child skeleton
int child_idx = -1;
for (size_t j = 0; j < skel.animations.size(); j++) {
if (skel.animations[j].id == anim.id && skel.animations[j].variationIndex == anim.variationIndex) {
child_idx = static_cast<int>(j);
break;
}
}

if (child_idx < 0)
continue;

// copy decoded keyframe data from child bones into parent bones
for (size_t bi = 0; bi < bone_count; bi++) {
auto& pb = parent_skel.bones[bi];
const auto& cb = skel.bones[bi];

// translation
if (static_cast<size_t>(child_idx) < cb.translation.timestamps.size() &&
!cb.translation.timestamps[child_idx].empty()) {
if (i >= pb.translation.timestamps.size()) {
pb.translation.timestamps.resize(i + 1);
pb.translation.values.resize(i + 1);
}
pb.translation.timestamps[i] = cb.translation.timestamps[child_idx];
pb.translation.values[i] = cb.translation.values[child_idx];
}

// rotation
if (static_cast<size_t>(child_idx) < cb.rotation.timestamps.size() &&
!cb.rotation.timestamps[child_idx].empty()) {
if (i >= pb.rotation.timestamps.size()) {
pb.rotation.timestamps.resize(i + 1);
pb.rotation.values.resize(i + 1);
}
pb.rotation.timestamps[i] = cb.rotation.timestamps[child_idx];
pb.rotation.values[i] = cb.rotation.values[child_idx];
}

// scale
if (static_cast<size_t>(child_idx) < cb.scale.timestamps.size() &&
!cb.scale.timestamps[child_idx].empty()) {
if (i >= pb.scale.timestamps.size()) {
pb.scale.timestamps.resize(i + 1);
pb.scale.values.resize(i + 1);
}
pb.scale.timestamps[i] = cb.scale.timestamps[child_idx];
pb.scale.values[i] = cb.scale.values[child_idx];
}
}
}

gltf.setAnimations(convertSkelAnimsToGLTF(parent_skel.animations));
}

gltf.setBonesArray(convertSkelBonesToGLTF(parent_skel.bones));
} else {
if (core::view->config.value("modelsExportAnimations", false)) {
skel.loadAnims();
gltf.setAnimations(convertSkelAnimsToGLTF(skel.animations));
}

gltf.setBonesArray(convertSkelBonesToGLTF(skel.bones));
}

} else {
if (core::view->config.value("modelsExportAnimations", false)) {
m2->loadAnims().get();
gltf.setAnimations(convertM2AnimsToGLTF(m2->animations));
}

gltf.setBonesArray(convertM2BonesToGLTF(m2->bones));
}

gltf.setVerticesArray(m2->vertices);
gltf.setNormalArray(m2->normals);
gltf.setBoneWeightArray(m2->boneWeights);
gltf.setBoneIndexArray(m2->boneIndices);

gltf.addUVArray(m2->uv);
gltf.addUVArray(m2->uv2);

std::map<std::string, M2TextureExportInfo> textureMap;
if (format == "glb") {
auto texResult = exportTextures(outDir, false, nullptr, helper, true, true);
textureMap = std::move(texResult.validTextures);
gltf.setTextureBuffers(std::move(texResult.texture_buffers));
} else {
auto texResult = exportTextures(outDir, false, nullptr, helper, true, false);
textureMap = std::move(texResult.validTextures);
}

// Build string-keyed GLTF texture map (preserves "data-N" keys for data textures).
std::map<std::string, GLTFTextureEntry> gltfTexMap;
for (const auto& [key, info] : textureMap)
gltfTexMap[key] = { info.matPathRelative, info.matName };
gltf.setTextureMap(gltfTexMap);

for (size_t mI = 0, mC = skin.subMeshes.size(); mI < mC; mI++) {
// Skip geosets that are not enabled.
if (!geosetMask.empty() && (mI >= geosetMask.size() || !geosetMask[mI].checked))
continue;

const auto& mesh = skin.subMeshes[mI];
std::vector<uint32_t> indices(mesh.triangleCount);
for (uint16_t vI = 0; vI < mesh.triangleCount; vI++)
indices[vI] = skin.indices[skin.triangles[mesh.triangleStart + vI]];

// Find texture for this submesh
std::string matName;
auto texUnitIt = std::find_if(skin.textureUnits.begin(), skin.textureUnits.end(),
[mI](const TextureUnit& tex) { return tex.skinSectionIndex == static_cast<uint16_t>(mI); });

if (texUnitIt != skin.textureUnits.end()) {
const auto& texture = m2->textures[m2->textureCombos[texUnitIt->textureComboIndex]];

if (texture.fileDataID > 0) {
auto it = textureMap.find(std::to_string(texture.fileDataID));
if (it != textureMap.end())
matName = it->second.matName;
}

uint32_t texType = m2->textureTypes[m2->textureCombos[texUnitIt->textureComboIndex]];
if (dataTextures.count(texType)) {
std::string dataTextureKey = "data-" + std::to_string(texType);
auto it = textureMap.find(dataTextureKey);
if (it != textureMap.end())
matName = it->second.matName;
}
}

gltf.addMesh(geoset_mapper::getGeosetName(static_cast<int>(mI), mesh.submeshID), indices, matName);
}

// add equipment models for GLTF export
if (!equipmentModelsGLTF.empty()) {
for (const auto& equip : equipmentModelsGLTF) {
_addEquipmentToGLTF(gltf, equip, textureMap, outDir, format, helper);
}
}

gltf.write(core::view->config.value("overwriteFiles", true), format);
}

/**
 * Add equipment model to GLTF writer.
 * @private
 */
void M2Exporter::_addEquipmentToGLTF(GLTFWriter& gltf, const EquipmentModelGLTF& equip,
std::map<std::string, M2TextureExportInfo>& textureMap,
const std::filesystem::path& outDir, const std::string& format, casc::ExportHelper* helper)
{
const auto& [slot_id, item_id, renderer, vertices, normals, uv, uv2, boneIndices, boneWeights, textures, is_collection_style] = equip;

if (!renderer || !renderer->m2)
return;

auto& equipM2 = *renderer->m2;
equipM2.load().get();

// JS: const skin = await m2.getSkin(0); if (!skin) return;
Skin* equipSkinPtr = equipM2.getSkin(0).get();
if (!equipSkinPtr)
	return;
auto& equipSkin = *equipSkinPtr;

auto slotNameOpt = wow::get_slot_name(slot_id);
std::string slot_name = slotNameOpt.has_value() ? std::string(slotNameOpt.value()) : std::format("Slot{}", slot_id);

// export equipment textures and build material map
const auto& config = core::view->config;
std::map<uint32_t, M2TextureExportInfo> equipTextures;

if (config.value("modelsExportTextures", false) && !textures.empty()) {
for (size_t i = 0; i < textures.size(); i++) {
const uint32_t texFileDataID = textures[i];
if (texFileDataID == 0)
continue;

// use existing texture if already exported
auto existingIt = textureMap.find(std::to_string(texFileDataID));
if (existingIt != textureMap.end()) {
equipTextures[static_cast<uint32_t>(i)] = existingIt->second;
continue;
}

try {
std::string fileName = casc::listfile::getByID(texFileDataID).value_or("");
std::string matName = "mat_equip_" + std::to_string(texFileDataID);
std::string texFile = std::to_string(texFileDataID) + ".png";
auto texPath = outDir / texFile;

if (!fileName.empty()) {
std::string baseName = std::filesystem::path(fileName).stem().string();
std::transform(baseName.begin(), baseName.end(), baseName.begin(),
[](unsigned char c) { return std::tolower(c); });
matName = "mat_" + baseName;
if (config.value("removePathSpaces", false))
std::erase_if(matName, [](char c) { return std::isspace(static_cast<unsigned char>(c)); });
}

if (config.value("enableSharedTextures", false) && !fileName.empty()) {
std::string sharedFileName = casc::ExportHelper::replaceExtension(fileName, ".png");
texPath = casc::ExportHelper::getExportPath(sharedFileName);
texFile = std::filesystem::relative(texPath, outDir).string();
}

// for glb mode, we need to get the texture buffer
if (format == "glb") {
auto fileData = casc->getVirtualFileByID(texFileDataID);
casc::BLPImage blp(fileData);
auto png_buffer = blp.toPNG(config.value("modelsExportAlpha", false) ? 0b1111 : 0b0111);

gltf.addTextureBuffer(std::to_string(texFileDataID), std::move(png_buffer));
} else if (config.value("overwriteFiles", true) || !generics::fileExists(texPath)) {
auto fileData = casc->getVirtualFileByID(texFileDataID);
casc::BLPImage blp(fileData);
blp.saveToPNG(texPath, config.value("modelsExportAlpha", false) ? 0b1111 : 0b0111);
}

const bool usePosix = config.value("pathFormat", std::string("")) == "posix";
if (usePosix)
texFile = casc::ExportHelper::win32ToPosix(texFile);

M2TextureExportInfo texInfo = { matName, texFile, texPath };
textureMap[std::to_string(texFileDataID)] = texInfo;
equipTextures[static_cast<uint32_t>(i)] = texInfo;
} catch (const std::exception& e) {
logging::write(std::format("Failed to export equipment GLTF texture {}: {}", texFileDataID, e.what()));
}
}
}

// build meshes for this equipment
std::vector<GLTFMesh> meshes;
int mesh_idx = 0;

for (size_t mI = 0; mI < equipSkin.subMeshes.size(); mI++) {
// check visibility via draw_calls if available
const auto& draw_calls = renderer->get_draw_calls();
if (!draw_calls.empty() && mI < draw_calls.size() && !draw_calls[mI].visible)
continue;

const auto& mesh = equipSkin.subMeshes[mI];
std::vector<uint32_t> triangles(mesh.triangleCount);
for (uint16_t vI = 0; vI < mesh.triangleCount; vI++)
triangles[vI] = equipSkin.indices[equipSkin.triangles[mesh.triangleStart + vI]];

// find texture for this submesh
std::string matName;
auto texUnitIt = std::find_if(equipSkin.textureUnits.begin(), equipSkin.textureUnits.end(),
[mI](const TextureUnit& tex) { return tex.skinSectionIndex == static_cast<uint16_t>(mI); });

if (texUnitIt != equipSkin.textureUnits.end()) {
const uint16_t textureIdx = equipM2.textureCombos[texUnitIt->textureComboIndex];
const auto& texture = equipM2.textures[textureIdx];
const uint32_t textureType = equipM2.textureTypes[textureIdx];

// check for replaceable texture
if (textureType >= 11 && textureType < 14) {
auto texInfoIt = equipTextures.find(textureType - 11);
if (texInfoIt != equipTextures.end())
matName = texInfoIt->second.matName;
} else if (textureType > 1 && textureType < 5) {
auto texInfoIt = equipTextures.find(textureType - 2);
if (texInfoIt != equipTextures.end())
matName = texInfoIt->second.matName;
} else if (texture.fileDataID > 0) {
auto texMapIt = textureMap.find(std::to_string(texture.fileDataID));
if (texMapIt != textureMap.end())
matName = texMapIt->second.matName;
}
}

meshes.push_back({ std::to_string(mesh_idx++), std::move(triangles), matName });
}

// add equipment to GLTF
GLTFEquipmentModel gltfEquip;
gltfEquip.name = std::format("{}_Item{}", slot_name, item_id);
gltfEquip.vertices = vertices;
gltfEquip.normals = normals;
gltfEquip.uv = uv;
gltfEquip.uv2 = uv2;
gltfEquip.boneIndices = is_collection_style ? boneIndices : std::vector<uint8_t>();
gltfEquip.boneWeights = is_collection_style ? boneWeights : std::vector<uint8_t>();
gltfEquip.meshes = std::move(meshes);
gltf.addEquipmentModel(gltfEquip);

logging::write(std::format("Added equipment GLTF meshes for slot {} (item {})", slot_id, item_id));
}

/**
 * Export equipment model geometry and textures to OBJ/MTL.
 * @private
 */
void M2Exporter::_exportEquipmentToOBJ(OBJWriter& obj, MTLWriter& mtl, const std::filesystem::path& outDir,
const EquipmentModel& equip, std::map<std::string, M2TextureExportInfo>& validTextures,
casc::ExportHelper* helper, std::vector<M2ExportFileManifest>* fileManifest)
{
const auto& config = core::view->config;
const bool useAlpha = config.value("modelsExportAlpha", false);
const bool usePosix = config.value("pathFormat", std::string("")) == "posix";
const auto& [slot_id, item_id, renderer, vertices, normals, uv, uv2, textures] = equip;

if (!renderer || !renderer->m2)
return;

auto& equipM2 = *renderer->m2;
equipM2.load().get();

// JS: const skin = await m2.getSkin(0); if (!skin) return;
Skin* equipSkinPtr = equipM2.getSkin(0).get();
if (!equipSkinPtr)
	return;
auto& equipSkin = *equipSkinPtr;

// build UV arrays
std::vector<std::vector<float>> uvArrays;
if (!uv.empty())
uvArrays.push_back(uv);

if (config.value("modelsExportUV2", false) && !uv2.empty())
uvArrays.push_back(uv2);

// append geometry to OBJ
obj.appendGeometry(vertices, normals, uvArrays);

// export equipment textures and build material map
std::map<uint32_t, M2TextureExportInfo> equipTextures;
if (config.value("modelsExportTextures", false) && !textures.empty()) {
for (size_t i = 0; i < textures.size(); i++) {
const uint32_t texFileDataID = textures[i];
if (texFileDataID == 0)
continue;

// skip if already exported
auto existingIt = validTextures.find(std::to_string(texFileDataID));
if (existingIt != validTextures.end()) {
equipTextures[static_cast<uint32_t>(i)] = existingIt->second;
continue;
}

try {
std::string texFile = std::to_string(texFileDataID) + ".png";
auto texPath = outDir / texFile;
std::string matName = "mat_equip_" + std::to_string(texFileDataID);

std::string fileName = casc::listfile::getByID(texFileDataID).value_or("");
if (!fileName.empty()) {
std::string baseName = std::filesystem::path(fileName).stem().string();
std::transform(baseName.begin(), baseName.end(), baseName.begin(),
[](unsigned char c) { return std::tolower(c); });
matName = "mat_" + baseName;
if (config.value("removePathSpaces", false))
std::erase_if(matName, [](char c) { return std::isspace(static_cast<unsigned char>(c)); });
}

if (config.value("enableSharedTextures", false) && !fileName.empty()) {
std::string sharedFileName = casc::ExportHelper::replaceExtension(fileName, ".png");
texPath = casc::ExportHelper::getExportPath(sharedFileName);
texFile = std::filesystem::relative(texPath, outDir).string();
}

if (config.value("overwriteFiles", true) || !generics::fileExists(texPath)) {
auto fileData = casc->getVirtualFileByID(texFileDataID);
casc::BLPImage blp(fileData);
blp.saveToPNG(texPath, useAlpha ? 0b1111 : 0b0111);
logging::write(std::format("Exported equipment texture {} -> {}", texFileDataID, texPath.string()));
}

if (usePosix)
texFile = casc::ExportHelper::win32ToPosix(texFile);

mtl.addMaterial(matName, texFile);
M2TextureExportInfo texInfo = { matName, texFile, texPath };
validTextures[std::to_string(texFileDataID)] = texInfo;
equipTextures[static_cast<uint32_t>(i)] = texInfo;
if (fileManifest)
fileManifest->push_back({ "PNG", texFileDataID, texPath });
} catch (const std::exception& e) {
logging::write(std::format("Failed to export equipment texture {}: {}", texFileDataID, e.what()));
}
}
}

// add equipment meshes
auto slotNameOpt = wow::get_slot_name(slot_id);
std::string slot_name = slotNameOpt.has_value() ? std::string(slotNameOpt.value()) : std::format("Slot{}", slot_id);
int mesh_idx = 0;

for (size_t mI = 0; mI < equipSkin.subMeshes.size(); mI++) {
// check visibility via draw_calls if available
const auto& draw_calls = renderer->get_draw_calls();
if (!draw_calls.empty() && mI < draw_calls.size() && !draw_calls[mI].visible)
continue;

const auto& mesh = equipSkin.subMeshes[mI];
std::vector<uint32_t> verts(mesh.triangleCount);
for (uint16_t vI = 0; vI < mesh.triangleCount; vI++)
verts[vI] = equipSkin.indices[equipSkin.triangles[mesh.triangleStart + vI]];

// find texture for this submesh
std::string matName;
auto texUnitIt = std::find_if(equipSkin.textureUnits.begin(), equipSkin.textureUnits.end(),
[mI](const TextureUnit& tex) { return tex.skinSectionIndex == static_cast<uint16_t>(mI); });

if (texUnitIt != equipSkin.textureUnits.end()) {
const uint16_t textureIdx = equipM2.textureCombos[texUnitIt->textureComboIndex];
const auto& texture = equipM2.textures[textureIdx];
const uint32_t textureType = equipM2.textureTypes[textureIdx];

// check for replaceable texture
if (textureType >= 11 && textureType < 14) {
auto texInfoIt = equipTextures.find(textureType - 11);
if (texInfoIt != equipTextures.end())
matName = texInfoIt->second.matName;
} else if (textureType > 1 && textureType < 5) {
auto texInfoIt = equipTextures.find(textureType - 2);
if (texInfoIt != equipTextures.end())
matName = texInfoIt->second.matName;
} else if (texture.fileDataID > 0) {
auto texMapIt = validTextures.find(std::to_string(texture.fileDataID));
if (texMapIt != validTextures.end())
matName = texMapIt->second.matName;
}
}

std::string meshName = std::format("{}_Item{}_{}", slot_name, item_id, mesh_idx++);
obj.addMesh(meshName, verts, matName);
}

logging::write(std::format("Added equipment meshes for slot {} (item {})", slot_id, item_id));
}

/**
 * Export equipment model geometry to STL.
 * @private
 */
void M2Exporter::_exportEquipmentToSTL(STLWriter& stl, const EquipmentModel& equip, casc::ExportHelper* helper) {
const auto& [slot_id, item_id, renderer, vertices, normals, uv, uv2, textures] = equip;

if (!renderer || !renderer->m2)
return;

auto& equipM2 = *renderer->m2;
equipM2.load().get();

// JS: const skin = await m2.getSkin(0); if (!skin) return;
Skin* equipSkinPtr = equipM2.getSkin(0).get();
if (!equipSkinPtr)
	return;
auto& equipSkin = *equipSkinPtr;

// append geometry to STL
stl.appendGeometry(vertices, normals);

// add equipment meshes
auto slotNameOpt = wow::get_slot_name(slot_id);
std::string slot_name = slotNameOpt.has_value() ? std::string(slotNameOpt.value()) : std::format("Slot{}", slot_id);
int mesh_idx = 0;

for (size_t mI = 0; mI < equipSkin.subMeshes.size(); mI++) {
// check visibility via draw_calls if available
const auto& draw_calls = renderer->get_draw_calls();
if (!draw_calls.empty() && mI < draw_calls.size() && !draw_calls[mI].visible)
continue;

const auto& mesh = equipSkin.subMeshes[mI];
std::vector<uint32_t> verts(mesh.triangleCount);
for (uint16_t vI = 0; vI < mesh.triangleCount; vI++)
verts[vI] = equipSkin.indices[equipSkin.triangles[mesh.triangleStart + vI]];

std::string meshName = std::format("{}_Item{}_{}", slot_name, item_id, mesh_idx++);
stl.addMesh(meshName, verts);
}

logging::write(std::format("Added equipment STL meshes for slot {} (item {})", slot_id, item_id));
}

/**
 * Export the M2 model as a WaveFront OBJ.
 * @param out
 * @param exportCollision
 * @param helper
 * @param fileManifest
 */
void M2Exporter::exportAsOBJ(const std::filesystem::path& out, bool exportCollision,
casc::ExportHelper* helper, std::vector<M2ExportFileManifest>* fileManifest)
{
m2->load().get();
auto& skin = *m2->getSkin(0).get();

const auto& config = core::view->config;
const bool exportMeta = config.value("exportM2Meta", false);
const bool exportBones = config.value("exportM2Bones", false);

OBJWriter obj(out);
auto mtlPath = casc::ExportHelper::replaceExtension(out.string(), ".mtl");
MTLWriter mtl(mtlPath);

const auto outDir = out.parent_path();

// Use internal M2 name or fallback to the OBJ file name.
const auto model_name = out.stem().string();
obj.setName(model_name);

logging::write(std::format("Exporting M2 model {} as OBJ: {}", model_name, out.string()));

// verts, normals, UVs - use posed geometry if available
obj.setVertArray(!posedVertices.empty() ? posedVertices : m2->vertices);
obj.setNormalArray(!posedNormals.empty() ? posedNormals : m2->normals);
obj.addUVArray(m2->uv);

if (config.value("modelsExportUV2", false))
obj.addUVArray(m2->uv2);

// Textures
auto texResult = exportTextures(outDir, false, &mtl, helper);
auto& validTextures = texResult.validTextures;
for (const auto& [texKey, texInfo] : validTextures) {
if (fileManifest) {
// JS: texFileDataID is a number for regular textures, "data-X" string for data textures.
uint32_t texID = 0;
bool is_numeric = false;
try { texID = std::stoul(texKey); is_numeric = true; } catch (...) {}
if (is_numeric)
fileManifest->push_back({ "PNG", texID, texInfo.matPath });
else
fileManifest->push_back({ "PNG", texKey, texInfo.matPath });
}
}

// Abort if the export has been cancelled.
if (helper && helper->isCancelled())
return;

// Export bone data to a separate JSON file due to the excessive size.
// A normal meta-data file is around 8kb without bones, 65mb with bones.
if (exportBones) {
JSONWriter json(casc::ExportHelper::replaceExtension(out.string(), "_bones.json"));

if (m2->skeletonFileID) {
auto skel_file = casc->getVirtualFileByID(m2->skeletonFileID);
SKELLoader skel(skel_file);

skel.load();

if (skel.parent_skel_file_id > 0) {
auto parent_skel_file = casc->getVirtualFileByID(skel.parent_skel_file_id);
SKELLoader parent_skel(parent_skel_file);
parent_skel.load();

json.addProperty("bones", bonesToJson(parent_skel.bones));
			} else {
				json.addProperty("bones", bonesToJson(skel.bones));
			}

		} else {
			json.addProperty("bones", bonesToJson(m2->bones));
		}

		json.addProperty("boneWeights", uint8VecToJsonArray(m2->boneWeights));
		json.addProperty("boneIndicies", uint8VecToJsonArray(m2->boneIndices));

json.write(config.value("overwriteFiles", true));
}

if (exportMeta) {
JSONWriter json(casc::ExportHelper::replaceExtension(out.string(), ".json"));

// Clone the submesh array and add a custom 'enabled' property
// to indicate to external readers which submeshes are not included
// in the actual geometry file.
nlohmann::json subMeshes = nlohmann::json::array();
for (size_t i = 0, n = skin.subMeshes.size(); i < n; i++) {
const bool subMeshEnabled = geosetMask.empty() || (i < geosetMask.size() && geosetMask[i].checked);
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

// Clone M2 textures array and expand the entries to include internal
// and external paths/names for external convenience. GH-208
nlohmann::json texturesJson = nlohmann::json::array();
for (size_t i = 0, n = m2->textures.size(); i < n; i++) {
const auto& texture = m2->textures[i];
auto textureIt = validTextures.find(std::to_string(texture.fileDataID));

nlohmann::json texObj;
texObj["fileDataID"] = texture.fileDataID;
texObj["flags"] = texture.flags;
// JS Object.assign also spreads texture.fileName if set
if (!texture.fileName.empty())
	texObj["fileName"] = texture.fileName;
// JS: listfile.getByID() returns undefined when not found;
// use null in JSON to match JS behavior instead of empty string.
{
	auto internalName = casc::listfile::getByID(texture.fileDataID).value_or("");
	if (!internalName.empty())
		texObj["fileNameInternal"] = internalName;
	else
		texObj["fileNameInternal"] = nullptr;
}
if (textureIt != validTextures.end()) {
texObj["fileNameExternal"] = textureIt->second.matPathRelative;
texObj["mtlName"] = textureIt->second.matName;
} else {
texObj["fileNameExternal"] = nullptr;
texObj["mtlName"] = nullptr;
}
texturesJson.push_back(texObj);
}

json.addProperty("fileType", "m2");
json.addProperty("fileDataID", fileDataID);
json.addProperty("fileName", casc::listfile::getByID(fileDataID).value_or(""));
json.addProperty("internalName", m2->name);
json.addProperty("textures", texturesJson);
json.addProperty("textureTypes", nlohmann::json(m2->textureTypes));
json.addProperty("materials", m2MaterialsToJson(m2->materials));
json.addProperty("textureCombos", nlohmann::json(m2->textureCombos));
json.addProperty("skeletonFileID", m2->skeletonFileID);
json.addProperty("boneFileIDs", nlohmann::json(m2->boneFileIDs));
json.addProperty("animFileIDs", m2AnimFileIDsToJson(m2->animFileIDs));
json.addProperty("colors", m2ColorsToJson(m2->colors));
json.addProperty("textureWeights", m2TracksToJson(m2->textureWeights));
json.addProperty("transparencyLookup", nlohmann::json(m2->transparencyLookup));
json.addProperty("textureTransforms", m2TextureTransformsToJson(m2->textureTransforms));
json.addProperty("textureTransformsLookup", nlohmann::json(m2->textureTransformsLookup));
json.addProperty("boundingBox", caabbToJson(m2->boundingBox));
json.addProperty("boundingSphereRadius", m2->boundingSphereRadius);
json.addProperty("collisionBox", caabbToJson(m2->collisionBox));
json.addProperty("collisionSphereRadius", m2->collisionSphereRadius);

nlohmann::json skinTextureUnits = nlohmann::json::array();
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
skinTextureUnits.push_back(tuObj);
}

json.addProperty("skin", nlohmann::json{
{ "subMeshes", subMeshes },
{ "textureUnits", skinTextureUnits },
{ "fileName", skin.fileName },
{ "fileDataID", skin.fileDataID }
});

json.write(config.value("overwriteFiles", true));
if (fileManifest)
fileManifest->push_back({ "META", fileDataID, std::filesystem::path(casc::ExportHelper::replaceExtension(out.string(), ".json")) });
}

// Faces
for (size_t mI = 0, mC = skin.subMeshes.size(); mI < mC; mI++) {
// Skip geosets that are not enabled.
if (!geosetMask.empty() && (mI >= geosetMask.size() || !geosetMask[mI].checked))
continue;

const auto& mesh = skin.subMeshes[mI];
std::vector<uint32_t> verts(mesh.triangleCount);
for (uint16_t vI = 0; vI < mesh.triangleCount; vI++)
verts[vI] = skin.indices[skin.triangles[mesh.triangleStart + vI]];

// Find texture
std::string matName;
auto texUnitIt = std::find_if(skin.textureUnits.begin(), skin.textureUnits.end(),
[mI](const TextureUnit& tex) { return tex.skinSectionIndex == static_cast<uint16_t>(mI); });

if (texUnitIt != skin.textureUnits.end()) {
const auto& texture = m2->textures[m2->textureCombos[texUnitIt->textureComboIndex]];

if (texture.fileDataID > 0) {
auto it = validTextures.find(std::to_string(texture.fileDataID));
if (it != validTextures.end())
matName = it->second.matName;
}

uint32_t texType = m2->textureTypes[m2->textureCombos[texUnitIt->textureComboIndex]];
if (dataTextures.count(texType)) {
std::string dataTextureKey = "data-" + std::to_string(texType);
auto it = validTextures.find(dataTextureKey);
if (it != validTextures.end())
matName = it->second.matName;
}
}

obj.addMesh(geoset_mapper::getGeosetName(static_cast<int>(mI), mesh.submeshID), verts, matName);
}

// export equipment models if present
if (!equipmentModels.empty()) {
for (const auto& equip : equipmentModels) {
if (helper && helper->isCancelled())
return;

_exportEquipmentToOBJ(obj, mtl, outDir, equip, validTextures, helper, fileManifest);
}
}

if (!mtl.isEmpty())
obj.setMaterialLibrary(std::filesystem::path(mtlPath).filename().string());

obj.write(config.value("overwriteFiles", true));
if (fileManifest)
fileManifest->push_back({ "OBJ", fileDataID, out });

mtl.write(config.value("overwriteFiles", true));
if (fileManifest)
fileManifest->push_back({ "MTL", fileDataID, std::filesystem::path(mtlPath) });

if (exportCollision) {
OBJWriter phys(casc::ExportHelper::replaceExtension(out.string(), ".phys.obj"));
phys.setVertArray(m2->collisionPositions);
phys.setNormalArray(m2->collisionNormals);
phys.addMesh("Collision", std::vector<uint32_t>(m2->collisionIndices.begin(), m2->collisionIndices.end()));

phys.write(config.value("overwriteFiles", true));
if (fileManifest)
fileManifest->push_back({ "PHYS_OBJ", fileDataID, std::filesystem::path(casc::ExportHelper::replaceExtension(out.string(), ".phys.obj")) });
}
}

/**
 * Export the M2 model as an STL file.
 * @param out
 * @param exportCollision
 * @param helper
 * @param fileManifest
 */
void M2Exporter::exportAsSTL(const std::filesystem::path& out, bool exportCollision,
casc::ExportHelper* helper, std::vector<M2ExportFileManifest>* fileManifest)
{
m2->load().get();
auto& skin = *m2->getSkin(0).get();

const auto& config = core::view->config;

STLWriter stl(out);
const auto model_name = out.stem().string();
stl.setName(model_name);

logging::write(std::format("Exporting M2 model {} as STL: {}", model_name, out.string()));

// verts, normals - use posed geometry if available
stl.setVertArray(!posedVertices.empty() ? posedVertices : m2->vertices);
stl.setNormalArray(!posedNormals.empty() ? posedNormals : m2->normals);

// abort if the export has been cancelled
if (helper && helper->isCancelled())
return;

// faces
for (size_t mI = 0, mC = skin.subMeshes.size(); mI < mC; mI++) {
// skip geosets that are not enabled
if (!geosetMask.empty() && (mI >= geosetMask.size() || !geosetMask[mI].checked))
continue;

const auto& mesh = skin.subMeshes[mI];
std::vector<uint32_t> verts(mesh.triangleCount);
for (uint16_t vI = 0; vI < mesh.triangleCount; vI++)
verts[vI] = skin.indices[skin.triangles[mesh.triangleStart + vI]];

stl.addMesh(geoset_mapper::getGeosetName(static_cast<int>(mI), mesh.submeshID), verts);
}

// export equipment models if present
if (!equipmentModels.empty()) {
for (const auto& equip : equipmentModels) {
if (helper && helper->isCancelled())
return;

_exportEquipmentToSTL(stl, equip, helper);
}
}

stl.write(config.value("overwriteFiles", true));
if (fileManifest)
fileManifest->push_back({ "STL", fileDataID, out });

if (exportCollision) {
STLWriter phys(casc::ExportHelper::replaceExtension(out.string(), ".phys.stl"));
phys.setVertArray(m2->collisionPositions);
phys.setNormalArray(m2->collisionNormals);
phys.addMesh("Collision", std::vector<uint32_t>(m2->collisionIndices.begin(), m2->collisionIndices.end()));

phys.write(config.value("overwriteFiles", true));
if (fileManifest)
fileManifest->push_back({ "PHYS_STL", fileDataID, std::filesystem::path(casc::ExportHelper::replaceExtension(out.string(), ".phys.stl")) });
}
}

/**
 * Export the model as a raw M2 file, including related files
 * such as textures, bones, animations, etc.
 * @param out
 * @param helper
 * @param fileManifest
 */
void M2Exporter::exportRaw(const std::filesystem::path& out, casc::ExportHelper* helper,
std::vector<M2ExportFileManifest>* fileManifest)
{
const auto& config = core::view->config;

const auto manifestFile = casc::ExportHelper::replaceExtension(out.string(), ".manifest.json");
JSONWriter manifest(manifestFile);

manifest.addProperty("fileDataID", fileDataID);

// Write the M2 file with no conversion.
data.writeToFile(out);
if (fileManifest)
fileManifest->push_back({ "M2", fileDataID, out });

// Only load M2 data if we need to export related files.
if (config.value("modelsExportSkin", false) || config.value("modelsExportSkel", false) ||
config.value("modelsExportBone", false) || config.value("modelsExportAnim", false))
m2->load().get();

// Directory that relative files should be exported to.
const auto outDir = out.parent_path();

// Write relative skin files.
if (config.value("modelsExportSkin", false)) {
auto texResult = exportTextures(outDir, true, nullptr, helper);
nlohmann::json texturesManifest = nlohmann::json::array();
for (const auto& [texKey, texInfo] : texResult.validTextures) {
uint32_t texID = 0;
bool is_numeric = false;
try { texID = std::stoul(texKey); is_numeric = true; } catch (...) {}
texturesManifest.push_back({
{ "fileDataID", is_numeric ? nlohmann::json(texID) : nlohmann::json(texKey) },
{ "file", std::filesystem::relative(texInfo.matPath, outDir).string() }
});
if (fileManifest) {
if (is_numeric)
fileManifest->push_back({ "BLP", texID, texInfo.matPath });
else
fileManifest->push_back({ "BLP", texKey, texInfo.matPath });
}
}

manifest.addProperty("textures", texturesManifest);

auto exportSkins = [&](std::vector<Skin>& skins, const std::string& typeName, const std::string& manifestName) {
nlohmann::json skinsManifest = nlohmann::json::array();
for (auto& skin : skins) {
// Abort if the export has been cancelled.
if (helper && helper->isCancelled())
return;

auto skinData = casc->getVirtualFileByID(skin.fileDataID);

std::string skinFile;
if (config.value("enableSharedChildren", false))
skinFile = casc::ExportHelper::getExportPath(skin.fileName);
else
skinFile = (outDir / std::filesystem::path(skin.fileName).filename()).string();

skinData.writeToFile(skinFile);
skinsManifest.push_back({
{ "fileDataID", skin.fileDataID },
{ "file", std::filesystem::relative(skinFile, outDir).string() }
});
if (fileManifest)
fileManifest->push_back({ typeName, skin.fileDataID, std::filesystem::path(skinFile) });
}

manifest.addProperty(manifestName, skinsManifest);
};

auto& skinList = m2->getSkinList();
exportSkins(skinList, "SKIN", "skins");
exportSkins(m2->lodSkins, "LOD_SKIN", "lodSkins");
}

// Write relative skeleton files.
if (config.value("modelsExportSkel", false) && m2->skeletonFileID) {
auto skelData = casc->getVirtualFileByID(m2->skeletonFileID);
std::string skelFileName = casc::listfile::getByID(m2->skeletonFileID).value_or("");

std::string skelFile;
if (config.value("enableSharedChildren", false))
skelFile = casc::ExportHelper::getExportPath(skelFileName);
else
skelFile = (outDir / std::filesystem::path(skelFileName).filename()).string();

skelData.writeToFile(skelFile);
manifest.addProperty("skeleton", nlohmann::json{
{ "fileDataID", m2->skeletonFileID },
{ "file", std::filesystem::relative(skelFile, outDir).string() }
});
if (fileManifest)
fileManifest->push_back({ "SKEL", m2->skeletonFileID, std::filesystem::path(skelFile) });

SKELLoader skel(skelData);
skel.load();

if (config.value("modelsExportAnim", false)) {
skel.loadAnims();
if (!skel.animFileIDs.empty()) {
nlohmann::json animManifest = nlohmann::json::array();
std::set<uint32_t> animCache;
for (const auto& anim : skel.animFileIDs) {
if (anim.fileDataID > 0 && !animCache.count(anim.fileDataID)) {
auto animData = casc->getVirtualFileByID(anim.fileDataID);
std::string animFileName = casc::listfile::getByIDOrUnknown(anim.fileDataID, ".anim");

std::string animFile;
if (config.value("enableSharedChildren", false))
animFile = casc::ExportHelper::getExportPath(animFileName);
else
animFile = (outDir / std::filesystem::path(animFileName).filename()).string();

animData.writeToFile(animFile);
animManifest.push_back({
{ "fileDataID", anim.fileDataID },
{ "file", std::filesystem::relative(animFile, outDir).string() },
{ "animID", anim.animID },
{ "subAnimID", anim.subAnimID }
});
if (fileManifest)
fileManifest->push_back({ "ANIM", anim.fileDataID, std::filesystem::path(animFile) });
animCache.insert(anim.fileDataID);
}
}

manifest.addProperty("skelAnims", animManifest);
}

if (config.value("modelsExportBone", false) && !skel.boneFileIDs.empty()) {
nlohmann::json boneManifest = nlohmann::json::array();
for (size_t i = 0, n = skel.boneFileIDs.size(); i < n; i++) {
const uint32_t boneFileID = skel.boneFileIDs[i];
auto boneData = casc->getVirtualFileByID(boneFileID);
std::string boneFileName = casc::listfile::getByIDOrUnknown(boneFileID, ".bone");

std::string boneFile;
if (config.value("enableSharedChildren", false))
boneFile = casc::ExportHelper::getExportPath(boneFileName);
else
boneFile = (outDir / std::filesystem::path(boneFileName).filename()).string();

boneData.writeToFile(boneFile);
boneManifest.push_back({
{ "fileDataID", boneFileID },
{ "file", std::filesystem::relative(boneFile, outDir).string() }
});
if (fileManifest)
fileManifest->push_back({ "BONE", boneFileID, std::filesystem::path(boneFile) });
}

manifest.addProperty("skelBones", boneManifest);
}
}

if (skel.parent_skel_file_id > 0) {
auto parentSkelData = casc->getVirtualFileByID(skel.parent_skel_file_id);
std::string parentSkelFileName = casc::listfile::getByID(skel.parent_skel_file_id).value_or("");

std::string parentSkelFile;
if (config.value("enableSharedChildren", false))
parentSkelFile = casc::ExportHelper::getExportPath(parentSkelFileName);
else
parentSkelFile = (outDir / std::filesystem::path(parentSkelFileName).filename()).string();

parentSkelData.writeToFile(parentSkelFile);

manifest.addProperty("parentSkeleton", nlohmann::json{
{ "fileDataID", skel.parent_skel_file_id },
{ "file", std::filesystem::relative(parentSkelFile, outDir).string() }
});
if (fileManifest)
fileManifest->push_back({ "PARENT_SKEL", skel.parent_skel_file_id, std::filesystem::path(parentSkelFile) });

SKELLoader parentSkel(parentSkelData);
parentSkel.load();

if (config.value("modelsExportAnim", false)) {
parentSkel.loadAnims();
if (!parentSkel.animFileIDs.empty()) {
nlohmann::json animManifest = nlohmann::json::array();
std::set<uint32_t> animCache;
for (const auto& anim : parentSkel.animFileIDs) {
if (anim.fileDataID > 0 && !animCache.count(anim.fileDataID)) {
auto animData = casc->getVirtualFileByID(anim.fileDataID);
std::string animFileName = casc::listfile::getByIDOrUnknown(anim.fileDataID, ".anim");

std::string animFile;
if (config.value("enableSharedChildren", false))
animFile = casc::ExportHelper::getExportPath(animFileName);
else
animFile = (outDir / std::filesystem::path(animFileName).filename()).string();

animData.writeToFile(animFile);
animManifest.push_back({
{ "fileDataID", anim.fileDataID },
{ "file", std::filesystem::relative(animFile, outDir).string() },
{ "animID", anim.animID },
{ "subAnimID", anim.subAnimID }
});
if (fileManifest)
fileManifest->push_back({ "ANIM", anim.fileDataID, std::filesystem::path(animFile) });
animCache.insert(anim.fileDataID);
}
}

manifest.addProperty("parentSkelAnims", animManifest);
}
}

if (config.value("modelsExportBone", false) && !parentSkel.boneFileIDs.empty()) {
nlohmann::json boneManifest = nlohmann::json::array();
for (size_t i = 0, n = parentSkel.boneFileIDs.size(); i < n; i++) {
const uint32_t boneFileID = parentSkel.boneFileIDs[i];
auto boneData = casc->getVirtualFileByID(boneFileID);
std::string boneFileName = casc::listfile::getByIDOrUnknown(boneFileID, ".bone");

std::string boneFile;
if (config.value("enableSharedChildren", false))
boneFile = casc::ExportHelper::getExportPath(boneFileName);
else
boneFile = (outDir / std::filesystem::path(boneFileName).filename()).string();

boneData.writeToFile(boneFile);
boneManifest.push_back({
{ "fileDataID", boneFileID },
{ "file", std::filesystem::relative(boneFile, outDir).string() }
});
if (fileManifest)
fileManifest->push_back({ "BONE", boneFileID, std::filesystem::path(boneFile) });
}

manifest.addProperty("parentSkelBones", boneManifest);
}
}
}

// Write relative bone files.
if (config.value("modelsExportBone", false) && !m2->boneFileIDs.empty()) {
nlohmann::json boneManifest = nlohmann::json::array();
for (size_t i = 0, n = m2->boneFileIDs.size(); i < n; i++) {
const uint32_t boneFileID = m2->boneFileIDs[i];
auto boneData = casc->getVirtualFileByID(boneFileID);
std::string boneFileName = casc::listfile::getByIDOrUnknown(boneFileID, ".bone");

std::string boneFile;
if (config.value("enableSharedChildren", false))
boneFile = casc::ExportHelper::getExportPath(boneFileName);
else
boneFile = (outDir / std::filesystem::path(boneFileName).filename()).string();

boneData.writeToFile(boneFile);
boneManifest.push_back({
{ "fileDataID", boneFileID },
{ "file", std::filesystem::relative(boneFile, outDir).string() }
});
if (fileManifest)
fileManifest->push_back({ "BONE", boneFileID, std::filesystem::path(boneFile) });
}

manifest.addProperty("bones", boneManifest);
}

// Write relative animation files.
if (config.value("modelsExportAnim", false) && !m2->animFileIDs.empty()) {
nlohmann::json animManifest = nlohmann::json::array();
std::set<uint32_t> animCache;
for (const auto& anim : m2->animFileIDs) {
if (anim.fileDataID > 0 && !animCache.count(anim.fileDataID)) {
auto animData = casc->getVirtualFileByID(anim.fileDataID);
std::string animFileName = casc::listfile::getByIDOrUnknown(anim.fileDataID, ".anim");

std::string animFile;
if (config.value("enableSharedChildren", false))
animFile = casc::ExportHelper::getExportPath(animFileName);
else
animFile = (outDir / std::filesystem::path(animFileName).filename()).string();

animData.writeToFile(animFile);
animManifest.push_back({
{ "fileDataID", anim.fileDataID },
{ "file", std::filesystem::relative(animFile, outDir).string() },
{ "animID", anim.animID },
{ "subAnimID", anim.subAnimID }
});
if (fileManifest)
fileManifest->push_back({ "ANIM", anim.fileDataID, std::filesystem::path(animFile) });
animCache.insert(anim.fileDataID);
}
}

manifest.addProperty("anims", animManifest);
}

manifest.write();
}
