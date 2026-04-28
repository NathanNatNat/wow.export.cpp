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

CharacterCameraControlsGL::CharacterCameraControlsGL(CameraGL& camera,
                                                       DomElementGL& dom_element)
	: camera(camera), dom_element(dom_element),
	  target({0, 0, 0}),
	  model_rotation_y(0.0f),
	  on_model_rotate(nullptr),
	  is_rotating(false),
	  is_panning(false),
	  prev_mouse_x(0),
	  prev_mouse_y(0) {
	// JS constructor stores four handler refs and registers three dom_element listeners:
	//   dom_element: mousedown → on_mouse_down, wheel → on_mouse_wheel,
	//                contextmenu → e.preventDefault()
	// mousemove / mouseup are NOT registered here — they are added dynamically
	// inside on_mouse_down() and removed inside on_mouse_up().
	//
	// In C++/GLFW there is no DOM event system. The caller is responsible for
	// forwarding all input to the on_* methods. GLFW has no context menu,
	// so contextmenu prevention is unnecessary. The dynamic add/remove of
	// mousemove/mouseup listeners is represented here by the is_rotating /
	// is_panning flags: move/up events are only processed when those flags are set,
	// which is exactly when JS would have the document listeners attached.
}

bool CharacterCameraControlsGL::on_mouse_down(int button, int clientX, int clientY) {
	if (button == 0) {
		is_rotating = true;
		prev_mouse_x = clientX;

		// JS: document.addEventListener('mousemove', ...) / ('mouseup', ...)
		// In C++/GLFW the caller always forwards move/up; is_rotating flag acts as the gate.

		// JS: e.preventDefault() — prevents text selection / focus steal.
		return true;
	} else if (button == 2) {
		is_panning = true;
		prev_mouse_x = clientX;
		prev_mouse_y = clientY;

		// JS: document.addEventListener('mousemove', ...) / ('mouseup', ...) — same as above.

		// JS: e.preventDefault()
		return true;
	}

	return false;
}

bool CharacterCameraControlsGL::on_mouse_move(int clientX, int clientY) {
	if (is_rotating) {
		const int delta_x = clientX - prev_mouse_x;
		prev_mouse_x = clientX;

		model_rotation_y += static_cast<float>(delta_x) * ROTATE_SPEED;

		if (on_model_rotate)
			on_model_rotate(model_rotation_y);

		// JS: e.preventDefault() — prevents text selection during rotate.
		return true;
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

		// JS calls camera.lookAt() unconditionally here (line 112).
		camera.lookAt(target[0], target[1], target[2]);

		// JS: e.preventDefault() — prevents text selection during pan.
		return true;
	}

	return false;
}

void CharacterCameraControlsGL::on_mouse_up(int button) {
	if (button == 0) {
		is_rotating = false;

		// JS: document.removeEventListener('mousemove', ...) / ('mouseup', ...)
		// In C++/GLFW, is_rotating = false is the equivalent gate: move/up events
		// forwarded by the caller will be no-ops until the next mouse-down.
	} else if (button == 2) {
		is_panning = false;

		// Same as above — is_panning = false gates further move/up processing.
	}
}

bool CharacterCameraControlsGL::on_mouse_wheel(float deltaY) {
	// JS: e.preventDefault(); e.stopPropagation(); — called unconditionally before any logic.
	// stopPropagation is handled by the caller zeroing io.MouseWheel when we return true.

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
		return true;  // JS called preventDefault/stopPropagation before this return

	const float new_distance = current_distance - zoom_amount;

	if (new_distance >= MIN_DISTANCE && new_distance <= MAX_DISTANCE) {
		// normalize direction
		const float dir_x = dx / current_distance;
		const float dir_y = dy / current_distance;
		const float dir_z = dz / current_distance;

		camera.position[0] -= dir_x * zoom_amount;
		camera.position[1] -= dir_y * zoom_amount;
		camera.position[2] -= dir_z * zoom_amount;

		// JS calls camera.update_view() unconditionally here (line 162).
		camera.update_view();
	}

	return true;  // always consumed — JS called preventDefault/stopPropagation unconditionally
}

void CharacterCameraControlsGL::update() {
	// no-op for compatibility
}

void CharacterCameraControlsGL::dispose() {
	// JS dispose() removes four event listeners:
	//   dom_element: mousedown, wheel
	//   document:    mousemove, mouseup (which were added dynamically in on_mouse_down)
	// In C++/GLFW, the caller must stop forwarding input events to this instance.
	// Resetting is_rotating/is_panning to false ensures any stale forwarded move/up
	// events are no-ops, mirroring the effect of removing the document listeners.
	is_rotating = false;
	is_panning = false;
}
