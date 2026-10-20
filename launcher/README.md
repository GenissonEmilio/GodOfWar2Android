# Launcher

A small window to adjust the port's settings before starting the game, so that nothing has to be recompiled to change them.

**Status: settings are saved, not applied yet.** **Save settings** writes `GoW2Launcher.ini` next to the executable and the launcher reads it at startup; **Cancel** goes back to what was last saved, **Restore defaults** resets the controls (save to keep them), and the footer says when there are unsaved changes. Nothing passes the settings to the game yet and **PLAY does not start it**; Open game folder and View logs do nothing yet. The title bar works (drag to move, minimize, close).

```ini
; God of War II native port: launcher settings (written by GoW2Launcher)
[graphics]
resolution=2x          ; 1x | 2x | 3x | 4x
aspect_ratio=16:9      ; 16:9 | 4:3
window_mode=borderless ; windowed | borderless
anti_aliasing=fxaa     ; off | fxaa
sharpening=40          ; 0..100
vsync=1
show_fps=0
```

A missing file or a bad value keeps the default for that setting.

## Build and run

```powershell
.\scripts\build-launcher.ps1 -Run
```

The executable ends up in `out\launcher-build\Release\GoW2Launcher.exe`, with its `fonts\` folder next to it. It uses the same libraries and versions as PS2Recomp's runtime (raylib 5.5, Dear ImGui v1.92.7-docking, rlImGui `Raylib_5_5`); if the runner has already been built, the script reuses the copies in `out\build\_deps` instead of downloading them again.

## Layout

- `src/main.cpp`: the whole UI, drawn with Dear ImGui in a borderless window with rounded corners: the 960 × 600 mockup scaled by `kScale` (1.125, so 1080 × 675); every position is written in mockup pixels and goes through `px()`. Positions follow the approved mockup: left panel (title, PLAY, shortcuts, version), tabs (Graphics, Performance, Controls, Audio), settings rows and footer (Cancel, Save settings). Only the Graphics tab has rows for now.
- `fonts/`: Cinzel Bold (display) and Inter Regular / Medium / SemiBold, static instances of the Google Fonts variable fonts made with `fontTools.varLib.instancer`. Both are under the SIL Open Font License 1.1 (`OFL-Cinzel.txt`, `OFL-Inter.txt`); neither declares a Reserved Font Name.

## Next

Pass the saved settings to what the runner already reads (`PS2X_GS_HW_SCALE`, `GOW_SHOW_FPS`…) and start the runner from PLAY, and add what does not exist yet in the runtime (aspect ratio switch, FXAA, CAS sharpening, vertical sync).
