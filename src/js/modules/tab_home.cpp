/*!
	wow.export (https://github.com/Kruithne/wow.export)
	Authors: Kruithne <kruithne@gmail.com>
	License: MIT
 */

// Stub: see tab_home.js for the full implementation reference.
//
// The JS template renders (src/js/modules/tab_home.js):
//
//   <div class="tab" id="tab-home">
//     <HomeShowcase />                      (left column, grid rows 1-3; see home-showcase.js)
//     <div id="home-changes">               (right column, bordered panel)
//       <div v-html="$core.view.whatsNewHTML"></div>
//     </div>
//     <div id="home-help-buttons">          (bottom full-width row; hidden < 900px viewport)
//       <div data-external="::DISCORD">...</div>
//       <div data-external="::GITHUB">...</div>
//       <div data-external="::PATREON">...</div>
//     </div>
//   </div>
//
//   CSS grid: grid-template-columns: 1fr 1fr; padding: 50px; gap: 0 50px;
//   #home-changes: border 1px, border-radius 10px, background home-background.webp cover.
//   #home-help-buttons: flex row, justify-content center, gap 20px.
//     Each card: 300×105px, SVG icon 120px at right -20px rotated 20deg, title + subtitle.
//   @media (max-height: 899px): #home-help-buttons { display: none; }
//
// See TODO entries 152-166 in TODO_TRACKER.md.

#include "tab_home.h"
#include "../modules.h"
#include "../components/home-showcase.h"

namespace tab_home {

static home_showcase::HomeShowcaseState s_showcaseState;

void renderHomeLayout() {
	// Stub: see file-level comment and TODO entries 152-166.
}

void render() {
	renderHomeLayout();
}

void navigate(const char* module_name) {
	modules::set_active(module_name);
}

void cleanup() {
	home_showcase::cleanup(s_showcaseState);
}

} // namespace tab_home
