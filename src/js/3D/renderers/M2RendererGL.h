/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
*/
#pragma once

#include <glad/gl.h>
#include <array>
#include <cstdint>
#include <future>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "../loaders/M2Loader.h"
#include "../loaders/SKELLoader.h"
#include "../loaders/M2Generics.h"
#include "../Texture.h"
#include "../Skin.h"
#include "../gl/GLContext.h"
#include "../gl/GLTexture.h"
#include "../gl/VertexArray.h"
#include "../gl/ShaderProgram.h"

namespace casc { class CASC; }
class BufferWrapper;

/**
 * Draw call descriptor for a single M2 submesh.
 */
struct M2DrawCall {
	gl::VertexArray* vao = nullptr;
	uint32_t start = 0;
	uint32_t count = 0;
	std::array<int, 4> tex_indices = { -1, -1, -1, -1 };
	uint16_t texture_count = 1;
	int vertex_shader = 0;
	int pixel_shader = 0;
	int blend_mode = 0;
	uint16_t flags = 0;
	bool visible = true;
};

/**
 * Material properties for a texture.
 */
struct M2MaterialProps {
	int blendMode = 0;
	uint16_t flags = 0;
};

/**
 * Geoset entry for reactive UI.
 */
struct M2GeosetEntry {
	std::string label;
	bool checked = false;
	uint16_t id = 0;
};

/**
 * Display info for applyReplaceableTextures.
 * JS equivalent: the `displays` argument with a `textures` array of fileDataIDs.
 */
struct M2DisplayInfo {
	std::vector<uint32_t> textures; // fileDataID per texture slot (index 0,1,2...)
};

/**
 * Modern M2 renderer using OpenGL.
 *
 * JS equivalent: class M2RendererGL — module.exports = M2RendererGL
 */
class M2RendererGL {
public:
	M2RendererGL(BufferWrapper& data, gl::GLContext& gl_context, bool reactive = false, bool useRibbon = true);

	static std::unique_ptr<gl::ShaderProgram> load_shaders(gl::GLContext& ctx);

	/**
	 * Set the active CASC source for texture and skeleton loading.
	 * JS equivalent: accesses core.view.casc directly.
	 * @param source Pointer to active CASC source.
	 */
	void setCASCSource(casc::CASC* source) { casc_source_ = source; }

	std::future<void> load();

	void _create_default_texture();
	std::future<void> _load_textures();

	std::future<void> loadSkin(int index);

	std::future<void> _create_skeleton();

	std::future<void> playAnimation(int index);
	void stopAnimation();
	void updateAnimation(float delta_time);

	float get_animation_duration();
	int get_animation_frame_count();
	int get_animation_frame();
	void set_animation_frame(int frame);
	void set_animation_paused(bool paused);
	void step_animation_frame(int delta);

	void _update_bone_matrices();

	// M2 modern format: direct per-animation keyframe arrays (M2Value variant)
	std::array<float, 3> _sample_raw_vec3(
		const std::vector<M2Value>& timestamps,
		const std::vector<M2Value>& values,
		float time_ms,
		const std::array<float, 3>& default_value = {0, 0, 0});

	std::array<float, 4> _sample_raw_quat(
		const std::vector<M2Value>& timestamps,
		const std::vector<M2Value>& values,
		float time_ms);

	/**
	 * Build bone correspondence table by matching pivot positions and boneNameCRC
	 * @param char_bones - character model bone data
	 */
	void buildBoneRemapTable(const std::vector<M2Bone>& char_bones);

	/**
	 * Apply external bone matrices using the remap table
	 * @param char_bone_matrices - character's bone matrices (flat column-major array)
	 * @param matrix_count - number of matrices available (in float elements / 16)
	 */
	void applyExternalBoneMatrices(const float* char_bone_matrices, size_t matrix_count);

	/**
	 * Set geoset visibility by group using attachmentGeosetGroup values
	 * @param group - geoset group (e.g., 18 for belt = 1800-1899)
	 * @param value - specific geoset value (1 + attachmentGeosetGroup[n])
	 */
	void setGeosetGroupDisplay(int group, int value);

	/**
	 * Hide all geosets (used before selectively showing collection geosets)
	 */
	void hideAllGeosets();

	void updateGeosets();

	void updateWireframe();

	void setTransform(const std::array<float, 3>& position, const std::array<float, 3>& rotation, const std::array<float, 3>& scale);
	void setTransformQuat(const std::array<float, 3>& position, const std::array<float, 4>& quat, const std::array<float, 3>& scale);

	/**
	 * Set model transformation using a pre-computed matrix
	 * @param matrix - 4x4 column-major transform matrix (16 floats)
	 */
	void setTransformMatrix(const float* matrix);

	/**
	 * Set hand grip state for weapon attachment
	 * @param close_right - close right hand
	 * @param close_left - close left hand
	 */
	void setHandGrip(bool close_right, bool close_left);

	void _update_model_matrix();

	void render(const float* view_matrix, const float* projection_matrix);

	/**
	 * Override texture by type using a CASC fileDataID
	 * @param type - texture type (textureTypes[i])
	 * @param fileDataID - CASC file data ID
	 */
	std::future<void> overrideTextureType(uint32_t type, uint32_t fileDataID);

	/**
	 * Override texture with raw pixel data (adapted from JS canvas override)
	 * @param type - texture type
	 * @param pixels - RGBA pixel data
	 * @param width - texture width
	 * @param height - texture height
	 */
	std::future<void> overrideTextureTypeWithCanvas(uint32_t type, const uint8_t* pixels, int width, int height);

	/**
	 * Override texture with raw RGBA pixel data
	 * @param type - texture type
	 * @param width - texture width
	 * @param height - texture height
	 * @param pixels - RGBA pixel data
	 */
	std::future<void> overrideTextureTypeWithPixels(uint32_t type, int width, int height, const uint8_t* pixels);

	/**
	 * Apply replaceable textures from display info
	 * @param displays - display info with textures array
	 */
	std::future<void> applyReplaceableTextures(const M2DisplayInfo& displays);

	/**
	 * UV layer data entry.
	 */
	struct UVLayer {
		std::string name;
		std::vector<float> data;
		bool active = false;
	};

	/**
	 * Result from getUVLayers().
	 */
	struct UVLayerResult {
		std::vector<UVLayer> layers;
		const std::vector<uint16_t>* indices = nullptr; // pointer to internal indices_data
	};

	/**
	 * Get UV layer data
	 */
	UVLayerResult getUVLayers();

	/**
	 * Posed geometry with current bone transforms applied.
	 */
	struct BakedGeometry {
		std::vector<float> vertices;
		std::vector<float> normals;
	};

	/**
	 * Get posed geometry with current bone transforms applied
	 * @returns BakedGeometry or nullopt if m2 is not loaded
	 */
	std::optional<BakedGeometry> getBakedGeometry();

	/**
	 * Get world transform matrix for an attachment point.
	 * @param attachmentId - attachment ID (e.g., 11 for helmet)
	 * @returns 4x4 column-major matrix or nullopt if not found
	 */
	std::optional<std::array<float, 16>> getAttachmentTransform(uint32_t attachmentId);

	struct BoundingBoxResult {
		std::array<float, 3> min;
		std::array<float, 3> max;
	};
	std::optional<BoundingBoxResult> getBoundingBox();

	void _dispose_skin();
	void dispose();

	// Accessors for private bone data (JS accesses these as public properties)
	const std::vector<float>& get_bone_matrices() const { return bone_matrices; }
	const std::vector<int16_t>& get_bone_remap_table() const { return bone_remap_table; }
	const std::vector<M2DrawCall>& get_draw_calls() const { return draw_calls; }

	// Accessor for model_matrix (JS: renderer.model_matrix public property)
	const float* get_model_matrix() const { return model_matrix.data(); }

	// Accessor for animation_paused (JS: renderer.animation_paused public property)
	bool is_animation_paused() const { return animation_paused; }

	// Accessor for current_animation (JS: renderer.current_animation public property)
	int get_current_animation() const { return current_animation; }

	// Accessor for skelLoader (JS: renderer.skelLoader public property)
	const SKELLoader* getSkelLoader() const { return skelLoader.get(); }

	// Accessor/mutator for geosetKey (JS: public property)
	void setGeosetKey(const std::string& key) { geosetKey = key; }
	const std::string& getGeosetKey() const { return geosetKey; }

	// Accessor for bone data (JS: renderer.bones public property)
	const std::vector<M2Bone>* get_bones_m2() const { return bones_m2; }

	// Public state (matches JS properties)
	std::unique_ptr<M2Loader> m2;
	std::unique_ptr<gl::ShaderProgram> shader;
	int syncID = -1;

private:
	BufferWrapper* data_ptr;
	gl::GLContext& ctx;
	bool reactive;
	bool useRibbon;

	// CASC source for texture/skeleton loading
	casc::CASC* casc_source_ = nullptr;

	// rendering state
	std::vector<std::unique_ptr<gl::VertexArray>> vaos;
	std::map<int, std::unique_ptr<gl::GLTexture>> textures;
	std::unique_ptr<gl::GLTexture> default_texture;
	std::vector<GLuint> buffers;
	std::vector<M2DrawCall> draw_calls;
	std::vector<uint16_t> indices_data;
	GLuint bone_ssbo = 0; // SSBO for bone matrices (avoids uniform register limits)

	// skeleton loaders
	std::unique_ptr<SKELLoader> skelLoader;
	std::unique_ptr<SKELLoader> childSkelLoader;
	std::set<std::string> childAnimKeys;
	// owned buffers for skeleton files loaded from CASC
	std::vector<std::unique_ptr<BufferWrapper>> skel_buffers_;

	// animation state
	// structural bones (hierarchy/pivot) — only one of these will be non-null at a time
	// M2Bone and SKELBone are structurally identical; separate pointers preserve type safety
	std::vector<M2Bone>*  bones_m2   = nullptr; // used when m2->bones provides the skeleton
	std::vector<SKELBone>* bones_skel = nullptr; // used when skelLoader provides the skeleton

	// returns true if any structural bone data is available
	bool has_bones() const { return bones_m2 != nullptr || bones_skel != nullptr; }
	// returns number of structural bones
	size_t bones_count() const {
		if (bones_m2) return bones_m2->size();
		if (bones_skel) return bones_skel->size();
		return 0;
	}

	// animation source tracking (which skeleton provides animation tracks)
	// If current_anim_from_skel = true, animation tracks come from skelLoader
	// If current_anim_from_child = true, animation tracks come from childSkelLoader
	// Otherwise animation tracks come from m2->bones
	bool current_anim_from_skel = false;
	bool current_anim_from_child = false;

	std::vector<float> bone_matrices;
	int current_animation = -1;   // -1 = null (no animation playing)
	int current_anim_index = -1;  // index within the source skeleton
	float animation_time = 0;
	bool animation_paused = false;

	// hand grip state for weapon attachment
	bool close_right_hand = false;
	bool close_left_hand = false;
	int hands_closed_anim_idx = -1;

	// collection model support
	std::vector<int16_t> bone_remap_table;
	bool use_external_bones = false;

	// reactive state
	std::string geosetKey = "modelViewerGeosets";
	std::vector<M2GeosetEntry> geosetArray;
	std::vector<bool> watcher_geoset_checked;
	bool watcher_wireframe = false;
	bool watcher_show_bones = false;
	bool watcher_state_initialized = false;
	std::function<void()> geosetWatcher;
	std::function<void()> wireframeWatcher;
	std::function<void()> bonesWatcher;

	// transforms
	std::array<float, 16> model_matrix;
	std::array<float, 3> position_ = {0, 0, 0};
	std::array<float, 3> rotation_ = {0, 0, 0};
	std::array<float, 3> scale_ = {1, 1, 1};

	// material data
	std::map<int, M2MaterialProps> material_props;
	std::map<int, int> shader_map;

	// internal helpers
	uint32_t _get_anim_duration_ms() const;

	// Resolve geosetKey to the corresponding view vector (JS: core.view[this.geosetKey])
	std::vector<nlohmann::json>& _get_geoset_view();
};
