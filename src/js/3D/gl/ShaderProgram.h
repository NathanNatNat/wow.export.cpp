/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <glad/gl.h>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace gl {

class GLContext;

/**
 * GLSL shader program wrapper with compilation, uniform caching,
 * and hot-reload support.
 *
 * JS equivalent: class ShaderProgram — module.exports = ShaderProgram
 */
class ShaderProgram {
public:
	ShaderProgram(GLContext& ctx, const std::string& vert_source,
	              const std::string& frag_source);
	~ShaderProgram();

	bool is_valid() const;
	void use();

	GLint get_uniform_location(const std::string& name);
	GLuint get_uniform_block_index(const std::string& name);
	/**
	 * Query a parameter for an active uniform block.
	 *
	 * Adaptation from JS: WebGL2's gl.getActiveUniformBlockParameter returns
	 * mixed types depending on @p pname — a `GLboolean`/`GLint` scalar for most
	 * pnames, but a `Uint32Array` for `GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES`.
	 * This C++ wrapper only returns a single GLint, which is sufficient for the
	 * scalar pnames (e.g. `GL_UNIFORM_BLOCK_DATA_SIZE`,
	 * `GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS`,
	 * `GL_UNIFORM_BLOCK_REFERENCED_BY_VERTEX_SHADER`, etc.).
	 *
	 * For `GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES` (which yields an array of
	 * uniform indices), callers must bypass this wrapper and call
	 * `glGetActiveUniformBlockiv` directly with a sufficiently sized
	 * `GLint[]` buffer (size obtained via
	 * `GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS`).
	 *
	 * JS source: src/js/3D/gl/ShaderProgram.js lines 122–127.
	 */
	GLint get_uniform_block_param(const std::string& name, GLenum pname);
	std::vector<GLint> get_active_uniform_offsets(const std::vector<std::string>& names);
	void bind_uniform_block(const std::string& name, GLuint binding_point);

	void set_uniform_1i(const std::string& name, int value);
	void set_uniform_1f(const std::string& name, float value);
	void set_uniform_2f(const std::string& name, float x, float y);
	void set_uniform_3f(const std::string& name, float x, float y, float z);
	void set_uniform_4f(const std::string& name, float x, float y,
	                    float z, float w);
	/**
	 * Upload one or more vec3 uniforms.
	 *
	 * Adaptation from JS: WebGL2's `gl.uniform3fv(loc, value)` infers the
	 * element count from the typed array's length. The desktop-OpenGL
	 * `glUniform3fv(loc, count, value)` requires the count to be supplied
	 * explicitly. Callers must pass @p count = number of vec3 elements in
	 * @p value (defaults to 1 for the common single-vector case). Passing
	 * an array of multiple vectors without updating @p count will silently
	 * upload only the first vector.
	 *
	 * JS source: src/js/3D/gl/ShaderProgram.js lines 204–208.
	 */
	void set_uniform_3fv(const std::string& name, const float* value,
	                     GLsizei count = 1);
	/**
	 * Upload one or more vec4 uniforms.
	 *
	 * Adaptation from JS: WebGL2's `gl.uniform4fv(loc, value)` infers the
	 * element count from the typed array's length. The desktop-OpenGL
	 * `glUniform4fv(loc, count, value)` requires the count to be supplied
	 * explicitly. Callers must pass @p count = number of vec4 elements in
	 * @p value (defaults to 1 for the common single-vector case). Passing
	 * an array of multiple vectors without updating @p count will silently
	 * upload only the first vector.
	 *
	 * JS source: src/js/3D/gl/ShaderProgram.js lines 213–218.
	 */
	void set_uniform_4fv(const std::string& name, const float* value,
	                     GLsizei count = 1);
	void set_uniform_mat3(const std::string& name, bool transpose,
	                      const float* value);
	void set_uniform_mat4(const std::string& name, bool transpose,
	                      const float* value);
	/**
	 * Upload one or more mat4 uniforms as a contiguous array.
	 *
	 * Adaptation from JS: WebGL2's `gl.uniformMatrix4fv(loc, transpose, value)`
	 * infers the matrix count from the typed array's length (length / 16).
	 * The desktop-OpenGL `glUniformMatrix4fv(loc, count, transpose, value)`
	 * requires @p count to be supplied explicitly. Callers must pass
	 * @p count = number of mat4 elements in @p value (defaults to 1 for the
	 * single-matrix case). Passing an array of multiple matrices without
	 * updating @p count will silently upload only the first matrix.
	 *
	 * JS source: src/js/3D/gl/ShaderProgram.js lines 247–251.
	 */
	void set_uniform_mat4_array(const std::string& name, bool transpose,
	                            const float* value, GLsizei count = 1);

	bool recompile(const std::string& vert_source,
	               const std::string& frag_source);

	void dispose();

	// Public state
	GLuint program = 0;
	std::unordered_map<std::string, GLint> uniform_locations;
	std::unordered_map<std::string, GLuint> uniform_block_indices;

	// Set by Shaders module to enable unregistration on dispose.
	std::string _shader_name;

	// Callback for shader unregistration — set by the Shaders module once
	// it is converted.  Replaces the lazy require('../Shaders') in JS dispose().
	static inline std::function<void(ShaderProgram*)> _unregister_fn;

private:
	void _compile(const std::string& vert_source,
	              const std::string& frag_source);
	GLuint _compile_shader(GLenum type, const std::string& source);

	GLContext& ctx_;
};

} // namespace gl
