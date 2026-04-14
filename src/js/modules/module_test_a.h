/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

/**
 * Test module A — developer test module for hot-reload and module switching.
 *
 * JS equivalent: Vue component with a counter that increments, and a button
 * to switch to module_test_b.
 */
namespace module_test_a {

/**
 * Render the test module A widget using ImGui.
 */
void render();

/**
 * Mounted callback (logs to console).
 */
void mounted();

/**
 * Unmounted callback (logs to console).
 * JS equivalent: unmounted() { console.log('module_test_a unmounted'); }
 */
void unmounted();

} // namespace module_test_a
