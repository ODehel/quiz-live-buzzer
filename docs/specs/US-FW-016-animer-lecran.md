# US-FW-016 — Animer l'écran

- **Couche** : rendu
- **Statut** : À cadrer
- **Dépend du hub** : —

## Périmètre

Effets qui demandent une horloge : zone mise en évidence qui clignote, bandeau clignotant. Aujourd'hui la mise en évidence est fixe. Le décompte local du chrono entre deux `timer_tick` n'est plus d'ici : il est spécifié par US-FW-011 (CA-7, CA-15, décision #54) ; il partage avec ces effets l'horloge et le rafraîchissement hors message, à définir une seule fois.

## Sources

CDC §8.2 (vocabulaire des effets d'écran) ; US-FW-011 (CA-7, CA-15) ; `DEBTS.md` (« Zone en évidence sans clignotement », « Deux instances de `DisplayState` », « Chrono : ni horloge ni rafraîchissement hors message »).

## Critères d'acceptance

À rédiger (session `docs(specs)` dédiée).
