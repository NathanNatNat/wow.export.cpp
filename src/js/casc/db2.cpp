/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>, Marlamin <marlamin@marlamin.com>
	License: MIT
 */
#include "db2.h"
#include "../db/WDCReader.h"
#include "../buffer.h"

#include <unordered_map>
#include <memory>
#include <string>

namespace casc {
namespace db2 {

// JS: const cache = new Map();
static std::unordered_map<std::string, std::unique_ptr<db::WDCReader>> cache;

// JS: const create_wrapper = (reader) => { ... }
// In JS, create_wrapper returns a Proxy that intercepts property/method access
// to auto-parse the reader on first use. In C++, callers are expected to call
// parse() explicitly or use preloadTable(). The wrapper concept is not needed
// because C++ doesn't have dynamic property access like JS Proxies.

// JS: db2_proxy get handler — creates reader on first access
db::WDCReader& getTable(const std::string& table_name) {
	auto it = cache.find(table_name);
	if (it != cache.end())
		return *it->second;

	// JS: const file_path = `DBFilesClient/${table_name}.db2`;
	const std::string file_path = "DBFilesClient/" + table_name + ".db2";

	// JS: const reader = new WDCReader(file_path);
	auto reader = std::make_unique<db::WDCReader>(file_path);

	auto& ref = *reader;
	cache.emplace(table_name, std::move(reader));
	return ref;
}

// JS: preload_proxy get handler — parses and preloads on access
db::WDCReader& preloadTable(const std::string& table_name) {
	auto it = cache.find(table_name);

	if (it != cache.end()) {
		// JS: if (!existing.isLoaded) await existing.parse();
		if (!it->second->isLoaded)
			it->second->parse();

		// JS: existing.preload();
		it->second->preload();
		return *it->second;
	}

	// JS: const file_path = `DBFilesClient/${table_name}.db2`;
	const std::string file_path = "DBFilesClient/" + table_name + ".db2";

	// JS: const reader = new WDCReader(file_path);
	auto reader = std::make_unique<db::WDCReader>(file_path);

	// JS: await reader.parse(); reader.preload();
	reader->parse();
	reader->preload();

	auto& ref = *reader;
	cache.emplace(table_name, std::move(reader));
	return ref;
}

void clearCache() {
	cache.clear();
}

} // namespace db2
} // namespace casc
