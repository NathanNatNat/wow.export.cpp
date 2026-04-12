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
 * Camera interface expected by CameraControlsGL.
 * The actual camera implementation must provide these members.
 */
struct CameraGL {
	std::array<float, 3> position = {0, 0, 0};
	std::array<float, 3> up = {0, 1, 0};
	std::array<float, 4> quaternion = {0, 0, 0, 1};
	float fov = 50.0f;

	std::function<void(float, float, float)> lookAt;
};

/**
 * The actual implementation wraps GLFW callbacks.
 */
struct DomElementGL {
	int clientWidth = 800;
	int clientHeight = 600;
	int tabIndex = -1;

	std::function<void()> focus;
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

	void on_mouse_down(int button, int clientX, int clientY,
	                   bool ctrlKey, bool metaKey, bool shiftKey);
	void on_mouse_wheel(float deltaY);
	void on_mouse_move(int clientX, int clientY);
	void on_mouse_up();
	void on_key_down(int keyCode, bool shiftKey, bool altKey);

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
