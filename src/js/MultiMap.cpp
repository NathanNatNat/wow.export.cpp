/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "MultiMap.h"

// MultiMap is a template class, fully defined in the header.
// This .cpp exists to match the original JS file structure.

// set() collision semantics verified against JS source (src/js/MultiMap.js lines 19-29):
//   - Key not found                  → store value directly (not wrapped in array), matching JS: super.set(key, value)
//   - Key found, stored as single T  → convert to vector{existing, new},            matching JS: super.set(key, [check, value])
//   - Key found, stored as vector<T> → push_back new value,                          matching JS: check.push(value)