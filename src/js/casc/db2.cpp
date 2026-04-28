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
 * Necessary adaptation: the JS Proxy defers parsing until the first method
 * is invoked on the wrapper. C++ has no equivalent transparent intercept, so
 * getTable() triggers ensureParsed() before returning the WDCReader&.
 * std::call_once deduplicates so the parse cost is paid exactly once per
 * table, identical to the JS observable behaviour after first method call.
 *
 * Two separate once_flags (parseFlag, preloadFlag) are intentional:
 * ensurePreloaded() invokes ensureParsed() inside its own call_once lambda,
 * which cannot deadlock because the two flags are independent. This permits
 * a caller to call ensureParsed() first (lazy path) and ensurePreloaded()
 * later (eager path) without re-parsing.
 *
 * The JS Proxy also enforces that getRelationRows() requires preload() to have
 * been called. In C++, WDCReader::getRelationRows() enforces the same preload
 * requirement and throws with a matching message.
 */
struct CacheEntry {
	std::unique_ptr<db::WDCReader> reader;
	std::once_flag parseFlag;
	std::once_flag preloadFlag;

	void ensureParsed() {
		std::call_once(parseFlag, [this]() {
			reader->parse();
		});
	}

	void ensurePreloaded() {
		std::call_once(preloadFlag, [this]() {
			ensureParsed();
			reader->preload();
		});
	}
};

static std::unordered_map<std::string, std::unique_ptr<CacheEntry>> cache;
static std::mutex cache_mutex;

db::WDCReader& getTable(const std::string& table_name) {
	CacheEntry* entry_ptr = nullptr;
	{
		std::unique_lock lock(cache_mutex);
		auto it = cache.find(table_name);
		if (it != cache.end()) {
			entry_ptr = it->second.get();
		} else {
			const std::string file_path = "DBFilesClient/" + table_name + ".db2";
			auto entry = std::make_unique<CacheEntry>();
			entry->reader = std::make_unique<db::WDCReader>(file_path);
			entry_ptr = entry.get();
			cache.emplace(table_name, std::move(entry));
		}
	}
	// JS Proxy auto-parses on first method call; ensureParsed() is the C++ equivalent.
	// call_once ensures safe concurrent calls for the same table.
	entry_ptr->ensureParsed();
	return *entry_ptr->reader;
}

db::WDCReader& preloadTable(const std::string& table_name) {
	CacheEntry* entry_ptr = nullptr;
	{
		std::unique_lock lock(cache_mutex);
		auto it = cache.find(table_name);
		if (it != cache.end()) {
			entry_ptr = it->second.get();
		} else {
			const std::string file_path = "DBFilesClient/" + table_name + ".db2";
			auto entry = std::make_unique<CacheEntry>();
			entry->reader = std::make_unique<db::WDCReader>(file_path);
			entry_ptr = entry.get();
			cache.emplace(table_name, std::move(entry));
		}
	}
	// Parse and preload outside the lock so other threads can proceed with different tables.
	// ensurePreloaded() uses call_once so concurrent callers for the same table are safe.
	entry_ptr->ensurePreloaded();
	return *entry_ptr->reader;
}

} // namespace db2
} // namespace casc
