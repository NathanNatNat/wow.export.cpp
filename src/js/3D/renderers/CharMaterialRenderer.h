/*!
wow.export (https://github.com/Kruithne/wow.export)
Authors: Kruithne <kruithne@gmail.com>, Marlamin <marlamin@marlamin.com>
License: MIT
 */
#pragma once

#include <glad/gl.h>
#include <cstdint>
#include <string>
#include <vector>

namespace casc {
class BLPImage;
class CASC;
}

/**
 * Texture target entry — represents a single texture layer placed on
 * the character material canvas.
 *
 * JS equivalent: the object literal pushed into this.textureTargets[]
 */
struct CharTextureTarget {
	int id = 0;

	// CharComponentTextureSection fields
	struct {
		int SectionType = 0;
		int X = 0;
		int Y = 0;
		int Width = 0;
		int Height = 0;
		int OverlapSectionMask = 0;
	} section;

	// ChrModelMaterial fields
	struct {
		int TextureType = 0;
		int Width = 0;
		int Height = 0;
		int Flags = 0;
		int Unk = 0;
	} material;

	// ChrModelTextureLayer fields
	struct {
		int TextureType = 0;
		int Layer = 0;
		int Flags = 0;
		int BlendMode = 0;
		int TextureSectionTypeBitMask = 0;
		int TextureSectionTypeBitMask2 = 0;
	} textureLayer;

	// ChrCustomizationMaterial fields
	struct {
		int ChrModelTextureTargetID = 0;
		uint32_t FileDataID = 0;
	} custMaterial;

	GLuint textureID = 0;
	std::string filename;
};

/**
 * Offscreen renderer that composites character texture layers into a
 * single baked material using an FBO (replacing the JS WebGL canvas).
 *
 * JS equivalent: class CharMaterialRenderer — module.exports = CharMaterialRenderer
 */
class CharMaterialRenderer {
public:
	/**
	 * Construct a new CharMaterialRenderer instance.
	 */
	CharMaterialRenderer(int textureLayer, int width, int height);

	/**
	 * Initialize the CharMaterialRenderer.
	 */
	void init();

	/**
	 * Get raw pixel data from the framebuffer.
	 * Returns vector of RGBA pixels, with y-axis flipped.
	 */
	std::vector<uint8_t> getRawPixels();

	/**
	 * Get canvas.
	 * JS equivalent: returns the HTML canvas element. In C++ returns the FBO
	 * color-attachment texture ID, which serves as the layer identity in
	 * char_texture_overlay.
	 */
	GLuint getCanvas() const { return fbo_texture_; }

	/**
	 * Get URI from raw pixels, avoiding canvas alpha premultiplication.
	 */
	std::string getURI();

	/**
	 * Reset canvas.
	 */
	void reset();

	/**
	 * Loads a specific texture to a target.
	 */
	void setTextureTarget(
		int chrModelTextureTargetID,
		uint32_t fileDataID,
		int sectionX, int sectionY, int sectionWidth, int sectionHeight,
		int materialTextureType, int materialWidth, int materialHeight,
		int textureLayerBlendMode,
		bool useAlpha = true,
		casc::BLPImage* blpOverride = nullptr
	);

	/**
	 * Disposes of all the things
	 */
	void dispose();

	/**
	 * Load a texture from CASC and bind it to the GL context.
	 * @param fileDataID 
	 * @param useAlpha
	 */
	GLuint loadTexture(uint32_t fileDataID, bool useAlpha = true);

	GLuint loadTextureFromBLP(casc::BLPImage& blp, bool useAlpha = true);

	/**
	 * Unbind all textures from the GL context.
	 */
	void unbindAllTextures();

	/**
	 * Clear the canvas, resetting it to black.
	 */
	void clearCanvas();

	/**
	 * Compile the vertex and fragment shaders used for baking.
	 */
	void compileShaders();

	/**
	 * Update 3D data.
	 */
	void update();

	/**
	 * Set the active CASC source for texture loading.
	 * JS equivalent: accesses core.view.casc directly.
	 * @param source Pointer to active CASC source.
	 */
	void setCASCSource(casc::CASC* source) { casc_source_ = source; }

	std::vector<CharTextureTarget> textureTargets;

	int getWidth() const { return width_; }
	int getHeight() const { return height_; }

private:
	int width_;
	int height_;

	// Active CASC source for loading texture files.
	// JS: core.view.casc — set by caller before texture loading.
	casc::CASC* casc_source_ = nullptr;

	// FBO-based offscreen rendering (replaces JS canvas + WebGL context)
	GLuint fbo_ = 0;
	GLuint fbo_texture_ = 0;
	GLuint fbo_depth_ = 0;

	// VAO — required in OpenGL 4.6 core profile (WebGL 1.0 has default VAO)
	GLuint vao_ = 0;

	// Shader program (manually compiled, not via Shaders module —
	// matches JS which uses raw WebGL, not the ShaderProgram wrapper)
	GLuint glShaderProg = 0;

	// Attribute/uniform locations
	GLint uvPositionAttribute = -1;
	GLint textureLocation = -1;
	GLint baseTextureLocation = -1;
	GLint blendModeLocation = -1;
	GLint vertexPositionAttribute = -1;
};
