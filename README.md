# God of War II: port nativo para PC (PS2 → Windows)

> [!NOTE]
> **Trabajo en curso (WIP).** Todavía no hay una versión para descargar y jugar. Lo que hay es un avance real y documentado: el juego ya llega a la primera zona jugable, con sonido, cinemáticas y memory card, a ~60 FPS. Si quieres seguirlo, ayudar o basar tu propio fork en él, estás en el sitio adecuado.

Recompilación estática de **God of War II** (PS2, 2007) a un ejecutable nativo de Windows con [PS2Recomp](https://github.com/ran-j/PS2Recomp). No es un emulador: el código del juego se traduce a C++ y se compila como un programa de PC, junto a un runtime que hace de PS2 (GS, VU1, IOP, sonido, mando, vídeos y memory card).

Es el segundo proyecto del equipo después de [God of War](https://github.com/cxldsakeb/ROMS-nativas-PS2), que ya es jugable a ~60 FPS sobre el runtime de Waffle565. GoW II usa una evolución del mismo motor, así que partimos de ese runtime y lo que hacemos aquí es lo propio del juego: el mapa de funciones, la configuración del recompilador, los overrides y los arreglos del runtime que pide.

> [!IMPORTANT]
> Este repositorio **no contiene nada del juego** (ISO, ELF, IRX, datos del disco, imágenes) ni el C++ generado a partir de él, y **nunca lo va a contener**. Cada uno necesita **su propia copia legal** de God of War II (NTSC-U, `SCUS-97481`, v1.01). No pidas ni compartas ISOs en los issues.

## Estado

**Fase 4 (jugable), en curso.** Lo que funciona hoy, comprobado jugando:

- Arranca, pasa los logos, la pantalla de título y el menú.
- **Cinemáticas** a 30 imágenes por segundo, con el audio a la par.
- **Primera zona jugable (Rodas)** con sonido, escenario, sombras y combate.
- **Memory card:** formatea, guarda y carga.
- **Velocidad:** 60 FPS en el menú y ~44–59 FPS en la primera zona con combate (medido en un PC del equipo), con la VU1 y el IOP compilados a código nativo.

Lo que falta: algunos fallos de imagen (una línea azul que acompaña a Kratos, líneas onduladas bajo el agua, un "fantasma" a su izquierda), revisar el resto del juego más allá de la primera zona y llegar a 60 FPS estables en combate.

| Fase | Estado |
|---|---|
| 0. Entorno | Hecha |
| 1. Mapa de funciones y configuración del recompilador | Hecha: 6 607 funciones, 76 stubs de librerías de Sony, 240 accesos a registros |
| 2. Primer arranque | Hecha: carga los 8 módulos del IOP, lee el disco y llega a la pantalla de título |
| 3. Menú y vídeos | Hecha |
| 4. Jugable a velocidad completa | En curso: 18 bloqueos resueltos hasta ahora; quedan detalles de imagen y velocidad en combate |

Detalle en [docs/ROADMAP.md](docs/ROADMAP.md). Cada bloqueo, con su causa y su arreglo, está en [docs/NOTES.md](docs/NOTES.md): es también una buena guía de cómo se lleva un juego de PS2 a PC con PS2Recomp.

## Empezar

Hace falta Windows, Visual Studio 2022 y tu copia del juego. Sigue [docs/SETUP.md](docs/SETUP.md). En resumen: extraer tu ISO con `scripts/extract-iso.ps1` (el SHA1 del ELF tiene que coincidir con el de NOTES.md), compilar las herramientas, `scripts/recompile.ps1` y `scripts/run.ps1 -Config RelWithDebInfo` (la primera compilación tarda ~20–50 min según el PC). Para ir a velocidad completa hay que generar la VU1 y el IOP nativos con datos de tu propia partida (SETUP.md, sección 7b).

**Controles:** el mando del PC o el teclado. Teclado: flechas = cruceta, `Z`/`X`/`C`/`V` = cuadrado/cruz/círculo/triángulo, `Intro` = Start, `Q`/`E` = L1/R1, `1`/`3` = L2/R2, `WASD` e `IJKL` = sticks.

**Teclas de la ventana:** `F12` = pantalla completa / ventana · `F10` = contador de FPS (como el de PCSX2) · `F1` = panel de depuración.

## Colaborar

Toda ayuda es bienvenida: probar en tu PC, informar de fallos o mandar arreglos. Lee [CONTRIBUTING.md](CONTRIBUTING.md) antes de abrir un issue o un PR. El runtime vive en nuestro fork de PS2Recomp ([DaniSBR1/PS2Recomp](https://github.com/DaniSBR1/PS2Recomp), rama `godofwar2`), que es el submódulo `PS2Recomp/`.

Y si prefieres llevarlo por tu cuenta: es GPLv3, así que puedes hacer tu propio fork siempre que mantengas la misma licencia y cites el origen.

El contexto para Claude Code (y para quien se una) está en [CLAUDE.md](CLAUDE.md).

## English summary

**Work in progress.** A static recompilation of God of War II (PS2) into a native Windows executable, built with [PS2Recomp](https://github.com/ran-j/PS2Recomp) on top of the runtime that already runs the first God of War at ~60 FPS. There is no playable release yet, but the game boots, plays its cutscenes with audio, saves and loads, and reaches the first playable area (Rhodes) at ~60 FPS in menus and ~44–59 FPS in combat. **No game files are included: you need your own copy** (NTSC-U, SCUS-97481, v1.01). Docs are in Spanish; issues and PRs in English are welcome. Licensed under GPLv3: forks are welcome too.

## Créditos

- [**PS2Recomp**](https://github.com/ran-j/PS2Recomp) de ran-j y colaboradores (GPLv3): el recompilador y la base del runtime.
- [**Waffle565/God-Of-War-Native-Port**](https://github.com/Waffle565/God-Of-War-Native-Port): su rama `runtime` (GPL-3.0) es la base de nuestro runtime; de su rama `main`, con permiso del autor y a través de KIexster, salen `tools/iso_extract.py`, `tools/iop/` y `tools/vu1/vu1n_gen.py`.
- [**KIexster/god-of-war-recomp**](https://github.com/KIexster/god-of-war-recomp) (GPLv3): los parches que montan esa base sobre PS2Recomp y, para GoW1, la configuración y los overrides que guardamos como referencia en `referencia/gow1/`.
- [**PCSX2**](https://github.com/PCSX2/pcsx2) (GPLv3+): referencia del hardware.
- Equipo: [@cxldsakeb](https://github.com/cxldsakeb), [@DaniSBR1](https://github.com/DaniSBR1) y [@Alejrr2](https://github.com/Alejrr2).

## Licencia

**GPLv3** ([LICENSE](LICENSE)), igual que PS2Recomp. God of War es una marca de Sony Interactive Entertainment; este proyecto no está afiliado ni respaldado por Sony. No se distribuye ningún archivo del juego ni nada derivado de él.
