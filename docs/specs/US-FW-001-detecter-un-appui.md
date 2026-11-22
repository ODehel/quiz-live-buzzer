# US-FW-001 — Détecter un appui

- **Couche** : logique
- **Statut** : Implémentée (bouton rouge non branché à l'ESP32-S3 : l'appui SPEED n'a jamais été constaté)
- **Dépend du hub** : —

## Périmètre

Une seule écoute d'appui, permanente, pour les zones tactiles A–D et le bouton rouge : la détection (GPIO, GT911, front montant) vit dans `src/`, la décision d'agir dans `lib/`. Un appui porte une famille (A–D ou buzz) et une valeur (la lettre, ou rien pour le buzz).

## Sources

CDC §3 ; `lib/input/` ; décisions #1 et #18 ; `DEBTS.md` (« Appui SPEED jamais testé », erreurs GPIO du GT911).

## Critères d'acceptance

À rédiger (session `docs(specs)` dédiée).
