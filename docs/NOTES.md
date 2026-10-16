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
