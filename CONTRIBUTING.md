# Cómo colaborar

Gracias por querer ayudar. El proyecto está en marcha y cualquier prueba, informe o arreglo cuenta. La documentación está en español, pero los issues y PRs en inglés son bienvenidos.

## La regla que no se rompe

**Nada del juego entra en el repositorio ni en los issues:** ni ISO, ni ELF, ni IRX, ni archivos del disco, ni el C++ de `generated/`, ni capturas de la VU1 o del IOP (`out/capturas/`), ni partidas guardadas. Ya están en `.gitignore`; si `git status` te enseña algo de eso, no lo añadas. Los issues que pidan o enlacen copias del juego se cierran.

Para enseñar un fallo de imagen, describe la escena o enlaza un vídeo tuyo alojado fuera del repo.

## Informar de un fallo

Abre un issue con:

1. **SHA1 de tu ELF** (lo imprime `scripts/extract-iso.ps1`). Si no coincide con el de [docs/NOTES.md](docs/NOTES.md), las direcciones no valen para tu copia: dilo en el issue, pero no esperes que funcione.
2. **Commit** del repo y del submódulo (`git rev-parse --short HEAD` y `git -C PS2Recomp rev-parse --short HEAD`).
3. Si tienes la **VU1 y el IOP nativos** generados (SETUP.md, 7b) o no.
4. Qué hiciste, qué esperabas y qué pasó. Las **últimas líneas del log** de `out/logs/` (solo texto) ayudan mucho.

Antes, mira la tabla de bloqueos y "Estado del repositorio" en [docs/NOTES.md](docs/NOTES.md): puede que ya esté anotado.

## Mandar un arreglo

1. Haz un fork y una rama por cambio.
2. **Primero intenta un override** (`overrides/`). Si el arreglo es del runtime, va en el fork [DaniSBR1/PS2Recomp](https://github.com/DaniSBR1/PS2Recomp), rama `godofwar2`: abre ahí el PR (comentarios del código en inglés) y aquí otro con el nuevo puntero del submódulo.
3. Las direcciones **solo valen para el ELF de referencia** (SHA1 en NOTES.md). Las de GoW1 no valen aquí.
4. Si encuentras algo (una función, la causa de un bloqueo), **anótalo en `docs/NOTES.md`** en el mismo PR. Es lo que permite que los demás sigan donde lo dejaste.
5. Los apaños temporales (`ret0`, saltarse una función…) llevan un comentario `TEMPORAL` y una fila en NOTES.md.
6. Sin trazas de diagnóstico en el runtime: déjalas en tu copia local.

Los PRs de fuera del equipo los revisa alguien del equipo antes de fusionarlos.

## Licencia

Al contribuir aceptas que tu aportación se publique bajo **GPLv3**, como el resto del proyecto. Si traes código de otro proyecto, tiene que ser compatible con GPLv3 y citar su origen en un comentario.
