# Avatars des réactions

Deux personnages, dix réactions chacun, utilisés par les pages `reaction-*.html` (masculin) et `reaction-*-f.html` (féminin). Les SVG ont un fond transparent : le fond (or, rose ou lavande) vient de la page, selon la situation.

## Origine et licence

Générés avec [DiceBear](https://www.dicebear.com/) (`@dicebear/core` et `@dicebear/collection`), style **avataaars** de Pablo Stanley ([avataaars.com](https://avataaars.com/)), licence « libre pour usage personnel et commercial ». À relire avant toute autre utilisation : ce n'est pas du CC0.

## Personnage masculin (d'après les photos d'Olivier)

| Paramètre | Valeur |
|---|---|
| Cheveux | `theCaesar`, `2c1b18` |
| Barbe | `beardLight`, `4a312c` |
| Lunettes | `prescription02`, `3c4f5c` |
| Peau | `edb98a` |
| Tenue | `blazerAndShirt`, `25557c` |

## Personnage féminin (d'après des photos fournies par Olivier)

| Paramètre | Valeur |
|---|---|
| Cheveux | `fro`, `cf7a2e` |
| Barbe | aucune |
| Lunettes | `prescription02`, `2e1b12` |
| Peau | `6b4226` |
| Tenue | `shirtScoopNeck`, `f2efe6` |

Non reproduits, faute d'option dans avataaars : le collier, le motif rayé du haut, les mèches cuivrées (une seule teinte de cheveux).

## Les dix réactions (identiques pour les deux personnages)

| Fichier (féminin : suffixe `-f`) | Yeux | Sourcils | Bouche | Situation |
|---|---|---|---|---|
| `ravi.svg` | `happy` | `raisedExcitedNatural` | `smile` | bonne réponse |
| `hilare.svg` | `happy` | `raisedExcitedNatural` | `smile` + larmes de joie (surcouche dessinée à la main) | bonne réponse |
| `amoureux.svg` | `hearts` | `raisedExcitedNatural` | `smile` | bonne réponse |
| `malin.svg` | `wink` | `upDownNatural` | `twinkle` | bonne réponse |
| `taquin.svg` | `winkWacky` | `upDownNatural` | `tongue` | bonne réponse |
| `decu.svg` | `cry` | `sadConcernedNatural` | `sad` | mauvaise réponse |
| `en-colere.svg` | `squint` | `angryNatural` | `grimace` | mauvaise réponse |
| `depite.svg` | `eyeRoll` | `flatNatural` | `disbelief` | mauvaise réponse |
| `sonne.svg` | `xDizzy` | `defaultNatural` | `concerned` | mauvaise réponse |
| `endormi.svg` | `closed` | `defaultNatural` | `serious` | sans réponse |

## Régénérer

Les options ci-dessus, avec `backgroundColor: ["transparent"]` et `size: 280`, redonnent les SVG. Le personnage masculin utilise `seed: "olivier"` ; pour le féminin la graine est sans effet (toutes les options sont fixées, `facialHairProbability: 0`). Larmes de joie de `hilare.svg` et `hilare-f.svg` : quatre gouttes bleues ajoutées avant `</svg>`, dans un groupe `tears-of-joy` (mêmes coordonnées pour les deux personnages).
