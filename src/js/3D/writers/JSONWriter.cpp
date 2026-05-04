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

static void convertBigIntsToStrings(nlohmann::json& j) {
	if (j.is_number_unsigned() && j.get<uint64_t>() > 9007199254740991ULL) {
		j = std::to_string(j.get<uint64_t>());
	} else if (j.is_number_integer() && (j.get<int64_t>() > 9007199254740991LL || j.get<int64_t>() < -9007199254740991LL)) {
		j = std::to_string(j.get<int64_t>());
	} else if (j.is_object()) {
		for (auto& [k, v] : j.items())
			convertBigIntsToStrings(v);
	} else if (j.is_array()) {
		for (auto& v : j)
			convertBigIntsToStrings(v);
	}
}

void JSONWriter::write(bool overwrite) {
	// If overwriting is disabled, check file existence.
	if (!overwrite && generics::fileExists(out))
		return;

	generics::createDirectory(out.parent_path());
	nlohmann::json output = data;
	convertBigIntsToStrings(output);
	FileWriter writer(out);
	writer.writeLine(output.dump(1, '\t'));
	writer.close();
}