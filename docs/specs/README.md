# Spécifications du firmware buzzer : index des user stories

> Ce dossier remplace progressivement `docs/CDC-FIRMWARE-BUZZER.md`. Le CDC reste en lecture seule jusqu'à ce que chaque section soit rédigée dans une US ; il est alors supprimé, avec `docs/Quiz Buzzer Game - Liste des états.csv` (son origine), dans une session `docs(specs)` dédiée.

## Principes directeurs

Le buzzer est un **exécutant piloté par le serveur** : le hub Node.js est le chef d'orchestre. Le buzzer n'initie rien de lui-même, à trois exceptions près, toutes locales :

1. le **feedback immédiat** (écran, éventuellement son) lors d'un appui armé, pour éviter la latence réseau ;
2. l'**auto-verrouillage** après un appui armé (lock-first) ;
3. le passage en mode **Éliminé** après 3 échecs consécutifs de reconnexion WiFi.

Frontière d'architecture : la **logique pure** vit dans `lib/` (testable en natif, sans dépendance matérielle) ; les **adaptateurs matériels** vivent dans `src/` (validés au smoke test), derrière des abstractions.

## Matériel par buzzer

| Composant | Rôle |
|---|---|
| Écran LCD tactile | Affichage piloté par le serveur ; zones tactiles A–D pour répondre en mode MCQ |
| Un gros bouton rouge « buzz » (sans LED intégrée) | Réponse en mode SPEED |
| Haut-parleurs | Restitution sonore |
| WiFi / WebSocket | Communication avec le hub (réseau local uniquement) |
| Module NFC | Paramétrage du buzzer via l'application mobile (non cadré, voir US-FW-022) |

Le buzzer ne compte qu'un seul bouton physique, le bouton rouge (`DECISIONS.md` #1). Il n'y a ni boutons A–D ni LED.

## Statuts et couches

| Statut | Sens |
|---|---|
| Implémentée | Le comportement existe dans le code ; l'US le spécifie, les CA sont à rédiger depuis le réel |
| Partielle | Une partie est implémentée ; le reste est en dette (`DEBTS.md`) |
| À cadrer | Périmètre connu, comportement pas encore décidé |
| Vide | Titre et périmètre seulement ; aucun CA tant que la décision bloquante n'est pas prise |

- **Logique et connexion** (US-FW-001 à 007) : comportement du buzzer, sans écran.
- **Rendu** (US-FW-008 à 016) : ce que le joueur voit.
- **Vides** (US-FW-017 à 022) : numéros réservés, dépendent de décisions ou du hub.

Ordre de travail : ordre numérique, la logique avant le rendu. Une US vide est sautée tant qu'elle n'a pas de CA.

## Index

| US | Titre | Couche | Dépend du hub | Statut |
|---|---|---|---|---|
| [US-FW-001](US-FW-001-detecter-un-appui.md) | Détecter un appui | Logique | — | Implémentée |
| [US-FW-002](US-FW-002-repondre-a-une-question-a-choix-multiples.md) | Répondre à une question à choix multiples | Logique | US-011 | Partielle |
| [US-FW-003](US-FW-003-buzzer-sur-une-question-de-rapidite.md) | Buzzer sur une question de rapidité | Logique | US-012 | Partielle |
| [US-FW-004](US-FW-004-dialoguer-avec-le-hub.md) | Dialoguer avec le hub | Logique | US-009 | Partielle |
| [US-FW-005](US-FW-005-demarrer-et-sauthentifier.md) | Démarrer et s'authentifier | Logique | US-003, US-009 | Partielle |
| [US-FW-006](US-FW-006-se-reconnecter-ou-etre-elimine.md) | Se reconnecter ou être éliminé | Logique | US-009, US-019 | Partielle |
| [US-FW-007](US-FW-007-garder-la-session-ouverte.md) | Garder la session ouverte | Logique | US-021 | Partielle |
| [US-FW-008](US-FW-008-afficher-des-textes-lisibles.md) | Afficher des textes lisibles | Rendu | — | Partielle |
| [US-FW-009](US-FW-009-bandeau-du-haut.md) | Bandeau du haut | Rendu | US-009, US-010, US-019 (nom du participant à définir) | Partielle |
| [US-FW-010](US-FW-010-ecrans-dinit-et-dattente.md) | Écrans d'init et d'attente | Rendu | US-009, US-019 (`game_resumed` non émis au 2026-11-22) | Partielle |
| [US-FW-011](US-FW-011-question-a-choix-multiples-a-lecran.md) | Question à choix multiples à l'écran | Rendu | US-011 (`question_index` à ajouter à `question_choices` ; aucun de ses messages n'est émis au 2026-11-22) | Partielle |
| [US-FW-012](US-FW-012-question-de-rapidite-a-lecran.md) | Question de rapidité à l'écran | Rendu | US-012 (aucun de ses messages n'est émis au 2026-11-22 ; `question_open` porte déjà `question_index`) | Partielle |
| [US-FW-013](US-FW-013-resultat-dune-question.md) | Résultat d'une question | Rendu | US-011, US-012 (`question_result` : aucune occurrence dans `src/` du hub au 2026-11-22 ; `player_answer` d'un joueur sans réponse non précisé) | Partielle |
| [US-FW-014](US-FW-014-reactions-du-personnage.md) | Réactions du personnage | Rendu | US-011, US-012 (`question_result` et `buzz_invalidated` : aucune occurrence dans `src/` du hub au 2026-11-22 ; aucun champ pour le personnage, fixé par buzzer) | Partielle |
| [US-FW-015](US-FW-015-coupure-elimination-et-refus-a-lecran.md) | Coupure, élimination et refus à l'écran | Rendu | — | Partielle |
| [US-FW-016](US-FW-016-animer-lecran.md) | Animer l'écran | Rendu | — | À cadrer |
| [US-FW-017](US-FW-017-classement-intermediaire.md) | Classement intermédiaire | Vide | US-014 | Vide |
| [US-FW-018](US-FW-018-classement-final-et-fin-de-partie.md) | Classement final et fin de partie | Vide | Aucun message défini | Vide |
| [US-FW-019](US-FW-019-attente-du-maitre-et-presentation-de-la-partie.md) | Attente du maître du jeu et présentation de la partie | Vide | Aucun message défini | Vide |
| [US-FW-020](US-FW-020-photo-ou-son-avant-une-question.md) | Photo ou son avant une question | Vide | Aucun message défini | Vide |
| [US-FW-021](US-FW-021-sons.md) | Sons | Vide | US-018 | Vide |
| [US-FW-022](US-FW-022-parametrage-par-nfc-ou-qr-code.md) | Paramétrage par NFC ou QR code | Vide | — | Vide, bloquée |

Les numéros d'US du hub viennent du CDC et de `DEBTS.md` ; ils sont à confirmer à la rédaction de chaque US.

## Correspondance CDC → US

Table de migration : le CDC n'est supprimé que lorsque chaque ligne est couverte par une US rédigée. ✅ marque la part déjà rédigée dans une US ; les destinations sans ✅ restent à rédiger.

| Section du CDC | Destination |
|---|---|
| §0 Note de passation | Supprimée (périmée, dette « CDC §0 et §11 périmés ») |
| §1 Principe directeur, §2 Matériel | Ce fichier |
| §3 Mécanique d'appui | ✅ US-FW-001 (détection, famille et valeur) ; appui d'une famille non armée ignoré : ✅ US-FW-002 (MCQ, Inerte, Éliminé), ✅ US-FW-003 (SPEED) ; zones tactiles actives seulement sur l'écran des propositions : ✅ US-FW-011 (CA-13) ; message non reconnu, invalide ou `error` ignoré : ✅ US-FW-004 (CA-6, CA-7) ; anti-rebond : constat reporté (`DEBTS.md`) |
| §4.1 Mode Inerte | ✅ US-FW-002 (CA-8, appuis ignorés, tous modes) ; appui du bouton rouge en Inerte : ✅ US-FW-003 |
| §4.2 Modes armés MCQ | ✅ US-FW-002 |
| §4.3 Modes armés SPEED (dont ré-armement) | ✅ US-FW-003 |
| §4.4 Mode Éliminé | ✅ US-FW-006 (CA-3, CA-4, CA-9, CA-10 : silence total, réseau compris) ; appui ignoré en Éliminé : ✅ US-FW-002 (CA-9, non câblé : câblage à US-FW-006 CA-9) |
| §5 Reconnexion WiFi | ✅ US-FW-006 (CA-1 à CA-8 : compteur, tentatives sur le WebSocket vers le hub, perte initiale exclue, intervalle de 5 s) |
| §6 Table de transitions | ✅ lignes MCQ, Inerte, « tout mode armé » et Éliminé-appui : US-FW-002 ; lignes SPEED : ✅ US-FW-003 ; passage en Éliminé : ✅ US-FW-006 (CA-3, CA-9) |
| §7 Recadrage du CSV | Supprimée (historique) ; écart « affichage progressif abandonné » : ✅ US-FW-011 (CA-2) |
| §8.1 Messages reçus et émis | ✅ US-FW-004 (lecture du type, rendu puis aiguillage, messages sans effet sur le mode, format de `auth`, `auth_refresh`, `answer`, `buzz`) ; armement et désarmement SPEED (`question_open`, `buzz_unlocked`, `buzz_locked`, `buzz_accepted`, `buzz_invalidated`, `buzz` émis) : ✅ US-FW-003 ; `token_expiring_soon`, `token_expired`, `auth_refresh` : ✅ US-FW-007 (CA-1 à CA-7 : login frais, aucun envoi sur échec ou `401`, `auth_success` de réponse sans effet sur l'écran) ; `play_*` : US-FW-021 |
| §8.2 Habillage commun | ✅ US-FW-009 (bandeau du haut : nom, titre, liaison, présence sur chaque écran) ; bandeau dès la mise sous tension : ✅ US-FW-010 (CA-1) ; points, rang et batterie : constats reportés (`DEBTS.md`) ; en-tête et chrono de la question à choix multiples : ✅ US-FW-011 (CA-3, CA-7, CA-8, CA-12), capsules : constat reporté (`DEBTS.md`) ; en-tête et chrono de la question de rapidité : ✅ US-FW-012 (CA-1 à CA-5), capsules : constat reporté (`DEBTS.md`) |
| §8.2 Vocabulaire des effets d'écran | US-FW-016 (et chaque US de rendu pour ses effets) ; zones masquées, actives et conservées, mise en évidence statique de la zone choisie (MCQ) : ✅ US-FW-011 (CA-10 à CA-13) ; « Zones masquées » de la question de rapidité : ✅ US-FW-012 (CA-14) ; clignotements : US-FW-016 |
| §8.2 Lignes init, `auth_success`, `game_resumed` | ✅ US-FW-010 (CA-1 à CA-7 : écran d'init statique, « Connecté » et « Partie en cours » sur deux lignes, enchaînement à la reconnexion, dessin) ; anneau, icônes et clignotement : constats reportés, US-FW-016 |
| §8.2 Lignes `question_title`, `question_choices`, `timer_tick`, `timer_end`, `answer_received`, appui local MCQ | ✅ US-FW-011 (CA-1 à CA-15 : titre, propositions, en-tête, chrono à la seconde recalé au tick, « Trop tard », appui local, « Réponse enregistrée », zones tactiles, dessin) ; mise en évidence de la bonne réponse : ✅ US-FW-013 (CA-3) |
| §8.2 Lignes `question_open`, `buzz_accepted`, `buzz_locked`, `buzz_invalidated`, `buzz_unlocked`, appui local SPEED | ✅ US-FW-012 (CA-1 à CA-16 : écran d'ouverture, chrono, `timer_tick` et `timer_end` du SPEED, appui local, « À toi ! », « X a buzzé », élimination, réouverture, état retenu dont « ce buzzer a buzzé », ignoré hors Inerte ; réaction R9 : ✅ US-FW-014 (CA-4), clignotement : US-FW-016) ; titre de 250 caractères au plus, taille réduite s'il est long : ✅ US-FW-008 (CA-2, CA-6, CA-7) |
| §8.2 Lignes `question_result` | ✅ US-FW-013 (CA-1 à CA-14 : MCQ ou SPEED déduit du contenu, « Bonne réponse : B · Votre réponse : C » dans un en-tête de 60 px, zone de la bonne réponse en évidence, réponse fausse du joueur à double cadre, propositions retenues, SPEED « Validé · +N pts », « Raté », « Question terminée » et réponse attendue ; lit l'état « ce buzzer a buzzé » posé par ✅ US-FW-012 CA-7) ; réactions du personnage : ✅ US-FW-014 (CA-1 à CA-3) ; couleurs, clignotements, points du bandeau : constats reportés |
| §8.2 Réactions du personnage | ✅ US-FW-014 (CA-1 à CA-11 : situation lue dans le message, tirage dans R1 à R5, R6 à R8 ou R10, R9 fixe sur `buzz_invalidated`, 2000 ms puis l'écran du message, interruption par tout message qui produit un écran, personnage fixé par buzzer, libellés, fond blanc et dessin) ; personnage par joueur, stockage des images, couleurs : constats reportés |
| §8.2 Lignes réseau (connexion perdue, Hors jeu, refus 401) | US-FW-015 ; logique du refus : ✅ US-FW-005 (CA-5), ✅ US-FW-006 (CA-11, refus en reconnexion) |
| §8.2 Lignes `play_sound_url`, `play_system_sound` | US-FW-021 |
| §9 Démarrage et connexion initiale | ✅ US-FW-005 (séquence, login, refus `401`, `auth` frais, `auth_success`, appui pendant l'init) ; rendu d'init : ✅ US-FW-010 (CA-1, CA-2) ; articulation avec le compteur : ✅ US-FW-005 (CA-10, armement) et ✅ US-FW-006 (CA-1, CA-2 consommation) |
| §10 Points ouverts (NFC, son du feedback local) | US-FW-022, US-FW-021 |
| §11 Prochaine étape TDD | Supprimée (périmée) |

Hors CDC, issus des maquettes (`docs/mockups/`) et de `DEBTS.md` : classements, attente et présentation de la partie, photo et son, avatar par joueur, configuration. Ils alimentent US-FW-014 et US-FW-017 à 022.
