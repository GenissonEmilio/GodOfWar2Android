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
- [x] Funciones que Ghidra no ve (`scan_missing_functions.py`). (DaniSBR1: 733; desde la fase 2 van en `funcmap.csv`)
- [x] `config/funcmap.csv` (`Name,Start,End,Size`) y `config/recomp.template.toml` con `@ELF@`, `@MAP@` y `@OUT@`, tomando como modelo los de `referencia/gow1/config/`. (DaniSBR1: primera versión; `recompile.ps1` genera C++ sin errores)
- [x] Stubs de las librerías de Sony (`sceMpeg*`, `sceIpu*`, `sceGs*`, `sceCd*`, `sceDma*`, libkernel…): localizar cada una en GoW II (las cadenas y los patrones de llamada ayudan; DeepSeek puede cruzar listas). (DaniSBR1: 76 de 79; los otros 3 no están enlazados en GoW II. SDK 3.0.0, ver NOTES.md)
- [x] Tabla `[mmio]`: los accesos del juego a registros del hardware que el runtime convierte en llamadas nativas. (DaniSBR1: 240 entradas de `ps2_analyzer`, equivalentes a la tabla de GoW1; cxldsakeb confirmó que el runner compila y enlaza con ella)

**Hecho cuando:** `recompile.ps1` genera C++ sin errores y el runner compila. **Hecho** (2026-10-15): con stubs y `[mmio]`, `recompile.ps1` sin errores y el runner `RelWithDebInfo` enlaza sin símbolos sin resolver (cxldsakeb).

### Fase 2: primer arranque — ✅ hecha (2026-10-16)

**Método** (el de GoW1): `.\scripts\run.ps1 -Config RelWithDebInfo -NoBuild`, leer el **primer** error del log en `out/logs/`, arreglarlo y repetir. Cada bloqueo va a la tabla "Bloqueos" de [NOTES.md](NOTES.md). Los overrides de GoW II van en `overrides/gow2_overrides.cpp` y se registran con su propio `PS2_REGISTER_GAME_OVERRIDE` (`"SCUS_974.81"`, entrada `0x00100008`): los de GoW1 no se activan aquí, porque están atados a `SCUS_973.99`.

- [x] **Cada uno compila su runner** (`recompile.ps1` + `run.ps1`, ~40–50 min la primera vez). El `.exe` y `generated/` no se suben (regla 1), así que no se pueden compartir. (cxldsakeb y DaniSBR1)
- [x] **Arranca y carga los 8 IRX** en el intérprete del IOP (`SIO2MAN`, `DBCMAN`, `SIO2D`, `MC2_D`, `DS2U_D`, `LIBSD`, `989NOMID`, `SMPD`) y se conecta a sus servidores. Bloqueos 1–2: `sceCdReadDvdDualInfo` y el **TEMPORAL** de `0x1875E0` (el runtime le pasa `argc = 0` a `main`). (DaniSBR1)
- [x] **Pasa la inicialización y lee el disco.** Bloqueos 3–5 (DaniSBR1): el heap del runtime pisaba el del juego, los puntos de reanudación de las funciones del escáner (ahora en `funcmap.csv`) y el módulo del IOP que va dentro del ELF. Bloqueos 6–9 (DaniSBR1 y cxldsakeb, en el fork): `ioman` para leer `GODOFWAR.TOC`, `strtok`/`GetSystemTimeLow` de SMPD, el registro `Count` del EE y las sustituciones nativas del LIBSD de GoW II.
- [x] **Overrides imprescindibles**, con direcciones de GoW II: `sceIpuInit` (bloqueo 11) y `libpad2`, el mando (bloqueo 10; sin mando, el teclado). No han hecho falta `DelayThread` ni el timer 2.
- Resuelto sin override: `sceSifInitRpc` e `iWakeupThread`. En GoW1 el recompilador las descartaba; en GoW II Ghidra las detecta.

**Hecho cuando:** llega a la primera pantalla (logos). **Hecho:** pantalla de título completa.

### Fase 3: menú y vídeos — ✅ hecha (2026-10-16)

- [x] El menú responde (con el mando, el teclado o `GOW2_PAD_SCRIPT`).
- [x] Al empezar partida nueva se reproduce el primer vídeo (bloqueo 11, `sceIpuInit`). (cxldsakeb)

**Hecho cuando:** se llega al menú y la intro se reproduce.

### Fase 4: jugable — en curso

Se llega a la **primera zona jugable (Rodas), con sonido** (cxldsakeb, 2026-10-16). Lo que falta:

- [x] **Bloqueo 12, el escenario verde.** El juego cuenta las sombras de los personajes en el canal verde del framebuffer y lo restaura con copias por columnas de 8 px a través de una vista de 16 bits. Esas copias acaban en una fracción de píxel (`y = 895.4375`) y el backend del GS las descartaba. Arreglado en el fork (`8a4abab`, DaniSBR1, 2026-10-16): el escenario sale bien y **se dibuja la sombra de Kratos**.
- [x] **Bloqueo 13, el polígono azul del cielo:** la copia Z16 → framebuffer leía el target de profundidad equivocado. Arreglado en el fork (`d0b6162`, DaniSBR1, 2026-10-16).
- [ ] Otros fallos de imagen: la línea azul que acompaña a Kratos, las líneas onduladas bajo el agua, el "fantasma" a la izquierda de Kratos y el borde tenue del horizonte lejano (la línea azul brillante del bloom y la geometría lejana ya están: bloqueos 14 y 15). Detalle en NOTES.md, "Estado del repositorio".
- [ ] **Velocidad completa.** El contador F10 no sirve (marca 60 siempre). Medida de verdad con `GOW_PERF_DIAG=1` y `GOW_PERF_FRAME_PC=0x14E380` en el PC de cxldsakeb, con VU1 e IOP nativos (sección 7b de [SETUP.md](SETUP.md)): **60 FPS en el menú y ~44–59 en la primera zona con combate**. Ahora manda el hilo del GS (y la VU1), no el EE. Detalle en NOTES.md, "Estado del repositorio".
- [x] **Controles de la ventana** (DaniSBR1, 2026-10-16, fork `cf4e3f2`/`3b71410`): F12 pantalla completa sin bordes / ventana; F10 contador de FPS como el de PCSX2.
- [ ] Partida, combate y HUD, revisados en pantalla.
- [x] Guardar y cargar (memory card): bloqueo 16 (cxldsakeb, 2026-10-17). Formatea, guarda y carga.
- [x] **Cinemáticas** (bloqueo 17, cxldsakeb, 2026-10-17): el audio ADPCM de las películas no llegaba al juego y el reproductor las dejaba; además iban a ~0,57×. La inicial se ve entera; la de tras el QTE contra el Coloso, por confirmar jugando.
- [x] Retirar el TEMPORAL de `0x1875E0`: `SetupThread` rellena los argumentos en el runtime (fork `5b1a25a`, cxldsakeb, 2026-10-17).

**Hecho cuando:** se juega el primer nivel a velocidad completa.
