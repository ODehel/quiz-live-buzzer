# US-FW-011 — Question à choix multiples à l'écran

- **Couche** : rendu
- **Statut** : Partielle (rendu cible des maquettes non appliqué)
- **Dépend du hub** : US-011

## Périmètre

Titre de la question, quatre zones tactiles affichées simultanément (l'affichage progressif des réponses est abandonné), chrono resynchronisé à chaque `timer_tick`, rendu local d'un appui, accusé `answer_received` qui conserve les zones, `timer_end` (« Trop tard »).

## Sources

CDC §7 (écart avec le CSV), §8.2 (lignes `question_title`, `question_choices`, `timer_tick`, `timer_end`, `answer_received`, appui local MCQ) ; maquettes `qcm-01` à `qcm-05` ; décisions #12, #14, #16, #17, #19, #25 ; `DEBTS.md` (« Table de rendu §8.2 incomplète », « Buzzer armé sans zone tactile à l'écran », « Textes des zones retenus par l'adaptateur »).

## Critères d'acceptance

À rédiger (session `docs(specs)` dédiée).
