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

static std::unordered_map<std::string, std::unique_ptr<db::WDCReader>> cache;

// In JS, create_wrapper returns a Proxy that intercepts property/method access
// to auto-parse the reader on first use. In C++, callers are expected to call
// parse() explicitly or use preloadTable(). The wrapper concept is not needed
// because C++ doesn't have dynamic property access like JS Proxies.

db::WDCReader& getTable(const std::string& table_name) {
	auto it = cache.find(table_name);
	if (it != cache.end())
		return *it->second;

	const std::string file_path = "DBFilesClient/" + table_name + ".db2";

	auto reader = std::make_unique<db::WDCReader>(file_path);

	auto& ref = *reader;
	cache.emplace(table_name, std::move(reader));
	return ref;
}

db::WDCReader& preloadTable(const std::string& table_name) {
	auto it = cache.find(table_name);

	if (it != cache.end()) {
		if (!it->second->isLoaded)
			it->second->parse();

		it->second->preload();
		return *it->second;
	}

	const std::string file_path = "DBFilesClient/" + table_name + ".db2";

	auto reader = std::make_unique<db::WDCReader>(file_path);

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
