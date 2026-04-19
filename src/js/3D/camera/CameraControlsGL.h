/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
*/
#pragma once

#include <array>
#include <cmath>
#include <functional>

/**
 * Unified camera interface used by both CameraControlsGL and CharacterCameraControlsGL.
 * JS uses duck typing — the same camera object is passed to both control types.
 * This single struct provides all members needed by either control type.
 */
struct CameraGL {
	std::array<float, 3> position = {0, 0, 0};
	std::array<float, 3> up = {0, 1, 0};
	std::array<float, 4> quaternion = {0, 0, 0, 1};
	float fov = 50.0f;

	std::function<void(float, float, float)> lookAt;
	std::function<void()> update_view;
};

/**
 * Unified DOM element interface used by both CameraControlsGL and CharacterCameraControlsGL.
 * JS uses the same DOM element for both control types via duck typing.
 */
struct DomElementGL {
	int clientWidth = 800;
	int clientHeight = 600;
	int tabIndex = -1;

	std::function<void()> focus;
	// JS: window.focus() — fallback when dom_element.focus is unavailable (CameraControlsGL line 226).
	// Caller should set this to glfwFocusWindow (or equivalent) when dom_element.focus is not set.
	std::function<void()> window_focus;
};

struct Spherical {
	float radius = 1.0f;
	float theta = 0.0f;
	float phi = 0.0f;
};

class CameraControlsGL {
public:
	CameraControlsGL(CameraGL& camera, DomElementGL& dom_element);

	void init();
	void dispose();

	bool on_mouse_down(int button, int clientX, int clientY,
	                   bool ctrlKey, bool metaKey, bool shiftKey);
	bool on_mouse_wheel(float deltaY);
	bool on_mouse_move(int clientX, int clientY);
	void on_mouse_up();
	bool on_key_down(int keyCode, bool shiftKey, bool altKey);

	void dolly_out(float scale);
	void dolly_in(float scale);
	void rotate_left(float angle);
	void rotate_up(float angle);
	void pan(float x, float y, float z);
	float get_pan_scale();
	bool update();

	CameraGL& camera;
	DomElementGL& dom_element;

	std::array<float, 3> target;

	int state;
	float scale;

	std::array<float, 3> pan_offset;

	std::array<float, 3> transform_start;
	std::array<float, 3> transform_end;
	std::array<float, 3> transform_delta;

	Spherical spherical;
	Spherical spherical_delta;

	float min_distance;
	float max_distance;

	std::array<float, 3> offset;
	std::array<float, 4> quat;
	std::array<float, 4> quat_inverse;

	std::array<float, 3> last_position;
	std::array<float, 4> last_quaternion;

	// cached vectors for calculations
	std::array<float, 3> _cache_cam_dir;
	std::array<float, 3> _cache_cam_right;
	std::array<float, 3> _cache_cam_up;
};
