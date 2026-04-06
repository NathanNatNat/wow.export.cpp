/*!
wow.export (https://github.com/Kruithne/wow.export)
Authors: Kruithne <kruithne@gmail.com>, Marlamin <marlamin@marlamin.com>
License: MIT
 */
#include "GLTFWriter.h"
#include "GLBWriter.h"
#include "../BoneMapper.h"
#include "../AnimMapper.h"
#include "../../core.h"
#include "../../generics.h"
#include "../../casc/export-helper.h"
#include "../../log.h"
#include "../../constants.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <algorithm>
#include <array>

// See https://gist.github.com/mhenry07/e31d8c94db91fb823f2eed2fc1b43f15
static constexpr int GLTF_ARRAY_BUFFER = 0x8892;
static constexpr int GLTF_ELEMENT_ARRAY_BUFFER = 0x8893;

static constexpr int GLTF_UNSIGNED_BYTE = 0x1401;
static constexpr int GLTF_UNSIGNED_SHORT = 0x1403;
static constexpr int GLTF_UNSIGNED_INT = 0x1405;
static constexpr int GLTF_FLOAT = 0x1406;

static constexpr int GLTF_TRIANGLES = 0x0004;

/**
 * Calculate the minimum/maximum values of an array buffer.
 * @param values Data values
 * @param stride Number of components per element
 * @param target JSON accessor to set min/max on
 */
template<typename T>
static void calculate_min_max(const std::vector<T>& values, int stride, nlohmann::json& target) {
if (values.empty()) return;

std::vector<T> min_vals(stride);
std::vector<T> max_vals(stride);
bool first = true;

for (size_t i = 0; i < values.size(); i += static_cast<size_t>(stride)) {
for (int ofs = 0; ofs < stride; ofs++) {
const T value = values[i + ofs];

if (first || value < min_vals[ofs])
min_vals[ofs] = value;

if (first || value > max_vals[ofs])
max_vals[ofs] = value;
}
first = false;
}

nlohmann::json min_json = nlohmann::json::array();
nlohmann::json max_json = nlohmann::json::array();
for (int i = 0; i < stride; i++) {
min_json.push_back(min_vals[i]);
max_json.push_back(max_vals[i]);
}
target["min"] = std::move(min_json);
target["max"] = std::move(max_json);
}

/**
 * Transform Vec3 to Mat4x4
 * @param v Vector3
 * @returns Mat4x4
 */
static std::array<float, 16> vec3_to_mat4x4(const std::vector<float>& v) {
return {
1, 0, 0, 0,
0, 1, 0, 0,
0, 0, 1, 0,
v[0] * -1, v[1] * -1, v[2] * -1, 1
};
}

GLTFWriter::GLTFWriter(const std::filesystem::path& out, const std::string& name)
: out(out), name(name) {
}

/**
 * Set the texture map used for this writer.
 * @param textures
 */
void GLTFWriter::setTextureMap(const std::map<uint32_t, GLTFTextureEntry>& textures) {
this->textures = textures;
}

/**
 * Set the texture buffers for embedding in GLB.
 * @param texture_buffers
 */
void GLTFWriter::setTextureBuffers(const std::map<uint32_t, BufferWrapper>& texture_buffers) {
this->texture_buffers = texture_buffers;
}

/**
 * Set the bones array for this writer.
 * @param bones
 */
void GLTFWriter::setBonesArray(const std::vector<GLTFBone>& bones) {
this->bones = bones;
}

/**
 * Set the vertices array for this writer.
 * @param vertices
 */
void GLTFWriter::setVerticesArray(const std::vector<float>& vertices) {
this->vertices = vertices;
}

/**
 * Set the normals array for this writer.
 * @param normals
 */
void GLTFWriter::setNormalArray(const std::vector<float>& normals) {
this->normals = normals;
}

/**
 * Add a UV array for this writer.
 * @param uvs
 */
void GLTFWriter::addUVArray(const std::vector<float>& uvs) {
this->uvs.push_back(uvs);
}

/**
 * Set the bone weights array for this writer.
 * @param boneWeights
 */
void GLTFWriter::setBoneWeightArray(const std::vector<uint8_t>& boneWeights) {
this->boneWeights = boneWeights;
}

/**
 * Set the bone indicies array for this writer.
 * @param boneIndices
 */
void GLTFWriter::setBoneIndexArray(const std::vector<uint8_t>& boneIndices) {
this->boneIndices = boneIndices;
}

/**
 * Set the animations array for this writer.
 * @param animations
 */
void GLTFWriter::setAnimations(const std::vector<GLTFAnimation>& animations) {
this->animations = animations;
}

/**
 * Add a mesh to this writer.
 * @param name
 * @param triangles
 * @param matName
 */
void GLTFWriter::addMesh(const std::string& name, const std::vector<uint32_t>& triangles, const std::string& matName) {
meshes.push_back({ name, triangles, matName });
}

/**
 * Add an equipment model to be exported alongside the main model.
 * Equipment shares the main model's skeleton via bone index remapping.
 * @param equip - Equipment data
 * @param equip.name - Equipment name for mesh naming
 * @param equip.vertices - Vertex positions
 * @param equip.normals - Vertex normals
 * @param equip.uv - UV coordinates
 * @param equip.uv2 - Secondary UV coordinates (optional)
 * @param equip.boneIndices - Bone indices (remapped to char skeleton)
 * @param equip.boneWeights - Bone weights
 * @param equip.meshes - Array of {name, triangles, matName}
 * @param equip.attachment_bone - Bone index for attachment (non-skinned equipment)
 */
void GLTFWriter::addEquipmentModel(const GLTFEquipmentModel& equip) {
equipment_models.push_back(equip);
}

void GLTFWriter::write(bool overwrite, const std::string& format) {
const std::filesystem::path outGLTF = casc::ExportHelper::replaceExtension(out.string(), format == "glb" ? ".glb" : ".gltf");
const std::filesystem::path outBIN = casc::ExportHelper::replaceExtension(out.string(), ".bin");

const auto out_dir = outGLTF.parent_path();
const bool use_absolute = core::view->config.value("enableAbsoluteGLTFPaths", false);

// If overwriting is disabled, check file existence.
if (!overwrite && generics::fileExists(outGLTF))
return;

if (!overwrite && format == "gltf" && generics::fileExists(outBIN))
return;

const std::string generator = std::string("wow.export v") + std::string(constants::VERSION);
nlohmann::json root = {
{"asset", {
{"version", "2.0"},
{"generator", generator}
}},
{"nodes", nlohmann::json::array({
{
{"name", name},
{"children", nlohmann::json::array()}
}
})},
{"scenes", nlohmann::json::array({
{
{"name", name + "_Scene"},
{"nodes", nlohmann::json::array({0})}
}
})},
{"buffers", nlohmann::json::array({
{
{"byteLength", 0}
}
})},
{"bufferViews", nlohmann::json::array({
{
// Vertices ARRAY_BUFFER
{"buffer", 0},
{"byteLength", 0},
{"byteOffset", 0},
{"target", GLTF_ARRAY_BUFFER}
},
{
// Normals ARRAY_BUFFER
{"buffer", 0},
{"byteLength", 0},
{"byteOffset", 0},
{"target", GLTF_ARRAY_BUFFER}
}
})},
{"accessors", nlohmann::json::array({
{
// Vertices (Float)
{"name", "POSITION"},
{"bufferView", 0},
{"byteOffset", 0},
{"componentType", GLTF_FLOAT},
{"count", 0},
{"type", "VEC3"}
},
{
// Normals (Float)
{"name", "NORMAL"},
{"bufferView", 1},
{"byteOffset", 0},
{"componentType", GLTF_FLOAT},
{"count", 0},
{"type", "VEC3"}
}
})},
{"meshes", nlohmann::json::array()},
{"scene", 0}
};

nlohmann::json primitive_attributes = {
{"POSITION", 0},
{"NORMAL", 1}
};

auto add_scene_node = [&](nlohmann::json node) -> size_t {
root["nodes"].push_back(std::move(node));
size_t idx = root["nodes"].size() - 1;
root["nodes"][0]["children"].push_back(static_cast<int>(idx));
return idx;
};

auto add_buffered_accessor = [&](nlohmann::json accessor, int buffer_target, bool add_primitive = false) -> int {
nlohmann::json bv = {
{"buffer", 0},
{"byteLength", 0},
{"byteOffset", 0}
};
if (buffer_target >= 0)
bv["target"] = buffer_target;

root["bufferViews"].push_back(std::move(bv));
int buffer_idx = static_cast<int>(root["bufferViews"].size() - 1);

if (add_primitive)
primitive_attributes[accessor["name"].get<std::string>()] = buffer_idx;

accessor["bufferView"] = buffer_idx;
root["accessors"].push_back(std::move(accessor));
return static_cast<int>(root["accessors"].size() - 1);
};

auto& nodes = root["nodes"];
const auto& bones_ref = this->bones;

int idx_inv_bind = -1;
int idx_bone_joints = -1;
int idx_bone_weights = -1;
std::map<std::string, BufferWrapper> animationBufferMap;

if (!bones_ref.empty()) {
idx_bone_joints = add_buffered_accessor({
// Bone joints/indices (Byte)
{"name", "JOINTS_0"},
{"byteOffset", 0},
{"componentType", GLTF_UNSIGNED_BYTE},
{"count", 0},
{"type", "VEC4"}
}, GLTF_ARRAY_BUFFER, true);

idx_bone_weights = add_buffered_accessor({
// Bone weights (Byte)
{"name", "WEIGHTS_0"},
{"byteOffset", 0},
{"componentType", GLTF_UNSIGNED_BYTE},
{"count", 0},
{"normalized", true},
{"type", "VEC4"}
}, GLTF_ARRAY_BUFFER, true);

idx_inv_bind = add_buffered_accessor({
// Inverse matrices (Float)
{"name", "INV_BIND_MATRICES"},
{"byteOffset", 0},
{"componentType", GLTF_FLOAT},
{"count", 0},
{"type", "MAT4"}
}, -1); // undefined target

root["skins"] = nlohmann::json::array({{
{"name", name + "_Armature"},
{"joints", nlohmann::json::array()},
{"inverseBindMatrices", idx_inv_bind},
{"skeleton", 0}
}});

size_t skeleton_idx = add_scene_node({
{"name", name + "_skeleton"},
{"children", nlohmann::json::array()}
});

std::map<int, size_t> bone_lookup_map;

std::map<std::string, int> animation_buffer_lookup_map;

if (core::view->config.value("modelsExportAnimations", false)) {
for (size_t animationIndex = 0; animationIndex < animations.size(); animationIndex++) {
size_t requiredBufferSize = 0;
for (const auto& bone : this->bones) {
// Timestamps are all floats (uints originally), so 4 bytes each.
for (size_t i = 0; i < bone.translation.timestamps.size(); i++) {
if (i == animationIndex && bone.translation.interpolation < 2) {
requiredBufferSize += bone.translation.timestamps[i].size() * 4;
break;
}
}

for (size_t i = 0; i < bone.rotation.timestamps.size(); i++) {
if (i == animationIndex && bone.rotation.interpolation < 2) {
requiredBufferSize += bone.rotation.timestamps[i].size() * 4;
break;
}
}

for (size_t i = 0; i < bone.scale.timestamps.size(); i++) {
if (i == animationIndex && bone.scale.interpolation < 2) {
requiredBufferSize += bone.scale.timestamps[i].size() * 4;
break;
}
}

// Vector3 values
for (size_t i = 0; i < bone.translation.values.size(); i++) {
if (i == animationIndex && bone.translation.interpolation < 2) {
requiredBufferSize += bone.translation.values[i].size() * 3 * 4;
break;
}
}

for (size_t i = 0; i < bone.scale.values.size(); i++) {
if (i == animationIndex && bone.scale.interpolation < 2) {
requiredBufferSize += bone.scale.values[i].size() * 3 * 4;
break;
}
}

// Quaternion values
for (size_t i = 0; i < bone.rotation.values.size(); i++) {
if (i == animationIndex && bone.rotation.interpolation < 2) {
requiredBufferSize += bone.rotation.values[i].size() * 4 * 4;
break;
}
}
}

if (requiredBufferSize > 0) {
std::string animKey = std::to_string(animations[animationIndex].id) + "-" + std::to_string(animations[animationIndex].variationIndex);
animationBufferMap.emplace(animKey, BufferWrapper::alloc(requiredBufferSize, true));

if (format == "glb") {
// glb mode: animations go into buffer 0 (main binary chunk)
animation_buffer_lookup_map[animKey] = 0;
} else {
// gltf mode: animations get separate buffer files
root["buffers"].push_back({
{"uri", outBIN.stem().string() + "_anim" + animKey + ".bin"},
{"byteLength", requiredBufferSize}
});
animation_buffer_lookup_map[animKey] = static_cast<int>(root["buffers"].size() - 1);
}
}
}

// Animations
root["animations"] = nlohmann::json::array();

for (const auto& animation : animations) {
root["animations"].push_back({
{"samplers", nlohmann::json::array()},
{"channels", nlohmann::json::array()},
{"name", get_anim_name(animation.id) + " (ID " + std::to_string(animation.id) + " variation " + std::to_string(animation.variationIndex) + ")"}
});
}
}

// Add bone nodes.
for (size_t bi = 0; bi < bones_ref.size(); bi++) {
const size_t nodeIndex = nodes.size();
const auto& bone = bones_ref[bi];

float parent_pos[3] = {0, 0, 0};
if (bone.parentBone > -1) {
const auto& parent_bone = bones_ref[bone.parentBone];
parent_pos[0] = parent_bone.pivot[0];
parent_pos[1] = parent_bone.pivot[1];
parent_pos[2] = parent_bone.pivot[2];

size_t parent_node_idx = bone_lookup_map[bone.parentBone];
auto& parent_node = nodes[parent_node_idx];
if (!parent_node.contains("children"))
parent_node["children"] = nlohmann::json::array();
parent_node["children"].push_back(static_cast<int>(nodeIndex));
} else {
// Parent stray bones to the skeleton root.
nodes[skeleton_idx]["children"].push_back(static_cast<int>(nodeIndex));
}

const std::string bone_name = get_bone_name(bone.boneID, static_cast<int>(bi), bone.boneNameCRC);

const bool usePrefix = core::view->config.value("modelsExportWithBonePrefix", false);

nlohmann::json prefix_node = {
{"name", bone_name + "_p"},
{"translation", nlohmann::json::array({
bone.pivot[0] - parent_pos[0],
bone.pivot[1] - parent_pos[1],
bone.pivot[2] - parent_pos[2]
})},
{"children", nlohmann::json::array({static_cast<int>(nodeIndex + 1)})}
};

// Define how node acts, if we don't use prefixes we need to add position translation
nlohmann::json node;
if (usePrefix) {
node = {{"name", bone_name}};
} else {
node = {
{"name", bone_name},
{"translation", nlohmann::json::array({
bone.pivot[0] - parent_pos[0],
bone.pivot[1] - parent_pos[1],
bone.pivot[2] - parent_pos[2]
})}
};
}

size_t actual_node_idx;
if (usePrefix) {
nodes.push_back(prefix_node);
actual_node_idx = nodes.size();
nodes.push_back(node);
} else {
actual_node_idx = nodes.size();
nodes.push_back(node);
}

bone_lookup_map[static_cast<int>(bi)] = actual_node_idx;

auto mat = vec3_to_mat4x4(bone.pivot);
inverseBindMatrices.insert(inverseBindMatrices.end(), mat.begin(), mat.end());

// We need to wrap this in ifelse or we will create race condition due to the node push above (what)
if (usePrefix) {
root["skins"][0]["joints"].push_back(static_cast<int>(nodeIndex + 1));
} else {
//Don't do +1 if we remove the prefix nodes
//https://github.com/Kruithne/wow.export/commit/7a19dcb60ff20b5ca1e2b2f83b6c10ae0afcf5a2#diff-e1681bb244fd61a8a3840e513733c6d99e50b715768f2971a87200d2abd86152L291-L304
root["skins"][0]["joints"].push_back(static_cast<int>(nodeIndex));
}


// Skip rest of the bone logic if we're not exporting animations.
if (!core::view->config.value("modelsExportAnimations", false))
continue;

// Check interpolation, right now we only support NONE (0, hopefully matches glTF STEP), LINEAR (1). The rest (2 - bezier spline, 3 - hermite spline) will require... well, math.
if (bone.translation.interpolation < 2) {
// Sanity check -- check if the timestamps/values array are the same length as the amount of animations.
if (!bone.translation.timestamps.empty() && bone.translation.timestamps.size() != animations.size()) {
logging::write("timestamps array length (" + std::to_string(bone.translation.timestamps.size()) + ") does not match the amount of animations (" + std::to_string(animations.size()) + "), skipping bone " + std::to_string(bi));
continue;
}

if (!bone.translation.values.empty() && bone.translation.values.size() != animations.size()) {
logging::write("values array length (" + std::to_string(bone.translation.timestamps.size()) + ") does not match the amount of animations (" + std::to_string(animations.size()) + "), skipping bone " + std::to_string(bi));
continue;
}

// TIMESTAMPS
for (size_t i = 0; i < bone.translation.timestamps.size(); i++) {
if (bone.translation.timestamps[i].empty())
continue;

const std::string animName = std::to_string(animations[i].id) + "-" + std::to_string(animations[i].variationIndex);
auto& animationBuffer = animationBufferMap.at(animName);

// pair timestamps with values and sort to maintain gltf 2.0 spec compliance
const uint32_t anim_duration = animations[i].duration;
struct TimedValue { float time; const std::vector<float>* value; };
std::vector<TimedValue> paired;
for (size_t j = 0; j < bone.translation.timestamps[i].size(); j++) {
const uint32_t raw_ts = bone.translation.timestamps[i][j];
uint32_t norm_ts = raw_ts;
if (anim_duration > 0) {
norm_ts = raw_ts % anim_duration;
// preserve end-of-loop keyframe instead of wrapping to 0
if (norm_ts == 0 && raw_ts > 0)
norm_ts = anim_duration;
}
const float time = static_cast<float>(norm_ts) / 1000.0f;
paired.push_back({time, &bone.translation.values[i][j]});
}

// sort by time to ensure strictly increasing timestamps (required by gltf 2.0 spec)
std::sort(paired.begin(), paired.end(), [](const TimedValue& a, const TimedValue& b) {
return a.time < b.time;
});

// Add new bufferView for bone timestamps.
// note: byteOffset stored here is relative to animation buffer, will be updated later for glb
root["bufferViews"].push_back({
{"buffer", animation_buffer_lookup_map.at(animName)},
{"byteLength", static_cast<int>(paired.size() * 4)},
{"byteOffset", animationBuffer.offset()},
{"name", "TRANS_TIMESTAMPS_" + std::to_string(bi) + "_" + std::to_string(i)}
});

root["animations"][i]["samplers"].push_back({
{"input", 0}, // Timestamps accessor index is set later
{"interpolation", bone.translation.interpolation == 0 ? "STEP" : "LINEAR"},
{"output", 0} // Values accessor index is set later
});

float time_min = 0;
float time_max = static_cast<float>(anim_duration) / 1000.0f;

for (const auto& entry : paired)
animationBuffer.writeFloatLE(entry.time);

// Add new SCALAR accessor for this bone's translation timestamps as floats.
root["accessors"].push_back({
{"name", "TRANS_TIMESTAMPS_" + std::to_string(bi) + "_" + std::to_string(i)},
{"bufferView", static_cast<int>(root["bufferViews"].size() - 1)},
{"byteOffset", 0},
{"type", "SCALAR"},
{"componentType", 5126}, // Float
{"min", nlohmann::json::array({time_min})},
{"max", nlohmann::json::array({time_max})}
});

root["animations"][i]["samplers"].back()["input"] = static_cast<int>(root["accessors"].size() - 1);

root["accessors"].back()["count"] = static_cast<int>(paired.size());

// VALUES
// Add new bufferView for bone timestamps.
root["bufferViews"].push_back({
{"buffer", animation_buffer_lookup_map.at(animName)},
{"byteLength", static_cast<int>(paired.size() * 3 * 4)},
{"byteOffset", animationBuffer.offset()},
{"name", "TRANS_VALUES_" + std::to_string(bi) + "_" + std::to_string(i)}
});

// Write out bone values to buffer in sorted order
float val_min[3] = {9999999, 9999999, 9999999};
float val_max[3] = {-9999999, -9999999, -9999999};
for (const auto& entry : paired) {
const auto& v = *entry.value;
animationBuffer.writeFloatLE(v[0]);
animationBuffer.writeFloatLE(v[1]);
animationBuffer.writeFloatLE(v[2]);

if (v[0] < val_min[0]) val_min[0] = v[0];
if (v[1] < val_min[1]) val_min[1] = v[1];
if (v[2] < val_min[2]) val_min[2] = v[2];
if (v[0] > val_max[0]) val_max[0] = v[0];
if (v[1] > val_max[1]) val_max[1] = v[1];
if (v[2] > val_max[2]) val_max[2] = v[2];
}

// Add new VEC3 accessor for this bone's translation values.
root["accessors"].push_back({
{"name", "TRANS_VALUES_" + std::to_string(bi) + "_" + std::to_string(i)},
{"bufferView", static_cast<int>(root["bufferViews"].size() - 1)},
{"byteOffset", 0},
{"type", "VEC3"},
{"componentType", 5126}, // Float
{"min", nlohmann::json::array({val_min[0], val_min[1], val_min[2]})},
{"max", nlohmann::json::array({val_max[0], val_max[1], val_max[2]})}
});

root["animations"][i]["samplers"].back()["output"] = static_cast<int>(root["accessors"].size() - 1);

root["accessors"].back()["count"] = static_cast<int>(bone.translation.values[i].size());

root["animations"][i]["channels"].push_back({
{"sampler", static_cast<int>(root["animations"][i]["samplers"].size() - 1)},
{"target", {
{"node", static_cast<int>(nodeIndex + 1)},
{"path", "translation"}
}}
});
}
} else { 
logging::write("Bone " + std::to_string(bi) + " has unsupported interpolation type for translation, skipping.");
}

if (bone.rotation.interpolation < 2) {
// ROTATION
for (size_t i = 0; i < bone.rotation.timestamps.size(); i++) {
if (bone.rotation.timestamps[i].empty())
continue;

const std::string animName = std::to_string(animations[i].id) + "-" + std::to_string(animations[i].variationIndex);
auto& animationBuffer = animationBufferMap.at(animName);

// pair timestamps with values and sort to maintain gltf 2.0 spec compliance
const uint32_t anim_duration = animations[i].duration;
struct TimedValue { float time; const std::vector<float>* value; };
std::vector<TimedValue> paired;
for (size_t j = 0; j < bone.rotation.timestamps[i].size(); j++) {
const uint32_t raw_ts = bone.rotation.timestamps[i][j];
uint32_t norm_ts = raw_ts;
if (anim_duration > 0) {
norm_ts = raw_ts % anim_duration;
// preserve end-of-loop keyframe instead of wrapping to 0
if (norm_ts == 0 && raw_ts > 0)
norm_ts = anim_duration;
}
const float time = static_cast<float>(norm_ts) / 1000.0f;
paired.push_back({time, &bone.rotation.values[i][j]});
}

// sort by time to ensure strictly increasing timestamps (required by gltf 2.0 spec)
std::sort(paired.begin(), paired.end(), [](const TimedValue& a, const TimedValue& b) {
return a.time < b.time;
});

// Add new bufferView for bone timestamps.
root["bufferViews"].push_back({
{"buffer", animation_buffer_lookup_map.at(animName)},
{"byteLength", static_cast<int>(paired.size() * 4)},
{"byteOffset", animationBuffer.offset()},
{"name", "ROT_TIMESTAMPS_" + std::to_string(bi) + "_" + std::to_string(i)}
});

root["animations"][i]["samplers"].push_back({
{"input", 0}, // Timestamps accessor index is set later
{"interpolation", bone.rotation.interpolation == 0 ? "STEP" : "LINEAR"},
{"output", 0} // Values accessor index is set later
});

float time_min = 0;
float time_max = static_cast<float>(anim_duration) / 1000.0f;

for (const auto& entry : paired)
animationBuffer.writeFloatLE(entry.time);

// Add new SCALAR accessor for this bone's rotation timestamps as floats.
root["accessors"].push_back({
{"name", "ROT_TIMESTAMPS_" + std::to_string(bi) + "_" + std::to_string(i)},
{"bufferView", static_cast<int>(root["bufferViews"].size() - 1)},
{"byteOffset", 0},
{"type", "SCALAR"},
{"componentType", 5126}, // Float
{"min", nlohmann::json::array({time_min})},
{"max", nlohmann::json::array({time_max})}
});

root["animations"][i]["samplers"].back()["input"] = static_cast<int>(root["accessors"].size() - 1);

root["accessors"].back()["count"] = static_cast<int>(paired.size());

// VALUES
// Add new bufferView for bone timestamps.
root["bufferViews"].push_back({
{"buffer", animation_buffer_lookup_map.at(animName)},
{"byteLength", static_cast<int>(paired.size() * 4 * 4)},
{"byteOffset", animationBuffer.offset()},
{"name", "ROT_VALUES_" + std::to_string(bi) + "_" + std::to_string(i)}
});

// Write out bone values to buffer in sorted order
float val_min[4] = {9999999, 9999999, 9999999, 9999999};
float val_max[4] = {-9999999, -9999999, -9999999, -9999999};
for (const auto& entry : paired) {
const auto& v = *entry.value;
animationBuffer.writeFloatLE(v[0]);
animationBuffer.writeFloatLE(v[1]);
animationBuffer.writeFloatLE(v[2]);
animationBuffer.writeFloatLE(v[3]);

if (v[0] < val_min[0]) val_min[0] = v[0];
if (v[1] < val_min[1]) val_min[1] = v[1];
if (v[2] < val_min[2]) val_min[2] = v[2];
if (v[3] < val_min[3]) val_min[3] = v[3];
if (v[0] > val_max[0]) val_max[0] = v[0];
if (v[1] > val_max[1]) val_max[1] = v[1];
if (v[2] > val_max[2]) val_max[2] = v[2];
if (v[3] > val_max[3]) val_max[3] = v[3];
}

// Add new VEC3 accessor for this bone's rotation values.
root["accessors"].push_back({
{"name", "ROT_VALUES_" + std::to_string(bi) + "_" + std::to_string(i)},
{"bufferView", static_cast<int>(root["bufferViews"].size() - 1)},
{"byteOffset", 0},
{"type", "VEC4"},
{"componentType", 5126}, // Float
{"min", nlohmann::json::array({val_min[0], val_min[1], val_min[2], val_min[3]})},
{"max", nlohmann::json::array({val_max[0], val_max[1], val_max[2], val_max[3]})}
});

root["animations"][i]["samplers"].back()["output"] = static_cast<int>(root["accessors"].size() - 1);

root["accessors"].back()["count"] = static_cast<int>(bone.rotation.values[i].size());

root["animations"][i]["channels"].push_back({
{"sampler", static_cast<int>(root["animations"][i]["samplers"].size() - 1)},
{"target", {
{"node", static_cast<int>(nodeIndex + 1)},
{"path", "rotation"}
}}
});
}
} else { 
logging::write("Bone " + std::to_string(bi) + " has unsupported interpolation type for rotation, skipping.");
}

if (bone.scale.interpolation < 2) {
// SCALING
for (size_t i = 0; i < bone.scale.timestamps.size(); i++) {
if (bone.scale.timestamps[i].empty())
continue;

const std::string animName = std::to_string(animations[i].id) + "-" + std::to_string(animations[i].variationIndex);
auto& animationBuffer = animationBufferMap.at(animName);

// pair timestamps with values and sort to maintain gltf 2.0 spec compliance
const uint32_t anim_duration = animations[i].duration;
struct TimedValue { float time; const std::vector<float>* value; };
std::vector<TimedValue> paired;
for (size_t j = 0; j < bone.scale.timestamps[i].size(); j++) {
const uint32_t raw_ts = bone.scale.timestamps[i][j];
uint32_t norm_ts = raw_ts;
if (anim_duration > 0) {
norm_ts = raw_ts % anim_duration;
// preserve end-of-loop keyframe instead of wrapping to 0
if (norm_ts == 0 && raw_ts > 0)
norm_ts = anim_duration;
}
const float time = static_cast<float>(norm_ts) / 1000.0f;
paired.push_back({time, &bone.scale.values[i][j]});
}

// sort by time to ensure strictly increasing timestamps (required by gltf 2.0 spec)
std::sort(paired.begin(), paired.end(), [](const TimedValue& a, const TimedValue& b) {
return a.time < b.time;
});

// Add new bufferView for bone timestamps.
root["bufferViews"].push_back({
{"buffer", animation_buffer_lookup_map.at(animName)},
{"byteLength", static_cast<int>(paired.size() * 4)},
{"byteOffset", animationBuffer.offset()},
{"name", "SCALE_TIMESTAMPS_" + std::to_string(bi) + "_" + std::to_string(i)}
});

root["animations"][i]["samplers"].push_back({
{"input", 0}, // Timestamps accessor index is set later
{"interpolation", bone.scale.interpolation == 0 ? "STEP" : "LINEAR"},
{"output", 0} // Values accessor index is set later
});

float time_min = 0;
float time_max = static_cast<float>(anim_duration) / 1000.0f;

for (const auto& entry : paired)
animationBuffer.writeFloatLE(entry.time);

// Add new SCALAR accessor for this bone's scale timestamps as floats.
root["accessors"].push_back({
{"name", "SCALE_TIMESTAMPS_" + std::to_string(bi) + "_" + std::to_string(i)},
{"bufferView", static_cast<int>(root["bufferViews"].size() - 1)},
{"byteOffset", 0},
{"type", "SCALAR"},
{"componentType", 5126}, // Float
{"min", nlohmann::json::array({time_min})},
{"max", nlohmann::json::array({time_max})}
});

root["animations"][i]["samplers"].back()["input"] = static_cast<int>(root["accessors"].size() - 1);

root["accessors"].back()["count"] = static_cast<int>(paired.size());

// VALUES
// Add new bufferView for bone timestamps.
root["bufferViews"].push_back({
{"buffer", animation_buffer_lookup_map.at(animName)},
{"byteLength", static_cast<int>(paired.size() * 3 * 4)},
{"byteOffset", animationBuffer.offset()},
{"name", "SCALE_VALUES_" + std::to_string(bi) + "_" + std::to_string(i)}
});

// Write out bone values to buffer in sorted order
float val_min[3] = {9999999, 9999999, 9999999};
float val_max[3] = {-9999999, -9999999, -9999999};
for (const auto& entry : paired) {
const auto& v = *entry.value;
animationBuffer.writeFloatLE(v[0]);
animationBuffer.writeFloatLE(v[1]);
animationBuffer.writeFloatLE(v[2]);

if (v[0] < val_min[0]) val_min[0] = v[0];
if (v[1] < val_min[1]) val_min[1] = v[1];
if (v[2] < val_min[2]) val_min[2] = v[2];
if (v[0] > val_max[0]) val_max[0] = v[0];
if (v[1] > val_max[1]) val_max[1] = v[1];
if (v[2] > val_max[2]) val_max[2] = v[2];
}

// Add new VEC3 accessor for this bone's scale values.
root["accessors"].push_back({
{"name", "SCALE_VALUES_" + std::to_string(bi) + "_" + std::to_string(i)},
{"bufferView", static_cast<int>(root["bufferViews"].size() - 1)},
{"byteOffset", 0},
{"type", "VEC3"},
{"componentType", 5126}, // Float
{"min", nlohmann::json::array({val_min[0], val_min[1], val_min[2]})},
{"max", nlohmann::json::array({val_max[0], val_max[1], val_max[2]})}
});

root["animations"][i]["samplers"].back()["output"] = static_cast<int>(root["accessors"].size() - 1);

root["accessors"].back()["count"] = static_cast<int>(bone.scale.values[i].size());

root["animations"][i]["channels"].push_back({
{"sampler", static_cast<int>(root["animations"][i]["samplers"].size() - 1)},
{"target", {
{"node", static_cast<int>(nodeIndex + 1)},
{"path", "scale"}
}}
});
}
} else { 
logging::write("Bone " + std::to_string(bi) + " has unsupported interpolation type for scale, skipping.");
}
}
}

if (!textures.empty()) {
root["images"] = nlohmann::json::array();
root["textures"] = nlohmann::json::array();
root["materials"] = nlohmann::json::array();
}

std::map<std::string, int> materialMap;
struct TextureBufferView { uint32_t fileDataID; BufferWrapper buffer; };
std::vector<TextureBufferView> texture_buffer_views;

for (const auto& [fileDataID, texFile] : textures) {
const int imageIndex = static_cast<int>(root["images"].size());
const int textureIndex = static_cast<int>(root["textures"].size());
const int materialIndex = static_cast<int>(root["materials"].size());

if (format == "glb" && texture_buffers.count(fileDataID)) {
// glb mode with embedded textures: use bufferView reference
texture_buffer_views.push_back({fileDataID, texture_buffers.at(fileDataID)});
root["images"].push_back({
{"bufferView", -1},
{"mimeType", "image/png"}
});
} else {
// gltf mode or no buffer: use uri reference
std::string mat_path = texFile.matPathRelative;
if (use_absolute)
mat_path = std::filesystem::absolute(out_dir / mat_path).string();

root["images"].push_back({{"uri", mat_path}});
}

root["textures"].push_back({{"source", imageIndex}});
root["materials"].push_back({
{"name", std::filesystem::path(texFile.matName).stem().string()},
{"emissiveFactor", nlohmann::json::array({0, 0, 0})},
{"pbrMetallicRoughness", {
{"baseColorTexture", {
{"index", textureIndex}
}},
{"metallicFactor", 0}
}}
});

materialMap[texFile.matName] = materialIndex;
}

struct MeshComponentMeta { size_t byte_length; int component_type; };
std::vector<MeshComponentMeta> mesh_component_meta(meshes.size());
for (size_t i = 0, n = meshes.size(); i < n; i++) {
const auto& mesh = meshes[i];

int component_type = GLTF_UNSIGNED_BYTE;
int component_sizeof = 1;
for (const auto idx : mesh.triangles) {
if (idx > 255 && component_type == GLTF_UNSIGNED_BYTE) {
component_type = GLTF_UNSIGNED_SHORT;
component_sizeof = 2;
} else if (idx > 65535 && component_type == GLTF_UNSIGNED_SHORT) {
component_type = GLTF_UNSIGNED_INT;
component_sizeof = 4;
break;
}
}

const size_t byte_length = mesh.triangles.size() * component_sizeof;

mesh_component_meta[i] = {
byte_length,
component_type
};
}

for (auto& uv : uvs) {
// Flip UVs on Y axis.
for (size_t i = 0; i < uv.size(); i += 2)
uv[i + 1] = (uv[i + 1] - 1) * -1;
}

std::vector<BufferWrapper> bins;

const std::map<int, int> component_sizes = {
{GLTF_UNSIGNED_BYTE, 1},
{GLTF_UNSIGNED_SHORT, 2},
{GLTF_UNSIGNED_INT, 4},
{GLTF_FLOAT, 4}
};

size_t bin_ofs = 0;
auto writeData = [&](size_t index, const auto& arr, int stride, int componentType) {
auto& view = root["bufferViews"][index];
auto& accessor = root["accessors"][index];

const int component_size = component_sizes.at(componentType);
const size_t misalignment = bin_ofs % component_size;
const size_t padding = misalignment > 0 ? component_size - misalignment : 0;

bin_ofs += padding;
view["byteOffset"] = bin_ofs;

const size_t buffer_length = arr.size() * component_size;
view["byteLength"] = buffer_length;

bin_ofs += buffer_length;

BufferWrapper buffer = BufferWrapper::alloc(buffer_length + padding, true);

if (padding > 0)
buffer.fill(0, padding);

accessor["count"] = static_cast<int>(arr.size() / stride);

calculate_min_max(arr, stride, accessor);
for (const auto& node : arr) {
if (componentType == GLTF_FLOAT)
buffer.writeFloatLE(static_cast<float>(node));
else if (componentType == GLTF_UNSIGNED_BYTE)
buffer.writeUInt8(static_cast<uint8_t>(node));
}

bins.push_back(std::move(buffer));
};

writeData(0, vertices, 3, GLTF_FLOAT);
writeData(1, normals, 3, GLTF_FLOAT);

if (!bones_ref.empty()) {
writeData(idx_bone_joints, boneIndices, 4, GLTF_UNSIGNED_BYTE);
writeData(idx_bone_weights, boneWeights, 4, GLTF_UNSIGNED_BYTE);
writeData(idx_inv_bind, inverseBindMatrices, 16, GLTF_FLOAT);
}

for (size_t i = 0, n = uvs.size(); i < n; i++) {
const auto& uv = uvs[i];
const int index = static_cast<int>(root["bufferViews"].size());

const std::string accessor_name = "TEXCOORD_" + std::to_string(i);
primitive_attributes[accessor_name] = index;

root["accessors"].push_back({
{"name", accessor_name},
{"bufferView", static_cast<int>(root["bufferViews"].size())},
{"byteOffset", 0},
{"componentType", GLTF_FLOAT},
{"count", static_cast<int>(uv.size() / 2)},
{"type", "VEC2"}
});

root["bufferViews"].push_back({
{"buffer", 0},
{"byteLength", 0},
{"byteOffset", 0},
{"target", GLTF_ARRAY_BUFFER}
});

writeData(index, uv, 2, GLTF_FLOAT);
}

for (size_t i = 0, n = meshes.size(); i < n; i++) {
const auto& mesh = meshes[i];
const auto& mesh_meta = mesh_component_meta[i];

const int component_type = mesh_meta.component_type;

const int bufferViewIndex = static_cast<int>(root["bufferViews"].size());
const int accessorIndex = static_cast<int>(root["accessors"].size());

const int component_size = component_sizes.at(component_type);
const size_t misalignment = bin_ofs % component_size;
const size_t padding = misalignment > 0 ? component_size - misalignment : 0;

bin_ofs += padding;

// Create ELEMENT_ARRAY_BUFFER for mesh indices.
root["bufferViews"].push_back({
{"buffer", 0},
{"byteLength", mesh_meta.byte_length},
{"byteOffset", bin_ofs},
{"target", GLTF_ELEMENT_ARRAY_BUFFER}
});

bin_ofs += mesh_meta.byte_length;

BufferWrapper buffer = BufferWrapper::alloc(mesh_meta.byte_length + padding, true);

if (padding > 0)
buffer.fill(0, padding);

// Create accessor for the mesh indices.
root["accessors"].push_back({
{"bufferView", bufferViewIndex},
{"byteOffset", 0},
{"componentType", component_type},
{"count", mesh.triangles.size()},
{"type", "SCALAR"}
});

// Write indices into the binary.
if (component_type == GLTF_UNSIGNED_BYTE) {
for (const auto idx : mesh.triangles)
buffer.writeUInt8(static_cast<uint8_t>(idx));
} else if (component_type == GLTF_UNSIGNED_SHORT) {
for (const auto idx : mesh.triangles)
buffer.writeUInt16LE(static_cast<uint16_t>(idx));
} else if (component_type == GLTF_UNSIGNED_INT) {
for (const auto idx : mesh.triangles)
buffer.writeUInt32LE(idx);
}

const int meshIndex = static_cast<int>(root["meshes"].size());

nlohmann::json primitive = {
{"attributes", primitive_attributes},
{"indices", accessorIndex},
{"mode", GLTF_TRIANGLES}
};
auto mat_it = materialMap.find(mesh.matName);
if (mat_it != materialMap.end())
primitive["material"] = mat_it->second;

root["meshes"].push_back({
{"primitives", nlohmann::json::array({primitive})}
});

nlohmann::json scene_node = {{"name", name + "_" + mesh.name}, {"mesh", meshIndex}};
if (!bones_ref.empty())
scene_node["skin"] = 0;

add_scene_node(std::move(scene_node));
bins.push_back(std::move(buffer));
}

// export equipment models
for (size_t eqIdx = 0; eqIdx < equipment_models.size(); eqIdx++) {
const auto& equip = equipment_models[eqIdx];

// create accessors for equipment geometry
const int eq_vert_accessor = static_cast<int>(root["accessors"].size());
const int eq_vert_bufview = static_cast<int>(root["bufferViews"].size());
root["bufferViews"].push_back({
{"buffer", 0},
{"byteLength", 0},
{"byteOffset", 0},
{"target", GLTF_ARRAY_BUFFER}
});
root["accessors"].push_back({
{"name", "EQ_POSITION_" + std::to_string(eqIdx)},
{"bufferView", eq_vert_bufview},
{"byteOffset", 0},
{"componentType", GLTF_FLOAT},
{"count", 0},
{"type", "VEC3"}
});

const int eq_norm_accessor = static_cast<int>(root["accessors"].size());
const int eq_norm_bufview = static_cast<int>(root["bufferViews"].size());
root["bufferViews"].push_back({
{"buffer", 0},
{"byteLength", 0},
{"byteOffset", 0},
{"target", GLTF_ARRAY_BUFFER}
});
root["accessors"].push_back({
{"name", "EQ_NORMAL_" + std::to_string(eqIdx)},
{"bufferView", eq_norm_bufview},
{"byteOffset", 0},
{"componentType", GLTF_FLOAT},
{"count", 0},
{"type", "VEC3"}
});

// equipment primitive attributes
nlohmann::json eq_prim_attribs = {
{"POSITION", eq_vert_accessor},
{"NORMAL", eq_norm_accessor}
};

// write equipment vertices
{
const int component_size = 4;
const size_t misalignment = bin_ofs % component_size;
const size_t padding = misalignment > 0 ? component_size - misalignment : 0;
bin_ofs += padding;

root["bufferViews"][eq_vert_bufview]["byteOffset"] = bin_ofs;
const size_t buffer_length = equip.vertices.size() * component_size;
root["bufferViews"][eq_vert_bufview]["byteLength"] = buffer_length;
bin_ofs += buffer_length;

root["accessors"][eq_vert_accessor]["count"] = static_cast<int>(equip.vertices.size() / 3);
calculate_min_max(equip.vertices, 3, root["accessors"][eq_vert_accessor]);

BufferWrapper buffer = BufferWrapper::alloc(buffer_length + padding, true);
if (padding > 0)
buffer.fill(0, padding);

for (const auto v : equip.vertices)
buffer.writeFloatLE(v);

bins.push_back(std::move(buffer));
}

// write equipment normals
{
const int component_size = 4;
const size_t misalignment = bin_ofs % component_size;
const size_t padding = misalignment > 0 ? component_size - misalignment : 0;
bin_ofs += padding;

root["bufferViews"][eq_norm_bufview]["byteOffset"] = bin_ofs;
const size_t buffer_length = equip.normals.size() * component_size;
root["bufferViews"][eq_norm_bufview]["byteLength"] = buffer_length;
bin_ofs += buffer_length;

root["accessors"][eq_norm_accessor]["count"] = static_cast<int>(equip.normals.size() / 3);
calculate_min_max(equip.normals, 3, root["accessors"][eq_norm_accessor]);

BufferWrapper buffer = BufferWrapper::alloc(buffer_length + padding, true);
if (padding > 0)
buffer.fill(0, padding);

for (const auto n : equip.normals)
buffer.writeFloatLE(n);

bins.push_back(std::move(buffer));
}

// write equipment UVs
if (!equip.uv.empty()) {
// flip UVs on Y axis
std::vector<float> flipped_uv(equip.uv.size());
for (size_t i = 0; i < equip.uv.size(); i += 2) {
flipped_uv[i] = equip.uv[i];
flipped_uv[i + 1] = (equip.uv[i + 1] - 1) * -1;
}

const int eq_uv_accessor = static_cast<int>(root["accessors"].size());
const int eq_uv_bufview = static_cast<int>(root["bufferViews"].size());
eq_prim_attribs["TEXCOORD_0"] = eq_uv_accessor;

root["bufferViews"].push_back({
{"buffer", 0},
{"byteLength", 0},
{"byteOffset", 0},
{"target", GLTF_ARRAY_BUFFER}
});
root["accessors"].push_back({
{"name", "EQ_TEXCOORD_0_" + std::to_string(eqIdx)},
{"bufferView", eq_uv_bufview},
{"byteOffset", 0},
{"componentType", GLTF_FLOAT},
{"count", static_cast<int>(flipped_uv.size() / 2)},
{"type", "VEC2"}
});

const int component_size = 4;
const size_t misalignment = bin_ofs % component_size;
const size_t padding = misalignment > 0 ? component_size - misalignment : 0;
bin_ofs += padding;

root["bufferViews"][eq_uv_bufview]["byteOffset"] = bin_ofs;
const size_t buffer_length = flipped_uv.size() * component_size;
root["bufferViews"][eq_uv_bufview]["byteLength"] = buffer_length;
bin_ofs += buffer_length;

BufferWrapper buffer = BufferWrapper::alloc(buffer_length + padding, true);
if (padding > 0)
buffer.fill(0, padding);

for (const auto uv : flipped_uv)
buffer.writeFloatLE(uv);

bins.push_back(std::move(buffer));
}

// write equipment bone indices (for skinned equipment)
bool eq_has_skin = false;
if (!equip.boneIndices.empty() && !bones_ref.empty()) {
eq_has_skin = true;

const int eq_joints_accessor = static_cast<int>(root["accessors"].size());
const int eq_joints_bufview = static_cast<int>(root["bufferViews"].size());
eq_prim_attribs["JOINTS_0"] = eq_joints_accessor;

root["bufferViews"].push_back({
{"buffer", 0},
{"byteLength", 0},
{"byteOffset", 0},
{"target", GLTF_ARRAY_BUFFER}
});
root["accessors"].push_back({
{"name", "EQ_JOINTS_0_" + std::to_string(eqIdx)},
{"bufferView", eq_joints_bufview},
{"byteOffset", 0},
{"componentType", GLTF_UNSIGNED_BYTE},
{"count", static_cast<int>(equip.boneIndices.size() / 4)},
{"type", "VEC4"}
});

[[maybe_unused]] const size_t misalignment = bin_ofs % 1;
const size_t padding = 0;
bin_ofs += padding;

root["bufferViews"][eq_joints_bufview]["byteOffset"] = bin_ofs;
const size_t buffer_length = equip.boneIndices.size();
root["bufferViews"][eq_joints_bufview]["byteLength"] = buffer_length;
bin_ofs += buffer_length;

BufferWrapper buffer = BufferWrapper::alloc(buffer_length, true);
for (const auto idx : equip.boneIndices)
buffer.writeUInt8(idx);

bins.push_back(std::move(buffer));

// write equipment bone weights
const int eq_weights_accessor = static_cast<int>(root["accessors"].size());
const int eq_weights_bufview = static_cast<int>(root["bufferViews"].size());
eq_prim_attribs["WEIGHTS_0"] = eq_weights_accessor;

root["bufferViews"].push_back({
{"buffer", 0},
{"byteLength", 0},
{"byteOffset", 0},
{"target", GLTF_ARRAY_BUFFER}
});
root["accessors"].push_back({
{"name", "EQ_WEIGHTS_0_" + std::to_string(eqIdx)},
{"bufferView", eq_weights_bufview},
{"byteOffset", 0},
{"componentType", GLTF_UNSIGNED_BYTE},
{"normalized", true},
{"count", static_cast<int>(equip.boneWeights.size() / 4)},
{"type", "VEC4"}
});

root["bufferViews"][eq_weights_bufview]["byteOffset"] = bin_ofs;
const size_t weights_buffer_length = equip.boneWeights.size();
root["bufferViews"][eq_weights_bufview]["byteLength"] = weights_buffer_length;
bin_ofs += weights_buffer_length;

BufferWrapper weights_buffer = BufferWrapper::alloc(weights_buffer_length, true);
for (const auto w : equip.boneWeights)
weights_buffer.writeUInt8(w);

bins.push_back(std::move(weights_buffer));
}

// write equipment meshes
for (size_t mI = 0; mI < equip.meshes.size(); mI++) {
const auto& mesh = equip.meshes[mI];

// determine component type for indices
int component_type = GLTF_UNSIGNED_BYTE;
int component_sizeof = 1;
for (const auto idx : mesh.triangles) {
if (idx > 255 && component_type == GLTF_UNSIGNED_BYTE) {
component_type = GLTF_UNSIGNED_SHORT;
component_sizeof = 2;
} else if (idx > 65535 && component_type == GLTF_UNSIGNED_SHORT) {
component_type = GLTF_UNSIGNED_INT;
component_sizeof = 4;
break;
}
}

const size_t byte_length = mesh.triangles.size() * component_sizeof;
const int bufferViewIndex = static_cast<int>(root["bufferViews"].size());
const int accessorIndex = static_cast<int>(root["accessors"].size());

const int component_size = component_sizeof;
const size_t misalignment = bin_ofs % component_size;
const size_t padding = misalignment > 0 ? component_size - misalignment : 0;
bin_ofs += padding;

root["bufferViews"].push_back({
{"buffer", 0},
{"byteLength", byte_length},
{"byteOffset", bin_ofs},
{"target", GLTF_ELEMENT_ARRAY_BUFFER}
});

bin_ofs += byte_length;

BufferWrapper buffer = BufferWrapper::alloc(byte_length + padding, true);
if (padding > 0)
buffer.fill(0, padding);

root["accessors"].push_back({
{"bufferView", bufferViewIndex},
{"byteOffset", 0},
{"componentType", component_type},
{"count", mesh.triangles.size()},
{"type", "SCALAR"}
});

if (component_type == GLTF_UNSIGNED_BYTE) {
for (const auto idx : mesh.triangles)
buffer.writeUInt8(static_cast<uint8_t>(idx));
} else if (component_type == GLTF_UNSIGNED_SHORT) {
for (const auto idx : mesh.triangles)
buffer.writeUInt16LE(static_cast<uint16_t>(idx));
} else if (component_type == GLTF_UNSIGNED_INT) {
for (const auto idx : mesh.triangles)
buffer.writeUInt32LE(idx);
}

const int meshIndex = static_cast<int>(root["meshes"].size());

nlohmann::json eq_primitive = {
{"attributes", eq_prim_attribs},
{"indices", accessorIndex},
{"mode", GLTF_TRIANGLES}
};
auto eq_mat_it = materialMap.find(mesh.matName);
if (eq_mat_it != materialMap.end())
eq_primitive["material"] = eq_mat_it->second;

root["meshes"].push_back({
{"primitives", nlohmann::json::array({eq_primitive})}
});

nlohmann::json eq_node = {{"name", equip.name + "_" + mesh.name}, {"mesh", meshIndex}};

// apply skin or parent to attachment bone
if (eq_has_skin)
eq_node["skin"] = 0;
else if (equip.attachment_bone >= 0)
eq_node["parent_bone"] = equip.attachment_bone;

add_scene_node(std::move(eq_node));
bins.push_back(std::move(buffer));
}
}

// pack texture buffers into binary for glb mode
if (format == "glb" && !texture_buffer_views.empty()) {
for (size_t i = 0; i < texture_buffer_views.size(); i++) {
auto& tex_view = texture_buffer_views[i];
auto& tex_buffer = tex_view.buffer;

// create bufferView for this texture
const int buffer_view_index = static_cast<int>(root["bufferViews"].size());
root["bufferViews"].push_back({
{"buffer", 0},
{"byteLength", tex_buffer.byteLength()},
{"byteOffset", bin_ofs}
});

// update the image's bufferView reference
root["images"][i]["bufferView"] = buffer_view_index;

bin_ofs += tex_buffer.byteLength();
bins.push_back(std::move(tex_buffer));
}
}

// pack animation buffers into binary for glb mode
if (format == "glb" && !animationBufferMap.empty()) {
std::map<std::string, size_t> anim_buffer_base_offsets;

// store base offset for each animation buffer
for (auto& [animName, animBuffer] : animationBufferMap) {
anim_buffer_base_offsets[animName] = bin_ofs;
bin_ofs += animBuffer.byteLength();
bins.push_back(std::move(animBuffer));
}

// update all bufferViews that reference animation data
for (auto& bufferView : root["bufferViews"]) {
if (bufferView.value("buffer", -1) == 0 && bufferView.contains("name")) {
const std::string bv_name = bufferView["name"].get<std::string>();
if (bv_name.starts_with("TRANS_") ||
bv_name.starts_with("ROT_") ||
bv_name.starts_with("SCALE_")) {
// extract animation name from bufferView name
// name format: TYPE_SUBTYPE_boneIdx_animIdx (e.g. TRANS_TIMESTAMPS_0_1 or TRANS_VALUES_0_1)
size_t last_underscore = bv_name.rfind('_');
size_t second_last = bv_name.rfind('_', last_underscore - 1);
const int anim_idx = std::stoi(bv_name.substr(last_underscore + 1));
const std::string animName = std::to_string(animations[anim_idx].id) + "-" + std::to_string(animations[anim_idx].variationIndex);

// update byteOffset to absolute position in combined buffer
const size_t base_offset = anim_buffer_base_offsets.at(animName);
bufferView["byteOffset"] = bufferView["byteOffset"].get<size_t>() + base_offset;
}
}
}
}

BufferWrapper bin_combined = BufferWrapper::concat(bins);
root["buffers"][0]["byteLength"] = bin_combined.byteLength();

generics::createDirectory(out.parent_path());

if (format == "glb") {
// glb mode: package json and bin into glb container
std::string json_str = root.dump();
GLBWriter glb_writer(json_str, bin_combined);
BufferWrapper glb_buffer = glb_writer.pack();
glb_buffer.writeToFile(outGLTF);
} else {
// gltf mode: write separate json and bin files
root["buffers"][0]["uri"] = outBIN.filename().string();
std::ofstream ofs(outGLTF);
ofs << root.dump(1, '\t');
bin_combined.writeToFile(outBIN);
}

// write out animation buffers (gltf mode only, glb embeds them)
if (format == "gltf") {
for (auto& [animationName, animationBuffer] : animationBufferMap) {
const std::filesystem::path animationPath = out_dir / (outBIN.stem().string() + "_anim" + animationName + ".bin");
animationBuffer.writeToFile(animationPath);
}
}
}
