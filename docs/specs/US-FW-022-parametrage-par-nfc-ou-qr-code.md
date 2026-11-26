# US-FW-022 — Paramétrage par NFC ou QR code

- **Couche** : rendu
- **Statut** : Partielle, CA-1 à CA-6 rédigés le 2026-11-22 (aucun CA tenu ; CA-4 implémenté en partie : l'Inerte ignore l'appui, testé, et l'écran D6 n'existe pas ; CA-1 à CA-3, CA-5 et CA-6 non implémentés : aucun modèle d'écran de configuration, aucun QR, aucune méthode de `ScreenPresenter`). Rédigée en **cadrage seul** : l'US était « Vide, bloquée » (canal d'appairage, déclencheur et contenu du QR non décidés, application mobile inexistante) et le dernier handoff (`2026-11-22-1929`) proposait une session de code (`fix(game): arm only from the inert mode`), non cette US. Seul l'**écran D6** (maquette `start-06-config`, décision #36) est décidable : il est rédigé comme **code dormant**, aucun déclencheur ne l'affiche. Canal, NFC, déclencheur, sortie, stockage, contenu du QR et habillage : constats reportés.
- **Dépend du hub** : aucun message (écran local, avant toute connexion : décision #36). Lus à `ed6bd31` (clone anonyme, profondeur 1) : US-002 (CA-1, CA-2, CA-4 : dix comptes `quiz_buzzer_01` à `_10`, mots de passe de 12 à 72 caractères) et `docs/shared/VISION.md` (« App mobile : Android / NFC, configuration WiFi des buzzers »). `src/` du hub : **aucune occurrence** de `nfc`, `qr`, appairage ou `provision` ; aucun point d'accès ni message de configuration.

## User story

> **En tant que** maître du jeu,
> **quand** le buzzer affiche son écran de configuration,
> **alors** je vois « Configuration » et un QR code que mon téléphone peut lire, sans aucune consigne à l'écran.

## Périmètre

L'écran de configuration **D6**, rendu à partir d'une **chaîne de QR fournie** (d'où elle vient : constat 4) :

1. la **demande** d'affichage de D6 et son remplacement de l'écran courant, sous le bandeau d'init (CA-1) ;
2. le repli quand la chaîne est inexploitable : D6 **sans carte** (CA-2) ;
3. les **zones tactiles masquées** et l'**absence d'effet** d'un appui (CA-3, CA-4) ;
4. le **dessin** : titre, carte blanche, QR (CA-5, CA-6).

D6 est un écran **local**, sans message du hub, statique, sans consigne (règle de la décision #33).

**Hors périmètre** (constats reportés ci-dessous, sauf mention) :

- le **canal d'appairage** (point d'accès temporaire, BLE, NFC), l'**application mobile** (constat 1) et le **module NFC** (constat 2) ;
- le **contenu de la configuration** (Wi-Fi, hub, compte, décision #65), son **stockage** (constat 3) et le **contenu du QR** (constat 4) ;
- le **déclencheur** de D6 et sa **sortie** (constat 5) ;
- l'**habillage** (engrenage dans l'anneau or, tailles de la maquette) et le **bandeau** (constat 6) ;
- la lisibilité du QR, la bibliothèque et la pile (constat 7) ;
- la séquence de démarrage (Wi-Fi, login) : US-FW-005 ; les écrans d'init et d'attente : US-FW-010 et US-FW-019 ; le bandeau : US-FW-009 ; le dessin du texte : US-FW-008.

## Critères d'acceptance

« Étiquette » dit où vit le comportement : **logique** (`lib/`) ou **adaptateur** (`src/`, validé au smoke test). « Preuve » dit ce qui établit le CA ; « Statut » dit où il en est dans le code au 2026-11-22. Les tests ne portent pas de numéro de CA : la correspondance se fait par le nom des tests. Aucun test ni smoke test n'a été écrit ou lancé pour cette US.

**Géométrie** (mesurée hors dépôt avec les avances réelles d'Oswald 48 px lues dans `src/fonts/OswaldRegular48.h` par `scripts/extract-font-advances.py`, en caractères ; reprise des proportions de la maquette `start-06-config` à 800 px de large, 1 unité = 8 px ; rien mesuré sur le matériel) :

- **dalle** 800 × 480 px, bandeau de 60 px (US-FW-009 CA-6), zone de **420 px** (y 60 à 480) ;
- **« Configuration »** en Oswald 48 : **235 px** (13 caractères) ;
- **carte blanche** : **288 × 288 px**, coins de 20 px, remplissage de 20 px, soit **248 px** utiles ;
- **ligne** : titre 235 px, écart de 48 px, carte 288 px = **571 px**, centrée sur 800 px : x 114 à 685 ; titre de x 114 à 349, carte de x 397 à 685 ; carte de **y 126 à 414** (centrée dans les 420 px), titre centré sur y 270 ;
- **carré du QR** : x 417 à 665, y 146 à 394 (248 × 248 px) ;
- **QR** (niveau de correction bas, module entier : ⌊248 ÷ côté⌋ px). Capacités en octets, niveau bas ; 321 et 322 octets vérifiés (versions 11 et 12) par `segno`, hors dépôt :

| Version | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 |
|---|---|---|---|---|---|---|---|---|---|---|---|
| Octets au plus | 17 | 32 | 53 | 78 | 106 | 134 | 154 | 192 | 230 | 271 | **321** |
| Modules par côté | 21 | 25 | 29 | 33 | 37 | 41 | 45 | 49 | 53 | 57 | 61 |
| Pixels par module | 11 | 9 | 8 | 7 | 6 | 6 | 5 | 5 | 4 | 4 | **4** |

- **mémoire** : `LGFXBase::qrcode` (LovyanGFX `dd228d1`, 1.3.0) cherche la version de 1 à 40 et réserve un tampon par `alloca` **à chaque tour, sans le libérer avant la sortie** : 2 541 octets cumulés jusqu'à la version 11, **59 700** jusqu'à la 40 ; taille de la pile non lue (constat 7). D'où la borne de **321 octets**.

| # | Étiquette | Critère | Résultat attendu | Preuve | Statut |
|---|---|---|---|---|---|
| CA-1 | logique | D6 est demandé avec une chaîne de **1 à 321 octets**, sans octet nul | L'écran courant, quel qu'il soit, est **remplacé** par D6 sous le **bandeau d'init** (US-FW-010 CA-1 : « Buzz’O’Live » au centre, nom vide, liaison « coupée », **sans batterie** : écart avec la maquette, constat 6). D6 porte le titre « Configuration » et la chaîne **telle que reçue**, octet pour octet, sans normalisation ni troncature, et **aucune consigne**. Il est **statique** : ni clignotement ni minuterie (US-FW-016 CA-5 ne le liste pas) | test à écrire (`test_screen_presenter`, `test_lcd_renderer`). Aujourd'hui `ScreenModel` a six variantes (`ScreenUnchanged`, `TextScreen`, `ZonesScreen`, `HighlightedZoneScreen`, `BannerScreen`, `ChosenZoneScreen`) : aucune ne porte de chaîne de QR ; `ScreenPresenter` n'a aucune méthode pour D6 (`ScreenFor`, `ScreenForBuzzSent`, `ScreenForLoginRefused`, `ScreenForTappedAnswer`) | **Non implémenté** |
| CA-2 | logique | La chaîne est **vide**, de **plus de 321 octets**, ou contient un **octet nul** (LovyanGFX lit une chaîne C) | D6 est affiché **sans carte** : le titre « Configuration » seul, centré dans la zone de 780 × 400 px comme un écran de texte (US-FW-008), sous le même bandeau, zones masquées (CA-3). Aucun QR partiel, aucun message d'erreur, aucun écran inchangé. Repli lisible, choix de Claude (décision #65) | test à écrire (une chaîne vide, 321 octets, 322 octets, un octet nul au milieu) | **Non implémenté** |
| CA-3 | logique | D6 s'affiche, alors que les quatre zones tactiles A à D étaient actives | Plus **aucune zone** n'est touchable, comme pour l'écran de refus (US-FW-015 CA-9) et l'init (US-FW-010 CA-1) ; l'affichage passe par `tappableZonesScreen` | test à écrire (`TappableZonesScreenTest`). `TappableZonesScreenTest.NoZoneIsTouchableBeforeAnyZoneIsShown` et `TappableZonesScreenTest.OnlyTheShownZonesAreTouchable` figent les zones des écrans existants, pas D6 | **Non implémenté** |
| CA-4 | logique | D6 est affiché et un appui arrive (bouton rouge ou tactile) | D6 est **local** : il ne répond à aucun message du hub (décision #36), n'en émet aucun, ne change **ni le mode ni le compteur d'élimination**. Un appui est **sans effet** : l'Inerte ignore l'appui (US-FW-002 CA-8, US-FW-005 CA-11). Afficher D6 pendant un mode armé n'est pas spécifié : le déclencheur est reporté (constat 5) | `BuzzerBehaviorTest.InertBuzzerDoesNotSendAnswerOnButtonPress` : fige l'Inerte (aucune réponse envoyée à un appui de bouton), pas un appui tactile ni D6 | Implémenté **en partie** (Inerte testé, D6 absent) |
| CA-5 | adaptateur | D6 est dessiné | `GfxScreen` : fond noir ; bandeau (US-FW-009 CA-6) ; **« Configuration »** en Oswald 48 blanc, de x 114 à 349 ; **carte blanche** de 288 × 288 px (x 397 à 685, y 126 à 414, coins de 20 px) ; le QR dans le carré de 248 px (CA-6). Sans carte (CA-2) : le titre seul, centré (US-FW-008). **Ni engrenage, ni anneau, ni couleur** (constat 6) | à écrire : smoke test (dessin) ; rendu sur la dalle jamais constaté (décision #42) | **Non implémenté** |
| CA-6 | adaptateur | Le QR est dessiné dans la carte | `lcd.qrcode(chaîne, 417, 146, 248, 1)` : modules **noirs sur fond blanc**, **niveau de correction bas** (fixé par LovyanGFX), **version minimale** qui contient la chaîne en partant de 1, **module entier** de ⌊248 ÷ côté⌋ px (11 à 4 px, table ci-dessus), le QR centré dans les 248 px. La marge blanche est celle de la carte (20 px, plus le reste de la division, réparti de part et d'autre) | à écrire : smoke test avec un téléphone (lecture du QR de 26 et de 140 caractères) ; marge et lisibilité : constat 7 | **Non implémenté** |

## Constats reportés

Rien de ce qui suit n'est un CA : ce sont des constats consignés dans `DEBTS.md`, à rouvrir comme indiqué. Les décisions de canal, de déclencheur et de contenu du QR ont été **reportées par Olivier le 2026-11-22** (une salve de quatre questions, recommandations retenues).

1. **Canal d'appairage et application mobile : non décidés.** Trois pistes, chiffrées : un point d'accès temporaire avec un QR `WIFI:` standard (41 caractères : version 3, 29 modules, 8 px ; le téléphone rejoint le point d'accès puis envoie la configuration en HTTP ; le buzzer coupe son Wi-Fi station pendant ce temps), le BLE avec un QR d'identifiant (26 caractères : version 2, 25 modules, 9 px), le NFC (constat 2). **Poids en flash non mesuré** (BLE, serveur HTTP) sur 253 Kio libres (mesure d'Olivier citée par `DECISIONS.md`, ancienne), déjà visés par l'audio (US-FW-021 constats 3 et 4). L'application mobile n'existe pas ; côté hub, seule la VISION la mentionne. À rouvrir quand l'application est cadrée.
2. **Module NFC.** Le CDC §2 le liste, la décision #1 n'en dit rien. Elecrow annonce pour la CrowPanel 5″ une carte TF, 2 GPIO, un haut-parleur, une UART0, deux I²C et une batterie : **pas de NFC** ; l'I²C (GPIO 19 et 20) sert déjà au tactile (`Gt911TouchReader`). Un lecteur serait un module externe, non vérifié sur le schéma de la carte. Le CDC §2 n'est pas modifié (règle de la session) : à corriger quand le CDC est supprimé.
3. **Contenu de la configuration et stockage** (décision #65). **Cible** : le buzzer reçoit son **Wi-Fi** (nom et mot de passe), l'**adresse du hub** (hôte, port) et son **compte** (nom, mot de passe), pour **un seul binaire** pour les dix buzzers. Aujourd'hui : `secrets.h` (quatre macros, compilées) et `192.168.1.183:3000` en dur deux fois dans `main.cpp` : **un binaire par buzzer**. Non décidés : le stockage persistant (rien dans le code), l'effacement, le mot de passe en clair, le HTTP clair. US-FW-005 (CA-3, CA-9) lit encore `secrets.h` : inchangée.
4. **Contenu du QR : une chaîne fournie.** L'exemple de la maquette, `buzzolive://setup?b=7F3A91` (26 caractères, version 2), est un identifiant, non des secrets ; l'origine de `7F3A91` n'est définie nulle part. Mesures (`segno`, niveau bas) : Wi-Fi seul de 20 + 30 caractères, 68 caractères, version 4 (7 px) ; Wi-Fi, hub et compte (mot de passe de 20), 140 caractères, version 7 (5 px) ; maximum (32 + 63 + 72), 239 caractères, version 10 (4 px) : sous la borne de 321 octets. Un QR qui porterait des secrets reste photographiable : non traité.
5. **Déclencheur, sortie, Wi-Fi sans fin et `401`.** D6 n'est affiché par personne. À lever avant tout déclencheur : US-FW-005 CA-2 (Wi-Fi attendu sans fin, rien à l'écran, aucune distinction entre un mot de passe faux et un réseau absent) ; US-FW-005 CA-5 et US-FW-015 CA-9 (un `401` arrête le buzzer jusqu'au redémarrage ; l'écran de refus porte le bandeau de l'init depuis la décision #85 ; l'anneau et le cadenas sont dessinés depuis la décision #86 : US-FW-015 CA-13) ; US-FW-005 CA-11 (aucun appui lu pendant `setup()` bloquant : un appui long exige une init non bloquante, dette « `setup()` bloquant »). Pistes : aucune configuration enregistrée, appui long, `401`. La sortie de D6 (redémarrage, message de l'application) n'est pas décidée.
6. **Habillage et bandeau de D6.** La maquette montre un engrenage plein or dans un anneau or de 160 px, un titre de 68 px, et un bandeau de 44 px avec la batterie (78 %). Le rendu spécifié : titre blanc de 48 px sans anneau (même choix que US-FW-019 constat 2), bandeau d'init de 60 px sans batterie (US-FW-009 constat ; la liaison « coupée » dépend de US-FW-009 CA-4 et CA-8, non implémentés). La maquette n'est pas corrigée (hors périmètre).
7. **Lisibilité, marge, bibliothèque, pile.** La marge blanche est de 20 px : 2,2 modules à 9 px, 5 modules à 4 px, alors que la norme en demande 4 : non vérifié à la lecture par un téléphone. Le module de 4 px est une borne choisie, non mesurée sur la dalle. `LGFXBase::qrcode` lue à `dd228d1` (1.3.0) : niveau de correction fixé à bas, `alloca` en boucle (voir Géométrie), taille de la pile de `loopTask` non lue ; LovyanGFX n'est pas épinglée (dette « Dépendances flottantes ») : la version résolue peut différer.

## Sources

CDC §2 (module NFC), §9 (séquence de démarrage), §10 n°1 ; maquette `docs/mockups/start-06-config.html` et `mockup.css` (`.qr`, `.config .side`, `.wait-ring`, `.wait-title`) ; décisions #1, #33, #36, #58, #62, #65 ; US-FW-002 (CA-8), US-FW-005 (CA-2, CA-3, CA-5, CA-9, CA-11), US-FW-008, US-FW-009 (CA-4, CA-6, CA-8), US-FW-010 (CA-1), US-FW-015 (CA-9), US-FW-016 (CA-5), US-FW-019 (constat 2), US-FW-021 (constats 3 et 4) ; US-002 (CA-1, CA-2, CA-4) du hub (`quiz-live-server`, `docs/specs/`, `ed6bd31`) et `docs/shared/VISION.md` ; code lu : `lib/rendering/ScreenModel.h`, `ScreenPresenter.h`, `TappableZonesScreen.h`, `src/main.cpp` (`setup()`, globales), `src/secrets.h.example`, `platformio.ini` ; LovyanGFX lue hors dépôt à `dd228d1` (`LGFXBase.cpp` `qrcode`, `lgfx_qrcode.h`, `lgfx_qrcode.c`) ; `test/test_game`, `test/test_startup`, `test/test_tappable_zones` ; Elecrow, wiki de la CrowPanel 5.0″ ; `DEBTS.md` (« Paramétrage NFC non spécifié », « Configuration maquettée sans déclencheur, canal ni report », « Adresse du hub en dur, en HTTP clair », « Dépendances flottantes », « `setup()` bloquant pendant l'init »).
