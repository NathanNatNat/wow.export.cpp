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
#include <mutex>
#include <string>

namespace casc {
namespace db2 {

/**
 * Cache entry wrapping a WDCReader with auto-parse support.
 *
 * JS equivalent: create_wrapper() returns a Proxy that intercepts method calls
 * to auto-parse the reader on first use. In C++, we achieve this by wrapping
 * the reader with a mutex-protected parse-once guard.
 *
 * The JS Proxy also enforces that getRelationRows() requires preload() to have
 * been called. In C++, WDCReader::getRelationRows() enforces the same preload
 * requirement and throws with a matching message.
 */
struct CacheEntry {
	std::unique_ptr<db::WDCReader> reader;
	std::once_flag parseFlag;

	void ensureParsed() {
		std::call_once(parseFlag, [this]() {
			reader->parse();
		});
	}
};

static std::unordered_map<std::string, std::unique_ptr<CacheEntry>> cache;

db::WDCReader& getTable(const std::string& table_name) {
	auto it = cache.find(table_name);
	if (it != cache.end()) {
		// JS Proxy auto-parses on first method call; ensureParsed() is the C++ equivalent.
		it->second->ensureParsed();
		return *it->second->reader;
	}

	const std::string file_path = "DBFilesClient/" + table_name + ".db2";

	auto entry = std::make_unique<CacheEntry>();
	entry->reader = std::make_unique<db::WDCReader>(file_path);

	// JS Proxy auto-parses on first method call.
	entry->ensureParsed();

	auto& ref = *entry->reader;
	cache.emplace(table_name, std::move(entry));
	return ref;
}

db::WDCReader& preloadTable(const std::string& table_name) {
	auto it = cache.find(table_name);

	if (it != cache.end()) {
		it->second->ensureParsed();
		it->second->reader->preload();
		return *it->second->reader;
	}

	const std::string file_path = "DBFilesClient/" + table_name + ".db2";

	auto entry = std::make_unique<CacheEntry>();
	entry->reader = std::make_unique<db::WDCReader>(file_path);

	entry->ensureParsed();
	entry->reader->preload();

	auto& ref = *entry->reader;
	cache.emplace(table_name, std::move(entry));
	return ref;
}

} // namespace db2
} // namespace casc
