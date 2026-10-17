# tools/

Scripts de análisis en Python para investigar bloqueos sin abrir Ghidra. Leen **tu copia local** del juego (`game/disc/`); no contienen ni generan nada del juego que se suba al repo.

```powershell
py -3 -m pip install capstone pyelftools
```

| Script | Para qué |
|---|---|
| `elf_dis.py 0xINICIO 0xFIN` | Desensamblar un rango del ELF. `--str 0xDIR …` lee cadenas (nombres de recursos, mensajes de error). |
| `elf_xref.py "jal 0x13d710"` | Buscar quién llama a una función o quién accede a una global (`"-0x3b24("` con base `0x2A0000`). |
| `irx_dis.py game/disc/X.IRX 0xINICIO 0xFIN` | Desensamblar un módulo del IOP marcando sus imports (`sysmem:4`, `cdvdman:6`…). Direcciones relativas al módulo: súmales la `base=` que sale en el log al cargarlo. |
| `irx_imports.py game/disc/IOP_MOD/*.IRX` | Lista, por librería y ordinal, las funciones del IOP que piden los IRX (`thbase 43 SMPD.IRX`) y las librerías que exportan ellos mismos (`libsd`, `snd989`…). Para cruzarlo con lo que implementa el runtime (NOTES.md, "Imports del IOP que usa GoW II"). Sin dependencias. |
| `vu_dis.py volcado.bin 0xINICIO 0xFIN` | Desensamblar microcódigo de la VU1 a partir de un volcado (16 KB de código al principio): la parte upper (`MUL`, `MADD`, `CLIP`, `FTOI4`…) y la lower (saltos, aritmética entera, cargas/escrituras, `DIV`, `XGKICK`), con los bits I/E/M/D/T. `--trace traza.txt [salida.txt]` anota una traza de instrucciones (líneas `PC c=… U=… L=…`, como la de `[vu-trace]`) con el desensamblado de cada línea. Sin dependencias. |
| `scan_missing_functions.py` | Buscar funciones que Ghidra no detecta: métodos virtuales (`vfn_`), destinos de `JAL` en huecos (`fn_`) y punteros que el código construye con `lui`+`addiu` (`cfn_`, callbacks). **Ejecutadlo después de cada export de Ghidra** y regenerad `funcmap.csv` con `make_funcmap.py --extra` (no van en `entry_points`: ahí el recompilador no registra sus puntos de reanudación). |
| `make_funcmap.py [--extra out/scan_missing.txt]` | Genera `config/funcmap.csv`: las funciones del export de Ghidra (sin las etiquetas internas `entry_XXXX`, `caseD_*`) y, con `--extra`, las del escáner. |
| `gsdump/gsdump_z.py volcado.gs [frame]` | Lee un volcado del GS de PCSX2 (`.gs` **sin comprimir**: *GS Dump Compression = Uncompressed*) y resume, por estado de dibujo y en orden de aparición, cuántos vértices hay y su rango de Z (`FRAME`, `ZBUF`, `TEST`, `TEX0`). Para comparar con los registros del port lo que llega al GS en la PS2 emulada. |
| `gsdump/gsdump_zsim.py volcado.gs frame [alpha]` | Simula solo el buffer de Z del frame en una cuadrícula de píxeles con las reglas del GS (interpolación lineal, `ZTST`, `ZMSK`, saturación; con `alpha`, la prueba de alfa con alfa de textura `0x80`) e imprime qué dibujo deja el Z en cada punto. Así se vio que faltaba el borrado del Z (bloqueo 15). |
| `gsdump/gsdump_zclear.py volcado.gs frame` | Lista los sprites con formato de Z dibujados en la memoria del Z (`FRAME.FBP` = 120): coordenadas, color, `XYOFFSET`, `SCISSOR`. |

Capstone decodifica MIPS32 estándar: lo propio del Emotion Engine (`sq`/`lq` de 128 bits, MMI, `daddu`…) sale como `.byte`. Para eso, Ghidra con la extensión EE o los comentarios `// 0x…` de los archivos de `generated/`.
