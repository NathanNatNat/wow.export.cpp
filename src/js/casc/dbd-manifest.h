/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <optional>
#include <string>
#include <vector>

namespace casc {
namespace dbd_manifest {

/**
 * preload the dbd manifest from configured urls
 */
void preload();

/**
 * prepare the manifest for use, awaiting preload if necessary
 */
void prepareManifest();

/**
 * get table name by filedataid
 * @param id
 * @returns table name or std::nullopt
 */
std::optional<std::string> getByID(int id);

/**
 * get filedataid by table name
 * @param table_name
 * @returns filedataid or std::nullopt
 */
std::optional<int> getByTableName(const std::string& table_name);

/**
 * get all table names
 * @returns sorted vector of table names
 */
std::vector<std::string> getAllTableNames();

} // namespace dbd_manifest
} // namespace casc
