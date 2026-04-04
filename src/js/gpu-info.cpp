/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "gpu-info.h"
#include "log.h"
#include "generics.h"

#include <string>
#include <string_view>
#include <vector>
#include <format>
#include <cstdio>
#include <cstdlib>
#include <array>
#include <optional>
#include <regex>
#include <stdexcept>

#include <glad/gl.h>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <cstring>
#endif

namespace {

/**
 * GPU capabilities structure.
 */
struct GLCaps {
	int max_tex_size = 0;
	int max_cube_size = 0;
	int max_varyings = 0;
	int max_vert_uniforms = 0;
	int max_frag_uniforms = 0;
	int max_vert_attribs = 0;
	int max_tex_units = 0;
	int max_vert_tex_units = 0;
	int max_combined_tex_units = 0;
	int max_renderbuffer = 0;
	std::array<int, 2> max_viewport = {0, 0};
};

/**
 * OpenGL info result structure.
 */
struct GLInfo {
	std::string vendor;
	std::string renderer;
	GLCaps caps;
	std::vector<std::string> extensions;
	std::string error;
};

/**
 * Platform GPU info structure.
 */
struct PlatformGPUInfo {
	std::string name = "Unknown";
	std::string vram = "Unknown";
	std::string driver = "Unknown";
};

/**
 * Execute a shell command and return stdout.
 * @param cmd Command to execute.
 * @returns Trimmed stdout output.
 */
std::string exec_cmd(const std::string& cmd) {
#ifdef _WIN32
	FILE* pipe = _popen(cmd.c_str(), "r");
#else
	FILE* pipe = popen(cmd.c_str(), "r");
#endif
	if (!pipe)
		throw std::runtime_error("Failed to execute command");

	std::string result;
	std::array<char, 256> buffer;

	while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr)
		result += buffer.data();

#ifdef _WIN32
	int status = _pclose(pipe);
#else
	int status = pclose(pipe);
#endif

	if (status != 0)
		throw std::runtime_error("Command failed with exit code " + std::to_string(status));

	// trim whitespace
	auto start = result.find_first_not_of(" \t\n\r");
	auto end = result.find_last_not_of(" \t\n\r");
	if (start == std::string::npos)
		return "";
	return result.substr(start, end - start + 1);
}

/**
 * Parse VRAM bytes to human-readable format.
 * @param bytes Number of bytes.
 * @returns Formatted string or "Unknown".
 */
std::string format_vram(int64_t bytes) {
	if (bytes <= 0)
		return "Unknown";

	return generics::filesize(static_cast<double>(bytes));
}

/**
 * Get GPU renderer/vendor info and capabilities via OpenGL.
 * @returns GLInfo struct with GPU information.
 */
GLInfo get_gl_info() {
	GLInfo result;

	try {
		// vendor/renderer
		const char* vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
		const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));

		if (vendor)
			result.vendor = vendor;
		if (renderer)
			result.renderer = renderer;

		// capability limits
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &result.caps.max_tex_size);
		glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &result.caps.max_cube_size);
		glGetIntegerv(GL_MAX_VARYING_VECTORS, &result.caps.max_varyings);
		glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &result.caps.max_vert_uniforms);
		glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &result.caps.max_frag_uniforms);
		glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &result.caps.max_vert_attribs);
		glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &result.caps.max_tex_units);
		glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &result.caps.max_vert_tex_units);
		glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &result.caps.max_combined_tex_units);
		glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &result.caps.max_renderbuffer);
		glGetIntegerv(GL_MAX_VIEWPORT_DIMS, result.caps.max_viewport.data());

		// extensions
		int num_extensions = 0;
		glGetIntegerv(GL_NUM_EXTENSIONS, &num_extensions);
		result.extensions.reserve(static_cast<size_t>(num_extensions));

		for (int i = 0; i < num_extensions; ++i) {
			const char* ext = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, static_cast<GLuint>(i)));
			if (ext)
				result.extensions.emplace_back(ext);
		}
	} catch (const std::exception& e) {
		result.error = e.what();
	}

	return result;
}

#ifdef _WIN32
/**
 * Get accurate VRAM from Windows registry.
 * @returns VRAM in bytes, or -1 on failure.
 */
int64_t get_windows_registry_vram() {
	try {
		// query registry for qwMemorySize (64-bit accurate VRAM value)
		std::string output = exec_cmd(
			"powershell -Command \"Get-ItemProperty -Path "
			"'HKLM:\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0*' "
			"-Name 'HardwareInformation.qwMemorySize' -ErrorAction SilentlyContinue | "
			"Select-Object -ExpandProperty 'HardwareInformation.qwMemorySize' | "
			"Select-Object -First 1\""
		);

		// trim and parse
		auto start = output.find_first_not_of(" \t\n\r");
		auto end = output.find_last_not_of(" \t\n\r");
		if (start != std::string::npos) {
			std::string trimmed = output.substr(start, end - start + 1);
			int64_t vram = std::stoll(trimmed);
			if (vram > 0)
				return vram;
		}
	} catch (...) {
		// registry query failed, fall back to WMI
	}

	return -1;
}

/**
 * Get GPU info on Windows via WMIC/PowerShell.
 * @returns PlatformGPUInfo or std::nullopt on failure.
 */
std::optional<PlatformGPUInfo> get_windows_gpu_info() {
	try {
		std::string output = exec_cmd("wmic path win32_VideoController get Name,AdapterRAM,DriverVersion /format:csv");

		// split by both \r\n and \n, filter empty lines
		std::vector<std::string> lines;
		std::string line;
		for (char c : output) {
			if (c == '\n') {
				// trim \r
				if (!line.empty() && line.back() == '\r')
					line.pop_back();
				// trim whitespace
				auto s = line.find_first_not_of(" \t");
				if (s != std::string::npos)
					lines.push_back(line.substr(s));
				line.clear();
			} else {
				line += c;
			}
		}
		if (!line.empty()) {
			auto s = line.find_first_not_of(" \t");
			if (s != std::string::npos)
				lines.push_back(line.substr(s));
		}

		if (lines.size() < 2) {
			logging::write(std::format("GPU: WMIC returned insufficient data ({} lines)", lines.size()));
			return std::nullopt;
		}

		// csv format: Node,AdapterRAM,DriverVersion,Name
		const std::string& data_line = lines[1];
		std::vector<std::string> parts;
		std::string part;
		for (char c : data_line) {
			if (c == ',') {
				parts.push_back(part);
				part.clear();
			} else {
				part += c;
			}
		}
		parts.push_back(part);

		if (parts.size() < 4) {
			logging::write(std::format("GPU: WMIC CSV parse failed, expected 4 fields, got {}", parts.size()));
			return std::nullopt;
		}

		int64_t adapter_ram = 0;
		try {
			adapter_ram = std::stoll(parts[1]);
		} catch (...) {}

		const std::string& driver_version = parts[2];
		const std::string& name = parts[3];

		// wmi AdapterRAM is 32-bit, try registry for accurate value on >4GB GPUs
		int64_t registry_vram = get_windows_registry_vram();
		if (registry_vram > 0)
			adapter_ram = registry_vram;

		PlatformGPUInfo info;
		info.name = name.empty() ? "Unknown" : name;
		info.vram = format_vram(adapter_ram);
		info.driver = driver_version.empty() ? "Unknown" : driver_version;
		return info;
	} catch (const std::exception& e) {
		logging::write(std::format("GPU: Windows WMIC query failed: {}", e.what()));
		return std::nullopt;
	}
}
#endif // _WIN32

#ifdef __linux__
/**
 * Get GPU info on Linux.
 * @returns PlatformGPUInfo with available information.
 */
std::optional<PlatformGPUInfo> get_linux_gpu_info() {
	PlatformGPUInfo result;

	// gpu name via lspci
	try {
		std::string lspci_output = exec_cmd("lspci | grep -i vga");
		std::regex re(": (.+)$", std::regex::multiline);
		std::smatch match;
		if (std::regex_search(lspci_output, match, re))
			result.name = match[1].str();
	} catch (const std::exception& e) {
		logging::write(std::format("GPU: Linux lspci query failed: {}", e.what()));
	}

	// nvidia vram + driver
	try {
		std::string nvidia_mem = exec_cmd("nvidia-smi --query-gpu=memory.total --format=csv,noheader,nounits");
		int64_t mem_mb = std::stoll(nvidia_mem);
		if (mem_mb > 0)
			result.vram = format_vram(mem_mb * 1024 * 1024);

		std::string nvidia_driver = exec_cmd("nvidia-smi --query-gpu=driver_version --format=csv,noheader");
		if (!nvidia_driver.empty())
			result.driver = nvidia_driver;
	} catch (...) {
		// nvidia-smi not available, try glxinfo
		try {
			std::string glx_output = exec_cmd("glxinfo 2>/dev/null | grep -E \"(OpenGL version|Video memory)\"");

			std::regex version_re("OpenGL version string: (.+)");
			std::smatch version_match;
			if (std::regex_search(glx_output, version_match, version_re))
				result.driver = version_match[1].str();

			std::regex mem_re("Video memory: (\\d+)");
			std::smatch mem_match;
			if (std::regex_search(glx_output, mem_match, mem_re)) {
				int64_t mem_mb = std::stoll(mem_match[1].str());
				result.vram = format_vram(mem_mb * 1024 * 1024);
			}
		} catch (...) {
			// glxinfo not available
		}
	}

	return result;
}
#endif // __linux__

/**
 * Get platform-specific GPU info (VRAM, driver version).
 * @returns PlatformGPUInfo or std::nullopt on failure.
 */
std::optional<PlatformGPUInfo> get_platform_gpu_info() {
#ifdef _WIN32
	return get_windows_gpu_info();
#elif defined(__linux__)
	return get_linux_gpu_info();
#else
	return std::nullopt;
#endif
}

/**
 * Format extension list into compact categories.
 * @param extensions List of GL extension strings.
 * @returns Formatted compact string.
 */
std::string format_extensions(const std::vector<std::string>& extensions) {
	std::vector<std::string> compressed;
	std::vector<std::string> float_exts;
	std::vector<std::string> depth;
	bool instanced = false;
	bool vao = false;
	bool anisotropic = false;
	bool draw_buffers = false;

	for (const auto& ext : extensions) {
		if (ext.find("compressed_texture") != std::string::npos) {
			std::string name = ext;
			// strip common GL extension prefixes
			auto pos = name.find("GL_ARB_compressed_texture_");
			if (pos != std::string::npos) { name = name.substr(pos + 26); }
			else {
				pos = name.find("GL_EXT_texture_compression_");
				if (pos != std::string::npos) { name = name.substr(pos + 27); }
				else {
					pos = name.find("compressed_texture_");
					if (pos != std::string::npos) { name = name.substr(pos + 19); }
				}
			}
			compressed.push_back(name);
		} else if (ext.find("float") != std::string::npos || ext.find("half_float") != std::string::npos) {
			std::string name = ext;
			auto pos = name.find("GL_ARB_");
			if (pos != std::string::npos) { name = name.substr(pos + 7); }
			else {
				pos = name.find("GL_EXT_");
				if (pos != std::string::npos) { name = name.substr(pos + 7); }
				else {
					pos = name.find("GL_OES_");
					if (pos != std::string::npos) { name = name.substr(pos + 7); }
				}
			}
			// abbreviate color_buffer to cb_
			auto cb_pos = name.find("color_buffer_");
			if (cb_pos != std::string::npos)
				name = "cb_" + name.substr(cb_pos + 13);
			float_exts.push_back(name);
		} else if (ext.find("depth") != std::string::npos) {
			std::string name = ext;
			auto pos = name.find("GL_ARB_");
			if (pos != std::string::npos) { name = name.substr(pos + 7); }
			else {
				pos = name.find("GL_EXT_");
				if (pos != std::string::npos) { name = name.substr(pos + 7); }
			}
			depth.push_back(name);
		} else if (ext.find("instanced") != std::string::npos) {
			instanced = true;
		} else if (ext.find("vertex_array_object") != std::string::npos) {
			vao = true;
		} else if (ext.find("anisotropic") != std::string::npos) {
			anisotropic = true;
		} else if (ext.find("draw_buffers") != std::string::npos) {
			draw_buffers = true;
		}
	}

	std::vector<std::string> parts;

	if (!compressed.empty()) {
		std::string joined;
		for (size_t i = 0; i < compressed.size(); ++i) {
			if (i > 0) joined += '/';
			joined += compressed[i];
		}
		parts.push_back("tex:" + joined);
	}

	if (!float_exts.empty()) {
		std::string joined;
		for (size_t i = 0; i < float_exts.size(); ++i) {
			if (i > 0) joined += '/';
			joined += float_exts[i];
		}
		parts.push_back("float:" + joined);
	}

	if (!depth.empty()) {
		std::string joined;
		for (size_t i = 0; i < depth.size(); ++i) {
			if (i > 0) joined += '/';
			joined += depth[i];
		}
		parts.push_back("depth:" + joined);
	}

	std::vector<std::string> flags;
	if (instanced)
		flags.emplace_back("instanced");
	if (vao)
		flags.emplace_back("vao");
	if (anisotropic)
		flags.emplace_back("aniso");
	if (draw_buffers)
		flags.emplace_back("mrt");

	if (!flags.empty()) {
		std::string joined;
		for (size_t i = 0; i < flags.size(); ++i) {
			if (i > 0) joined += ',';
			joined += flags[i];
		}
		parts.push_back(joined);
	}

	std::string result;
	for (size_t i = 0; i < parts.size(); ++i) {
		if (i > 0) result += " | ";
		result += parts[i];
	}
	return result;
}

/**
 * Format capabilities into compact string.
 * @param caps GL capabilities struct.
 * @returns Formatted compact string.
 */
std::string format_caps(const GLCaps& caps) {
	std::string viewport = std::to_string(caps.max_viewport[0]) + "x" + std::to_string(caps.max_viewport[1]);

	return std::format("tex:{} cube:{} varyings:{} uniforms:{}v/{}f attribs:{} texunits:{}/{} rb:{} vp:{}",
		caps.max_tex_size,
		caps.max_cube_size,
		caps.max_varyings,
		caps.max_vert_uniforms,
		caps.max_frag_uniforms,
		caps.max_vert_attribs,
		caps.max_tex_units,
		caps.max_combined_tex_units,
		caps.max_renderbuffer,
		viewport
	);
}

} // anonymous namespace

namespace gpu_info {

void log_gpu_info() {
	GLInfo gl = get_gl_info();
	std::optional<PlatformGPUInfo> platform_info;

	try {
		platform_info = get_platform_gpu_info();
	} catch (const std::exception& e) {
		logging::write(std::format("GPU: Platform query failed: {}", e.what()));
	}

	// log everything together
	if (!gl.error.empty()) {
		logging::write(std::format("GPU: GL query failed: {}", gl.error));
	} else {
		if (!gl.renderer.empty())
			logging::write(std::format("GPU: {} ({})", gl.renderer, gl.vendor));
		else
			logging::write("GPU: GL debug info unavailable");

		if (gl.caps.max_tex_size > 0)
			logging::write(std::format("GPU caps: {}", format_caps(gl.caps)));

		if (!gl.extensions.empty())
			logging::write(std::format("GPU ext ({}): {}", gl.extensions.size(), format_extensions(gl.extensions)));
	}

	if (platform_info.has_value())
		logging::write(std::format("GPU: VRAM {}, Driver {}", platform_info->vram, platform_info->driver));
	else
		logging::write("GPU: Platform-specific info unavailable");
}

} // namespace gpu_info
