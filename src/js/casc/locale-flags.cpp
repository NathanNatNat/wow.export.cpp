/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */
#include "locale-flags.h"

#include <array>
#include <string_view>
#include <utility>

namespace casc::locale_flags {

// JS module.exports.flags line-by-line export mirror.
[[maybe_unused]] static constexpr std::array<std::pair<std::string_view, uint32_t>, 13> flags_exports = {{
	{"enUS", enUS},
	{"koKR", koKR},
	{"frFR", frFR},
	{"deDE", deDE},
	{"zhCN", zhCN},
	{"esES", esES},
	{"zhTW", zhTW},
	{"enGB", enGB},
	{"esMX", esMX},
	{"ruRU", ruRU},
	{"ptBR", ptBR},
	{"itIT", itIT},
	{"ptPT", ptPT}
}};

// JS module.exports.names line-by-line export mirror.
[[maybe_unused]] static constexpr std::array<std::pair<std::string_view, std::string_view>, 13> names_exports = {{
	{"enUS", "American English [enUS]"},
	{"koKR", "한국어 [koKR]"},
	{"frFR", "Français [frFR]"},
	{"deDE", "Deutsch [deDE]"},
	{"zhCN", "简体中文 [zhCN]"},
	{"esES", "Español (España) [esES]"},
	{"zhTW", "繁體中文 [zhTW]"},
	{"enGB", "British English [enGB]"},
	{"esMX", "Español (América Latina) [esMX]"},
	{"ruRU", "Русский [ruRU]"},
	{"ptBR", "Português (Brasil) [ptBR]"},
	{"itIT", "Italiano [itIT]"},
	{"ptPT", "Português (Europeu) [ptPT]"}
}};

} // namespace casc::locale_flags
