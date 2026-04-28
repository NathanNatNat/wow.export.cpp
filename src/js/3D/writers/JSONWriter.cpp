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

// JS write() is async (uses await for I/O). C++ runs synchronously by design.
void JSONWriter::write(bool overwrite) {
	// If overwriting is disabled, check file existence.
	if (!overwrite && generics::fileExists(out))
		return;

	generics::createDirectory(out.parent_path());
	FileWriter writer(out);
	// nlohmann::json handles integers up to uint64_t natively. Values exceeding
	// uint64_t (JS BigInt above 2^64) must be pre-converted to strings by callers
	// — there is no BigInt type in C++.
	writer.writeLine(data.dump(1, '\t'));
	writer.close();
}