/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>, Marlamin <marlamin@marlamin.com>
	License: MIT
 */
#pragma once

#include <string>

namespace db {
	class WDCReader;
}

namespace casc {

/**
 * DB2 table cache.
 *
 * JS equivalent: module.exports = db2_proxy
 *
 * In JS, db2 is a Proxy object that lazily creates WDCReader instances
 * for any table name accessed as a property:
 *   - db2.SomeTable.getAllRows()  → lazy parse on first method call
 *   - db2.preload.SomeTable()    → parse + preload immediately
 *
 *   - db2::getTable("SomeTable") → returns a WDCReader& (lazy parse)
 *   - db2::preloadTable("SomeTable") → parse + preload, returns WDCReader&
 */
namespace db2 {

/**
 * Get a WDCReader for the given table name.
 * Creates and caches the reader if not already cached.
 * The reader is automatically parsed on first access (matching JS Proxy behavior
 * where any method call triggers parse). Parse is deduplicated via std::once_flag.
 *
 * JS equivalent: db2[table_name] (Proxy get handler with auto-parse)
 *
 * @param table_name DB2 table name (e.g. "TextureFileData").
 * @returns Reference to the cached WDCReader.
 */
db::WDCReader& getTable(const std::string& table_name);

/**
 * Get a WDCReader for the given table name, ensuring it is
 * parsed and preloaded (all rows cached in memory).
 *
 * JS equivalent: db2.preload[table_name]()
 *
 * @param table_name DB2 table name (e.g. "TextureFileData").
 * @returns Reference to the cached WDCReader.
 */
db::WDCReader& preloadTable(const std::string& table_name);

/**
 * Clear the entire table cache, releasing all readers.
 */
void clearCache();

} // namespace db2
} // namespace casc
