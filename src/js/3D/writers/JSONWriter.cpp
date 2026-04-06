/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "JSONWriter.h"
#include "../../generics.h"
#include "../../file-writer.h"

JSONWriter::JSONWriter(const std::filesystem::path& out)
	: out(out), data(nlohmann::json::object()) {}

void JSONWriter::addProperty(const std::string& name, const nlohmann::json& data) {
	this->data[name] = data;
}

void JSONWriter::write(bool overwrite) {
	// If overwriting is disabled, check file existence.
	if (!overwrite && generics::fileExists(out))
		return;

	generics::createDirectory(out.parent_path());
	FileWriter writer(out);
	// nlohmann::json handles all types natively including large integers;
	// no special BigInt serialization needed as in JS.
	writer.writeLine(data.dump(1, '\t'));
	writer.close();
}