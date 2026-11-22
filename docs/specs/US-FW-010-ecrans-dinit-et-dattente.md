# US-FW-010 — Écrans d'init et d'attente

- **Couche** : rendu
- **Statut** : Partielle (« Connecté » et « Partie en cours » en texte ; pas d'écran d'init avec logo)
- **Dépend du hub** : US-009

## Périmètre

Écran d'init avec le logo pendant l'attente du WiFi et du login, puis « Connecté » à `auth_success` et « Partie en cours » à `game_resumed`, avec « En attente d'une question ».

## Sources

CDC §8.2 (lignes init, `auth_success`, `game_resumed`), §9 ; maquettes `start-01` à `start-03` ; décisions #28, #30 ; `DEBTS.md` (« `setup()` bloquant pendant l'init » ; le logo n'est pas dans le dépôt).

## Critères d'acceptance

À rédiger (session `docs(specs)` dédiée).
