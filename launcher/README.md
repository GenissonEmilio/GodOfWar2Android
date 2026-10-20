# Launcher

A small window to adjust the port's settings before starting the game, so that nothing has to be recompiled to change them.

**Status: front end only.** The window, the tabs and every control are there and react, but nothing is read, saved or applied yet, and **PLAY does not start the game**. Only the title bar works for real (drag to move, minimize, close).

## Build and run

```powershell
.\scripts\build-launcher.ps1 -Run
```

The executable ends up in `out\launcher-build\Release\GoW2Launcher.exe`, with its `fonts\` folder next to it. It uses the same libraries and versions as PS2Recomp's runtime (raylib 5.5, Dear ImGui v1.92.7-docking, rlImGui `Raylib_5_5`); if the runner has already been built, the script reuses the copies in `out\build\_deps` instead of downloading them again.

## Layout

- `src/main.cpp`: the whole UI, drawn with Dear ImGui in a borderless 960 × 600 window with rounded corners. Positions follow the approved mockup: left panel (title, PLAY, shortcuts, version), tabs (Graphics, Performance, Controls, Audio), settings rows and footer (Cancel, Save settings). Only the Graphics tab has rows for now.
- `fonts/`: Cinzel Bold (display) and Inter Regular / Medium / SemiBold, static instances of the Google Fonts variable fonts made with `fontTools.varLib.instancer`. Both are under the SIL Open Font License 1.1 (`OFL-Cinzel.txt`, `OFL-Inter.txt`); neither declares a Reserved Font Name.

## Next

Wire the settings to what the runner already reads (`PS2X_GS_HW_SCALE`, `GOW_SHOW_FPS`…), save them to a file, start the runner from PLAY, and add what does not exist yet in the runtime (aspect ratio switch, FXAA, CAS sharpening, vertical sync).
