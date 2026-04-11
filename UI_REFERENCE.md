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

---

## Models Tab — Nothing Loaded

The models tab after data has loaded, showing the file list, empty 3D preview area, and Preview/Export controls on the right.

![Models Tab — Nothing Loaded](https://github.com/user-attachments/assets/52168fdb-bb8c-43ad-922e-42f7ff1b62fd)

---

## Models Tab — M2 Model Loaded

An M2 model selected and previewed. M3 models render with the same layout.

![Models Tab — M2 Model Loaded](https://github.com/user-attachments/assets/3cfbcd92-78e5-4e64-ba42-bd6e19f9631d)

---

## Models Tab — WMO Model Loaded

A WMO model selected and previewed.

![Models Tab — WMO Model Loaded](https://github.com/user-attachments/assets/dfc64133-f177-453a-98ce-1ac21cc0fb9d)

---

## Textures Tab

The textures tab with file list, texture preview, and export controls.

![Textures Tab](https://github.com/user-attachments/assets/580cb305-db6a-4bf9-b2d9-a940e7bb635a)

---

## Textures Tab — Atlas Regions Enabled

The textures tab with the "Atlas Regions" option enabled, showing region overlays on the texture preview.

![Textures Tab — Atlas Regions](https://github.com/user-attachments/assets/e0bc21de-e8dd-4651-9a19-a4ee4078f809)

---

## Characters Tab — Export Panel

The characters tab with character selection options on the left (race, gender, skin color, etc.), 3D preview in the center, and the Export panel selected in the bottom-right. Note: dropdown options are populated from the database.

![Characters Tab — Export](https://github.com/user-attachments/assets/da6d1ff2-f4be-42f5-ad11-19f62c3efd53)

---

## Characters Tab — Textures Panel

The characters tab with the Textures panel selected in the bottom-right, showing texture file list and export options.

![Characters Tab — Textures](https://github.com/user-attachments/assets/826989f8-0dba-42d4-a2b0-01148872e816)

---

## Characters Tab — Settings Panel

The characters tab with the Settings panel selected in the bottom-right, showing model display settings.

![Characters Tab — Settings](https://github.com/user-attachments/assets/972a530b-c380-40fc-9ae8-6f23344c1ac3)

---

## Characters Tab — Custom Geoset Selection

The characters tab with the custom geoset selection panel open on the left, allowing toggling of individual geometry groups on the character model.

![Characters Tab — Custom Geoset Selection](https://github.com/user-attachments/assets/545ae7f4-a7ee-4a6e-8571-4b3d1803b0f7)
