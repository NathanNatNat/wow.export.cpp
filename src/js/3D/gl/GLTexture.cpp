/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

#include "GLTexture.h"
#include "GLContext.h"
#include "casc/blp.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace gl {

GLTexture::GLTexture(GLContext& ctx) : ctx_(ctx) {
	glGenTextures(1, &texture);
}

void GLTexture::bind(int unit) {
	ctx_.bind_texture(unit, texture);
}

void GLTexture::set_rgba(const uint8_t* pixels, int w, int h,
                          const TextureOptions& options) {
	width = w;
	height = h;
	has_alpha = options.has_alpha;

	// Bind directly — invalidate cache entry for active unit so subsequent
	// bind_texture() calls won't skip the real glBindTexture.
	glBindTexture(GL_TEXTURE_2D, texture);
	ctx_.invalidate_cache();

	// flip Y to match Three.js / WebGL UNPACK_FLIP_Y_WEBGL behaviour.
	// Desktop GL has no UNPACK_FLIP_Y state, so we flip the rows manually.
	if (options.flip_y && pixels) {
		const size_t row_bytes = static_cast<size_t>(w) * 4;
		std::vector<uint8_t> flipped(static_cast<size_t>(w) * h * 4);
		for (int row = 0; row < h; ++row) {
			std::memcpy(flipped.data() + row * row_bytes,
			            pixels + (h - 1 - row) * row_bytes,
			            row_bytes);
		}
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0,
		             GL_RGBA, GL_UNSIGNED_BYTE, flipped.data());
	} else {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0,
		             GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	}

	_apply_wrap(options.wrap_s, options.wrap_t);
	_apply_filter(options.min_filter, options.mag_filter);

	if (options.generate_mipmaps)
		glGenerateMipmap(GL_TEXTURE_2D);
}

// via texImage2D(... canvas).  In desktop C++ there is no canvas, so the
// caller supplies pixel data + dimensions directly.
void GLTexture::set_canvas(const uint8_t* pixels, int w, int h,
                            const TextureOptions& options) {
	TextureOptions opts = options;
	opts.has_alpha = true;
	set_rgba(pixels, w, h, opts);
}

void GLTexture::set_compressed(std::span<const CompressedMipmap> mipmaps,
                                GLenum format) {
	if (!ctx_.ext_s3tc)
		throw std::runtime_error("S3TC compression not supported");

	glBindTexture(GL_TEXTURE_2D, texture);
	ctx_.invalidate_cache();

	for (size_t i = 0; i < mipmaps.size(); ++i) {
		const auto& mip = mipmaps[i];
		glCompressedTexImage2D(GL_TEXTURE_2D, static_cast<GLint>(i), format,
		                       mip.width, mip.height, 0,
		                       static_cast<GLsizei>(mip.data.size()),
		                       mip.data.data());
	}

	width = mipmaps[0].width;
	height = mipmaps[0].height;

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL,
	                static_cast<GLint>(mipmaps.size() - 1));
	_apply_wrap();
	_apply_filter(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
}

void GLTexture::_apply_wrap(GLenum wrap_s, GLenum wrap_t) {
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_s);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_t);
}

void GLTexture::_apply_filter(GLenum min_filter, GLenum mag_filter) {
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);

	// apply anisotropic filtering if available
	if (ctx_.ext_aniso && min_filter != GL_NEAREST)
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY,
		                std::min(8.0f, ctx_.max_anisotropy));
}

void GLTexture::set_wrap(GLenum wrap_s, GLenum wrap_t) {
	glBindTexture(GL_TEXTURE_2D, texture);
	ctx_.invalidate_cache();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_s);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_t);
}

void GLTexture::set_blp(casc::BLPImage& blp, const BLPTextureFlags& flags) {
	// determine wrap mode from flags
	bool should_wrap_s = flags.wrap_s.value_or((flags.flags & 0x1) != 0);
	bool should_wrap_t = flags.wrap_t.value_or((flags.flags & 0x2) != 0);
	GLenum wrap_s = should_wrap_s ? GL_REPEAT : GL_CLAMP_TO_EDGE;
	GLenum wrap_t = should_wrap_t ? GL_REPEAT : GL_CLAMP_TO_EDGE;

	// for now, always decode to RGBA
	// todo: support compressed textures when format issues are resolved
	std::vector<uint8_t> pixels = blp.toUInt8Array(0, 0b1111);
	set_rgba(pixels.data(), static_cast<int>(blp.width),
	         static_cast<int>(blp.height), {
		.has_alpha = blp.alphaDepth > 0,
		.wrap_s = wrap_s,
		.wrap_t = wrap_t,
		.generate_mipmaps = true
	});
}

void GLTexture::dispose() {
	if (texture) {
		glDeleteTextures(1, &texture);
		texture = 0;
	}
}

} // namespace gl
