# Notas del proyecto

Memoria compartida del equipo: todo hallazgo va aquí (regla 3 de CLAUDE.md).

## Build de referencia

| Campo | Valor |
|---|---|
| Juego | God of War II |
| Región / serie | USA, SCUS-97481, **versión 1.01** (`VER = 1.01` en `SYSTEM.CNF`), NTSC |
| ELF | `SCUS_974.81`, 2 125 688 bytes |
| SHA1 del ELF | **`E6A62A0F3E781C29513F563F4625B9B3A9103C56`** (cxldsakeb, 2026-10-15; DaniSBR1 lo confirma el mismo día). Ale: comprueba que `extract-iso.ps1` te da el mismo |
| ISO | DVD-9, capa 1 en el sector 2 083 872 |
| Runtime | fork `DaniSBR1/PS2Recomp`, rama `godofwar2` (base de Waffle565, `083a286`) |

Las direcciones de este archivo solo valen para el ELF con ese SHA1.

## Estado del repositorio (cxldsakeb, 2026-10-16, ~11:00)

Foto de dónde lo dejé, para quien siga (y su Claude). Lo de abajo, en "Bloqueos", explica cada paso.

| Qué | Estado |
|---|---|
| Fase 0 | cxldsakeb y DaniSBR1, hecha. Falta Ale |
| Fase 1 | Hecha (mapa de funciones, 76 stubs, `[mmio]`; el runner compila) |
| Fase 2, objetivo 1 (arranca y carga los IRX) | **Hecho**: cargan los 8 IRX y el juego se conecta a sus servidores |
| Fase 2, objetivo 2 (inicialización y lectura del disco) | **Hecho**: 9 bloqueos resueltos. El juego carga sus recursos (`R_LglScA`, `R_PermA`, el banco de sonido `SBP_general`…), entra en su bucle principal y **dibuja**: ~235 000 dibujos del GS por intervalo de `GOW_GS_HW_STATS`, estables. Miles de RPC a SMPD por minuto (lectura continua del disco) |
| Fork, rama `godofwar2` | **`3b71410`**: `ioman` (bloqueo 6), `strtok`/`GetSystemTimeLow` (7), registro `Count` del EE (8), LIBSD de GoW II (9), *channel shuffles* que acaban en fracción de píxel (12, el verde), lectura del Z con el target del ancho correcto (13, el polígono del cielo), y **F12 pantalla completa / F10 contador de FPS** |
| `main` del repo | El submódulo apunta a `3b71410`. Tras el `git pull`: `git submodule update --init --recursive`, **reconfigurar CMake** (`cmake -S PS2Recomp -B out/build`, por las opciones nuevas de raylib) y **recompilar el runner entero** (cambió `gs_frontend.h`, que incluye todo: ~45 min) |

**Mando (bloqueo 10, override de `libpad2`):** sin mando en el PC funciona el **teclado**. Botones: flechas = cruceta, `Z`/`X`/`C`/`V` = cuadrado/**cruz**/círculo/triángulo, `Q`/`E` = L1/R1, `1`/`3` = L2/R2, **`Intro` = Start**, `Mayús dcha.` = Select, `Ctrl izq./dcha.` = L3/R3. Sticks: `WASD` (izquierdo) e `IJKL` (derecho).

**Fase 3 (cxldsakeb, 2026-10-16, ~12:15): se llega al gameplay.** Pantalla de título completa (a pocos FPS), menú, primer vídeo (bloqueo 11, `sceIpuInit`) y **la primera zona jugable, con sonido** (con `GOW2_PAD_SCRIPT=15:start,20-120/3:x` se llega sin teclado en unos 2 minutos). **Siguiente: el bloqueo 12**, el escenario verde por las sombras de los personajes (ver "Bloqueos"): enseñar al backend del GS las copias de 16 bits con máscara de bits. Pendiente de mirar después: un "fantasma" pegado a Kratos que vio cxldsakeb (probablemente otra pasada que el backend descarta: el destino Z24 `fpsm=0x31`, el brillo o la profundidad de campo), el cielo plano y los pocos FPS. Mirar la ventana durante un minuto de ejecución es lo más rápido. `GOW_HOST_SHOT=<frame>` guarda una captura en `host_shot.png` **en la raíz del repo**: es una imagen del juego y no se sube (regla 1).

**Fase 4 (DaniSBR1, 2026-10-16, ~15:45): bloqueos 12 y 13 resueltos, F12/F10 hechos.** El escenario ya no sale verde, se dibuja la sombra de Kratos y desapareció el polígono azul del cielo (fork `d0b6162`). **Velocidad medida con el contador nuevo (F10) en este PC: ~58–60 FPS reales, VPS 60, velocidad 100 % en la primera zona.** Los "pocos FPS" de antes no se reproducen aquí (otro equipo, o antes de los arreglos). **Siguiente, por orden:**
1. **Contornos azules finos** donde la geometría lejana se recorta contra el cielo (cambios de profundidad), a escala interna 2. Probable efecto del escalado en las pasadas que leen la profundidad, como en PCSX2 al subir la resolución en los God of War. Comprobar comparando la **misma escena** a `PS2X_GS_HW_SCALE=1` (la prueba de hoy cayó en otra escena y allí no había contornos). **Comprobado (cxldsakeb, 2026-10-16): son del escalado**, porque a escala 1 no salen. Ver "Opciones de PCSX2 para God of War II". Siguiente: localizar la pasada (un contador local de dibujos que leen el Z o un target más pequeño que la pantalla) y dibujarla a escala 1, como `nativeScaling` de PCSX2.

**Ojo con el contador F10:** en el PC de cxldsakeb marca "FPS 60, VPS 60, velocidad 100 %", pero el juego se ve a **~2 FPS**. El contador no mide la velocidad real del juego: no hay que fiarse de él (ni de la medición de arriba) hasta revisarlo. La velocidad queda para después de los arreglos gráficos.
2. El "fantasma" pegado a Kratos que vio cxldsakeb: mirarlo en pantalla durante el combate.
3. Partida, combate y HUD, revisados jugando (con teclado o mando; ver "Mando").
4. Guardar y cargar (memory card).

**Controles de la ventana (fork `cf4e3f2`/`3b71410`):** **F12** = pantalla completa sin bordes / ventana (`ToggleBorderlessWindowed`: no cambia la resolución del monitor). **F10** = contador de rendimiento como el de PCSX2 (adaptado de su `PerformanceMetrics` y `DrawPerformanceOverlay`, GPLv3): FPS interno (`[P]` = cambios de búfer de pantalla; `[B]` = frames nuevos entregados al GS, el que aplica en GoW II porque copia cada frame a un único búfer), VPS con media de 16 muestras, velocidad respecto a 59,94 Hz (rojo < 95 %, verde > 105 %) y tiempos de frame de la ventana. `GOW_SHOW_FPS=1` lo muestra desde el arranque. **F1** sigue abriendo y cerrando el panel de depuración. Dos trampas resueltas: F10 es tecla de sistema en Windows y abría el "modo menú" de la ventana, que se comía la siguiente tecla (`ps2_window_win.cpp` lo anula), y raylib traía de serie "F12 = captura" (`screenshotNNN.png` en la raíz del repo; desactivado al compilar raylib y en `.gitignore`).

**Probar teclas sin tocar el teclado:** `keybd_event` de Windows con la tecla mantenida ~150 ms (con `SendKeys` la pulsación es tan corta que raylib no la ve) y `GOW_HOST_SHOT` para la captura. El tamaño de la ventana sale en la línea `[gs-present] host_shot … window=`.

**Cómo se encontró el bloqueo 12 (método reutilizable):** un contador local en `submit()` de `gs_hw_backend.cpp` que, cada 10 s, imprime los motivos de rechazo de `tryShuffle` (`shuffleWhy`) por `FBMSK`, más unas muestras con la geometría de cada uno. No se sube al fork (regla 5): el diff está en el historial de esta sesión; rehacerlo cuando haga falta es copiar el `dumpSkipped` de al lado.

**Parches TEMPORALES vivos:** `0x1875E0` (argumentos de `main`; sobra cuando `SetupThread` los rellene en el runtime).

## Datos del ELF y del disco

- **Punto de entrada `0x100008`** (como GoW1). Un solo segmento cargable: `0x100000`, 0x1FBDA8 bytes en el archivo y 0x27FE00 en memoria (hasta `0x37FE00`). Sin tabla de símbolos.
- **Secciones:** `.text`, `.reginfo`, `.data`, `.rodata`, `.sbss`, `.bss` y **26 `.DVP.overlay…` con su `.DVP.ovlytab`/`.DVP.ovlystrtab`**: los microprogramas de la VU van dentro del ELF como overlays. En GoW1 hubo que capturarlos ejecutando el juego (`GOW_VU1_CAPTURA`); aquí quizá se puedan sacar directamente del ELF para la VU1 nativa. Por investigar en la fase 4.
- **Disco: la misma estructura que GoW1.** `PART1.PAK` y `PART2.PAK` (~4 GB cada uno), `GODOFWAR.TOC`, `IOPRP300.IMG` y los IRX `989NOMID`, `DBCMAN`, `DS1O_S1`, `DS2U_D`, `LIBSD`, `MC2_D`, `MC2_S1`, `SIO2D`, `SIO2MAN`. El único cambio de nombre: **`SMPD.IRX`** (en GoW1, `SMPD_IOP.IRX`). Buena señal para que el runtime valga casi tal cual.
- **SDK de Sony 3.0.0** (`PsIIlibipu  3000`, `PsIIlibdma  3000`, `PsIIlibcdvd 3000` en `.data`, `0x2D9540`…). GoW1 usaba otra versión: por eso las funciones de librería **no tienen los mismos tamaños** que en GoW1 y no se pueden emparejar por tamaño (probado: solo cuadran 6 de 79). Hay que localizarlas por sus cadenas y por quién las llama.
- **Las librerías de Sony van al final de `.text`**: envoltorios de syscalls en `0x2CF520`–`0x2CFDA0` (los reconoce Ghidra) y cadenas de libgraph, libdma, libcdvd, libmpeg, libsifrpc, libdbc, libmc2 y la TTY en `.rodata` `0x2F8DC0`–`0x2FBB10`.

## Mapa de funciones (fase 1)

- **Ghidra** (`ghidra-analyze.ps1`, 2026-10-15): 5 874 funciones y 31 986 etiquetas de código. El CSV del export mezcla las dos cosas (`entry_XXXX`, `caseD_*`, `default*` son etiquetas dentro de funciones); `tools/make_funcmap.py` deja **solo las 5 874 funciones** en `config/funcmap.csv` (sin duplicados ni solapes). Con las etiquetas, `ps2_recomp` habría generado ~38 000 funciones solapadas. GoW1 también usaba solo funciones.
- **Escáner** (`scan_missing_functions.py --csv config/funcmap.csv`): **733 funciones que Ghidra no ve** (607 `vfn_`, 12 `fn_`, 114 `cfn_`; en GoW1 fueron 634). Desde la fase 2 van en `funcmap.csv` (`make_funcmap.py --extra`), no en `entry_points`: ahí el recompilador no registraba sus puntos de reanudación (ver "Bloqueos"). Muestreo: 525 de los 607 `vfn_` son funciones hoja cortas (típico de métodos virtuales) y los `cfn_` revisados a mano empiezan tras un `jr ra` o un epílogo.
- **Primer `recompile.ps1`**: 5 874 funciones + 733 entradas = **6 607 funciones generadas, 0 errores**. 2 300 avisos, todos `control-flow ... unresolved JR/JALR` (saltos indirectos que el recompilador cubre con entradas de respaldo).
- No hay que meter los 47 envoltorios de syscalls que exporta Ghidra en `stubs`: GoW1 no lo hacía (`patch_syscalls = false`, el runtime trata la instrucción `syscall`).

## Stubs de librerías de Sony

Los 79 de la plantilla de GoW1, buscados en GoW II: **76 localizados** (en `config/recomp.template.toml`) y **3 que no están enlazados** en GoW II. DaniSBR1, 2026-10-15.

**Método.** Las librerías de Sony están en el mismo orden y casi con la misma disposición que en GoW1, solo desplazadas en bloque. Para cada stub se aplica el desplazamiento de su librería a la dirección de GoW1 (eso solo da la hipótesis, regla 2) y se comprueba en el ELF de GoW II que ahí empieza una función, que el tamaño coincide con el de `referencia/gow1/config/funcmap.csv` (±4 bytes: un `nop` de relleno que cada herramienta cuenta en una función distinta) y, si no coincide, que el final también cae en su sitio (es decir, que Ghidra partió la función de otra manera). Cuando se pudo, se confirmó por un camino independiente: cadenas que usa la función, a quién llama o el propio código.

| Grupo | Desplazamiento | Stubs | Comprobaciones independientes |
|---|---|---|---|
| libmpeg | +0x3C0E8 | `sceMpegCreate`…`sceMpegAddStrCallback` (10), `0x2AB3E0`–`0x2B01D8` | 6 coinciden en tamaño; los otros 4 cuadran por los dos extremos |
| libipu, libgraph, libdma, libcdvd | +0x3C0F0 | `sceIpu*` (4), `sceGs*` (4), `memclr`, `sceDma*` (5), `sceCd*` (4), `0x2B53D8`–`0x2B6CC0` | `sceGsSyncPath` (`0x2B5BC8`) imprime `"sceGsSyncPath: …"`; `sceCdInit` (`0x2B6900`) usa `"Libcdvd bind err %d CD_Init %d"` y `"Libcdvd Exit"` |
| libc / libgcc | variable | `__divdi3` `0x2BF830`, `memcmp` `0x2C6A38`, `rand` `0x2C8448`, `strrchr` `0x2C94F0` | `memcmp`: 9 de 10 vecinas. `rand`: `x*0x41C64E6D+0x3039 & 0x7FFFFFFF` (newlib). `strrchr`: bucle que guarda la última coincidencia. `__divdi3`: mira el signo de la parte alta y niega los operandos (libgcc); 1 772 frente a 1 776 bytes |
| libkernel | variable | `write` `0x2CFEE8`, `_sceSDC` `0x2D0028`, `_sceIDC` `0x2D0158`, `InitThread` `0x2D0510`, `iWakeupThread` `0x2D0600`, `StartThread` `0x2D0768`, `sceTtyHandler` `0x2D0978`, `sceTtyWrite` `0x2D0B10`, `sceTtyInit` `0x2D0C88` | Emparejadas por la secuencia de tamaños de la zona. `write` llama a `sceTtyInit` y `sceTtyWrite`. `InitThread` usa `"SceKerneltopThread"`. `sceTtyHandler` usa las cadenas `"TTY: …"`. `sceTtyInit` llama a `sceDeci2Open(…, sceTtyHandler)` |
| libsif (cmd, rpc, fs, iopheap, loadfile), alarmas, deci2 | +0x3BB68 | `sceSifInitCmd`…`sceSifRebootIop` (28), `InitAlarm`, `sceDeci2*` (5), `InitTLB`, `0x2D2080`–`0x2D65B0` | `sceSifBindRpc` (`0x2D2DF8`) usa `"SceSifrpcBind"`; `sceOpen`/`sceClose`/`sceRead` usan los semáforos `SceStdioOpenSema`/`…Close…`/`…Read…`; `sceSifRebootIop` (`0x2D4C88`) usa `"rom0:UDNL"`; `sceDeci2Open` (`0x2D6310`) es a quien llama `sceTtyInit` |

**SDK 3.0: funciones en C delante de las syscalls.** `StartThread`, `iWakeupThread` e `InitTLB` son funciones en C de libkernel que acaban llamando al envoltorio de la syscall (`0x2CF740`, `0x2CF860`, `0x2CFD90`, que solo llaman ellas). GoW1 ponía el stub en la función en C, y aquí también. En GoW1, `iWakeupThread` e `InitTLB` parecían caer "a mitad de función" porque su mapa juntaba dos funciones en una (`InitThread`+`iWakeupThread`: `0x2D0510 + 0xF0 = 0x2D0600`).

**No enlazados en GoW II** (se dejan fuera):
- `exit`: nadie llama a `_exit` (`0x1001D0`, el del arranque) salvo `abort` (`0x2C7820`, que hace `_exit(1)`), ni a `Exit` (`0x2D5218` → syscall 4 en `0x2CF540`) salvo el arranque y libkernel. Un juego de PS2 no sale nunca.
- `read` y `sceTtyRead`: en la zona de libkernel la secuencia de tamaños salta de `write` a la siguiente función, y de `sceTtyWrite` directamente a `sceTtyInit`.

**Stubs por nombre (a vigilar en la fase 2).** Además de los de la plantilla, `ps2_recomp` convierte en stub los envoltorios de syscalls a los que Ghidra pone nombre de librería (`sceSifSetDma`, `sceSifDmaStat`, `sceSifSetDChain`, `sceSifSetReg`, `sceSifGetReg`, `sceSifStopDma`, `isceSifSetDma`, `isceSifSetDChain`): con los 76 de la plantilla salen 84 stubs. En GoW1 se recompilaban como código normal que hace la syscall.

## Tabla [mmio] y contraste con ps2_analyzer

DaniSBR1, 2026-10-15. `ps2_analyzer` (la otra herramienta de `build-tools.ps1`) analiza el ELF por su cuenta y lleva una base de datos de símbolos de Sony integrada. Se ejecuta desde Bash o cmd: en PowerShell 5.1 con `_common.ps1` cargado, su primer aviso por stderr aborta el comando.

```
./out/build/ps2xAnalyzer/Release/ps2_analyzer.exe game/disc/SCUS_974.81 out/analyzer.toml
```

- **Stubs:** reconoce los 76 de la plantilla **exactamente en las mismas direcciones** (0 discrepancias). Es una comprobación independiente del método del desplazamiento por librería.
- **Stubs que propone y no se usan:** `sin`, `pow`, `sqrt`, `fabs` y `floor`. GoW1 no los tenía como stubs: recompilados se comportan como en la PS2, y un manejador del PC podría redondear distinto que la FPU del EE (física, animaciones). Probarlos solo si hiciera falta por rendimiento.
- **`entry_points`:** de sus 301 funciones de librería con nombre, 299 ya estaban; se añaden `__sjpopnthrow` (`0x2C1838`) y `__eh_rtime_match` (`0x2C1950`), que Ghidra deja dentro de otra función.
- **`[mmio]`:** la plantilla lleva sus **240 entradas** para GoW II (instrucción `lw`/`sw` → registro del hardware). La tabla de GoW1 (227) era la salida del analizador sin retocar: 158 de sus entradas tienen equivalente exacto en GoW II desplazando por librería (+0x3C0E8, +0x3C0F0, +0x3BB68, +0x3BD00) y las otras 69, en código del juego, tienen su gemelo con la misma secuencia de registros y el mismo espaciado (p. ej. el grupo DMA `e010, d410, d480, d420, d400`: `0x1018C8` en GoW1 → `0x101848` en GoW II). GoW II tiene además 14 accesos nuevos: una cuarta copia de ese grupo DMA (`0x12B99C`), un bloque de configuración de DMA en `0x1901F8`–`0x1902A4` y el timer 1 en `0x2D55A8`.
- **Compilación con `[mmio]`** (cxldsakeb, 2026-10-15): `recompile.ps1` sin errores (6 612 archivos en `generated/`) y el runner `RelWithDebInfo` compila y enlaza en ~38 min (32 hilos, 207 bloques del unity build). El runner se enlaza con `/FORCE`, que crea el `.exe` aunque falten símbolos, así que el criterio no es "hay `.exe`" sino que el log no tenga `LNK2001`/`LNK2019`/`LNK4088`. Aquí el único aviso del enlazador es `LNK4075` (se ignora `/INCREMENTAL` por el `/FORCE`), que es inofensivo.

## Fase 2: preparación del primer arranque

DaniSBR1, 2026-10-16. Funciones de GoW II equivalentes a los overrides **de arranque** de GoW1 (el bloque final de `applyGowOverrides` en `referencia/gow1/overrides/gow_overrides.cpp`). No se hace override hasta que el log lo pida.

| Override de GoW1 | En GoW II | Verificación | ¿Hace falta? |
|---|---|---|---|
| `sceSifInitRpc` (`bindAddressHandler`) | `0x2D27B0` | stub de la plantilla; el C++ generado llama a `ps2_stubs::sceSifInitRpc` | **No.** En GoW1 el recompilador la descartaba (empieza en el *delay slot* de un `jr ra` suelto); aquí Ghidra la detecta como función |
| `iWakeupThread` (`bindAddressHandler`) | `0x2D0600` | ídem | **No**, por lo mismo |
| `sceCdReadDvdDualInfo` → "doble capa, capa 1 en el LBN 2 083 872" | `0x2B6BF0` | libcdvd +0x3C0F0; el código de `0x188748` la llama **en bucle mientras devuelva 0** y en `0x18877C` imprime `"sceCdReadDvdDualInfo function fail"` | **Sí: fue el bloqueo 1** (override hecho, ver "Bloqueos") |
| `sceSifLoadStartModuleBuffer` → "cargado y terminado con NO_RESIDENT_END" | `0x2D4850` | envoltorio de 28 bytes que llama a `_sceSifLoadModuleBuffer` (`0x2D4640`) y va justo detrás de ella (en GoW1, 32 bytes justo detrás); la llama `0x2BF708` | Si el log se para cargando el módulo embebido ("ck01" en GoW1) |
| `snd989SendCommand` (solo registro) | por buscar | — | No es imprescindible: en GoW1 pasa el comando al original |

## Compilación más rápida (DaniSBR1, 2026-10-16)

Mediciones en el PC de DaniSBR1 (Ryzen 5 5500, 6 núcleos / 12 hilos, 16 GB, repo en HDD), recompilando el runner entero (borrando sus `.obj`), con un script que muestrea cada 5 s los `cl.exe` a la vez, la CPU y la RAM, y el resumen de MSBuild (`/clp:PerformanceSummary`).

| | Antes | Con `/MP6` |
|---|---|---|
| **Tiempo total** | **40 min 24 s** | **21 min 13 s** (1,9× más rápido) |
| Compilar (`ClCompile`) | 2 389 s (98,6 %) | 1 243 s |
| Enlazar (`Link`) | 30 s | 26 s |
| `cl.exe` a la vez | 1 (media 0,99) | 6,6 de media |
| CPU | ~8 % | 89 % de media |
| `cl.exe` más grande | 1,85 GB | 3,27 GB |
| RAM libre mínima | 1,7 GB | 267 MB (por debajo de 1 GB el 5 % del tiempo) |

- **Causa:** el generador de Visual Studio compila los archivos de un proyecto de uno en uno si no se le da `/MP`. `--parallel` solo ejecuta a la vez proyectos distintos. Los 207 bloques del runner se compilaban en serie.
- **`/MP`** (fork, `ps2xRuntime/CMakeLists.txt`): para `ps2EntryRunner` y `ps2_runtime`. El número de compiladores sale de la RAM y los núcleos de cada PC (1,5 GB por compilador y 6 GB para el sistema: 6 en 16 GB / 12 hilos, unos 17 en 32 GB / 32 hilos, 1 = sin paralelismo por debajo de ~8 GB). Se cambia con `-DPS2X_MSVC_MP_PROCESSES=N`. CMake lo muestra al configurar: `PS2X: MSVC parallel compilation /MP6`. MSBuild no recompila nada por activarlo.
- **Copiado por contenido** (`scripts/recompile.ps1`): copia al runner solo los archivos cuyo contenido cambió (tamaño y, si coincide, MD5), sin tocar la fecha de los demás, y solo borra las copias que ya no existen. Antes lo borraba y lo copiaba todo con la fecha actual, y cada sincronización recompilaba el runner entero.
  - `-SyncOnly` sin cambios: **0 archivos copiados y 0 bloques recompilados** (la compilación, 6 s). Antes: recompilación completa.
  - Cambiar un override y `-SyncOnly`: **1 archivo copiado, 1 bloque recompilado, 21 s**.
  - La comparación lee todos los archivos: 40–170 s en HDD según la caché de Windows.
- **Comprobado:** el juego llega a la zona jugable igual que antes (FPS 57,99 `[B]`, VPS 60, velocidad 100 %, sin `missing-target`).
- **No sirve de mucho aquí:** mover la compilación a un SSD (el enlazado son 30 s de 40 min). Opcional en cada PC: excluir la carpeta del repo de Windows Defender, que mientras compila revisa miles de archivos nuevos y ocupa ~400 MB.
- Un cambio en una cabecera que lo incluye todo (`gs_frontend.h`, `ps2_recompiled_functions.h`…) sigue obligando a recompilar entero, pero ahora en la mitad de tiempo.
- **PC de cxldsakeb** (Ryzen 9 5900XT, 16 núcleos / 32 hilos, 32 GB, repo en HDD): recompilación entera del runner tras el cambio de `gs_frontend.h`, **12 min 53 s** (antes ~38 min), sin `LNK2001`/`LNK2019`.

## Opciones de PCSX2 para God of War II (cxldsakeb, 2026-10-16)

Para buscar el origen de los contornos azules y del "fantasma", se miró qué activa PCSX2 para este juego (material público: `GameIndex.yaml`, `GSRendererHW.cpp`, `Config.h`). Resumen con DeepSeek, comprobado contra el código. `GSHwHack.cpp` (los *CRC hacks*) **no tiene nada de God of War**.

Todas las regiones de God of War II (`SCUS-97481`, `SCES-54206`, `SCKA-30006`…) llevan lo mismo. God of War I solo lleva las dos primeras:

| Opción | Qué hace en PCSX2 | Solo a escala > 1 | En nuestro backend (`gs_hw_backend.cpp`) |
|---|---|---|---|
| `halfPixelOffset: 5` (`NativeWTexOffset`, "Fixes misaligned bloom") | Coloca los vértices como si se dibujara a resolución nativa (`GSRendererHW.cpp` `DetermineVSConfig`, ~6327-6365), con un ajuste extra en sprites cuya fracción tiene el bit 8, y desplaza las coordenadas de textura `8 − 4/escala` (en 1/16 de téxel) en sprites FST (~8210-8232; su comentario cita "la cabina del barco en God of War") | sí | Parcial: solo un `+0,5/escala` fijo en cada vértice (`addVertex`) |
| `autoFlush: 1` (`SpritesOnly`, "Fixes sun occlusion") | Vacía los dibujos pendientes cuando un sprite lee su propio target | no | Por otro camino: `TextureBarrier` cuando un dibujo lee su target |
| `alignSprite: 1` ("Fixes water vertical lines") | Sprites con `X` en fracción 8: alarga medio píxel la columna derecha (`X += 8`, y `U += 8` si la textura está desalineada) (~5236-5263) | sí | No |
| `nativeScaling: 1` (`Normal`, "Fixes light blooms") | Detecta los dibujos que reducen o amplían la imagen (`IsScalingDraw`, ~10840) y los hace a escala 1: bloom, brillos, copias de post-proceso (~3675-3708) | sí | **No**: una sola escala para todos los targets |

Además, `mvuFlag: 0` (*speedhack* de la VU, "Fixes enemies attacks turning into squares"). A tener en cuenta si salen cuadrados en los ataques de los enemigos.

**Prueba de escala** (misma zona, `GOW2_PAD_SCRIPT=15:start,20-120/3:x`, `GOW_HOST_SHOT=9000`): a `PS2X_GS_HW_SCALE=2` hay un contorno azul **escalonado, en bloques**, por toda la silueta de la ciudad y la montaña contra el cielo, y una línea de puntos diagonal en el cielo. A escala 1, el mismo horizonte sale limpio. **Los contornos son del escalado.** Por la forma de bloques, apunta a una pasada que el juego hace a menor resolución (niebla, cielo o profundidad de campo, que leen la profundidad) y que, escalada, no coincide con los bordes de la imagen: el caso que cubre `nativeScaling`. El "fantasma" no se distingue en capturas fijas. Ojo: las dos ejecuciones no cayeron en el mismo momento del combate, porque el guion va por segundos de reloj y el juego no avanza igual en cada arranque.

## Memory card en GoW II (cxldsakeb, 2026-10-16)

- `MC2_D.IRX`, `SIO2D.IRX` y `SIO2MAN.IRX` son **idénticos** a los de GoW1 (mismo SHA1). Por eso sus sustituciones nativas (`ps2xIOP/src/native/iop_native_overrides.cpp`), que no comprueban el tamaño del texto, caen bien. De los IRX con sustituciones, solo cambia `LIBSD` (bloqueo 9).
- Por vigilar al probar guardar y cargar (lectura de DeepSeek del runtime, sin comprobar aún): `sceMcRename` está implementada pero no enganchada en `PS2IopHostAdapter::memoryCard` (`ps2_iop_host.cpp`), y `GetDir` por MCSERV devuelve 0 entradas (`mcserv.cpp`).

## Diagnóstico de bloqueos (overrides/gow2_overrides.cpp)

Solo se activan con la variable de entorno; sin ella no cambian nada.

- `GOW2_THREAD_WATCH=<s>`: cada `<s>` segundos, los hilos del EE (`status`, espera, `pc`, `ra`). Para bloqueos silenciosos: si un hilo `status=0` repite el mismo `pc` con el ciclo avanzando, es un bucle activo. Como `GOW_THREAD_WATCH` en GoW1.
- `GOW2_BOOT_TRACE=1`: cuenta las llamadas a las funciones de la secuencia de arranque (`sceSifRebootIop`, `sceCdInit`, `sceSifInitRpc`…) y registra cada `sceSifLoadModule` (ruta y resultado) y cada `sceSifBindRpc` (servidor y resultado).
- Del runtime, sin recompilar: `PS2X_TRACE_RPC=<prefijo>` registra las llamadas RPC que llegan al IOP (p. ej. `8` para los servidores de Sony).
- Del runtime, para el IOP: `PS2X_IOP_TRACE=<n>` registra las primeras `n` llamadas a imports (`[IOP:trace] #k libreria:ordinal a0=… ra=…`); `PS2X_IOP_TRACE_NOCLIB=1` quita `sysclib` (la mayor parte del volumen) y `PS2X_IOP_TRACE_EVERY=<n>` muestrea una de cada `n`. Con `PS2X_TRACE_RPC=123` se ven las RPC del EE a SMPD (`sid 0x123456`/`0x123457`) con sus 8 primeras palabras. Así se encontraron los bloqueos 6 a 8.
- `GOW2_CALL_PROFILE=<s>` (override): envuelve todas las funciones del juego y, cada `<s>` segundos, imprime las 25 más llamadas. **Es la mejor herramienta para un bucle silencioso:** las funciones del bucle salen arriba, y si solo sale una dirección, es que el bucle no llama a nada (bloqueo 8). Tarda unos segundos más en arrancar.
- `GOW2_PAD_SCRIPT` (override): pulsaciones del mando a tiempos fijos (segundos desde la primera lectura del mando), para repetir una secuencia sin teclado. `"T:boton"` o `"T1-T2/P:boton"` (cada `P` segundos). Botones: `start`, `select`, `x`, `o`, `sq`, `tri`, flechas (`up`…), `l1`, `r1`, `l2`, `r2`. Ej.: `GOW2_PAD_SCRIPT=15:start,20-60/3:x` (pantalla de título → menú → partida nueva). En el log: `[gow2-pad2:script]`.
- `GOW2_PEEK=dir,dir,…` (override, con `GOW2_THREAD_WATCH`): imprime esas palabras de memoria del EE en cada pasada. Para seguir variables del juego (estado del cargador, banderas).
- `PS2X_IOP_TRACE_DMA=1` (runtime): cada DMA del IOP al EE, con origen, destino, tamaño y los primeros 16 bytes.
- `GOW_GS_HW_STATS=1` (runtime): dibujos, lotes y texturas del GS por intervalo. Sirve para saber si el juego dibuja sin mirar la pantalla.
- **El `pc` de `GOW2_THREAD_WATCH` no siempre indica un cuelgue:** en código recompilado solo se actualiza en ciertos puntos (al volver de una llamada, por ejemplo). Un bucle principal que da vueltas cada frame también sale siempre con el mismo `pc`. Para saber si avanza, mirar la actividad (RPC, imports del IOP).
- `tools/elf_dis.py` necesita `capstone` del entorno del usuario: ejecutarlo con `py -3` a secas (con `py -3 -I` no lo encuentra).
- Para probar un override sin recompilar todo: copiar solo `overrides/gow2_overrides.cpp` a `PS2Recomp/ps2xRuntime/src/runner/`, `touch` y `cmake --build` (~1 min; solo cambia su bloque del unity build). `recompile.ps1 -SyncOnly` vuelve a fechar los 6 600 archivos y recompila todo (~50 min). Ejecutar CMake desde Bash: en PowerShell, con `_common.ps1` cargado, el primer aviso por stderr aborta el comando.

## DeepSeek en la fase 2

Regla 1: DeepSeek solo lee material público (el runtime y `referencia/gow1/`), nunca desensamblado ni logs de GoW II. Lo que ha funcionado: trocear los archivos grandes (~60 KB) y lanzar un agente por trozo en paralelo, con una tabla de salida exacta (6 agentes en 1–3 min). Resultado útil: un catálogo de los overrides de GoW1 por tipo (`arranque`, `memoria`, `gráficos`…) para reconocer cada bloqueo; lo comprobado a mano coincide con el código (`gowCdReadDvdDualInfo`, `gowLoadStartModuleBuffer`). Candidatos de arranque que aún no han hecho falta en GoW II: `DelayThread` y el timer 2 (`gowDelayThread`, `gowTimerSystemNoop`). Los archivos pequeños (<10 KB) es más rápido leerlos directamente.

Catálogo de los imports del IOP que implementa el runtime (cxldsakeb, 2026-10-16): 3 trozos en paralelo, ~1–2 min cada uno, con la tabla `librería | ordinal | función | estado | archivo:línea`. Comprobado a mano en el código, sin errores en lo revisado. Dos detalles: el script lo lanza en modo plan, así que DeepSeek acaba preguntando "¿lo escribo en `salida.md`?", pero su respuesta **ya es** la salida y no hace falta contestar; y solo cataloga lo que le pasas: `thbase`, `thsemap`, `thevent`, `sifcmd` y `sifman` viven en `core/iop_kernel.cpp` y `services/iop_rpc.cpp`, no en `imports/`.

Bloqueo 12 (DaniSBR1, 2026-10-16): 8 agentes en paralelo sobre material público. PCSX2 (`GSRendererHW.cpp` troceado por funciones: detección y emulación de *texture shuffles* y `FBMSK`; `tfx_fs.glsl`) y el backend del GS del runtime en 3 trozos. Lo útil: PCSX2 emula `FBMSK` por bits con `C = (C & ~FbMask) | (RT & FbMask)` leyendo el destino con `texelFetch(RtSampler, gl_FragCoord)` y una barrera, y en el *shuffle* el byte bajo del halfword va a R/B y el alto a G/A. Al final el bloqueo no era eso (ver "Bloqueos"), pero sirvió para descartarlo deprisa. 2 de los 8 agentes no terminaron (sin "Hecho"): repetirlos si hace falta; los archivos de PCSX2 se bajan de `raw.githubusercontent.com/PCSX2/pcsx2/master/…`.

F10/F12 (DaniSBR1, 2026-10-16): 4 agentes, 30–50 s cada uno. `PerformanceMetrics` y `DrawPerformanceOverlay` de PCSX2 (definición de FPS/VPS/velocidad, intervalo de 0,5 s, colores y formato) y el panel de depuración del runtime en 2 trozos (cómo usa ImGui y qué datos de rendimiento tiene). Comprobado al implementarlo: la definición de VPS y la media de 16 muestras coinciden con el código de PCSX2.

## Cargador de recursos y protocolo con SMPD

cxldsakeb, 2026-10-16 (bloqueos 7 a 9). Sacado de `PS2X_TRACE_RPC=123`, `PS2X_IOP_TRACE_DMA=1` y el desensamblado.

- **RPC al servidor `0x123456` de SMPD, todas en modo 1 (sin esperar).** Se envían con `0x2A7FA0`: espera a que acabe la anterior (`sceSifCheckStatRpc`), llama a `sceSifCallRpc` y espera la respuesta con `0x2A7C10`. Comandos vistos:
  - `fn 0x68` sin más: **registro**. El EE da a SMPD dos búferes suyos, `0x338500` (0x40 bytes, recibe la fecha del TOC) y **`0x338540` (0x5C0, bloque de estado)**. SMPD escribe ahí por DMA un número de secuencia (`+0`) cada vez que cambia algo, y las **ranuras** de las peticiones (`0x338540 + 0xF4 + índice*0x24`).
  - `fn 0x68` con nombre: **petición de un recurso** (`R_LglScA`, `R_PermA`, `SBP_general`…; tipo `0xF` = WAD, `0xD` = banco de sonido).
  - `fn 0x4C`: **"dame datos"**. SMPD los envía por DMA al doble búfer del EE (`0x3DD2C0` / `0x3FD2C0`, trozos de hasta 0x20000 bytes).
  - `fn 0x4D`: un **paquete** con varios comandos dentro (`n`, luego `tamaño<<16 | fn` de cada uno).
- **El cargador `0x18CF70`** es una máquina de estados (`[0x2D8130]`). Lee del búfer de entrada (`[0x2D8168]`, bytes disponibles en `+0x10`) cabeceras de 0x20 bytes y procesa los trozos. **Trabaja hasta agotar un presupuesto de tiempo por frame**, medido con el registro `Count` (de ahí el bloqueo 8), y devuelve 1 mientras le quede trabajo. Si `[0x2D8154] ≠ 0` (en **pausa**), vuelve enseguida con 1.
- **Pausa y reanudación:** `0x17ED58` (contador `[0x2D8154]`++) y `0x17ED98` (−−).
- **Peticiones pendientes:** las recorre `0x17F170`, llamado desde `0x17F578` junto con el sondeo de SMPD (`0x2A7990`). Estado en `+0x38`: **2** = espera a que la ranura tenga el bit `0x2000` en `+0x20` (si `+0` ≠ 0 pasa a 3; si es 0, no encontrado: reanuda y pasa a 5); **3** = envía subpeticiones de lectura (`0x2A8A88`, comando 0x13, hasta 8 por vuelta) y al terminar reanuda (4).
- **Espera de carga:** `0x1EAAE0` (dentro de `0x1EA8D8`), `do { 0x17F578(); 0x25F140(); } while (0x18CF70())`.
- **Bancos de sonido:** SMPD se trae el banco desde el EE (`sceSifGetOtherData`, trozos de 0x2000 bytes), se lo pasa a 989snd (`snd989:129`) y este lo sube a la SPU2 con `sceSdVoiceTrans`. Sin las sustituciones nativas de LIBSD, eso espera un DMA de la SPU2 que no existe (bloqueo 9).

## Imports del IOP que usa GoW II

cxldsakeb, 2026-10-16. `tools/irx_imports.py game/disc/IOP_MOD/*.IRX` lista lo que piden los 8 IRX que carga el juego; se cruzó con el catálogo de lo que implementa el runtime (ver "DeepSeek en la fase 2"). El bloqueo 6 (`ioman`) y el 7 (`strtok`, `GetSystemTimeLow`) eran justo esto: imports que el juego usa y que el runtime no atendía. **Al cambiar de módulos o de runtime, repetir el cruce.**

- Los IRX piden al runtime 16 librerías: `cdvdman`, `dmacman`, `intrman`, `ioman`, `loadcore`, `secrman`, `sifcmd`, `sifman`, `stdio`, `sysclib`, `sysmem`, `thbase`, `thevent`, `thsemap`, `timrman` y `vblank`. Las otras cinco (`libsd`, `sio2man`, `sio2d`, `snd989` y `dbcman`) las exporta otro IRX del juego y se ejecuta su código.
- `ioman` `open`/`close`/`read`/`lseek` las usan **SMPD y 989NOMID**.
- Con `df2a082`, todo lo que piden está atendido salvo esto, que de momento no ha dado problemas:

| Import | Lo pide | En el runtime | Por qué no corre prisa |
|---|---|---|---|
| `sysclib` 8 `look_ctype_table` | SMPD, SIO2MAN | Devuelve 0 (`isdigit`, `isalpha`… siempre falsos) | No sale en las primeras 20 000 llamadas de la traza |
| `sysclib` 5 `longjmp` | SMPD | No salta | Solo en caminos de error; no se ha llamado |
| `cdvdman` 15 `sceCdStop`, 39 `sceCdBreak` | 989NOMID | `[IOP] unhandled import` y 0 | Si se llaman, se verá en el log |
| `dmacman` (todos) | SIO2MAN | 0, a propósito (ya no hay transferencias del SIO2) | Heredado de GoW1, funciona |

## Funciones identificadas

Dirección = inicio de la función en el ELF de referencia.

| Dirección | Nombre | Qué hace | Equivalente en GoW1 | Estado | Quién |
|---|---|---|---|---|---|
| `0x002D4850` | `sceSifLoadStartModuleBuffer` | Arranca un módulo ya copiado a la memoria del IOP | `0x298CE8` (`gowLoadStartModuleBuffer`) | reimplementada | DaniSBR1 |
| `0x002BF660` | `loadEmbeddedIopModule` (nombre nuestro) | Copia al IOP el módulo de `0x2DB590` (`sceSifAllocIopHeap` + `sceSifSetDma`) y lo arranca; resultado en `0x2D7FE4`, id en `0x2D7FE0` | — | investigando | DaniSBR1 |
| `0x00146318` | `initMainHeap` (nombre nuestro) | Antes de `main`: heap principal del final del `.bss` a `0x1FF7FF8`, con `0x0BADC0DE` en `0x1FF7FFC` | — | investigando | DaniSBR1 |
| `0x00183330` | `rbTreeFind` (nombre nuestro) | Búsqueda en un árbol rojo-negro con nodos del "RB NODE Heap" (pool en `[0x2D7FC8]`, centinela en `[0x2D7FC0]`, hijos como índices de 16 bits) | — | investigando | DaniSBR1 |
| `0x0027D6B0` / `0x0027D788` / `0x0027CE10` | push / pop / top de la pila de estados (nombres nuestros) | Pila en `objeto+0x48` (contador `int8` en `+0xC8`); métodos virtuales `vtable[0x84]`/`[0x8C]`/`[0x94]` del objeto `tabla[22]` (`0x362CA0`; tabla de objetos en `0x362C48`) | — | investigando | DaniSBR1 |
| `0x002B56F0` | `sceIpuInit` | libipu: inicializa la IPU (llama a `SetD4_CHCR` `0x2B5678` y carga las tablas IQ `0x2D9550` y VQ `0x2D95A0`) | `0x279600` | reimplementada | cxldsakeb |
| `0x0014E380` / `0x0018ADD0` | — / `waitGsFinish` (nombre nuestro) | `0x14E380` pone `[0x2D8104] = 1`, envía el frame (`0x14D150`) y espera en `0x18ADD0` mientras siga a 1 | — | investigando | cxldsakeb |
| `0x0018A278` | `gsInterruptHandler` (nombre nuestro) | Manejador de interrupción del GS: con `FINISH` en `GS_CSR`, pone `[0x2D8104] = 2` | — | investigando | cxldsakeb |
| `0x002B78D8`–`0x002B7C88` | `scePad2Init`, `End`, `CreateSocket`, `DeleteSocket`, `Read`, `GetButtonProfile`, `GetState` | libpad2: el mando en el EE (sockets, paquete de 18 bytes). Siguen en el mismo bloque `InitDmaDBuff` `0x2B7DC8`, `LinkDriver` `0x2B7E40`, `GetSide` `0x2B7E98`, `CheckDma` `0x2B7F68`, `SetButtonOrder` `0x2B7FC8`, y libvib: `sceVibGetProfile` `0x2B8080`, `sceVibSetActParam` `0x2B8100` | `0x27B7E8`… (+`0x3C0F0`) | reimplementada (las 7 primeras) | cxldsakeb |
| `0x0018CF70` | `loaderUpdate` (nombre nuestro) | El cargador de recursos: máquina de estados `[0x2D8130]` con presupuesto de tiempo por frame medido con `Count` (amplía `Count` a 64 bits: último valor en `0x2D7784`, vueltas en `0x2D7780`). Devuelve 1 mientras le quede trabajo. Ver "Cargador de recursos y protocolo con SMPD" | — | investigando | cxldsakeb |
| `0x0017ED58` / `0x0017ED98` | `loaderPause` / `loaderResume` (nombres nuestros) | Suben y bajan el contador de pausa `[0x2D8154]`; la pausa guarda dos argumentos en `0x2D7F3C`/`0x2D7F40` | — | investigando | cxldsakeb |
| `0x0017F170` | `processLoadRequests` (nombre nuestro) | Recorre las peticiones de carga pendientes (estado en `+0x38`) | — | investigando | cxldsakeb |
| `0x002A7FA0` | `smpdSendCommand` (nombre nuestro) | Envía un comando a SMPD (`sceSifCallRpc`, servidor `0x123456`, modo 1) y espera la respuesta | — | investigando | cxldsakeb |
| `0x001EA8D8` | — | Contiene la espera de carga `0x1EAAE0` (`do … while (loaderUpdate())`) | — | investigando | cxldsakeb |
| `0x0018A3C8` | `vsyncStartHandler` (nombre nuestro) | Manejador de VBlank (`AddIntcHandler(2, …)` en `0x18A428`): alterna el campo en `0x2D810C` y cuenta frames en `0x2D8108` | — | investigando | cxldsakeb |
| `0x002B6BF0` | `sceCdReadDvdDualInfo` | libcdvd: informa de si el disco es de doble capa y dónde empieza la capa 1 | `0x27AB00` (`gowCdReadDvdDualInfo`) | reimplementada | DaniSBR1 |
| `0x001875E0` | `bootDeviceFromArgs` (nombre nuestro) | Mira `argv[0]` y guarda en `0x2D7FE8` si se arrancó desde `cdrom` (0) o desde `host0:` (1) | — | binding temporal (TEMPORAL) | DaniSBR1 |
| `0x00187630` | `makeDevicePath` (nombre nuestro) | Ruta → `"cdrom0:\RUTA;1"` (mayúsculas, `\`) o `"host0:ruta"` según `0x2D7FE8`; con otro valor devuelve el búfer sin escribir | — | investigando | DaniSBR1 |
| `0x001885F8` | `loadIopModule` (nombre nuestro) | Carga un IRX: `IOP_MOD/` si viene de host, ruta con `0x187630`, `sceSifLoadModule` (`0x2D4A98`). El arranque la llama para 8 módulos (tabla en `0x32F340`) | — | investigando | DaniSBR1 |
| `0x002B6DF0` | `sceDbcInit`? | `sceSifBindRpc(0x80001300)` (DBCMAN) en bucle hasta que exista el servidor | `0x27AD00` | investigando | DaniSBR1 |

Estados: `investigando` · `binding temporal (ret0/ret1)` · `binding real` · `reimplementada`.

## Bloqueos

Los más recientes arriba. Un bloqueo = el primer error del log que impide avanzar.

| Fecha | Síntoma (línea del log) | Causa | Solución / PR | Quién |
|---|---|---|---|---|
| 2026-10-16 | Bloqueo 13. Un **polígono azul translúcido** enorme sobre el cielo de la primera zona | La tercera copia de las sombras (Z16 `tbp=0xF00` → framebuffer, columnas de 8 px de 896 filas) se descartaba con `shuffleWhy=13`: `tryShuffle` tomaba como origen el **último** target de profundidad con ese `zbp` (`0x78`), uno de 128 px de ancho (`fbw=2`), y la vista de 512 px se salía de él. El del frame (512 px) también usa esa memoria de Z | Fork `d0b6162`: entre los targets que comparten el Z, el que tiene el ancho de la vista (`fbw == tbw`) y, entre iguales, el último que escribió la profundidad (`lastDepthSeq`). **Comprobado con captura:** el polígono desaparece. Quedan contornos azules finos a escala 2 (ver "Estado del repositorio") | DaniSBR1 |
| 2026-10-16 | Bloqueo 12. En la primera zona (Rodas), todo el escenario sale **verde**; los personajes y el HUD, bien | El juego usa el **canal verde del framebuffer como contador de las sombras** de los personajes. `0x12F738` (desde `0x139008` ← `0x16E720` ← `0x16EDB8`, que `0x16F9A0` llama 8 veces) lo pone a 127 con un sprite a pantalla completa (`FBMSK=0xFFFF00FF`, color `0x7F00`). Los volúmenes suman y restan en G (`COLCLAMP=0`). Después lo restaura con copias por una **vista de 16 bits con máscara de bits** (`fpsm=CT16`, `FBMSK=0x3FFF`, columnas de 8 px, de `tbp` `0x100`/`0x2B00`/`0xF00` Z16). El backend (`gs_hw_backend.cpp`, `tryShuffle`) solo hace esas copias con bytes enteros (`shuffleWhy=5`) y las descarta, así que G se queda a 127. **Comprobado** con capturas (`GOW_HOST_SHOT`): descartando los dibujos con `FBMSK=0xFFFF00FF`, la imagen es correcta y solo faltan las sombras. Descartados: anular `0x16EDB8` desde el juego deja la pantalla **negra**; descartar las mezclas con alfa de destino no quita el verde. También se descartan dibujos sobre un destino Z24 (`fpsm=0x31`, sprites de 32×224) | **Resuelto** en el fork, `8a4abab` (DaniSBR1, 2026-10-16). La causa no era la máscara parcial: `0x3FFF` cubre bytes enteros en la vista de 16 bits. Un contador local de los motivos de rechazo de `tryShuffle` mostró que esas copias son **columnas de 8 px a pantalla completa que acaban en `y = 895.4375`**, y `tryShuffle` exigía extremos enteros (`shuffleWhy=9`). En el GS un píxel se dibuja si `y0 <= y < y1`, así que cubren 896 filas. Ahora el final es `ceil(y1)` y la escala 1:1 se comprueba en 1/16 de píxel. Por cada grupo de columnas hay tres copias: framebuffer → búfer temporal de 16 bits (`fbp=0x158`, `fbw=1`), vuelta al framebuffer y **Z16 (`tbp=0xF00`) → framebuffer**; las dos primeras restauran el verde. **Comprobado con captura:** el escenario sale bien y **se dibuja la sombra de Kratos**. La tercera (desde Z) sigue descartada por salirse del target (`shuffleWhy=13`), ver "Estado del repositorio" | DaniSBR1 |
| 2026-10-16 | Bloqueo 11. Tras Start y unas `X` en el menú, el juego se para (deja de leer el mando, apenas usa CPU). `[vif1] unhandled VIFcode` con códigos sin sentido (bytes de texto como `WA`, `GL`) y `[dma] chain tag with SPR flag … addr=711f11f2 qwc=21233`, justo después de `[MPEG:AddCallback]` (empieza un vídeo). `GOW2_CALL_PROFILE`: el hilo 1 solo ejecuta `0x18ADEC` | `0x14E380` pone `[0x2D8104] = 1`, envía el frame (`0x14D150`) y espera en `0x18ADD0` a que el manejador de interrupción del GS (`0x18A278`, bit `FINISH` de `GS_CSR`) la ponga a 2. El `FINISH` no llega porque la cadena del VIF1 se ha desincronizado. Causa: el **stub genérico de `sceIpuInit`** del runtime (`Kernel/Stubs/IPU.cpp`) lleva direcciones fijas de otro juego (llama a `0x126428` y carga las tablas de la IPU de `0x1721E0`/`0x172230`); GoW1 ya lo sustituía (`gowIpuInit`) | Override de `sceIpuInit` (`0x2B56F0`), adaptado de GoW1: la misma secuencia de registros que el `sceIpuInit` de GoW II (decodificada de sus `lq`/`sq`), con sus tablas IQ `0x2D9550` y VQ `0x2D95A0` (GoW1 `0x2A1610`/`0x2A1660`). Resultado: 0 errores del VIF1, el vídeo se reproduce (funciones de libmpeg y del reproductor en el perfil) y el juego recibe todas las pulsaciones | cxldsakeb |
| 2026-10-16 | Bloqueo 10. En pantalla, el aviso de que no hay mando conectado (lo vio cxldsakeb) | El juego lee el mando en el EE con `libpad2` (`scePad2*`), que habla con DBCMAN en el IOP; GoW1 lo sustituía por el mando nativo del runtime y en GoW II faltaban esas sustituciones | Override de las 7 funciones de `libpad2` (adaptado de GoW1): `scePad2Init` `0x2B78D8`, `End` `0x2B7918`, `CreateSocket` `0x2B7980`, `DeleteSocket` `0x2B7A88`, `Read` `0x2B7AE0`, `GetButtonProfile` `0x2B7BB8`, `GetState` `0x2B7C88`. Direcciones: GoW1 + `0x3C0F0`, cada una empieza una función del mapa con el tamaño de GoW1 (±4), y el juego las llama desde `0x188B4C`/`0x188BC8`/`0x188C5C`. El juego abre los dos puertos y lee el 1 cada frame; lo que dibuja cambia (de ~4 200 a ~2 300 lotes, ahora con cientos de texturas decodificadas por intervalo). **Falta confirmarlo en pantalla** | cxldsakeb |
| 2026-10-16 | Bloqueo 9. El hilo 1 da vueltas en `0x1EAAE0` (`do { 0x17F578(); 0x25F140(); } while (0x18CF70())`) tras pedir el banco de sonido `SBP_general`; el cargador está en pausa (`[0x2D8154] = 1`) | La petición sigue en el estado 2 (ver "Cargador de recursos"): espera que SMPD marque su ranura. SMPD sube el banco a la SPU2 con LIBSD (`sceSdVoiceTrans`, `libsd:17`) y LIBSD espera en `WaitEventFlag(5)` el fin de un DMA de la SPU2 que el runtime no emula. En GoW1 eso no pasaba porque las funciones de LIBSD se sustituyen por C++ nativo (`iop_native_overrides.cpp`). En GoW II **no se aplicaba ninguna sustitución nativa del IOP**: (1) el juego carga los módulos como `cdrom0:\NAME.IRX;1` y el identificador salía `libsd_irx_1` (en GoW1 eran `IOP_MOD/libsd.irx`); (2) el LIBSD de GoW II es **otra compilación** (texto `0x4BB0`; GoW1 `0x4840`) y ninguno de los 16 desplazamientos coincidía. MC2_D, SIO2MAN, SIO2D, DBCMAN y DS2U_D son idénticos a los de GoW1 (SHA1 comparado con el disco de GoW1); 989NOMID también cambia, pero no tiene sustituciones | Fork `9ccffe1`: el nombre del módulo pierde el `;1`, y cada sustitución puede indicar el tamaño de texto de su compilación (`IopNativeOverride::textSize`). Las de LIBSD de GoW II salen de su tabla de exports; la configuración del bus (no exportada) es la única coincidencia del patrón de GoW1 (`0x1560` → `0x1550`). `tools/iop/native_overrides.txt` lleva ya los desplazamientos de GoW II. Con eso, el banco se sube, la carga sigue, el juego entra en su bucle principal y dibuja; desaparecen además los `wait outside a thread timed out` de MC2_D | cxldsakeb |
| 2026-10-16 | Bloqueo 8. `GOW2_THREAD_WATCH`: el hilo 1 siempre en `pc=0x18D050`, dentro del cargador `0x18CF70`. El EE deja de hacer RPC a SMPD tras pedirle `R_PermA…`; SMPD lee 384 KB de `PART1.PAK` y espera un `fn 0x4C` que no llega | `GOW2_CALL_PROFILE`: el EE solo ejecuta `0x18D050` (no llama a nada). `GOW2_PEEK`: estado 1 con `[0x32F740] = -2`, así que el cargador va a `0x18D358`, que lee el registro **`Count`** de la CPU y repite hasta pasar una fecha límite (presupuesto de tiempo por frame). El recompilador traduce `mfc0 Count` a `ctx->cop0_count`, **que nada hacía avanzar**: el tiempo estaba parado. Solo esta función lee `Count` y nadie lo escribe | Fork `9ccffe1`: `cop0_count` sigue al contador de ciclos del EE (`EeScheduler`: se fija antes de ejecutar y `accountCycles` lo actualiza). La carga de `R_PermA` termina y el EE pide `SBP_general` | cxldsakeb |
| 2026-10-16 | SMPD imprime `files = NULL`, `movies = NULL`, `streams = NULL`, `banks = NULL`; el hilo 1 sigue en el bucle de `0x18CF70` | SMPD trocea su argumento (`wad/,Z:/GoW2/…,sound/`) con `strtok` (`sysclib:35`), que el runtime dejaba siempre en 0: sin rutas no encuentra los recursos. Además llama a `GetSystemTimeLow` (`thbase:43`, junto a sus lecturas del disco), que no existía y, como los demás imports de `thbase`, fallaba **sin aviso y sin tocar `$v0`** | Fork `df2a082`: `strtok` con su posición guardada (como el `sysclib` real), `GetSystemTimeLow` = parte baja del contador del timer, y los ordinales sin atender de `thbase`/`thsemap`/`thevent`/`sifcmd`/`sifman`/`sysclib` pasan al aviso `[IOP] unhandled import` y devuelven 0. Ahora: `files = wad/`… y el EE empieza a pedir recursos. Se encontró cruzando los imports de los IRX con lo que implementa el runtime (ver "Imports del IOP que usa GoW II") | cxldsakeb |
| 2026-10-16 | Bucle activo en `0x27D7E8` (`ra=0x1EAAB8`): recorre una lista en `0x38` | `0x27D788` (método virtual `vtable[0x8C]`, "pop") saca de una pila de estados **vacía** (`obj = NULL`, `cabeza = 0x38`). El push de `0x18D5BC` no ocurre porque `top()` (`0x27CE10`, `vtable[0x94]`) ya devuelve NULL. **Causa:** SMPD abre `cdrom0:\GODOFWAR.TOC` con `ioman` (`open`/`lseek`/`read`/`close`, ordinales 4/8/6/5), que el IOP del runtime no implementaba (solo `AddDrv`/`DelDrv`; un import sin implementar devuelve 0): cargaba un índice vacío y no había recursos | `open`/`close`/`read`/`lseek` de `ioman` en el fork para el dispositivo `cdrom`, sobre los archivos del disco (`IopCdvd::findDiscFile`/`readDiscFile`), `b14b951` (DaniSBR1). **Probado** (cxldsakeb): la traza muestra `open` → fd 3, `lseek` al final y al principio, `read` de 0x9858 bytes (los 39 000 del TOC) y `close`; el hilo 1 sale de `0x27D7E8`. Fusionado en `godofwar2` | DaniSBR1 y cxldsakeb |
| 2026-10-16 | Cuelgue deliberado en `0x18D5A0` (`nop`/`b` en bucle) | Error fatal de arranque: `0x2BF660` carga en el IOP un módulo mínimo que va dentro del ELF (`0x2DB590`) con `sceSifLoadStartModuleBuffer` y exige `id >= 0` y `(resultado & ~3) == 0`. El runtime no ejecuta módulos cargados desde la memoria del IOP | Override de `0x2D4850`: cargado y terminado con `NO_RESIDENT_END`, como en GoW1 | DaniSBR1 |
| 2026-10-16 | `[guest-branch:missing-target] kind=DirectJump op=EE scheduler target=0x1857f8` y `main thread dormant` | El hilo se reanuda tras una llamada virtual de la función del escáner `0x185748`. `ps2_recomp` llama `entry_XXXX` a las funciones de `entry_points` e `isEntryFunctionName` (`ps2_recompiler.cpp:283`) las excluye del registro de puntos de reanudación: **afecta a las 735** | Las funciones del escáner van ahora en `funcmap.csv` con su nombre (`vfn_`/`cfn_`/`fn_`; `tools/make_funcmap.py --extra`), no en `entry_points`: el recompilador registra sus puntos de reanudación (169 986 en total; `0x1857F8` → `cfn_00185748`). Sin tocar el fork. Hubo un TEMPORAL mientras tanto, ya retirado | DaniSBR1 |
| 2026-10-16 | Bucle activo en `0x1833C8` (`ra=0x181568`) | Búsqueda en el árbol rojo-negro del "RB NODE Heap" (`0x183330`) que no termina: nodos en `0x84000010`. El heap interno del runtime nacía en la misma zona que el heap principal del juego (`0x146318`: del final del `.bss` a `0x1FF7FF8`) y se pisaban | `runtime.setPrivateGuestHeap(0xA0000, 0xFF000)` en el override, como en GoW1 | DaniSBR1 |
| 2026-10-16 | Silencio tras `[SifInitRpc] Initialized`; `GOW2_THREAD_WATCH`: bucle activo en `0x2B6E58` (`sceSifBindRpc(0x80001300)` en bucle). `GOW2_BOOT_TRACE`: los 8 `sceSifLoadModule("")` devuelven -1 | El `SetupThread` del runtime ignora el bloque de argumentos (`$a3 = 0x2FBE00`): `main` recibe `argc = 0`, `0x1875E0` deja `0x2D7FE8 = -1` (valor de `.data`), `0x187630` devuelve rutas vacías, no se carga DBCMAN y su servidor nunca aparece | Override **TEMPORAL** de `0x1875E0`: con `argc = 0` hace como una consola (`argv[0] = "cdrom0:\SCUS_974.81;1"`). Cargan los 8 IRX y se enlazan sus servidores | DaniSBR1 |
| 2026-10-16 | Silencio tras `[SifInitRpc] Initialized` | El arranque (`0x188738`) repite `sceCdReadDvdDualInfo` mientras devuelva 0, y esta devuelve 0 sin llamar al IOP si el servidor S del CDVD no está listo (`0x2B67A8`). Como en GoW1 | Override de `0x2B6BF0`: doble capa, capa 1 en 2 083 872 | DaniSBR1 |

## Cambios necesarios en PS2Recomp

Lo que haya que cambiar en el runtime (rama `godofwar2`) y por qué. Si también sirve para GoW1, apuntarlo para llevarlo a `godofwar`.

| Commit | Qué | ¿Vale para GoW1? |
|---|---|---|
| `cf4e3f2` | Ventana: **F12** pantalla completa sin bordes, **F10** contador de rendimiento (PCSX2), `ps2_window_win.cpp` (F10 sin el modo menú de Windows) | Sí: es del runtime, no del juego |
| `d0b6162` | Backend del GS (`tryShuffle`): el origen Z16 es el target de profundidad con el ancho de la vista y, entre iguales, el último que escribió la profundidad. Bloqueo 13 | Sí, sin riesgo aparente: solo cambia qué target se elige cuando varios comparten la memoria de Z |
| `3b71410` | Contador de FPS: FPS interno estilo PCSX2 (`[P]` *flips* de `DISPFB` / `[B]` frames al GS), dibujado encima del panel de depuración; `GS::privilegedRegisters()` (lectura). raylib con `CUSTOMIZE_BUILD` y sin `SUPPORT_SCREEN_CAPTURE`/`SUPPORT_GIF_RECORDING` (su F12 de serie guardaba capturas) | Sí: comodidad de uso, sin efecto en la emulación. Ojo: `gs_frontend.h` obliga a recompilar entero |
| `8a4abab` | Backend del GS (`gs_hw_backend.cpp`, `tryShuffle`): los *channel shuffles* cuyo sprite acaba en una fracción de píxel (regla del GS `y0 <= y < y1`: el final es `ceil(y1)`) y la escala 1:1 comprobada en 1/16 de píxel. Bloqueo 12 | Sí, sin riesgo: solo acepta sprites que antes se descartaban (y que el GS sí dibuja) |
| `9ccffe1` | (1) `cop0_count` sigue al contador de ciclos del EE (`EeScheduler.cpp`). (2) El nombre de los módulos del IOP pierde el sufijo `;1`. (3) `IopNativeOverride::textSize` ata cada sustitución nativa a una compilación del módulo; sustituciones de LIBSD para la de GoW II | **Sí, con cuidado.** (1) y (2) no cambian nada en GoW1 (no lee `Count`; sus rutas no llevan `;1`). (3) ata las sustituciones de LIBSD de GoW1 a su tamaño, `0x4840`, sacado de su disco: comprobar que en GoW1 sigue habiendo sonido tras el `cherry-pick` |
| `df2a082` | `sysclib` `strtok` (35) con su posición guardada entre llamadas; `thbase` `GetSystemTimeLow` (43); los ordinales que `thbase`/`thsemap`/`thevent`/`sifcmd`/`sifman`/`sysclib` no atienden pasan al aviso `[IOP] unhandled import` (antes fallaban sin aviso y sin escribir `$v0`) | Sí, sin riesgo: si el SMPD de GoW1 no las usa, no cambia nada; el aviso ayuda a cualquier juego |
| `b14b951` (fusionado en `godofwar2`, probado) | `ioman` del IOP (`ps2xIOP/src/emulator/imports/iop_ioman.cpp`): `open`/`close`/`read`/`lseek` (ordinales 4/5/6/8) para `cdrom0:`, de solo lectura, sobre el disco a través de `IopCdvd` (nuevos `findDiscFile`/`readDiscFile`). Otros dispositivos (`host1:`) devuelven `-ENOENT`, como en una consola. Antes solo existían `AddDrv`/`DelDrv` y lo demás devolvía 0 | Sí, si algún IRX de GoW1 usa `ioman` (su SMPD lee por sectores con `cdvdman`, así que probablemente no le afecte) |
| (propuesta, sin hacer) | `SetupThread` (syscall `0x3C`, `ps2xRuntime/src/lib/Kernel/Syscalls/System.cpp`) debería rellenar el bloque de argumentos de `$a3` como el kernel de la PS2 (`argc = 1`, `argv[0] = "cdrom0:\<ELF>;1"`). Con eso sobraría el override TEMPORAL de `0x1875E0` | Probablemente: cualquier juego que mire `argv` |
