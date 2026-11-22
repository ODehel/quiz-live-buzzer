# US-FW-001 — Détecter un appui

- **Couche** : logique
- **Statut** : Implémentée, CA-1 à CA-8 rédigés depuis le réel le 2026-11-22 (bouton rouge non branché à l'ESP32-S3 : l'appui SPEED n'a jamais été constaté)
- **Dépend du hub** : —

## User story

> **En tant que** joueur,
> **quand** je touche une zone de l'écran ou j'appuie sur le bouton rouge,
> **alors** le buzzer reconnaît mon appui, sa famille (A–D ou buzz) et sa valeur (la lettre, ou rien pour le buzz), et le transmet une seule fois à son comportement.

## Périmètre

Une seule écoute d'appui, permanente, pour les zones tactiles A–D et le bouton rouge (CDC §3, décision #1). La détection (GPIO, GT911) vit dans `src/`, la décision d'agir dans `lib/`. Un appui porte une **famille**, qui dit quelle méthode de `BuzzerBehavior` est appelée, et une **valeur**, la lettre pour A–D et rien pour le buzz.

US-FW-001 s'arrête à l'appel de `OnMcqPressed` ou `OnBuzzPressed`.

**Hors périmètre** :

- l'appui d'une famille non armée dans le mode courant, y compris en mode Inerte, est ignoré par `BuzzerBehavior` : US-FW-002 et US-FW-003 ;
- les zones tactiles ne réagissent que lorsqu'elles sont affichées (décision #18) : US-FW-011 ;
- le message du hub non reconnu, invalide ou hors contexte, ignoré sans remontée d'erreur : US-FW-004.

## Critères d'acceptance

Tous les CA sont de la couche **logique** : aucun rendu dans cette US. « Preuve » dit ce qui établit le CA ; « Statut » dit où il en est dans le code au 2026-11-22.

| # | Étiquette | Critère | Résultat attendu | Preuve | Statut |
|---|---|---|---|---|---|
| CA-1 | logique | Le bouton rouge est lu sur sa broche (GPIO 38, pull-up) | `PhysicalButtonReader` déduit `Buzz` quand `GpioPinReader::Read()` retourne vrai (bouton appuyé), `Unknown` sinon. Le passage « niveau bas = appuyé » est fait par l'adaptateur `ArduinoGpioPinReader` | test natif `test_pin_reader` (2 tests) ; adaptateur : smoke test | Logique implémentée et testée ; adaptateur non testé, bouton non branché |
| CA-2 | logique | Un appui maintenu compte pour un seul appui | `ButtonPressDetector::Poll()` appelle le traducteur quand le bouton lu change et n'est pas `Unknown` (front montant), une seule fois tant qu'il reste le même ; le relâchement (`Unknown`) n'appelle rien ; un nouvel appui après relâchement appelle de nouveau | test natif `test_press_detector` (3 tests, source physique) | Implémenté et testé. Source tactile : même code, non testée avec un lecteur tactile |
| CA-3 | logique | Un point tactile est converti en bouton par les zones affichées | `TouchZoneReader` retourne le bouton de la première zone qui contient le point, bords compris (`left ≤ x ≤ right`, `top ≤ y ≤ bottom`) ; aucune zone ne le contient, ou pas de contact (`TouchPoint::NoContact`, soit (−1, −1)) : `Unknown`. Deux zones qui se chevaucheraient : la première de la liste gagne (cas inexistant, la grille 2×2 de `ZoneGrid` ne chevauche pas) | test natif `test_touch_zone_reader` (7 tests) et `test_input` (2 tests, via `TouchButtonReader`) | Implémenté et testé ; bords inclus et chevauchement non testés |
| CA-4 | logique | Les coordonnées du capteur GT911 sont projetées sur celles de l'écran | `TouchProjection` fait une proportion linéaire du capteur (800×480, origine (0, 0)) vers l'écran (0 à 799, 0 à 479), en entiers : l'origine donne (0, 0), le coin opposé (800, 480) donne (799, 479). Le sens des axes est fixé par `ROTATION_INVERTED` (identité dans `TAMC_GT911` v1.0.2) | test natif `test_touch_projection` (2 tests : origine et coin opposé) ; sens des axes : smoke test du 2026-11-08, les quatre coins donnent A, B, C, D | Implémenté, testé en partie : point intermédiaire et troncature entière non testés |
| CA-5 | logique | La famille décide de la méthode appelée, la valeur renseigne le message | `BuzzerButtonPressTranslator` : `A` à `D` appellent `OnMcqPressed('A')` à `OnMcqPressed('D')` ; `Buzz` appelle `OnBuzzPressed()` (pas de valeur) ; `Unknown` n'appelle rien. `A` à `D` ne viennent que du tactile (décision #1) | test natif `test_input` (6 tests) | Implémenté et testé |
| CA-6 | logique | Un doigt qui passe d'une zone à une autre sans perdre le contact | Chaque changement de zone vers un bouton connu est un nouvel appui : le glissement de A vers B appelle le traducteur pour A puis pour B. Passer par une position sans zone (`Unknown`) puis revenir sur la même zone en fait un deuxième appui (CA-2). Sous le lock-first de US-FW-002, seul le premier appui est pris : c'est la **première zone effleurée** qui part, pas forcément la zone visée (décision #44) | test natif à écrire : `ButtonPressDetector` avec un lecteur scripté A puis B (deux appels), puis A, `Unknown`, A | Implémenté, non testé |
| CA-7 | logique | Plusieurs doigts sur l'écran | Seul le premier point renvoyé par le GT911 (`points[0]`) est lu ; les autres sont ignorés. Aucun contact : `NoContact` (décision #44) | smoke test (adaptateur de `src/`, non couvert par un test natif) ; l'ordre des points renvoyés par la bibliothèque n'est pas vérifié | Implémenté, non testé, non constaté |
| CA-8 | logique | Les deux sources sont écoutées à chaque tour de `loop()` | `loop()` appelle `Poll()` sur deux `ButtonPressDetector` indépendants, l'un pour le bouton rouge (`PhysicalButtonReader`), l'autre pour le tactile (`TouchButtonReader`), vers le même `BuzzerButtonPressTranslator`. Chaque détecteur retient son propre dernier bouton ; aucune coordination entre eux : l'arbitrage d'appuis simultanés appartient à `BuzzerBehavior` (US-FW-002 et US-FW-003). Aucun contrat de latence : la cadence est celle de `loop()` | lecture de `src/main.cpp` ; smoke test | Implémenté, non testé |

## Constats reportés

Rien de ce qui suit n'est un CA : ce sont des constats consignés dans `DEBTS.md`, à rouvrir comme indiqué.

- **Anti-rebond du bouton rouge** : cité par le CDC §3, aucun code. À traiter après branchement du bouton et smoke test, seulement si des rebonds sont observés.
- **Doigt posé quand les zones apparaissent** : un contact présent à l'apparition des zones compte comme un appui, car `lastButton` repasse à `Unknown` quand elles disparaissent (décision #18 : « improbable, non vu »). Déjà en dette (« Buzzer armé sans zone tactile à l'écran »).
- **Bouton rouge non branché** : l'appui SPEED n'a jamais été constaté (dette « Appui SPEED jamais testé »).
- **Écarts de preuve** : tests natifs manquants (glissement, bords des zones, point intermédiaire de la projection) et ordre des points GT911 non vérifié (CA-3, CA-4, CA-6, CA-7).

## Sources

CDC §3 ; `lib/input/` (`ButtonPressDetector`, `PhysicalButtonReader`, `TouchButtonReader`, `TouchZoneReader`, `TouchProjection`, `BuzzerButtonPressTranslator`) ; `src/ArduinoGpioPinReader`, `src/Gt911TouchReader`, `src/main.cpp` ; `test/test_press_detector`, `test_pin_reader`, `test_input`, `test_touch_zone_reader`, `test_touch_projection` ; décisions #1, #18 et #44 ; `DEBTS.md`.
