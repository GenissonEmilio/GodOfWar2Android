# config/

Configuración del recompilador para GoW II. **Vacía al empezar: construirla es la fase 1** (`docs/ROADMAP.md`).

| Archivo | Qué es | De dónde sale |
|---|---|---|
| `gow.toml`, `gow_functions.csv` | Export de Ghidra (no se suben: llevan rutas locales y se regeneran) | `scripts/ghidra-analyze.ps1` |
| `funcmap.csv` | Mapa de funciones (`Name,Start,End,Size`) que usa `recompile.ps1` | `py -3 tools/make_funcmap.py`: las funciones del CSV de Ghidra sin sus etiquetas internas. Las que Ghidra no ve (`tools/scan_missing_functions.py --csv config/funcmap.csv`) van en `entry_points` de la plantilla |
| `recomp.template.toml` | Configuración de `ps2_recomp` con `@ELF@`, `@MAP@` y `@OUT@` (los rellena `recompile.ps1`): stubs, entry points y tabla `[mmio]` | Se escribe a mano, con `referencia/gow1/config/recomp.template.toml` como modelo y **las direcciones de GoW II** |
