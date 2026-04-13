/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
*/

#include "Shaders.h"
#include "../constants.h"
#include "../log.h"
#include "gl/ShaderProgram.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace shaders {

const std::unordered_map<std::string, ShaderManifestEntry> SHADER_MANIFEST = {
	{"m2",       {"m2.vertex.shader",   "m2.fragment.shader"}},
	{"wmo",      {"wmo.vertex.shader",  "wmo.fragment.shader"}},
	{"adt",      {"adt.vertex.shader",  "adt.fragment.shader"}},
	{"adt_old",  {"adt.vertex.shader",  "adt.fragment.old.shader"}},
	{"char",     {"char.vertex.shader", "char.fragment.shader"}}
};

// cached shader source text
static std::unordered_map<std::string, ShaderSource> source_cache;

// active shader programs grouped by shader name
// Map<name, Set<ShaderProgram*>>
static std::unordered_map<std::string, std::unordered_set<gl::ShaderProgram*>> active_programs;

/**
 * Load shader source from disk (cached)
 * @param name
 * @returns ShaderSource with vert and frag source text
 */
const ShaderSource& get_source(const std::string& name) {
	auto it = source_cache.find(name);
	if (it != source_cache.end())
		return it->second;

	auto manifest_it = SHADER_MANIFEST.find(name);
	if (manifest_it == SHADER_MANIFEST.end())
		throw std::runtime_error("Unknown shader: " + name);

	const auto& manifest = manifest_it->second;
	const auto shader_path = constants::SHADER_PATH();

	auto read_file = [](const std::filesystem::path& path) -> std::string {
		std::ifstream file(path, std::ios::in);
		if (!file.is_open())
			throw std::runtime_error("Failed to open shader file: " + path.string());
		std::ostringstream ss;
		ss << file.rdbuf();
		return ss.str();
	};

	ShaderSource sources;
	sources.vert = read_file(shader_path / manifest.vert);
	sources.frag = read_file(shader_path / manifest.frag);

	auto [insert_it, inserted] = source_cache.emplace(name, std::move(sources));
	return insert_it->second;
}

/**
 * Create and register a shader program
 * @param ctx
 * @param name
 * @returns Pointer to a ShaderProgram
 */
std::unique_ptr<gl::ShaderProgram> create_program(gl::GLContext& ctx, const std::string& name) {
	// Wire up the unregister callback on first use
	if (!gl::ShaderProgram::_unregister_fn) {
		gl::ShaderProgram::_unregister_fn = [](gl::ShaderProgram* p) {
			shaders::unregister(p);
		};
	}

	const auto& sources = get_source(name);
	auto program = std::make_unique<gl::ShaderProgram>(ctx, sources.vert, sources.frag);

	if (!program->is_valid())
		throw std::runtime_error("Failed to compile shader: " + name);

	// track for hot-reload
	program->_shader_name = name;

	active_programs[name].insert(program.get());

	return program;
}

/**
 * Unregister a shader program (call on dispose)
 * @param program
 */
void unregister(gl::ShaderProgram* program) {
	if (!program)
		return;

	const auto& name = program->_shader_name;
	if (name.empty())
		return;

	auto it = active_programs.find(name);
	if (it != active_programs.end())
		it->second.erase(program);
}

/**
 * Reload all shaders from disk
 */
void reload_all() {
	logging::write("Reloading all shaders...");

	// clear source cache to force re-read from disk
	source_cache.clear();

	size_t success_count = 0;
	size_t fail_count = 0;

	for (auto& [name, programs] : active_programs) {
		if (programs.empty())
			continue;

		try {
			const auto& sources = get_source(name);

			for (auto* program : programs) {
				if (program->recompile(sources.vert, sources.frag)) {
					success_count++;
				} else {
					fail_count++;
					logging::write("Failed to recompile shader program: " + name);
				}
			}
		} catch (const std::exception& e) {
			fail_count += programs.size();
			logging::write("Failed to reload shader source: " + name + " - " + e.what());
		}
	}

	logging::write("Shader reload complete: " + std::to_string(success_count) +
	               " succeeded, " + std::to_string(fail_count) + " failed");
}

/**
 * Get count of active programs for a shader
 * @param name
 * @returns number of active programs
 */
size_t get_program_count(const std::string& name) {
	auto it = active_programs.find(name);
	return it != active_programs.end() ? it->second.size() : 0;
}

/**
 * Get total count of all active programs
 * @returns total number of active programs
 */
size_t get_total_program_count() {
	size_t count = 0;
	for (const auto& [name, programs] : active_programs)
		count += programs.size();

	return count;
}

} // namespace shaders
