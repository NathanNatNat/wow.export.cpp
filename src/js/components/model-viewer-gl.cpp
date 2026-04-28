/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
*/
#include "model-viewer-gl.h"

#include <imgui.h>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <numbers>
#include <regex>
#include <spdlog/spdlog.h>

#include "../core.h"
#include "../3D/gl/GLContext.h"
#include "../3D/renderers/GridRenderer.h"
#include "../3D/renderers/ShadowPlaneRenderer.h"
#include "../3D/renderers/M2RendererGL.h"
#include "../wow/EquipmentSlots.h"

namespace model_viewer_gl {


std::array<float, 3> parse_hex_color(const std::string& hex) {
	static const std::regex hex_regex("^#?([a-fA-F\\d]{2})([a-fA-F\\d]{2})([a-fA-F\\d]{2})$");
	std::smatch result;
	if (!std::regex_match(hex, result, hex_regex))
		return {0, 0, 0};

	return {
		static_cast<float>(std::stoi(result[1].str(), nullptr, 16)) / 255.0f,
		static_cast<float>(std::stoi(result[2].str(), nullptr, 16)) / 255.0f,
		static_cast<float>(std::stoi(result[3].str(), nullptr, 16)) / 255.0f
	};
}


// simple perspective camera implementation
PerspectiveCamera::PerspectiveCamera(float fov, float aspect, float near_val, float far_val)
	: fov(fov), aspect(aspect), near_plane(near_val), far_plane(far_val) {
	position = {0, 0, 5};
	target = {0, 0, 0};
	up = {0, 1, 0};

	view_matrix.fill(0);
	projection_matrix.fill(0);

	update_projection();
	update_view();
}

void PerspectiveCamera::update_projection() {
	const float f = 1.0f / std::tan(fov * 0.5f * std::numbers::pi_v<float> / 180.0f);
	const float nf = 1.0f / (near_plane - far_plane);

	projection_matrix[0] = f / aspect;
	projection_matrix[1] = 0;
	projection_matrix[2] = 0;
	projection_matrix[3] = 0;
	projection_matrix[4] = 0;
	projection_matrix[5] = f;
	projection_matrix[6] = 0;
	projection_matrix[7] = 0;
	projection_matrix[8] = 0;
	projection_matrix[9] = 0;
	projection_matrix[10] = (far_plane + near_plane) * nf;
	projection_matrix[11] = -1;
	projection_matrix[12] = 0;
	projection_matrix[13] = 0;
	projection_matrix[14] = 2 * far_plane * near_plane * nf;
	projection_matrix[15] = 0;
}

void PerspectiveCamera::update_view() {
	const float px = position[0], py = position[1], pz = position[2];
	const float tx = target[0], ty = target[1], tz = target[2];
	const float ux = up[0], uy = up[1], uz = up[2];

	// forward
	float fx = px - tx, fy = py - ty, fz = pz - tz;
	float fl = std::sqrt(fx * fx + fy * fy + fz * fz);
	if (fl > 0) { fx /= fl; fy /= fl; fz /= fl; }

	// right = up x forward
	float rx = uy * fz - uz * fy;
	float ry = uz * fx - ux * fz;
	float rz = ux * fy - uy * fx;
	float rl = std::sqrt(rx * rx + ry * ry + rz * rz);
	if (rl > 0) { rx /= rl; ry /= rl; rz /= rl; }

	// up = forward x right
	const float nux = fy * rz - fz * ry;
	const float nuy = fz * rx - fx * rz;
	const float nuz = fx * ry - fy * rx;

	view_matrix[0] = rx;
	view_matrix[1] = nux;
	view_matrix[2] = fx;
	view_matrix[3] = 0;
	view_matrix[4] = ry;
	view_matrix[5] = nuy;
	view_matrix[6] = fy;
	view_matrix[7] = 0;
	view_matrix[8] = rz;
	view_matrix[9] = nuz;
	view_matrix[10] = fz;
	view_matrix[11] = 0;
	view_matrix[12] = -(rx * px + ry * py + rz * pz);
	view_matrix[13] = -(nux * px + nuy * py + nuz * pz);
	view_matrix[14] = -(fx * px + fy * py + fz * pz);
	view_matrix[15] = 1;
}

void PerspectiveCamera::lookAt(float x, float y, float z) {
	target[0] = x;
	target[1] = y;
	target[2] = z;
	update_view();
}

void PerspectiveCamera::setPosition(float x, float y, float z) {
	position[0] = x;
	position[1] = y;
	position[2] = z;
	update_view();
}


/**
 * Fit camera to bounding box using diagonal view approach
 * @param bounding_box
 * @param camera
 * @param controls
 */
void fit_camera_to_bounding_box(const BoundingBox* bounding_box, PerspectiveCamera& camera,
                                CameraControlsGL* controls) {
	if (!bounding_box)
		return;

	const auto& min = bounding_box->min;
	const auto& max = bounding_box->max;

	// calculate center with offset
	const float center_x = (min[0] + max[0]) / 2.0f;
	const float center_y = (min[1] + max[1]) / 2.0f + CAMERA_FIT_CENTER_OFFSET_Y;
	const float center_z = (min[2] + max[2]) / 2.0f;

	// calculate dimensions and max dimension
	const float size_x = max[0] - min[0];
	const float size_y = max[1] - min[1];
	const float size_z = max[2] - min[2];
	const float max_dimension = std::max({size_x, size_y, size_z});

	// calculate required distance based on camera FOV and object size
	const float fov_radians = camera.fov * (std::numbers::pi_v<float> / 180.0f);
	const float distance = (max_dimension / 2.0f) / std::tan(fov_radians / 2.0f) * CAMERA_FIT_DISTANCE_MULTIPLIER;

	// calculate camera position at diagonal angle with elevation
	const float angle_rad = CAMERA_FIT_DIAGONAL_ANGLE * (std::numbers::pi_v<float> / 180.0f);

	const float offset_x = distance * std::sin(angle_rad);
	const float offset_y = distance * CAMERA_FIT_ELEVATION_FACTOR;
	const float offset_z = distance * std::cos(angle_rad);

	// position camera
	camera.position[0] = center_x + offset_x;
	camera.position[1] = center_y + offset_y;
	camera.position[2] = center_z + offset_z;
	camera.lookAt(center_x, center_y, center_z);

	// update controls target
	if (controls) {
		controls->target[0] = center_x;
		controls->target[1] = center_y;
		controls->target[2] = center_z;

		controls->max_distance = distance * 3;
		controls->update();
	}
}

/**
 * Fit camera for character view - fixed position tuned for humanoid models
 * @param bounding_box
 * @param camera
 * @param orbit_controls
 * @param char_controls
 */
void fit_camera_for_character(const BoundingBox* /*bounding_box*/, PerspectiveCamera& camera,
                              CameraControlsGL* orbit_controls,
                              CharacterCameraControlsGL* char_controls) {
	camera.position[0] = 0;
	camera.position[1] = 1.609f;
	camera.position[2] = 2.347f;

	const float target_x = 0;
	const float target_y = 1.247f;
	const float target_z = 0.537f;

	camera.lookAt(target_x, target_y, target_z);

	// JS takes a single duck-typed `controls` parameter and updates only that one.
	// C++ splits into two typed pointers. Only the active one (non-null) should be updated.
	// The caller passes the appropriate controls; typically only one is non-null.
	if (char_controls) {
		char_controls->target[0] = target_x;
		char_controls->target[1] = target_y;
		char_controls->target[2] = target_z;

		char_controls->update();
	} else if (orbit_controls) {
		orbit_controls->target[0] = target_x;
		orbit_controls->target[1] = target_y;
		orbit_controls->target[2] = target_z;

		orbit_controls->update();
	}
}


// via ImGui::Image. These helpers manage FBO lifecycle.

static void create_fbo(State& state, int width, int height) {
	// Clean up existing FBO
	if (state.fbo != 0) {
		glDeleteFramebuffers(1, &state.fbo);
		glDeleteTextures(1, &state.color_texture);
		glDeleteRenderbuffers(1, &state.depth_rbo);
	}

	// Create color texture
	glGenTextures(1, &state.color_texture);
	glBindTexture(GL_TEXTURE_2D, state.color_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Create depth/stencil renderbuffer
	glGenRenderbuffers(1, &state.depth_rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, state.depth_rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

	// Create FBO
	glGenFramebuffers(1, &state.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, state.fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, state.color_texture, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, state.depth_rbo);

	// Verify completeness
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		spdlog::error("model-viewer-gl: FBO not complete, status={}", status);
	}

	// Unbind
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	state.fbo_width = width;
	state.fbo_height = height;
}

static void destroy_fbo(State& state) {
	if (state.fbo != 0) {
		glDeleteFramebuffers(1, &state.fbo);
		glDeleteTextures(1, &state.color_texture);
		glDeleteRenderbuffers(1, &state.depth_rbo);
		state.fbo = 0;
		state.color_texture = 0;
		state.depth_rbo = 0;
		state.fbo_width = 0;
		state.fbo_height = 0;
	}
}


// Sync PerspectiveCamera position → CameraGL adapter (before orbit controls update)
static void sync_camera_to_gl(State& state) {
	state.camera_gl.position = state.camera.position;
	state.camera_gl.up = state.camera.up;
	state.camera_gl.fov = state.camera.fov;
}

// Sync PerspectiveCamera position → CharacterCameraGL adapter (before char controls update)
static void sync_camera_to_char_gl(State& state) {
	state.char_camera_gl.position = state.camera.position;
}

// Set up the CameraGL adapter lookAt callback to sync back to PerspectiveCamera
static void setup_camera_gl_callbacks(State& state) {
	state.camera_gl.lookAt = [&state](float x, float y, float z) {
		// Controls modified camera_gl.position, sync it back to PerspectiveCamera
		state.camera.position = state.camera_gl.position;
		state.camera.lookAt(x, y, z);
	};

	state.char_camera_gl.lookAt = [&state](float x, float y, float z) {
		// Controls modified char_camera_gl.position, sync it back
		state.camera.position = state.char_camera_gl.position;
		state.camera.lookAt(x, y, z);
	};

	state.char_camera_gl.update_view = [&state]() {
		state.camera.update_view();
	};
}


// mouse and keyboard events. In C++/ImGui, we query ImGui::GetIO() and forward
// events to the camera controls. This handles the equivalent of:
//   canvas.addEventListener('mousedown', ...)
//   document.addEventListener('mousemove', ...)
//   document.addEventListener('mouseup', ...)
//   canvas.addEventListener('wheel', ...)
//   document.addEventListener('keydown', ...)
// Handlers return bool to indicate event consumption (JS preventDefault/stopPropagation).
// Consumed wheel events zero io.MouseWheel to prevent parent window scrolling (stopPropagation).
static void handle_input(State& state) {
	ImGuiIO& io = ImGui::GetIO();
	const bool is_hovered = ImGui::IsItemHovered();

	// Mouse position relative to widget
	ImVec2 widget_pos = ImGui::GetItemRectMin();
	int mouse_x = static_cast<int>(io.MousePos.x - widget_pos.x);
	int mouse_y = static_cast<int>(io.MousePos.y - widget_pos.y);

	// Mouse wheel (canvas-level in JS)
	// JS: on_mouse_wheel calls preventDefault() + stopPropagation() when consumed.
	// stopPropagation equivalent: zero io.MouseWheel to prevent ImGui parent scroll.
	if (is_hovered && io.MouseWheel != 0.0f) {
		bool consumed = false;
		if (state.use_character_controls && state.char_controls) {
			consumed = state.char_controls->on_mouse_wheel(-io.MouseWheel * 100.0f);
		} else if (state.orbit_controls) {
			consumed = state.orbit_controls->on_mouse_wheel(-io.MouseWheel * 100.0f);
		}
		if (consumed)
			io.MouseWheel = 0.0f;  // stopPropagation: prevent parent window from scrolling
	}

	// Mouse down (canvas-level in JS)
	for (int btn = 0; btn < 3; btn++) {
		if (is_hovered && ImGui::IsMouseClicked(btn)) {
			if (state.use_character_controls && state.char_controls) {
				state.char_controls->on_mouse_down(btn, mouse_x, mouse_y);
			} else if (state.orbit_controls) {
				state.orbit_controls->on_mouse_down(btn, mouse_x, mouse_y,
					io.KeyCtrl, io.KeySuper, io.KeyShift);
			}
		}
	}

	// Key down — on_key_down returns true only for handled keys (JS preventDefault)
	if (state.orbit_controls && !state.use_character_controls) {
		for (int key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; key++) {
			if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(key))) {
				state.orbit_controls->on_key_down(key, io.KeyShift, io.KeyAlt);
			}
		}
	}

	// Mouse move (always forward, regardless of hover, since panning may extend outside)
	if (io.MouseDelta.x != 0 || io.MouseDelta.y != 0) {
		if (state.use_character_controls && state.char_controls) {
			state.char_controls->on_mouse_move(mouse_x, mouse_y);
		} else if (state.orbit_controls) {
			state.orbit_controls->on_mouse_move(mouse_x, mouse_y);
		}
	}

	// Mouse up (document-level in JS)
	for (int btn = 0; btn < 3; btn++) {
		if (ImGui::IsMouseReleased(btn)) {
			if (state.use_character_controls && state.char_controls) {
				state.char_controls->on_mouse_up(btn);
			} else if (state.orbit_controls) {
				state.orbit_controls->on_mouse_up();
			}
		}
	}
}


/**
 * Render the 3D scene to the FBO.
 * JS equivalent: the render() method of the Vue component.
 */
static void render_scene(State& state, Context& context) {
	if (!state.isRendering || !state.gl_context)
		return;

	// Calculate delta time
	float currentTime = static_cast<float>(ImGui::GetTime());
	if (state.lastTime < 0)
		state.lastTime = currentTime;

	const float deltaTime = currentTime - state.lastTime;
	state.lastTime = currentTime;

	// update animation
	// JS: if (activeRenderer && activeRenderer.updateAnimation) — duck-typed check.
	// In C++, M2RendererGL always has updateAnimation, so a null check suffices.
	M2RendererGL* activeRenderer = context.getActiveRenderer ? context.getActiveRenderer() : nullptr;

	if (activeRenderer) {
		activeRenderer->updateAnimation(deltaTime);

		// update frame counter in view (throttled to ~15fps for UI updates)
		// JS: if (activeRenderer.get_animation_frame && !activeRenderer.animation_paused)
		// In C++, M2RendererGL always has these methods, so null check on activeRenderer suffices.
		if (!activeRenderer->is_animation_paused()) {
			state.frameUpdateCounter = (state.frameUpdateCounter) + 1;
			if (state.frameUpdateCounter >= 4) {
				state.frameUpdateCounter = 0;
				const std::string frame_key = context.useCharacterControls ? "chrModelViewerAnimFrame" : "modelViewerAnimFrame";
				if (context.useCharacterControls) {
					core::view->chrModelViewerAnimFrame = activeRenderer->get_animation_frame();
				} else {
					core::view->modelViewerAnimFrame = activeRenderer->get_animation_frame();
				}
			}
		}
	}

	// apply model rotation if speed is non-zero (non-character mode)
	// JS: if (rotation_speed !== 0 && activeRenderer && activeRenderer.setTransform && !this.use_character_controls)
	//
	// C++ extension: JS auto-rotation only routes through `activeRenderer.setTransform`
	// (M2 only). C++ adds an `else if` branch using `context.setActiveModelTransform`
	// when `activeRenderer` is null, to support WMO/M3 models that have no
	// `activeRenderer` equivalent in this port.
	const double rotation_speed = core::view->modelViewerRotationSpeed;
	if (rotation_speed != 0 && activeRenderer && !state.use_character_controls) {
		state.model_rotation_y += static_cast<float>(rotation_speed) * deltaTime;
		activeRenderer->setTransform(
			{0, 0, 0},
			{0, state.model_rotation_y, 0},
			{1, 1, 1}
		);
	} else if (rotation_speed != 0 && !state.use_character_controls && context.setActiveModelTransform) {
		state.model_rotation_y += static_cast<float>(rotation_speed) * deltaTime;
		context.setActiveModelTransform(
			{0, 0, 0},
			{0, state.model_rotation_y, 0},
			{1, 1, 1}
		);
	}

	// update controls (JS: this.controls.update())
	if (context.controls_update)
		context.controls_update();

	// Bind FBO for 3D rendering
	glBindFramebuffer(GL_FRAMEBUFFER, state.fbo);

	// ImGui's OpenGL3 backend modifies GL state (program, VAO, textures,
	// blend, depth, etc.) between frames without going through our GLContext
	// cache.  Invalidate the cache so all subsequent set_*/bind_* calls
	// actually issue the real GL commands.
	state.gl_context->invalidate_cache();

	state.gl_context->set_viewport(state.fbo_width, state.fbo_height);

	// clear with appropriate background
	const bool is_chr = context.useCharacterControls;
	const bool show_bg = is_chr
		? core::view->config.value("chrShowBackground", false)
		: core::view->config.value("modelViewerShowBackground", false);
	const std::string bg_color = is_chr
		? core::view->config.value("chrBackgroundColor", std::string("#343a40"))
		: core::view->config.value("modelViewerBackgroundColor", std::string("#343a40"));

	if (show_bg) {
		const auto [r, g, b] = parse_hex_color(bg_color);
		state.gl_context->set_clear_color(r, g, b, 1);
	} else {
		state.gl_context->set_clear_color(0, 0, 0, 0);
	}
	state.gl_context->clear(true, true);

	const float* view_mat = state.camera.view_matrix.data();
	const float* proj_mat = state.camera.projection_matrix.data();

	// render shadow plane (before model, for character mode)
	if (state.shadow_renderer && state.shadow_renderer->visible)
		state.shadow_renderer->render(view_mat, proj_mat);

	// render grid (not in character mode)
	if (core::view->config.value("modelViewerShowGrid", true) && state.grid_renderer && !context.useCharacterControls)
		state.grid_renderer->render(view_mat, proj_mat);

	// render equipment models at attachment points (character mode only)
	auto* equipment_renderers = context.getEquipmentRenderers ? context.getEquipmentRenderers() : nullptr;

	// determine hand grip state based on equipped weapons
	// JS: if (activeRenderer && equipment_renderers && activeRenderer.setHandGrip)
	// In C++, M2RendererGL always has setHandGrip, so null check on activeRenderer suffices.
	if (activeRenderer && equipment_renderers) {
		bool close_right = false;
		bool close_left = false;

		// check main-hand (slot 16) and off-hand (slot 17)
		auto mainhand_it = equipment_renderers->find(16);
		auto offhand_it = equipment_renderers->find(17);

		if (mainhand_it != equipment_renderers->end() && !mainhand_it->second.renderers.empty()) {
			const auto& mainhand = mainhand_it->second;
			// bows use left-hand attachment despite being main-hand items
			bool uses_left_hand = false;
			for (const auto& r : mainhand.renderers) {
				if (r.attachment_id == wow::ATTACHMENT_ID::HAND_LEFT) {
					uses_left_hand = true;
					break;
				}
			}
			if (uses_left_hand)
				close_left = true;
			else
				close_right = true;
		}

		if (offhand_it != equipment_renderers->end() && !offhand_it->second.renderers.empty()) {
			const auto& offhand = offhand_it->second;
			// shields don't require grip (attached to wrist)
			bool has_shield = false;
			for (const auto& r : offhand.renderers) {
				if (r.attachment_id == wow::ATTACHMENT_ID::SHIELD) {
					has_shield = true;
					break;
				}
			}
			if (!has_shield)
				close_left = true;
		}

		activeRenderer->setHandGrip(close_right, close_left);
	}

	// render model
	// JS: if (activeRenderer && activeRenderer.render) activeRenderer.render(...)
	if (activeRenderer) {
		activeRenderer->render(view_mat, proj_mat);
	} else if (context.renderActiveModel) {
		context.renderActiveModel(view_mat, proj_mat);
	}

	if (equipment_renderers && activeRenderer) {
		const auto& char_bone_matrices = activeRenderer->get_bone_matrices();
		const float* char_model_matrix = activeRenderer->get_model_matrix();

		for (auto& [slot_id, slot_entry] : *equipment_renderers) {
			if (slot_entry.renderers.empty())
				continue;

			for (const auto& entry : slot_entry.renderers) {
				if (!entry.renderer)
					continue;

				// collection-style models (e.g. backpacks) need bone matrix remapping
				if (entry.is_collection_style) {
					if (!char_bone_matrices.empty())
						entry.renderer->applyExternalBoneMatrices(char_bone_matrices.data(), char_bone_matrices.size() / 16);

					if (char_model_matrix)
						entry.renderer->setTransformMatrix(char_model_matrix);
				} else if (entry.attachment_id >= 0) {
					// regular attachment models use attachment transform
					auto attach_transform = activeRenderer->getAttachmentTransform(static_cast<uint32_t>(entry.attachment_id));
					if (attach_transform)
						entry.renderer->setTransformMatrix(attach_transform->data());
				}

				entry.renderer->render(view_mat, proj_mat);
			}
		}
	}

	// render collection models with remapped bone matrices
	auto* collection_renderers = context.getCollectionRenderers ? context.getCollectionRenderers() : nullptr;
	if (collection_renderers && activeRenderer) {
		const auto& char_bone_matrices = activeRenderer->get_bone_matrices();
		const float* char_model_matrix = activeRenderer->get_model_matrix();

		for (auto& [slot_id, slot_entry] : *collection_renderers) {
			if (slot_entry.renderers.empty())
				continue;

			for (auto* renderer : slot_entry.renderers) {
				if (!renderer)
					continue;

				// apply remapped bone matrices for proper rigging
				if (!char_bone_matrices.empty())
					renderer->applyExternalBoneMatrices(char_bone_matrices.data(), char_bone_matrices.size() / 16);

				// use character's model transform (rotation from controls)
				if (char_model_matrix)
					renderer->setTransformMatrix(char_model_matrix);

				renderer->render(view_mat, proj_mat);
			}
		}
	}

	// Unbind FBO
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


void recreate_controls(State& state, Context& context) {
	// Dispose existing controls
	if (state.orbit_controls) {
		state.orbit_controls->dispose();
		state.orbit_controls.reset();
	}
	if (state.char_controls) {
		state.char_controls->dispose();
		state.char_controls.reset();
	}

	const bool use_3d_camera = context.useCharacterControls
		? core::view->config.value("chrUse3DCamera", true)
		: true;

	if (context.useCharacterControls && !use_3d_camera) {
		// Character camera controls
		sync_camera_to_char_gl(state);
		state.char_controls = std::make_unique<CharacterCameraControlsGL>(state.char_camera_gl, state.char_dom_element);

		state.char_controls->on_model_rotate = [&state, &context](float rotation_y) {
			M2RendererGL* active_renderer = context.getActiveRenderer ? context.getActiveRenderer() : nullptr;
			if (active_renderer)
				active_renderer->setTransform({0, 0, 0}, {0, rotation_y, 0}, {1, 1, 1});
		};

		// initial 90 degree clockwise rotation
		state.char_controls->model_rotation_y = -std::numbers::pi_v<float> / 2.0f;
		state.char_controls->on_model_rotate(state.char_controls->model_rotation_y);

		state.use_character_controls = true;

		// Update context outputs (JS: this.context.controls = this.controls).
		// JS deviation: JS uses a single polymorphic `context.controls` pointer.
		// C++ splits this into typed pointers `controls_orbit` (CameraControlsGL)
		// and `controls_character` (CharacterCameraControlsGL) because C++ has no
		// duck-typing. Only one is non-null at any time; the active mode determines
		// which. External callers (e.g. `tab_characters.cpp`) must check the
		// appropriate typed pointer. See header for the unified `controls_update`
		// callback that abstracts over both.
		context.controls_character = state.char_controls.get();
		context.controls_orbit = nullptr;
		context.controls_update = [&state]() {
			sync_camera_to_char_gl(state);
			if (state.char_controls)
				state.char_controls->update();
		};
	} else {
		// Orbit camera controls
		sync_camera_to_gl(state);
		state.orbit_controls = std::make_unique<CameraControlsGL>(state.camera_gl, state.dom_element);

		state.use_character_controls = false;

		// Update context outputs (JS: this.context.controls = this.controls).
		// JS deviation: see character-controls branch above for rationale on the
		// `controls_orbit` / `controls_character` split.
		context.controls_orbit = state.orbit_controls.get();
		context.controls_character = nullptr;
		context.controls_update = [&state]() {
			sync_camera_to_gl(state);
			if (state.orbit_controls)
				state.orbit_controls->update();
		};
	}
}

void update_shadow_visibility(State& state, const Context& context) {
	if (!state.shadow_renderer)
		return;

	const bool should_show = context.useCharacterControls &&
		core::view->config.value("chrRenderShadow", false) &&
		!core::view->chrModelLoading;

	state.shadow_renderer->visible = should_show;
}

static void setup_character_watchers(State& state, Context& context) {
	state.watchers.clear();

	if (!context.useCharacterControls)
		return;

	// JS deviation: JS uses Vue's `$watch` for immediate reactive callbacks on
	// config changes (e.g. `core.view.$watch('config.chrUse3DCamera', ...)`).
	// C++ has no Vue runtime; instead each watcher records its previous value
	// and is polled per-frame in `poll_watchers`, comparing prev vs current.
	// This introduces a one-frame latency between config change and callback
	// invocation, which is acceptable for camera/control mode toggles.
	state.watchers.push_back(State::BoolWatcher{
		.get = []() { return core::view->config.value("chrUse3DCamera", true); },
		.callback = [&state, &context]() { recreate_controls(state, context); },
		.previous = core::view->config.value("chrUse3DCamera", true)
	});

	state.watchers.push_back(State::BoolWatcher{
		.get = []() { return core::view->config.value("chrRenderShadow", false); },
		.callback = [&state, &context]() { update_shadow_visibility(state, context); },
		.previous = core::view->config.value("chrRenderShadow", false)
	});

	state.watchers.push_back(State::BoolWatcher{
		.get = []() { return core::view->chrModelLoading; },
		.callback = [&state, &context]() { update_shadow_visibility(state, context); },
		.previous = core::view->chrModelLoading
	});
}

// JS deviation: JS uses Vue's `$watch` for immediate reactive callbacks on
// config changes. C++ has no Vue runtime; instead each watcher records its
// previous value and is polled per-frame here, comparing prev vs current.
// This introduces a one-frame latency between config change and callback
// invocation, which is acceptable for camera/control mode toggles.
static void poll_watchers(State& state) {
	for (auto& watcher : state.watchers) {
		const bool value = watcher.get ? watcher.get() : watcher.previous;
		if (value != watcher.previous) {
			watcher.previous = value;
			if (watcher.callback)
				watcher.callback();
		}
	}
}

void fit_camera(State& state, Context& context) {
	// JS: const active_renderer = this.context.getActiveRenderer?.();
	//     if (!active_renderer || !active_renderer.getBoundingBox) return;
	//     const bounding_box = active_renderer.getBoundingBox();
	M2RendererGL* active_renderer = context.getActiveRenderer ? context.getActiveRenderer() : nullptr;
	std::optional<BoundingBox> bounding_box;

	if (active_renderer) {
		auto m2_bb = active_renderer->getBoundingBox();
		if (!m2_bb)
			return;
		bounding_box = BoundingBox{ m2_bb->min, m2_bb->max };
	} else if (context.getActiveBoundingBox) {
		bounding_box = context.getActiveBoundingBox();
		if (!bounding_box)
			return;
	} else {
		return;
	}

	if (context.useCharacterControls) {
		fit_camera_for_character(&(*bounding_box), state.camera,
			state.orbit_controls.get(),
			state.char_controls.get());
	} else {
		fit_camera_to_bounding_box(&(*bounding_box), state.camera, state.orbit_controls.get());
	}
}


void init(State& state, Context& context) {
	// JS: this.gl_context = new GLContext(canvas, { antialias: true, alpha: true, preserveDrawingBuffer: true });
	// In desktop OpenGL, these WebGL context options map as follows:
	// - antialias: Multisampling is not needed since we render to an FBO (can add MSAA FBO if needed).
	// - alpha: The FBO color attachment uses GL_RGBA8, which includes alpha.
	// - preserveDrawingBuffer: Not applicable — desktop GL framebuffers are always preserved.
	state.gl_context = std::make_unique<gl::GLContext>();

	// store context for renderers
	context.gl_context = state.gl_context.get();

	// create camera (already default-constructed in State)
	// (State default-initializes camera with these values)

	// model rotation
	state.model_rotation_y = 0;
	state.use_character_controls = false;

	// Set up camera adapter callbacks
	setup_camera_gl_callbacks(state);

	// create controls
	recreate_controls(state, context);

	// expose fit_camera on context
	context.fitCamera = [&state, &context]() { fit_camera(state, context); };

	// create grid renderer
	state.grid_renderer = std::make_unique<GridRenderer>(*state.gl_context, 100.0f, 100);

	// create shadow renderer (for character mode)
	state.shadow_renderer = std::make_unique<ShadowPlaneRenderer>(*state.gl_context, 2.0f);
	state.shadow_renderer->visible = false;
	update_shadow_visibility(state, context);
	setup_character_watchers(state, context);

	// start render loop
	// isRendering controls whether render_scene() executes.
	state.isRendering = true;

	state.initialized = true;
}

void dispose(State& state) {
	state.isRendering = false;

	if (state.orbit_controls) {
		state.orbit_controls->dispose();
		state.orbit_controls.reset();
	}
	if (state.char_controls) {
		state.char_controls->dispose();
		state.char_controls.reset();
	}

	if (state.grid_renderer) {
		state.grid_renderer->dispose();
		state.grid_renderer.reset();
	}

	if (state.shadow_renderer) {
		state.shadow_renderer->dispose();
		state.shadow_renderer.reset();
	}

	if (state.gl_context) {
		state.gl_context->dispose();
		state.gl_context.reset();
	}


	// Destroy FBO
	destroy_fbo(state);
	state.watchers.clear();

	state.initialized = false;
}

void renderWidget(const char* id, State& state, Context& context) {
	ImGui::PushID(id);

	ImVec2 avail = ImGui::GetContentRegionAvail();
	int width = static_cast<int>(avail.x);
	int height = static_cast<int>(avail.y);

	if (width <= 0 || height <= 0) {
		ImGui::PopID();
		return;
	}

	// Initialize if needed
	if (!state.initialized) {
		init(state, context);
	}

	// resize handler
	// JS: canvas.width = width * window.devicePixelRatio (lines 482–483).
	// In desktop OpenGL, ImGui::GetContentRegionAvail() already returns dimensions
	// in framebuffer pixels (accounting for DPI scaling done by GLFW). The FBO is
	// sized to match these pixel dimensions, providing full-resolution rendering.
	// If GLFW content scale != 1.0 in the future, this should be revisited to
	// multiply by the content scale factor (equivalent of devicePixelRatio).
	if (state.fbo_width != width || state.fbo_height != height) {
		create_fbo(state, width, height);

		state.gl_context->set_viewport(width, height);

		state.camera.aspect = static_cast<float>(width) / static_cast<float>(height);
		state.camera.update_projection();

		// Update dom element dimensions for controls
		state.dom_element.clientWidth = width;
		state.dom_element.clientHeight = height;
		state.char_dom_element.clientWidth = width;
		state.char_dom_element.clientHeight = height;
	}

	// JS parity: execute mounted character-mode watchers.
	poll_watchers(state);

	// Render 3D scene to FBO
	render_scene(state, context);

	// Display FBO as ImGui image (UV flipped for OpenGL → ImGui coordinate convention)
	ImGui::Image(static_cast<ImTextureID>(static_cast<uintptr_t>(state.color_texture)),
	             ImVec2(static_cast<float>(width), static_cast<float>(height)),
	             ImVec2(0, 1), ImVec2(1, 0));

	// Handle input (mouse/keyboard → camera controls)
	handle_input(state);

	// renderWidget is called each frame by the parent tab, which is equivalent
	// to requestAnimationFrame in the JS version.

	ImGui::PopID();
}

void render_one_frame(State& state, Context& context) {
	render_scene(state, context);
}

} // namespace model_viewer_gl
