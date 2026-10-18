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

### Fase 4: cinemáticas (bloqueo 17), velocidad medida y adiós al TEMPORAL (cxldsakeb, 2026-10-17, tarde)

**Tras el `git pull`:** `git submodule update --init --recursive` (submódulo en **`6d3379b`**), **`recompile.ps1`** con `GOW_VU1_MICROCODIGO` y `GOW_IOP_MODULOS` puestas (cambió la traducción de `movn`/`movz`: hay que regenerar `generated/`) y compilar el runner.

- **Bloqueo 17, las cinemáticas:** no se veían ni la inicial ni la de tras el primer QTE contra el Coloso. Sus vídeos llevan audio **ADPCM** y el runtime solo se lo entregaba al juego si lo registraba como PCM (el caso de GoW1): el reproductor esperaba el primer paquete de audio y dejaba la película. Arreglado en el fork (`ac9dfaf`). Además, las películas iban a ~0,57× por una espera activa de libmpeg: override de `0x190A08` con `catchUpToHost`, como GoW1. **La cinemática inicial se ve entera, a 30 imágenes por segundo y con el audio a la par.** La de tras el QTE, por confirmar jugando. **Su audio era un pitido** (bloqueo 18): el runtime no hacía el DMA del scratchpad en modo entrelazado, con el que el juego separa los dos canales; arreglado en el fork (`6d3379b`), falta oírlo.
- **Velocidad real, por fin medida:** `GOW_PERF_DIAG=1` + `GOW_PERF_FRAME_PC=0x14E380` → `guest_flip_hz`. En el PC de cxldsakeb: **60 FPS en el menú y ~44–59 en la primera zona con combate**. El **F10 ya marca esos FPS reales** (`[G]`, fork `19c65a5`). Lo que limita: el juego envía cada frame y espera su FINISH antes de preparar el siguiente, así que EE (~12 ms) y VU1 (~12 ms) van en serie; `Release` y una cola del GS mayor no cambian nada (NOTES.md, "Estado del repositorio").
- También en el fork: `SetupThread` da `argc`/`argv` como una consola (**ya no queda ningún TEMPORAL**), `movn`/`movz` escriben solo 64 bits (como el R5900) y la memory card deja su archivo abierto entre escrituras (guardar debería ir más rápido; **falta probarlo guardando**).
- Pendientes de imagen, sin cambios (NOTES.md, "Estado del repositorio"): línea azul con Kratos, líneas onduladas bajo el agua, fantasma a la izquierda de Kratos (para DaniSBR1).

### Fase 4: memory card, guarda y carga (bloqueo 16) (cxldsakeb, 2026-10-17)

**Tras el `git pull`: hay que regenerar** (`recompile.ps1` con `GOW_VU1_MICROCODIGO` y `GOW_IOP_MODULOS` puestas, ~1 min) y compilar el runner (~14 min en el PC de cxldsakeb): cambió `config/funcmap.csv`.

- Guardar no llegaba a escribir la tarjeta (y pedía formatear en cada arranque): el `switch` de libmc2 que reparte sus operaciones (`0x2B8CC0`) estaba partido en dos funciones en `funcmap.csv`, y sus casos daban `missing-target`. Unido con `tools/find_split_switches.py --fix` (solo había este caso). **Comprobado jugando: formatea, guarda (~10–20 s la primera vez) y carga sin pedir formatear.** Además, la tarjeta reabre su archivo de 8 MB en cada página escrita: lento (NOTES.md, "Memory card: rendimiento").
- Pendientes para DaniSBR1 en NOTES.md ("Estado del repositorio"): línea azul con Kratos, líneas onduladas bajo el agua, cinemáticas que se saltan (la inicial y la de tras el primer QTE contra el Coloso), fantasma a la izquierda de Kratos.

### Fase 4: la sala ya se ve entera (bloqueo 15) y PCSX2 como referencia (cxldsakeb, 2026-10-17)

**Tras el `git pull`:** `git submodule update --init --recursive` (submódulo en **`eb5657e`**) y `cmake --build` (solo cambia `gs_hw_backend.cpp`).

- **Bloqueo 15:** paredes, techos y NPC a más de unos metros desaparecían detrás del cielo. El juego borra el Z dibujando sprites con **formato de Z en la memoria del Z** (y con su propio `ZBUF` para la mitad inferior), y el backend los descartaba. Arreglado en el fork; la primera zona se ve como en la PS2 (ventanales con el Coloso).
- **PCSX2 portátil como referencia** y `tools/gsdump/` para leer sus volcados del GS (Z por dibujo, simulación del buffer de Z). Cómo montarlo en NOTES.md, "PCSX2 para comparar".
- **Pendientes nuevos** (NOTES.md, "Estado del repositorio"): línea azul que acompaña a Kratos (para DaniSBR1), la cinemática inicial de la traición que no aparece, el fantasma a la izquierda de Kratos y el borde tenue del horizonte lejano.

### Fase 4: VU1 e IOP nativos, de ~3 a ~35–40 FPS estimados (cxldsakeb, 2026-10-16, noche)

- **Cada uno tiene que capturar y generar en su PC** (SETUP.md, 7b; los datos son del juego y no se suben): 4 min de partida con `GOW_VU1_CAPTURA` + `GOW_VU1_NATIVE_OFF=1` + `PS2X_IOP_DUMP_MODULES`, y después `recompile.ps1 -SyncOnly` con `GOW_VU1_MICROCODIGO` y `GOW_IOP_MODULOS` puestas (~20 min). Sin esto el juego va a ~3 FPS: la VU1 interpretada se lleva el 97 % del tiempo.
- Hizo falta arreglar `tools/iop/iopn_gen.py` (etiquetas de *delay slot* en LIBSD y 989NOMID de GoW II). Mediciones en NOTES.md, "VU1 e IOP nativos en GoW II".
- **Siguiente para la velocidad:** medir FPS reales con `GOW_PERF_FRAME_PC` (falta la dirección del *flip* de GoW II). Con la velocidad ya razonable, se puede volver a los gráficos.

### Fase 4: los contornos azules son del escalado (cxldsakeb, 2026-10-16, noche)

- **Contornos azules:** solo salían a escala 2. **La línea azul brillante está resuelta (bloqueo 14, fork `2b20dc4`)**: el bloom pondera por el alfa del frame, y una lectura reinterpretada lo dejaba a resolución PS2. Tras el `git pull`: `git submodule update --init --recursive` y `cmake --build` (solo cambia `gs_hw_backend.cpp`, unos minutos). **Queda** un borde tenue solo en el horizonte lejano, que ya está antes del bloom (sospecha y método en NOTES.md, "Estado del repositorio"). Lo que PCSX2 activa para GoW II está en NOTES.md, "Opciones de PCSX2 para God of War II".
- **No te fíes del contador F10:** en el PC de cxldsakeb marca 60 FPS y velocidad 100 %, pero el juego va a ~2 FPS. La velocidad se mira después de los gráficos.
- **Memory card:** MC2_D, SIO2D y SIO2MAN son idénticos a los de GoW1, así que sus sustituciones del IOP valen.

### Compilación más rápida (DaniSBR1, 2026-10-16)

**Tras el `git pull`:** `git submodule update --init --recursive` (submódulo en **`a50613b`**). No hace falta nada más: CMake se reconfigura solo y activar `/MP` no recompila nada.

- **`/MP`** (fork): el runner y el runtime se compilan con varios `cl.exe` a la vez; antes, de uno en uno. El número sale de la RAM y los núcleos de cada PC (al configurar: `PS2X: MSVC parallel compilation /MPn`; se cambia con `-DPS2X_MSVC_MP_PROCESSES=N`). Recompilación completa en el PC de DaniSBR1: **40 min 24 s → 21 min 13 s**.
- **`recompile.ps1`** copia al runner solo los archivos cuyo contenido cambió: un `-SyncOnly` sin cambios ya no recompila nada, y un override cambiado recompila un solo bloque (~20 s).
- Mediciones y detalles en NOTES.md, "Compilación más rápida". Con esto ya no hace falta copiar el override a mano para iterar rápido: `recompile.ps1 -SyncOnly` + `cmake --build` hace lo mismo.

### Fase 4: cielo arreglado, F12/F10 y velocidad medida (DaniSBR1, 2026-10-16, tarde)

**Tras el `git pull`:** `git submodule update --init --recursive` (submódulo en **`3b71410`**), `cmake -S PS2Recomp -B out/build` (opciones nuevas de raylib) y **recompilar el runner entero** (~45 min: cambió `gs_frontend.h`, que lo incluye todo).

**Hecho:** bloqueo 13 (un polígono azul sobre el cielo: la copia Z16 de las sombras leía el target de profundidad equivocado, fork `d0b6162`). **F12** = pantalla completa sin bordes / ventana; **F10** = contador de FPS como el de PCSX2 (`GOW_SHOW_FPS=1` lo muestra al arrancar). Con él se midió la primera zona en el PC de DaniSBR1: **~58–60 FPS reales y velocidad 100 %** (`FPS … [B]`: GoW II copia cada frame a un único búfer de pantalla). Se desactivó la captura F12 de serie de raylib, que dejaba `screenshotNNN.png` (imágenes del juego) en la raíz del repo.

**Siguiente:** los contornos azules finos contra el cielo a escala 2 (comparar la misma escena a `PS2X_GS_HW_SCALE=1`), el "fantasma" de Kratos, y revisar combate, HUD y guardar/cargar jugando. Detalle en NOTES.md, "Estado del repositorio".

### Fase 4: el escenario ya no sale verde (DaniSBR1, 2026-10-16)

**Para el Claude de quien siga: lee primero "Estado del repositorio" en `docs/NOTES.md`.** Tras el `git pull`: `git submodule update --init --recursive` (el submódulo apunta a **`8a4abab`**) y recompilar el runner (solo cambia `gs_hw_backend.cpp`: `cmake --build`, unos minutos, si `generated/` ya está al día).

**Hecho, bloqueo 12:** el backend del GS descartaba las copias por columnas con las que el juego restaura el canal verde, porque **acaban en una fracción de píxel** (`y = 895.4375`) y `tryShuffle` exigía extremos enteros. La máscara parcial que se sospechaba no era la causa (`0x3FFF` cubre bytes enteros). Arreglado en el fork: el escenario sale bien y se dibuja la sombra de Kratos (comprobado con captura). Las fases 2 y 3 están cerradas en el ROADMAP.

**Siguiente:** (1) un polígono azul translúcido en el cielo; (2) la copia Z16 → framebuffer de las sombras, que el backend aún descarta (`shuffleWhy=13`); (3) la velocidad (VU1 e IOP nativos). Método para el GS: un contador local de los motivos de rechazo del backend (no se sube al fork), descrito en NOTES.md. DeepSeek ayuda a leer PCSX2 y el backend (material público), nunca capturas ni logs del juego.

### Fase 2, inicialización superada: el juego carga sus recursos y dibuja (cxldsakeb, 2026-10-16)

**Para el Claude de quien siga: lee primero "Estado del repositorio" en `docs/NOTES.md`.** Tras el `git pull`: `git submodule update --init --recursive` (el submódulo apunta a `9ccffe1`) y recompilar el runner (si `generated/` ya está al día, `cmake --build`, unos minutos).

**Hecho:** bloqueos 8 y 9, los dos en el runtime.
- **8:** el registro `Count` del EE no avanzaba, y el cargador de recursos (`0x18CF70`, con un presupuesto de tiempo por frame) no terminaba nunca.
- **9:** en GoW II no se aplicaba **ninguna** sustitución nativa del IOP (LIBSD, memory card, SIO2). El juego nombra los módulos `NAME.IRX;1` y su LIBSD es otra compilación, con otros desplazamientos. Sin ellas, subir un banco de sonido esperaba un DMA de la SPU2 que no existe.

Ahora el juego carga (`R_PermA`, `SBP_general`…), entra en su bucle principal y el GS dibuja de forma estable. El protocolo con SMPD y el cargador están en NOTES.md ("Cargador de recursos y protocolo con SMPD"). Herramientas nuevas en el override: `GOW2_CALL_PROFILE` (funciones más llamadas) y `GOW2_PEEK` (variables en memoria).

**Después (bloqueo 10):** salía el aviso "no hay mando conectado". Se arregló con el override de `libpad2` (adaptado de GoW1, direcciones de GoW II verificadas). Sin mando funciona el teclado: `Intro` = Start, `X` = cruz, flechas, `WASD`… (el mapa completo está en NOTES.md, "Estado del repositorio").

**Fase 3: se llega al gameplay, con sonido** (cxldsakeb, 2026-10-16). El escenario sale **verde** (bloqueo 12, abierto). El juego cuenta las sombras de los personajes en el canal verde y lo restaura con copias de 16 bits con máscara de bits que el backend del GS (`tryShuffle`) aún no hace; el diagnóstico completo, con capturas que lo comprueban, está en NOTES.md. **Siguiente tarea: implementar esas copias en `gs_hw_backend.cpp` (fork).** Un apaño desde el juego (anular `0x16EDB8`) deja la pantalla negra: descartado.

**Antes:** sale la pantalla de título completa (a pocos FPS), el menú responde y, al empezar partida nueva, se reproduce el primer vídeo. Para eso hizo falta el bloqueo 11: el stub genérico de `sceIpuInit` del runtime lleva direcciones de otro juego, y se sustituyó por un override con las de GoW II. **Siguiente:** confirmar que el vídeo se ve bien y llegar al gameplay. Sin teclado, `GOW2_PAD_SCRIPT=15:start,20-60/3:x` (NOTES.md, "Diagnóstico de bloqueos"). Vigilar el sonido, que ahora pasa por las sustituciones de LIBSD de GoW II.

### Fase 2: bloqueos 6 y 7 resueltos en el runtime; el juego pide recursos a SMPD (cxldsakeb, 2026-10-16)

**Para el Claude de quien siga: lee primero "Estado del repositorio" en `docs/NOTES.md`.** Tras el `git pull`: `git submodule update --init --recursive` (el submódulo apunta a `df2a082` de `godofwar2`) y recompila el runner (si `generated/` ya está al día con el `funcmap.csv` de la fase 2, basta `cmake --build`, ~2 min; si no, `recompile.ps1` y ~25–50 min).

**Hecho:** la rama `godofwar2-ioman` de Daniel funciona (SMPD lee el TOC entero) y está fusionada en `godofwar2`. Bloqueo 7: SMPD troceaba sus rutas con `strtok` (el runtime devolvía 0, `files = NULL`) y usaba `GetSystemTimeLow`, que no existía. Arreglados en el fork; además, los imports del IOP sin atender ya dejan siempre el aviso `[IOP] unhandled import`. **Método nuevo:** `tools/irx_imports.py` cruzado con el catálogo de lo que implementa el runtime (NOTES.md, "Imports del IOP que usa GoW II"): predijo el bloqueo 7 antes de ejecutar.

**~~Siguiente: bloqueo 8~~ hecho, ver arriba.** El EE pide recursos a SMPD (`R_LglScA…`, `R_PermA…`), SMPD lee 384 KB de `PART1.PAK` y los dos se quedan esperando; el hilo 1, en el bucle de `0x18CF70`. Detalles y trazas útiles (`PS2X_IOP_TRACE`, `PS2X_TRACE_RPC=123`) en NOTES.md.

### Fase 2 en marcha: arranca, carga los IRX y empieza a leer el disco (DaniSBR1, 2026-10-16)

**Para el Claude de quien siga: lee primero "Estado del repositorio" en `docs/NOTES.md`.** Resumen:

**Hecho (en `main`):** el juego arranca, carga sus 8 IRX en el intérprete del IOP (`SIO2MAN`, `DBCMAN`, `SIO2D`, `MC2_D`, `DS2U_D`, `LIBSD`, `989NOMID`, `SMPD`), se enlaza a sus servidores RPC y empieza a leer el disco. Cinco bloqueos resueltos, todos en `overrides/gow2_overrides.cpp` (registrado para `SCUS_974.81`):
1. `sceCdReadDvdDualInfo` (`0x2B6BF0`), como en GoW1.
2. **TEMPORAL** `0x1875E0`: el `SetupThread` del runtime ignora el bloque de argumentos y `main` recibe `argc = 0`; las rutas de los IRX salían vacías. Arreglo de verdad: que el runtime rellene los argumentos (propuesta en NOTES.md).
3. `setPrivateGuestHeap(0xA0000, 0xFF000)`: el heap interno del runtime pisaba el heap principal del juego (como en GoW1).
4. Puntos de reanudación: `ps2_recomp` no los registra en las funciones de `entry_points` (las llama `entry_XXXX`). **Las 733 funciones del escáner van ahora en `config/funcmap.csv`** (`tools/make_funcmap.py --extra`), no en `entry_points`.
5. `sceSifLoadStartModuleBuffer` (`0x2D4850`): el módulo del IOP que va dentro del ELF (como en GoW1).

**~~Siguiente paso (pendiente de probar)~~ hecho, ver arriba:** el bloqueo 6. SMPD lee `cdrom0:\GODOFWAR.TOC` con `ioman` (`open`/`read`/`lseek`/`close`) y el IOP del runtime no lo implementaba: carga un índice vacío y el juego se cuelga en `0x27D7E8` (pila de estados vacía). **La implementación está en la rama `godofwar2-ioman` del fork** (`b14b951`, sin fusionar en `godofwar2`): compila, pero no se llegó a probar en el juego. Para probarla:
1. `git pull`, `git submodule update --init --recursive` y luego `git -C PS2Recomp fetch` + `git -C PS2Recomp switch godofwar2-ioman`.
2. `.\scripts\recompile.ps1` (hay que regenerar: `funcmap.csv` cambió) y `.\scripts\run.ps1 -Config RelWithDebInfo` (~50 min la primera vez).
3. En el log, sin `unhandled import ioman`. Con `GOW2_THREAD_WATCH=5`, el hilo 1 ya no debe quedarse en `pc=0x0027d7e8`.
4. Si funciona: fusionar `godofwar2-ioman` en `godofwar2` del fork, push, y commit del nuevo puntero del submódulo aquí (regla 5). Si no: el siguiente bloqueo, con el mismo método.

Para investigar: `GOW2_THREAD_WATCH=5` y `GOW2_BOOT_TRACE=1` (NOTES.md, "Diagnóstico de bloqueos", que explica también cómo probar un override en ~1 min sin recompilar todo). DeepSeek solo sirve para leer material público (el runtime, `referencia/gow1/`): NOTES.md, "DeepSeek en la fase 2".

### Fase 1 cerrada: primera configuración que recompila y compila (DaniSBR1 y cxldsakeb, 2026-10-15)

**Hecho:** fase 0 de DaniSBR1 (mismo SHA1). Ghidra → 5 874 funciones; `tools/make_funcmap.py` → `config/funcmap.csv` (solo funciones: el export trae además 31 986 etiquetas internas que no deben ir); escáner → 733 funciones más (al principio en `entry_points`, como en GoW1; **desde la fase 2 van en `funcmap.csv`**, ver arriba). `recompile.ps1` genera 6 607 funciones **sin errores** y **el runner compila y enlaza** (0 símbolos sin resolver). Arreglados restos de GoW1: `$ConfigToml` sin definir en `_common.ps1` y los scripts de `tools/` apuntando al ELF de GoW1.

**Pendiente para cerrar la fase 1:**
1. ~~Stubs de Sony~~ **hecho: 76 de 79** (los otros 3, `exit`, `read` y `sceTtyRead`, no están enlazados en GoW II). **GoW II usa el SDK 3.0.0**: las librerías están desplazadas en bloque respecto a GoW1 (+0x3C0E8 libmpeg, +0x3C0F0 ipu/graph/dma/cdvd, +0x3BB68 sif/deci2) y cada stub se verificó en el ELF de GoW II. Método en NOTES.md ("Stubs de librerías de Sony"). Ojo: en el SDK 3.0, `StartThread`, `iWakeupThread` e `InitTLB` son funciones en C delante del envoltorio de la syscall, y el stub va en la función en C.
2. ~~Tabla `[mmio]`~~ **hecho: 240 entradas** de `ps2_analyzer`, equivalentes a la tabla de GoW1 (que era la salida del analizador sin retocar). El analizador, con su base de símbolos de Sony, también confirma los 76 stubs en las mismas direcciones. Ver NOTES.md ("Tabla [mmio] y contraste con ps2_analyzer").
3. Cada `recompile.ps1` regenera todo `generated/` y el runner tarda ~50 min en recompilarse entero: juntar cambios antes de regenerar.

~~Falta confirmar que el runner compila con la tabla `[mmio]`~~ **Fase 1 cerrada** (cxldsakeb, 2026-10-15): con stubs y `[mmio]`, el runner `RelWithDebInfo` compila y enlaza sin `LNK2001`/`LNK2019`/`LNK4088`. Ojo: el runner se enlaza con `/FORCE` y siempre sale un `.exe`; lo que vale es que el log del enlazador no tenga esos errores.

**Siguiente: fase 2** (primer arranque), ya en marcha (ver arriba). Vigilar los 8 `sceSif*` que `ps2_recomp` convierte en stub por nombre.

### Traspaso a DaniSBR1: fase 1, mapa de funciones y primera configuración (cxldsakeb, 2026-10-15)

cxldsakeb cerró la fase 0 por su parte y le pasa la fase 1 a Daniel. Claude de Daniel: guíale paso a paso y explica el porqué.

**Hecho (fase 0, cxldsakeb):** repo creado (scripts y herramientas de GoW1, submódulo en `godofwar2`, `referencia/gow1/`), disco extraído, **ELF de referencia en NOTES.md** (`SCUS_974.81` v1.01, SHA1 `E6A62A0F3E781C29513F563F4625B9B3A9103C56`) y `build-tools.ps1` compilando. Hallazgos: el disco tiene la misma estructura que GoW1 (solo cambia `SMPD.IRX`) y los microprogramas de la VU van dentro del ELF (`.DVP.overlay`).

**Pasos para Daniel, en orden:**
1. Aceptar la invitación al repo y clonarlo con submódulos (`docs/SETUP.md`, pasos 1–5). Con su ISO, `.\scripts\extract-iso.ps1` y **comprobar que el SHA1 coincide**. Si no coincide (por ejemplo, una v1.00), parar y avisar: las direcciones no valdrían.
2. Ghidra (SETUP, paso 6) y `.\scripts\ghidra-analyze.ps1` → `config/gow.toml` y `config/gow_functions.csv`. Después, `py -3 tools/scan_missing_functions.py` para las funciones que Ghidra no ve (en GoW1 fueron 634, sobre todo métodos virtuales).
3. **`config/funcmap.csv`**: el CSV de Ghidra (mismo formato, `Name,Start,End,Size`) más las funciones del escáner.
4. **`config/recomp.template.toml`**, tomando como modelo `referencia/gow1/config/recomp.template.toml`:
   - `[general]` con `input = "@ELF@"`, `ghidra_output = "@MAP@"`, `output = "@OUT@"` (los rellena `recompile.ps1`);
   - **stubs**: cada función de librería de Sony de la plantilla de GoW1 (`sceMpeg*`, `sceIpu*`, `sceGs*`, `sceCd*`, `sceDma*`, libkernel…) hay que **encontrarla en GoW II** y poner su dirección. Pistas: cadenas cercanas, el patrón de instrucciones de la función en GoW1 (`tools/elf_dis.py`), y quién la llama (`tools/elf_xref.py`). Una lista que no se encuentre, mejor dejarla fuera y anotarla en NOTES.md que poner una dirección dudosa;
   - `[mmio]`: igual, los accesos a registros del hardware, buscando en GoW II los equivalentes de GoW1. Puede ir después del primer arranque si cuesta.
   - **DeepSeek puede ayudar** a cruzar listas (por ejemplo, la tabla de stubs de GoW1 contra un listado de funciones de GoW II que solo contenga nombres y patrones públicos), pero **nunca con desensamblado ni datos del ELF** (regla 1).
5. `.\scripts\recompile.ps1` hasta que genere C++ **sin errores**, y compilar el runner: es el criterio de "hecho" de la fase 1. Apuntar en NOTES.md las funciones identificadas y los problemas, y actualizar esta sección.

Lo de GoW1 que no se ha llevado aquí: sus overrides se rehacen con direcciones de GoW II solo cuando un bloqueo lo pida (fase 2).
