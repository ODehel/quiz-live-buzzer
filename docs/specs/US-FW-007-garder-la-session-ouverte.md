# US-FW-007 — Garder la session ouverte

- **Couche** : logique
- **Statut** : Partielle (chemin mort : le hub n'émet pas encore `token_expiring_soon`)
- **Dépend du hub** : US-021

## Périmètre

Renouvellement du jeton : à `token_expiring_soon`, le buzzer émet `auth_refresh` avec un jeton valide ; `token_expired` est suivi de la fermeture de la connexion par le hub. Le jeton est aussi redemandé à chaque connexion, reconnexions comprises.

## Sources

CDC §8.1 (messages `token_*`, `auth_refresh`) ; `lib/session/` ; `DEBTS.md` (« `auth_refresh` sur `token_expiring_soon` : chemin mort », « Un `POST /api/v1/token` synchrone à chaque connexion »).

## Critères d'acceptance

À rédiger (session `docs(specs)` dédiée).
