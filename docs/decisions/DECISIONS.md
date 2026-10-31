# Décisions de conception — quiz-live-buzzer

Journal append-only des décisions de conception tranchées en session, avec le fait qui les a forcées. Une décision revue donne une **nouvelle** entrée qui référence l'ancienne, jamais une réécriture. Seules les décisions de *conception* y figurent (contrat, emplacement, responsabilité) ; les leçons de méthode restent dans les récaps.

Format : **Contexte** (le fait qui a forcé le choix) · **Décision** · **Conséquences** (ce que ça engage, dettes créées).

L'en-tête de chaque décision porte le **titre du commit** qui l'a livrée, pas son hash : le titre est connu avant le commit, et `git log --grep` le retrouve.

Les décisions antérieures à ce journal sont consignées dans `docs/CDC-FIRMWARE-BUZZER.md` (machine à états §6, protocole §8.1, rendu §8.2, démarrage §9) ; elles n'y sont pas recopiées.

---

## #1 — docs(decisions): drop the physical A-D buttons

**Contexte** — Le matériel est une CrowPanel 5″ (DIS07050H). D'après le wiki d'Elecrow, un seul GPIO du connecteur est libre sans réserve, `GPIO_D` (38) ; les autres broches exposées servent l'I²C du tactile (19, 20), le haut-parleur I²S (17, 18, 42), l'UART de la console (43, 44) ou la carte SD (10 à 13). Les broches 6, 7, 8 et 9 de `ArduinoLocalFeedback` sont sur le bus RGB de la dalle. Olivier a décidé que le buzzer ne compterait plus qu'un bouton rouge pour les questions de rapidité (SPEED), tout le reste passant par l'écran, hors haut-parleurs. Ce bouton n'a pas de LED intégrée.

**Décision** — Le buzzer a **un seul bouton physique**, le bouton rouge (`ButtonInput::Buzz`, `GPIO_D` 38). Les réponses A, B, C et D du mode MCQ sont des **zones tactiles** de l'écran. Il n'y a ni boutons-poussoir A–D ni LED. Le feedback local immédiat d'un appui armé passe par **l'écran** et, éventuellement, le **haut-parleur** intégré.

**Conséquences** —
- Le CDC est aligné sur cette décision (§1, §2, §3, §4, §6). La colonne LED du §8.2 est signalée caduque ; sa réécriture et celle du rendu d'init (§9) restent à faire.
- Le code des boutons physiques A–D (`PhysicalButton` A à D et leur traduction) et `ArduinoLocalFeedback` (quatre broches) sont à retirer, avec leurs tests, dans un `refactor` dédié.
- La dette « pins 6/7/8/9 du retour local » disparaît avec ce retrait ; la dette « zones tactiles figées en 2×2 » devient centrale puisque le tactile porte désormais toutes les réponses MCQ.

