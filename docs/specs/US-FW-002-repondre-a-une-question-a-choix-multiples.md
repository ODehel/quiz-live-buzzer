# US-FW-002 — Répondre à une question à choix multiples

- **Couche** : logique
- **Statut** : Implémentée
- **Dépend du hub** : US-011

## Périmètre

Mode MCQ armé par `question_choices` : un appui sur une zone A–D pose le verrou immédiatement (lock-first), déclenche le feedback local puis émet `answer { value }`. Un appui hors famille (bouton rouge) ou un second appui est ignoré ; `timer_end` désarme. En mode Inerte, tout appui est écarté.

## Sources

CDC §4.1, §4.2, §6, §8.1 ; `lib/game/` ; `DEBTS.md` (« Buzzer armé sans zone tactile à l'écran »).

## Critères d'acceptance

À rédiger (session `docs(specs)` dédiée).
