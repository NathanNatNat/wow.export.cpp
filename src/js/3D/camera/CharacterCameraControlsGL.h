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
 * Camera interface expected by CharacterCameraControlsGL.
 */
struct CharacterCameraGL {
	std::array<float, 3> position = {0, 0, 0};

	std::function<void(float, float, float)> lookAt;
	std::function<void()> update_view;
};

struct CharacterDomElementGL {
	int clientWidth = 800;
	int clientHeight = 600;
};

class CharacterCameraControlsGL {
public:
	CharacterCameraControlsGL(CharacterCameraGL& camera, CharacterDomElementGL& dom_element);

	void on_mouse_down(int button, int clientX, int clientY);
	void on_mouse_move(int clientX, int clientY);
	void on_mouse_up(int button);
	void on_mouse_wheel(float deltaY);
	void update();
	void dispose();

	CharacterCameraGL& camera;
	CharacterDomElementGL& dom_element;

	std::array<float, 3> target;
	float model_rotation_y;
	std::function<void(float)> on_model_rotate;

	bool is_rotating;
	bool is_panning;
	int prev_mouse_x;
	int prev_mouse_y;
};
