# God of War II: port nativo para PC (PS2 → Windows)

Recompilación estática de **God of War II** (PS2, 2007) a un ejecutable nativo de Windows con [PS2Recomp](https://github.com/ran-j/PS2Recomp).

Es el segundo proyecto del equipo después de [God of War](https://github.com/cxldsakeb/ROMS-nativas-PS2), que ya es jugable a ~60 FPS sobre el runtime de Waffle565. GoW II usa una evolución del mismo motor, así que partimos de ese runtime (GS nativo, VU1 e IOP compilados por adelantado, sonido, mando, vídeos, memory card) y lo que hay que hacer aquí es lo propio del juego: el mapa de funciones, la configuración del recompilador y los overrides.

> [!IMPORTANT]
> Este repositorio **no contiene nada del juego** (ISO, ELF, IRX, datos del disco) ni el C++ generado a partir de él. Cada uno necesita **su propia copia legal** de God of War II.

## Estado

**Fase 0: entorno.** Ver [docs/ROADMAP.md](docs/ROADMAP.md) y [docs/NOTES.md](docs/NOTES.md).

## Empezar

Sigue [docs/SETUP.md](docs/SETUP.md). El contexto para Claude Code (y para quien se una) está en [CLAUDE.md](CLAUDE.md).

## Créditos

- [**PS2Recomp**](https://github.com/ran-j/PS2Recomp) de ran-j y colaboradores (GPLv3): el recompilador y la base del runtime.
- [**Waffle565/God-Of-War-Native-Port**](https://github.com/Waffle565/God-Of-War-Native-Port): su rama `runtime` (GPL-3.0) es la base de nuestro runtime; de su rama `main`, a través de KIexster, salen `tools/iso_extract.py`, `tools/iop/` y `tools/vu1/vu1n_gen.py`.
- [**KIexster/god-of-war-recomp**](https://github.com/KIexster/god-of-war-recomp) (GPLv3): los parches que montan esa base sobre PS2Recomp y, para GoW1, la configuración y los overrides que guardamos como referencia en `referencia/gow1/`.
- [**PCSX2**](https://github.com/PCSX2/pcsx2) (GPLv3+): referencia del hardware.

## Licencia

**GPLv3** ([LICENSE](LICENSE)), igual que PS2Recomp. God of War es una marca de Sony Interactive Entertainment; este proyecto no está afiliado ni respaldado por Sony.
