# UI Reference Screenshots

Reference screenshots from the original JavaScript/NW.js wow.export application.
These were captured at **full screen on Windows on a 4K monitor** (3840×2160).

When making UI changes to the C++ ImGui conversion, **always compare against these screenshots** to ensure visual fidelity with the original app.

---

## Source Select Screen

The initial screen where the user chooses how to open a World of Warcraft installation (local, CDN, or legacy MPQ).

![Source Select Screen](https://github.com/user-attachments/assets/56a22cc1-81c3-4f93-87f0-2255a00bb755)

---

## Build Select Screen

Shown after choosing a source — lists available game builds to explore.

![Build Select Screen](https://github.com/user-attachments/assets/9f4df6f4-fa96-4ed8-b49d-c799a04695a2)

---

## Loading Screen

Displayed whenever the app is loading resources — opening a CASC/legacy source, loading tab data (models, textures, maps, creatures, characters, items, audio, videos, etc.), and exporting. Any operation that calls `showLoadingScreen()` shows this overlay with a progress bar.

![Loading Screen](https://github.com/user-attachments/assets/af551c6f-a3af-44ad-9fa1-c3b15108cf9d)

---

## Home Screen

The main landing screen after a build has been loaded, showing navigation options for the various content tabs.

![Home Screen](https://github.com/user-attachments/assets/150151e0-4012-4a59-a46c-3e2df60f9fd6)
