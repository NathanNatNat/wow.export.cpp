/*!
wow.export (https://github.com/Kruithne/wow.export)
Authors: Kruithne <kruithne@gmail.com>, Marlamin <marlamin@marlamin.com>
License: MIT
*/

#include "CharMaterialRenderer.h"
#include "../../casc/blp.h"
#include "../../core.h"
#include "../../log.h"
#include "../../casc/listfile.h"
#include "../../casc/casc-source.h"
#include "../../png-writer.h"
#include "../../ui/char-texture-overlay.h"
#include "../Shaders.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstring>
#include <format>
#include <stdexcept>

static const float UV_BUFFER_DATA[] = {
	0.0f, 1.0f,
	1.0f, 1.0f,
	0.0f, 0.0f,
	0.0f, 0.0f,
	1.0f, 1.0f,
	1.0f, 0.0f
};

CharMaterialRenderer::CharMaterialRenderer(int textureLayer, int width, int height)
	: width_(width), height_(height)
{
	// VAO is required in OpenGL 4.6 core profile (no default VAO exists).
	glGenVertexArrays(1, &vao_);
	glBindVertexArray(vao_);

	// Create FBO for offscreen rendering (replaces JS canvas + WebGL context)
	glGenFramebuffers(1, &fbo_);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

	// Color attachment
	glGenTextures(1, &fbo_texture_);
	glBindTexture(GL_TEXTURE_2D, fbo_texture_);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width_, height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_texture_, 0);

	// Depth attachment
	glGenRenderbuffers(1, &fbo_depth_);
	glBindRenderbuffer(GL_RENDERBUFFER, fbo_depth_);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width_, height_);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, fbo_depth_);

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
		logging::write(std::format("Failed to create FBO for CharMaterialRenderer layer {} (status: {})", textureLayer, static_cast<int>(status)));

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// character texture overlay so the user can cycle through them.
	char_texture_overlay::add(fbo_texture_);
}

/**
 * Initialize the CharMaterialRenderer.
 */
void CharMaterialRenderer::init() {
	compileShaders();
	reset();
}

/**
 * Get raw pixel data from the framebuffer.
 * Returns vector of RGBA pixels, with y-axis flipped.
 */
std::vector<uint8_t> CharMaterialRenderer::getRawPixels() {
	std::vector<uint8_t> pixels(width_ * height_ * 4);

	glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
	glReadPixels(0, 0, width_, height_, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// flip y-axis since gl.readPixels returns bottom-up
	std::vector<uint8_t> flipped(width_ * height_ * 4);
	for (int y = 0; y < height_; y++) {
		for (int x = 0; x < width_; x++) {
			const int src_idx = (y * width_ + x) * 4;
			const int dst_idx = ((height_ - y - 1) * width_ + x) * 4;
			flipped[dst_idx]     = pixels[src_idx];
			flipped[dst_idx + 1] = pixels[src_idx + 1];
			flipped[dst_idx + 2] = pixels[src_idx + 2];
			flipped[dst_idx + 3] = pixels[src_idx + 3];
		}
	}

	return flipped;
}

/**
 * Get URI from raw pixels, avoiding canvas alpha premultiplication.
 */
std::string CharMaterialRenderer::getURI() {
	auto pixels = getRawPixels();
	PNGWriter png(static_cast<uint32_t>(width_), static_cast<uint32_t>(height_));
	auto& pixel_data = png.getPixelData();
	std::memcpy(pixel_data.data(), pixels.data(), pixels.size());
	
	BufferWrapper buffer = png.getBuffer();
	std::string base64 = buffer.toBase64();
	return "data:image/png;base64," + base64;
}

/**
 * Reset canvas.
 */
void CharMaterialRenderer::reset() {
	unbindAllTextures();
	textureTargets.clear();
	clearCanvas();
}

/**
 * Loads a specific texture to a target.
 */
void CharMaterialRenderer::setTextureTarget(
	int chrModelTextureTargetID,
	uint32_t fileDataID,
	int sectionX, int sectionY, int sectionWidth, int sectionHeight,
	int materialTextureType, int materialWidth, int materialHeight,
	int textureLayerBlendMode,
	bool useAlpha,
	casc::BLPImage* blpOverride)
{
	// CharComponentTextureSection: SectionType, X, Y, Width, Height, OverlapSectionMask
	// ChrModelTextureLayer: TextureType, Layer, Flags, BlendMode, TextureSectionTypeBitMask, TextureSectionTypeBitMask2, ChrModelTextureTargetID[2]
	// ChrModelMaterial: TextureType, Width, Height, Flags, Unk
	// ChrCustomizationMaterial: ChrModelTextureTargetID, FileDataID (this is actually MaterialResourceID but we translate it before here)

	// For debug purposes
	std::string filename = casc::listfile::getByID(fileDataID).value_or("");
	spdlog::info("Loading texture {} for target {} with alpha {}", filename, chrModelTextureTargetID, useAlpha);

	GLuint textureID;
	if (blpOverride) {
		textureID = loadTextureFromBLP(*blpOverride, useAlpha);
		filename = "baked npc texture (override)";
	} else {
		textureID = loadTexture(fileDataID, useAlpha);
	}

	CharTextureTarget target;
	target.id = chrModelTextureTargetID;
	target.section.X = sectionX;
	target.section.Y = sectionY;
	target.section.Width = sectionWidth;
	target.section.Height = sectionHeight;
	target.material.TextureType = materialTextureType;
	target.material.Width = materialWidth;
	target.material.Height = materialHeight;
	target.textureLayer.BlendMode = textureLayerBlendMode;
	target.custMaterial.ChrModelTextureTargetID = chrModelTextureTargetID;
	target.custMaterial.FileDataID = fileDataID;
	target.textureID = textureID;
	target.filename = filename;

	textureTargets.push_back(std::move(target));

	update();
}

void CharMaterialRenderer::setTextureTarget(
	const TextureTargetInput& chr_cust_mat,
	const TextureSectionInput& char_component_texture_section,
	const ModelMaterialInput& chr_model_material,
	const TextureLayerInput& chr_model_texture_layer,
	bool useAlpha,
	casc::BLPImage* blpOverride)
{
	setTextureTarget(
		chr_cust_mat.ChrModelTextureTargetID,
		chr_cust_mat.FileDataID,
		char_component_texture_section.X,
		char_component_texture_section.Y,
		char_component_texture_section.Width,
		char_component_texture_section.Height,
		chr_model_material.TextureType,
		chr_model_material.Width,
		chr_model_material.Height,
		chr_model_texture_layer.BlendMode,
		useAlpha,
		blpOverride
	);
}

/**
 * Disposes of all the things
 */
void CharMaterialRenderer::dispose() {
	unbindAllTextures();

	// Delete all loaded layer textures.
	// associated resources; in desktop GL we must free them explicitly.
	for (auto& target : textureTargets) {
		if (target.textureID) {
			glDeleteTextures(1, &target.textureID);
			target.textureID = 0;
		}
	}
	textureTargets.clear();

	if (glShaderProg) {
		glDeleteProgram(glShaderProg);
		glShaderProg = 0;
	}

	clearCanvas();

	// destroying the FBO resources.
	char_texture_overlay::remove(fbo_texture_);

	// Delete FBO resources (replaces JS overlay.remove + WEBGL_lose_context)
	if (fbo_texture_) {
		glDeleteTextures(1, &fbo_texture_);
		fbo_texture_ = 0;
	}

	if (fbo_depth_) {
		glDeleteRenderbuffers(1, &fbo_depth_);
		fbo_depth_ = 0;
	}

	if (fbo_) {
		glDeleteFramebuffers(1, &fbo_);
		fbo_ = 0;
	}

	if (vao_) {
		glDeleteVertexArrays(1, &vao_);
		vao_ = 0;
	}
}

/**
 * Load a texture from CASC and bind it to the GL context.
 * @param fileDataID 
 * @param useAlpha
 */
GLuint CharMaterialRenderer::loadTexture(uint32_t fileDataID, bool useAlpha) {
	GLuint texture;
	glGenTextures(1, &texture);

	// core.view.casc is the active CASC source — use getVirtualFileByID to get file data.
	BufferWrapper fileData = casc_source_->getVirtualFileByID(fileDataID);
	casc::BLPImage blp(std::move(fileData));

	// TODO: DXT(1/3/5) support

	// For unknown reasons, we have to store blpData as a variable. Inlining it into the
	// parameter list causes issues, despite it being synchronous.

	std::vector<uint8_t> blpData = blp.toUInt8Array(0, useAlpha ? 0b1111 : 0b0111);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, static_cast<GLsizei>(blp.width), static_cast<GLsizei>(blp.height), 0, GL_RGBA, GL_UNSIGNED_BYTE, blpData.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	return texture;
}

GLuint CharMaterialRenderer::loadTextureFromBLP(casc::BLPImage& blp, bool useAlpha) {
	spdlog::info("loadtexturefromblp called with blp: width: {} height: {}", blp.width, blp.height);
	GLuint texture;
	glGenTextures(1, &texture);
	std::vector<uint8_t> blpData = blp.toUInt8Array(0, useAlpha ? 0b1111 : 0b0111);
	spdlog::info("blp data length: {} expected: {}", blpData.size(), blp.width * blp.height * 4);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, static_cast<GLsizei>(blp.width), static_cast<GLsizei>(blp.height), 0, GL_RGBA, GL_UNSIGNED_BYTE, blpData.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	spdlog::info("texture created successfully: {}", texture);
	return texture;
}

/**
 * Unbind all textures from the GL context.
 */
void CharMaterialRenderer::unbindAllTextures() {
	// Unbind textures.
	GLint maxUnits = 0;
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxUnits);
	for (int i = 0; i < maxUnits; i++) {
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

/**
 * Clear the canvas, resetting it to black.
 */
void CharMaterialRenderer::clearCanvas() {
	if (!fbo_)
		return;

	glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
	glViewport(0, 0, width_, height_);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

/**
 * Compile the vertex and fragment shaders used for baking.
 * Will be attached to the current GL context.
 */
void CharMaterialRenderer::compileShaders() {
	if (!fbo_)
		return;

	const shaders::ShaderSource& sources = shaders::get_source("char");

	glShaderProg = glCreateProgram();

	// Compile vertex shader.
	GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
	const char* vertSrc = sources.vert.c_str();
	glShaderSource(vertShader, 1, &vertSrc, nullptr);
	glCompileShader(vertShader);

	GLint compileStatus = 0;
	glGetShaderiv(vertShader, GL_COMPILE_STATUS, &compileStatus);
	if (!compileStatus) {
		char infoLog[1024];
		glGetShaderInfoLog(vertShader, sizeof(infoLog), nullptr, infoLog);
		logging::write(std::format("Vertex shader failed to compile: {}", infoLog));
		glDeleteShader(vertShader);
		throw std::runtime_error("Failed to compile vertex shader");
	}

	// Compile fragment shader.
	GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
	const char* fragSrc = sources.frag.c_str();
	glShaderSource(fragShader, 1, &fragSrc, nullptr);
	glCompileShader(fragShader);

	glGetShaderiv(fragShader, GL_COMPILE_STATUS, &compileStatus);
	if (!compileStatus) {
		char infoLog[1024];
		glGetShaderInfoLog(fragShader, sizeof(infoLog), nullptr, infoLog);
		logging::write(std::format("Fragment shader failed to compile: {}", infoLog));
		glDeleteShader(vertShader);
		glDeleteShader(fragShader);
		throw std::runtime_error("Failed to compile fragment shader");
	}

	// Attach shaders.
	glAttachShader(glShaderProg, vertShader);
	glAttachShader(glShaderProg, fragShader);

	// Link program.
	glLinkProgram(glShaderProg);
	GLint linkStatus = 0;
	glGetProgramiv(glShaderProg, GL_LINK_STATUS, &linkStatus);
	if (!linkStatus) {
		char infoLog[1024];
		glGetProgramInfoLog(glShaderProg, sizeof(infoLog), nullptr, infoLog);
		logging::write(std::format("Unable to link shader program: {}", infoLog));
		glDeleteShader(vertShader);
		glDeleteShader(fragShader);
		throw std::runtime_error("Failed to link shader program");
	}

	// Shaders can be detached/deleted after linking
	glDeleteShader(vertShader);
	glDeleteShader(fragShader);

	glUseProgram(glShaderProg);

	uvPositionAttribute = glGetAttribLocation(glShaderProg, "a_texCoord");
	textureLocation = glGetUniformLocation(glShaderProg, "u_texture");
	baseTextureLocation = glGetUniformLocation(glShaderProg, "u_baseTexture");
	blendModeLocation = glGetUniformLocation(glShaderProg, "u_blendMode");
	vertexPositionAttribute = glGetAttribLocation(glShaderProg, "a_position");
}

/**
 * Update 3D data.
 */
void CharMaterialRenderer::update() {
	if (!fbo_)
		return;

	// Save current framebuffer and VAO bindings, then bind ours
	GLint prevFBO = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);
	GLint prevVAO = 0;
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVAO);

	glBindVertexArray(vao_);

	clearCanvas();

	glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
	glViewport(0, 0, width_, height_);
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glDisable(GL_DEPTH_TEST);

	glUseProgram(glShaderProg);

	// order this.textureTargets by key
	std::sort(textureTargets.begin(), textureTargets.end(),
		[](const CharTextureTarget& a, const CharTextureTarget& b) { return a.id < b.id; });
	
	for (const auto& layer : textureTargets) {
		// hide underwear based on settings (target IDs 13/14 are upper/lower body base clothing)
		if (!core::view->config.value("chrIncludeBaseClothing", true) && (layer.id == 13 || layer.id == 14))
			continue;

		const float materialMiddleX = static_cast<float>(layer.material.Width) / 2.0f;
		const float materialMiddleY = static_cast<float>(layer.material.Height) / 2.0f;

		const float sectionTopLeftX = (static_cast<float>(layer.section.X) - materialMiddleX) / materialMiddleX;
		const float sectionTopLeftY = (static_cast<float>(layer.section.Y + layer.section.Height) - materialMiddleY) / materialMiddleY * -1.0f;
		
		const float sectionBottomRightX = (static_cast<float>(layer.section.X + layer.section.Width) - materialMiddleX) / materialMiddleX;
		const float sectionBottomRightY = (static_cast<float>(layer.section.Y) - materialMiddleY) / materialMiddleY * -1.0f;

		spdlog::info("[{}] Placing texture {} of blend mode {} for target {} with offset {}x{} of size {}x{} at {}, {} to {}, {}",
			layer.material.TextureType, layer.filename, layer.textureLayer.BlendMode, layer.id,
			layer.section.X, layer.section.Y, layer.section.Width, layer.section.Height,
			sectionTopLeftX, sectionTopLeftY, sectionBottomRightX, sectionBottomRightY);

		// Vertex buffer
		GLuint vBuffer;
		glGenBuffers(1, &vBuffer);
		const float vBufferData[] = {
			sectionTopLeftX, sectionTopLeftY, 0.0f,
			sectionBottomRightX, sectionTopLeftY, 0.0f,
			sectionTopLeftX, sectionBottomRightY, 0.0f,
			sectionTopLeftX, sectionBottomRightY, 0.0f,
			sectionBottomRightX, sectionTopLeftY, 0.0f,
			sectionBottomRightX, sectionBottomRightY, 0.0f
		};

		glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vBufferData), vBufferData, GL_STATIC_DRAW);

		glVertexAttribPointer(vertexPositionAttribute, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
		glEnableVertexAttribArray(vertexPositionAttribute);

		// TexCoord buffer
		GLuint uvBuffer;
		glGenBuffers(1, &uvBuffer);

		glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(UV_BUFFER_DATA), UV_BUFFER_DATA, GL_STATIC_DRAW);

		glVertexAttribPointer(uvPositionAttribute, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
		glEnableVertexAttribArray(uvPositionAttribute);

		glUniform1i(textureLocation, 0); // Bind materials
		glUniform1f(blendModeLocation, static_cast<float>(layer.textureLayer.BlendMode)); // Bind blend mode

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, layer.textureID);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		switch (layer.textureLayer.BlendMode) {
			case 0: // None
				glDisable(GL_BLEND);
				glBlendFunc(GL_ONE, GL_ZERO);
				break;
			case 1: // Blit
			case 4: // Multiply
			case 6: // Overlay
			case 7: // Screen
			case 15: // Infer alpha blend
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				break;
			case 9: // Alpha Straight
				glEnable(GL_BLEND);
				glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
				break;
			// The following blend modes are not used in character customization
			case 2: // Blit Alphamask 
			case 3: // Add 
			case 5: // Mod2x 
			case 8: // Hardlight
			case 10: // Blend black
			case 11: // Mask greyscale
			case 12: // Mask greyscale using color as alpha
			case 13: // Generate greyscale
			case 14: // Colorize
				logging::write(std::format("Warning: encountered previously unused blendmode {} during character texture baking, poke a dev", layer.textureLayer.BlendMode));
				break;
			// These are used but we don't know if they need blending enabled -- so just turn it on anyways
			case 16: // Unknown, only used for TaunkaMale.m2, probably experimental/unused
			default:
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				break;
		}

		if (layer.textureLayer.BlendMode == 4 || layer.textureLayer.BlendMode == 6 || layer.textureLayer.BlendMode == 7) {
			// Create new texture of current canvas
			GLuint canvasTexture;
			glGenTextures(1, &canvasTexture);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, canvasTexture);

			if (layer.material.Width == layer.section.Width && layer.material.Height == layer.section.Height) {
				// Just copy the canvas (read from FBO and upload).
				// (canvas is top-down, textures are bottom-up). glReadPixels
				// returns bottom-up data, so we must flip to match JS behavior.
				std::vector<uint8_t> canvasPixels(width_ * height_ * 4);
				glReadPixels(0, 0, width_, height_, GL_RGBA, GL_UNSIGNED_BYTE, canvasPixels.data());

				std::vector<uint8_t> flippedCanvas(width_ * height_ * 4);
				const int rowBytes = width_ * 4;
				for (int y = 0; y < height_; y++)
					std::memcpy(&flippedCanvas[(height_ - y - 1) * rowBytes], &canvasPixels[y * rowBytes], rowBytes);

				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width_, height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, flippedCanvas.data());
			} else {
				// Get pixels of relevant section
				std::vector<uint8_t> pixelBuffer(layer.section.Width * layer.section.Height * 4);
				glReadPixels(layer.section.X, layer.section.Y, layer.section.Width, layer.section.Height, GL_RGBA, GL_UNSIGNED_BYTE, pixelBuffer.data());

				// Flip pixelbuffer on its y-axis
				std::vector<uint8_t> flippedPixelBuffer(layer.section.Width * layer.section.Height * 4);
				for (int y = 0; y < layer.section.Height; y++) {
					for (int x = 0; x < layer.section.Width; x++) {
						const int index = (y * layer.section.Width + x) * 4;
						const int flippedIndex = ((layer.section.Height - y - 1) * layer.section.Width + x) * 4;
						flippedPixelBuffer[flippedIndex]     = pixelBuffer[index];
						flippedPixelBuffer[flippedIndex + 1] = pixelBuffer[index + 1];
						flippedPixelBuffer[flippedIndex + 2] = pixelBuffer[index + 2];
						flippedPixelBuffer[flippedIndex + 3] = pixelBuffer[index + 3];
					}
				}

				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, layer.section.Width, layer.section.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, flippedPixelBuffer.data());
			}
			
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glUniform1i(baseTextureLocation, 1);

			// Draw
			glDrawArrays(GL_TRIANGLES, 0, 6);

			// Clean up canvasTexture — JS relies on WebGL GC / loseContext;
			// in desktop GL we must delete explicitly.
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, 0);
			glDeleteTextures(1, &canvasTexture);
		} else {
			// Draw
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}

		// Clean up per-layer buffers
		glDeleteBuffers(1, &vBuffer);
		glDeleteBuffers(1, &uvBuffer);
	}

	// Restore previous framebuffer and VAO
	glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(prevFBO));
	glBindVertexArray(static_cast<GLuint>(prevVAO));
}
