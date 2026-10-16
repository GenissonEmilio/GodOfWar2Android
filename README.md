# God of War II: port nativo para PC (PS2 → Windows)

Recompilación estática de **God of War II** (PS2, 2007) a un ejecutable nativo de Windows con [PS2Recomp](https://github.com/ran-j/PS2Recomp).

Es el segundo proyecto del equipo después de [God of War](https://github.com/cxldsakeb/ROMS-nativas-PS2), que ya es jugable a ~60 FPS sobre el runtime de Waffle565. GoW II usa una evolución del mismo motor, así que partimos de ese runtime (GS nativo, VU1 e IOP compilados por adelantado, sonido, mando, vídeos, memory card) y lo que hay que hacer aquí es lo propio del juego: el mapa de funciones, la configuración del recompilador y los overrides.

> [!IMPORTANT]
> Este repositorio **no contiene nada del juego** (ISO, ELF, IRX, datos del disco) ni el C++ generado a partir de él. Cada uno necesita **su propia copia legal** de God of War II.

## Estado

**Fase 4 (jugable), en curso.** El juego arranca, pasa los logos, muestra la pantalla de título y el menú, reproduce el primer vídeo y **llega a la primera zona jugable con sonido, con el escenario y las sombras bien dibujados**. Todavía no es jugable de verdad: quedan fallos de imagen (por ejemplo, en el cielo) y va a pocos FPS.

| Fase | Estado |
|---|---|
| 0. Entorno | Hecha (falta un miembro del equipo) |
| 1. Mapa de funciones y configuración del recompilador | Hecha: 6 607 funciones, 76 stubs de librerías de Sony, 240 accesos a registros |
| 2. Primer arranque | Hecha: carga los 8 módulos del IOP, lee el disco y llega a la pantalla de título (11 bloqueos resueltos) |
| 3. Menú y vídeos | Hecha |
| 4. Jugable a velocidad completa | En curso: fallos de imagen restantes, velocidad (VU1 e IOP nativos), guardar y cargar |

Detalle en [docs/ROADMAP.md](docs/ROADMAP.md); cada bloqueo, con su causa y su arreglo, en [docs/NOTES.md](docs/NOTES.md).

## Empezar

Sigue [docs/SETUP.md](docs/SETUP.md). En resumen: extraer tu ISO con `scripts/extract-iso.ps1` (el SHA1 del ELF tiene que coincidir con el de NOTES.md), compilar las herramientas, `scripts/recompile.ps1` y `scripts/run.ps1 -Config RelWithDebInfo` (la primera compilación tarda ~40–50 min).

**Controles:** el mando del PC o el teclado. Teclado: flechas = cruceta, `Z`/`X`/`C`/`V` = cuadrado/cruz/círculo/triángulo, `Intro` = Start, `Q`/`E` = L1/R1, `1`/`3` = L2/R2, `WASD` e `IJKL` = sticks.

El contexto para Claude Code (y para quien se una) está en [CLAUDE.md](CLAUDE.md).

## Créditos

- [**PS2Recomp**](https://github.com/ran-j/PS2Recomp) de ran-j y colaboradores (GPLv3): el recompilador y la base del runtime.
- [**Waffle565/God-Of-War-Native-Port**](https://github.com/Waffle565/God-Of-War-Native-Port): su rama `runtime` (GPL-3.0) es la base de nuestro runtime; de su rama `main`, a través de KIexster, salen `tools/iso_extract.py`, `tools/iop/` y `tools/vu1/vu1n_gen.py`.
- [**KIexster/god-of-war-recomp**](https://github.com/KIexster/god-of-war-recomp) (GPLv3): los parches que montan esa base sobre PS2Recomp y, para GoW1, la configuración y los overrides que guardamos como referencia en `referencia/gow1/`.
- [**PCSX2**](https://github.com/PCSX2/pcsx2) (GPLv3+): referencia del hardware.

## Licencia

**GPLv3** ([LICENSE](LICENSE)), igual que PS2Recomp. God of War es una marca de Sony Interactive Entertainment; este proyecto no está afiliado ni respaldado por Sony.
