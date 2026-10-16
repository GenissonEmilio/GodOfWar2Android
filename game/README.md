# game/ (solo local)

Aquí vive **tu** copia del juego. Git ignora todo lo que hay en esta carpeta salvo este archivo.

Se rellena con:

```powershell
.\scripts\extract-iso.ps1 -IsoPath "ruta\a\God of War II.iso"
```

Resultado:

```
game/
  disc/            ← los archivos del DVD (las dos capas), cada uno en su sector original
    SCUS_974.81    ← ELF principal (versión USA; el nombre depende de la región)
    .disc_layout   ← mapa de sectores que usa el runtime para leer los datos
    IOP_MOD/       ← copia de los IRX, donde el juego los busca
  elf-name.txt     ← nombre del ELF detectado (lo leen los scripts)
  iso-path.txt     ← ruta a tu ISO
```

**Nunca** subas nada de aquí al repo ni lo compartas por Discord u otros sitios. Cada uno usa su propia copia del juego.
