# US-FW-004 — Dialoguer avec le hub

- **Couche** : logique
- **Statut** : Implémentée
- **Dépend du hub** : US-009

## Périmètre

Messages WebSocket JSON échangés avec le hub : lecture du type des messages reçus, aiguillage vers le comportement ou le rendu, sérialisation des messages émis (`auth`, `auth_refresh`, `answer`, `buzz`). Un message non reconnu, invalide ou hors contexte est ignoré silencieusement.

## Sources

CDC §3 (règles transverses), §8.1 ; `lib/dispatch/`, `lib/serialization/`, `lib/session/` ; décision #6.

## Critères d'acceptance

À rédiger (session `docs(specs)` dédiée).
