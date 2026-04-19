/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
*/
#pragma once

#include "CameraControlsGL.h"

#include <array>
#include <functional>

class CharacterCameraControlsGL {
public:
	CharacterCameraControlsGL(CameraGL& camera, DomElementGL& dom_element);

	bool on_mouse_down(int button, int clientX, int clientY);
	bool on_mouse_move(int clientX, int clientY);
	void on_mouse_up(int button);
	bool on_mouse_wheel(float deltaY);
	void update();
	void dispose();

	CameraGL& camera;
	DomElementGL& dom_element;

	std::array<float, 3> target;
	float model_rotation_y;
	std::function<void(float)> on_model_rotate;

	bool is_rotating;
	bool is_panning;
	int prev_mouse_x;
	int prev_mouse_y;
};
