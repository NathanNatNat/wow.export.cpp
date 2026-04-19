/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "content-flags.h"

#include <array>
#include <string_view>
#include <utility>

namespace casc::content_flags {

// JS module.exports line-by-line export mirror:
// {
//   LoadOnWindows, LoadOnMacOS, LowViolence, DoNotLoad, UpdatePlugin,
//   Encrypted, NoNameHash, UncommonResolution, Bundle, NoCompression
// }
[[maybe_unused]] static constexpr std::array<std::pair<std::string_view, uint32_t>, 10> exports = {{
	{"LoadOnWindows", LoadOnWindows},
	{"LoadOnMacOS", LoadOnMacOS},
	{"LowViolence", LowViolence},
	{"DoNotLoad", DoNotLoad},
	{"UpdatePlugin", UpdatePlugin},
	{"Encrypted", Encrypted},
	{"NoNameHash", NoNameHash},
	{"UncommonResolution", UncommonResolution},
	{"Bundle", Bundle},
	{"NoCompression", NoCompression}
}};

} // namespace casc::content_flags
