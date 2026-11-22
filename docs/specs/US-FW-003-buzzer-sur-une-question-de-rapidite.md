# US-FW-003 — Buzzer sur une question de rapidité

- **Couche** : logique
- **Statut** : Implémentée côté logique ; bouton rouge non branché, appui SPEED jamais testé sur le matériel
- **Dépend du hub** : US-012

## Périmètre

Mode SPEED armé par `question_open` ou `buzz_unlocked` : un appui sur le bouton rouge pose le verrou, déclenche le feedback local puis émet `buzz`. Le mode peut être ré-armé plusieurs fois sur une même question (buzz invalidé par le maître). Appuis hors famille et seconds appuis ignorés ; `timer_end` désarme.

## Sources

CDC §4.1, §4.3 (dont la note « SPEED réentrant »), §6, §8.1 ; `lib/game/` ; `DEBTS.md` (« Appui SPEED jamais testé »).

## Critères d'acceptance

À rédiger (session `docs(specs)` dédiée).
