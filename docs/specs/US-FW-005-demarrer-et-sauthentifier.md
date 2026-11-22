# US-FW-005 — Démarrer et s'authentifier

- **Couche** : logique
- **Statut** : Partielle (init bloquant, un seul type d'erreur arrête le login)
- **Dépend du hub** : US-003, US-009

## Périmètre

Séquence d'initialisation linéaire : mise sous tension, WiFi, `POST /api/v1/token`, ouverture du WebSocket, `auth`, réception de `auth_success` qui met fin à l'init et fait entrer en mode Inerte. Un `401` au login arrête le buzzer jusqu'au redémarrage ; tout autre échec est réessayé. Pendant l'init, un appui est sans effet. Le compteur d'élimination est inactif pendant toute cette phase.

## Sources

CDC §9 ; `lib/startup/`, `lib/provisioning/`, `src/main.cpp`, `src/HttpTokenFetcher.*` ; décision #42 ; `DEBTS.md` (« `setup()` bloquant pendant l'init », « Seul `401` arrête le login », « `HttpTokenFetcher` non testé », adresse du hub en dur). L'écran associé relève d'US-FW-010 et US-FW-015.

## Critères d'acceptance

À rédiger (session `docs(specs)` dédiée).
