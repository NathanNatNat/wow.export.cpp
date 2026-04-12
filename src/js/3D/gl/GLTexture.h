/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <glad/gl.h>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace casc { class BLPImage; }

namespace gl {

class GLContext;

/**
 * Options for set_rgba / set_canvas texture uploads.
 */
struct TextureOptions {
	bool has_alpha = true;
	GLenum wrap_s = GL_CLAMP_TO_EDGE;
	GLenum wrap_t = GL_CLAMP_TO_EDGE;
	GLenum min_filter = GL_LINEAR;
	GLenum mag_filter = GL_LINEAR;
	bool generate_mipmaps = false;
	bool flip_y = true; // match JS UNPACK_FLIP_Y_WEBGL behaviour
};

/**
 * Mipmap entry for compressed (DXT) texture uploads.
 */
struct CompressedMipmap {
	std::span<const uint8_t> data;
	int width = 0;
	int height = 0;
};

/**
 * BLP-specific texture flags (wrap mode from M2/WMO material flags).
 */
struct BLPTextureFlags {
	uint32_t flags = 0;
	std::optional<bool> wrap_s;
	std::optional<bool> wrap_t;
};

/**
 * OpenGL texture wrapper.
 *
 * JS equivalent: class GLTexture — module.exports = GLTexture
 */
class GLTexture {
public:
	explicit GLTexture(GLContext& ctx);

	void bind(int unit);

	void set_rgba(const uint8_t* pixels, int width, int height,
	              const TextureOptions& options = {});

	// (Browser API systemic translation — no HTML canvas in desktop GL)
	void set_canvas(const uint8_t* pixels, int width, int height,
	                const TextureOptions& options = {});

	void set_compressed(std::span<const CompressedMipmap> mipmaps, GLenum format);

	void set_wrap(GLenum wrap_s, GLenum wrap_t);

	void set_blp(casc::BLPImage& blp, const BLPTextureFlags& flags = {});

	void dispose();

	// DXT format constants (from S3TC extension)
	static constexpr GLenum DXT1_RGB  = 0x83F0;
	static constexpr GLenum DXT1_RGBA = 0x83F1;
	static constexpr GLenum DXT3      = 0x83F2;
	static constexpr GLenum DXT5      = 0x83F3;

	// Public state (matches JS properties)
	GLuint texture = 0;
	int width = 0;
	int height = 0;
	bool has_alpha = false;

private:
	void _apply_wrap(GLenum wrap_s = GL_CLAMP_TO_EDGE,
	                 GLenum wrap_t = GL_CLAMP_TO_EDGE);
	void _apply_filter(GLenum min_filter = GL_LINEAR,
	                   GLenum mag_filter = GL_LINEAR);

	GLContext& ctx_;
};

} // namespace gl
