#!/usr/bin/env python3
"""Génère une police GFXfont (LovyanGFX) en 1 bit à partir d'un fichier TrueType.

Usage : python3 scripts/generate-gfx-font.py <police.ttf> <taille en px> <nom> <sortie.h>
Dépendance : pip install freetype-py

Couvre U+0020-U+00FF (Latin-1). Un code que la police ne contient pas (les contrôles
0x7F-0x9F) reçoit le glyphe de `?`, comme le suppose `ReadFontMetrics` (lib/rendering).
La sortie est déterministe : même police, même taille, même fichier.
"""
import sys

import freetype

FIRST = 0x20
LAST = 0xFF
FILL_CODE = ord("?")


def read_glyph(face, code):
    """Retourne (largeur, hauteur, avance, xOffset, yOffset, bits) du glyphe `code`.

    `bits` est la liste des pixels, ligne par ligne, 1 = encre.
    """
    face.load_char(chr(code), freetype.FT_LOAD_RENDER | freetype.FT_LOAD_TARGET_MONO)
    slot = face.glyph
    bitmap = slot.bitmap
    bits = []
    for y in range(bitmap.rows):
        for x in range(bitmap.width):
            byte = bitmap.buffer[y * bitmap.pitch + x // 8]
            bits.append((byte >> (7 - x % 8)) & 1)
    advance = (slot.advance.x + 32) >> 6
    return bitmap.width, bitmap.rows, advance, slot.bitmap_left, -slot.bitmap_top, bits


def pack(bits):
    """Empaquette les pixels de gauche à droite et de haut en bas, 8 par octet, bit de poids fort d'abord.

    Pas de remplissage entre les lignes (format Adafruit GFX) : seul le dernier octet est complété.
    """
    data = []
    for i in range(0, len(bits), 8):
        byte = 0
        for j, bit in enumerate(bits[i : i + 8]):
            byte |= bit << (7 - j)
        data.append(byte)
    return data


def generate(ttf, size, name):
    face = freetype.Face(ttf)
    face.set_pixel_sizes(0, size)
    y_advance = (face.size.height + 32) >> 6

    bitmaps = []
    glyphs = []
    filler = None
    for code in range(FIRST, LAST + 1):
        missing = face.get_char_index(code) == 0
        if missing and filler is not None:
            glyphs.append(filler)
            continue
        width, height, advance, x_offset, y_offset, bits = read_glyph(face, FILL_CODE if missing else code)
        record = (len(bitmaps), width, height, advance, x_offset, y_offset)
        bitmaps.extend(pack(bits))
        glyphs.append(record)
        if code == FILL_CODE:
            filler = record
    return bitmaps, glyphs, y_advance


def to_header(bitmaps, glyphs, y_advance, name, ttf, size):
    lines = [
        "// Généré par scripts/generate-gfx-font.py : ne pas modifier à la main.",
        "// Police : %s, %d px, U+0020-U+00FF. Licence : SIL OFL 1.1 (voir OFL.txt)." % (ttf, size),
        "#ifndef %s_H" % name.upper(),
        "#define %s_H" % name.upper(),
        "",
        "#include <LovyanGFX.hpp>",
        "",
        "static const uint8_t %sBitmaps[] PROGMEM = {" % name,
    ]
    for i in range(0, len(bitmaps), 16):
        lines.append("    " + ", ".join("0x%02X" % b for b in bitmaps[i : i + 16]) + ",")
    lines += ["};", "", "static const lgfx::GFXglyph %sGlyphs[] PROGMEM = {" % name]
    for offset, width, height, advance, x_offset, y_offset in glyphs:
        lines.append("    {%d, %d, %d, %d, %d, %d}," % (offset, width, height, advance, x_offset, y_offset))
    lines += [
        "};",
        "",
        "static const lgfx::GFXfont %s = {(uint8_t *)%sBitmaps, (lgfx::GFXglyph *)%sGlyphs, 0x%02X, 0x%02X, %d};"
        % (name, name, name, FIRST, LAST, y_advance),
        "",
        "#endif",
        "",
    ]
    return "\n".join(lines)


def main(argv):
    if len(argv) != 5:
        sys.stderr.write(__doc__)
        return 2
    ttf, size, name, output = argv[1], int(argv[2]), argv[3], argv[4]
    bitmaps, glyphs, y_advance = generate(ttf, size, name)
    with open(output, "w", encoding="utf-8", newline="\n") as handle:
        handle.write(to_header(bitmaps, glyphs, y_advance, name, ttf.replace("\\", "/").split("/")[-1], size))
    print("%s : %d octets de bitmaps, %d glyphes, yAdvance %d" % (name, len(bitmaps), len(glyphs), y_advance))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
