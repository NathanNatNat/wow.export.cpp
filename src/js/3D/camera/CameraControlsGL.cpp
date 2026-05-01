/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
*/

#include "CameraControlsGL.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numbers>

static constexpr int MOUSE_BUTTON_LEFT = 0;
static constexpr int MOUSE_BUTTON_MIDDLE = 1;
static constexpr int MOUSE_BUTTON_RIGHT = 2;

static constexpr int STATE_NONE = 0x0;
static constexpr int STATE_PANNING = 0x1;
static constexpr int STATE_ROTATING = 0x2;
static constexpr int STATE_DOLLYING = 0x3;

static constexpr int KEY_W = 87;
static constexpr int KEY_S = 83;
static constexpr int KEY_A = 65;
static constexpr int KEY_D = 68;
static constexpr int KEY_Q = 81;
static constexpr int KEY_E = 69;

static constexpr float EPS = 0.000001f;

static constexpr float ZOOM_SCALE = 0.95f;

static constexpr float ROTATE_SPEED = 1.0f;
static constexpr float PAN_SPEED = 1.0f;

static constexpr float KEY_PAN_SPEED = 3.0f;
static constexpr float KEY_PAN_SPEED_SHIFT = 0.5f;
static constexpr float KEY_PAN_SPEED_ALT = 0.05f;

static constexpr float MIN_POLAR_ANGLE = 0.0f;
static constexpr float MAX_POLAR_ANGLE = static_cast<float>(std::numbers::pi);

// vec3 math utilities
using vec3 = std::array<float, 3>;
using quat = std::array<float, 4>;

static vec3 vec3_create() { return {0, 0, 0}; }
static vec3& vec3_copy(vec3& out, const vec3& a) { out[0] = a[0]; out[1] = a[1]; out[2] = a[2]; return out; }
static vec3& vec3_set(vec3& out, float x, float y, float z) { out[0] = x; out[1] = y; out[2] = z; return out; }
static vec3& vec3_add(vec3& out, const vec3& a, const vec3& b) { out[0] = a[0] + b[0]; out[1] = a[1] + b[1]; out[2] = a[2] + b[2]; return out; }
static vec3& vec3_sub(vec3& out, const vec3& a, const vec3& b) { out[0] = a[0] - b[0]; out[1] = a[1] - b[1]; out[2] = a[2] - b[2]; return out; }
static vec3& vec3_scale(vec3& out, const vec3& a, float s) { out[0] = a[0] * s; out[1] = a[1] * s; out[2] = a[2] * s; return out; }
static float vec3_length(const vec3& a) { return std::sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]); }
static vec3& vec3_normalize(vec3& out, const vec3& a) {
	const float len = vec3_length(a);
	if (len > 0) {
		out[0] = a[0] / len;
		out[1] = a[1] / len;
		out[2] = a[2] / len;
	}
	return out;
}
static vec3& vec3_cross(vec3& out, const vec3& a, const vec3& b) {
	const float ax = a[0], ay = a[1], az = a[2];
	const float bx = b[0], by = b[1], bz = b[2];
	out[0] = ay * bz - az * by;
	out[1] = az * bx - ax * bz;
	out[2] = ax * by - ay * bx;
	return out;
}
static float vec3_distance_squared(const vec3& a, const vec3& b) {
	const float dx = a[0] - b[0], dy = a[1] - b[1], dz = a[2] - b[2];
	return dx * dx + dy * dy + dz * dz;
}

// quat math utilities
static quat quat_create() { return {0, 0, 0, 1}; }
static quat& quat_set_from_unit_vectors(quat& out, const vec3& from, const vec3& to) {
	float r = from[0] * to[0] + from[1] * to[1] + from[2] * to[2] + 1.0f;

	if (r < EPS) {
		r = 0;
		if (std::abs(from[0]) > std::abs(from[2])) {
			out[0] = -from[1];
			out[1] = from[0];
			out[2] = 0;
			out[3] = r;
		} else {
			out[0] = 0;
			out[1] = -from[2];
			out[2] = from[1];
			out[3] = r;
		}
	} else {
		out[0] = from[1] * to[2] - from[2] * to[1];
		out[1] = from[2] * to[0] - from[0] * to[2];
		out[2] = from[0] * to[1] - from[1] * to[0];
		out[3] = r;
	}

	// normalize
	const float len = std::sqrt(out[0] * out[0] + out[1] * out[1] + out[2] * out[2] + out[3] * out[3]);
	if (len > 0) {
		out[0] /= len;
		out[1] /= len;
		out[2] /= len;
		out[3] /= len;
	}
	return out;
}

static quat& quat_invert(quat& out, const quat& a) {
	out[0] = -a[0];
	out[1] = -a[1];
	out[2] = -a[2];
	out[3] = a[3];
	return out;
}

static float quat_dot(const quat& a, const quat& b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3]; }

// apply quaternion to vec3
static vec3& vec3_apply_quat(vec3& out, const vec3& v, const quat& q) {
	const float x = v[0], y = v[1], z = v[2];
	const float qx = q[0], qy = q[1], qz = q[2], qw = q[3];

	const float ix = qw * x + qy * z - qz * y;
	const float iy = qw * y + qz * x - qx * z;
	const float iz = qw * z + qx * y - qy * x;
	const float iw = -qx * x - qy * y - qz * z;

	out[0] = ix * qw + iw * -qx + iy * -qz - iz * -qy;
	out[1] = iy * qw + iw * -qy + iz * -qx - ix * -qz;
	out[2] = iz * qw + iw * -qz + ix * -qy - iy * -qx;
	return out;
}

// spherical coordinates
static Spherical& spherical_set_from_vec3(Spherical& out, const vec3& v) {
	out.radius = vec3_length(v);
	if (out.radius == 0.0f) {
		out.theta = 0;
		out.phi = 0;
	} else {
		out.theta = std::atan2(v[0], v[2]);
		out.phi = std::acos(std::max(-1.0f, std::min(1.0f, v[1] / out.radius)));
	}
	return out;
}

static vec3& vec3_set_from_spherical(vec3& out, const Spherical& s) {
	const float sin_phi = std::sin(s.phi);
	out[0] = s.radius * sin_phi * std::sin(s.theta);
	out[1] = s.radius * std::cos(s.phi);
	out[2] = s.radius * sin_phi * std::cos(s.theta);
	return out;
}

static Spherical& spherical_make_safe(Spherical& s) {
	s.phi = std::max(EPS, std::min(static_cast<float>(std::numbers::pi) - EPS, s.phi));
	return s;
}

CameraControlsGL::CameraControlsGL(CameraGL& camera, DomElementGL& dom_element)
	: camera(camera), dom_element(dom_element) {
	target = vec3_create();

	state = STATE_NONE;
	scale = 1.0f;

	pan_offset = vec3_create();

	transform_start = vec3_create();
	transform_end = vec3_create();
	transform_delta = vec3_create();

	spherical = {1.0f, 0.0f, 0.0f};
	spherical_delta = {0.0f, 0.0f, 0.0f};

	min_distance = 0.0f;
	max_distance = std::numeric_limits<float>::infinity();

	offset = vec3_create();

	// quaternion to rotate to y-up space
	const vec3& cam_up = camera.up;
	const vec3 y_up = {0, 1, 0};
	quat = quat_create();
	quat_set_from_unit_vectors(quat, cam_up, y_up);
	quat_inverse = quat_create();
	quat_invert(quat_inverse, quat);

	last_position = vec3_create();
	last_quaternion = quat_create();

	// cached vectors for calculations
	_cache_cam_dir = vec3_create();
	_cache_cam_right = vec3_create();
	_cache_cam_up = vec3_create();

	init();
}

void CameraControlsGL::init() {
	if (dom_element.tabIndex == -1)
		dom_element.tabIndex = 0;

	update();
}

void CameraControlsGL::dispose() {
	state = STATE_NONE;
}

bool CameraControlsGL::on_mouse_down(int button, int clientX, int clientY,
                                      bool ctrlKey, bool metaKey, bool shiftKey) {
	if (dom_element.focus)
		dom_element.focus();
	else if (dom_element.window_focus)
		dom_element.window_focus();

	if (button == MOUSE_BUTTON_LEFT || button == MOUSE_BUTTON_MIDDLE) {
		if (ctrlKey || metaKey || shiftKey) {
			vec3_set(transform_start, static_cast<float>(clientX), static_cast<float>(clientY), 0);
			state = STATE_PANNING;
		} else {
			vec3_set(transform_start, static_cast<float>(clientX), static_cast<float>(clientY), 0);
			state = STATE_ROTATING;
		}
	} else if (button == MOUSE_BUTTON_RIGHT) {
		vec3_set(transform_start, static_cast<float>(clientX), static_cast<float>(clientY), 0);
		state = STATE_PANNING;
	}

	return true;
}

bool CameraControlsGL::on_mouse_wheel(float deltaY) {
	if (state != STATE_NONE && state != STATE_ROTATING)
		return false;

	if (deltaY < 0)
		dolly_out(ZOOM_SCALE);
	else if (deltaY > 0)
		dolly_in(ZOOM_SCALE);

	update();
	return true;
}

bool CameraControlsGL::on_mouse_move(int clientX, int clientY) {
	if (state == STATE_NONE)
		return false;

	if (state == STATE_ROTATING) {
		vec3_set(transform_end, static_cast<float>(clientX), static_cast<float>(clientY), 0);
		vec3_sub(transform_delta, transform_end, transform_start);
		vec3_scale(transform_delta, transform_delta, ROTATE_SPEED);

		const float height = static_cast<float>(dom_element.clientHeight);
		rotate_left(2.0f * static_cast<float>(std::numbers::pi) * transform_delta[0] / height);
		rotate_up(2.0f * static_cast<float>(std::numbers::pi) * transform_delta[1] / height);

		vec3_copy(transform_start, transform_end);
		update();
	} else if (state == STATE_PANNING) {
		vec3_set(transform_end, static_cast<float>(clientX), static_cast<float>(clientY), 0);

		const float pan_scale_val = get_pan_scale() * PAN_SPEED;
		vec3_sub(transform_delta, transform_end, transform_start);
		vec3_scale(transform_delta, transform_delta, pan_scale_val);
		pan(transform_delta[0], 0, transform_delta[1]);

		vec3_copy(transform_start, transform_end);
		update();
	} else if (state == STATE_DOLLYING) {
		vec3_set(transform_end, static_cast<float>(clientX), static_cast<float>(clientY), 0);
		vec3_sub(transform_delta, transform_end, transform_start);

		if (transform_delta[1] > 0)
			dolly_in(ZOOM_SCALE);
		else if (transform_delta[1] < 0)
			dolly_out(ZOOM_SCALE);

		vec3_copy(transform_start, transform_end);
		update();
	}

	return true;
}

float CameraControlsGL::get_pan_scale() {
	const float height = static_cast<float>(dom_element.clientHeight > 0 ? dom_element.clientHeight : 1);
	const float distance = spherical.radius;
	const float fov = camera.fov != 0.0f ? camera.fov : 50.0f;
	const float v_fov = fov * static_cast<float>(std::numbers::pi) / 180.0f;
	return (2.0f * std::tan(v_fov / 2.0f) * distance) / height;
}

void CameraControlsGL::on_mouse_up() {
	state = STATE_NONE;
}

bool CameraControlsGL::on_key_down(int keyCode, bool shiftKey, bool altKey) {
	const float key_speed = shiftKey ? KEY_PAN_SPEED_SHIFT : (altKey ? KEY_PAN_SPEED_ALT : KEY_PAN_SPEED);

	if (keyCode == KEY_S)
		pan(0, key_speed, 0);
	else if (keyCode == KEY_W)
		pan(0, -key_speed, 0);
	else if (keyCode == KEY_A)
		pan(key_speed, 0, 0);
	else if (keyCode == KEY_D)
		pan(-key_speed, 0, 0);
	else if (keyCode == KEY_Q)
		pan(0, 0, key_speed);
	else if (keyCode == KEY_E)
		pan(0, 0, -key_speed);
	else
		return false;

	update();
	return true;
}

void CameraControlsGL::dolly_out(float scale_val) {
	scale *= scale_val;
}

void CameraControlsGL::dolly_in(float scale_val) {
	scale /= scale_val;
}

void CameraControlsGL::rotate_left(float angle) {
	spherical_delta.theta -= angle;
}

void CameraControlsGL::rotate_up(float angle) {
	spherical_delta.phi -= angle;
}

void CameraControlsGL::pan(float x, float y, float z) {
	// get camera direction
	vec3& cam_dir = _cache_cam_dir;
	vec3_sub(cam_dir, target, camera.position);
	vec3_normalize(cam_dir, cam_dir);

	// get right vector
	vec3& cam_right = _cache_cam_right;
	const vec3& cam_up_vec = camera.up;
	vec3_cross(cam_right, cam_dir, cam_up_vec);
	vec3_normalize(cam_right, cam_right);

	// get true up vector
	vec3& true_up = _cache_cam_up;
	vec3_cross(true_up, cam_right, cam_dir);
	vec3_normalize(true_up, true_up);

	// calculate pan offset
	vec3 pan_right = vec3_create();
	vec3_scale(pan_right, cam_right, -x);

	vec3 pan_up_vec = vec3_create();
	vec3_scale(pan_up_vec, true_up, z);

	vec3 pan_forward = vec3_create();
	vec3_scale(pan_forward, cam_dir, -y);

	vec3_add(pan_offset, pan_offset, pan_right);
	vec3_add(pan_offset, pan_offset, pan_up_vec);
	vec3_add(pan_offset, pan_offset, pan_forward);
}

bool CameraControlsGL::update() {
	vec3_sub(offset, camera.position, target);

	// rotate offset to y-axis-is-up space
	vec3_apply_quat(offset, offset, quat);

	// angle from z-axis around y-axis
	spherical_set_from_vec3(spherical, offset);

	spherical.theta += spherical_delta.theta;
	spherical.phi += spherical_delta.phi;

	// restrict phi to be between desired limits
	spherical.phi = std::max(MIN_POLAR_ANGLE, std::min(MAX_POLAR_ANGLE, spherical.phi));
	spherical_make_safe(spherical);

	spherical.radius *= scale;
	spherical.radius = std::max(min_distance, std::min(max_distance, spherical.radius));

	// move target to panned location
	vec3_add(target, target, pan_offset);

	vec3_set_from_spherical(offset, spherical);

	// rotate offset back to camera-up-vector-is-up space
	vec3_apply_quat(offset, offset, quat_inverse);

	vec3_add(camera.position, target, offset);
	camera.lookAt(target[0], target[1], target[2]);

	spherical_delta.theta = 0;
	spherical_delta.phi = 0;
	vec3_set(pan_offset, 0, 0, 0);

	scale = 1.0f;

	if (vec3_distance_squared(last_position, camera.position) > EPS ||
		8.0f * (1.0f - quat_dot(last_quaternion, camera.quaternion)) > EPS) {
		vec3_copy(last_position, camera.position);
		last_quaternion = camera.quaternion;

		return true;
	}

	return false;
}
