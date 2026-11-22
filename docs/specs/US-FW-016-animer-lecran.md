# US-FW-016 — Animer l'écran

- **Couche** : rendu
- **Statut** : À cadrer
- **Dépend du hub** : US-011 (`timer_tick` toutes les 5 s)

## Périmètre

Effets qui demandent une horloge : zone mise en évidence qui clignote, bandeau clignotant, décompte local du chrono entre deux `timer_tick`. Aujourd'hui la mise en évidence est fixe et le chrono se resynchronise seulement à chaque tick.

## Sources

CDC §8.2 (vocabulaire des effets d'écran, chrono) ; `DEBTS.md` (« Zone en évidence sans clignotement », « Deux instances de `DisplayState` »).

## Critères d'acceptance

À rédiger (session `docs(specs)` dédiée).
