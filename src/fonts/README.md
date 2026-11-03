# Polices de l'écran

Trois tailles d'Oswald Regular (corps 30, 38 et 48 px) au format `lgfx::GFXfont`, 1 bit par pixel, U+0020 à U+00FF (Latin-1). Aucune n'est encore branchée : `GfxScreen` mesure toujours à chasse fixe (voir `DEBTS.md`).

| Fichier | Corps | Cap-height | Bitmaps | `yAdvance` |
|---|---|---|---|---|
| `OswaldRegular30.h` | 30 px | 24 px | 6 061 o | 44 px |
| `OswaldRegular38.h` | 38 px | 31 px | 9 368 o | 56 px |
| `OswaldRegular48.h` | 48 px | 39 px | 15 070 o | 71 px |

Les trois tables de glyphes pèsent 2 688 octets chacune (224 glyphes de 12 octets).

## Licence

Oswald est publiée sous SIL Open Font License 1.1 (`OFL.txt`, copie de la licence du dépôt source). Les en-têtes générés sont des œuvres dérivées : la licence et la mention de copyright restent avec eux.

## Source

- Dépôt : `https://github.com/googlefonts/OswaldFont`, commit `89795261ac9eeb9aa8cd99f43982c4e4b0e53261`.
- Fichier : `fonts/ttf/Oswald-Regular.ttf`, SHA-256 `0d9980da817ee71ad9c34bb46950fcf15dd53e928031f39d2e11ae19178ab826`.

## Régénérer

```bash
pip install freetype-py
git clone https://github.com/googlefonts/OswaldFont
git -C OswaldFont checkout 89795261ac9eeb9aa8cd99f43982c4e4b0e53261
for s in 30 38 48; do
  python3 scripts/generate-gfx-font.py OswaldFont/fonts/ttf/Oswald-Regular.ttf $s OswaldRegular$s src/fonts/OswaldRegular$s.h
done
git diff --stat src/fonts   # vide si la version de FreeType rend les mêmes pixels
```

La sortie est déterministe pour une version de FreeType donnée ; les en-têtes actuels ont été générés avec `freetype-py` (FreeType 2.13.2).

## Ce que le script fait, et ne fait pas

- Rendu monochrome (`FT_LOAD_TARGET_MONO`), pas de lissage : à ces tailles, les bords sont crénelés.
- Un code absent de la police (les contrôles 0x7F à 0x9F) reçoit le glyphe de `?`, comme le suppose `ReadFontMetrics` (`lib/rendering/GfxFontMetrics.h`).
- `yAdvance` est la hauteur de ligne native d'Oswald (environ 1,48 em) : l'interligne se règle à l'affichage, pas ici.
- Hors Latin-1 : `œ`, `Œ`, `’`, `…` et `€` n'ont pas de glyphe (voir `DEBTS.md`).
