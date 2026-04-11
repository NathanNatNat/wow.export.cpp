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

---

## Items Tab

The items tab with an item selected, showing the 3D model preview and export controls.

![Items Tab](https://github.com/user-attachments/assets/b4f3393b-705d-4d08-b208-0ec33ce00be3)

---

## Item Sets Tab

The item sets tab showing grouped equipment sets with preview and export controls.

![Item Sets Tab](https://github.com/user-attachments/assets/1a9c5d73-cc14-4256-9452-3e540a2f333e)

---

## Decor Tab

The decor tab with a model loaded, showing 3D preview and export controls.

![Decor Tab](https://github.com/user-attachments/assets/ef3b3d72-b66f-4403-9a51-50b42d3a65e7)

---

## Creatures Tab

The creatures tab with a creature model loaded, showing 3D preview and export controls.

![Creatures Tab](https://github.com/user-attachments/assets/0739440c-21f2-4824-b595-0c9b7d9c3e65)

---

## Audio Tab

The audio tab with a sound playing, showing the animated model, playback controls, progress bar, and export options.

![Audio Tab](https://github.com/user-attachments/assets/3c07c360-d126-4a53-bd05-bf7e17f2d112)

---

## Videos Tab

The videos tab with a video playing, showing the video list, playback area, and streaming controls.

![Videos Tab](https://github.com/user-attachments/assets/b9dfce9f-968b-43ee-a7b0-9adeb4350827)

---

## Maps Tab

The maps tab with a map section selected, showing the tile map viewer with selected region highlighted.

![Maps Tab - Map Selected](https://github.com/user-attachments/assets/22f4ed11-c73f-47cd-a4cc-4da11f1ec412)

The maps tab when there is only a WMO (no terrain tiles), showing the WMO-only export view.

![Maps Tab - WMO Only](https://github.com/user-attachments/assets/93fc6baa-0912-445e-afbf-95fd5fcf7846)

---

## Zones Tab

The zones tab with a zone map loaded, showing the zone image viewer with navigation and export controls.

![Zones Tab](https://github.com/user-attachments/assets/0e0190ac-9541-4642-b052-158400ec7124)

---

## Text Tab

The text tab showing the text file list with search/filter and file preview.

![Text Tab](https://github.com/user-attachments/assets/17bc8831-c9c7-4677-a744-bf20c1664efb)

---

## Fonts Tab

The fonts tab with a font file loaded, showing the font preview and export controls.

![Fonts Tab](https://github.com/user-attachments/assets/87c1f420-39d9-4b32-9429-a9e7e49e0db6)

---

## Data Tab

The data tab with a data table loaded, showing column headers and row data.

![Data Tab - Table Loaded](https://github.com/user-attachments/assets/a182bd36-058c-408d-be1a-be70326df55f)

The data tab showing the settings menu in the top right corner.

![Data Tab - Settings Menu](https://github.com/user-attachments/assets/99310ff8-76e4-486a-af7a-1c4a3cf417e9)

---

## Options Menu — Dev Build

The options/settings menu in the top right corner, shown with a dev build. Dev builds expose additional options not available in release builds.

![Options Menu — Dev Build](https://github.com/user-attachments/assets/9b6e1ccb-7ee1-4423-92be-95fee72f8daf)

---

## Install Manifest Tab

The "Browse Install Manifest" tab, showing the install manifest file list.

![Install Manifest Tab](https://github.com/user-attachments/assets/286867a4-ec47-47dd-8fe9-a4a05ae16bff)

---

## Raw Client Files Tab

The "Browse Raw Client Files" tab, showing the raw file browser for exploring CASC data directly.

![Raw Client Files Tab](https://github.com/user-attachments/assets/264c226a-a884-4e3d-8de4-457228c997d3)

---

## Settings Screen

The full settings screen, shown across multiple screenshots to capture the entire scrollable page. Settings include export directory, character save directory, scroll speed, file list ordering, unknown files, model skins, bone prefixes, shared textures/children, and more.

![Settings Screen — Top](https://github.com/user-attachments/assets/3159072d-1168-4cd6-999a-405665a04883)

![Settings Screen — Upper Middle](https://github.com/user-attachments/assets/be93641d-815a-47d4-9ffb-9ce2dc691cf9)

![Settings Screen — Middle](https://github.com/user-attachments/assets/b07a5fff-1307-4f23-bfe2-9800197dcb74)

![Settings Screen — Lower Middle](https://github.com/user-attachments/assets/e7e1cb11-5399-49a8-9789-30292a8feb8f)

![Settings Screen — Lower Bottom](https://github.com/user-attachments/assets/421f7185-e519-4071-b694-70e60d4dd908)

![Settings Screen — Bottom](https://github.com/user-attachments/assets/0e2d1fed-082e-473f-be66-47bff9a308a0)
