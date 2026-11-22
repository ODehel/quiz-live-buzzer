# US-FW-020 — Photo ou son avant une question

- **Couche** : vide
- **Statut** : Vide
- **Dépend du hub** : aucun message défini

## Périmètre

Écran de photo avant une question (grande carte, capsules « Question N » et thème) et écran d'écoute d'un son (« Écoute bien ! », haut-parleur et barres d'égaliseur).

## Bloqué par

Aucun message du hub ne les déclenche (`play_sound_url` laisse l'écran inchangé) ; non spécifié : ce qui met fin à l'écran, la photo pendant la question, la source et le format de l'image ; chargement et échec du fichier son non maquettés.

## Sources

Maquettes `pre-01-photo`, `pre-02-sound` ; décision #34 ; `DEBTS.md` (« Photo et son avant la question maquettés sans message ni report »).
