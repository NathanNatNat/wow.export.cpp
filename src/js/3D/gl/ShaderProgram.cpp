/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "ShaderProgram.h"
#include "GLContext.h"
#include "log.h"

#include <format>

namespace gl {

ShaderProgram::ShaderProgram(GLContext& ctx, const std::string& vert_source,
                              const std::string& frag_source)
	: ctx_(ctx) {
	_compile(vert_source, frag_source);
}

ShaderProgram::~ShaderProgram() {
	dispose();
}

void ShaderProgram::_compile(const std::string& vert_source,
                              const std::string& frag_source) {
	GLuint vert_shader = _compile_shader(GL_VERTEX_SHADER, vert_source);
	GLuint frag_shader = _compile_shader(GL_FRAGMENT_SHADER, frag_source);

	if (!vert_shader || !frag_shader) {
		if (vert_shader)
			glDeleteShader(vert_shader);
		if (frag_shader)
			glDeleteShader(frag_shader);
		return;
	}

	GLuint prog = glCreateProgram();
	glAttachShader(prog, vert_shader);
	glAttachShader(prog, frag_shader);
	glLinkProgram(prog);

	// shaders can be deleted after linking
	glDeleteShader(vert_shader);
	glDeleteShader(frag_shader);

	GLint link_status = GL_FALSE;
	glGetProgramiv(prog, GL_LINK_STATUS, &link_status);
	if (link_status != GL_TRUE) {
		GLint log_len = 0;
		glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &log_len);
		std::string info(static_cast<size_t>(log_len), '\0');
		glGetProgramInfoLog(prog, log_len, nullptr, info.data());
		logging::write(std::format("Shader program link error: {}", info));
		glDeleteProgram(prog);
		return;
	}

	program = prog;
}

GLuint ShaderProgram::_compile_shader(GLenum type, const std::string& source) {
	GLuint shader = glCreateShader(type);
	const char* src = source.c_str();
	glShaderSource(shader, 1, &src, nullptr);
	glCompileShader(shader);

	GLint compile_status = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);
	if (compile_status != GL_TRUE) {
		GLint log_len = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);
		std::string info(static_cast<size_t>(log_len), '\0');
		glGetShaderInfoLog(shader, log_len, nullptr, info.data());
		const char* type_name = (type == GL_VERTEX_SHADER) ? "vertex" : "fragment";
		logging::write(std::format("Shader compile error ({}): {}", type_name, info));
		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

bool ShaderProgram::is_valid() const {
	return program != 0;
}

void ShaderProgram::use() {
	ctx_.use_program(program);
}

GLint ShaderProgram::get_uniform_location(const std::string& name) {
	auto it = uniform_locations.find(name);
	if (it != uniform_locations.end())
		return it->second;

	GLint location = glGetUniformLocation(program, name.c_str());
	uniform_locations[name] = location;
	return location;
}

GLuint ShaderProgram::get_uniform_block_index(const std::string& name) {
	auto it = uniform_block_indices.find(name);
	if (it != uniform_block_indices.end())
		return it->second;

	GLuint index = glGetUniformBlockIndex(program, name.c_str());
	uniform_block_indices[name] = index;
	return index;
}

void ShaderProgram::bind_uniform_block(const std::string& name,
                                        GLuint binding_point) {
	GLuint index = get_uniform_block_index(name);
	if (index != GL_INVALID_INDEX)
		glUniformBlockBinding(program, index, binding_point);
}

void ShaderProgram::set_uniform_1i(const std::string& name, int value) {
	GLint loc = get_uniform_location(name);
	if (loc != -1)
		glUniform1i(loc, value);
}

void ShaderProgram::set_uniform_1f(const std::string& name, float value) {
	GLint loc = get_uniform_location(name);
	if (loc != -1)
		glUniform1f(loc, value);
}

void ShaderProgram::set_uniform_2f(const std::string& name, float x, float y) {
	GLint loc = get_uniform_location(name);
	if (loc != -1)
		glUniform2f(loc, x, y);
}

void ShaderProgram::set_uniform_3f(const std::string& name, float x, float y,
                                    float z) {
	GLint loc = get_uniform_location(name);
	if (loc != -1)
		glUniform3f(loc, x, y, z);
}

void ShaderProgram::set_uniform_4f(const std::string& name, float x, float y,
                                    float z, float w) {
	GLint loc = get_uniform_location(name);
	if (loc != -1)
		glUniform4f(loc, x, y, z, w);
}

void ShaderProgram::set_uniform_3fv(const std::string& name,
                                     const float* value, GLsizei count) {
	GLint loc = get_uniform_location(name);
	if (loc != -1)
		glUniform3fv(loc, count, value);
}

void ShaderProgram::set_uniform_4fv(const std::string& name,
                                     const float* value, GLsizei count) {
	GLint loc = get_uniform_location(name);
	if (loc != -1)
		glUniform4fv(loc, count, value);
}

void ShaderProgram::set_uniform_mat3(const std::string& name, bool transpose,
                                      const float* value) {
	GLint loc = get_uniform_location(name);
	if (loc != -1)
		glUniformMatrix3fv(loc, 1, transpose ? GL_TRUE : GL_FALSE, value);
}

void ShaderProgram::set_uniform_mat4(const std::string& name, bool transpose,
                                      const float* value) {
	GLint loc = get_uniform_location(name);
	if (loc != -1)
		glUniformMatrix4fv(loc, 1, transpose ? GL_TRUE : GL_FALSE, value);
}

void ShaderProgram::set_uniform_mat4_array(const std::string& name,
                                            bool transpose,
                                            const float* value,
                                            GLsizei count) {
	GLint loc = get_uniform_location(name);
	if (loc != -1)
		glUniformMatrix4fv(loc, count, transpose ? GL_TRUE : GL_FALSE, value);
}

bool ShaderProgram::recompile(const std::string& vert_source,
                               const std::string& frag_source) {
	GLuint vert_shader = _compile_shader(GL_VERTEX_SHADER, vert_source);
	GLuint frag_shader = _compile_shader(GL_FRAGMENT_SHADER, frag_source);

	if (!vert_shader || !frag_shader) {
		if (vert_shader)
			glDeleteShader(vert_shader);

		if (frag_shader)
			glDeleteShader(frag_shader);

		return false;
	}

	GLuint new_program = glCreateProgram();
	glAttachShader(new_program, vert_shader);
	glAttachShader(new_program, frag_shader);
	glLinkProgram(new_program);

	glDeleteShader(vert_shader);
	glDeleteShader(frag_shader);

	GLint link_status = GL_FALSE;
	glGetProgramiv(new_program, GL_LINK_STATUS, &link_status);
	if (link_status != GL_TRUE) {
		GLint log_len = 0;
		glGetProgramiv(new_program, GL_INFO_LOG_LENGTH, &log_len);
		std::string info(static_cast<size_t>(log_len), '\0');
		glGetProgramInfoLog(new_program, log_len, nullptr, info.data());
		logging::write(std::format("Shader program link error on recompile: {}", info));
		glDeleteProgram(new_program);
		return false;
	}

	// delete old program and swap in new one
	if (program)
		glDeleteProgram(program);

	program = new_program;

	// clear uniform caches since locations change
	uniform_locations.clear();
	uniform_block_indices.clear();

	return true;
}

void ShaderProgram::dispose() {
	// unregister from Shaders module if tracked
	if (!_shader_name.empty() && _unregister_fn)
		_unregister_fn(this);

	if (program) {
		glDeleteProgram(program);
		program = 0;
	}

	uniform_locations.clear();
	uniform_block_indices.clear();
}

} // namespace gl
