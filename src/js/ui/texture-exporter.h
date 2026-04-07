/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <string>
#include <vector>
#include <optional>
#include <cstdint>

#include <nlohmann/json.hpp>

namespace casc {
class CASC;
}

namespace mpq {
class MPQInstall;
}

/**
 * Texture export utilities.
 *
 * JS equivalent: module.exports = { exportFiles, exportSingleTexture, getFileInfoPair }
 */
namespace texture_exporter {

/**
 * File info pair result from getFileInfoPair.
 * JS equivalent: { fileName, fileDataID }
 */
struct FileInfoPair {
	std::string fileName;
	std::optional<uint32_t> fileDataID;
};

/**
 * Retrieve the fileDataID and fileName for a given fileDataID (number) or fileName (string).
 * @param input nlohmann::json value — either a number (fileDataID) or a string (fileName / listfile entry).
 * @returns FileInfoPair with fileName and optional fileDataID.
 */
FileInfoPair getFileInfoPair(const nlohmann::json& input);

/**
 * Export texture files to the configured format.
 * @param files      Array of fileDataIDs (json::number) or file paths (json::string).
 * @param casc       CASC source (may be nullptr if isMPQ or isLocal).
 * @param mpq        MPQ install (non-null implies isMPQ mode).
 * @param isLocal    If true, files are local filesystem paths.
 * @param exportID   Export ID for tracking (default -1).
 */
void exportFiles(
	const std::vector<nlohmann::json>& files,
	casc::CASC* casc = nullptr,
	mpq::MPQInstall* mpq = nullptr,
	bool isLocal = false,
	int exportID = -1
);

/**
 * Export a single texture by fileDataID.
 * @param fileDataID The file data ID of the texture to export.
 * @param casc       CASC source.
 */
void exportSingleTexture(uint32_t fileDataID, casc::CASC* casc);

} // namespace texture_exporter
