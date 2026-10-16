# Hoja de ruta

## Qué esperar

- **No empezamos de cero.** El runtime ya sabe dibujar el GS, ejecutar la VU1 y el IOP como código nativo, sonar, leer el mando y la memory card y reproducir vídeos: eso es lo que en GoW1 costó semanas. GoW II usa una evolución del mismo motor, así que mucho debería valer tal cual.
- **Lo que sí es nuevo:** el ELF de GoW II no tiene símbolos y sus direcciones son otras. Hay que construir el mapa de funciones, la configuración del recompilador (stubs de librerías de Sony y la tabla `[mmio]`) y los overrides, y arreglar los bloqueos propios del juego con el método de siempre: ejecutar, arreglar el **primer** bloqueo y repetir.
- `referencia/gow1/` es el modelo: qué stubs hicieron falta, qué accesos a registros se convierten en llamadas nativas y qué overrides necesitó GoW1. Para cada cosa hay que encontrar **su equivalente en GoW II**.

## Fases

Cada fase tiene un criterio de "hecho" comprobable.

### Fase 0: entorno (todos) — falta Ale

- [ ] Cada uno sigue [SETUP.md](SETUP.md) hasta `build-tools.ps1`. (cxldsakeb y DaniSBR1: hecho)
- [ ] Cada uno extrae su copia con `extract-iso.ps1` y todos tenemos el **mismo SHA1 del ELF**, apuntado en [NOTES.md](NOTES.md). (cxldsakeb y DaniSBR1: hecho, `E6A62A0F…`)

**Hecho cuando:** los 3 compilamos las herramientas y tenemos el ELF de referencia.

### Fase 1: mapa de funciones y primera configuración — ✅ hecha (2026-10-15)

- [x] Ghidra (`ghidra-analyze.ps1`) → `config/gow_functions.csv` y `config/gow.toml`. (DaniSBR1: 5 874 funciones)
- [x] Funciones que Ghidra no ve (`scan_missing_functions.py`). (DaniSBR1: 733, en `entry_points`)
- [x] `config/funcmap.csv` (`Name,Start,End,Size`) y `config/recomp.template.toml` con `@ELF@`, `@MAP@` y `@OUT@`, tomando como modelo los de `referencia/gow1/config/`. (DaniSBR1: primera versión; `recompile.ps1` genera C++ sin errores)
- [x] Stubs de las librerías de Sony (`sceMpeg*`, `sceIpu*`, `sceGs*`, `sceCd*`, `sceDma*`, libkernel…): localizar cada una en GoW II (las cadenas y los patrones de llamada ayudan; DeepSeek puede cruzar listas). (DaniSBR1: 76 de 79; los otros 3 no están enlazados en GoW II. SDK 3.0.0, ver NOTES.md)
- [x] Tabla `[mmio]`: los accesos del juego a registros del hardware que el runtime convierte en llamadas nativas. (DaniSBR1: 240 entradas de `ps2_analyzer`, equivalentes a la tabla de GoW1; cxldsakeb confirmó que el runner compila y enlaza con ella)

**Hecho cuando:** `recompile.ps1` genera C++ sin errores y el runner compila. **Hecho** (2026-10-15): con stubs y `[mmio]`, `recompile.ps1` sin errores y el runner `RelWithDebInfo` enlaza sin símbolos sin resolver (cxldsakeb).

### Fase 2: primer arranque — en curso

**Método** (el de GoW1): `.\scripts\run.ps1 -Config RelWithDebInfo -NoBuild`, leer el **primer** error del log en `out/logs/`, arreglarlo y repetir. Cada bloqueo va a la tabla "Bloqueos" de [NOTES.md](NOTES.md). Los overrides de GoW II van en `overrides/` y se registran con su propio `PS2_REGISTER_GAME_OVERRIDE` (`"SCUS_974.81"`, entrada `0x00100008`): los de GoW1 no se activan aquí, porque están atados a `SCUS_973.99`.

- [ ] **Cada uno compila su runner** (`recompile.ps1` + `run.ps1`, ~40–50 min la primera vez). (cxldsakeb y DaniSBR1: hecho) El `.exe` y `generated/` no se suben (regla 1), así que no se pueden compartir.
- [x] **Arranca** y carga los IRX en el intérprete del IOP (`IOPRP300.IMG`, `989NOMID`, `SMPD`, `LIBSD`…; en el log, cada módulo sale con su `base=`).
  - **Hecho** (DaniSBR1, 2026-10-16): cargan los 8 IRX (`SIO2MAN`, `DBCMAN`, `SIO2D`, `MC2_D`, `DS2U_D`, `LIBSD`, `989NOMID`, `SMPD`) y se enlazan sus servidores. Hicieron falta dos overrides: `sceCdReadDvdDualInfo` y uno **TEMPORAL** de `0x1875E0`, porque el runtime le pasa `argc = 0` a `main` (ver NOTES.md, "Bloqueos").
  - En GoW1, `sceSifLoadStartModuleBuffer` (un módulo mínimo, "ck01", que va dentro del ELF) necesitó override. En GoW II es `0x2D4850` (verificada); de momento no ha hecho falta.
- [ ] **Pasa la inicialización** y empieza a leer datos del disco (`GODOFWAR.TOC`, `PART*.PAK` a través de `.disc_layout`).
  - Resueltos (DaniSBR1, 2026-10-16): el heap del runtime pisaba el del juego (`setPrivateGuestHeap`), los puntos de reanudación de las funciones del escáner (TEMPORAL) y el módulo del IOP que va dentro del ELF (`sceSifLoadStartModuleBuffer`). El juego ya empieza a leer el disco (SMPD arranca).
  - **Bloqueo abierto:** SMPD lee `cdrom0:\GODOFWAR.TOC` con `ioman` (`open`/`read`/`lseek`/`close`), que el IOP del runtime no implementa: carga un índice vacío y el juego se cuelga en `0x27D7E8`. Hay que implementarlo en el fork (ver NOTES.md, "Bloqueos").
- [ ] **Overrides imprescindibles**, solo cuando el log los pida y con direcciones de GoW II. Candidatos según GoW1: `sceIpuInit` (tablas del IPU), la configuración de pantalla y la interrupción FINISH del GS, `DelayThread` y el timer 2, `libpad2` (mando) y el envío de comandos a 989snd.
- Resuelto sin override: `sceSifInitRpc` e `iWakeupThread`. En GoW1 el recompilador las descartaba; en GoW II Ghidra las detecta y van a los manejadores del runtime.

### Fase 3: menú y vídeos

**Hecho cuando:** se llega al menú y la intro se reproduce.

### Fase 4: jugable

- [ ] Partida, combate, HUD.
- [ ] VU1 e IOP nativos (capturas propias de GoW II).
- [ ] Guardar y cargar.

**Hecho cuando:** se juega el primer nivel a velocidad completa.
