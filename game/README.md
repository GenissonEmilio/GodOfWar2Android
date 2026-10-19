# game/ (local only)

This is where **your** copy of the game lives. Git ignores everything in this folder except this file.

It is filled in with:

```powershell
.\scripts\extract-iso.ps1 -IsoPath "path\to\God of War II.iso"
```

Result:

```
game/
  disc/            ← the DVD files (both layers), each one at its original sector
    SCUS_974.81    ← main ELF (USA version; the name depends on the region)
    .disc_layout   ← sector map that the runtime uses to read the data
    IOP_MOD/       ← copy of the IRX, where the game looks for them
  elf-name.txt     ← name of the detected ELF (read by the scripts)
  iso-path.txt     ← path to your ISO
```

**Never** upload anything from here to the repo or share it on Discord or anywhere else. Everyone uses their own copy of the game.
