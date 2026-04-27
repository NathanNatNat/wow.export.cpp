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
	GLint get_uniform_block_param(const std::string& name, GLenum pname);
	std::vector<GLint> get_active_uniform_offsets(const std::vector<std::string>& names);
	void bind_uniform_block(const std::string& name, GLuint binding_point);

	void set_uniform_1i(const std::string& name, int value);
	void set_uniform_1f(const std::string& name, float value);
	void set_uniform_2f(const std::string& name, float x, float y);
	void set_uniform_3f(const std::string& name, float x, float y, float z);
	void set_uniform_4f(const std::string& name, float x, float y,
	                    float z, float w);
	// Adaptation: JS WebGL2 gl.uniform3fv/4fv/uniformMatrix4fv infer array
	// length from the typed array automatically; OpenGL glUniform*fv requires
	// an explicit element count. count defaults to 1 for the single-value case.
	void set_uniform_3fv(const std::string& name, const float* value,
	                     GLsizei count = 1);
	void set_uniform_4fv(const std::string& name, const float* value,
	                     GLsizei count = 1);
	void set_uniform_mat3(const std::string& name, bool transpose,
	                      const float* value);
	void set_uniform_mat4(const std::string& name, bool transpose,
	                      const float* value);
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
