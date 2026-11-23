# US-FW-009 — Bandeau du haut

- **Couche** : rendu
- **Statut** : Partielle, CA-1 à CA-8 rédigés depuis le réel le 2026-11-22 (CA-1 tenu par des tests natifs ; CA-6 implémenté sans test natif ; CA-2, CA-3, CA-4, CA-5, CA-7 et CA-8 non implémentés : le bandeau n'existe que sur l'écran « Connecté », sans état, sans nom de participant, sans liaison ni troncature). CA-1, CA-3 et CA-4 corrigés le 2026-11-22 par US-FW-010 (décision #53) : le bandeau est présent dès la mise sous tension
- **Dépend du hub** : US-009 (`auth_success`, `username`), US-010 (assignation buzzer → participant, CA-65), US-019 (`game_resumed`) ; le message qui porterait le nom du participant n'existe pas (CA-2). Lus à `ed6bd31`.

## User story

> **En tant que** joueur,
> **quand** mon buzzer est allumé,
> **alors** je lis en haut de chaque écran qui je suis, le titre « Buzz’O’Live » et si la liaison avec le hub tient.

## Périmètre

Un bandeau commun en haut de la dalle (800 × 480 px), sur une ligne, en petite police : le nom à gauche, « Buzz’O’Live » au centre, l'état de la liaison avec le hub à droite, un trait dessous. Il apparaît dès la mise sous tension, sur l'écran d'init (US-FW-010 CA-1) : nom vide et liaison « coupée » tant qu'aucun `auth_success` n'est reçu. Il reste ensuite sur chaque écran jusqu'au redémarrage. Cela vaut quelle que soit l'US qui affiche l'écran en dessous.

**Hors périmètre** (constats reportés ci-dessous, sauf mention) :

- **points** et **classement** (« 120 pts », « 2e/6 ») et **charge de la batterie** : sans source dans le protocole ou sans mesure ;
- le contenu de l'écran d'init sous le bandeau : US-FW-010 ; les variantes du refus `401` (bandeau sans nom ni points) et des coupures (bandeau rouge sous le bandeau du haut) : US-FW-015 ;
- l'en-tête sous le bandeau (texte LCD, chrono, capsules) : US-FW-011 (CA-3, CA-7, CA-8, CA-12 ; capsules reportées) et US-FW-012 ;
- les couleurs, la hauteur de la maquette, le logo animé de « Buzz’O’Live » : constat reporté ; les effets qui demandent une horloge : US-FW-016 ;
- le dessin d'un texte dans une zone : US-FW-008 (le nom y est mis en page comme tout texte).

## Critères d'acceptance

« Étiquette » dit où vit le comportement : **logique** (`lib/`, testable en natif) ou **adaptateur** (`src/`, validé au smoke test). « Preuve » dit ce qui établit le CA ; « Statut » dit où il en est dans le code au 2026-11-22. Les tests ne portent pas de numéro de CA : la correspondance se fait par le nom des tests. Aucun test ni smoke test n'est écrit dans cette session.

| # | Étiquette | Critère | Résultat attendu | Preuve | Statut |
|---|---|---|---|---|---|
| CA-1 | logique | Le buzzer reçoit `auth_success` | Le bandeau, visible depuis la mise sous tension (CA-3), prend comme nom l'`username` du compte du buzzer (« quiz_buzzer_01 ») : c'est le **repli** tant qu'aucun nom de participant n'est reçu (CA-2). Champ `username` absent ou non textuel : bandeau visible, nom vide | `test_screen_presenter` : `ScreenPresenterTopBarTest.AuthSuccessShowsTheUsernameInTheTopBar`, `…AuthSuccessWithoutUsernameShowsAnEmptyName` ; bout en bout `test_connection` : `ConnectionEventHandlerRenderingTest.AuthSuccessIsRenderedWithTheUsernameOfTheHubInTheTopBar` | Implémenté, testé |
| CA-2 | logique | Le buzzer reçoit le nom du participant auquel le hub l'a assigné (« Camille ») | Ce nom remplace le repli à gauche du bandeau, sur chaque écran, jusqu'au redémarrage ; un nom reçu à nouveau (reprise de partie) le remplace. **Le hub n'a aucun message qui le porte** : l'assignation se fait au passage `PENDING → OPEN` (US-010 CA-65), après `auth_success`, et `game_resumed` ne porte que `question_index`, `cumulative_score` et `status` (US-019 CA-18) | à écrire : `test_screen_presenter` et `test_connection` (nom reçu, puis nom remplacé) ; dépend de la définition du message côté hub | **Non implémenté**, bloqué par le hub (décision #52) |
| CA-3 | logique | Un écran s'affiche ou change alors que le bandeau est visible | Le bandeau reste visible avec le même nom pour tout écran qui change (question, zones, évidence, bandeau de texte, zone choisie, feedback local d'un appui MCQ ou SPEED) ; un message qui laisse l'écran inchangé ne le touche pas. Dès la mise sous tension, l'écran d'init (US-FW-010 CA-1) porte le bandeau avec un nom vide ; le nom devient l'`username` au premier `auth_success` (CA-1) | à écrire : `test_screen_presenter`, `test_lcd_renderer`, `test_local_feedback` ; **les tests actuels figent le contraire** : `MessageWithoutTopBarTest.HidesTheTopBar` et chaque attente `Display(TopBar::Hidden(), …)` de `test_lcd_renderer`, `test_local_feedback` et `test_connection` | **Non implémenté** : `TopBarFor` sans état rend `Hidden()` pour tout autre message que `auth_success` ; `ScreenLocalFeedback` passe toujours `Hidden()` (dette « Deux instances de `DisplayState` ») |
| CA-4 | logique | La liaison avec le hub change d'état | Elle est « coupée » dès la mise sous tension (aucune connexion encore établie) et dès que la connexion établie est perdue, « liée » à l'`auth_success` suivant ; le rendu en est informé et le bandeau est redessiné **sans message du hub** (une perte n'est pas un message). Une perte comptée par `ReconnectionPolicy` n'est pas ce signal : voir constat « Liaison et compteur » | à écrire : `test_connection` (perte puis `auth_success`) et un port d'état de liaison vers le rendu | **Non implémenté** : aucun port ne l'expose au rendu |
| CA-5 | logique | Le nom est plus large que sa place | Il est tronqué par « … » pour ne pas toucher « Buzz’O’Live » : il tient dans la colonne de gauche, de largeur (écran − titre) ÷ 2 moins la marge de 10 px, mesuré avec les largeurs réelles de la police (US-FW-008 CA-1). Un nom qui tient est affiché entier | à écrire : `test_text_layout` ou test dédié, largeurs réelles d'Oswald 30 px | **Non implémenté** : aucune troncature (dette) ; un nom de plus d'une vingtaine de caractères (non mesuré) chevauche le titre |
| CA-6 | adaptateur | Le bandeau est dessiné | `GfxScreen::DrawTopBar` : nom à gauche (marge de 10 px), « Buzz’O’Live » centré, un trait horizontal dessous, en petite police (30 px), sur une ligne, blanc sur noir, 60 px de haut trait compris ; le texte plein écran se centre dans le reste (780 × 400 px, US-FW-008) | `src/GfxScreen.h` (lecture) ; smoke test du 2026-11-09 (faux hub), relevé d'Olivier, non rejoué | Implémenté, **non testé** (smoke test seulement) |
| CA-7 | adaptateur | Un écran autre que le texte plein écran est dessiné alors que le bandeau est visible | Le bandeau est dessiné au-dessus de chaque modèle d'écran (zones, évidence, bandeau de texte, zone choisie) et le contenu occupe la dalle moins 60 px | à écrire : smoke test (dessin) ; la hauteur restante change les zones de US-FW-008 CA-9 : voir constats | **Non implémenté** : `GfxScreen::Display` ne dessine le bandeau qu'avec un `TextScreen`, les autres modèles l'ignorent |
| CA-8 | adaptateur | La liaison est dessinée | À droite du bandeau, une icône de liaison : blanche si « liée », barrée et rouge si « coupée » (CA-4) | maquettes `qcm-02-choices.html` (liée), `qcm-02d-choices-hub-lost.html` (coupée) ; à écrire : smoke test avec un hub arrêté | **Non implémenté** |

## Constats reportés

Rien de ce qui suit n'est un CA : ce sont des constats consignés dans `DEBTS.md`, à rouvrir comme indiqué.

- **Points et classement** : le hub envoie `cumulative_score` dans `question_result` (US-011 CA-25, US-012 CA-31) et `game_resumed` (US-019 CA-18), que le firmware ignore ; aucun message ne donne le rang ni le nombre de joueurs (`intermediate_ranking`, US-014, diffuse tout le classement par nom de participant, sans lien avec le buzzer). Sa valeur avant le premier résultat n'est pas définie. Le bandeau n'affiche ni points ni rang. À rouvrir avec le hub : US-FW-017 (décision #60) n'y apporte ni points ni rang, la ligne du joueur n'y étant pas reconnue (son constat 1).
- **Batterie** : aucune mesure dans le code ; matériel non vérifié sur la CrowPanel 5″ (un buzzer sans batterie n'aurait rien à afficher). À rouvrir après un smoke test matériel ; le CDC §8.2 et la maquette demandent la charge, rouge à 20 % ou moins.
- **Nom du participant** : message à définir côté hub (CA-2). Aucune session firmware ne peut le créer.
- **Hauteur et habillage de la maquette** : 60 px en blanc sur noir dans le code, 5,5 u soit 44 px à 800 px en violet foncé dans la maquette (décision #19), sans jetons de design. À trancher avec le rendu cible (dette « Rendu cible des maquettes partiellement reporté »).
- **Logo de la maquette** : « Buzz’ » vibrant en écho, le « O » en disque rouge, « ’Live » en or ; le code écrit « Buzz’O’Live » en texte. Animation : non définie, constat reporté de US-FW-016.
- **Liaison et compteur** : la perte de liaison de CA-4 s'appuie sur l'événement de fermeture de la bibliothèque `WebSockets`, qui ne le produit qu'à la fermeture d'une connexion ouverte ; un hub arrêté après la perte ne produit aucun nouvel événement de fermeture, mais un signal « tentative échouée » distinct depuis US-FW-006 CA-7 (`ConnectionAttemptFailed`, 2026-11-23), que le bandeau pourra lire avec le port d'état de liaison. Non vérifié sur le matériel.
- **Zones sous le bandeau** (mesuré par la sonde du 2026-11-22, largeurs réelles d'Oswald 30 px) : avec 60 px de moins, une zone de la grille passe de 380 × 220 à 380 × 190 px (CA-7) ; les 5 lignes (200 px) que demandent 40 `¼` (US-FW-008 CA-9) débordent de 10 px, 40 `W` (160 px) tiennent. Sous le bandeau **et** l'en-tête de US-FW-011 (CA-4), la zone utile est de 380 × 160 px : 40 `¼` débordent de 40 px, 40 `W` tiennent pile. Une zone sous le bandeau de résultat passe de 380 × 160 à 380 × 130 px (calcul, non sondé : US-FW-013).
- **Variantes** : le bandeau de l'init est tranché par US-FW-010 CA-1 (titre, nom vide, liaison « coupée », sans batterie) ; celui du refus `401` est tranché par US-FW-015 CA-9 et CA-10 (bandeau de l'init : nom vide, liaison coupée), celui de « Hors jeu » garde le dernier nom, liaison coupée (CA-8) ; la perte de liaison de CA-4 est le signal de la coupure (US-FW-015 CA-1).

## Sources

CDC §8.2 (habillage commun, init, coupures) ; décisions #19 (maquettes), #38, #40, #41 (bandeau du nom), #52, #53 (bandeau dès l'init) ; maquettes `docs/mockups/` (`qcm-02-choices.html`, `qcm-02c-choices-low-battery.html`, `qcm-02d-choices-hub-lost.html`, `mockup.css`) ; `lib/rendering/` (`TopBar`, `Screen`, `ScreenPresenter`, `LcdRenderer`, `ScreenLocalFeedback`, `TappableZonesScreen`), `lib/startup/LoginOutcomeDisplay.h`, `lib/connection/ConnectionEventHandler.h`, `src/GfxScreen.h` ; `test/test_screen_presenter`, `test_lcd_renderer`, `test_local_feedback`, `test_connection`, `test_tappable_zones`, `test_startup` ; hub `quiz-live-server` à `ed6bd31` : US-009 (CA-6), US-010 (CA-65 à CA-67), US-011 (CA-22, CA-25), US-012 (CA-31), US-014, US-019 (CA-17 à CA-19) ; `DEBTS.md`.
