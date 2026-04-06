/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <string>
#include <filesystem>
#include <nlohmann/json.hpp>

class JSONWriter {
public:
	/**
	 * Construct a new JSONWriter instance.
	 * @param out Output path to write to.
	 */
	JSONWriter(const std::filesystem::path& out);

	/**
	 * Add a property to this JSON.
	 * @param name Property name.
	 * @param data Property value.
	 */
	void addProperty(const std::string& name, const nlohmann::json& data);

	/**
	 * Write the JSON to disk.
	 * @param overwrite Whether to overwrite existing files.
	 */
	void write(bool overwrite = true);

private:
	std::filesystem::path out;
	nlohmann::json data;
};
