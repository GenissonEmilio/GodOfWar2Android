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

## Datos del ELF y del disco

- **Punto de entrada `0x100008`** (como GoW1). Un solo segmento cargable: `0x100000`, 0x1FBDA8 bytes en el archivo y 0x27FE00 en memoria (hasta `0x37FE00`). Sin tabla de símbolos.
- **Secciones:** `.text`, `.reginfo`, `.data`, `.rodata`, `.sbss`, `.bss` y **26 `.DVP.overlay…` con su `.DVP.ovlytab`/`.DVP.ovlystrtab`**: los microprogramas de la VU van dentro del ELF como overlays. En GoW1 hubo que capturarlos ejecutando el juego (`GOW_VU1_CAPTURA`); aquí quizá se puedan sacar directamente del ELF para la VU1 nativa. Por investigar en la fase 4.
- **Disco: la misma estructura que GoW1.** `PART1.PAK` y `PART2.PAK` (~4 GB cada uno), `GODOFWAR.TOC`, `IOPRP300.IMG` y los IRX `989NOMID`, `DBCMAN`, `DS1O_S1`, `DS2U_D`, `LIBSD`, `MC2_D`, `MC2_S1`, `SIO2D`, `SIO2MAN`. El único cambio de nombre: **`SMPD.IRX`** (en GoW1, `SMPD_IOP.IRX`). Buena señal para que el runtime valga casi tal cual.
- **SDK de Sony 3.0.0** (`PsIIlibipu  3000`, `PsIIlibdma  3000`, `PsIIlibcdvd 3000` en `.data`, `0x2D9540`…). GoW1 usaba otra versión: por eso las funciones de librería **no tienen los mismos tamaños** que en GoW1 y no se pueden emparejar por tamaño (probado: solo cuadran 6 de 79). Hay que localizarlas por sus cadenas y por quién las llama.
- **Las librerías de Sony van al final de `.text`**: envoltorios de syscalls en `0x2CF520`–`0x2CFDA0` (los reconoce Ghidra) y cadenas de libgraph, libdma, libcdvd, libmpeg, libsifrpc, libdbc, libmc2 y la TTY en `.rodata` `0x2F8DC0`–`0x2FBB10`.

## Mapa de funciones (fase 1)

- **Ghidra** (`ghidra-analyze.ps1`, 2026-10-15): 5 874 funciones y 31 986 etiquetas de código. El CSV del export mezcla las dos cosas (`entry_XXXX`, `caseD_*`, `default*` son etiquetas dentro de funciones); `tools/make_funcmap.py` deja **solo las 5 874 funciones** en `config/funcmap.csv` (sin duplicados ni solapes). Con las etiquetas, `ps2_recomp` habría generado ~38 000 funciones solapadas. GoW1 también usaba solo funciones.
- **Escáner** (`scan_missing_functions.py --csv config/funcmap.csv`): **733 funciones que Ghidra no ve** (607 `vfn_`, 12 `fn_`, 114 `cfn_`; en GoW1 fueron 634). Van en `entry_points` de la plantilla. Muestreo: 525 de los 607 `vfn_` son funciones hoja cortas (típico de métodos virtuales) y los `cfn_` revisados a mano empiezan tras un `jr ra` o un epílogo.
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

## Diagnóstico de bloqueos (overrides/gow2_overrides.cpp)

Solo se activan con la variable de entorno; sin ella no cambian nada.

- `GOW2_THREAD_WATCH=<s>`: cada `<s>` segundos, los hilos del EE (`status`, espera, `pc`, `ra`). Para bloqueos silenciosos: si un hilo `status=0` repite el mismo `pc` con el ciclo avanzando, es un bucle activo. Como `GOW_THREAD_WATCH` en GoW1.
- `GOW2_BOOT_TRACE=1`: cuenta las llamadas a las funciones de la secuencia de arranque (`sceSifRebootIop`, `sceCdInit`, `sceSifInitRpc`…) y registra cada `sceSifLoadModule` (ruta y resultado) y cada `sceSifBindRpc` (servidor y resultado).
- Del runtime, sin recompilar: `PS2X_TRACE_RPC=<prefijo>` registra las llamadas RPC que llegan al IOP (p. ej. `8` para los servidores de Sony).
- Para probar un override sin recompilar todo: copiar solo `overrides/gow2_overrides.cpp` a `PS2Recomp/ps2xRuntime/src/runner/`, `touch` y `cmake --build` (~1 min; solo cambia su bloque del unity build). `recompile.ps1 -SyncOnly` vuelve a fechar los 6 600 archivos y recompila todo (~50 min). Ejecutar CMake desde Bash: en PowerShell, con `_common.ps1` cargado, el primer aviso por stderr aborta el comando.

## Funciones identificadas

Dirección = inicio de la función en el ELF de referencia.

| Dirección | Nombre | Qué hace | Equivalente en GoW1 | Estado | Quién |
|---|---|---|---|---|---|
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
| 2026-10-16 | Tras cargar los IRX, el hilo principal se queda en `pc=0x1833C8 ra=0x181568` (`GOW2_THREAD_WATCH`) | Por investigar: es el **siguiente** bloqueo (fase 2, objetivo 2) | — | — |
| 2026-10-16 | Silencio tras `[SifInitRpc] Initialized`; `GOW2_THREAD_WATCH`: bucle activo en `0x2B6E58` (`sceSifBindRpc(0x80001300)` en bucle). `GOW2_BOOT_TRACE`: los 8 `sceSifLoadModule("")` devuelven -1 | El `SetupThread` del runtime ignora el bloque de argumentos (`$a3 = 0x2FBE00`): `main` recibe `argc = 0`, `0x1875E0` deja `0x2D7FE8 = -1` (valor de `.data`), `0x187630` devuelve rutas vacías, no se carga DBCMAN y su servidor nunca aparece | Override **TEMPORAL** de `0x1875E0`: con `argc = 0` hace como una consola (`argv[0] = "cdrom0:\SCUS_974.81;1"`). Cargan los 8 IRX y se enlazan sus servidores | DaniSBR1 |
| 2026-10-16 | Silencio tras `[SifInitRpc] Initialized` | El arranque (`0x188738`) repite `sceCdReadDvdDualInfo` mientras devuelva 0, y esta devuelve 0 sin llamar al IOP si el servidor S del CDVD no está listo (`0x2B67A8`). Como en GoW1 | Override de `0x2B6BF0`: doble capa, capa 1 en 2 083 872 | DaniSBR1 |

## Cambios necesarios en PS2Recomp

Lo que haya que cambiar en el runtime (rama `godofwar2`) y por qué. Si también sirve para GoW1, apuntarlo para llevarlo a `godofwar`.

| Commit | Qué | ¿Vale para GoW1? |
|---|---|---|
| (propuesta, sin hacer) | `SetupThread` (syscall `0x3C`, `ps2xRuntime/src/lib/Kernel/Syscalls/System.cpp`) debería rellenar el bloque de argumentos de `$a3` como el kernel de la PS2 (`argc = 1`, `argv[0] = "cdrom0:\<ELF>;1"`). Con eso sobraría el override TEMPORAL de `0x1875E0` | Probablemente: cualquier juego que mire `argv` |
