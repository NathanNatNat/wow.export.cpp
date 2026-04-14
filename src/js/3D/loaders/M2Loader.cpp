/*!
wow.export (https://github.com/Kruithne/wow.export)
Authors: Kruithne <kruithne@gmail.com>, Marlamin <marlamin@marlamin.com>
License: MIT
 */

#include "M2Loader.h"
#include "M2Generics.h"
#include "ANIMLoader.h"
#include "../AnimMapper.h"
#include "../Texture.h"
#include "../Skin.h"
#include "../../buffer.h"
#include "../../constants.h"
#include "../../core.h"
#include "../../casc/casc-source.h"
#include "../../log.h"

#include <format>
#include <stdexcept>

static constexpr uint32_t CHUNK_SFID = 0x44494653;
static constexpr uint32_t CHUNK_TXID = 0x44495854;
static constexpr uint32_t CHUNK_SKID = 0x44494B53;
static constexpr uint32_t CHUNK_BFID = 0x44494642;
static constexpr uint32_t CHUNK_AFID = 0x44494641;

M2Loader::M2Loader(BufferWrapper& data)
: isLoaded(false), data(data) {
}

/**
 * Load the M2 model.
 */
void M2Loader::load() {
// Prevent multiple loading of the same M2.
if (this->isLoaded == true)
return;

while (this->data.remainingBytes() > 0) {
const uint32_t chunkID = this->data.readUInt32LE();
const uint32_t chunkSize = this->data.readUInt32LE();
const size_t nextChunkPos = this->data.offset() + chunkSize;

switch (chunkID) {
case constants::MAGIC::MD21: this->parseChunk_MD21(); break;
case CHUNK_SFID: this->parseChunk_SFID(chunkSize); break;
case CHUNK_TXID: this->parseChunk_TXID(); break;
case CHUNK_SKID: this->parseChunk_SKID(); break;
case CHUNK_BFID: this->parseChunk_BFID(chunkSize); break;
case CHUNK_AFID: this->parseChunk_AFID(chunkSize); break;
}

// Ensure that we start at the next chunk exactly.
this->data.seek(nextChunkPos);
}

this->isLoaded = true;
}

/**
 * Get a skin at a given index from this->skins.
 */
Skin& M2Loader::getSkin(uint32_t index) {
Skin& skin = this->skins[index];
if (!skin.isLoaded)
skin.load();

return skin;
}

/**
 * Returns the internal array of Skin objects.
 * Note: Unlike getSkin(), this does not load any of the skins.
 */
std::vector<Skin>& M2Loader::getSkinList() {
return this->skins;
}

/**
 * Load and apply .anim files to loaded M2 model.
 */
void M2Loader::loadAnims(bool load_all) {
if (!load_all)
return;

for (size_t i = 0; i < this->animations.size(); i++) {
M2Animation* animation = &this->animations[i];

// if animation is an alias, resolve it
if ((animation->flags & 0x40) == 0x40) {
while ((animation->flags & 0x40) == 0x40)
animation = &this->animations[animation->aliasNext];
}

if ((animation->flags & 0x20) == 0x20) {
logging::write("Skipping .anim loading for " + get_anim_name(animation->id) + " because it should be in M2");
continue;
}

for (const auto& entry : this->animFileIDs) {
if (entry.animID != animation->id || entry.subAnimID != animation->variationIndex)
continue;

const uint32_t fileDataID = entry.fileDataID;
if (!this->animFiles.count(static_cast<uint32_t>(i))) {
if (fileDataID == 0) {
logging::write("Skipping .anim loading for " + get_anim_name(entry.animID) + " because it has no fileDataID");
continue;
}

logging::write(std::format("Loading .anim file for animation: {} ({}) - {}", entry.animID, get_anim_name(entry.animID), entry.subAnimID));

try {
BufferWrapper animData = core::view->casc->getVirtualFileByID(fileDataID);
auto ownedBuf = std::make_unique<BufferWrapper>(std::move(animData));
BufferWrapper* bufPtr = ownedBuf.get();
this->ownedAnimBuffers.push_back(std::move(ownedBuf));

bool animIsChunked = (this->flags & 0x200000) == 0x200000 || this->skeletonFileID > 0;

auto loader = std::make_unique<ANIMLoader>(*bufPtr);
loader->load(animIsChunked);

// Select the correct parsed data from the ANIMLoader.
if (!loader->skeletonBoneData.empty()) {
auto parsedBuf = std::make_unique<BufferWrapper>(std::move(loader->skeletonBoneData));
BufferWrapper* parsedPtr = parsedBuf.get();
this->ownedAnimBuffers.push_back(std::move(parsedBuf));
this->animFiles[static_cast<uint32_t>(i)] = parsedPtr;
} else {
auto parsedBuf = std::make_unique<BufferWrapper>(std::move(loader->animData));
BufferWrapper* parsedPtr = parsedBuf.get();
this->ownedAnimBuffers.push_back(std::move(parsedBuf));
this->animFiles[static_cast<uint32_t>(i)] = parsedPtr;
}

// patch this animation into bones
this->_patch_bone_animation(static_cast<uint32_t>(i));
} catch (const std::exception& e) {
logging::write(std::format("Failed to load .anim file (fileDataID={}): {}", fileDataID, e.what()));
}
}
}

if (!this->animFiles.count(static_cast<uint32_t>(i)))
logging::write("Failed to load .anim file for animation: " + std::to_string(animation->id) + " (" + get_anim_name(animation->id) + ") - " + std::to_string(animation->variationIndex));
}
}

/**
 * Load .anim file for a specific animation index (lazy loading).
 */
bool M2Loader::loadAnimsForIndex(uint32_t animationIndex) {
// check if already loaded
if (this->animFiles.count(animationIndex))
return true;

if (this->animFileIDs.empty() || animationIndex >= this->animations.size())
return false;

M2Animation* animation = &this->animations[animationIndex];

// resolve animation alias
if ((animation->flags & 0x40) == 0x40) {
while ((animation->flags & 0x40) == 0x40)
animation = &this->animations[animation->aliasNext];
}

// animation data is in M2 file, not external
if ((animation->flags & 0x20) == 0x20) {
logging::write("Animation " + get_anim_name(animation->id) + " should be in M2, not loading .anim");
return false;
}

// find matching AFID entry
for (const auto& entry : this->animFileIDs) {
if (entry.animID != animation->id || entry.subAnimID != animation->variationIndex)
continue;

const uint32_t fileDataID = entry.fileDataID;
if (fileDataID == 0) {
logging::write("Skipping .anim loading for " + get_anim_name(entry.animID) + " because it has no fileDataID");
return false;
}

logging::write(std::format("Loading .anim file for animation: {} ({}) - {}", entry.animID, get_anim_name(entry.animID), entry.subAnimID));

try {
BufferWrapper animData = core::view->casc->getVirtualFileByID(fileDataID);
auto ownedBuf = std::make_unique<BufferWrapper>(std::move(animData));
BufferWrapper* bufPtr = ownedBuf.get();
this->ownedAnimBuffers.push_back(std::move(ownedBuf));

bool animIsChunked = (this->flags & 0x200000) == 0x200000 || this->skeletonFileID > 0;

auto loader = std::make_unique<ANIMLoader>(*bufPtr);
loader->load(animIsChunked);

// store .anim data
if (!loader->skeletonBoneData.empty()) {
auto parsedBuf = std::make_unique<BufferWrapper>(std::move(loader->skeletonBoneData));
BufferWrapper* parsedPtr = parsedBuf.get();
this->ownedAnimBuffers.push_back(std::move(parsedBuf));
this->animFiles[animationIndex] = parsedPtr;
} else {
auto parsedBuf = std::make_unique<BufferWrapper>(std::move(loader->animData));
BufferWrapper* parsedPtr = parsedBuf.get();
this->ownedAnimBuffers.push_back(std::move(parsedBuf));
this->animFiles[animationIndex] = parsedPtr;
}

// patch animation data into existing bones
this->_patch_bone_animation(animationIndex);

return true;
} catch (const std::exception& e) {
logging::write(std::format("Failed to load .anim file (fileDataID={}): {}", fileDataID, e.what()));
return false;
}
}

logging::write("No .anim file found for animation: " + std::to_string(animation->id) + " (" + get_anim_name(animation->id) + ") - " + std::to_string(animation->variationIndex));
return false;
}

/**
 * Patch bone animation data for a specific animation index.
 */
void M2Loader::_patch_bone_animation(uint32_t animIndex) {
auto it = this->animFiles.find(animIndex);
if (it == this->animFiles.end() || !it->second || this->bones.empty())
return;

BufferWrapper& animBuffer = *it->second;

for (auto& bone : this->bones) {
patch_track_animation(bone.translation, animIndex, animBuffer, M2DataType::float3);
patch_track_animation(bone.rotation, animIndex, animBuffer, M2DataType::compquat);
patch_track_animation(bone.scale, animIndex, animBuffer, M2DataType::float3);

// apply coordinate system conversion to patched data
if (animIndex < bone.translation.values.size()) {
auto& translations = bone.translation.values[animIndex];
for (size_t j = 0; j < translations.size(); j++) {
auto& v = std::get<std::vector<float>>(translations[j]);
const float dx = v[0];
const float dy = v[1];
const float dz = v[2];
v[0] = dx;
v[2] = dy * -1;
v[1] = dz;
}
}

if (animIndex < bone.rotation.values.size()) {
auto& rotations = bone.rotation.values[animIndex];
for (size_t j = 0; j < rotations.size(); j++) {
auto& v = std::get<std::vector<float>>(rotations[j]);
const float dx = v[0];
const float dy = v[1];
const float dz = v[2];
const float dw = v[3];
v[0] = dx;
v[2] = dy * -1;
v[1] = dz;
v[3] = dw;
}
}

if (animIndex < bone.scale.values.size()) {
auto& scaleVals = bone.scale.values[animIndex];
for (size_t j = 0; j < scaleVals.size(); j++) {
auto& v = std::get<std::vector<float>>(scaleVals[j]);
const float dx = v[0];
const float dy = v[1];
const float dz = v[2];
v[0] = dx;
v[2] = dy;
v[1] = dz;
}
}
}
}

/**
 * Parse SFID chunk for skin file data IDs.
 */
void M2Loader::parseChunk_SFID(uint32_t chunkSize) {
if (this->viewCount == 0)
throw std::runtime_error("Cannot parse SFID chunk in M2 before MD21 chunk!");

const uint32_t lodSkinCount = (chunkSize / 4) - this->viewCount;
this->skins.reserve(this->viewCount);
this->lodSkins.reserve(lodSkinCount);

for (uint32_t i = 0; i < this->viewCount; i++)
this->skins.emplace_back(this->data.readUInt32LE());

for (uint32_t i = 0; i < lodSkinCount; i++)
this->lodSkins.emplace_back(this->data.readUInt32LE());
}

/**
 * Parse TXID chunk for texture file data IDs.
 */
void M2Loader::parseChunk_TXID() {
if (this->textures.empty())
throw std::runtime_error("Cannot parse TXID chunk in M2 before MD21 chunk!");

for (size_t i = 0; i < this->textures.size(); i++)
this->textures[i].fileDataID = this->data.readUInt32LE();
}

/**
 * Parse SKID chunk for .skel file data ID.
 */
void M2Loader::parseChunk_SKID() {
this->skeletonFileID = this->data.readUInt32LE();
}

/**
 * Parse BFID chunk for .bone file data IDs.
 */
void M2Loader::parseChunk_BFID(uint32_t chunkSize) {
this->boneFileIDs = this->data.readUInt32LE(chunkSize / 4);
}

/**
 * Parse AFID chunk for animation file data IDs.
 */
void M2Loader::parseChunk_AFID(uint32_t chunkSize) {
const uint32_t entryCount = chunkSize / 8;
this->animFileIDs.resize(entryCount);

for (uint32_t i = 0; i < entryCount; i++) {
M2AnimFileEntry& entry = this->animFileIDs[i];
entry.animID = this->data.readUInt16LE();
entry.subAnimID = this->data.readUInt16LE();
entry.fileDataID = this->data.readUInt32LE();
}
}

/**
 * Parse MD21 chunk.
 */
void M2Loader::parseChunk_MD21() {
const uint32_t ofs = static_cast<uint32_t>(this->data.offset());

const uint32_t magic = this->data.readUInt32LE();
if (magic != constants::MAGIC::MD20)
throw std::runtime_error("Invalid M2 magic: " + std::to_string(magic));

this->version = this->data.readUInt32LE();
this->parseChunk_MD21_modelName(ofs);
this->flags = this->data.readUInt32LE();
this->parseChunk_MD21_globalLoops(ofs);
this->parseChunk_MD21_animations(ofs);
this->parseChunk_MD21_animationLookup(ofs);
this->parseChunk_MD21_bones(ofs);
this->data.move(8);
this->parseChunk_MD21_vertices(ofs);
this->viewCount = this->data.readUInt32LE();
this->parseChunk_MD21_colors(ofs);
this->parseChunk_MD21_textures(ofs);
this->parseChunk_MD21_textureWeights(ofs);
this->parseChunk_MD21_textureTransforms(ofs);
this->parseChunk_MD21_replaceableTextureLookup(ofs);
this->parseChunk_MD21_materials(ofs);
this->data.move(2 * 4); // boneCombos
this->parseChunk_MD21_textureCombos(ofs);
this->data.move(8); // textureTransformBoneMap
this->parseChunk_MD21_transparencyLookup(ofs);
this->parseChunk_MD21_textureTransformLookup(ofs);
this->parseChunk_MD21_collision(ofs);
this->parseChunk_MD21_attachments(ofs);
this->parseChunk_MD21_attachmentLookup(ofs);
}

void M2Loader::parseChunk_MD21_bones(uint32_t ofs) {
auto& d = this->data;
const uint32_t boneCount = d.readUInt32LE();
const uint32_t boneOfs = d.readUInt32LE();

const size_t base = d.offset();
d.seek(boneOfs + ofs);

this->md21Ofs = ofs;

// Build M2Sequence vector from animations for read_m2_track
std::vector<M2Sequence> sequences(this->animations.size());
for (size_t i = 0; i < this->animations.size(); i++)
sequences[i] = { this->animations[i].flags };

// store offsets for lazy .anim patching
this->bones.resize(boneCount);
for (uint32_t i = 0; i < boneCount; i++) {
M2Bone bone;
bone.boneID = d.readInt32LE();
bone.flags = d.readUInt32LE();
bone.parentBone = d.readInt16LE();
bone.subMeshID = d.readUInt16LE();
bone.boneNameCRC = d.readUInt32LE();
bone.translation = read_m2_track(d, ofs, M2DataType::float3, false, this->animFiles, true, &sequences);
bone.rotation = read_m2_track(d, ofs, M2DataType::compquat, false, this->animFiles, true, &sequences);
bone.scale = read_m2_track(d, ofs, M2DataType::float3, false, this->animFiles, true, &sequences);
bone.pivot = d.readFloatLE(3);

// Convert bone transformations coordinate system.
auto& translations = bone.translation.values;
auto& rotations = bone.rotation.values;
auto& scale = bone.scale.values;
auto& pivot = bone.pivot;

for (size_t ai = 0; ai < translations.size(); ai++) {
for (size_t j = 0; j < translations[ai].size(); j++) {
auto& v = std::get<std::vector<float>>(translations[ai][j]);
const float dx = v[0];
const float dy = v[1];
const float dz = v[2];
v[0] = dx;
v[2] = dy * -1;
v[1] = dz;
}
}

for (size_t ai = 0; ai < rotations.size(); ai++) {
for (size_t j = 0; j < rotations[ai].size(); j++) {
auto& v = std::get<std::vector<float>>(rotations[ai][j]);
const float dx = v[0];
const float dy = v[1];
const float dz = v[2];
const float dw = v[3];
v[0] = dx;
v[2] = dy * -1;
v[1] = dz;
v[3] = dw;
}
}

for (size_t ai = 0; ai < scale.size(); ai++) {
for (size_t j = 0; j < scale[ai].size(); j++) {
auto& v = std::get<std::vector<float>>(scale[ai][j]);
const float dx = v[0];
const float dy = v[1];
const float dz = v[2];
v[0] = dx;
v[2] = dy;
v[1] = dz;
}
}

{
const float pivotX = pivot[0];
const float pivotY = pivot[1];
const float pivotZ = pivot[2];
pivot[0] = pivotX;
pivot[2] = pivotY * -1;
pivot[1] = pivotZ;
}

this->bones[i] = std::move(bone);
}

d.seek(base);
}

/**
 * Parse collision data from an MD21 chunk.
 */
void M2Loader::parseChunk_MD21_collision(uint32_t ofs) {
// Parse collision boxes before the full collision chunk.
this->boundingBox = read_caa_bb(this->data);
this->boundingSphereRadius = this->data.readFloatLE();
this->collisionBox = read_caa_bb(this->data);
this->collisionSphereRadius = this->data.readFloatLE();

const uint32_t indicesCount = this->data.readUInt32LE();
const uint32_t indicesOfs = this->data.readUInt32LE();

const uint32_t positionsCount = this->data.readUInt32LE();
const uint32_t positionsOfs = this->data.readUInt32LE();

const uint32_t normalsCount = this->data.readUInt32LE();
const uint32_t normalsOfs = this->data.readUInt32LE();

const size_t base = this->data.offset();

// indices
this->data.seek(indicesOfs + ofs);
this->collisionIndices = this->data.readUInt16LE(indicesCount);

// Positions
this->data.seek(positionsOfs + ofs);
this->collisionPositions.resize(positionsCount * 3);
for (uint32_t i = 0; i < positionsCount; i++) {
const uint32_t index = i * 3;
this->collisionPositions[index] = this->data.readFloatLE();
this->collisionPositions[index + 2] = this->data.readFloatLE() * -1;
this->collisionPositions[index + 1] = this->data.readFloatLE();
}

// Normals
this->data.seek(normalsOfs + ofs);
this->collisionNormals.resize(normalsCount * 3);
for (uint32_t i = 0; i < normalsCount; i++) {
const uint32_t index = i * 3;
this->collisionNormals[index] = this->data.readFloatLE();
this->collisionNormals[index + 2] = this->data.readFloatLE() * -1;
this->collisionNormals[index + 1] = this->data.readFloatLE();
}

this->data.seek(base);
}

/**
 * Parse attachments data from an MD21 chunk.
 */
void M2Loader::parseChunk_MD21_attachments(uint32_t ofs) {
const uint32_t attachmentCount = this->data.readUInt32LE();
const uint32_t attachmentOffset = this->data.readUInt32LE();

const size_t base = this->data.offset();
this->data.seek(attachmentOffset + ofs);

// Build sequences for read_m2_track
std::vector<M2Sequence> sequences(this->animations.size());
for (size_t i = 0; i < this->animations.size(); i++)
sequences[i] = { this->animations[i].flags };

this->attachments.resize(attachmentCount);
for (uint32_t i = 0; i < attachmentCount; i++) {
M2Attachment& att = this->attachments[i];
att.id = this->data.readUInt32LE();
att.bone = this->data.readUInt16LE();
att.unknown = this->data.readUInt16LE();
att.position = this->data.readFloatLE(3);
std::map<uint32_t, BufferWrapper*> emptyAnimFiles;
att.animateAttached = read_m2_track(this->data, this->md21Ofs, M2DataType::uint8, false, emptyAnimFiles, false, &sequences);
}

this->data.seek(base);
}

/**
 * Parse attachment lookup table from an MD21 chunk.
 */
void M2Loader::parseChunk_MD21_attachmentLookup(uint32_t ofs) {
const uint32_t lookupCount = this->data.readUInt32LE();
const uint32_t lookupOfs = this->data.readUInt32LE();

const size_t base = this->data.offset();
this->data.seek(lookupOfs + ofs);

this->attachmentLookup = this->data.readInt16LE(lookupCount);

this->data.seek(base);
}

/**
 * Get attachment by attachment ID (e.g., 11 for helmet).
 */
const M2Attachment* M2Loader::getAttachmentById(uint32_t attachmentId) const {
if (this->attachmentLookup.empty() || attachmentId >= this->attachmentLookup.size())
return nullptr;

const int16_t index = this->attachmentLookup[attachmentId];
if (index < 0 || static_cast<size_t>(index) >= this->attachments.size())
return nullptr;

return &this->attachments[static_cast<size_t>(index)];
}

/**
 * Parse replaceable texture lookups from an MD21 chunk.
 */
void M2Loader::parseChunk_MD21_replaceableTextureLookup(uint32_t ofs) {
const uint32_t lookupCount = this->data.readUInt32LE();
const uint32_t lookupOfs = this->data.readUInt32LE();

const size_t base = this->data.offset();
this->data.seek(lookupOfs + ofs);

this->replaceableTextureLookup = this->data.readInt16LE(lookupCount);

this->data.seek(base);
}

/**
 * Parse material meta-data from an MD21 chunk.
 */
void M2Loader::parseChunk_MD21_materials(uint32_t ofs) {
const uint32_t materialCount = this->data.readUInt32LE();
const uint32_t materialOfs = this->data.readUInt32LE();

const size_t base = this->data.offset();
this->data.seek(materialOfs + ofs);

this->materials.resize(materialCount);
for (uint32_t i = 0; i < materialCount; i++) {
this->materials[i].flags = this->data.readUInt16LE();
this->materials[i].blendingMode = this->data.readUInt16LE();
}

this->data.seek(base);
}

/**
 * Parse the model name from an MD21 chunk.
 */
void M2Loader::parseChunk_MD21_modelName(uint32_t ofs) {
const uint32_t modelNameLength = this->data.readUInt32LE();
const uint32_t modelNameOfs = this->data.readUInt32LE();

const size_t base = this->data.offset();
this->data.seek(modelNameOfs + ofs);

// Always followed by single 0x0 character, -1 to trim).
this->data.seek(modelNameOfs + ofs);
this->name = this->data.readString(modelNameLength - 1);

this->data.seek(base);
}

/**
 * Parse vertices from an MD21 chunk.
 */
void M2Loader::parseChunk_MD21_vertices(uint32_t ofs) {
const uint32_t verticesCount = this->data.readUInt32LE();
const uint32_t verticesOfs = this->data.readUInt32LE();

const size_t base = this->data.offset();
this->data.seek(verticesOfs + ofs);

// Read vertices.
this->vertices.resize(verticesCount * 3);
this->normals.resize(verticesCount * 3);
this->uv.resize(verticesCount * 2);
this->uv2.resize(verticesCount * 2);
this->boneWeights.resize(verticesCount * 4);
this->boneIndices.resize(verticesCount * 4);

for (uint32_t i = 0; i < verticesCount; i++) {
this->vertices[i * 3] = this->data.readFloatLE();
this->vertices[i * 3 + 2] = this->data.readFloatLE() * -1;
this->vertices[i * 3 + 1] = this->data.readFloatLE();

for (int x = 0; x < 4; x++)
this->boneWeights[i * 4 + x] = this->data.readUInt8();

for (int x = 0; x < 4; x++)
this->boneIndices[i * 4 + x] = this->data.readUInt8();

this->normals[i * 3] = this->data.readFloatLE();
this->normals[i * 3 + 2] = this->data.readFloatLE() * -1;
this->normals[i * 3 + 1] = this->data.readFloatLE();

this->uv[i * 2] = this->data.readFloatLE();
this->uv[i * 2 + 1] = (this->data.readFloatLE() - 1) * -1;

this->uv2[i * 2] = this->data.readFloatLE();
this->uv2[i * 2 + 1] = (this->data.readFloatLE() - 1) * -1;
}

this->data.seek(base);
}

/**
 * Parse texture transformation definitions from an MD21 chunk.
 */
void M2Loader::parseChunk_MD21_textureTransforms(uint32_t ofs) {
const uint32_t transformCount = this->data.readUInt32LE();
const uint32_t transformOfs = this->data.readUInt32LE();

const size_t base = this->data.offset();
this->data.seek(transformOfs + ofs);

// Build sequences for read_m2_track
std::vector<M2Sequence> sequences(this->animations.size());
for (size_t i = 0; i < this->animations.size(); i++)
sequences[i] = { this->animations[i].flags };

this->textureTransforms.resize(transformCount);
for (uint32_t i = 0; i < transformCount; i++) {
std::map<uint32_t, BufferWrapper*> emptyAnimFiles;
this->textureTransforms[i].translation = read_m2_track(this->data, this->md21Ofs, M2DataType::float3, false, emptyAnimFiles, false, &sequences);
this->textureTransforms[i].rotation = read_m2_track(this->data, this->md21Ofs, M2DataType::float4, false, emptyAnimFiles, false, &sequences);
this->textureTransforms[i].scaling = read_m2_track(this->data, this->md21Ofs, M2DataType::float3, false, emptyAnimFiles, false, &sequences);
}

this->data.seek(base);
}

/**
 * Parse texture transform lookup table from an MD21 chunk.
 */
void M2Loader::parseChunk_MD21_textureTransformLookup(uint32_t ofs) {
const uint32_t entryCount = this->data.readUInt32LE();
const uint32_t entryOfs = this->data.readUInt32LE();

const size_t base = this->data.offset();
this->data.seek(entryOfs + ofs);

this->textureTransformsLookup.resize(entryCount);
for (uint32_t i = 0; i < entryCount; i++)
this->textureTransformsLookup[i] = this->data.readUInt16LE();

this->data.seek(base);
}

/**
 * Parse transparency lookup table from an MD21 chunk.
 */
void M2Loader::parseChunk_MD21_transparencyLookup(uint32_t ofs) {
const uint32_t entryCount = this->data.readUInt32LE();
const uint32_t entryOfs = this->data.readUInt32LE();

const size_t base = this->data.offset();
this->data.seek(entryOfs + ofs);

this->transparencyLookup.resize(entryCount);
for (uint32_t i = 0; i < entryCount; i++)
this->transparencyLookup[i] = this->data.readUInt16LE();

this->data.seek(base);
}

/**
 * Parse global transparency weights from an MD21 chunk.
 */
void M2Loader::parseChunk_MD21_textureWeights(uint32_t ofs) {
const uint32_t weightCount = this->data.readUInt32LE();
const uint32_t weightOfs = this->data.readUInt32LE();

const size_t base = this->data.offset();
this->data.seek(weightOfs + ofs);

// Build sequences for read_m2_track
std::vector<M2Sequence> sequences(this->animations.size());
for (size_t i = 0; i < this->animations.size(); i++)
sequences[i] = { this->animations[i].flags };

this->textureWeights.resize(weightCount);
for (uint32_t i = 0; i < weightCount; i++) {
std::map<uint32_t, BufferWrapper*> emptyAnimFiles;
this->textureWeights[i] = read_m2_track(this->data, this->md21Ofs, M2DataType::int16, false, emptyAnimFiles, false, &sequences);
}

this->data.seek(base);
}

/**
 * Parse color/transparency data from an MD21 chunk.
 */
void M2Loader::parseChunk_MD21_colors(uint32_t ofs) {
const uint32_t colorsCount = this->data.readUInt32LE();
const uint32_t colorsOfs = this->data.readUInt32LE();

const size_t base = this->data.offset();
this->data.seek(colorsOfs + ofs);

// Build sequences for read_m2_track
std::vector<M2Sequence> sequences(this->animations.size());
for (size_t i = 0; i < this->animations.size(); i++)
sequences[i] = { this->animations[i].flags };

this->colors.resize(colorsCount);
for (uint32_t i = 0; i < colorsCount; i++) {
std::map<uint32_t, BufferWrapper*> emptyAnimFiles;
this->colors[i].color = read_m2_track(this->data, this->md21Ofs, M2DataType::float3, false, emptyAnimFiles, false, &sequences);
this->colors[i].alpha = read_m2_track(this->data, this->md21Ofs, M2DataType::int16, false, emptyAnimFiles, false, &sequences);
}

this->data.seek(base);
}

/**
 * Parse textures from an MD21 chunk.
 */
void M2Loader::parseChunk_MD21_textures(uint32_t ofs) {
const uint32_t texturesCount = this->data.readUInt32LE();
const uint32_t texturesOfs = this->data.readUInt32LE();

const size_t base = this->data.offset();
this->data.seek(texturesOfs + ofs);

// Read textures.
this->textures.reserve(texturesCount);
this->textureTypes.resize(texturesCount);

for (uint32_t i = 0; i < texturesCount; i++) {
const uint32_t textureType = this->data.readUInt32LE();
this->textureTypes[i] = textureType;
Texture texture(this->data.readUInt32LE());

const uint32_t nameLength = this->data.readUInt32LE();
const uint32_t nameOfs = this->data.readUInt32LE();

// Check if texture has a filename (legacy).
if (textureType == 0 && nameOfs > 0) {
const size_t pos = this->data.offset();

this->data.seek(nameOfs);
std::string fileName = this->data.readString(nameLength);
// Remove NULL characters.
fileName.erase(std::remove(fileName.begin(), fileName.end(), '\0'), fileName.end());

if (!fileName.empty())
texture.setFileName(fileName);

this->data.seek(pos);
}

this->textures.push_back(std::move(texture));
}

this->data.seek(base);
}

/**
 * Parse texture combos from an MD21 chunk.
 */
void M2Loader::parseChunk_MD21_textureCombos(uint32_t ofs) {
const uint32_t textureComboCount = this->data.readUInt32LE();
const uint32_t textureComboOfs = this->data.readUInt32LE();

const size_t base = this->data.offset();
this->data.seek(textureComboOfs + ofs);
this->textureCombos = this->data.readUInt16LE(textureComboCount);
this->data.seek(base);
}

/**
 * Parse animations.
 */
void M2Loader::parseChunk_MD21_animations(uint32_t ofs) {
const uint32_t animationCount = this->data.readUInt32LE();
const uint32_t animationOfs = this->data.readUInt32LE();

const size_t base = this->data.offset();
this->data.seek(animationOfs + ofs);

this->animations.resize(animationCount);
for (uint32_t i = 0; i < animationCount; i++) {
M2Animation& anim = this->animations[i];
anim.id = this->data.readUInt16LE();
anim.variationIndex = this->data.readUInt16LE();
anim.duration = this->data.readUInt32LE();
anim.movespeed = this->data.readFloatLE();
anim.flags = this->data.readUInt32LE();
anim.frequency = this->data.readInt16LE();
anim.padding = this->data.readUInt16LE();
anim.replayMin = this->data.readUInt32LE();
anim.replayMax = this->data.readUInt32LE();
anim.blendTimeIn = this->data.readUInt16LE();
anim.blendTimeOut = this->data.readUInt16LE();
anim.boxPosMin = this->data.readFloatLE(3);
anim.boxPosMax = this->data.readFloatLE(3);
anim.boxRadius = this->data.readFloatLE();
anim.variationNext = this->data.readInt16LE();
anim.aliasNext = this->data.readUInt16LE();
}

this->data.seek(base);
}

/**
 * Parse animation lookup.
 */
void M2Loader::parseChunk_MD21_animationLookup(uint32_t ofs) {
const uint32_t animationLookupCount = this->data.readUInt32LE();
const uint32_t animationLookupOfs = this->data.readUInt32LE();

const size_t base = this->data.offset();
this->data.seek(animationLookupOfs + ofs);

this->animationLookup = this->data.readInt16LE(animationLookupCount);

this->data.seek(base);
}

/**
 * Parse global loops.
 */
void M2Loader::parseChunk_MD21_globalLoops(uint32_t ofs) {
const uint32_t globalLoopCount = this->data.readUInt32LE();
const uint32_t globalLoopOfs = this->data.readUInt32LE();

const size_t base = this->data.offset();
this->data.seek(globalLoopOfs + ofs);

this->globalLoops = this->data.readInt16LE(globalLoopCount);

this->data.seek(base);
}
