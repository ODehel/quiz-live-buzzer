# Avatars des réactions

Dix réactions d'un même personnage, utilisées par les pages `reaction-*.html`. Les SVG ont un fond transparent : le fond (or, rose ou lavande) vient de la page, selon la situation.

## Origine et licence

Générés avec [DiceBear](https://www.dicebear.com/) (`@dicebear/core` et `@dicebear/collection`), style **avataaars** de Pablo Stanley ([avataaars.com](https://avataaars.com/)), licence « libre pour usage personnel et commercial ». À relire avant toute autre utilisation : ce n'est pas du CC0.

## Traits du personnage (d'après les photos d'Olivier)

| Paramètre | Valeur |
|---|---|
| Cheveux | `theCaesar`, `2c1b18` |
| Barbe | `beardLight`, `4a312c` |
| Lunettes | `prescription02`, `3c4f5c` |
| Peau | `edb98a` |
| Tenue | `blazerAndShirt`, `25557c` |

## Les dix réactions

| Fichier | Yeux | Sourcils | Bouche | Situation |
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

Les options ci-dessus, avec `seed: "olivier"`, `backgroundColor: ["transparent"]` et `size: 280`, redonnent les SVG (hors larmes de joie de `hilare.svg`, quatre gouttes bleues ajoutées avant `</svg>`, dans un groupe `tears-of-joy`).
