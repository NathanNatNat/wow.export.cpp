/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#pragma once

#include <string>
#include <string_view>

namespace casc {
namespace tact_keys {

/**
 * Retrieve a registered decryption key.
 * @param keyName 16-character hex key name.
 * @returns The key value (32-character hex) or empty string if not found.
 */
std::string getKey(std::string_view keyName);

/**
 * Add a decryption key. Subject to validation.
 * Decryption keys will be saved to disk on next tick.
 * Returns true if added, else false if the pair failed validation.
 * @param keyName 16-character hex key name.
 * @param key 32-character hex key value.
 */
bool addKey(std::string_view keyName, std::string_view key);

/**
 * Load tact keys from disk cache and request updated
 * keys from remote server.
 */
void load();

} // namespace tact_keys
} // namespace casc
