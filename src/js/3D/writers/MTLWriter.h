/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <string>
#include <vector>
#include <filesystem>

class MTLWriter {
public:
	/**
	 * Construct a new MTLWriter instance.
	 * @param out Output path to write to.
	 */
	MTLWriter(const std::filesystem::path& out);

	/**
	 * Add a material to this material library.
	 * @param name Material name.
	 * @param file Texture file path.
	 */
	void addMaterial(const std::string& name, const std::string& file);

	/**
	 * Returns true if this material library is empty.
	 */
	bool isEmpty() const;

	/**
	 * Write the material library to disk.
	 * @param overwrite Whether to overwrite existing files.
	 */
	void write(bool overwrite = true);

private:
	struct Material {
		std::string name;
		std::string file;
	};

	std::filesystem::path out;
	std::vector<Material> materials;
};
