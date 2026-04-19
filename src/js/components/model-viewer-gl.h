/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <glad/gl.h>

#include "../3D/camera/CameraControlsGL.h"
#include "../3D/camera/CharacterCameraControlsGL.h"
#include "../3D/gl/GLContext.h"
#include "../3D/renderers/GridRenderer.h"
#include "../3D/renderers/ShadowPlaneRenderer.h"

class M2RendererGL;

/**
 * Model viewer GL component (ImGui immediate-mode equivalent).
 *
 * JS equivalent: Vue component with props: ['context']
 * template: `<div class="image ui-model-viewer"></div>`
 *
 * Renders a 3D model viewport with camera controls, grid, shadow plane,
 * equipment/collection model rendering, and animation updates.
 *
 * In ImGui, there is no HTML canvas or WebGL context. The 3D scene is
 * rendered to an offscreen FBO and displayed as an ImGui::Image widget.
 */
namespace model_viewer_gl {


/**
 * Parse a hex color string to normalized RGB values.
 * @param hex Hex color string (e.g., "#343a40" or "343a40")
 * @returns Array of [r, g, b] in range [0, 1]
 */
std::array<float, 3> parse_hex_color(const std::string& hex);


// general model camera fit constants
constexpr float CAMERA_FIT_DIAGONAL_ANGLE = 45.0f;
constexpr float CAMERA_FIT_DISTANCE_MULTIPLIER = 2.0f;
constexpr float CAMERA_FIT_ELEVATION_FACTOR = 0.3f;
constexpr float CAMERA_FIT_CENTER_OFFSET_Y = -0.5f;


/**
 * simple perspective camera implementation
 *
 * JS equivalent: class PerspectiveCamera { ... }
 */
class PerspectiveCamera {
public:
	PerspectiveCamera(float fov, float aspect, float near_val, float far_val);

	void update_projection();
	void update_view();
	void lookAt(float x, float y, float z);
	void setPosition(float x, float y, float z);

	float fov;
	float aspect;
	float near_plane;
	float far_plane;

	std::array<float, 3> position = {0, 0, 5};
	std::array<float, 3> target = {0, 0, 0};
	std::array<float, 3> up = {0, 1, 0};

	std::array<float, 16> view_matrix = {};
	std::array<float, 16> projection_matrix = {};
};


struct BoundingBox {
	std::array<float, 3> min;
	std::array<float, 3> max;
};

/**
 * Fit camera to bounding box using diagonal view approach
 * @param bounding_box
 * @param camera
 * @param controls
 */
void fit_camera_to_bounding_box(const BoundingBox* bounding_box, PerspectiveCamera& camera,
                                CameraControlsGL* controls);

/**
 * Fit camera for character view - fixed position tuned for humanoid models
 * @param bounding_box
 * @param camera
 * @param orbit_controls  CameraControlsGL pointer (or nullptr)
 * @param char_controls   CharacterCameraControlsGL pointer (or nullptr)
 */
void fit_camera_for_character(const BoundingBox* bounding_box, PerspectiveCamera& camera,
                              CameraControlsGL* orbit_controls,
                              CharacterCameraControlsGL* char_controls);


/**
 * Entry for an equipment renderer attached to a character.
 * JS equivalent: { renderer, attachment_id, is_collection_style } entries
 * in the equipment_renderers Map.
 */
struct EquipmentRendererEntry {
	M2RendererGL* renderer = nullptr;
	int attachment_id = -1;  // -1 = undefined (JS undefined)
	bool is_collection_style = false;
};

/**
 * Equipment slot renderers for a single slot.
 * JS equivalent: { renderers: [{renderer, attachment_id, is_collection_style}, ...] }
 */
struct EquipmentSlotRenderers {
	std::vector<EquipmentRendererEntry> renderers;
};

/**
 * Collection renderers for a single slot.
 * JS equivalent: { renderers: [renderer, ...] }
 * (no attachment_id/is_collection_style — just renderer pointers)
 */
struct CollectionSlotRenderers {
	std::vector<M2RendererGL*> renderers;
};


/**
 * Shared context between the parent tab module and the model viewer component.
 * JS equivalent: the 'context' prop passed to the Vue component.
 *
 * Some fields are set by the parent (inputs), others are written back by
 * the model viewer (outputs) during init/recreate_controls.
 */
struct Context {
	bool useCharacterControls = false;

	// Active M2 renderer (for animation, equipment, bones, etc.)
	// Returns nullptr if no M2 is active or no renderer at all.
	std::function<M2RendererGL*()> getActiveRenderer;

	// Bounding box provider for the active model of any type (M2/M3/WMO/Legacy).
	// Returns the bounding box or nullopt if no model is loaded.
	// Used by fit_camera when the active renderer may not be M2RendererGL.
	std::function<std::optional<BoundingBox>()> getActiveBoundingBox;

	// Generic render callback for active model of any type (M2/M3/WMO).
	// If getActiveRenderer returns non-null, this may be redundant (M2 render is called directly).
	// This callback covers M3/WMO rendering when the active renderer is not M2.
	std::function<void(const float*, const float*)> renderActiveModel;

	// Set transform on active model (any type). Used for auto-rotation.
	// May be null if the active model doesn't support setTransform.
	std::function<void(const std::array<float, 3>&, const std::array<float, 3>&, const std::array<float, 3>&)> setActiveModelTransform;

	// Equipment renderers (character mode only).
	// Returns a map of slot_id → EquipmentSlotRenderers, or nullptr.
	std::function<std::unordered_map<int, EquipmentSlotRenderers>*()> getEquipmentRenderers;

	// Collection renderers (character mode only).
	// Returns a map of slot_id → CollectionSlotRenderers, or nullptr.
	std::function<std::unordered_map<int, CollectionSlotRenderers>*()> getCollectionRenderers;


	// GL context used by this model viewer instance.
	gl::GLContext* gl_context = nullptr;

	// Pointer to active controls.
	// JS: `this.context.controls = this.controls;` — one duck-typed reference.
	// C++ uses two typed pointers because CameraControlsGL and CharacterCameraControlsGL
	// are distinct types. Only one should be non-null at a time (the active one).
	// Parent code accessing `context.controls` in JS should check whichever typed pointer
	// corresponds to the current mode (useCharacterControls → controls_character, else → controls_orbit).
	CameraControlsGL* controls_orbit = nullptr;
	CharacterCameraControlsGL* controls_character = nullptr;
	// Unified controls update callback (JS-equivalent duck-typed `context.controls.update()` target).
	std::function<void()> controls_update;

	// Fit camera callback. Exposed so the parent can trigger camera fitting.
	std::function<void()> fitCamera;
};


/**
 * Per-instance state for the model-viewer-gl component.
 * JS equivalent: component instance properties (this.*).
 */
struct State {
	// Camera
	PerspectiveCamera camera{70.0f, 1.0f, 0.01f, 2000.0f};

	// Camera control adapters (bridges between PerspectiveCamera and control types)
	// Both use unified CameraGL and DomElementGL types (matching JS duck typing).
	CameraGL camera_gl;
	CameraGL char_camera_gl;
	DomElementGL dom_element;
	DomElementGL char_dom_element;

	// Controls (only one active at a time)
	std::unique_ptr<CameraControlsGL> orbit_controls;
	std::unique_ptr<CharacterCameraControlsGL> char_controls;
	bool use_character_controls = false;

	// model rotation
	float model_rotation_y = 0.0f;

	// Renderers
	std::unique_ptr<GridRenderer> grid_renderer;
	std::unique_ptr<ShadowPlaneRenderer> shadow_renderer;

	// GL context (owned by this component, like JS this.gl_context = new GLContext(...))
	std::unique_ptr<gl::GLContext> gl_context;

	// Render state
	bool isRendering = false;
	float lastTime = -1.0f;
	int frameUpdateCounter = 0;

	// FBO for offscreen 3D rendering
	// via ImGui::Image. The FBO is created/resized as needed.
	GLuint fbo = 0;
	GLuint color_texture = 0;
	GLuint depth_rbo = 0;
	int fbo_width = 0;
	int fbo_height = 0;

	struct BoolWatcher {
		std::function<bool()> get;
		std::function<void()> callback;
		bool previous = false;
	};

	// Character-mode watchers (JS mounted: this.watchers.push(core.view.$watch(...)))
	std::vector<BoolWatcher> watchers;

	bool initialized = false;
};


/**
 * Initialize the model viewer (replaces Vue mounted lifecycle).
 * Creates GL context, camera, controls, grid, shadow renderers.
 */
void init(State& state, Context& context);

/**
 * Main render entry point for the model-viewer-gl ImGui widget.
 * Call once per frame from the parent tab/module.
 *
 * @param id       Unique ImGui ID for this widget.
 * @param state    Per-instance state.
 * @param context  Shared context with parent.
 */
void renderWidget(const char* id, State& state, Context& context);

/**
 * Dispose of the model viewer (replaces Vue beforeUnmount lifecycle).
 * Stops rendering, disposes GL resources.
 */
void dispose(State& state);

/**
 * Render one frame to the offscreen FBO without ImGui widget logic.
 * Used by thumbnail capture and similar scenarios that need to render
 * with modified camera/animation state outside the normal renderWidget flow.
 */
void render_one_frame(State& state, Context& context);


/**
 * Recreate camera controls based on current mode (orbit or character).
 * JS equivalent: recreate_controls method.
 */
void recreate_controls(State& state, Context& context);

/**
 * Update shadow plane visibility based on mode and config.
 * JS equivalent: update_shadow_visibility method.
 */
void update_shadow_visibility(State& state, const Context& context);

/**
 * Fit camera to the active model's bounding box.
 * JS equivalent: fit_camera method.
 */
void fit_camera(State& state, Context& context);

} // namespace model_viewer_gl
