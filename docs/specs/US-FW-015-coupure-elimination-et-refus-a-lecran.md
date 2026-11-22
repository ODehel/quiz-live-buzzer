# US-FW-015 — Coupure, élimination et refus à l'écran

- **Couche** : rendu
- **Statut** : Partielle (refus `401` en texte seul)
- **Dépend du hub** : —

## Périmètre

Trois écrans nés d'événements réseau : « Connexion perdue » avec « Tentative N/3 » sous le bandeau du haut (question atténuée, zones et bouton rouge inactifs), « Hors jeu » en mode Éliminé (terminal), « Connexion refusée » au `401` du login.

## Sources

CDC §8.2 (lignes `(réseau : …)`), §5, §9 ; maquettes `fail-01` à `fail-03` ; décisions #29, #30, #42 ; `DEBTS.md` (« Écran de login refusé en texte seul » ; le compteur « Tentative N/3 » n'est exposé au rendu par aucun port).

## Critères d'acceptance

À rédiger (session `docs(specs)` dédiée).
