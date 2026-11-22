# US-FW-013 — Résultat d'une question

- **Couche** : rendu
- **Statut** : Partielle
- **Dépend du hub** : US-011, US-012

## Périmètre

À `question_result` : MCQ (zone de la bonne réponse mise en évidence, « Bonne réponse » et « Votre réponse », réponse fausse cerclée, cas sans réponse) et SPEED (« Validé » avec gain, « Raté », « Question terminée », réponse attendue). La distinction MCQ/SPEED se déduit du contenu du message. Le firmware retient « ce buzzer a buzzé » pendant la question.

## Sources

CDC §8.2 (lignes `question_result`, « Ce buzzer a buzzé ») ; maquettes `qcm-06` à `qcm-08` et `speed-06` à `speed-08` ; décisions #13, #15, #24, #26 ; `DEBTS.md` (« Rendu cible des maquettes partiellement reporté »).

## Critères d'acceptance

À rédiger (session `docs(specs)` dédiée).
