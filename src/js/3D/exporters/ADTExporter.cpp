/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "ADTExporter.h"

#include <glad/gl.h>

#include <stb_image_resize2.h>

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <format>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <nlohmann/json.hpp>
#include <variant>
#include <vector>

#include "../../core.h"
#include "../../constants.h"
#include "../../generics.h"
#include "../../log.h"
#include "../../buffer.h"
#include "../../png-writer.h"

#include "../../casc/casc-source.h"
#include "../../casc/listfile.h"
#include "../../casc/export-helper.h"
#include "../../casc/blp.h"
#include "../../casc/db2.h"
#include "../../db/WDCReader.h"

#include "../loaders/WDTLoader.h"
#include "../loaders/ADTLoader.h"
#include "../Shaders.h"

#include "../writers/OBJWriter.h"
#include "../writers/MTLWriter.h"
#include "../writers/CSVWriter.h"
#include "../writers/JSONWriter.h"

#include "M2Exporter.h"
#include "WMOExporter.h"

#include "../loaders/M2Loader.h"
#include "../loaders/M3Loader.h"
#include "../loaders/WMOLoader.h"
#include "../Skin.h"
#include "../Texture.h"

namespace fs = std::filesystem;

static constexpr int MAP_SIZE = constants::GAME::MAP_SIZE;
static constexpr double TILE_SIZE = constants::GAME::TILE_SIZE;
static constexpr double CHUNK_SIZE = TILE_SIZE / 16.0;
static constexpr double UNIT_SIZE = CHUNK_SIZE / 8.0;
static constexpr double UNIT_SIZE_HALF = UNIT_SIZE / 2.0;

/**
 * WDT cache entry: stores both the BufferWrapper (which WDTLoader references)
 * and the WDTLoader itself. Must use unique_ptr since WDTLoader holds a
 * BufferWrapper& reference and cannot be moved.
 */
struct WDTCacheEntry {
	BufferWrapper data;
	std::unique_ptr<WDTLoader> loader;

	WDTCacheEntry(BufferWrapper d)
		: data(std::move(d))
		, loader(std::make_unique<WDTLoader>(data))
	{
	}
};

static std::unordered_map<std::string, std::unique_ptr<WDTCacheEntry>> wdtCache;

static bool isFoliageAvailable = false;
static bool hasLoadedFoliage = false;
static db::WDCReader* dbTextures = nullptr;
static db::WDCReader* dbDoodads = nullptr;

static GLuint glShaderProg = 0;
static GLuint glFBO = 0;
static GLuint glFBOTexture = 0;
static int glCanvasWidth = 0;
static int glCanvasHeight = 0;
static bool glInitialized = false;

/**
 * Load a texture from CASC and bind it to the GL context.
 * @param fileDataID
 * @param casc CASC source
 */
static GLuint loadTexture(uint32_t fileDataID, casc::CASC* casc) {
	GLuint texture;
	glGenTextures(1, &texture);

	auto data = casc->getVirtualFileByID(fileDataID);
	casc::BLPImage blp(std::move(data));
	auto blpData = blp.toUInt8Array(0);

	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, blp.getScaledWidth(), blp.getScaledHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, blpData.data());
	glGenerateMipmap(GL_TEXTURE_2D);

	return texture;
}

/**
 * Load and cache GroundEffectDoodad and GroundEffectTexture data tables.
 */
static void loadFoliageTables() {
	if (!hasLoadedFoliage) {
		try {
			dbDoodads = &casc::db2::getTable("GroundEffectDoodad");
			dbTextures = &casc::db2::getTable("GroundEffectTexture");

			hasLoadedFoliage = true;
			isFoliageAvailable = true;
		} catch (const std::exception&) {
			isFoliageAvailable = false;
			logging::write("Unable to load foliage tables, foliage exporting will be unavailable for all tiles.");
		}

		hasLoadedFoliage = true;
	}
}

/**
 * Bind an alpha layer to the GL context.
 * @param layer
 */
static GLuint bindAlphaLayer(const std::vector<uint8_t>& layer) {
	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	std::vector<uint8_t> data(layer.size() * 4);
	for (size_t i = 0, j = 0, n = layer.size(); i < n; i++, j += 4)
		data[j + 0] = data[j + 1] = data[j + 2] = data[j + 3] = layer[i];

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
	glGenerateMipmap(GL_TEXTURE_2D);

	return texture;
}

/**
 * Unbind all textures from the GL context.
 */
static void unbindAllTextures() {
	GLint maxUnits = 0;
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxUnits);
	for (int i = 0; i < maxUnits; i++) {
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	}
}

static GLuint build_texture_array(const std::vector<uint32_t>& file_data_ids, [[maybe_unused]] bool is_height, casc::CASC* casc) {
	// Force 512x512 to avoid canvas corruption
	const int target_size = 512;

	GLuint tex_array;
	glGenTextures(1, &tex_array);
	glBindTexture(GL_TEXTURE_2D_ARRAY, tex_array);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, target_size, target_size, static_cast<GLsizei>(file_data_ids.size()), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	for (size_t i = 0; i < file_data_ids.size(); i++) {
		auto fileData = casc->getVirtualFileByID(file_data_ids[i]);
		casc::BLPImage blp(std::move(fileData));
		auto blp_rgba = blp.toUInt8Array(0);

		if (blp.width == static_cast<uint32_t>(target_size) && blp.height == static_cast<uint32_t>(target_size)) {
			// Direct upload without processing
			glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, static_cast<GLint>(i), target_size, target_size, 1, GL_RGBA, GL_UNSIGNED_BYTE, blp_rgba.data());
		} else {
			// Manual nearest-neighbor resize to avoid canvas corruption
			std::vector<uint8_t> resized(target_size * target_size * 4);
			const float scale_x = static_cast<float>(blp.width) / target_size;
			const float scale_y = static_cast<float>(blp.height) / target_size;

			for (int y = 0; y < target_size; y++) {
				for (int x = 0; x < target_size; x++) {
					const int src_x = static_cast<int>(std::floor(x * scale_x));
					const int src_y = static_cast<int>(std::floor(y * scale_y));
					const int src_idx = (src_y * static_cast<int>(blp.width) + src_x) * 4;
					const int dst_idx = (y * target_size + x) * 4;

					resized[dst_idx] = blp_rgba[src_idx];
					resized[dst_idx + 1] = blp_rgba[src_idx + 1];
					resized[dst_idx + 2] = blp_rgba[src_idx + 2];
					resized[dst_idx + 3] = blp_rgba[src_idx + 3];
				}
			}

			glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, static_cast<GLint>(i), target_size, target_size, 1, GL_RGBA, GL_UNSIGNED_BYTE, resized.data());
		}
	}

	glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
	return tex_array;
}

/**
 * Clear the canvas, resetting it to black.
 */
static void clearCanvas() {
	glViewport(0, 0, glCanvasWidth, glCanvasHeight);
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
}

/**
 * Convert an RGBA object into an integer.
 * @param rgba
 * @returns integer
 */
static uint32_t rgbaToInt(const ADTVertexShading& rgba) {
	uint32_t intval = rgba.r;
	intval = (intval << 8) + rgba.g;
	intval = (intval << 8) + rgba.b;
	return (intval << 8) + rgba.a;
}

/**
 * Calculate number of images required for given layer count.
 * Each image can hold 4 layers (RGBA channels), starting from layer 1.
 * @param layerCount Total number of layers including base layer 0
 * @returns Number of images needed
 */
static int calculateRequiredImages(int layerCount) {
	if (layerCount <= 1) return 0; // no alpha layers to export
	return static_cast<int>(std::ceil((layerCount - 1) / 4.0)); // layer 0 is base, skip it
}

/**
 * Compile the vertex and fragment shaders used for baking.
 * Will be attached to the current GL context.
 */
static void compileShaders(bool useOld = false) {
	const std::string shader_name = useOld ? "adt_old" : "adt";
	const auto& sources = shaders::get_source(shader_name);

	glShaderProg = glCreateProgram();

	// Compile fragment shader.
	GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
	const char* fragSrc = sources.frag.c_str();
	glShaderSource(fragShader, 1, &fragSrc, nullptr);
	glCompileShader(fragShader);

	GLint fragStatus = 0;
	glGetShaderiv(fragShader, GL_COMPILE_STATUS, &fragStatus);
	if (!fragStatus) {
		char infoLog[512];
		glGetShaderInfoLog(fragShader, 512, nullptr, infoLog);
		logging::write(std::format("Fragment shader failed to compile: {}", infoLog));
		throw std::runtime_error("Failed to compile fragment shader");
	}

	// Compile vertex shader.
	GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
	const char* vertSrc = sources.vert.c_str();
	glShaderSource(vertShader, 1, &vertSrc, nullptr);
	glCompileShader(vertShader);

	GLint vertStatus = 0;
	glGetShaderiv(vertShader, GL_COMPILE_STATUS, &vertStatus);
	if (!vertStatus) {
		char infoLog[512];
		glGetShaderInfoLog(vertShader, 512, nullptr, infoLog);
		logging::write(std::format("Vertex shader failed to compile: {}", infoLog));
		throw std::runtime_error("Failed to compile vertex shader");
	}

	// Attach shaders.
	glAttachShader(glShaderProg, fragShader);
	glAttachShader(glShaderProg, vertShader);

	// Link program.
	glLinkProgram(glShaderProg);
	GLint linkStatus = 0;
	glGetProgramiv(glShaderProg, GL_LINK_STATUS, &linkStatus);
	if (!linkStatus) {
		char infoLog[512];
		glGetProgramInfoLog(glShaderProg, 512, nullptr, infoLog);
		logging::write(std::format("Unable to link shader program: {}", infoLog));
		throw std::runtime_error("Failed to link shader program");
	}

	glUseProgram(glShaderProg);
}

/**
 * Initialize offscreen FBO for texture baking.
 * @param width
 * @param height
 */
static void initFBO(int width, int height) {
	if (glFBO != 0) {
		glDeleteFramebuffers(1, &glFBO);
		glDeleteTextures(1, &glFBOTexture);
	}

	glCanvasWidth = width;
	glCanvasHeight = height;

	glGenFramebuffers(1, &glFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, glFBO);

	glGenTextures(1, &glFBOTexture);
	glBindTexture(GL_TEXTURE_2D, glFBOTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, glFBOTexture, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, glFBO);
}

/**
 * Read pixels from the current FBO into a buffer.
 * @param width
 * @param height
 * @returns pixel data as RGBA
 */
static std::vector<uint8_t> readFBOPixels(int width, int height) {
	std::vector<uint8_t> pixels(width * height * 4);
	glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
	return pixels;
}

ADTExporter::ADTExporter(uint32_t mapID, const std::string& mapDir, uint32_t tileIndex)
	: mapID(mapID)
	, mapDir(mapDir)
	, tileX(tileIndex % MAP_SIZE)
	, tileY(tileIndex / MAP_SIZE)
	, tileID(std::to_string(tileY) + "_" + std::to_string(tileX))
	, tileIndex(tileIndex)
{
}

/**
 * Calculate UV bounds for normalization.
 * @param rootAdt The root ADT data
 * @param firstChunkX First chunk X coordinate
 * @param firstChunkY First chunk Y coordinate
 * @returns UV bounds {minU, maxU, minV, maxV}
 */
UVBounds ADTExporter::calculateUVBounds(const ADTLoader& rootAdt, float firstChunkX, float firstChunkY) {
	float minU = std::numeric_limits<float>::infinity();
	float maxU = -std::numeric_limits<float>::infinity();
	float minV = std::numeric_limits<float>::infinity();
	float maxV = -std::numeric_limits<float>::infinity();

	for (int x = 0; x < 16; x++) {
		for (int y = 0; y < 16; y++) {
			const size_t ci = static_cast<size_t>(x * 16 + y);
			if (ci >= rootAdt.chunks.size())
				continue;

			const auto& chunk = rootAdt.chunks[ci];
			if (chunk.vertices.empty())
				continue;

			const float chunkX = chunk.position[0];
			const float chunkY = chunk.position[1];

			for (int row = 0, idx = 0; row < 17; row++) {
				const bool isShort = !!(row % 2);
				const int colCount = isShort ? 8 : 9;

				for (int col = 0; col < colCount; col++) {
					float vx = chunkY - (col * static_cast<float>(UNIT_SIZE));
					float vz = chunkX - (row * static_cast<float>(UNIT_SIZE_HALF));

					if (isShort)
						vx -= static_cast<float>(UNIT_SIZE_HALF);

					const float u = -(vx - firstChunkX) / static_cast<float>(TILE_SIZE);
					const float v = (vz - firstChunkY) / static_cast<float>(TILE_SIZE);

					minU = std::min(minU, u);
					maxU = std::max(maxU, u);
					minV = std::min(minV, v);
					maxV = std::max(maxV, v);

					idx++;
				}
			}
		}
	}

	return { minU, maxU, minV, maxV };
}

/**
 * Export the ADT tile.
 * @param dir Directory to export the tile into.
 * @param quality
 * @param gameObjects Additional game objects to export.
 * @param helper
 * @returns Export result with type and path.
 */
ADTExportResult ADTExporter::exportTile(const fs::path& dir, int quality,
	const std::vector<ADTGameObject>* gameObjects, casc::ExportHelper* helper,
	casc::CASC* casc)
{
	const auto& config = core::view->config;

	const bool isRawExport = config.value("exportMapFormat", std::string("")) == "RAW";
	ADTExportResult out;
	out.type = isRawExport ? "ADT_RAW" : "ADT_OBJ";

	const bool usePosix = config.value("pathFormat", std::string("")) == "posix";
	const std::string prefix = std::format("world/maps/{}/{}", mapDir, mapDir);

	// Load the WDT. We cache this to speed up exporting large amounts of tiles
	// from the same map. Make sure ADTLoader.clearCache() is called after exporting.
	WDTLoader* wdt = nullptr;
	auto wdtIt = wdtCache.find(mapDir);
	if (wdtIt != wdtCache.end()) {
		wdt = wdtIt->second->loader.get();
	} else {
		auto wdtFile = casc->getVirtualFileByName(prefix + ".wdt");

		if (isRawExport)
			wdtFile.writeToFile(dir / (mapDir + ".wdt"));

		auto entry = std::make_unique<WDTCacheEntry>(std::move(wdtFile));
		entry->loader->load();
		wdt = entry->loader.get();
		wdtCache[mapDir] = std::move(entry);

		if (isRawExport) {

			if (wdt->lgtFileDataID > 0) {
				auto lgtFile = casc->getVirtualFileByID(wdt->lgtFileDataID);
				lgtFile.writeToFile(dir / (mapDir + "_lgt.wdt"));
			}

			if (wdt->occFileDataID > 0) {
				auto occFile = casc->getVirtualFileByID(wdt->occFileDataID);
				occFile.writeToFile(dir / (mapDir + "_occ.wdt"));
			}

			if (wdt->fogsFileDataID > 0) {
				auto fogsFile = casc->getVirtualFileByID(wdt->fogsFileDataID);
				fogsFile.writeToFile(dir / (mapDir + "_fogs.wdt"));
			}

			if (wdt->mpvFileDataID > 0) {
				auto mpvFile = casc->getVirtualFileByID(wdt->mpvFileDataID);
				mpvFile.writeToFile(dir / (mapDir + "_mpv.wdt"));
			}

			if (wdt->texFileDataID > 0) {
				auto texFile = casc->getVirtualFileByID(wdt->texFileDataID);
				texFile.writeToFile(dir / (mapDir + ".tex"));
			}

			if (wdt->wdlFileDataID > 0) {
				auto wdlFile = casc->getVirtualFileByID(wdt->wdlFileDataID);
				wdlFile.writeToFile(dir / (mapDir + ".wdl"));
			}

			if (wdt->pd4FileDataID > 0) {
				auto pd4File = casc->getVirtualFileByID(wdt->pd4FileDataID);
				pd4File.writeToFile(dir / (mapDir + ".pd4"));
			}
		}
	}

	const std::string tilePrefix = prefix + "_" + tileID;

	const auto& maid = wdt->entries[tileIndex];
	const uint32_t rootFileDataID = maid.rootADT > 0 ? maid.rootADT : casc::listfile::getByFilename(tilePrefix + ".adt").value_or(0);
	const uint32_t tex0FileDataID = maid.tex0ADT > 0 ? maid.tex0ADT : casc::listfile::getByFilename(tilePrefix + "_tex0.adt").value_or(0);
	const uint32_t obj0FileDataID = maid.obj0ADT > 0 ? maid.obj0ADT : casc::listfile::getByFilename(tilePrefix + "_obj0.adt").value_or(0);
	const uint32_t obj1FileDataID = maid.obj1ADT > 0 ? maid.obj1ADT : casc::listfile::getByFilename(tilePrefix + "_obj1.adt").value_or(0);

	// Ensure we actually have the fileDataIDs for the files we need. LOD is not available on Classic.
	if (rootFileDataID == 0 || tex0FileDataID == 0 || obj0FileDataID == 0 || obj1FileDataID == 0)
		throw std::runtime_error(std::format("Missing fileDataID for ADT files: {}, {}, {}", rootFileDataID, tex0FileDataID, obj0FileDataID));

	auto rootFile = casc->getVirtualFileByID(rootFileDataID);
	auto texFile = casc->getVirtualFileByID(tex0FileDataID);
	auto objFile = casc->getVirtualFileByID(obj0FileDataID);

	if (isRawExport) {
		rootFile.writeToFile(dir / (mapDir + "_" + tileID + ".adt"));
		texFile.writeToFile(dir / (mapDir + "_" + tileID + "_tex0.adt"));
		objFile.writeToFile(dir / (mapDir + "_" + tileID + "_obj0.adt"));

		// We only care about these when exporting raw files.
		auto obj1File = casc->getVirtualFileByID(obj1FileDataID);
		obj1File.writeToFile(dir / (mapDir + "_" + tileID + "_obj1.adt"));

		// LOD is not available on Classic.
		if (maid.lodADT > 0) {
			auto lodFile = casc->getVirtualFileByID(maid.lodADT);
			lodFile.writeToFile(dir / (mapDir + "_" + tileID + "_lod.adt"));
		}
	}

	ADTLoader rootAdt(rootFile);
	rootAdt.loadRoot();

	ADTLoader texAdt(texFile);
	texAdt.loadTex(*wdt);

	ADTLoader objAdt(objFile);
	objAdt.loadObj();

	if (!isRawExport) {
		std::vector<float> vertices(16 * 16 * 145 * 3);
		std::vector<float> normals(16 * 16 * 145 * 3);
		std::vector<float> uvs(16 * 16 * 145 * 2);
		std::vector<float> uvsBake(16 * 16 * 145 * 2);
		std::vector<float> vertexColors(16 * 16 * 145 * 4);

		std::vector<std::vector<uint32_t>> chunkMeshes(256);

		const fs::path objOut = dir / ("adt_" + tileID + ".obj");
		out.path = objOut;

		OBJWriter obj(objOut);
		const fs::path mtlPath = dir / ("adt_" + tileID + ".mtl");
		MTLWriter mtl(mtlPath);

		const auto& firstChunk = rootAdt.chunks[0];
		const float firstChunkX = firstChunk.position[0];
		const float firstChunkY = firstChunk.position[1];

		const bool isAlphaMaps = quality == -1;
		const bool isLargeBake = quality >= 8192;
		const bool isSplittingAlphaMaps = isAlphaMaps && config.value("splitAlphaMaps", false);
		const bool isSplittingTextures = isLargeBake && config.value("splitLargeTerrainBakes", false);
		const bool includeHoles = config.value("mapsIncludeHoles", false);

		// Calculate UV bounds for single texture mode normalization
		UVBounds uvBounds;
		bool hasUVBounds = false;
		if (quality != 0 && !isSplittingTextures && !isSplittingAlphaMaps) {
			uvBounds = calculateUVBounds(rootAdt, firstChunkX, firstChunkY);
			hasUVBounds = true;
		}

		int ofs = 0;
		int chunkID = 0;
		for (int x = 0, midX = 0; x < 16; x++) {
			for (int y = 0; y < 16; y++) {
				std::vector<uint32_t> indices;

				const int chunkIndex = (x * 16) + y;
				const auto& chunk = rootAdt.chunks[chunkIndex];

				const float chunkX = chunk.position[0];
				const float chunkY = chunk.position[1];
				const float chunkZ = chunk.position[2];

				for (int row = 0, idx = 0; row < 17; row++) {
					const bool isShort = !!(row % 2);
					const int colCount = isShort ? 8 : 9;

					for (int col = 0; col < colCount; col++) {
						float vx = chunkY - (col * static_cast<float>(UNIT_SIZE));
						float vy = chunk.vertices[idx] + chunkZ;
						float vz = chunkX - (row * static_cast<float>(UNIT_SIZE_HALF));

						if (isShort)
							vx -= static_cast<float>(UNIT_SIZE_HALF);

						const int vIndex = midX * 3;
						vertices[vIndex + 0] = vx;
						vertices[vIndex + 1] = vy;
						vertices[vIndex + 2] = vz;

						const auto& normal = chunk.normals[idx];
						normals[vIndex + 0] = normal[0] / 127.0f;
						normals[vIndex + 1] = normal[1] / 127.0f;
						normals[vIndex + 2] = normal[2] / 127.0f;

						const int cIndex = midX * 4;
						if (!chunk.vertexShading.empty()) {
							// Store vertex shading in BGRA format.
							const auto& color = chunk.vertexShading[idx];
							vertexColors[cIndex + 0] = color.b / 255.0f;
							vertexColors[cIndex + 1] = color.g / 255.0f;
							vertexColors[cIndex + 2] = color.r / 255.0f;
							vertexColors[cIndex + 3] = color.a / 255.0f;
						} else {
							// No vertex shading, default to this.
							vertexColors[cIndex + 0] = 0.5f;
							vertexColors[cIndex + 1] = 0.5f;
							vertexColors[cIndex + 2] = 0.5f;
							vertexColors[cIndex + 3] = 1.0f;
						}

						const float uvIdx = isShort ? col + 0.5f : static_cast<float>(col);
						const int uvIndex = midX * 2;

						const float uRaw = -(vx - firstChunkX) / static_cast<float>(TILE_SIZE);
						const float vRaw = (vz - firstChunkY) / static_cast<float>(TILE_SIZE);

						uvsBake[uvIndex + 0] = uRaw;
						uvsBake[uvIndex + 1] = vRaw;

						if (quality == 0) {
							uvs[uvIndex + 0] = uvIdx / 8.0f;
							uvs[uvIndex + 1] = (row * 0.5f) / 8.0f;
						} else if (isSplittingTextures || isSplittingAlphaMaps) {
							uvs[uvIndex + 0] = uvIdx / 8.0f;
							uvs[uvIndex + 1] = 1.0f - (row / 16.0f);
						} else {
							// Single texture mode - apply normalization
							if (hasUVBounds) {
								uvs[uvIndex + 0] = (uRaw - uvBounds.minU) / (uvBounds.maxU - uvBounds.minU);
								uvs[uvIndex + 1] = (vRaw - uvBounds.minV) / (uvBounds.maxV - uvBounds.minV);
							} else {
								// Fallback to raw values if bounds calculation failed
								uvs[uvIndex + 0] = uRaw;
								uvs[uvIndex + 1] = vRaw;
							}
						}

						idx++;
						midX++;
					}
				}

				const auto& holesHighRes = chunk.holesHighRes;
				for (int j = 9, xx = 0, yy = 0; j < 145; j++, xx++) {
					if (xx >= 8) {
						xx = 0;
						yy++;
					}

					bool isHole = true;
					if (includeHoles) {
						if (!(chunk.flags & 0x10000)) {
							const int current = static_cast<int>(std::trunc(std::pow(2.0, (xx / 2) + (yy / 2) * 4)));

							if (!(chunk.holesLowRes & current))
								isHole = false;
						} else {
							if (!((holesHighRes[yy] >> xx) & 1))
								isHole = false;
						}
					} else {
						isHole = false;
					}

					if (!isHole) {
						const uint32_t indOfs = ofs + j;
						indices.push_back(indOfs);
						indices.push_back(indOfs - 9);
						indices.push_back(indOfs + 8);
						indices.push_back(indOfs);
						indices.push_back(indOfs - 8);
						indices.push_back(indOfs - 9);
						indices.push_back(indOfs);
						indices.push_back(indOfs + 9);
						indices.push_back(indOfs - 8);
						indices.push_back(indOfs);
						indices.push_back(indOfs + 8);
						indices.push_back(indOfs + 9);
					}

					if (!((j + 1) % (9 + 8)))
						j += 9;
				}

				ofs = midX;

				if (isSplittingTextures || isSplittingAlphaMaps) {
					const std::string objName = tileID + "_" + std::to_string(chunkID);
					const std::string matName = "tex_" + objName;
					mtl.addMaterial(matName, matName + ".png");
					obj.addMesh(objName, indices, matName);
				} else {
					obj.addMesh(std::to_string(chunkID), indices, "tex_" + tileID);
				}
				chunkMeshes[chunkIndex] = std::move(indices);

				chunkID++;
			}
		}

		if (quality != 0 && ((!isAlphaMaps && !isSplittingTextures) || (isAlphaMaps && !isSplittingAlphaMaps)))
			mtl.addMaterial("tex_" + tileID, "tex_" + tileID + ".png");

		obj.setVertArray(vertices);
		obj.setNormalArray(normals);
		obj.addUVArray(uvs);

		if (!mtl.isEmpty())
			obj.setMaterialLibrary(mtlPath.filename().string());

		obj.write(config.value("overwriteFiles", true));
		mtl.write(config.value("overwriteFiles", true));

		if (quality != 0) {
			if (isAlphaMaps) {
				// Export alpha maps.

				const auto& materialIDs = texAdt.diffuseTextureFileDataIDs;
				const auto& heightIDs = texAdt.heightTextureFileDataIDs;
				const auto& texParams = texAdt.texParams;

				auto saveLayerTexture = [&](uint32_t fileDataID) -> std::string {
					auto blpData = casc->getVirtualFileByID(fileDataID);
					casc::BLPImage blp(std::move(blpData));
					std::string fileName = casc::listfile::getByID(fileDataID).value_or("");
					if (!fileName.empty())
						fileName = casc::ExportHelper::replaceExtension(fileName, ".png");
					else
						fileName = casc::listfile::formatUnknownFile(fileDataID, ".png");

					std::string texFileStr;
					fs::path texPath;

					if (config.value("enableSharedTextures", false)) {
						texPath = casc::ExportHelper::getExportPath(fileName);
						texFileStr = fs::relative(texPath, dir).string();
					} else {
						texPath = dir / fs::path(fileName).filename();
						texFileStr = fs::path(fileName).filename().string();
					}

					blp.saveToPNG(texPath);

					return usePosix ? casc::ExportHelper::win32ToPosix(texFileStr) : texFileStr;
				};

				struct AlphaMapMaterial {
					float scale = 1.0f;
					uint32_t fileDataID = 0;
					std::string file;
					std::string heightFile;
					uint32_t heightFileDataID = 0;
					float heightScale = 0.0f;
					float heightOffset = 0.0f;
					bool hasHeightScale = false;
					bool valid = false;
				};

				// Export the raw diffuse textures to disk.
				std::vector<AlphaMapMaterial> materials(materialIDs.size());
				for (size_t i = 0, n = materials.size(); i < n; i++) {
					// Abort if the export has been cancelled.
					if (helper->isCancelled())
						return out;

					const uint32_t diffuseFileDataID = materialIDs[i];
					const uint32_t heightFileDataID = (i < heightIDs.size()) ? heightIDs[i] : 0;
					if (diffuseFileDataID == 0)
						continue;

					auto& mat = materials[i];
					mat.valid = true;
					mat.scale = 1.0f;
					mat.fileDataID = diffuseFileDataID;
					mat.file = saveLayerTexture(diffuseFileDataID);

					// Include a reference to the height map texture if it exists.
					if (heightFileDataID > 0) {
						mat.heightFile = saveLayerTexture(heightFileDataID);
						mat.heightFileDataID = heightFileDataID;
					}

					if (i < texParams.size()) {
						const auto& params = texParams[i];
						mat.scale = static_cast<float>(std::pow(2, (params.flags & 0xF0) >> 4));

						if (params.height != 0 || params.offset != 1) {
							mat.heightScale = params.height;
							mat.heightOffset = params.offset;
							mat.hasHeightScale = true;
						}
					}
				}

				// Alpha maps are 64x64, we're not up-scaling here.

				const auto& chunks = texAdt.texChunks;
				const size_t chunkCount = chunks.size();

				helper->setCurrentTaskName("Tile " + tileID + " alpha maps");
				helper->setCurrentTaskMax(16 * 16);

				nlohmann::json layers = nlohmann::json::array();
				nlohmann::json vertexColorsJson = nlohmann::json::array();

				for (size_t chunkIndex = 0; chunkIndex < chunkCount; chunkIndex++) {
					// Abort if the export has been cancelled.
					if (helper->isCancelled())
						return out;

					helper->setCurrentTaskValue(static_cast<int>(chunkIndex));

					const auto& texChunk = texAdt.texChunks[chunkIndex];
					const auto& rootChunk = rootAdt.chunks[chunkIndex];

					const bool fix_alpha_map = !(rootChunk.flags & (1 << 15));

					const auto& alphaLayers = texChunk.alphaLayers;
					const int requiredImages = calculateRequiredImages(static_cast<int>(alphaLayers.size()));

					if (isSplittingAlphaMaps) {
						// Export individual chunk files with multi-image support
						const std::string chunkPrefix = tileID + "_" + std::to_string(chunkIndex);
						const auto& texLayers = texChunk.layers;

						for (int imageIndex = 0; imageIndex < std::max(1, requiredImages); imageIndex++) {
							PNGWriter pngWriter(64, 64);
							auto& pixelData = pngWriter.getPixelData();

							// Set unused channels to 0 and alpha channel to 255 if not used for data first
							for (int j = 0; j < 64 * 64; j++) {
								const int pixelOffset = j * 4;
								pixelData[pixelOffset + 0] = 0;   // R = 0
								pixelData[pixelOffset + 1] = 0;   // G = 0
								pixelData[pixelOffset + 2] = 0;   // B = 0
								pixelData[pixelOffset + 3] = 255; // A = 255
							}

							// Write layers to this image (4 layers per image starting from layer 1)
							const int startLayer = (imageIndex * 4) + 1;
							const int endLayer = std::min(startLayer + 4, static_cast<int>(alphaLayers.size()));

							for (int layerIdx = startLayer; layerIdx < endLayer; layerIdx++) {
								const auto& layer = alphaLayers[layerIdx];
								const int channelIdx = (layerIdx - startLayer); // 0, 1, 2, 3 for R, G, B, A

								for (size_t j = 0; j < layer.size(); j++) {
									const bool isLastColumn = (j % 64) == 63;
									const bool isLastRow = j >= 63 * 64;

									// fix_alpha_map: layer is 63x63, fill last column/row.
									if (fix_alpha_map) {
										if (isLastColumn && !isLastRow) {
											pixelData[(j * 4) + channelIdx] = layer[j - 1];
										} else if (isLastRow) {
											const size_t prevRowIndex = j - 64;
											pixelData[(j * 4) + channelIdx] = layer[prevRowIndex];
										} else {
											pixelData[(j * 4) + channelIdx] = layer[j];
										}
									} else {
										pixelData[(j * 4) + channelIdx] = layer[j];
									}
								}
							}

							// determine file name: first image keeps original naming, additional get suffix
							const std::string imageSuffix = imageIndex == 0 ? "" : "_" + std::to_string(imageIndex);
							const fs::path tilePath = dir / ("tex_" + chunkPrefix + imageSuffix + ".png");

							pngWriter.write(tilePath).get();
						}

						// Create JSON metadata with image/channel mapping
						for (size_t i = 0, n = texLayers.size(); i < n; i++) {
							const auto& layer = texLayers[i];
							if (layer.textureId < materials.size() && materials[layer.textureId].valid) {
								const auto& mat = materials[layer.textureId];
								nlohmann::json layerInfo;
								layerInfo["index"] = i;
								layerInfo["effectID"] = layer.effectID;
								layerInfo["imageIndex"] = (i == 0) ? 0 : static_cast<int>((i - 1) / 4);
								layerInfo["channelIndex"] = (i == 0) ? -1 : static_cast<int>((i - 1) % 4);
								layerInfo["scale"] = mat.scale;
								layerInfo["fileDataID"] = mat.fileDataID;
								layerInfo["file"] = mat.file;
								if (!mat.heightFile.empty()) {
									layerInfo["heightFile"] = mat.heightFile;
									layerInfo["heightFileDataID"] = mat.heightFileDataID;
								}
								if (mat.hasHeightScale) {
									layerInfo["heightScale"] = mat.heightScale;
									layerInfo["heightOffset"] = mat.heightOffset;
								}
								layers.push_back(layerInfo);
							}
						}

						JSONWriter json(dir / ("tex_" + chunkPrefix + ".json"));
						json.addProperty("layers", layers);

						if (!rootChunk.vertexShading.empty()) {
							nlohmann::json shadingArr = nlohmann::json::array();
							for (const auto& e : rootChunk.vertexShading)
								shadingArr.push_back(rgbaToInt(e));
							json.addProperty("vertexColors", shadingArr);
						}

						json.write();

						layers = nlohmann::json::array(); // Reset for next chunk
					} else {
						// Combined alpha maps - metadata collection for combined export
						const auto& texLayers = texChunk.layers;
						for (size_t i = 0, n = texLayers.size(); i < n; i++) {
							const auto& layer = texLayers[i];
							if (layer.textureId < materials.size() && materials[layer.textureId].valid) {
								const auto& mat = materials[layer.textureId];
								nlohmann::json layerInfo;
								layerInfo["index"] = i;
								layerInfo["chunkIndex"] = chunkIndex;
								layerInfo["effectID"] = layer.effectID;
								layerInfo["imageIndex"] = (i == 0) ? 0 : static_cast<int>((i - 1) / 4);
								layerInfo["channelIndex"] = (i == 0) ? -1 : static_cast<int>((i - 1) % 4);
								layerInfo["scale"] = mat.scale;
								layerInfo["fileDataID"] = mat.fileDataID;
								layerInfo["file"] = mat.file;
								if (!mat.heightFile.empty()) {
									layerInfo["heightFile"] = mat.heightFile;
									layerInfo["heightFileDataID"] = mat.heightFileDataID;
								}
								if (mat.hasHeightScale) {
									layerInfo["heightScale"] = mat.heightScale;
									layerInfo["heightOffset"] = mat.heightOffset;
								}
								layers.push_back(layerInfo);
							}
						}

						if (!rootChunk.vertexShading.empty()) {
							nlohmann::json shadingEntry;
							shadingEntry["chunkIndex"] = chunkIndex;
							nlohmann::json shadingArr = nlohmann::json::array();
							for (const auto& e : rootChunk.vertexShading)
								shadingArr.push_back(rgbaToInt(e));
							shadingEntry["shading"] = shadingArr;
							vertexColorsJson.push_back(shadingEntry);
						}
					}
				}

				// For combined alpha maps, export everything together once done.
				if (!isSplittingAlphaMaps) {
					// determine max layers across all chunks to know how many images we need
					int maxLayersNeeded = 1;
					for (size_t chunkIndex2 = 0; chunkIndex2 < chunkCount; chunkIndex2++) {
						const auto& texChunk = texAdt.texChunks[chunkIndex2];
						const auto& alphaLayers2 = texChunk.alphaLayers;
						const int required = calculateRequiredImages(static_cast<int>(alphaLayers2.size()));
						maxLayersNeeded = std::max(maxLayersNeeded, required);
					}

					// export multiple combined images if needed
					for (int imageIndex = 0; imageIndex < maxLayersNeeded; imageIndex++) {
						PNGWriter pngWriter(64 * 16, 64 * 16);
						auto& pixelData = pngWriter.getPixelData();

						// Initialize all pixels to default values
						for (size_t i = 0; i < pixelData.size(); i += 4) {
							pixelData[i + 0] = 0;   // R = 0
							pixelData[i + 1] = 0;   // G = 0
							pixelData[i + 2] = 0;   // B = 0
							pixelData[i + 3] = 255; // A = 255
						}

						// process all chunks for this image
						for (size_t chunkIndex2 = 0; chunkIndex2 < chunkCount; chunkIndex2++) {
							const auto& texChunk = texAdt.texChunks[chunkIndex2];
							const auto& rootChunk = rootAdt.chunks[chunkIndex2];
							const bool fix_alpha_map2 = !(rootChunk.flags & (1 << 15));
							const auto& alphaLayers2 = texChunk.alphaLayers;

							const int cx = static_cast<int>(chunkIndex2 % 16);
							const int cy = static_cast<int>(chunkIndex2 / 16);

							// Write layers to this image (4 layers per image starting from layer 1)
							const int startLayer = (imageIndex * 4) + 1;
							const int endLayer = std::min(startLayer + 4, static_cast<int>(alphaLayers2.size()));

							for (int layerIdx = startLayer; layerIdx < endLayer; layerIdx++) {
								const auto& layer = alphaLayers2[layerIdx];
								const int channelIdx = (layerIdx - startLayer); // 0, 1, 2, 3 for R, G, B, A

								for (size_t j = 0; j < layer.size(); j++) {
									const bool isLastColumn = (j % 64) == 63;
									const bool isLastRow = j >= 63 * 64;

									// Calculate position in combined image
									const int localX = static_cast<int>(j % 64);
									const int localY = static_cast<int>(j / 64);
									const int globalX = cx * 64 + localX;
									const int globalY = cy * 64 + localY;
									const int globalIndex = (globalY * (64 * 16) + globalX) * 4 + channelIdx;

									// fix_alpha_map: layer is 63x63, fill last column/row.
									if (fix_alpha_map2) {
										if (isLastColumn && !isLastRow) {
											pixelData[globalIndex] = layer[j - 1];
										} else if (isLastRow) {
											const size_t prevRowIndex = j - 64;
											pixelData[globalIndex] = layer[prevRowIndex];
										} else {
											pixelData[globalIndex] = layer[j];
										}
									} else {
										pixelData[globalIndex] = layer[j];
									}
								}
							}
						}

						// save the combined image
						const std::string imageSuffix = imageIndex == 0 ? "" : "_" + std::to_string(imageIndex);
						const fs::path mergedPath = dir / ("tex_" + tileID + imageSuffix + ".png");
						pngWriter.write(mergedPath).get();
					}

					// write json metadata
					JSONWriter json(dir / ("tex_" + tileID + ".json"));
					json.addProperty("layers", layers);

					if (!vertexColorsJson.empty())
						json.addProperty("vertexColors", vertexColorsJson);

					json.write();
				}
			} else if (quality <= 512) {
				// Use minimaps for cheap textures.
				const std::string paddedX = std::format("{:02d}", tileY);
				const std::string paddedY = std::format("{:02d}", tileX);
				const std::string tilePath = std::format("world/minimaps/{}/map{}_{}.blp", mapDir, paddedX, paddedY);
				const fs::path tileOutPath = dir / ("tex_" + tileID + ".png");

				if (config.value("overwriteFiles", true) || !generics::fileExists(tileOutPath)) {
					auto data = casc->getVirtualFileByName(tilePath, true);
					casc::BLPImage blp(std::move(data));

					auto raw_pixels = blp.toUInt8Array(0, 0b0111);
					const int src_w = static_cast<int>(blp.getScaledWidth());
					const int src_h = static_cast<int>(blp.getScaledHeight());

					if (src_w != quality || src_h != quality) {
						std::vector<uint8_t> resized(quality * quality * 4);
							stbir_resize_uint8_linear(
							raw_pixels.data(), src_w, src_h, src_w * 4,
							resized.data(), quality, quality, quality * 4,
							STBIR_RGBA);
						PNGWriter png(static_cast<uint32_t>(quality), static_cast<uint32_t>(quality));
						auto& pd = png.getPixelData();
						std::memcpy(pd.data(), resized.data(), resized.size());
						png.write(tileOutPath).get();
					} else {
						blp.saveToPNG(tileOutPath, 0b0111);
					}
				} else {
					logging::write(std::format("Skipping ADT bake of {} (file exists, overwrite disabled)", tileOutPath.string()));
				}
			} else {
				const bool hasHeightTexturing = (wdt->flags & 0x80) == 0x80;
				const fs::path tileOutPath = dir / ("tex_" + tileID + ".png");

				std::vector<uint8_t> compositeBuffer;
				int compositeSize = 0;
				if (!isSplittingTextures) {
					compositeSize = quality;
					compositeBuffer.resize(quality * quality * 4, 0);
				}

				if (isSplittingTextures || config.value("overwriteFiles", true) || !generics::fileExists(tileOutPath)) {
					// Create new GL context and compile shaders.
					if (!glInitialized) {
						compileShaders(!hasHeightTexturing);
						glInitialized = true;
					}

					// Materials
					const auto& materialIDs2 = texAdt.diffuseTextureFileDataIDs;
					const auto& heightIDs2 = texAdt.heightTextureFileDataIDs;
					const auto& texParams2 = texAdt.texParams;

					helper->setCurrentTaskName("Tile " + tileID + ", building texture arrays");

					std::vector<uint32_t> unique_diffuse_ids;
					{
						std::unordered_set<uint32_t> seen;
						for (const auto id : materialIDs2) {
							if (id != 0 && seen.insert(id).second)
								unique_diffuse_ids.push_back(id);
						}
					}

					std::vector<uint32_t> unique_height_ids;
					{
						std::unordered_set<uint32_t> seen;
						for (const auto id : heightIDs2) {
							if (id != 0 && seen.insert(id).second)
								unique_height_ids.push_back(id);
						}
					}

					GLuint diffuse_array = build_texture_array(unique_diffuse_ids, false, casc);
					GLuint height_array = build_texture_array(!unique_height_ids.empty() ? unique_height_ids : unique_diffuse_ids, true, casc);

					// map file data id -> array index
					std::unordered_map<uint32_t, int> diffuse_id_to_index;
					for (size_t idx2 = 0; idx2 < unique_diffuse_ids.size(); idx2++)
						diffuse_id_to_index[unique_diffuse_ids[idx2]] = static_cast<int>(idx2);

					std::unordered_map<uint32_t, int> height_id_to_index;
					const auto& heightSource = !unique_height_ids.empty() ? unique_height_ids : unique_diffuse_ids;
					for (size_t idx2 = 0; idx2 < heightSource.size(); idx2++)
						height_id_to_index[heightSource[idx2]] = static_cast<int>(idx2);

					// build material metadata
					struct BakeMaterial {
						float scale = 1.0f;
						float heightScale = 0.0f;
						float heightOffset = 1.0f;
						int diffuseIndex = 0;
						int heightIndex = 0;
						bool valid = false;
					};

					std::vector<BakeMaterial> bakeMaterials(materialIDs2.size());
					for (size_t i = 0, n = materialIDs2.size(); i < n; i++) {
						if (materialIDs2[i] == 0)
							continue;

						auto& mat = bakeMaterials[i];
						mat.valid = true;
						mat.scale = 1.0f;
						mat.heightScale = 0.0f;
						mat.heightOffset = 1.0f;

						auto dit = diffuse_id_to_index.find(materialIDs2[i]);
						mat.diffuseIndex = (dit != diffuse_id_to_index.end()) ? dit->second : 0;

						if (i < heightIDs2.size() && heightIDs2[i] != 0) {
							auto hit = height_id_to_index.find(heightIDs2[i]);
							mat.heightIndex = (hit != height_id_to_index.end()) ? hit->second : 0;
						} else {
							mat.heightIndex = mat.diffuseIndex;
						}

						if (i < texParams2.size()) {
							const auto& params = texParams2[i];
							mat.scale = static_cast<float>(std::pow(2, (params.flags & 0xF0) >> 4));

							if (params.height != 0 || params.offset != 1) {
								mat.heightScale = params.height;
								mat.heightOffset = params.offset;
							}
						}
					}

					const GLint aVertexPosition = glGetAttribLocation(glShaderProg, "aVertexPosition");
					const GLint aTexCoord = glGetAttribLocation(glShaderProg, "aTextureCoord");
					const GLint aVertexColor = glGetAttribLocation(glShaderProg, "aVertexColor");

					const GLint uDiffuseLayers = glGetUniformLocation(glShaderProg, "uDiffuseLayers");
					const GLint uHeightLayers = glGetUniformLocation(glShaderProg, "uHeightLayers");
					const GLint uLayerCount = glGetUniformLocation(glShaderProg, "uLayerCount");

					GLint uAlphaBlends[7];
					for (int i = 0; i < 7; i++)
						uAlphaBlends[i] = glGetUniformLocation(glShaderProg, ("uAlphaBlend" + std::to_string(i)).c_str());

					const GLint uTranslation = glGetUniformLocation(glShaderProg, "uTranslation");
					const GLint uResolution = glGetUniformLocation(glShaderProg, "uResolution");
					const GLint uZoom = glGetUniformLocation(glShaderProg, "uZoom");

					const int tileSize = quality / 16;
					initFBO(tileSize, tileSize);

					clearCanvas();

					glUniform2f(uResolution, static_cast<float>(TILE_SIZE), static_cast<float>(TILE_SIZE));

					GLuint vertexBuffer;
					glGenBuffers(1, &vertexBuffer);
					glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
					glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
					glEnableVertexAttribArray(aVertexPosition);
					glVertexAttribPointer(aVertexPosition, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

					GLuint uvBuffer;
					glGenBuffers(1, &uvBuffer);
					glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
					glBufferData(GL_ARRAY_BUFFER, uvsBake.size() * sizeof(float), uvsBake.data(), GL_STATIC_DRAW);
					glEnableVertexAttribArray(aTexCoord);
					glVertexAttribPointer(aTexCoord, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

					GLuint vcBuffer;
					glGenBuffers(1, &vcBuffer);
					glBindBuffer(GL_ARRAY_BUFFER, vcBuffer);
					glBufferData(GL_ARRAY_BUFFER, vertexColors.size() * sizeof(float), vertexColors.data(), GL_STATIC_DRAW);
					glEnableVertexAttribArray(aVertexColor);
					glVertexAttribPointer(aVertexColor, 4, GL_FLOAT, GL_FALSE, 0, nullptr);

					const auto& firstChunk2 = rootAdt.chunks[0];
					const float deltaX = firstChunk2.position[1] - static_cast<float>(TILE_SIZE);
					const float deltaY = firstChunk2.position[0] - static_cast<float>(TILE_SIZE);

					glUniform1f(uZoom, 0.0625f);

					unbindAllTextures();

					helper->setCurrentTaskName("Tile " + tileID + ", baking textures");
					helper->setCurrentTaskMax(16 * 16);

					int bakeChunkID = 0;
					for (int bx = 0; bx < 16; bx++) {
						for (int by = 0; by < 16; by++) {
							// Abort if the export has been cancelled.
							if (helper->isCancelled())
								return out;

							helper->setCurrentTaskValue(bakeChunkID);

							const float ofsX = -deltaX - (static_cast<float>(CHUNK_SIZE) * 7.5f) + (by * static_cast<float>(CHUNK_SIZE));
							const float ofsY = -deltaY - (static_cast<float>(CHUNK_SIZE) * 7.5f) + (bx * static_cast<float>(CHUNK_SIZE));

							glUniform2f(uTranslation, ofsX, ofsY);

							const int bakeChunkIndex = (bx * 16) + by;
							const auto& texChunk = texAdt.texChunks[bakeChunkIndex];
							const auto& bakeIndices = chunkMeshes[bakeChunkIndex];
							const auto& texLayers = texChunk.layers;

							const int chunk_layer_count = std::min(static_cast<int>(texLayers.size()), 8);
							glUniform1i(uLayerCount, chunk_layer_count);

							// clear all texture bindings before setting up new ones
							unbindAllTextures();

							// rebind texture arrays
							glActiveTexture(GL_TEXTURE0);
							glBindTexture(GL_TEXTURE_2D_ARRAY, diffuse_array);
							glUniform1i(uDiffuseLayers, 0);

							glActiveTexture(GL_TEXTURE1);
							glBindTexture(GL_TEXTURE_2D_ARRAY, height_array);
							glUniform1i(uHeightLayers, 1);

							// bind alpha layers (units 2-8)
							const auto& bakeAlphaLayers = texChunk.alphaLayers;
							GLuint alphaTextures[8] = {0};

							for (int ai = 1; ai < std::min(static_cast<int>(bakeAlphaLayers.size()), 8); ai++) {
								glActiveTexture(GL_TEXTURE0 + 2 + (ai - 1));
								GLuint alphaTex = bindAlphaLayer(bakeAlphaLayers[ai]);
								glBindTexture(GL_TEXTURE_2D, alphaTex);
								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
								alphaTextures[ai - 1] = alphaTex;
							}

							// set alpha blend uniforms for all 7 possible layers
							for (int ai = 0; ai < 7; ai++)
								glUniform1i(uAlphaBlends[ai], 2 + ai);

							// set per-layer uniforms using chunk layer info
							float layer_scales[8] = {1,1,1,1,1,1,1,1};
							float height_scales[8] = {0,0,0,0,0,0,0,0};
							float height_offsets[8] = {1,1,1,1,1,1,1,1};
							float diffuse_indices[8] = {0,0,0,0,0,0,0,0};
							float height_indices[8] = {0,0,0,0,0,0,0,0};

							for (int li = 0; li < chunk_layer_count; li++) {
								const auto texId = texLayers[li].textureId;
								if (texId >= bakeMaterials.size() || !bakeMaterials[texId].valid)
									continue;

								const auto& mat = bakeMaterials[texId];
								layer_scales[li] = mat.scale;
								height_scales[li] = mat.heightScale;
								height_offsets[li] = mat.heightOffset;
								diffuse_indices[li] = static_cast<float>(mat.diffuseIndex);
								height_indices[li] = static_cast<float>(mat.heightIndex);
							}

							for (int ui = 0; ui < 8; ui++) {
								GLint loc = glGetUniformLocation(glShaderProg, ("uLayerScales[" + std::to_string(ui) + "]").c_str());
								glUniform1f(loc, layer_scales[ui]);
							}

							for (int ui = 0; ui < 8; ui++) {
								GLint loc = glGetUniformLocation(glShaderProg, ("uHeightScales[" + std::to_string(ui) + "]").c_str());
								glUniform1f(loc, height_scales[ui]);
							}

							for (int ui = 0; ui < 8; ui++) {
								GLint loc = glGetUniformLocation(glShaderProg, ("uHeightOffsets[" + std::to_string(ui) + "]").c_str());
								glUniform1f(loc, height_offsets[ui]);
							}

							for (int ui = 0; ui < 8; ui++) {
								GLint loc = glGetUniformLocation(glShaderProg, ("uDiffuseIndices[" + std::to_string(ui) + "]").c_str());
								glUniform1f(loc, diffuse_indices[ui]);
							}

							for (int ui = 0; ui < 8; ui++) {
								GLint loc = glGetUniformLocation(glShaderProg, ("uHeightIndices[" + std::to_string(ui) + "]").c_str());
								glUniform1f(loc, height_indices[ui]);
							}

							GLuint indexBuffer;
							glGenBuffers(1, &indexBuffer);
							glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
							std::vector<uint16_t> bakeIndices16(bakeIndices.begin(), bakeIndices.end());
							glBufferData(GL_ELEMENT_ARRAY_BUFFER, bakeIndices16.size() * sizeof(uint16_t), bakeIndices16.data(), GL_STATIC_DRAW);
							glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(bakeIndices16.size()), GL_UNSIGNED_SHORT, nullptr);

							// cleanup alpha textures
							for (const auto tex : alphaTextures) {
								if (tex)
									glDeleteTextures(1, &tex);
							}

							glDeleteBuffers(1, &indexBuffer);

							auto fboPixels = readFBOPixels(tileSize, tileSize);

							if (isSplittingTextures) {
								// Save this individual chunk.
								const fs::path chunkTilePath = dir / ("tex_" + tileID + "_" + std::to_string(bakeChunkID++) + ".png");

								if (config.value("overwriteFiles", true) || !generics::fileExists(chunkTilePath)) {
									PNGWriter chunkPng(tileSize, tileSize);
									auto& chunkPixelData = chunkPng.getPixelData();
									for (int py = 0; py < tileSize; py++) {
										for (int px = 0; px < tileSize; px++) {
											const int srcIdx = (py * tileSize + px) * 4;
											const int dstIdx = ((tileSize - 1 - py) * tileSize + (tileSize - 1 - px)) * 4;
											chunkPixelData[dstIdx + 0] = fboPixels[srcIdx + 0];
											chunkPixelData[dstIdx + 1] = fboPixels[srcIdx + 1];
											chunkPixelData[dstIdx + 2] = fboPixels[srcIdx + 2];
											chunkPixelData[dstIdx + 3] = fboPixels[srcIdx + 3];
										}
									}
									chunkPng.write(chunkTilePath).get();
								}
							} else {
								// Store as part of a larger composite.
								const int cxPos = bakeChunkIndex % 16;
								const int cyPos = bakeChunkIndex / 16;

								for (int py = 0; py < tileSize; py++) {
									for (int px = 0; px < tileSize; px++) {
										const int srcIdx = (py * tileSize + px) * 4;
										const int rotPx = tileSize - 1 - px;
										const int rotPy = tileSize - 1 - py;
										const int dstX = cxPos * tileSize + rotPx;
										const int dstY = cyPos * tileSize + rotPy;
										const int dstIdx = (dstY * compositeSize + dstX) * 4;
										compositeBuffer[dstIdx + 0] = fboPixels[srcIdx + 0];
										compositeBuffer[dstIdx + 1] = fboPixels[srcIdx + 1];
										compositeBuffer[dstIdx + 2] = fboPixels[srcIdx + 2];
										compositeBuffer[dstIdx + 3] = fboPixels[srcIdx + 3];
									}
								}
							}
						}
					}

					// cleanup texture arrays
					glDeleteTextures(1, &diffuse_array);
					glDeleteTextures(1, &height_array);

					// Save the completed composite tile.
					if (!isSplittingTextures) {
						PNGWriter compositePng(compositeSize, compositeSize);
						auto& compositePx = compositePng.getPixelData();
						std::copy(compositeBuffer.begin(), compositeBuffer.end(), compositePx.begin());
						compositePng.write(dir / ("tex_" + tileID + ".png")).get();
					}

					// Clear buffers.
					glDeleteBuffers(1, &vertexBuffer);
					glDeleteBuffers(1, &uvBuffer);
					glDeleteBuffers(1, &vcBuffer);
					glBindBuffer(GL_ARRAY_BUFFER, 0);
				}
			}
		}
	} else {
		// Raw export - save raw layer textures.
		auto saveRawLayerTexture = [&](uint32_t fileDataID) {
			if (fileDataID == 0)
				return;

			auto blpData = casc->getVirtualFileByID(fileDataID);

			std::string fileName = casc::listfile::getByID(fileDataID).value_or("");
			if (!fileName.empty())
				fileName = casc::ExportHelper::replaceExtension(fileName, ".blp");
			else
				fileName = casc::listfile::formatUnknownFile(fileDataID, ".blp");

			fs::path texPath;

			if (config.value("enableSharedTextures", false)) {
				texPath = casc::ExportHelper::getExportPath(fileName);
			} else {
				texPath = dir / fs::path(fileName).filename();
			}

			blpData.writeToFile(texPath);
		};

		const auto& materialIDs = texAdt.diffuseTextureFileDataIDs;
		for (const auto fileDataID : materialIDs)
			saveRawLayerTexture(fileDataID);

		const auto& heightIDs = texAdt.heightTextureFileDataIDs;
		for (const auto fileDataID : heightIDs)
			saveRawLayerTexture(fileDataID);
	}

	// Export doodads / WMOs.
	if (config.value("mapsIncludeWMO", false) || config.value("mapsIncludeM2", false) || config.value("mapsIncludeGameObjects", false)) {
		std::unordered_set<std::string> objectCache;

		const fs::path csvPath = dir / ("adt_" + tileID + "_ModelPlacementInformation.csv");
		if (config.value("overwriteFiles", true) || !generics::fileExists(csvPath)) {
			CSVWriter csv(csvPath);
			csv.addField({"ModelFile", "PositionX", "PositionY", "PositionZ", "RotationX", "RotationY", "RotationZ", "RotationW", "ScaleFactor", "ModelId", "Type", "FileDataID", "DoodadSetIndexes", "DoodadSetNames"});

			auto exportObjects = [&](const std::string& exportType, const auto& objects, const std::string& csvName) {
				const size_t nObjects = objects.size();
				logging::write(std::format("Exporting {} {} for ADT...", nObjects, exportType));

				helper->setCurrentTaskName("Tile " + tileID + ", " + exportType);
				helper->setCurrentTaskMax(static_cast<int>(nObjects));

				int index = 0;
				for (const auto& model : objects) {
					helper->setCurrentTaskValue(index++);

					uint32_t fileDataID = 0;
					if constexpr (std::is_same_v<std::decay_t<decltype(model)>, ADTGameObject>) {
						fileDataID = model.FileDataID;
					} else {
						fileDataID = model.mmidEntry;
					}

					std::string fileName = casc::listfile::getByID(fileDataID).value_or("");

					if (!isRawExport) {
						if (!fileName.empty()) {
							// Replace M2 extension with OBJ.
							fileName = casc::ExportHelper::replaceExtension(fileName, ".obj");
						} else {
							// Handle unknown file.
							fileName = casc::listfile::formatUnknownFile(fileDataID, ".obj");
						}
					}

					std::string modelPath;
					if (config.value("enableSharedChildren", false))
						modelPath = casc::ExportHelper::getExportPath(fileName);
					else
						modelPath = (dir / fs::path(fileName).filename()).string();

					try {
						const std::string cacheKey = std::to_string(fileDataID);
						if (objectCache.find(cacheKey) == objectCache.end()) {
							auto data = casc->getVirtualFileByID(fileDataID);
							M2Exporter m2(std::move(data), {}, fileDataID, casc);

							if (isRawExport)
								m2.exportRaw(modelPath, helper, nullptr);
							else
								m2.exportAsOBJ(modelPath, config.value("modelsExportCollision", false), helper, nullptr);

							// Abort if the export has been cancelled.
							if (helper->isCancelled())
								return;

							objectCache.insert(cacheKey);
						}

						std::string modelFile = fs::relative(modelPath, dir).string();
						if (usePosix)
							modelFile = casc::ExportHelper::win32ToPosix(modelFile);

						// Get position/rotation/scale.
						float posX = 0, posY = 0, posZ = 0;
						float rotX = 0, rotY = 0, rotZ = 0, rotW = 0;
						float scaleFactor = 1.0f;
						uint32_t modelId = 0;

						if constexpr (std::is_same_v<std::decay_t<decltype(model)>, ADTGameObject>) {
							if (model.Position.size() >= 3) {
								posX = model.Position[0]; posY = model.Position[1]; posZ = model.Position[2];
							}
							if (model.Rotation.size() >= 4) {
								rotX = model.Rotation[0]; rotY = model.Rotation[1]; rotZ = model.Rotation[2]; rotW = model.Rotation[3];
							}
							scaleFactor = model.scale != 0.0f ? model.scale / 1024.0f : 1.0f;
							modelId = model.uniqueId;
						} else {
							if (model.position.size() >= 3) {
								posX = model.position[0]; posY = model.position[1]; posZ = model.position[2];
							}
							if (model.rotation.size() >= 4) {
								rotX = model.rotation[0]; rotY = model.rotation[1]; rotZ = model.rotation[2]; rotW = model.rotation[3];
							}
							scaleFactor = static_cast<float>(model.scale) / 1024.0f;
							modelId = model.uniqueId;
						}

						csv.addRow({
							{ "ModelFile", modelFile },
							{ "PositionX", std::to_string(posX) },
							{ "PositionY", std::to_string(posY) },
							{ "PositionZ", std::to_string(posZ) },
							{ "RotationX", std::to_string(rotX) },
							{ "RotationY", std::to_string(rotY) },
							{ "RotationZ", std::to_string(rotZ) },
							{ "RotationW", std::to_string(rotW) },
							{ "ScaleFactor", std::to_string(scaleFactor) },
							{ "ModelId", std::to_string(modelId) },
							{ "Type", csvName },
							{ "FileDataID", std::to_string(fileDataID) },
							{ "DoodadSetIndexes", "0" },
							{ "DoodadSetNames", "" }
						});
					} catch (const std::exception& e) {
						logging::write(std::format("Failed to export {} [{}]", fileName, fileDataID));
						logging::write(std::format("Error: {}", e.what()));
					}
				}
			};

			if (config.value("mapsIncludeGameObjects", false) && gameObjects != nullptr && !gameObjects->empty())
				exportObjects("game objects", *gameObjects, "gobj");

			if (config.value("mapsIncludeM2", false))
				exportObjects("doodads", objAdt.models, "m2");

			if (config.value("mapsIncludeWMO", false)) {
				logging::write(std::format("Exporting {} WMOs for ADT...", objAdt.worldModels.size()));

				helper->setCurrentTaskName("Tile " + tileID + ", WMO objects");
				helper->setCurrentTaskMax(static_cast<int>(objAdt.worldModels.size()));

				std::unordered_map<uint32_t, std::vector<std::string>> setNameCache;

				int worldModelIndex = 0;
				const bool usingNames = !objAdt.wmoNames.empty();
				for (const auto& model : objAdt.worldModels) {
					const bool useADTSets = false;
					helper->setCurrentTaskValue(worldModelIndex++);

					uint32_t fileDataID = 0;
					std::string fileName;

					try {
						if (usingNames) {
							auto it = objAdt.wmoNames.find(objAdt.wmoOffsets[model.mwidEntry]);
							if (it != objAdt.wmoNames.end())
								fileName = it->second;
							fileDataID = casc::listfile::getByFilename(fileName).value_or(0);
						} else {
							fileDataID = model.mwidEntry;
							fileName = casc::listfile::getByID(fileDataID).value_or("");
						}

						if (!isRawExport) {
							if (!fileName.empty()) {
								// Replace WMO extension with OBJ.
								fileName = casc::ExportHelper::replaceExtension(fileName, "_set" + std::to_string(model.doodadSet) + ".obj");
							} else {
								// Handle unknown WMO files.
								fileName = casc::listfile::formatUnknownFile(fileDataID, "_set" + std::to_string(model.doodadSet) + ".obj");
							}
						}

						std::string modelPath;
						if (config.value("enableSharedChildren", false))
							modelPath = casc::ExportHelper::getExportPath(fileName);
						else
							modelPath = (dir / fs::path(fileName).filename()).string();

						std::vector<uint16_t> doodadSets;
						if (useADTSets)
							doodadSets = objAdt.doodadSets;
						else
							doodadSets = { model.doodadSet };

						std::string doodadSetsStr;
						for (size_t i = 0; i < doodadSets.size(); i++) {
							if (i > 0) doodadSetsStr += ",";
							doodadSetsStr += std::to_string(doodadSets[i]);
						}
						const std::string cacheID = std::to_string(fileDataID) + "-" + doodadSetsStr;

						if (objectCache.find(cacheID) == objectCache.end()) {
							auto data = casc->getVirtualFileByID(fileDataID);
							WMOExporter wmoLoader(std::move(data), fileDataID, casc);

							wmoLoader.loadWMO();

							setNameCache[fileDataID] = wmoLoader.getDoodadSetNames();

							if (config.value("mapsIncludeWMOSets", false)) {
								size_t maxSetIndex = 0;
								if (useADTSets) {
									for (const auto& setIndex : objAdt.doodadSets)
										maxSetIndex = std::max(maxSetIndex, static_cast<size_t>(setIndex));
								} else {
									maxSetIndex = model.doodadSet;
								}
								maxSetIndex = std::max(maxSetIndex, size_t(0));

								std::vector<WMOExportDoodadSetMask> mask(maxSetIndex + 1);
								mask[0].checked = true;
								if (useADTSets) {
									for (const auto& setIndex : objAdt.doodadSets)
										mask[setIndex].checked = true;
								} else {
									mask[model.doodadSet].checked = true;
								}

								wmoLoader.setDoodadSetMask(mask);
							}

							if (isRawExport)
								wmoLoader.exportRaw(modelPath, helper, nullptr);
							else
								wmoLoader.exportAsOBJ(modelPath, helper, nullptr);

							// Abort if the export has been cancelled.
							if (helper->isCancelled())
								return out;

							objectCache.insert(cacheID);
						}

						auto doodadNamesIt = setNameCache.find(fileDataID);
						const auto& doodadNames = (doodadNamesIt != setNameCache.end()) ? doodadNamesIt->second : std::vector<std::string>();

						std::string modelFile = fs::relative(modelPath, dir).string();
						if (usePosix)
							modelFile = casc::ExportHelper::win32ToPosix(modelFile);

						std::string doodadSetNamesStr;
						for (size_t i = 0; i < doodadSets.size(); i++) {
							if (i > 0) doodadSetNamesStr += ",";
							if (doodadSets[i] < doodadNames.size())
								doodadSetNamesStr += doodadNames[doodadSets[i]];
						}

						csv.addRow({
							{ "ModelFile", modelFile },
							{ "PositionX", std::to_string(model.position[0]) },
							{ "PositionY", std::to_string(model.position[1]) },
							{ "PositionZ", std::to_string(model.position[2]) },
							{ "RotationX", std::to_string(model.rotation[0]) },
							{ "RotationY", std::to_string(model.rotation[1]) },
							{ "RotationZ", std::to_string(model.rotation[2]) },
							{ "RotationW", "0" },
							{ "ScaleFactor", std::to_string(model.scale / 1024.0f) },
							{ "ModelId", std::to_string(model.uniqueId) },
							{ "Type", "wmo" },
							{ "FileDataID", std::to_string(fileDataID) },
							{ "DoodadSetIndexes", doodadSetsStr },
							{ "DoodadSetNames", doodadSetNamesStr }
						});
					} catch (const std::exception& e) {
						logging::write(std::format("Failed to export {} [{}]", fileName, fileDataID));
						logging::write(std::format("Error: {}", e.what()));
					}
				}

				WMOExporter::clearCache();
			}

			csv.write();
		} else {
			logging::write(std::format("Skipping model placement export {} (file exists, overwrite disabled)", csvPath.string()));
		}
	}

	// Export liquids.
	if (config.value("mapsIncludeLiquid", false) && !rootAdt.liquidChunks.empty()) {
		const fs::path liquidFile = dir / ("liquid_" + tileID + ".json");
		logging::write(std::format("Exporting liquid data to {}", liquidFile.string()));

		nlohmann::json enhancedLiquidChunks = nlohmann::json::array();
		for (size_t chunkIndex = 0; chunkIndex < rootAdt.liquidChunks.size(); chunkIndex++) {
			const auto& chunk = rootAdt.liquidChunks[chunkIndex];

			if (chunk.instances.empty()) {
				enhancedLiquidChunks.push_back(nullptr);
				continue;
			}

			const auto& terrainChunk = rootAdt.chunks[chunkIndex];

			nlohmann::json enhancedChunk;
			nlohmann::json enhancedInstances = nlohmann::json::array();

			for (const auto& instance : chunk.instances) {
				const float chunkX2 = terrainChunk.position[0];
				const float chunkY2 = terrainChunk.position[1];
				const float chunkZ2 = terrainChunk.position[2];

				const float centerX = instance.xOffset + instance.width / 2.0f;
				const float centerY = instance.yOffset + instance.height / 2.0f;

				const float worldX = chunkY2 - (centerX * static_cast<float>(UNIT_SIZE));
				const float worldY = (instance.minHeightLevel + instance.maxHeightLevel) / 2.0f + chunkZ2;
				const float worldZ = chunkX2 - (centerY * static_cast<float>(UNIT_SIZE));

				nlohmann::json inst;
				inst["chunkIndex"] = instance.chunkIndex;
				inst["instanceIndex"] = instance.instanceIndex;
				inst["liquidType"] = instance.liquidType;
				inst["liquidObject"] = instance.liquidObject;
				inst["minHeightLevel"] = instance.minHeightLevel;
				inst["maxHeightLevel"] = instance.maxHeightLevel;
				inst["xOffset"] = instance.xOffset;
				inst["yOffset"] = instance.yOffset;
				inst["width"] = instance.width;
				inst["height"] = instance.height;
				inst["offsetExistsBitmap"] = instance.offsetExistsBitmap;
				inst["offsetVertexData"] = instance.offsetVertexData;
				inst["bitmap"] = instance.bitmap;

				nlohmann::json vd;
				vd["height"] = instance.vertexData.height;
				vd["depth"] = instance.vertexData.depth;
				nlohmann::json uvArr = nlohmann::json::array();
				for (const auto& uv : instance.vertexData.uv)
					uvArr.push_back({ uv.x, uv.y });
				vd["uv"] = uvArr;
				inst["vertexData"] = vd;

				inst["worldPosition"] = { worldX, worldY, worldZ };
				inst["terrainChunkPosition"] = { chunkX2, chunkY2, chunkZ2 };

				enhancedInstances.push_back(inst);
			}

			enhancedChunk["instances"] = enhancedInstances;
			nlohmann::json attrs;
			attrs["fishable"] = chunk.attributes.fishable;
			attrs["deep"] = chunk.attributes.deep;
			enhancedChunk["attributes"] = attrs;
			enhancedLiquidChunks.push_back(enhancedChunk);
		}

		JSONWriter liquidJSON(liquidFile);
		liquidJSON.addProperty("liquidChunks", enhancedLiquidChunks);
		liquidJSON.write();
	}

	// Prepare foliage data tables if needed.
	if (config.value("mapsIncludeFoliage", false) && !hasLoadedFoliage)
		loadFoliageTables();

	// Export foliage.
	if (config.value("mapsIncludeFoliage", false) && isFoliageAvailable) {
		std::unordered_set<uint32_t> foliageExportCache;
		std::unordered_set<uint32_t> foliageEffectCache;
		const fs::path foliageDir = dir / "foliage";

		logging::write(std::format("Exporting foliage to {}", foliageDir.string()));

		for (const auto& chunk : texAdt.texChunks) {
			// Skip chunks that have no layers?
			if (chunk.layers.empty())
				continue;

			for (const auto& layer : chunk.layers) {
				// Skip layers with no effect.
				if (!layer.effectID)
					continue;

				auto groundEffectTexture = dbTextures->getRow(layer.effectID);
				if (!groundEffectTexture.has_value())
					continue;

				// Check if DoodadID field is an array.
				auto doodadIDIt = groundEffectTexture->find("DoodadID");
				if (doodadIDIt == groundEffectTexture->end())
					continue;

				// DoodadID should be a vector of ints.
				std::vector<int64_t> doodadIDs;
				if (auto* vec = std::get_if<std::vector<int64_t>>(&doodadIDIt->second)) {
					doodadIDs = *vec;
				} else if (auto* vec2 = std::get_if<std::vector<uint64_t>>(&doodadIDIt->second)) {
					for (auto v : *vec2) doodadIDs.push_back(static_cast<int64_t>(v));
				} else {
					continue;
				}

				// Create a foliage metadata JSON packed with the table data.
				std::unique_ptr<JSONWriter> foliageJSON;
				if (config.value("exportFoliageMeta", false) && foliageEffectCache.find(layer.effectID) == foliageEffectCache.end()) {
					foliageJSON = std::make_unique<JSONWriter>(foliageDir / (std::to_string(layer.effectID) + ".json"));

					for (const auto& [key, val] : *groundEffectTexture) {
						std::visit([&](const auto& v) {
							foliageJSON->addProperty(key, v);
						}, val);
					}

					foliageEffectCache.insert(layer.effectID);
				}

				nlohmann::json doodadModelIDs = nlohmann::json::object();
				for (const auto doodadEntryID : doodadIDs) {
					// Skip empty fields.
					if (!doodadEntryID)
						continue;

					auto groundEffectDoodad = dbDoodads->getRow(static_cast<uint32_t>(doodadEntryID));
					if (groundEffectDoodad.has_value()) {
						uint32_t modelID = 0;
						auto modelFileIDIt = groundEffectDoodad->find("ModelFileID");
						if (modelFileIDIt != groundEffectDoodad->end()) {
							if (auto* v = std::get_if<int64_t>(&modelFileIDIt->second))
								modelID = static_cast<uint32_t>(*v);
							else if (auto* v2 = std::get_if<uint64_t>(&modelFileIDIt->second))
								modelID = static_cast<uint32_t>(*v2);
						}

						nlohmann::json entry;
						entry["fileDataID"] = modelID;
						doodadModelIDs[std::to_string(doodadEntryID)] = entry;

						if (!modelID || foliageExportCache.count(modelID))
							continue;

						foliageExportCache.insert(modelID);
					}
				}

				if (foliageJSON) {
					// Map fileDataID to the exported OBJ file names.
					for (auto& [key, entry] : doodadModelIDs.items()) {
						uint32_t entryFDID = entry["fileDataID"].get<uint32_t>();
						std::string entryFileName = casc::listfile::getByID(entryFDID).value_or("");

						if (isRawExport)
							entry["fileName"] = fs::path(entryFileName).filename().string();
						else
							entry["fileName"] = casc::ExportHelper::replaceExtension(fs::path(entryFileName).filename().string(), ".obj");
					}

					foliageJSON->addProperty("DoodadModelIDs", doodadModelIDs);
					foliageJSON->write();
				}
			}
		}

		helper->setCurrentTaskName("Tile " + tileID + ", foliage doodads");
		helper->setCurrentTaskMax(static_cast<int>(foliageExportCache.size()));

		// Export foliage after collecting to give an accurate progress count.
		int foliageIndex = 0;
		for (const auto modelID : foliageExportCache) {
			helper->setCurrentTaskValue(foliageIndex++);

			const std::string modelName = fs::path(casc::listfile::getByID(modelID).value_or("")).filename().string();

			auto data = casc->getVirtualFileByID(modelID);
			M2Exporter m2(std::move(data), {}, modelID, casc);

			if (isRawExport) {
				m2.exportRaw(foliageDir / modelName, helper, nullptr);
			} else {
				const std::string modelPathStr = casc::ExportHelper::replaceExtension(modelName, ".obj");
				m2.exportAsOBJ(foliageDir / modelPathStr, config.value("modelsExportCollision", false), helper, nullptr);
			}

			// Abort if the export has been cancelled.
			if (helper->isCancelled())
				return out;
		}
	}

	return out;
}

/**
 * Clear internal tile-loading cache.
 */
void ADTExporter::clearCache() {
	wdtCache.clear();
}
