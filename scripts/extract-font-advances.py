#!/usr/bin/env python3
"""Extrait les avances de glyphes d'un en-tête GFXfont généré (src/fonts/*.h) vers un en-tête de test.

Usage : extract-font-advances.py <en-tête de police> <nom du tableau> <sortie>

Les tests natifs ne peuvent pas inclure les en-têtes de police (ils incluent LovyanGFX.hpp) : ils lisent
à la place cette copie des largeurs réelles, une avance par code de U+0020 à U+00FF. À régénérer à chaque
changement de police ; ne pas modifier à la main.
"""
import re
import sys

GLYPH_TABLE = re.compile(r"Glyphs\[\] PROGMEM = \{(.*?)\n\};", re.S)
GLYPH = re.compile(r"\{\s*(-?\d+),\s*(-?\d+),\s*(-?\d+),\s*(-?\d+),\s*(-?\d+),\s*(-?\d+)\s*\}")
FONT = re.compile(r"lgfx::GFXfont \w+ = \{[^}]*?, (0x[0-9A-Fa-f]+), (0x[0-9A-Fa-f]+), (\d+)\};")


def main(argv):
    if len(argv) != 4:
        print(__doc__)
        return 2
    header, name, output = argv[1], argv[2], argv[3]
    with open(header, encoding="utf-8") as handle:
        source = handle.read()
    table = GLYPH_TABLE.search(source)
    font = FONT.search(source)
    if table is None or font is None:
        print("Format d'en-tête inattendu : " + header)
        return 1
    first, last, y_advance = int(font.group(1), 16), int(font.group(2), 16), int(font.group(3))
    glyphs = [tuple(int(v) for v in glyph) for glyph in GLYPH.findall(table.group(1))]
    advances = [glyph[3] for glyph in glyphs]
    if first != 0x20 or last != 0xFF or len(advances) != last - first + 1:
        print("La police doit couvrir U+0020 à U+00FF sans trou")
        return 1

    # Géométrie verticale (pixels, par rapport à la ligne de base) : champs height = glyph[2], yOffset = glyph[5].
    accent_height = max(-glyphs[code - first][5] for code in range(0xC0, 0x100) if code not in (0xD7, 0xF7))
    descender_depth = max(glyphs[ord(c) - first][2] + glyphs[ord(c) - first][5] for c in "gjpqy")

    lines = [
        "// Généré par scripts/extract-font-advances.py à partir de %s : ne pas modifier à la main." % header.replace("\\", "/").split("/")[-1],
        "// Largeurs réelles (avance en pixels) de la police, une par code de U+0020 à U+00FF.",
        "#ifndef %s_ADVANCES_H" % name.upper(),
        "#define %s_ADVANCES_H" % name.upper(),
        "",
        "static const int %sYAdvance = %d;" % (name, y_advance),
        "// Hauteur de l'accent le plus haut (U+00C0 à U+00FF) au-dessus de la ligne de base.",
        "static const int %sAccentHeight = %d;" % (name, accent_height),
        "// Profondeur du descendant le plus bas (g, j, p, q, y) sous la ligne de base.",
        "static const int %sDescenderDepth = %d;" % (name, descender_depth),
        "",
        "static const int %sAdvances[] = {" % name,
    ]
    for start in range(0, len(advances), 16):
        lines.append("    " + ", ".join(str(a) for a in advances[start:start + 16]) + ",")
    lines += ["};", "", "#endif", ""]
    with open(output, "w", encoding="utf-8", newline="\n") as handle:
        handle.write("\n".join(lines))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
