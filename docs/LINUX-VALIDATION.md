# Linux validation — 0.1.0

Validated on 2026-10-22. Code revision:
`2d58a2dc61e63967469bd25bf8db915dd0278a70`.
[Automatic build and tests](https://github.com/GenissonEmilio/GodOfWar2PC/actions/runs/39716895796): **passed**.
Later documentation-only commits do not change the tested runtime.

## Local game test

Fedora 44, x86_64, Mesa Intel Graphics (RPL-P), native OpenGL 4.6,
1920×1080 monitor. The owned NTSC-U SCUS-97481 v1.01 ZIP was imported with the
production importer: 16 disc files, both DVD layers, sector layout and metadata.
Executable SHA256:
`fd403603be46dea70d130695820d28b3f31de6d7e48484a5f006398a605f7dd6`.

The 6,607-function EE map was recompiled locally. The preparation recorded 511
VU1 micro-memory images, generated 1,485 native blocks with no unsupported blocks,
and compiled all eight captured IOP modules. Compiler build ID:
`b2a91af7fdd6cdc4b6d1446e`. Nothing generated from the disc was uploaded.

Two runs exited normally, without missing-target errors:

- **150 seconds through the frozen launcher:** title menu, New Game and opening
  movie, native fullscreen, the imported sharpening shader and eight screenshots.
  The native sound mixer produced 14,483,456 signed-16-bit samples; peak 23,353,
  RMS 3,762. This establishes audio synthesis, not subjective speaker quality.
- **240 seconds using the installed package:** opening movie and the first
  playable room in Rhodes. Screenshots were inspected to confirm attacks, jumps,
  enemy hits and orb collection (0 → 26). Two observed combat snapshots displayed
  approximately 39–41 game FPS at the runtime's default internal scale 2;
  this is an observation, not a complete performance benchmark.

The installed application is under `~/.local/opt/GodOfWar2PC-0.1.0` and its Linux
application-menu shortcut points there. Moving from the build folder to the
installed folder reused the completed executable without changing the preparation
log or recompiling. Disc data remains in `~/.local/share/GodOfWar2PC`.

## ROM-free checks

- 18 tests: archive bounds/traversal/links/duplicates; dual-layer relative LBAs;
  wrong revision and cyclic directories; import rollback; corrupt executable
  cache; frozen GUI environment isolation; shader rollback and preferences.
- Portable GCC compiles and links an original synthetic fixture against the
  shipped runtime. The resulting program runs independently of a system SDK.
- The actual native memory-card class writes a synthetic page, closes and
  reopens it in another process: an 8,650,752-byte raw card is preserved.
- Native fullscreen, fit/stretch pixels, window restoration, valid/invalid GLSL
  and shader disable passed at 1920×1080 and 2560×1080.
- The actual GS worker created an OpenGL 4.6 context on the Intel GPU, shared a
  synthetic texture with the host and presented the exact expected pixel values.
- The frozen IOP and VU1 generators ran offline in a read-only Ubuntu container
  with no Python, GCC or CMake installed.
- The distribution archive was inspected for game files and includes runtime
  source, dependency notices and the portable compiler.

## Limits of this validation

This is an **experimental Linux release**. The entire campaign, later bosses,
all graphics effects and all disc-streaming paths have not been exercised.
Upstream rendering and performance limitations may remain. No physical controller
was connected; the controller implementation is retained, while the game tests
used scripted pad input. Native card persistence was tested with synthetic data;
the in-game save-point menu and loading a full campaign save were not tested.
Do not read the successful startup/combat tests as proof of full-game compatibility.
