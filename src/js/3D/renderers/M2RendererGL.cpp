/*!
wow.export (https://github.com/Kruithne/wow.export)
Authors: Kruithne <kruithne@gmail.com>
License: MIT
*/

#include "M2RendererGL.h"

#include "../../core.h"
#include "../../log.h"
#include "../../buffer.h"

#include "../../casc/blp.h"
#include "../../casc/casc-source.h"
#include "../GeosetMapper.h"
#include "../ShaderMapper.h"
#include "../Shaders.h"

#include "../../ui/texture-ribbon.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <format>
#include <functional>
#include <optional>
#include <unordered_map>

// -----------------------------------------------------------------------
// vertex shader name to ID mapping (matches vertex shader switch cases)
// -----------------------------------------------------------------------

static const std::unordered_map<std::string_view, int> VERTEX_SHADER_IDS = {
{"Diffuse_T1", 0},
{"Diffuse_Env", 1},
{"Diffuse_T1_T2", 2},
{"Diffuse_T1_Env", 3},
{"Diffuse_Env_T1", 4},
{"Diffuse_Env_Env", 5},
{"Diffuse_T1_Env_T1", 6},
{"Diffuse_T1_T1", 7},
{"Diffuse_T1_T1_T1", 8},
{"Diffuse_EdgeFade_T1", 9},
{"Diffuse_T2", 10},
{"Diffuse_T1_Env_T2", 11},
{"Diffuse_EdgeFade_T1_T2", 12},
{"Diffuse_EdgeFade_Env", 13},
{"Diffuse_T1_T2_T1", 14},
{"Diffuse_T1_T2_T3", 15},
{"Color_T1_T2_T3", 16},
{"BW_Diffuse_T1", 17},
{"BW_Diffuse_T1_T2", 18}
};

// pixel shader name to ID mapping (matches fragment shader switch cases)
static const std::unordered_map<std::string_view, int> PIXEL_SHADER_IDS = {
{"Combiners_Opaque", 0},
{"Combiners_Mod", 1},
{"Combiners_Opaque_Mod", 2},
{"Combiners_Opaque_Mod2x", 3},
{"Combiners_Opaque_Mod2xNA", 4},
{"Combiners_Opaque_Opaque", 5},
{"Combiners_Mod_Mod", 6},
{"Combiners_Mod_Mod2x", 7},
{"Combiners_Mod_Add", 8},
{"Combiners_Mod_Mod2xNA", 9},
{"Combiners_Mod_AddNA", 10},
{"Combiners_Mod_Opaque", 11},
{"Combiners_Opaque_Mod2xNA_Alpha", 12},
{"Combiners_Opaque_AddAlpha", 13},
{"Combiners_Opaque_AddAlpha_Alpha", 14},
{"Combiners_Opaque_Mod2xNA_Alpha_Add", 15},
{"Combiners_Mod_AddAlpha", 16},
{"Combiners_Mod_AddAlpha_Alpha", 17},
{"Combiners_Opaque_Alpha_Alpha", 18},
{"Combiners_Opaque_Mod2xNA_Alpha_3s", 19},
{"Combiners_Opaque_AddAlpha_Wgt", 20},
{"Combiners_Mod_Add_Alpha", 21},
{"Combiners_Opaque_ModNA_Alpha", 22},
{"Combiners_Mod_AddAlpha_Wgt", 23},
{"Combiners_Opaque_Mod_Add_Wgt", 24},
{"Combiners_Opaque_Mod2xNA_Alpha_UnshAlpha", 25},
{"Combiners_Mod_Dual_Crossfade", 26},
{"Combiners_Opaque_Mod2xNA_Alpha_Alpha", 27},
{"Combiners_Mod_Masked_Dual_Crossfade", 28},
{"Combiners_Opaque_Alpha", 29},
{"Guild", 30},
{"Guild_NoBorder", 31},
{"Guild_Opaque", 32},
{"Combiners_Mod_Depth", 33},
{"Illum", 34},
{"Combiners_Mod_Mod_Mod_Const", 35},
{"Combiners_Mod_Mod_Depth", 36}
};

// identity matrix
static constexpr std::array<float, 16> M2_IDENTITY_MAT4 = {
1, 0, 0, 0,
0, 1, 0, 0,
0, 0, 1, 0,
0, 0, 0, 1
};

// -----------------------------------------------------------------------
// Free-function math helpers (matching JS module-level functions exactly)
// -----------------------------------------------------------------------

static void mat4_multiply(float* out, const float* a, const float* b) {
const float a00 = a[0], a01 = a[1], a02 = a[2], a03 = a[3];
const float a10 = a[4], a11 = a[5], a12 = a[6], a13 = a[7];
const float a20 = a[8], a21 = a[9], a22 = a[10], a23 = a[11];
const float a30 = a[12], a31 = a[13], a32 = a[14], a33 = a[15];

float b0 = b[0], b1 = b[1], b2 = b[2], b3 = b[3];
out[0] = b0 * a00 + b1 * a10 + b2 * a20 + b3 * a30;
out[1] = b0 * a01 + b1 * a11 + b2 * a21 + b3 * a31;
out[2] = b0 * a02 + b1 * a12 + b2 * a22 + b3 * a32;
out[3] = b0 * a03 + b1 * a13 + b2 * a23 + b3 * a33;

b0 = b[4]; b1 = b[5]; b2 = b[6]; b3 = b[7];
out[4] = b0 * a00 + b1 * a10 + b2 * a20 + b3 * a30;
out[5] = b0 * a01 + b1 * a11 + b2 * a21 + b3 * a31;
out[6] = b0 * a02 + b1 * a12 + b2 * a22 + b3 * a32;
out[7] = b0 * a03 + b1 * a13 + b2 * a23 + b3 * a33;

b0 = b[8]; b1 = b[9]; b2 = b[10]; b3 = b[11];
out[8] = b0 * a00 + b1 * a10 + b2 * a20 + b3 * a30;
out[9] = b0 * a01 + b1 * a11 + b2 * a21 + b3 * a31;
out[10] = b0 * a02 + b1 * a12 + b2 * a22 + b3 * a32;
out[11] = b0 * a03 + b1 * a13 + b2 * a23 + b3 * a33;

b0 = b[12]; b1 = b[13]; b2 = b[14]; b3 = b[15];
out[12] = b0 * a00 + b1 * a10 + b2 * a20 + b3 * a30;
out[13] = b0 * a01 + b1 * a11 + b2 * a21 + b3 * a31;
out[14] = b0 * a02 + b1 * a12 + b2 * a22 + b3 * a32;
out[15] = b0 * a03 + b1 * a13 + b2 * a23 + b3 * a33;
}

static void mat4_from_translation(float* out, float x, float y, float z) {
out[0] = 1; out[1] = 0; out[2] = 0; out[3] = 0;
out[4] = 0; out[5] = 1; out[6] = 0; out[7] = 0;
out[8] = 0; out[9] = 0; out[10] = 1; out[11] = 0;
out[12] = x; out[13] = y; out[14] = z; out[15] = 1;
}

static void mat4_from_quat(float* out, float x, float y, float z, float w) {
const float x2 = x + x, y2 = y + y, z2 = z + z;
const float xx = x * x2, xy = x * y2, xz = x * z2;
const float yy = y * y2, yz = y * z2, zz = z * z2;
const float wx = w * x2, wy = w * y2, wz = w * z2;

out[0] = 1 - (yy + zz);
out[1] = xy + wz;
out[2] = xz - wy;
out[3] = 0;

out[4] = xy - wz;
out[5] = 1 - (xx + zz);
out[6] = yz + wx;
out[7] = 0;

out[8] = xz + wy;
out[9] = yz - wx;
out[10] = 1 - (xx + yy);
out[11] = 0;

out[12] = 0;
out[13] = 0;
out[14] = 0;
out[15] = 1;
}

static void mat4_from_scale(float* out, float x, float y, float z) {
out[0] = x; out[1] = 0; out[2] = 0; out[3] = 0;
out[4] = 0; out[5] = y; out[6] = 0; out[7] = 0;
out[8] = 0; out[9] = 0; out[10] = z; out[11] = 0;
out[12] = 0; out[13] = 0; out[14] = 0; out[15] = 1;
}

static void mat4_copy(float* out, const float* src) {
std::memcpy(out, src, 16 * sizeof(float));
}

static void mat4_from_quat_trs(float* out, const float* position, const float* quat, const float* scale) {
const float px = position[0], py = position[1], pz = position[2];
const float qx = quat[0], qy = quat[1], qz = quat[2], qw = quat[3];
const float sx = scale[0], sy = scale[1], sz = scale[2];

// rotation from quaternion
const float x2 = qx + qx, y2 = qy + qy, z2 = qz + qz;
const float xx = qx * x2, xy = qx * y2, xz = qx * z2;
const float yy = qy * y2, yz = qy * z2, zz = qz * z2;
const float wx = qw * x2, wy = qw * y2, wz = qw * z2;

// column 0 (scaled)
out[0] = (1 - (yy + zz)) * sx;
out[1] = (xy + wz) * sx;
out[2] = (xz - wy) * sx;
out[3] = 0;

// column 1 (scaled)
out[4] = (xy - wz) * sy;
out[5] = (1 - (xx + zz)) * sy;
out[6] = (yz + wx) * sy;
out[7] = 0;

// column 2 (scaled)
out[8] = (xz + wy) * sz;
out[9] = (yz - wx) * sz;
out[10] = (1 - (xx + yy)) * sz;
out[11] = 0;

// column 3 (translation)
out[12] = px;
out[13] = py;
out[14] = pz;
out[15] = 1;
}

static float lerp(float a, float b, float t) {
return a + (b - a) * t;
}

static void quat_slerp(float* out, float ax, float ay, float az, float aw, float bx, float by, float bz, float bw, float t) {
float cosom = ax * bx + ay * by + az * bz + aw * bw;

if (cosom < 0) {
cosom = -cosom;
bx = -bx; by = -by; bz = -bz; bw = -bw;
}

float scale0, scale1;
if (1 - cosom > 0.000001f) {
const float omega = std::acos(cosom);
const float sinom = std::sin(omega);
scale0 = std::sin((1 - t) * omega) / sinom;
scale1 = std::sin(t * omega) / sinom;
} else {
scale0 = 1 - t;
scale1 = t;
}

out[0] = scale0 * ax + scale1 * bx;
out[1] = scale0 * ay + scale1 * by;
out[2] = scale0 * az + scale1 * bz;
out[3] = scale0 * aw + scale1 * bw;
}

// -----------------------------------------------------------------------
// Helpers to extract typed data from M2Value variant
// -----------------------------------------------------------------------

// Extract uint32_t timestamp from M2Value (timestamps are stored as uint32_t)
static uint32_t m2value_to_uint32(const M2Value& v) {
if (auto* val = std::get_if<uint32_t>(&v))
return *val;
if (auto* val = std::get_if<uint8_t>(&v))
return static_cast<uint32_t>(*val);
if (auto* val = std::get_if<int16_t>(&v))
return static_cast<uint32_t>(*val);
return 0;
}

// Extract float3/float4 from M2Value (values are stored as std::vector<float>)
static const std::vector<float>& m2value_to_vec(const M2Value& v) {
static const std::vector<float> empty;
if (auto* val = std::get_if<std::vector<float>>(&v))
return *val;
return empty;
}

// -----------------------------------------------------------------------
// Template: bone matrix calculation shared between M2Bone and SKELBone sources
// Both structs have identical fields (parentBone, pivot, boneID, M2Track fields)
// Templated on both structural (hierarchy/pivot) and animation (track data) bone types
// -----------------------------------------------------------------------

template<typename StructuralBoneT, typename AnimBoneT>
static void calc_all_bones(
const std::vector<StructuralBoneT>& structural_bones,
const std::vector<AnimBoneT>& anim_bone_list,
int anim_idx,
float time_ms,
int hands_closed_anim_idx,
bool close_r,
bool close_l,
std::vector<float>& bone_matrices_vec,
const std::function<std::array<float,3>(const std::vector<M2Value>&, const std::vector<M2Value>&, float, const std::array<float,3>&)>& sample_vec3,
const std::function<std::array<float,4>(const std::vector<M2Value>&, const std::vector<M2Value>&, float)>& sample_quat
) {
const size_t bone_count = structural_bones.size();
std::vector<bool> calculated(bone_count, false);

float local_mat[16];
float trans_mat[16];
float rot_mat[16];
float scale_mat[16];
float pivot_mat[16];
float neg_pivot_mat[16];
float temp_result[16];

std::function<void(size_t)> calc_bone = [&](size_t idx) {
if (calculated[idx])
return;

const auto& bone = structural_bones[idx];
const int16_t parent_idx = bone.parentBone;

// calculate parent first
if (parent_idx >= 0 && static_cast<size_t>(parent_idx) < bone_count)
calc_bone(static_cast<size_t>(parent_idx));

// get pivot point (from structural bone)
const auto& pivot = bone.pivot;
const float px = (pivot.size() >= 3) ? pivot[0] : 0.0f;
const float py = (pivot.size() >= 3) ? pivot[1] : 0.0f;
const float pz = (pivot.size() >= 3) ? pivot[2] : 0.0f;

// determine which animation to use for this bone
// finger bones use HandsClosed animation when hand grip is active
// right finger bone IDs: 8-12, left finger bone IDs: 13-17
const int32_t bone_id = bone.boneID;
const bool is_right_finger = bone_id >= 8 && bone_id <= 12;
const bool is_left_finger = bone_id >= 13 && bone_id <= 17;
const bool use_closed_hand = (is_right_finger && close_r) || (is_left_finger && close_l);
const int effective_anim_idx = use_closed_hand ? hands_closed_anim_idx : anim_idx;
const float effective_time_ms = use_closed_hand ? 0.0f : time_ms; // use frame 0 for HandsClosed

// check if bone has any animation data (from anim_bone in anim_bone_list)
bool has_trans = false, has_rot = false, has_scale = false, has_scale_fallback = false;
if (idx < anim_bone_list.size() && effective_anim_idx >= 0) {
const auto& anim_bone = anim_bone_list[idx];
const size_t eff_idx = static_cast<size_t>(effective_anim_idx);
has_trans = eff_idx < anim_bone.translation.timestamps.size() &&
!anim_bone.translation.timestamps[eff_idx].empty();
has_rot = eff_idx < anim_bone.rotation.timestamps.size() &&
!anim_bone.rotation.timestamps[eff_idx].empty();
has_scale = eff_idx < anim_bone.scale.timestamps.size() &&
!anim_bone.scale.timestamps[eff_idx].empty();
has_scale_fallback = !has_scale && effective_anim_idx != 0 &&
!anim_bone.scale.timestamps.empty() &&
!anim_bone.scale.timestamps[0].empty();
}
const bool has_animation = has_trans || has_rot || has_scale || has_scale_fallback;

// start with identity
mat4_copy(local_mat, M2_IDENTITY_MAT4.data());

if (has_animation && idx < anim_bone_list.size()) {
const auto& anim_bone = anim_bone_list[idx];
const size_t eff_idx = static_cast<size_t>(effective_anim_idx);

// translate to pivot
mat4_from_translation(pivot_mat, px, py, pz);
mat4_multiply(temp_result, local_mat, pivot_mat);
mat4_copy(local_mat, temp_result);

// apply translation (raw animation offset from anim_bone data)
if (has_trans) {
const auto& ts = anim_bone.translation.timestamps[eff_idx];
const auto& vals = anim_bone.translation.values[eff_idx];
const auto result = sample_vec3(ts, vals, effective_time_ms, {0, 0, 0});

mat4_from_translation(trans_mat, result[0], result[1], result[2]);
mat4_multiply(temp_result, local_mat, trans_mat);
mat4_copy(local_mat, temp_result);
}

// apply rotation
if (has_rot) {
const auto& ts = anim_bone.rotation.timestamps[eff_idx];
const auto& vals = anim_bone.rotation.values[eff_idx];
const auto result = sample_quat(ts, vals, effective_time_ms);

mat4_from_quat(rot_mat, result[0], result[1], result[2], result[3]);
mat4_multiply(temp_result, local_mat, rot_mat);
mat4_copy(local_mat, temp_result);
}

// apply scale (fallback to animation 0 if current animation lacks scale data)
if (has_scale || has_scale_fallback) {
const int scale_anim_idx = has_scale ? effective_anim_idx : 0;
const auto& ts = anim_bone.scale.timestamps[static_cast<size_t>(scale_anim_idx)];
const auto& vals = anim_bone.scale.values[static_cast<size_t>(scale_anim_idx)];
const float scale_time = has_scale ? effective_time_ms : 0.0f;
const auto result = sample_vec3(ts, vals, scale_time, {1, 1, 1});

mat4_from_scale(scale_mat, result[0], result[1], result[2]);
mat4_multiply(temp_result, local_mat, scale_mat);
mat4_copy(local_mat, temp_result);
}

// translate back from pivot
mat4_from_translation(neg_pivot_mat, -px, -py, -pz);
mat4_multiply(temp_result, local_mat, neg_pivot_mat);
mat4_copy(local_mat, temp_result);
}
// if no animation, local_mat stays identity

// multiply with parent matrix
const size_t offset = idx * 16;
if (parent_idx >= 0 && static_cast<size_t>(parent_idx) < bone_count) {
const size_t parent_offset = static_cast<size_t>(parent_idx) * 16;
mat4_multiply(&bone_matrices_vec[offset], &bone_matrices_vec[parent_offset], local_mat);
} else {
std::copy(local_mat, local_mat + 16, bone_matrices_vec.begin() + static_cast<ptrdiff_t>(offset));
}

calculated[idx] = true;
};

for (size_t i = 0; i < bone_count; i++)
calc_bone(i);
}

// -----------------------------------------------------------------------
// Constructor
// -----------------------------------------------------------------------

M2RendererGL::M2RendererGL(BufferWrapper& data, gl::GLContext& gl_context, bool reactive, bool useRibbon)
: data_ptr(&data)
, ctx(gl_context)
, reactive(reactive)
, useRibbon(useRibbon)
{
m2 = nullptr;
syncID = -1;

// rendering state: vaos, textures, default_texture, buffers, draw_calls, indices_data default constructed

// skeleton: skelLoader, childSkelLoader, childAnimKeys default constructed

// animation state
bones_m2 = nullptr;
bones_skel = nullptr;
current_anim_from_skel = false;
current_anim_from_child = false;
current_animation = -1; // null equivalent
current_anim_index = -1;
animation_time = 0;
animation_paused = false;

// hand grip state for weapon attachment
// when true, finger bones use HandsClosed animation (ID 15)
close_right_hand = false;
close_left_hand = false;
hands_closed_anim_idx = -1;

// collection model support
use_external_bones = false;

// reactive state
geosetKey = "modelViewerGeosets";

// transforms
std::copy(M2_IDENTITY_MAT4.begin(), M2_IDENTITY_MAT4.end(), model_matrix.begin());
position_ = {0, 0, 0};
rotation_ = {0, 0, 0};
scale_ = {1, 1, 1};

// material data: material_props, shader_map default constructed
}

// -----------------------------------------------------------------------
// static load_shaders
// -----------------------------------------------------------------------

std::unique_ptr<gl::ShaderProgram> M2RendererGL::load_shaders(gl::GLContext& ctx) {
return shaders::create_program(ctx, "m2");
}

// -----------------------------------------------------------------------
// load
// -----------------------------------------------------------------------

void M2RendererGL::load() {
// parse M2 data
m2 = std::make_unique<M2Loader>(*data_ptr);
m2->load();

// load shader program
shader = M2RendererGL::load_shaders(ctx);

// create default texture
_create_default_texture();

// load textures
_load_textures();

// load first skin
if (!m2->vertices.empty()) {
loadSkin(0);

if (reactive) {
}
}

// drop reference to raw data
data_ptr = nullptr;
}

// -----------------------------------------------------------------------
// _create_default_texture
// -----------------------------------------------------------------------

void M2RendererGL::_create_default_texture() {
const uint8_t pixels[4] = {87, 175, 226, 255}; // 0x57afe2 blue
default_texture = std::make_unique<gl::GLTexture>(ctx);
gl::TextureOptions opts;
opts.has_alpha = false;
default_texture->set_rgba(pixels, 1, 1, opts);
}

// -----------------------------------------------------------------------
// _load_textures
// -----------------------------------------------------------------------

void M2RendererGL::_load_textures() {
auto& tex_list = m2->textures;

if (useRibbon)
syncID = texture_ribbon::reset();

for (size_t i = 0, n = tex_list.size(); i < n; i++) {
auto& texture = tex_list[i];
int ribbonSlot = useRibbon ? texture_ribbon::addSlot() : -1;

if (texture.fileDataID > 0) {
if (ribbonSlot >= 0)
texture_ribbon::setSlotFile(ribbonSlot, texture.fileDataID, syncID);

try {
if (casc_source_) {
BufferWrapper file_data = casc_source_->getVirtualFileByID(texture.fileDataID);
casc::BLPImage blp(std::move(file_data));
auto gl_tex = std::make_unique<gl::GLTexture>(ctx);
gl::BLPTextureFlags blp_flags;
blp_flags.flags = texture.flags;
gl_tex->set_blp(blp, blp_flags);
textures[static_cast<int>(i)] = std::move(gl_tex);

if (ribbonSlot >= 0)
texture_ribbon::setSlotSrc(ribbonSlot, blp.getDataURL(0b0111), syncID);
}
} catch (const std::exception& e) {
logging::write(std::format("Failed to load texture {}: {}", texture.fileDataID, e.what()));
}
}
}
}

// -----------------------------------------------------------------------
// loadSkin
// -----------------------------------------------------------------------

void M2RendererGL::loadSkin(int index) {
_dispose_skin();

auto& skin = m2->getSkin(static_cast<uint32_t>(index));

// create skeleton
_create_skeleton();

// build interleaved vertex buffer
// format: position(3f) + normal(3f) + bone_idx(4ub) + bone_weight(4ub) + uv(2f) = 40 bytes
const size_t vertex_count = m2->vertices.size() / 3;
const size_t stride = 40;
std::vector<uint8_t> vertex_data(vertex_count * stride);

for (size_t i = 0; i < vertex_count; i++) {
const size_t offset = i * stride;
const size_t v_idx = i * 3;
const size_t uv_idx = i * 2;
const size_t bone_idx = i * 4;

// position
std::memcpy(&vertex_data[offset + 0], &m2->vertices[v_idx], sizeof(float));
std::memcpy(&vertex_data[offset + 4], &m2->vertices[v_idx + 1], sizeof(float));
std::memcpy(&vertex_data[offset + 8], &m2->vertices[v_idx + 2], sizeof(float));

// normal
std::memcpy(&vertex_data[offset + 12], &m2->normals[v_idx], sizeof(float));
std::memcpy(&vertex_data[offset + 16], &m2->normals[v_idx + 1], sizeof(float));
std::memcpy(&vertex_data[offset + 20], &m2->normals[v_idx + 2], sizeof(float));

// bone indices
vertex_data[offset + 24] = m2->boneIndices[bone_idx];
vertex_data[offset + 25] = m2->boneIndices[bone_idx + 1];
vertex_data[offset + 26] = m2->boneIndices[bone_idx + 2];
vertex_data[offset + 27] = m2->boneIndices[bone_idx + 3];

// bone weights
vertex_data[offset + 28] = m2->boneWeights[bone_idx];
vertex_data[offset + 29] = m2->boneWeights[bone_idx + 1];
vertex_data[offset + 30] = m2->boneWeights[bone_idx + 2];
vertex_data[offset + 31] = m2->boneWeights[bone_idx + 3];

// texcoord
std::memcpy(&vertex_data[offset + 32], &m2->uv[uv_idx], sizeof(float));
std::memcpy(&vertex_data[offset + 36], &m2->uv[uv_idx + 1], sizeof(float));
}

// map triangle indices
indices_data.resize(skin.triangles.size());
for (size_t i = 0; i < skin.triangles.size(); i++)
indices_data[i] = skin.indices[skin.triangles[i]];

// create VAO
auto vao = std::make_unique<gl::VertexArray>(ctx);
vao->bind();

GLuint vbo;
glGenBuffers(1, &vbo);
glBindBuffer(GL_ARRAY_BUFFER, vbo);
glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertex_data.size()), vertex_data.data(), GL_STATIC_DRAW);
this->buffers.push_back(vbo);
vao->vbo = vbo;

GLuint ebo;
glGenBuffers(1, &ebo);
glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices_data.size() * sizeof(uint16_t)), indices_data.data(), GL_STATIC_DRAW);
this->buffers.push_back(ebo);
vao->ebo = ebo;

// set up vertex attributes
vao->setup_m2_vertex_format();

gl::VertexArray* vao_raw = vao.get();
vaos.push_back(std::move(vao));

// reactive geoset array
if (reactive)
geosetArray.resize(skin.subMeshes.size());

// build draw calls per submesh
draw_calls.clear();

for (size_t i = 0; i < skin.subMeshes.size(); i++) {
const auto& submesh = skin.subMeshes[i];

// find texture unit matching this submesh
const TextureUnit* tex_unit = nullptr;
for (const auto& tu : skin.textureUnits) {
if (tu.skinSectionIndex == static_cast<uint16_t>(i)) {
tex_unit = &tu;
break;
}
}

std::array<int, 4> tex_indices = { -1, -1, -1, -1 };
int vertex_shader = 0;
int pixel_shader = 0;
int blend_mode = 0;
uint16_t flags = 0;
uint16_t texture_count = 1;

if (tex_unit) {
texture_count = tex_unit->textureCount;

// get all texture indices for multi-texture shaders
for (uint16_t j = 0; j < std::min(texture_count, static_cast<uint16_t>(4)); j++) {
const uint16_t combo_idx = tex_unit->textureComboIndex + j;
if (combo_idx < m2->textureCombos.size())
tex_indices[j] = static_cast<int>(m2->textureCombos[combo_idx]);
}

// get vertex shader ID
const auto vs_name = shader_mapper::getVertexShader(tex_unit->textureCount, tex_unit->shaderID);
if (vs_name.has_value()) {
auto it = VERTEX_SHADER_IDS.find(vs_name.value());
if (it != VERTEX_SHADER_IDS.end())
vertex_shader = it->second;
}

// get pixel shader ID
const auto ps_name = shader_mapper::getPixelShader(tex_unit->textureCount, tex_unit->shaderID);
if (ps_name.has_value()) {
auto it = PIXEL_SHADER_IDS.find(ps_name.value());
if (it != PIXEL_SHADER_IDS.end())
pixel_shader = it->second;
}

if (tex_unit->materialIndex < m2->materials.size()) {
const auto& mat = m2->materials[tex_unit->materialIndex];
blend_mode = mat.blendingMode;
flags = mat.flags;
material_props[tex_indices[0]] = { blend_mode, flags };
}
}

M2DrawCall draw_call;
draw_call.vao = vao_raw;
draw_call.start = submesh.triangleStart;
draw_call.count = submesh.triangleCount;
draw_call.tex_indices = tex_indices;
draw_call.texture_count = texture_count;
draw_call.vertex_shader = vertex_shader;
draw_call.pixel_shader = pixel_shader;
draw_call.blend_mode = blend_mode;
draw_call.flags = flags;
draw_call.visible = true;

// reactive geoset
if (reactive) {
const std::string id_str = std::to_string(submesh.submeshID);
bool is_default = (submesh.submeshID == 0 ||
(id_str.size() >= 2 && id_str.substr(id_str.size() - 2) == "01") ||
(id_str.size() >= 2 && id_str.substr(0, 2) == "32"));
if ((id_str.size() >= 2 && id_str.substr(0, 2) == "17") ||
(id_str.size() >= 2 && id_str.substr(0, 2) == "35"))
is_default = false;

M2GeosetEntry entry;
entry.label = "Geoset " + std::to_string(i);
entry.checked = is_default;
entry.id = submesh.submeshID;
geosetArray[i] = entry;
draw_call.visible = is_default;
}

draw_calls.push_back(draw_call);
}

if (reactive) {
auto& geosets = core::view->modelViewerGeosets;
geosets.clear();
for (const auto& entry : geosetArray) {
nlohmann::json j;
j["label"] = entry.label;
j["checked"] = entry.checked;
j["id"] = entry.id;
geosets.push_back(j);
}

std::vector<geoset_mapper::Geoset> mapper_geosets;
for (const auto& entry : geosetArray) {
geoset_mapper::Geoset g;
g.id = entry.id;
g.label = entry.label;
mapper_geosets.push_back(g);
}
geoset_mapper::map(mapper_geosets);

// update labels back
for (size_t i = 0; i < mapper_geosets.size() && i < geosetArray.size(); i++)
geosetArray[i].label = mapper_geosets[i].label;

// update core state labels too
for (size_t i = 0; i < mapper_geosets.size() && i < geosets.size(); i++)
geosets[i]["label"] = mapper_geosets[i].label;
}

updateGeosets();
}

// -----------------------------------------------------------------------
// _create_skeleton
// -----------------------------------------------------------------------

void M2RendererGL::_create_skeleton() {
// reset bone pointers
bones_m2 = nullptr;
bones_skel = nullptr;

// load external skeleton if present
if (m2->skeletonFileID > 0 && casc_source_) {
try {
auto skel_buf = std::make_unique<BufferWrapper>(casc_source_->getVirtualFileByID(m2->skeletonFileID));
auto skel = std::make_unique<SKELLoader>(*skel_buf);
skel->load();

if (skel->parent_skel_file_id > 0) {
auto parent_buf = std::make_unique<BufferWrapper>(casc_source_->getVirtualFileByID(skel->parent_skel_file_id));
auto parent_skel = std::make_unique<SKELLoader>(*parent_buf);
parent_skel->load();

// track which animations come from child vs parent
// child skeleton's .anim files have different data layouts than parent's bone offsets expect
std::set<std::string> child_anim_keys;
for (const auto& entry : skel->animFileIDs) {
if (entry.fileDataID > 0) {
child_anim_keys.insert(
std::to_string(entry.animID) + "-" + std::to_string(entry.subAnimID));
}
}

// store child skeleton for animations that need it
if (!child_anim_keys.empty()) {
skel_buffers_.push_back(std::move(skel_buf));
childSkelLoader = std::move(skel);
childAnimKeys = std::move(child_anim_keys);
}

// don't merge child AFIDs into parent - they use incompatible bone offsets
// parent skeleton handles its own animations, child handles its own
skel_buffers_.push_back(std::move(parent_buf));
skelLoader = std::move(parent_skel);
bones_skel = &skelLoader->bones;
} else {
skel_buffers_.push_back(std::move(skel_buf));
skelLoader = std::move(skel);
bones_skel = &skelLoader->bones;
}
} catch (const std::exception& e) {
logging::write(std::format("Failed to load skeleton: {}", e.what()));
}
}

// fall back to m2->bones if no skel loaded
if (!bones_skel && !m2->bones.empty())
bones_m2 = &m2->bones;

const size_t bc = bones_count();
if (bc == 0) {
bone_matrices.assign(16, 0.0f);
std::copy(M2_IDENTITY_MAT4.begin(), M2_IDENTITY_MAT4.end(), bone_matrices.begin());
return;
}

bone_matrices.resize(bc * 16);

// initialize to identity
for (size_t i = 0; i < bc; i++) {
std::copy(M2_IDENTITY_MAT4.begin(), M2_IDENTITY_MAT4.end(),
bone_matrices.begin() + static_cast<ptrdiff_t>(i * 16));
}

// find HandsClosed animation (ID 15) for hand grip
hands_closed_anim_idx = -1;
if (skelLoader) {
for (size_t i = 0; i < skelLoader->animations.size(); i++) {
if (skelLoader->animations[i].id == 15) {
hands_closed_anim_idx = static_cast<int>(i);
break;
}
}
} else {
for (size_t i = 0; i < m2->animations.size(); i++) {
if (m2->animations[i].id == 15) {
hands_closed_anim_idx = static_cast<int>(i);
break;
}
}
}
}

// -----------------------------------------------------------------------
// playAnimation
// -----------------------------------------------------------------------

void M2RendererGL::playAnimation(int index) {
// determine animation source: default to skelLoader or m2
bool use_skel = (skelLoader != nullptr);
bool use_child = false;
int anim_index = index;

// check if this animation should come from child skeleton
if (childSkelLoader && !childAnimKeys.empty()) {
// access primary source animations (M2Animation and SKELAnimation have same layout)
if (use_skel) {
if (index >= 0 && static_cast<size_t>(index) < skelLoader->animations.size()) {
const auto& anim = skelLoader->animations[static_cast<size_t>(index)];
const std::string key = std::to_string(anim.id) + "-" + std::to_string(anim.variationIndex);
if (childAnimKeys.count(key)) {
for (size_t i = 0; i < childSkelLoader->animations.size(); i++) {
if (childSkelLoader->animations[i].id == anim.id &&
childSkelLoader->animations[i].variationIndex == anim.variationIndex)
{
use_child = true;
use_skel = false;
anim_index = static_cast<int>(i);
break;
}
}
}
}
} else {
if (index >= 0 && static_cast<size_t>(index) < m2->animations.size()) {
const auto& anim = m2->animations[static_cast<size_t>(index)];
const std::string key = std::to_string(anim.id) + "-" + std::to_string(anim.variationIndex);
if (childAnimKeys.count(key)) {
for (size_t i = 0; i < childSkelLoader->animations.size(); i++) {
if (childSkelLoader->animations[i].id == anim.id &&
childSkelLoader->animations[i].variationIndex == anim.variationIndex)
{
use_child = true;
use_skel = false;
anim_index = static_cast<int>(i);
break;
}
}
}
}
}
}

// ensure animation data is loaded
try {
if (use_child) {
childSkelLoader->loadAnimsForIndex(static_cast<uint32_t>(anim_index));
} else if (use_skel) {
skelLoader->loadAnimsForIndex(static_cast<uint32_t>(anim_index));
} else {
m2->loadAnimsForIndex(static_cast<uint32_t>(anim_index));
}
} catch (const std::exception& e) {
logging::write(std::format("Failed to load animation data: {}", e.what()));
}

// store which skeleton is being used for this animation
current_anim_from_skel = use_skel;
current_anim_from_child = use_child;
current_anim_index = anim_index;
current_animation = index;
animation_time = 0;
}

// -----------------------------------------------------------------------
// stopAnimation
// -----------------------------------------------------------------------

void M2RendererGL::stopAnimation() {
animation_time = 0;
animation_paused = false;

// calculate bone matrices using animation 0 (stand) at time 0 for rest pose
if (has_bones()) {
const int prev_anim = current_animation;
const int prev_anim_idx = current_anim_index;
const bool prev_from_skel = current_anim_from_skel;
const bool prev_from_child = current_anim_from_child;

current_animation = 0;
current_anim_index = 0;
current_anim_from_skel = (skelLoader != nullptr);
current_anim_from_child = false;
_update_bone_matrices();

current_animation = -1; // null
current_anim_index = -1;
current_anim_from_skel = prev_from_skel;
current_anim_from_child = prev_from_child;
}
}

// -----------------------------------------------------------------------
// _get_anim_duration_ms: helper to get duration from current anim source
// -----------------------------------------------------------------------

uint32_t M2RendererGL::_get_anim_duration_ms() const {
if (current_animation < 0 || current_anim_index < 0)
return 0;

if (current_anim_from_child) {
if (!childSkelLoader) return 0;
if (static_cast<size_t>(current_anim_index) >= childSkelLoader->animations.size()) return 0;
return childSkelLoader->animations[static_cast<size_t>(current_anim_index)].duration;
} else if (current_anim_from_skel) {
if (!skelLoader) return 0;
if (static_cast<size_t>(current_anim_index) >= skelLoader->animations.size()) return 0;
return skelLoader->animations[static_cast<size_t>(current_anim_index)].duration;
} else {
if (static_cast<size_t>(current_anim_index) >= m2->animations.size()) return 0;
return m2->animations[static_cast<size_t>(current_anim_index)].duration;
}
}

// -----------------------------------------------------------------------
// updateAnimation
// -----------------------------------------------------------------------

void M2RendererGL::updateAnimation(float delta_time) {
if (current_animation < 0 || !has_bones())
return;

const uint32_t duration_ms = _get_anim_duration_ms();
if (duration_ms == 0)
return;

if (!animation_paused)
animation_time += delta_time;

// wrap animation (duration is in milliseconds)
const float duration_sec = static_cast<float>(duration_ms) / 1000.0f;
if (duration_sec > 0)
animation_time = std::fmod(animation_time, duration_sec);

// update bone matrices
_update_bone_matrices();
}

// -----------------------------------------------------------------------
// get_animation_duration
// -----------------------------------------------------------------------

float M2RendererGL::get_animation_duration() {
if (current_animation < 0)
return 0;

return static_cast<float>(_get_anim_duration_ms()) / 1000.0f;
}

// -----------------------------------------------------------------------
// get_animation_frame_count
// -----------------------------------------------------------------------

int M2RendererGL::get_animation_frame_count() {
const float duration = get_animation_duration();
// 60 fps
return std::max(1, static_cast<int>(std::floor(duration * 60)));
}

// -----------------------------------------------------------------------
// get_animation_frame
// -----------------------------------------------------------------------

int M2RendererGL::get_animation_frame() {
const float duration = get_animation_duration();
if (duration <= 0)
return 0;

return static_cast<int>(std::floor((animation_time / duration) * static_cast<float>(get_animation_frame_count())));
}

// -----------------------------------------------------------------------
// set_animation_frame
// -----------------------------------------------------------------------

void M2RendererGL::set_animation_frame(int frame) {
const int frame_count = get_animation_frame_count();
if (frame_count <= 0)
return;

const float duration = get_animation_duration();
animation_time = (static_cast<float>(frame) / static_cast<float>(frame_count)) * duration;
_update_bone_matrices();
}

// -----------------------------------------------------------------------
// set_animation_paused
// -----------------------------------------------------------------------

void M2RendererGL::set_animation_paused(bool paused) {
animation_paused = paused;
}

// -----------------------------------------------------------------------
// step_animation_frame
// -----------------------------------------------------------------------

void M2RendererGL::step_animation_frame(int delta) {
const int frame = get_animation_frame();
const int frame_count = get_animation_frame_count();
int new_frame = frame + delta;

if (new_frame < 0)
new_frame = frame_count - 1;
else if (new_frame >= frame_count)
new_frame = 0;

set_animation_frame(new_frame);
}

// -----------------------------------------------------------------------
// _update_bone_matrices
// -----------------------------------------------------------------------

void M2RendererGL::_update_bone_matrices() {
if (!has_bones())
return;

const float time_ms = animation_time * 1000.0f; // convert to milliseconds for raw tracks
// use the correct animation index for the skeleton we're reading from
const int anim_idx = current_anim_index >= 0 ? current_anim_index : current_animation;

// hand grip: use HandsClosed animation for finger bones
const bool close_r = close_right_hand && hands_closed_anim_idx >= 0;
const bool close_l = close_left_hand && hands_closed_anim_idx >= 0;

// build sampling function wrappers
auto sample_vec3 = [this](const std::vector<M2Value>& ts, const std::vector<M2Value>& vals, float t_ms, const std::array<float,3>& def) -> std::array<float,3> {
return _sample_raw_vec3(ts, vals, t_ms, def);
};
auto sample_quat = [this](const std::vector<M2Value>& ts, const std::vector<M2Value>& vals, float t_ms) -> std::array<float,4> {
return _sample_raw_quat(ts, vals, t_ms);
};

// use the correct skeleton's bones for animation data
// child animations need child's bones which have correct offsets
// dispatch on all (structural_type, anim_type) combinations
if (current_anim_from_child && childSkelLoader) {
if (bones_skel)
calc_all_bones(*bones_skel, childSkelLoader->bones, anim_idx, time_ms,
hands_closed_anim_idx, close_r, close_l, bone_matrices, sample_vec3, sample_quat);
else if (bones_m2)
calc_all_bones(*bones_m2, childSkelLoader->bones, anim_idx, time_ms,
hands_closed_anim_idx, close_r, close_l, bone_matrices, sample_vec3, sample_quat);
} else if (current_anim_from_skel && skelLoader) {
if (bones_skel)
calc_all_bones(*bones_skel, skelLoader->bones, anim_idx, time_ms,
hands_closed_anim_idx, close_r, close_l, bone_matrices, sample_vec3, sample_quat);
else if (bones_m2)
calc_all_bones(*bones_m2, skelLoader->bones, anim_idx, time_ms,
hands_closed_anim_idx, close_r, close_l, bone_matrices, sample_vec3, sample_quat);
} else {
// animation tracks come from m2->bones (M2Bone)
if (bones_skel)
calc_all_bones(*bones_skel, m2->bones, anim_idx, time_ms,
hands_closed_anim_idx, close_r, close_l, bone_matrices, sample_vec3, sample_quat);
else if (bones_m2)
calc_all_bones(*bones_m2, m2->bones, anim_idx, time_ms,
hands_closed_anim_idx, close_r, close_l, bone_matrices, sample_vec3, sample_quat);
}
}

// -----------------------------------------------------------------------
// _sample_raw_vec3
// M2 modern format: direct per-animation keyframe arrays (M2Value variant)
// timestamps[i] = uint32_t ms, values[i] = std::vector<float> (3 elements)
// -----------------------------------------------------------------------

std::array<float, 3> M2RendererGL::_sample_raw_vec3(
const std::vector<M2Value>& timestamps,
const std::vector<M2Value>& values,
float time_ms,
const std::array<float, 3>& default_value)
{
if (timestamps.empty())
return default_value;

if (timestamps.size() == 1 || time_ms <= static_cast<float>(m2value_to_uint32(timestamps[0]))) {
const auto& v = m2value_to_vec(values[0]);
if (v.size() >= 3)
return {v[0], v[1], v[2]};
return default_value;
}

if (time_ms >= static_cast<float>(m2value_to_uint32(timestamps[timestamps.size() - 1]))) {
const auto& v = m2value_to_vec(values[values.size() - 1]);
if (v.size() >= 3)
return {v[0], v[1], v[2]};
return default_value;
}

// find keyframe
size_t frame = 0;
for (size_t i = 0; i < timestamps.size() - 1; i++) {
const float t_i = static_cast<float>(m2value_to_uint32(timestamps[i]));
const float t_next = static_cast<float>(m2value_to_uint32(timestamps[i + 1]));
if (time_ms >= t_i && time_ms < t_next) {
frame = i;
break;
}
}

const float t0 = static_cast<float>(m2value_to_uint32(timestamps[frame]));
const float t1 = static_cast<float>(m2value_to_uint32(timestamps[frame + 1]));
const float alpha = (t1 > t0) ? (time_ms - t0) / (t1 - t0) : 0.0f;

const auto& v0 = m2value_to_vec(values[frame]);
const auto& v1 = m2value_to_vec(values[frame + 1]);

if (v0.size() >= 3 && v1.size() >= 3) {
return {
lerp(v0[0], v1[0], alpha),
lerp(v0[1], v1[1], alpha),
lerp(v0[2], v1[2], alpha)
};
}

return default_value;
}

// -----------------------------------------------------------------------
// _sample_raw_quat
// -----------------------------------------------------------------------

std::array<float, 4> M2RendererGL::_sample_raw_quat(
const std::vector<M2Value>& timestamps,
const std::vector<M2Value>& values,
float time_ms)
{
if (timestamps.empty())
return {0, 0, 0, 1};

if (timestamps.size() == 1 || time_ms <= static_cast<float>(m2value_to_uint32(timestamps[0]))) {
const auto& v = m2value_to_vec(values[0]);
if (v.size() >= 4)
return {v[0], v[1], v[2], v[3]};
return {0, 0, 0, 1};
}

if (time_ms >= static_cast<float>(m2value_to_uint32(timestamps[timestamps.size() - 1]))) {
const auto& v = m2value_to_vec(values[values.size() - 1]);
if (v.size() >= 4)
return {v[0], v[1], v[2], v[3]};
return {0, 0, 0, 1};
}

// find keyframe
size_t frame = 0;
for (size_t i = 0; i < timestamps.size() - 1; i++) {
const float t_i = static_cast<float>(m2value_to_uint32(timestamps[i]));
const float t_next = static_cast<float>(m2value_to_uint32(timestamps[i + 1]));
if (time_ms >= t_i && time_ms < t_next) {
frame = i;
break;
}
}

const float t0 = static_cast<float>(m2value_to_uint32(timestamps[frame]));
const float t1 = static_cast<float>(m2value_to_uint32(timestamps[frame + 1]));
const float alpha = (t1 > t0) ? (time_ms - t0) / (t1 - t0) : 0.0f;

const auto& q0 = m2value_to_vec(values[frame]);
const auto& q1 = m2value_to_vec(values[frame + 1]);

if (q0.size() >= 4 && q1.size() >= 4) {
std::array<float, 4> out = {0, 0, 0, 1};
quat_slerp(out.data(), q0[0], q0[1], q0[2], q0[3], q1[0], q1[1], q1[2], q1[3], alpha);
return out;
}

return {0, 0, 0, 1};
}

// -----------------------------------------------------------------------
// buildBoneRemapTable
// -----------------------------------------------------------------------

void M2RendererGL::buildBoneRemapTable(const std::vector<M2Bone>& char_bones) {
// buildBoneRemapTable is only called for collection models (equipment, weapons) which
// use m2->bones (M2Bone) — not skel bones. Use bones_m2 as the structural source.
if (!bones_m2 || char_bones.empty()) {
bone_remap_table.clear();
use_external_bones = false;
return;
}

const float epsilon = 0.0001f;
bone_remap_table.resize(bones_m2->size());

for (size_t i = 0; i < bones_m2->size(); i++) {
const auto& coll_bone = (*bones_m2)[i];
const auto& coll_pivot = coll_bone.pivot;
int found = -1;

// match by pivot position AND boneNameCRC (like wowmodelviewer)
// boneNameCRC distinguishes left/right bones with similar pivots
for (size_t j = 0; j < char_bones.size(); j++) {
const auto& char_bone = char_bones[j];
const auto& char_pivot = char_bone.pivot;

if (coll_pivot.size() < 3 || char_pivot.size() < 3)
continue;

const float dx = std::abs(coll_pivot[0] - char_pivot[0]);
const float dy = std::abs(coll_pivot[1] - char_pivot[1]);
const float dz = std::abs(coll_pivot[2] - char_pivot[2]);

if (dx < epsilon && dy < epsilon && dz < epsilon &&
coll_bone.boneNameCRC == char_bone.boneNameCRC)
{
found = static_cast<int>(j);
break;
}
}

// fallback: match by pivot only if boneNameCRC match failed
if (found < 0) {
for (size_t j = 0; j < char_bones.size(); j++) {
const auto& char_bone = char_bones[j];
const auto& char_pivot = char_bone.pivot;

if (coll_pivot.size() < 3 || char_pivot.size() < 3)
continue;

const float dx = std::abs(coll_pivot[0] - char_pivot[0]);
const float dy = std::abs(coll_pivot[1] - char_pivot[1]);
const float dz = std::abs(coll_pivot[2] - char_pivot[2]);

if (dx < epsilon && dy < epsilon && dz < epsilon) {
found = static_cast<int>(j);
break;
}
}
}

bone_remap_table[i] = static_cast<int16_t>(found >= 0 ? found : static_cast<int>(i));
}

use_external_bones = true;
}

// -----------------------------------------------------------------------
// applyExternalBoneMatrices
// -----------------------------------------------------------------------

void M2RendererGL::applyExternalBoneMatrices(const float* char_bone_matrices, size_t matrix_count) {
if (bone_remap_table.empty() || bone_matrices.empty() || !char_bone_matrices)
return;

for (size_t i = 0; i < bone_remap_table.size(); i++) {
const size_t char_idx = static_cast<size_t>(bone_remap_table[i]);
const size_t char_offset = char_idx * 16;
const size_t local_offset = i * 16;

if (char_offset + 16 <= matrix_count)
std::memcpy(&bone_matrices[local_offset], &char_bone_matrices[char_offset], 16 * sizeof(float));
}
}

// -----------------------------------------------------------------------
// setGeosetGroupDisplay
// -----------------------------------------------------------------------

void M2RendererGL::setGeosetGroupDisplay(int group, int value) {
if (draw_calls.empty())
return;

const int range_min = group * 100;
const int range_max = (group + 1) * 100;
const int target_id = range_min + value;

// get skin submeshes to check geoset IDs
if (m2->skins.empty())
return;

const auto& skin = m2->skins[0];

for (size_t i = 0; i < draw_calls.size() && i < skin.subMeshes.size(); i++) {
const int submesh_id = static_cast<int>(skin.subMeshes[i].submeshID);

// check if this submesh is in the geoset group range
if (submesh_id > range_min && submesh_id < range_max)
draw_calls[i].visible = (submesh_id == target_id);
}
}

// -----------------------------------------------------------------------
// hideAllGeosets
// -----------------------------------------------------------------------

void M2RendererGL::hideAllGeosets() {
for (auto& dc : draw_calls)
dc.visible = false;
}

// -----------------------------------------------------------------------
// updateGeosets
// -----------------------------------------------------------------------

void M2RendererGL::updateGeosets() {
if (!reactive || geosetArray.empty() || draw_calls.empty())
return;

for (size_t i = 0; i < draw_calls.size() && i < geosetArray.size(); i++)
draw_calls[i].visible = geosetArray[i].checked;
}

// -----------------------------------------------------------------------
// updateWireframe
// -----------------------------------------------------------------------

void M2RendererGL::updateWireframe() {
// handled in render()
}

// -----------------------------------------------------------------------
// setTransform
// -----------------------------------------------------------------------

void M2RendererGL::setTransform(const std::array<float, 3>& position, const std::array<float, 3>& rotation, const std::array<float, 3>& scale) {
this->position_ = position;
this->rotation_ = rotation;
this->scale_ = scale;
_update_model_matrix();
}

// -----------------------------------------------------------------------
// setTransformQuat
// -----------------------------------------------------------------------

void M2RendererGL::setTransformQuat(const std::array<float, 3>& position, const std::array<float, 4>& quat, const std::array<float, 3>& scale) {
mat4_from_quat_trs(model_matrix.data(), position.data(), quat.data(), scale.data());
}

// -----------------------------------------------------------------------
// setTransformMatrix
// -----------------------------------------------------------------------

void M2RendererGL::setTransformMatrix(const float* matrix) {
std::memcpy(model_matrix.data(), matrix, 16 * sizeof(float));
}

// -----------------------------------------------------------------------
// setHandGrip
// -----------------------------------------------------------------------

void M2RendererGL::setHandGrip(bool close_right, bool close_left) {
close_right_hand = close_right;
close_left_hand = close_left;
}

// -----------------------------------------------------------------------
// _update_model_matrix
// -----------------------------------------------------------------------

void M2RendererGL::_update_model_matrix() {
// build model matrix from position/rotation/scale (TRS order)
auto& m = model_matrix;
const float px = position_[0], py = position_[1], pz = position_[2];
const float rx = rotation_[0], ry = rotation_[1], rz = rotation_[2];
const float sx = scale_[0], sy = scale_[1], sz = scale_[2];

// rotation (ZYX euler order, column-major)
const float cx = std::cos(rx), sinx = std::sin(rx);
const float cy = std::cos(ry), siny = std::sin(ry);
const float cz = std::cos(rz), sinz = std::sin(rz);

// column 0 (scaled by sx)
m[0] = cy * cz * sx;
m[1] = cy * sinz * sx;
m[2] = -siny * sx;
m[3] = 0;

// column 1 (scaled by sy)
m[4] = (sinx * siny * cz - cx * sinz) * sy;
m[5] = (sinx * siny * sinz + cx * cz) * sy;
m[6] = sinx * cy * sy;
m[7] = 0;

// column 2 (scaled by sz)
m[8] = (cx * siny * cz + sinx * sinz) * sz;
m[9] = (cx * siny * sinz - sinx * cz) * sz;
m[10] = cx * cy * sz;
m[11] = 0;

// column 3 (translation)
m[12] = px;
m[13] = py;
m[14] = pz;
m[15] = 1;
}

// -----------------------------------------------------------------------
// render
// -----------------------------------------------------------------------

void M2RendererGL::render(const float* view_matrix, const float* projection_matrix) {
if (!shader || draw_calls.empty())
return;

const bool wireframe = core::view->config.value("modelViewerWireframe", false);

shader->use();

// set scene uniforms
shader->set_uniform_mat4("u_view_matrix", false, view_matrix);
shader->set_uniform_mat4("u_projection_matrix", false, projection_matrix);
shader->set_uniform_mat4("u_model_matrix", false, model_matrix.data());
shader->set_uniform_3f("u_view_up", 0, 1, 0);

static const auto s_render_start = std::chrono::steady_clock::now();
auto now = std::chrono::steady_clock::now();
float time_sec = std::chrono::duration<float>(now - s_render_start).count();
shader->set_uniform_1f("u_time", time_sec);

// bone matrices
shader->set_uniform_1i("u_bone_count", static_cast<int>(bones_count()));
if (has_bones() && !bone_matrices.empty()) {
GLint loc = shader->get_uniform_location("u_bone_matrices");
if (loc >= 0)
glUniformMatrix4fv(loc, static_cast<GLsizei>(bones_count()), GL_FALSE, bone_matrices.data());
}

// texture matrix defaults
shader->set_uniform_1i("u_has_tex_matrix1", 0);
shader->set_uniform_1i("u_has_tex_matrix2", 0);
shader->set_uniform_mat4("u_tex_matrix1", false, M2_IDENTITY_MAT4.data());
shader->set_uniform_mat4("u_tex_matrix2", false, M2_IDENTITY_MAT4.data());

// lighting - transform light direction to view space
const float lx = 3, ly = -0.7f, lz = -2;
const float light_view_x = view_matrix[0] * lx + view_matrix[4] * ly + view_matrix[8] * lz;
const float light_view_y = view_matrix[1] * lx + view_matrix[5] * ly + view_matrix[9] * lz;
const float light_view_z = view_matrix[2] * lx + view_matrix[6] * ly + view_matrix[10] * lz;

shader->set_uniform_1i("u_apply_lighting", 1);
shader->set_uniform_3f("u_ambient_color", 0.5f, 0.5f, 0.5f);
shader->set_uniform_3f("u_diffuse_color", 0.7f, 0.7f, 0.7f);
shader->set_uniform_3f("u_light_dir", light_view_x, light_view_y, light_view_z);

// wireframe
shader->set_uniform_1i("u_wireframe", wireframe ? 1 : 0);
shader->set_uniform_4f("u_wireframe_color", 1, 1, 1, 1);

// alpha test
shader->set_uniform_1f("u_alpha_test", 0.501960814f);

// texture samplers
shader->set_uniform_1i("u_texture1", 0);
shader->set_uniform_1i("u_texture2", 1);
shader->set_uniform_1i("u_texture3", 2);
shader->set_uniform_1i("u_texture4", 3);

// default texture weights
shader->set_uniform_3f("u_tex_sample_alpha", 1, 1, 1);

// sort draw calls by blend mode (opaque first, then transparent)
std::vector<const M2DrawCall*> sorted_calls;
sorted_calls.reserve(draw_calls.size());
for (const auto& dc : draw_calls)
sorted_calls.push_back(&dc);

std::stable_sort(sorted_calls.begin(), sorted_calls.end(), [](const M2DrawCall* a, const M2DrawCall* b) {
const bool a_opaque = a->blend_mode == 0 || a->blend_mode == 1;
const bool b_opaque = b->blend_mode == 0 || b->blend_mode == 1;
if (a_opaque != b_opaque)
return a_opaque;

return false;
});

// render each draw call
for (const auto* dc : sorted_calls) {
if (!dc->visible)
continue;

// set material uniforms
shader->set_uniform_1i("u_vertex_shader", dc->vertex_shader);
shader->set_uniform_1i("u_pixel_shader", dc->pixel_shader);
shader->set_uniform_1i("u_blend_mode", dc->blend_mode);

// mesh color (white for now)
shader->set_uniform_4f("u_mesh_color", 1, 1, 1, 1);

// apply blend mode
ctx.apply_blend_mode(dc->blend_mode);

// culling based on flags
if (dc->flags & 0x04) {
ctx.set_cull_face(false);
} else {
ctx.set_cull_face(true);
ctx.set_cull_mode(GL_BACK);
}

// depth test flags
if (dc->flags & 0x08)
ctx.set_depth_test(false);
else
ctx.set_depth_test(true);

// bind textures (up to 4 for multi-texture shaders)
for (int t = 0; t < 4; t++) {
const int tex_idx = dc->tex_indices[t];
gl::GLTexture* texture = nullptr;
if (tex_idx >= 0) {
auto it = textures.find(tex_idx);
texture = (it != textures.end()) ? it->second.get() : default_texture.get();
} else {
texture = default_texture.get();
}
if (texture)
texture->bind(t);
}

// draw
dc->vao->bind();
glDrawElements(
wireframe ? GL_LINES : GL_TRIANGLES,
static_cast<GLsizei>(dc->count),
GL_UNSIGNED_SHORT,
reinterpret_cast<void*>(static_cast<uintptr_t>(dc->start * 2))
);
}

// reset state
ctx.set_blend(false);
ctx.set_depth_test(true);
ctx.set_depth_write(true);
ctx.set_cull_face(false);
}

// -----------------------------------------------------------------------
// overrideTextureType
// -----------------------------------------------------------------------

void M2RendererGL::overrideTextureType(uint32_t type, uint32_t fileDataID) {
if (!casc_source_)
return;

const auto& textureTypes = m2->textureTypes;

for (size_t i = 0; i < textureTypes.size(); i++) {
if (textureTypes[i] != type)
continue;

try {
BufferWrapper file_data = casc_source_->getVirtualFileByID(fileDataID);
casc::BLPImage blp(std::move(file_data));
auto gl_tex = std::make_unique<gl::GLTexture>(ctx);
gl::BLPTextureFlags blp_flags;
blp_flags.flags = m2->textures[i].flags;
gl_tex->set_blp(blp, blp_flags);

// dispose old texture
auto it = textures.find(static_cast<int>(i));
if (it != textures.end())
it->second->dispose();

textures[static_cast<int>(i)] = std::move(gl_tex);

if (useRibbon) {
texture_ribbon::setSlotFile(static_cast<int>(i), fileDataID, syncID);
texture_ribbon::setSlotSrc(static_cast<int>(i), blp.getDataURL(0b0111), syncID);
}
} catch (const std::exception& e) {
logging::write(std::format("Failed to override texture: {}", e.what()));
}
}
}

// -----------------------------------------------------------------------
// overrideTextureTypeWithCanvas
// (Browser API systemic translation — no HTML canvas in desktop GL)
// -----------------------------------------------------------------------

void M2RendererGL::overrideTextureTypeWithCanvas(uint32_t type, const uint8_t* pixels, int width, int height) {
const auto& textureTypes = m2->textureTypes;

for (size_t i = 0; i < textureTypes.size(); i++) {
if (textureTypes[i] != type)
continue;

const uint32_t tex_flags = m2->textures[i].flags;
gl::TextureOptions opts;
opts.wrap_s = (tex_flags & 0x1) ? GL_REPEAT : GL_CLAMP_TO_EDGE;
opts.wrap_t = (tex_flags & 0x2) ? GL_REPEAT : GL_CLAMP_TO_EDGE;

auto gl_tex = std::make_unique<gl::GLTexture>(ctx);
gl_tex->set_canvas(pixels, width, height, opts);

// dispose old texture
auto it = textures.find(static_cast<int>(i));
if (it != textures.end())
it->second->dispose();

textures[static_cast<int>(i)] = std::move(gl_tex);
}
}

// -----------------------------------------------------------------------
// overrideTextureTypeWithPixels
// -----------------------------------------------------------------------

void M2RendererGL::overrideTextureTypeWithPixels(uint32_t type, int width, int height, const uint8_t* pixels) {
const auto& textureTypes = m2->textureTypes;

for (size_t i = 0; i < textureTypes.size(); i++) {
if (textureTypes[i] != type)
continue;

const uint32_t tex_flags = m2->textures[i].flags;
gl::TextureOptions opts;
opts.wrap_s = (tex_flags & 0x1) ? GL_REPEAT : GL_CLAMP_TO_EDGE;
opts.wrap_t = (tex_flags & 0x2) ? GL_REPEAT : GL_CLAMP_TO_EDGE;

auto gl_tex = std::make_unique<gl::GLTexture>(ctx);
gl_tex->set_rgba(pixels, width, height, opts);

// dispose old texture
auto it = textures.find(static_cast<int>(i));
if (it != textures.end())
it->second->dispose();

textures[static_cast<int>(i)] = std::move(gl_tex);
}
}

// -----------------------------------------------------------------------
// applyReplaceableTextures
// -----------------------------------------------------------------------

void M2RendererGL::applyReplaceableTextures(const M2DisplayInfo& displays) {
for (size_t i = 0; i < m2->textureTypes.size(); i++) {
const uint32_t textureType = m2->textureTypes[i];
if (textureType >= 11 && textureType < 14) {
const uint32_t slot = textureType - 11;
if (slot < displays.textures.size())
overrideTextureType(textureType, displays.textures[slot]);
} else if (textureType > 1 && textureType < 5) {
const uint32_t slot = textureType - 2;
if (slot < displays.textures.size())
overrideTextureType(textureType, displays.textures[slot]);
}
}
}

// -----------------------------------------------------------------------
// getUVLayers
// -----------------------------------------------------------------------

M2RendererGL::UVLayerResult M2RendererGL::getUVLayers() {
if (!m2 || indices_data.empty())
return {};

UVLayerResult result;
result.indices = &indices_data;

{
UVLayer layer;
layer.name = "UV1";
layer.data = m2->uv; // copy
layer.active = false;
result.layers.push_back(std::move(layer));
}

if (!m2->uv2.empty()) {
UVLayer layer;
layer.name = "UV2";
layer.data = m2->uv2; // copy
layer.active = false;
result.layers.push_back(std::move(layer));
}

return result;
}

// -----------------------------------------------------------------------
// getBakedGeometry
// -----------------------------------------------------------------------

std::optional<M2RendererGL::BakedGeometry> M2RendererGL::getBakedGeometry() {
if (!m2)
return std::nullopt;

const auto& src_verts = m2->vertices;
const auto& src_normals = m2->normals;
const auto& bone_indices_arr = m2->boneIndices;
const auto& bone_weights_arr = m2->boneWeights;

const size_t vertex_count = src_verts.size() / 3;
BakedGeometry result;
result.vertices.resize(vertex_count * 3);
result.normals.resize(vertex_count * 3);

// no bones or no animation - return original geometry
if (!has_bones() || bone_matrices.empty() || current_animation < 0) {
result.vertices = src_verts;
result.normals = src_normals;
return result;
}

// apply bone transforms to each vertex
for (size_t i = 0; i < vertex_count; i++) {
const size_t v_idx = i * 3;
const size_t b_idx = i * 4;

const float vx = src_verts[v_idx];
const float vy = src_verts[v_idx + 1];
const float vz = src_verts[v_idx + 2];

const float nx = src_normals[v_idx];
const float ny = src_normals[v_idx + 1];
const float nz = src_normals[v_idx + 2];

float out_x = 0, out_y = 0, out_z = 0;
float out_nx = 0, out_ny = 0, out_nz = 0;

// blend up to 4 bone influences
for (int j = 0; j < 4; j++) {
const size_t bone_idx = bone_indices_arr[b_idx + static_cast<size_t>(j)];
const float weight = static_cast<float>(bone_weights_arr[b_idx + static_cast<size_t>(j)]) / 255.0f;

if (weight == 0.0f)
continue;

const size_t mat_offset = bone_idx * 16;
if (mat_offset + 16 > bone_matrices.size())
continue;

const float* bm = &bone_matrices[mat_offset];

// transform position: out = mat * vec4(v, 1)
const float tx = bm[0] * vx + bm[4] * vy + bm[8] * vz + bm[12];
const float ty = bm[1] * vx + bm[5] * vy + bm[9] * vz + bm[13];
const float tz = bm[2] * vx + bm[6] * vy + bm[10] * vz + bm[14];

out_x += tx * weight;
out_y += ty * weight;
out_z += tz * weight;

// transform normal: out = mat3(mat) * normal (no translation)
const float tnx = bm[0] * nx + bm[4] * ny + bm[8] * nz;
const float tny = bm[1] * nx + bm[5] * ny + bm[9] * nz;
const float tnz = bm[2] * nx + bm[6] * ny + bm[10] * nz;

out_nx += tnx * weight;
out_ny += tny * weight;
out_nz += tnz * weight;
}

// normalize the blended normal
const float len = std::sqrt(out_nx * out_nx + out_ny * out_ny + out_nz * out_nz);
if (len > 0.0001f) {
out_nx /= len;
out_ny /= len;
out_nz /= len;
}

result.vertices[v_idx] = out_x;
result.vertices[v_idx + 1] = out_y;
result.vertices[v_idx + 2] = out_z;

result.normals[v_idx] = out_nx;
result.normals[v_idx + 1] = out_ny;
result.normals[v_idx + 2] = out_nz;
}

return result;
}

// -----------------------------------------------------------------------
// getAttachmentTransform
// -----------------------------------------------------------------------

std::optional<std::array<float, 16>> M2RendererGL::getAttachmentTransform(uint32_t attachmentId) {
if (!m2)
return std::nullopt;

// try m2 first, then skelLoader (modern character models use .skel files)
const M2Attachment* attachment = m2->getAttachmentById(attachmentId);
if (!attachment && skelLoader) {
// SKELAttachment has identical layout to M2Attachment — safe reinterpret_cast
// (systemic translation: both structs are layout-identical by design)
const auto* skel_att = skelLoader->getAttachmentById(attachmentId);
attachment = reinterpret_cast<const M2Attachment*>(skel_att);
}

if (!attachment)
return std::nullopt;

const uint16_t bone_idx = attachment->bone;
if (bone_matrices.empty())
return std::nullopt;

// get bone world matrix (includes rotation from animation)
const size_t bone_offset = static_cast<size_t>(bone_idx) * 16;
if (bone_offset + 16 > bone_matrices.size())
return std::nullopt;

const float* bone_mat = &bone_matrices[bone_offset];

// attachment position is in WoW coords (X=right, Y=forward, Z=up)
// convert to GL coords (X=right, Y=up, Z=-forward)
const auto& pos = attachment->position;
const float att_x = (pos.size() >= 3) ? pos[0] : 0.0f;
const float att_y = (pos.size() >= 3) ? pos[2] : 0.0f;  // WoW Z -> GL Y
const float att_z = (pos.size() >= 3) ? -pos[1] : 0.0f; // WoW Y -> GL -Z

// create attachment local transform (translation only)
const float att_mat[16] = {
1, 0, 0, 0,
0, 1, 0, 0,
0, 0, 1, 0,
att_x, att_y, att_z, 1
};

// combine: world = bone_world * attachment_local
std::array<float, 16> result;
mat4_multiply(result.data(), bone_mat, att_mat);

// apply character model's transform (rotation from camera controls)
std::array<float, 16> final_result;
mat4_multiply(final_result.data(), model_matrix.data(), result.data());

return final_result;
}

// -----------------------------------------------------------------------
// getBoundingBox
// -----------------------------------------------------------------------

std::optional<M2RendererGL::BoundingBoxResult> M2RendererGL::getBoundingBox() {
if (!m2 || m2->boundingBox.min.empty() || m2->boundingBox.max.empty())
return std::nullopt;

const auto& src_min = m2->boundingBox.min;
const auto& src_max = m2->boundingBox.max;

// wow coords: X=right, Y=forward, Z=up
// webgl coords: X=right, Y=up, Z=forward (negated)
BoundingBoxResult result;
result.min = {src_min[0], src_min[2], -src_max[1]};
result.max = {src_max[0], src_max[2], -src_min[1]};
return result;
}

// -----------------------------------------------------------------------
// _dispose_skin
// -----------------------------------------------------------------------

void M2RendererGL::_dispose_skin() {
// vao.dispose() handles vbo/ebo deletion
for (auto& vao : vaos)
vao->dispose();

vaos.clear();
buffers.clear();
draw_calls.clear();
indices_data.clear();

if (!geosetArray.empty())
geosetArray.clear();
}

// -----------------------------------------------------------------------
// dispose
// -----------------------------------------------------------------------

void M2RendererGL::dispose() {

_dispose_skin();

// dispose textures
for (auto& [key, tex] : textures)
tex->dispose();

textures.clear();

if (default_texture) {
default_texture->dispose();
default_texture.reset();
}
}
