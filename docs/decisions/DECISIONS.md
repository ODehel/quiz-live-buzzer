# Décisions de conception — quiz-live-buzzer

Journal append-only des décisions de conception tranchées en session, avec le fait qui les a forcées. Une décision revue donne une **nouvelle** entrée qui référence l'ancienne, jamais une réécriture. Seules les décisions de *conception* y figurent (contrat, emplacement, responsabilité) ; les leçons de méthode restent dans les récaps.

Format : **Contexte** (le fait qui a forcé le choix) · **Décision** · **Conséquences** (ce que ça engage, dettes créées).

L'en-tête de chaque décision porte le **titre du commit** qui l'a livrée, pas son hash : le titre est connu avant le commit, et `git log --grep` le retrouve.

Les décisions antérieures à ce journal sont consignées dans `docs/CDC-FIRMWARE-BUZZER.md` (machine à états §6, protocole §8.1, rendu §8.2, démarrage §9) ; elles n'y sont pas recopiées.

---

*Aucune entrée pour l'instant. La première décision de conception de ce dépôt ouvrira le journal sous le numéro #1.*
