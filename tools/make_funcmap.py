"""Genera config/funcmap.csv a partir del export de Ghidra (config/gow_functions.csv).

ExportPS2Functions.java mezcla en el CSV las funciones con las etiquetas de código que caen dentro
de ellas (entry_XXXXXXXX, caseD_*, default*: destinos de saltos y casos de switch). ps2_recomp
convierte cada fila en una función aparte, así que con las etiquetas saldrían ~38 000 funciones
solapadas en vez de ~5 900. Como en GoW1, funcmap.csv lleva solo las funciones; las entradas que
hagan falta de verdad (métodos virtuales, callbacks) van en entry_points de recomp.template.toml
(tools/scan_missing_functions.py).

    py -3 tools/make_funcmap.py
"""
import argparse
import csv
import os
import re

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LABEL = re.compile(r"^(entry_[0-9a-fA-F]+|caseD_.*|default.*)$")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--csv", default=os.path.join(REPO, "config", "gow_functions.csv"))
    parser.add_argument("--out", default=os.path.join(REPO, "config", "funcmap.csv"))
    args = parser.parse_args()

    with open(args.csv, newline="") as f:
        rows = list(csv.DictReader(f))
    funcs = sorted((r for r in rows if not LABEL.match(r["Name"])), key=lambda r: int(r["Start"], 16))

    starts = [int(r["Start"], 16) for r in funcs]
    if len(starts) != len(set(starts)):
        raise SystemExit("Hay funciones con la misma dirección de inicio: revisa el export de Ghidra.")
    for a, b in zip(funcs, funcs[1:]):
        if int(a["End"], 16) > int(b["Start"], 16):
            raise SystemExit("Se solapan %s y %s: revisa sus límites en Ghidra." % (a["Name"], b["Name"]))

    with open(args.out, "w", newline="") as f:
        f.write("Name,Start,End,Size\n")
        for r in funcs:
            f.write("%s,%s,%s,%s\n" % (r["Name"], r["Start"], r["End"], r["Size"]))
    print("%d funciones (de %d filas del export) -> %s" % (len(funcs), len(rows), args.out))


if __name__ == "__main__":
    main()
