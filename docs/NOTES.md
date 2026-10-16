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
- **Stubs por nombre:** además de los 3 de la plantilla, `ps2_recomp` convierte en stub los envoltorios a los que Ghidra pone nombre de librería (`sceSifSetDma`, `sceSifDmaStat`, `sceSifSetDChain`, `sceSifSetReg`, `sceSifGetReg`, `sceSifStopDma`, `isceSifSetDma`, `isceSifSetDChain`): 11 en total. En GoW1 esos se recompilaban como código normal que hace la syscall. **A vigilar en la fase 2.**
- No hay que meter los 47 envoltorios de syscalls que exporta Ghidra en `stubs`: GoW1 no lo hacía (`patch_syscalls = false`, el runtime trata la instrucción `syscall`).

## Stubs de librerías de Sony

Los de la plantilla de GoW1 (79), buscados en GoW II. Solo entra en `config/recomp.template.toml` lo verificado en el ELF de GoW II.

| Función | Dirección en GoW II | Cómo se verificó | Quién |
|---|---|---|---|
| `StartThread` | `0x002D0768` | función en C de libkernel (SDK 3.0): llama a `ReferThreadStatus` y acaba en el envoltorio de la syscall `0x22` (`0x2CF740`), que solo ella llama. En GoW1 el stub también estaba en una función de 376 bytes, como esta. Además cuadra por tamaño con GoW1 | DaniSBR1 |
| `iWakeupThread` | `0x002CF860` | envoltorio de la syscall `-0x34`. **Duda:** en el SDK 3.0 hay una función en C delante (`0x2D0600`: mira `iGetThreadId`, syscall `-0x2F`, y luego llama aquí). En GoW1 el stub caía a mitad de una función, lo que encaja más con el envoltorio crudo. Revisar si da problemas en la fase 2 | DaniSBR1 |
| `InitTLB` | `0x002CFD90` | envoltorio de la syscall `0x82`; una sola llamada, en `0x2D65DC` (arranque) | DaniSBR1 |

Candidatas aún sin verificar (salen de las cadenas que usan; falta comprobar argumentos y vecinas antes de meterlas en la plantilla):

| Candidata | Dirección | Pista |
|---|---|---|
| `sceGsSyncPath` | `0x2B5BC8` | usa `"sceGsSyncPath: DMA Ch.1 does not terminate"` y sus hermanas |
| `sceCdInit` | `0x2B6900` | usa `"Libcdvd bind err %d CD_Init %d"` y `"Libcdvd Exit"` (modo `SCECdEXIT`) |
| `sceSifRebootIop` | `0x2D4C88` | usa `"rom0:UDNL"`, y por tamaño coincide con GoW1 (dos métodos independientes) |
| `sceSifAllocIopHeap` | `0x2D4360` | por tamaño (ventana de 3 funciones) |
| `_sceMpegFlush`, `sceMpegDemuxPssRing`, `sceMpegAddStrCallback` | `0x2ACDA8`, `0x2AFDD8`, `0x2B01D8` | por tamaño (ventana de 7 funciones: libmpeg no cambió) |
| `sceOpen`, `sceClose`, `sceRead` | `0x2D3BF8`, `0x2D3E88`, `0x2D4008` | usan `SceStdioOpenSema`, `…CloseSema`, `…ReadSema` |
| `sceTtyHandler` (?) | `0x2D0978` | usa las cadenas `"TTY: …"` |
| `InitThread` (?) | `0x2D0510` | usa `"SceKerneltopThread"` |
| zona de libmpeg/libipu | `0x2AB6E0` | usa `"sceMpegGetPicture is aborted"` y `"Need to re-setup libipu"` |
| zona de libdma | `0x2B5F40` | usa `"libdma: sync timeout"` |

Pendientes (76): `sceMpeg*` (10), `sceIpu*` (4), `sceGs*` (4), `sceDma*` (5), `sceCd*` (4), `sceSif*` cmd/rpc/iopheap/loadfile (≈20), `sceFs*`/`sceOpen`/`sceClose`/`sceRead`, `sceTty*` (4), `sceDeci2*` (5), libc (`memclr`, `memcmp`, `rand`, `strrchr`, `read`, `write`, `exit`, `__divdi3`), `InitThread`, `InitAlarm`, `_sceSDC`, `_sceIDC`.

## Funciones identificadas

Dirección = inicio de la función en el ELF de referencia.

| Dirección | Nombre | Qué hace | Equivalente en GoW1 | Estado | Quién |
|---|---|---|---|---|---|

Estados: `investigando` · `binding temporal (ret0/ret1)` · `binding real` · `reimplementada`.

## Bloqueos

Los más recientes arriba. Un bloqueo = el primer error del log que impide avanzar.

| Fecha | Síntoma (línea del log) | Causa | Solución / PR | Quién |
|---|---|---|---|---|

## Cambios necesarios en PS2Recomp

Lo que haya que cambiar en el runtime (rama `godofwar2`) y por qué. Si también sirve para GoW1, apuntarlo para llevarlo a `godofwar`.

| Commit | Qué | ¿Vale para GoW1? |
|---|---|---|
