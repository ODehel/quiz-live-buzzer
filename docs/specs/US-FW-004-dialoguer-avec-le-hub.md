# US-FW-004 — Dialoguer avec le hub

- **Couche** : logique
- **Statut** : Partielle, CA-1 à CA-12 rédigés depuis le réel le 2026-11-22 (CA-7 non implémenté : un JSON invalide peut encore être accepté)
- **Dépend du hub** : US-009

## User story

> **En tant que** joueur,
> **quand** mon buzzer échange des messages avec le hub,
> **alors** il ne réagit qu'aux messages qu'il comprend, ignore sans bruit tout le reste (message inconnu, invalide, `error`), et n'émet que des messages que le hub accepte.

## Périmètre

Messages WebSocket JSON (texte) échangés avec le hub :

- **Réception** : un événement WebSocket devient un événement du buzzer, le texte est lu comme un message JSON, le message est rendu puis aiguillé : `auth_success` et `token_expiring_soon` vers la session, `question_choices`, `question_open`, `timer_end` et `buzz_unlocked` vers `BuzzerBehavior`, tout le reste sans effet sur le mode.
- **Émission** : le format JSON de `auth`, `auth_refresh`, `answer` et `buzz`.
- **Message non reconnu, invalide ou `error`** : ignoré silencieusement, sans remontée d'erreur ni log (CDC §3, YAGNI).

**Hors périmètre** :

- ce que chaque message change au mode armé ou inerte : US-FW-002 (MCQ, règles communes) et US-FW-003 (SPEED, dont `buzz_locked`, CA-8) ;
- ce que le joueur voit à la réception de chaque message : US-FW-008 à US-FW-016 ; ici, seul l'ordre rendu puis aiguillage compte (CA-2) ;
- quand `auth` est émis et avec quel token (token frais avant chaque `auth`) : US-FW-005 ; `auth_refresh` sur `token_expiring_soon` et la fermeture `token_expired` : US-FW-007 ;
- la perte de connexion, la reconnexion et le passage en Éliminé : US-FW-006 ;
- la requête HTTP du fichier de `play_sound_url` et les sons : US-FW-021 ;
- l'adresse du hub, l'ouverture du WebSocket et l'obtention du token HTTP : US-FW-005.

## Critères d'acceptance

Tous les CA sont de la couche **logique** (CA-12 inclut un adaptateur de `src/`, validé au smoke test seulement). « Preuve » dit ce qui établit le CA ; « Statut » dit où il en est dans le code au 2026-11-22. Les tests ne portent pas de numéro de CA : la correspondance se fait par le nom des tests.

| # | Étiquette | Critère | Résultat attendu | Preuve | Statut |
|---|---|---|---|---|---|
| CA-1 | logique | Le buzzer reçoit un texte du hub | Le texte est lu comme un objet JSON ; son champ `type` (texte) identifie le message. Un message est une trame texte complète (`OnMessageReceived(payload)`) | `test_deserialization` (`ParsedQuestionOpenMessageIsTypeQuestionOpen`, `ParsedQuestionChoicesMessageIsTypeQuestionChoices`) ; `test_connection` (`ReceivedMessageIsRenderedByType`, `TextReceivedEventTriggersSpeedBuzzArmed`, `TextReceivedTriggersMessageReceived`) | Implémenté et testé |
| CA-2 | logique | Un message reçu est rendu **avant** d'être aiguillé | Le rendu d'un message voit le mode d'avant ce message : `timer_end` affiche « Trop tard » seulement si une question était armée, car le désarmement vient après. Cet ordre est un contrat (décision #5), pas un détail. Vaut aussi pour les messages sans effet sur le mode (CA-5) | `test_connection` (`TimerEndIsRenderedWithTheModeOfBeforeTheMessage`, `AuthSuccessIsRenderedWithTheUsernameOfTheHubInTheTopBar`) | Implémenté et testé |
| CA-3 | logique | Le buzzer reçoit `question_choices`, `question_open`, `timer_end` ou `buzz_unlocked` | Le message est transmis à `BuzzerBehavior` (`OnQuestionChoices`, `OnQuestionOpen`, `OnTimerEnd`, `OnBuzzUnlocked`). Leur effet sur le mode est dans US-FW-002 et US-FW-003. Aucun autre type n'atteint `BuzzerBehavior` ; `buzz_locked` y arrivera avec US-FW-003 CA-8 | `test_dispatch` (`QuestionOpenArmsSpeed`, `QuestionChoicesArmsMcq`, `TimerEndDisarmsArmedBuzzer`, `BuzzUnlockedArmsSpeed`) ; chaîne complète : `test_connection` (`SpeedBuzzArmedOnQuestionOpenMessageReceived`) | Implémenté et testé ; `buzz_locked` non aiguillé (écart de US-FW-003 CA-8) |
| CA-4 | logique | Le buzzer reçoit `auth_success` ou `token_expiring_soon` | Ces deux messages ne passent **pas** par le répartiteur : `auth_success` arme le compteur d'élimination (`ReconnectionPolicy::OnAuthSuccess`, US-FW-006), `token_expiring_soon` déclenche `SendAuthRefresh` (US-FW-007). Les deux sont rendus avant (CA-2) et n'ont aucun effet sur le mode | `test_connection` (`AuthSuccessMessageArmsThePolicy`, `TokenExpiringSoonTriggersAuthRefresh`) | Implémenté et testé |
| CA-5 | logique | Le buzzer reçoit un message connu sans effet sur le mode : `question_title`, `timer_tick`, `answer_received`, `buzz_accepted`, `buzz_invalidated`, `question_result`, `game_resumed`, `token_expired`, `play_sound_url`, `play_system_sound` | Le mode ne change pas et rien n'est envoyé : un MCQ armé reste armé, un SPEED armé reste armé, un Inerte reste Inerte. Seul l'affichage peut changer (US-FW-008 à US-FW-016). `token_expired` est suivi d'une fermeture du hub (US-FW-007) | test natif paramétré à écrire : pour chaque type, armé (MCQ puis SPEED) puis `Dispatch`, l'appui de la bonne famille envoie encore ; Inerte puis `Dispatch`, l'appui n'envoie rien | Implémenté par absence de branche ; **non testé** |
| CA-6 | logique | Le buzzer reçoit un message dont le `type` est inconnu, vide ou n'est pas écrit exactement comme un type connu (`QUESTION_OPEN`, ` question_open`), ou un message `error` du hub | Ignoré : le mode ne change pas, rien n'est envoyé, l'écran reste inchangé, aucune erreur n'est remontée. Les `error` du hub (`INVALID_MESSAGE`, `TOKEN_EXPIRED`, `TOKEN_INVALID` : US-011 CA-43, US-021 CA-14 à CA-17) entrent dans ce cas. La comparaison du `type` est exacte, sensible à la casse | rendu : `test_lcd_renderer`, `test_rendering`, `test_screen_presenter` (cas `unknown_type`, écran inchangé) ; mode et envoi : test natif à écrire (type inconnu, `QUESTION_OPEN`, `error`, `type` vide, sur un buzzer armé puis Inerte) | Implémenté ; testé pour le rendu, **non testé** pour le mode, l'envoi et la casse |
| CA-7 | logique | Le buzzer reçoit un texte qui n'est pas un objet JSON complet dont le `type` est un texte : texte vide, texte quelconque, tableau, nombre, `null`, objet sans `type`, `type` non textuel, JSON tronqué (`{"type":"question_open"`), texte parasite après l'objet (`{"type":"question_open"}garbage`) | Ignoré **en entier** : ni rendu, ni aiguillage, ni envoi, le mode ne change pas. Un `type` lisible dans un JSON invalide ne compte pas (CDC §3 : « invalide → ignoré »). Décision #47 | test natif à écrire avec le correctif : les neuf cas ci-contre sur `ConnectionEventHandler` (renderer et `BuzzerBehavior` non appelés) et sur `MessageDeserializer` | **Non implémenté** : sonde du 2026-11-22 sur `MessageDeserializer::Parse` : texte vide, texte quelconque, `[]`, `42`, `null`, `{}` donnent le type littéral « null », `{"type":42}` donne « 42 » (jamais un type connu, donc ignorés par hasard, mais rendus avec ce type) ; `{"type":"question_open"` et `{"type":"question_open"}garbage` donnent `question_open` : message accepté, buzzer armé. Écart consigné en dette |
| CA-8 | logique | Un message porte un champ attendu absent ou de mauvais type | Le champ est lu comme **absent** : jamais une valeur inventée. Un champ n'est présent que s'il a le bon type : `remaining_seconds` et `question_index` entiers, `buzzer_username`, `correct_answer` et `player_answer` textes, `correct` booléen, `choices` tableau dont **tous** les éléments sont des textes ; `username` et `title` valent le texte vide s'ils sont absents ou non textuels. Un champ inconnu est ignoré. Chaque consommateur décide de ce qu'il fait d'un champ absent (le rendu laisse l'écran inchangé, décision #6) | `test_deserialization` (`ParsedTimerTickWithATextRemainingSecondsHasNone`, `ParsedQuestionChoicesWithANumberAmongTheChoicesHasNone`, `ParsedQuestionResultWithANullPlayerAnswerHasNone` et les cas jumeaux sur chaque champ) | Implémenté et testé ; champ inconnu non testé (sans conséquence : `HubMessage` ne le lit pas) |
| CA-9 | logique | Le buzzer émet `auth` ou `auth_refresh` | Un message texte `{"type":"auth","token":"<JWT>"}` ou `{"type":"auth_refresh","token":"<JWT>"}`, conforme à US-009 CA-4 et US-021 CA-10. Le token est inséré tel quel, sans échappement : un JWT (base64url) ne contient ni guillemet ni antislash. Quand et avec quel token : US-FW-005 et US-FW-007 | `test_serialization` (`AuthMessageCarriesTheToken`, `AuthRefreshMessageCarriesTheToken`) ; envoi du payload : `test_session` (`SendsAuthCarryingAFreshlyFetchedToken`, `SendsAuthRefreshCarryingTheUpdatedToken`) | Implémenté et testé ; absence d'échappement non testée |
| CA-10 | logique | Le buzzer émet `answer` | `{"type":"answer","value":"A"}` avec la lettre touchée (`A`, `B`, `C` ou `D`, une seule lettre majuscule), conforme à US-011. Une valeur invalide serait ignorée par le hub (US-011 CA-20) ; le firmware n'en envoie pas, car seules les quatre zones produisent une lettre (US-FW-001) | `test_serialization` (`AnswerAMessageCarriesTheValue`, `AnswerBMessageCarriesTheValue`) | Implémenté ; testé pour `A` et `B`, **`C` et `D` non testés** |
| CA-11 | logique | Le buzzer émet `buzz` | `{"type":"buzz"}`, sans autre champ, conforme à US-012 | `test_serialization` (`BuzzMessageIsTypeBuzz`) | Implémenté et testé |
| CA-12 | logique | L'adaptateur WebSocket reçoit un événement de la bibliothèque | Connexion établie → `OnConnectionEstablished` ; texte → `OnMessageReceived` avec le payload complet (longueur respectée) ; déconnexion → `OnConnectionLost`. Tout autre événement (binaire, ping, pong, erreur, fragments de texte) est ignoré : un message fragmenté est donc perdu sans bruit | traduction événement du buzzer → écoute : `test_connection` (`BuzzerEventTranslatorTest`, trois cas) ; adaptateur `WebSocketEventListener` (`src/`) : smoke test seulement, jamais constaté pour binaire ni fragments | Implémenté ; adaptateur **non testé** (frontière voulue, `src/`) |

## Constats reportés

Rien de ce qui suit n'est un CA : ce sont des constats consignés dans `DEBTS.md`, à rouvrir comme indiqué.

- **Message de jeu avant `auth_success`** : `ConnectionEventHandler` n'a pas d'état « authentifié » ; un `question_open` reçu avant `auth_success` arme le buzzer. Le hub n'émet rien à un client non authentifié (US-009 CA-22). Décision #47 : pas de CA ni de code ; à rouvrir avec la reprise de connexion (US-FW-006, US-021).
- **Retour de `sendTXT` ignoré** : `WebSocketMessageSender` et `WebSocketPayloadSocketWriter` ne contrôlent pas l'envoi. Déjà consigné (US-FW-002), à rouvrir avec US-FW-006.
- **`auth_refresh` avec l'ancien token** : déjà consigné (« `auth_refresh` sur `token_expiring_soon` : chemin mort »), US-FW-007.
- **Hub** : le hub n'émet pas encore `question_open`, `question_title`, `question_result` ni `buzz_locked` vers les buzzers (dette « jamais émis par le hub ») ; les formats ci-dessus reposent sur US-009, US-011, US-012 et US-021 du hub, lus le 2026-11-22. Un message du hub destiné à l'admin (`buzzer_connected`, `game_state_sync`) tomberait dans CA-6 s'il atteignait un buzzer.
- **Écarts de preuve** : types sans effet (CA-5), type inconnu, `error` et casse (CA-6), `answer` pour `C` et `D` (CA-10), absence d'échappement du token (CA-9), adaptateur (CA-12).

## Sources

CDC §3 (règle « non reconnu / invalide / hors contexte → ignoré »), §8.1 (messages reçus et émis) ; `lib/dispatch/HubMessageDispatcher.h`, `lib/connection/` (`ConnectionEventHandler`, `BuzzerEventTranslator`), `lib/serialization/` (`HubMessage`, `MessageDeserializer`, `MessageSerializer`), `lib/session/TokenSessionMessageSender.h`, `src/WebSocketEventListener.cpp`, `src/WebSocketMessageSender.cpp`, `src/WebSocketPayloadSocketWriter.cpp`, `src/main.cpp` ; `test/test_dispatch`, `test/test_connection`, `test/test_deserialization`, `test/test_serialization`, `test/test_session` ; US-009, US-011, US-012 et US-021 du hub (`quiz-live-server`, `docs/specs/`) ; décisions #5, #6, #44, #45, #46 et #47 ; `DEBTS.md`.
