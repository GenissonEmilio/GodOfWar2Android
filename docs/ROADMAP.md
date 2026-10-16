# Hoja de ruta

## Qué esperar

- **No empezamos de cero.** El runtime ya sabe dibujar el GS, ejecutar la VU1 y el IOP como código nativo, sonar, leer el mando y la memory card y reproducir vídeos: eso es lo que en GoW1 costó semanas. GoW II usa una evolución del mismo motor, así que mucho debería valer tal cual.
- **Lo que sí es nuevo:** el ELF de GoW II no tiene símbolos y sus direcciones son otras. Hay que construir el mapa de funciones, la configuración del recompilador (stubs de librerías de Sony y la tabla `[mmio]`) y los overrides, y arreglar los bloqueos propios del juego con el método de siempre: ejecutar, arreglar el **primer** bloqueo y repetir.
- `referencia/gow1/` es el modelo: qué stubs hicieron falta, qué accesos a registros se convierten en llamadas nativas y qué overrides necesitó GoW1. Para cada cosa hay que encontrar **su equivalente en GoW II**.

## Fases

Cada fase tiene un criterio de "hecho" comprobable.

### Fase 0: entorno (todos)

- [ ] Cada uno sigue [SETUP.md](SETUP.md) hasta `build-tools.ps1`. (cxldsakeb y DaniSBR1: hecho)
- [ ] Cada uno extrae su copia con `extract-iso.ps1` y todos tenemos el **mismo SHA1 del ELF**, apuntado en [NOTES.md](NOTES.md). (cxldsakeb y DaniSBR1: hecho, `E6A62A0F…`)

**Hecho cuando:** los 3 compilamos las herramientas y tenemos el ELF de referencia.

### Fase 1: mapa de funciones y primera configuración

- [x] Ghidra (`ghidra-analyze.ps1`) → `config/gow_functions.csv` y `config/gow.toml`. (DaniSBR1: 5 874 funciones)
- [x] Funciones que Ghidra no ve (`scan_missing_functions.py`). (DaniSBR1: 733, en `entry_points`)
- [x] `config/funcmap.csv` (`Name,Start,End,Size`) y `config/recomp.template.toml` con `@ELF@`, `@MAP@` y `@OUT@`, tomando como modelo los de `referencia/gow1/config/`. (DaniSBR1: primera versión; `recompile.ps1` genera C++ sin errores)
- [x] Stubs de las librerías de Sony (`sceMpeg*`, `sceIpu*`, `sceGs*`, `sceCd*`, `sceDma*`, libkernel…): localizar cada una en GoW II (las cadenas y los patrones de llamada ayudan; DeepSeek puede cruzar listas). (DaniSBR1: 76 de 79; los otros 3 no están enlazados en GoW II. SDK 3.0.0, ver NOTES.md)
- [x] Tabla `[mmio]`: los accesos del juego a registros del hardware que el runtime convierte en llamadas nativas. (DaniSBR1: 240 entradas de `ps2_analyzer`, equivalentes a la tabla de GoW1)

**Hecho cuando:** `recompile.ps1` genera C++ sin errores y el runner compila. (DaniSBR1, 2026-10-15: ya se cumple con la primera configuración; faltan los stubs y `[mmio]` para que tenga sentido pasar a la fase 2)

### Fase 2: primer arranque

- [ ] Arranca y carga los IRX (en el intérprete del IOP).
- [ ] Pasa la inicialización y empieza a leer datos del disco.
- [ ] Overrides imprescindibles (como el `sceIpuInit` o el arranque de GoW1, pero con sus direcciones).

**Hecho cuando:** llega a la primera pantalla (logos).

### Fase 3: menú y vídeos

**Hecho cuando:** se llega al menú y la intro se reproduce.

### Fase 4: jugable

- [ ] Partida, combate, HUD.
- [ ] VU1 e IOP nativos (capturas propias de GoW II).
- [ ] Guardar y cargar.

**Hecho cuando:** se juega el primer nivel a velocidad completa.
