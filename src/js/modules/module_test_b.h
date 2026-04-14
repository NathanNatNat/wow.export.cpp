/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

/**
 * Test module B — developer test module for hot-reload, module switching, and toasts.
 *
 * JS equivalent: Vue component with a message input, buttons to switch to
 * module_test_a, reload self, and show a toast.
 */
namespace module_test_b {

/**
 * Render the test module B widget using ImGui.
 */
void render();

/**
 * Mounted callback (logs to console).
 */
void mounted();

/**
 * Unmounted callback (logs to console).
 * JS equivalent: unmounted() { console.log('module_test_b unmounted'); }
 */
void unmounted();

} // namespace module_test_b
