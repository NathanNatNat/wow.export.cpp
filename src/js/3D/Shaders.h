/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
*/
#pragma once

#include <string>
#include <memory>
#include <unordered_map>

namespace gl {
class GLContext;
class ShaderProgram;
}

namespace shaders {

struct ShaderManifestEntry {
	std::string vert;
	std::string frag;
};

/**
 * Shader manifest — maps shader names to vertex/fragment file names.
 */
extern const std::unordered_map<std::string, ShaderManifestEntry> SHADER_MANIFEST;

struct ShaderSource {
	std::string vert;
	std::string frag;
};

/**
 * Load shader source from disk (cached)
 * @param name
 * @returns ShaderSource with vert and frag source text
 */
const ShaderSource& get_source(const std::string& name);

/**
 * Create and register a shader program
 * @param ctx
 * @param name
 * @returns Pointer to a ShaderProgram (owned by the module)
 */
std::unique_ptr<gl::ShaderProgram> create_program(gl::GLContext& ctx, const std::string& name);

/**
 * Unregister a shader program (call on dispose)
 * @param program
 */
void unregister(gl::ShaderProgram* program);

/**
 * Reload all shaders from disk
 */
void reload_all();

/**
 * Get count of active programs for a shader
 * @param name
 * @returns number of active programs
 */
size_t get_program_count(const std::string& name);

/**
 * Get total count of all active programs
 * @returns total number of active programs
 */
size_t get_total_program_count();

} // namespace shaders
