# Contexto para Claude: port nativo de God of War II (PS2 → PC)

Este archivo lo carga Claude Code automáticamente. Es el **hilo compartido** entre las sesiones de Claude de todo el equipo: lo que no esté aquí o en `docs/` no lo sabe el Claude de los demás.

## Proyecto

- **Objetivo:** recompilar estáticamente God of War II (PS2, 2007) a un ejecutable nativo de Windows con [PS2Recomp](https://github.com/ran-j/PS2Recomp).
- **Viene de GoW1:** el repo [cxldsakeb/ROMS-nativas-PS2](https://github.com/cxldsakeb/ROMS-nativas-PS2) ya es jugable a ~60 FPS sobre el **runtime de Waffle565** (GS nativo, VU1 e IOP compilados por adelantado, sonido, mando, vídeos, memory card). GoW II usa una evolución del mismo motor: partimos de ese runtime y lo que hay que hacer aquí es **lo propio del juego** (mapa de funciones, configuración del recompilador, overrides) y los arreglos del runtime que pida.
- **Submódulo `PS2Recomp/`:** nuestro fork [DaniSBR1/PS2Recomp](https://github.com/DaniSBR1/PS2Recomp), **rama `godofwar2`** (sale de `godofwar` en `083a286`, la base de Waffle565). Es una rama distinta de la de GoW1 para que los cambios de un juego no rompan el otro. Si un arreglo vale para los dos, se hace en una y se lleva a la otra con `cherry-pick`.
- **Equipo:** @cxldsakeb (dueño del repo), @DaniSBR1 y @Alejrr2. Están aprendiendo: Claude hace de guía, así que explica el porqué de las cosas y no solo el qué.
- **Idioma:** todo en español (conversación, docs, comentarios y commits). Excepción: los comentarios de código dentro del fork de PS2Recomp van en inglés.
- **Repo:** privado por ahora (GPLv3). Si se hace público, la regla 1 importa todavía más.

## Reglas que no se rompen

1. **Nunca se sube nada del juego:** ni ISO, ni ELF, ni archivos del disco, ni el C++ de `generated/`, ni las capturas de la VU1 o del IOP (`out/capturas/`), ni imágenes del juego. Todo eso es local y está en `.gitignore`. Tampoco se le pasa a servicios externos (DeepSeek incluido).
2. **Las direcciones solo valen para el ELF de referencia**, identificado por el SHA1 de `docs/NOTES.md`. **Las de GoW1 no valen aquí**: `referencia/gow1/` es para entender qué hace cada cosa, no para copiar direcciones.
3. **Todo hallazgo va a `docs/NOTES.md`:** funciones identificadas, bloqueos y soluciones. Al terminar una tarea, actualízalo en la misma rama.
4. **Los parches temporales** (`ret0`/`ret1`, apaños) llevan un comentario `TEMPORAL` en el código y una fila en NOTES.md.
5. **Cambios en `PS2Recomp/`:** primero intenta arreglarlo con un override (`overrides/`). Si hay que cambiar el runtime: commit en la rama `godofwar2` **dentro** de `PS2Recomp/`, push a `DaniSBR1/PS2Recomp` y luego commit del nuevo puntero del submódulo aquí. **Nunca subas trazas de diagnóstico al fork.**
6. **Licencias:** del repo de Waffle565 solo usamos su rama `runtime` (GPL-3.0) y lo que KIexster redistribuye de su `main`. Su `main` no tiene licencia (pedida en [su issue #2](https://github.com/Waffle565/God-Of-War-Native-Port/issues/2)): se puede **leer y aprender** de su documentación, pero no copiar su código ni su texto. Todo lo adaptado cita su origen.

## Estructura y flujo

```
scripts/extract-iso.ps1 -IsoPath <iso>   ISO → game/disc/ con .disc_layout e IOP_MOD/ (imprime el SHA1)
scripts/build-tools.ps1                  compila ps2_analyzer y ps2_recomp (Release, VS 2022)
scripts/ghidra-analyze.ps1 [-Reanalyze]  Ghidra headless (r5900) → config/gow.toml + config/gow_functions.csv
scripts/recompile.ps1 [-SyncOnly]        config/recomp.template.toml + config/funcmap.csv → generated/ → runner; reconfigura CMake
scripts/run.ps1 [-Config] [-NoBuild]     compila ps2EntryRunner y arranca; log en out/logs/
scripts/deepseek.ps1 -Tarea -Archivos    tarea de solo lectura para DeepSeek (ver más abajo)
tools/                                   extractor del disco, generadores de la VU1 y el IOP, desensambladores (tools/README.md)
referencia/gow1/                         configuración y overrides de GoW1 (KIexster/Waffle565), solo como modelo
```

- **`config/` está vacío al empezar.** `recompile.ps1` espera `config/recomp.template.toml` (con `@ELF@`, `@MAP@` y `@OUT@`) y `config/funcmap.csv` (`Name,Start,End,Size`). Construirlos es la fase 1 (ver `docs/ROADMAP.md`): Ghidra da el mapa de funciones, y la plantilla de GoW1 (`referencia/gow1/config/recomp.template.toml`) sirve de modelo para los stubs y la tabla `[mmio]`, **buscando las direcciones equivalentes en el ELF de GoW II**.
- **Ghidra:** 12.1.4 + extensión ghidra-emotionengine-reloaded (`r5900:LE:32:default`) + JDK 21. El proyecto vive en `ghidra/` (ignorado). Ghidra no ve las funciones que solo se llaman por puntero (métodos virtuales): después de cada export, `py -3 tools/scan_missing_functions.py`.
- **IOP y VU1 nativos** (la gran ganancia de velocidad en GoW1, de 7–8 a ~60 FPS): se generan con datos del juego capturados en local. Ver la sección 7b de `docs/SETUP.md`. Para capturar la VU1 hace falta `GOW_VU1_NATIVE_OFF=1` y que la carpeta exista.
- **Al copiar overrides a mano, actualiza su fecha** (`(Get-Item $destino).LastWriteTime = Get-Date`): si no, MSBuild puede no recompilarlos.
- **Unity build:** varios overrides comparten unidad de compilación. Prefijos por archivo (`kIop…`, `g_iop…`) y que `PS2_REGISTER_GAME_OVERRIDE` no caiga en la misma línea que en otro override (usa `__LINE__`).
- Los scripts están en PowerShell 5.1 y se guardan en UTF-8 **con BOM**.
- Si VS Code tiene la extensión C/C++, excluye `generated/` y `PS2Recomp/ps2xRuntime/src/runner/` en `.vscode/settings.json` (`C_Cpp.files.exclude`).
- Método de depuración (el mismo de GoW1): ejecutar, arreglar el **primer** bloqueo del log y repetir. Guía: wiki de PS2Recomp, "PS2Recomp Stripped Game Walkthrough".

## Cómo colaboramos

- Una rama por tarea y PR a `main`. **No hace falta revisión** (acordado el 2026-10-15): quien la abre la fusiona.
- Antes de empezar: `git pull`, `git submodule sync --recursive` y `git submodule update --init --recursive`. Lee `docs/NOTES.md` y `docs/ROADMAP.md`.
- Quien vaya a cambiar el fork (`godofwar2`) lo avisa antes.
- Al terminar: actualiza NOTES.md y, si cambia la fase, la sección **Estado actual** de abajo.

## Tareas para DeepSeek (OpenCode)

Igual que en GoW1: **DeepSeek V4.1 Flash, gratis en OpenCode**, para trabajo de lectura de mucho volumen. Claude: cuando una tarea encaje, propón delegarla, lánzala con `scripts/deepseek.ps1` y **revisa siempre el resultado**.

- Instalación: `npm install -g opencode-ai`. Uso: `.\scripts\deepseek.ps1 -Tarea tarea.md -Archivos <archivos>`. Trabaja en `%TEMP%\gow-deepseek\`, fuera del repo, en solo lectura.
- **Sí:** resumir, clasificar o cruzar material público (PS2Recomp, PCSX2, KIexster, la documentación de Waffle565, nuestro repo de GoW1).
- **No:** nada derivado del juego (regla 1) ni decidir arreglos.
- Tareas con formato de salida exacto (tablas) para poder comprobarlas; las entradas grandes, en trozos de ~70 KB en paralelo.

## Estado actual

> Actualizar esta sección al cerrar cada hito, para que la vean las sesiones de todos.

### Fase 0: entorno (2026-10-15, repo creado por cxldsakeb)

- Repo creado con los scripts y las herramientas de GoW1, el submódulo en `godofwar2` y la configuración y overrides de GoW1 en `referencia/gow1/`.
- **Siguiente:** cada uno extrae su copia de GoW II y apuntamos el **SHA1 del ELF de referencia** en NOTES.md (regla 2); luego, la fase 1 de `docs/ROADMAP.md` (Ghidra y la primera configuración).
