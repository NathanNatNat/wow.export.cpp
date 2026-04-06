/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "MTLWriter.h"
#include "../../generics.h"
#include "../../file-writer.h"
#include "../../core.h"

MTLWriter::MTLWriter(const std::filesystem::path& out)
	: out(out) {}

void MTLWriter::addMaterial(const std::string& name, const std::string& file) {
	materials.push_back({ name, file });
}

bool MTLWriter::isEmpty() const {
	return materials.empty();
}

void MTLWriter::write(bool overwrite) {
	// Don't bother writing an empty material library.
	if (isEmpty())
		return;

	// If overwriting is disabled, check file existence.
	if (!overwrite && generics::fileExists(out))
		return;

	const auto mtlDir = out.parent_path();
	generics::createDirectory(mtlDir);

	const bool useAbsolute = core::view->config.value("enableAbsoluteMTLPaths", false);
	FileWriter writer(out);

	for (const auto& material : materials) {
		writer.writeLine("newmtl " + material.name);
		writer.writeLine("illum 1");

		std::string materialFile = material.file;
		if (useAbsolute)
			materialFile = std::filesystem::weakly_canonical(mtlDir / materialFile).string();

		writer.writeLine("map_Kd " + materialFile);
	}

	writer.close();
}