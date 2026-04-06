/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
*/

#include "CharacterCameraControlsGL.h"

#include <cmath>

static constexpr float ROTATE_SPEED = 0.005f;
static constexpr float PAN_SPEED = 0.002f;
static constexpr float ZOOM_SCALE = 0.95f;
static constexpr float MIN_DISTANCE = 0.1f;
static constexpr float MAX_DISTANCE = 100.0f;

CharacterCameraControlsGL::CharacterCameraControlsGL(CharacterCameraGL& camera,
                                                       CharacterDomElementGL& dom_element)
	: camera(camera), dom_element(dom_element),
	  target({0, 0, 0}),
	  model_rotation_y(0.0f),
	  on_model_rotate(nullptr),
	  is_rotating(false),
	  is_panning(false),
	  prev_mouse_x(0),
	  prev_mouse_y(0) {
	// In C++/GLFW, event listeners are wired externally via GLFW callbacks.
	// JS: this.dom_element.addEventListener('mousedown', ...) etc.
}

void CharacterCameraControlsGL::on_mouse_down(int button, int clientX, int clientY) {
	if (button == 0) {
		is_rotating = true;
		prev_mouse_x = clientX;

		// In JS, document.addEventListener('mousemove'/'mouseup') is added here.
		// In C++/GLFW, these are handled by the GLFW callback system.
	} else if (button == 2) {
		is_panning = true;
		prev_mouse_x = clientX;
		prev_mouse_y = clientY;

		// Same as above — GLFW callbacks handle document-level mouse events.
	}
}

void CharacterCameraControlsGL::on_mouse_move(int clientX, int clientY) {
	if (is_rotating) {
		const int delta_x = clientX - prev_mouse_x;
		prev_mouse_x = clientX;

		model_rotation_y += static_cast<float>(delta_x) * ROTATE_SPEED;

		if (on_model_rotate)
			on_model_rotate(model_rotation_y);

	} else if (is_panning) {
		const int delta_x = clientX - prev_mouse_x;
		const int delta_y = clientY - prev_mouse_y;
		prev_mouse_x = clientX;
		prev_mouse_y = clientY;

		// get camera vectors
		const float px = camera.position[0], py = camera.position[1], pz = camera.position[2];
		const float tx = target[0], ty = target[1], tz = target[2];

		// forward
		float fx = tx - px, fy = ty - py, fz = tz - pz;
		const float fl = std::sqrt(fx * fx + fy * fy + fz * fz);
		if (fl > 0) { fx /= fl; fy /= fl; fz /= fl; }

		// right = forward x up
		const float ux = 0, uy = 1, uz = 0;
		float rx = fy * uz - fz * uy;
		float ry = fz * ux - fx * uz;
		float rz = fx * uy - fy * ux;
		const float rl = std::sqrt(rx * rx + ry * ry + rz * rz);
		if (rl > 0) { rx /= rl; ry /= rl; rz /= rl; }

		// up = right x forward
		const float nux = ry * fz - rz * fy;
		const float nuy = rz * fx - rx * fz;
		const float nuz = rx * fy - ry * fx;

		const float distance = fl;
		const float pan_scale = distance * PAN_SPEED;

		// apply pan
		const float fdx = static_cast<float>(delta_x);
		const float fdy = static_cast<float>(delta_y);
		const float offset_x = -fdx * pan_scale * rx + fdy * pan_scale * nux;
		const float offset_y = -fdx * pan_scale * ry + fdy * pan_scale * nuy;
		const float offset_z = -fdx * pan_scale * rz + fdy * pan_scale * nuz;

		camera.position[0] += offset_x;
		camera.position[1] += offset_y;
		camera.position[2] += offset_z;
		target[0] += offset_x;
		target[1] += offset_y;
		target[2] += offset_z;

		if (camera.lookAt)
			camera.lookAt(target[0], target[1], target[2]);
	}
}

void CharacterCameraControlsGL::on_mouse_up(int button) {
	if (button == 0) {
		is_rotating = false;

		// In JS, document.removeEventListener('mousemove'/'mouseup') is called here.
		// In C++/GLFW, these are handled by the GLFW callback system.
	} else if (button == 2) {
		is_panning = false;

		// Same as above.
	}
}

void CharacterCameraControlsGL::on_mouse_wheel(float deltaY) {
	const float px = camera.position[0], py = camera.position[1], pz = camera.position[2];
	const float tx = target[0], ty = target[1], tz = target[2];

	const float dx = px - tx, dy = py - ty, dz = pz - tz;
	const float current_distance = std::sqrt(dx * dx + dy * dy + dz * dz);

	float zoom_amount;
	if (deltaY < 0)
		zoom_amount = current_distance * (1.0f - ZOOM_SCALE);
	else if (deltaY > 0)
		zoom_amount = -current_distance * (1.0f - ZOOM_SCALE);
	else
		return;

	const float new_distance = current_distance - zoom_amount;

	if (new_distance >= MIN_DISTANCE && new_distance <= MAX_DISTANCE) {
		// normalize direction
		const float dir_x = dx / current_distance;
		const float dir_y = dy / current_distance;
		const float dir_z = dz / current_distance;

		camera.position[0] -= dir_x * zoom_amount;
		camera.position[1] -= dir_y * zoom_amount;
		camera.position[2] -= dir_z * zoom_amount;

		if (camera.update_view)
			camera.update_view();
	}
}

void CharacterCameraControlsGL::update() {
	// no-op for compatibility
}

void CharacterCameraControlsGL::dispose() {
	// In C++/GLFW, event listeners are unwired externally.
	// JS: this.dom_element.removeEventListener(...) etc.
}
