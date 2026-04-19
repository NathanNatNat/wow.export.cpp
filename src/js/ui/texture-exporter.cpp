/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "texture-exporter.h"

#include <format>
#include <filesystem>
#include <algorithm>
#include <cctype>

#include <imgui.h>
#include <stb_image.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <cstdio>
#endif

#include "../core.h"
#include "../log.h"
#include "../generics.h"
#include "../buffer.h"
#include "../casc/blp.h"
#include "../casc/listfile.h"
#include "../casc/export-helper.h"
#include "../casc/casc-source.h"
#include "../mpq/mpq-install.h"
#include "../file-writer.h"
#include "../3D/writers/JSONWriter.h"

namespace texture_exporter {

namespace fs = std::filesystem;

/**
 * Copy PNG image data to the system clipboard.
 * JS equivalent: nw.Clipboard.get().set(png.toBase64(), 'png', true)
 *
 * On Windows, decodes the PNG to RGBA pixels and places a CF_DIB on the clipboard.
 * On Linux, pipes the raw PNG data to xclip -selection clipboard -target image/png.
 * Falls back to copying the base64-encoded text if platform clipboard fails.
 */
static void copyPNGToClipboard(const BufferWrapper& png) {
#ifdef _WIN32
	// Decode PNG → RGBA pixels using stb_image.
	int w = 0, h = 0, channels = 0;
	const auto* pngData = png.internalArrayBuffer();
	unsigned char* pixels = stbi_load_from_memory(pngData, static_cast<int>(png.byteLength()), &w, &h, &channels, 4);
	if (!pixels) {
		// Fallback to text clipboard if decode fails.
		ImGui::SetClipboardText(png.toBase64().c_str());
		return;
	}

	// Build a CF_DIB (BITMAPINFOHEADER + pixel data in bottom-up BGR order).
	const int stride = w * 4;
	const size_t pixelBytes = static_cast<size_t>(h) * stride;
	const size_t dibSize = sizeof(BITMAPINFOHEADER) + pixelBytes;

	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, dibSize);
	if (!hMem) {
		stbi_image_free(pixels);
		ImGui::SetClipboardText(png.toBase64().c_str());
		return;
	}

	void* mem = GlobalLock(hMem);
	auto* bmi = reinterpret_cast<BITMAPINFOHEADER*>(mem);
	std::memset(bmi, 0, sizeof(BITMAPINFOHEADER));
	bmi->biSize = sizeof(BITMAPINFOHEADER);
	bmi->biWidth = w;
	bmi->biHeight = h;  // positive = bottom-up
	bmi->biPlanes = 1;
	bmi->biBitCount = 32;
	bmi->biCompression = BI_RGB;

	// Convert RGBA top-down to BGRA bottom-up.
	auto* dst = reinterpret_cast<uint8_t*>(mem) + sizeof(BITMAPINFOHEADER);
	for (int row = 0; row < h; ++row) {
		const uint8_t* src = pixels + static_cast<size_t>(h - 1 - row) * stride;
		for (int col = 0; col < w; ++col) {
			dst[col * 4 + 0] = src[col * 4 + 2]; // B
			dst[col * 4 + 1] = src[col * 4 + 1]; // G
			dst[col * 4 + 2] = src[col * 4 + 0]; // R
			dst[col * 4 + 3] = src[col * 4 + 3]; // A
		}
		dst += stride;
	}

	GlobalUnlock(hMem);
	stbi_image_free(pixels);

	if (OpenClipboard(nullptr)) {
		EmptyClipboard();
		if (!SetClipboardData(CF_DIB, hMem))
			GlobalFree(hMem);  // only free on failure; clipboard owns hMem on success
		CloseClipboard();
	} else {
		GlobalFree(hMem);
		ImGui::SetClipboardText(png.toBase64().c_str());
	}
#else
	// Linux: pipe PNG data to xclip.
	FILE* proc = popen("xclip -selection clipboard -target image/png 2>/dev/null", "w");
	if (proc) {
		std::fwrite(png.internalArrayBuffer(), 1, png.byteLength(), proc);
		pclose(proc);
	} else {
		// Fallback to text clipboard if xclip is unavailable.
		ImGui::SetClipboardText(png.toBase64().c_str());
	}
#endif
}

/**
 * Case-insensitive check whether a filename ends with a given extension.
 * JS equivalent: fileName.toLowerCase().endsWith(ext)
 */
static bool endsWithCI(const std::string& str, const std::string& suffix) {
	if (suffix.size() > str.size())
		return false;
	return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin(),
		[](char a, char b) { return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b)); });
}

/**
 * Extract the file extension from a filename (from the last '.' to end), lowercased.
 * JS equivalent: fileName.slice(fileName.lastIndexOf('.')).toLowerCase()
 * Returns empty string if no '.' is found.
 */
static std::string getExtensionLower(const std::string& fileName) {
	const auto dotPos = fileName.rfind('.');
	if (dotPos == std::string::npos)
		return "";
	std::string ext = fileName.substr(dotPos);
	std::transform(ext.begin(), ext.end(), ext.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return ext;
}

/**
 * Retrieve the fileDataID and fileName for a given fileDataID or fileName.
 * @param input nlohmann::json value — either a number (fileDataID) or a string.
 * @returns FileInfoPair with fileName and optional fileDataID.
 */
FileInfoPair getFileInfoPair(const nlohmann::json& input) {
	FileInfoPair pair;

	if (input.is_number()) {
		pair.fileDataID = input.get<uint32_t>();
		std::string name = casc::listfile::getByID(pair.fileDataID.value().value_or(""));
		pair.fileName = name.empty()
			? casc::listfile::formatUnknownFile(pair.fileDataID.value(), ".blp")
			: name;
	} else {
		pair.fileName = casc::listfile::stripFileEntry(input.get<std::string>());
		pair.fileDataID = casc::listfile::getByFilename(pair.fileName);
	}

	return pair;
}

/**
 * Export BLP metadata to a JSON file alongside the texture.
 * @param blp          The BLP image instance.
 * @param exportPath   Export path of the image file.
 * @param overwrite    Whether to overwrite existing files.
 * @param manifest     Export manifest accumulator (succeeded array).
 * @param fileDataID   The file data ID.
 */
static void exportBLPMetadata(casc::BLPImage& blp, const std::string& exportPath,
	bool overwrite, nlohmann::json& manifest, std::optional<uint32_t> fileDataID)
{
	const std::string jsonOut = casc::ExportHelper::replaceExtension(exportPath, ".json");
	JSONWriter json(jsonOut);
	json.addProperty("encoding", blp.encoding);
	json.addProperty("alphaDepth", blp.alphaDepth);
	json.addProperty("alphaEncoding", blp.alphaEncoding);
	json.addProperty("mipmaps", blp.containsMipmaps);
	json.addProperty("width", blp.width);
	json.addProperty("height", blp.height);
	json.addProperty("mipmapCount", blp.mapCount);
	json.addProperty("mipmapSizes", blp.mapSizes);

	json.write(overwrite);
	nlohmann::json entry = {{"type", "META"}, {"file", jsonOut}};
	if (fileDataID) entry["fileDataID"] = *fileDataID;
	manifest["succeeded"].push_back(std::move(entry));
}

/**
 * Export texture files to the configured format.
 * @param files    Array of fileDataIDs (json::number) or file paths (json::string).
 * @param casc     CASC source (may be nullptr if mpq or isLocal).
 * @param mpq      MPQ install (non-null implies isMPQ mode).
 * @param isLocal  If true, files are local filesystem paths.
 * @param exportID Export ID for tracking.
 */
void exportFiles(
	const std::vector<nlohmann::json>& files,
	casc::CASC* casc,
	mpq::MPQInstall* mpq,
	bool isLocal,
	int exportID)
{
	const bool isMPQ = (mpq != nullptr);
	const std::string format = core::view->config.value("exportTextureFormat", std::string("PNG"));

	if (format == "CLIPBOARD") {
		const auto [fileName, fileDataID] = getFileInfoPair(files[0]);

		BufferWrapper data;
		if (isMPQ) {
			auto raw_data = mpq->getFile(fileName);
			if (raw_data)
				data = BufferWrapper(std::move(*raw_data));
		} else if (isLocal) {
			data = BufferWrapper::readFile(fs::path(fileName));
		} else if (casc && fileDataID) {
			data = casc->getVirtualFileByID(*fileDataID);
		}

		const uint8_t mask = static_cast<uint8_t>(core::view->config.value("exportChannelMask", 0b1111));
		casc::BLPImage blp(std::move(data));
		const BufferWrapper png = blp.toPNG(mask);

		// JS: clipboard.set(png.toBase64(), 'png', true) — copies actual PNG image data.
		copyPNGToClipboard(png);

		logging::write(std::format("Copied texture to clipboard ({})", fileName));
		core::setToast("success",
			std::format("Selected texture {} has been copied to the clipboard", fileName),
			{}, -1, true);
		return;
	}

	casc::ExportHelper helper(static_cast<int>(files.size()), "texture");
	helper.start();

	FileWriter exportPaths = core::openLastExportStream();

	const bool overwriteFiles = isLocal || core::view->config.value("overwriteFiles", true);
	const bool exportMeta = core::view->config.value("exportBLPMeta", false);

	nlohmann::json manifest = {
		{"type", "TEXTURES"},
		{"exportID", exportID},
		{"succeeded", nlohmann::json::array()},
		{"failed", nlohmann::json::array()}
	};

	for (const auto& fileEntry : files) {
		// Abort if the export has been cancelled.
		if (helper.isCancelled())
			return;

		const auto [fileName, fileDataID] = getFileInfoPair(fileEntry);

		std::string markFileName = fileName;

		try {
			std::string exportFileName = fileName;

			// Use fileDataID as filename if exportNamedFiles is disabled
			if (!isLocal && !core::view->config.value("exportNamedFiles", true) && fileDataID) {
				// JS: fileName.toLowerCase().endsWith('.blp') — fully case-insensitive
				const std::string ext = endsWithCI(fileName, ".blp") ? ".blp" : ".png";
				const fs::path namePath(fileName);
				const std::string fileDataIDName = std::to_string(*fileDataID) + ext;
				const fs::path dir = namePath.parent_path();
				exportFileName = dir.empty() || dir == fs::path(".")
					? fileDataIDName
					: (dir / fileDataIDName).string();
			}

			std::string exportPath = isLocal ? fileName : casc::ExportHelper::getExportPath(exportFileName);
			markFileName = exportFileName;

			if (format == "WEBP") {
				exportPath = casc::ExportHelper::replaceExtension(exportPath, ".webp");
				markFileName = casc::ExportHelper::replaceExtension(exportFileName, ".webp");
			} else if (format != "BLP") {
				exportPath = casc::ExportHelper::replaceExtension(exportPath, ".png");
				markFileName = casc::ExportHelper::replaceExtension(exportFileName, ".png");
			}

			if (overwriteFiles || !generics::fileExists(exportPath)) {
				BufferWrapper data;
				if (isMPQ) {
					auto raw_data = mpq->getFile(fileName);
					if (raw_data)
						data = BufferWrapper(std::move(*raw_data));
				} else if (isLocal) {
					data = BufferWrapper::readFile(fs::path(fileName));
				} else if (casc && fileDataID) {
					data = casc->getVirtualFileByID(*fileDataID);
				}

				// Determine file extension — JS: fileName.slice(fileName.lastIndexOf('.')).toLowerCase()
				const std::string file_ext = getExtensionLower(fileName);
				const bool is_png = (file_ext == ".png");
				const bool is_jpg = (file_ext == ".jpg" || file_ext == ".jpeg");

				if (is_png || is_jpg) {
					// Raw export for png/jpg (no BLP conversion)
					data.writeToFile(fs::path(exportPath));
					const std::string tag = is_png ? "PNG" : "JPG";
					exportPaths.writeLine(tag + ":" + exportPath);
				} else if (format == "BLP") {
					// Export as raw BLP file with no conversion
					data.writeToFile(fs::path(exportPath));
					exportPaths.writeLine("BLP:" + exportPath);
				} else if (format == "WEBP") {
					// Export as WebP
					const uint8_t mask = static_cast<uint8_t>(core::view->config.value("exportChannelMask", 0b1111));
					const int quality = core::view->config.value("exportWebPQuality", 90);
					casc::BLPImage blp(std::move(data));
					blp.saveToWebP(fs::path(exportPath), mask, 0, quality);
					exportPaths.writeLine("WEBP:" + exportPath);

					if (exportMeta)
						exportBLPMetadata(blp, exportPath, overwriteFiles, manifest, fileDataID);
				} else {
					// Export as PNG
					const uint8_t mask = static_cast<uint8_t>(core::view->config.value("exportChannelMask", 0b1111));
					casc::BLPImage blp(std::move(data));
					blp.saveToPNG(fs::path(exportPath), mask);
					exportPaths.writeLine("PNG:" + exportPath);

					if (exportMeta)
						exportBLPMetadata(blp, exportPath, overwriteFiles, manifest, fileDataID);
				}
			} else {
				logging::write(std::format("Skipping export of {} (file exists, overwrite disabled)", exportPath));
			}

			helper.mark(markFileName, true);
			nlohmann::json entry = {{"type", format}, {"file", exportPath}};
			if (fileDataID) entry["fileDataID"] = *fileDataID;
			manifest["succeeded"].push_back(std::move(entry));
		} catch (const std::exception& e) {
			// JS: helper.mark(markFileName, false, e.message, e.stack)
			// C++ std::exception has no .stack equivalent; pass std::nullopt for stackTrace.
			helper.mark(markFileName, false, e.what(), std::nullopt);
			nlohmann::json entry = {{"type", format}};
			if (fileDataID) entry["fileDataID"] = *fileDataID;
			manifest["failed"].push_back(std::move(entry));
		}
	}

	exportPaths.close();
	helper.finish();
}

/**
 * Export a single texture by fileDataID.
 * @param fileDataID The file data ID of the texture to export.
 * @param casc       CASC source.
 */
void exportSingleTexture(uint32_t fileDataID, casc::CASC* casc) {
	exportFiles({nlohmann::json(fileDataID)}, casc, nullptr, false);
}

} // namespace texture_exporter