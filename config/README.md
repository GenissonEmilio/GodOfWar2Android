# config/

Configuración del recompilador para GoW II. **Vacía al empezar: construirla es la fase 1** (`docs/ROADMAP.md`).

| Archivo | Qué es | De dónde sale |
|---|---|---|
| `gow.toml`, `gow_functions.csv` | Export de Ghidra | `scripts/ghidra-analyze.ps1` |
| `funcmap.csv` | Mapa de funciones (`Name,Start,End,Size`) que usa `recompile.ps1` | El CSV de Ghidra más las funciones que añade `tools/scan_missing_functions.py` |
| `recomp.template.toml` | Configuración de `ps2_recomp` con `@ELF@`, `@MAP@` y `@OUT@` (los rellena `recompile.ps1`): stubs, entry points y tabla `[mmio]` | Se escribe a mano, con `referencia/gow1/config/recomp.template.toml` como modelo y **las direcciones de GoW II** |
