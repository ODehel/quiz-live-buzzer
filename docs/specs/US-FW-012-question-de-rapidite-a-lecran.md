# US-FW-012 — Question de rapidité à l'écran

- **Couche** : rendu
- **Statut** : Partielle
- **Dépend du hub** : US-012

## Périmètre

Question avec bouton rouge dessiné, rendu local « Buzz envoyé », bandeau « À toi ! » à `buzz_accepted`, « X a buzzé » à `buzz_locked`, écran d'élimination à `buzz_invalidated`, ré-ouverture à `buzz_unlocked`. Le joueur répond à l'oral, sans chrono de réponse.

## Sources

CDC §8.2 (lignes `question_open`, `buzz_accepted`, `buzz_locked`, `buzz_invalidated`, `buzz_unlocked`, appui local SPEED) ; maquettes `speed-01` à `speed-05` ; décisions #22, #23, #26 ; `DEBTS.md` (« Titre de `question_open` remplacé par le premier `timer_tick` », « Rendu local SPEED sans effet d'écran », messages jamais émis par le hub).

## Critères d'acceptance

À rédiger (session `docs(specs)` dédiée).
