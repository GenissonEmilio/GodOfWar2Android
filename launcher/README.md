# Launcher

A small window to adjust the port's settings before starting the game, so that nothing has to be recompiled to change them.

**Status: PLAY starts the game.** **Save settings** writes `GoW2Launcher.ini` next to the executable and the launcher reads it at startup; **Cancel** goes back to what was last saved, **Restore defaults** resets the controls (save to keep them), and the footer says when there are unsaved changes. **Open game folder** and **View logs** open the repo folder and `out\logs`.

**PLAY** saves the settings, finds the port (it walks up from its own folder to the one with `game\elf-name.txt`, and takes the newest of `out\build\ps2xRuntime\{RelWithDebInfo,Release,Debug}\ps2EntryRunner.exe`) and starts it like `scripts\run.ps1`: same ELF, the repo as working directory, run.ps1's default variables (`PS2X_GS_WORKER`, `GOW_GS_FINISH_ASINCRONO`, `GOW_AUDIO_DEVICE`, kept if already set) and stdout/stderr in `out\logs\launcher-YYYYMMDD-HHMMSS.log`. If the game is still running after 4 s the launcher closes; if it quits before, the launcher stays open and shows the exit code. The bottom-left line says whether the game was found.

| Setting | Passed to the runner as | Status |
|---|---|---|
| Internal resolution | `PS2X_GS_HW_SCALE=1..4` | works |
| FPS counter | `GOW_SHOW_FPS=1` | works |
| Sharpening | `GOW_SCALER=sharp` (> 0) / `bilinear` (0) | on/off only: the present pass has a fixed light sharpen |
| Aspect ratio, window mode at startup, anti-aliasing, vertical sync | — | the runtime has no switch yet (16:9 is built in); marked "coming soon" in the window |

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

- `src/game.h`, `src/game_win.cpp`: finding the port and starting it (Win32 is kept out of `main.cpp`: `windows.h` clashes with raylib's names).
- `src/main.cpp`: the whole UI, drawn with Dear ImGui in a borderless window with rounded corners: the 960 × 600 mockup scaled by `kScale` (1.125, so 1080 × 675); every position is written in mockup pixels and goes through `px()`. Positions follow the approved mockup: left panel (title, PLAY, shortcuts, version), tabs (Graphics, Performance, Controls, Audio), settings rows and footer (Cancel, Save settings). Only the Graphics tab has rows for now.
- `fonts/`: Cinzel Bold (display) and Inter Regular / Medium / SemiBold, static instances of the Google Fonts variable fonts made with `fontTools.varLib.instancer`. Both are under the SIL Open Font License 1.1 (`OFL-Cinzel.txt`, `OFL-Inter.txt`); neither declares a Reserved Font Name.

## Next

Add to the runtime what does not exist yet (aspect ratio switch, borderless fullscreen at startup, FXAA, adjustable CAS sharpening, vertical sync) and pass it from the launcher; fill in the Performance, Controls and Audio tabs.
