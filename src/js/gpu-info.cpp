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
#include <sstream>

#include <glad/gl.h>
#include <nlohmann/json.hpp>

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
 * JS: exec(cmd, { timeout: 5000 }, callback) — kills child after 5 seconds.
 * @param cmd Command to execute.
 * @returns Trimmed stdout output.
 */
std::string exec_cmd(const std::string& cmd) {
	// JS uses { timeout: 5000 } and terminates the child process on timeout.
	std::string result;
#ifdef _WIN32
	SECURITY_ATTRIBUTES sa{};
	sa.nLength = sizeof(sa);
	sa.bInheritHandle = TRUE;

	HANDLE readPipe = nullptr;
	HANDLE writePipe = nullptr;
	if (!CreatePipe(&readPipe, &writePipe, &sa, 0))
		throw std::runtime_error("Failed to create stdout pipe");

	SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);

	STARTUPINFOA si{};
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdOutput = writePipe;
	si.hStdError = writePipe;

	PROCESS_INFORMATION pi{};
	std::string command = "cmd.exe /C " + cmd;
	std::vector<char> commandBuffer(command.begin(), command.end());
	commandBuffer.push_back('\0');

	if (!CreateProcessA(nullptr, commandBuffer.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
		CloseHandle(readPipe);
		CloseHandle(writePipe);
		throw std::runtime_error("Failed to execute command");
	}

	CloseHandle(writePipe);

	DWORD waitStatus = WaitForSingleObject(pi.hProcess, 5000);
	if (waitStatus == WAIT_TIMEOUT) {
		TerminateProcess(pi.hProcess, 1);
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
		CloseHandle(readPipe);
		throw std::runtime_error("Command timed out after 5 seconds");
	}

	char buffer[256];
	DWORD bytesRead = 0;
	while (ReadFile(readPipe, buffer, sizeof(buffer), &bytesRead, nullptr) && bytesRead > 0)
		result.append(buffer, buffer + bytesRead);

	DWORD exitCode = 1;
	GetExitCodeProcess(pi.hProcess, &exitCode);
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	CloseHandle(readPipe);

	if (exitCode != 0)
		throw std::runtime_error("Command failed with exit code " + std::to_string(exitCode));
#else
	// Wrap command with timeout(1) to enforce 5-second limit, matching JS behavior
	std::string timed_cmd = "timeout 5 " + cmd;
	FILE* pipe = popen(timed_cmd.c_str(), "r");
	if (!pipe)
		throw std::runtime_error("Failed to execute command");

	std::array<char, 256> buffer;
	while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr)
		result += buffer.data();

	int status = pclose(pipe);
	if (status != 0)
		throw std::runtime_error("Command failed with exit code " + std::to_string(status));
#endif

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
 * Returns std::nullopt if no GL context is available (equivalent to JS returning null).
 * Returns GLInfo with error field set if an exception occurs (equivalent to JS { error: msg }).
 * Returns full GLInfo on success (equivalent to JS { vendor, renderer, caps, extensions }).
 */
std::optional<GLInfo> get_gl_info() {
	// Check if GL functions are available (context was created)
	// JS: const gl = canvas.getContext('webgl'); if (!gl) return null;
	if (!glGetString) {
		return std::nullopt;
	}

	GLInfo result;

	try {
		// capability limits — using VECTORS (not COMPONENTS) to match JS WebGL queries
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &result.caps.max_tex_size);
		glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &result.caps.max_cube_size);
		glGetIntegerv(GL_MAX_VARYING_VECTORS, &result.caps.max_varyings);
		glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &result.caps.max_vert_uniforms);
		glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS, &result.caps.max_frag_uniforms);
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

		// JS checks for WEBGL_debug_renderer_info extension to get vendor/renderer.
		// In native OpenGL, GL_VENDOR and GL_RENDERER are always available.
		const char* vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
		const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
		if (vendor)
			result.vendor = vendor;
		if (renderer)
			result.renderer = renderer;
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
 * Get GPU info on Windows via PowerShell CIM.
 * Replaces WMIC which was removed in Windows 11 24H2.
 * JS equivalent: Get-CimInstance Win32_VideoController | ConvertTo-Json
 * @returns PlatformGPUInfo or std::nullopt on failure.
 */
std::optional<PlatformGPUInfo> get_windows_gpu_info() {
	try {
		std::string output = exec_cmd(
			"powershell -NoProfile -Command "
			"\"Get-CimInstance Win32_VideoController | Select-Object -First 1 Name,AdapterRAM,DriverVersion | ConvertTo-Json\""
		);

		auto gpu = nlohmann::json::parse(output);

		int64_t adapter_ram = gpu.value("AdapterRAM", int64_t{0});

		// CIM AdapterRAM is 32-bit, try registry for accurate value on >4GB GPUs
		int64_t registry_vram = get_windows_registry_vram();
		if (registry_vram > 0)
			adapter_ram = registry_vram;

		PlatformGPUInfo info;
		info.name = gpu.value("Name", std::string("Unknown"));
		info.vram = format_vram(adapter_ram);
		info.driver = gpu.value("DriverVersion", std::string("Unknown"));
		return info;
	} catch (const std::exception& e) {
		logging::write(std::format("GPU: Windows CIM query failed: {}", e.what()));
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

#ifdef __APPLE__
/**
 * Get GPU info on macOS via system_profiler.
 * @returns PlatformGPUInfo with available information.
 */
std::optional<PlatformGPUInfo> get_macos_gpu_info() {
	try {
		std::string output = exec_cmd("system_profiler SPDisplaysDataType");
		PlatformGPUInfo result;

		std::smatch match;
		if (std::regex_search(output, match, std::regex(R"(Chipset Model: (.+))")))
			result.name = match[1].str();

		if (std::regex_search(output, match, std::regex(R"(VRAM \([^)]+\): (.+))")))
			result.vram = match[1].str();

		if (std::regex_search(output, match, std::regex(R"(Metal.*: (.+))")))
			result.driver = "Metal " + match[1].str();

		return result;
	} catch (const std::exception& e) {
		logging::write(std::format("GPU: macOS system_profiler query failed: {}", e.what()));
		return std::nullopt;
	}
}
#endif // __APPLE__

/**
 * Get platform-specific GPU info (VRAM, driver version).
 * @returns PlatformGPUInfo or std::nullopt on failure.
 */
std::optional<PlatformGPUInfo> get_platform_gpu_info() {
#ifdef _WIN32
	return get_windows_gpu_info();
#elif defined(__linux__)
	return get_linux_gpu_info();
#elif defined(__APPLE__)
	return get_macos_gpu_info();
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
			if (name.rfind("WEBGL_compressed_texture_", 0) == 0)
				name = name.substr(25);
			else if (name.rfind("EXT_texture_compression_", 0) == 0)
				name = name.substr(24);
			compressed.push_back(name);
		} else if (ext.find("float") != std::string::npos || ext.find("half_float") != std::string::npos) {
			std::string name = ext;
			if (name.rfind("OES_texture_", 0) == 0)
				name = name.substr(12);
			if (name.rfind("WEBGL_color_buffer_", 0) == 0)
				name = "cb_" + name.substr(19);
			if (name.rfind("EXT_color_buffer_", 0) == 0)
				name = "cb_" + name.substr(17);
			float_exts.push_back(name);
		} else if (ext.find("depth") != std::string::npos) {
			std::string name = ext;
			if (name.rfind("WEBGL_", 0) == 0)
				name = name.substr(7);
			else if (name.rfind("EXT_", 0) == 0)
				name = name.substr(4);
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
	std::string viewport = std::to_string(caps.max_viewport[0]) + "," + std::to_string(caps.max_viewport[1]);

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
	auto gl = get_gl_info();
	std::optional<PlatformGPUInfo> platform_info;

	try {
		platform_info = get_platform_gpu_info();
	} catch (const std::exception& e) {
		logging::write(std::format("GPU: Platform query failed: {}", e.what()));
	}

	// log everything together
	// JS: if (webgl?.error) ... else if (webgl) ... else ... "GPU: WebGL unavailable"
	if (!gl.has_value()) {
		// JS: webgl === null — no GL context
		logging::write("GPU: WebGL unavailable");
	} else if (!gl->error.empty()) {
		// JS: webgl.error — exception occurred
		logging::write(std::format("GPU: WebGL query failed: {}", gl->error));
	} else {
		if (!gl->renderer.empty())
			logging::write(std::format("GPU: {} ({})", gl->renderer, gl->vendor));
		else
			logging::write("GPU: WebGL debug info unavailable");

		logging::write(std::format("GPU caps: {}", format_caps(gl->caps)));

		if (!gl->extensions.empty())
			logging::write(std::format("GPU ext ({}): {}", gl->extensions.size(), format_extensions(gl->extensions)));
	}

	if (platform_info.has_value())
		logging::write(std::format("GPU: VRAM {}, Driver {}", platform_info->vram, platform_info->driver));
	else
		logging::write("GPU: Platform-specific info unavailable");
}

} // namespace gpu_info
