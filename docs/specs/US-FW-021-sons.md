# US-FW-021 — Sons

- **Couche** : vide
- **Statut** : Vide
- **Dépend du hub** : US-018

## Périmètre

Restitution sonore : `play_sound_url` (téléchargement HTTP GET d'un fichier distant), `play_system_sound` (son préchargé selon `sound_id`), et éventuellement un son local lors d'un appui armé.

## Bloqué par

Le haut-parleur reste un point ouvert du CDC ; le son du feedback local n'est pas tranché ; aucun code de son dans le firmware.

## Sources

CDC §2, §8.1 (messages `play_*`), §8.2 (colonne Son), §10 point 2 ; `DEBTS.md` (« Photo et son avant la question maquettés sans message ni report »).
