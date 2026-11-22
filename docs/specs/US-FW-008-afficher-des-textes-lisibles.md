# US-FW-008 — Afficher des textes lisibles

- **Couche** : rendu
- **Statut** : Implémentée
- **Dépend du hub** : —

## Périmètre

Tout texte affiché est lisible à distance : police de quiz couvrant les caractères accentués français, mesure en pixels, retour à la ligne, titres jusqu'à 250 caractères. Modèle d'écran et presenter en `lib/`, dessin en `src/` (vue passive).

## Sources

CDC §8.2 (vocabulaire) ; décisions #7 à #11, #38, #40 ; `lib/rendering/` ; `DEBTS.md` (signes typographiques absents, polices en 1 bit, garantie des 250 caractères, boîte d'encre, largeurs de test copiées).

## Critères d'acceptance

À rédiger (session `docs(specs)` dédiée).
