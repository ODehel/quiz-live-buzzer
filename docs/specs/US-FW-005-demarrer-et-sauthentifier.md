# US-FW-005 — Démarrer et s'authentifier

- **Couche** : logique
- **Statut** : Partielle, CA-1 à CA-11 rédigés depuis le réel le 2026-11-22 ; CA-6 implémenté et testé le 2026-11-23 (`fix(provisioning): escape quotes and backslashes in login credentials`) ; reste en dette : preuves manquantes du démarrage (CA-5, CA-7, CA-8, CA-11), init bloquant, un seul statut arrête le login
- **Dépend du hub** : US-003, US-009

## User story

> **En tant que** joueur,
> **quand** je mets mon buzzer sous tension,
> **alors** il rejoint le réseau, obtient un token auprès du hub, s'authentifie sur le WebSocket et n'entre en jeu qu'une fois le hub d'accord, sans jamais réagir à un appui avant ce moment.

## Périmètre

Séquence d'initialisation linéaire, de la mise sous tension à `auth_success` :

1. connexion au Wi-Fi ;
2. `POST /api/v1/token` avec les identifiants du buzzer, réessayé tant qu'il échoue ;
3. ouverture du WebSocket `ws://<ip>:<port>/ws` ;
4. à l'ouverture, `auth` avec un token frais (un second login) ;
5. `auth_success` met fin à l'init.

Un `401` au login arrête le buzzer jusqu'au redémarrage. Tout autre échec est réessayé sans fin. Pendant l'init, un appui est sans effet. Le compteur d'élimination est inactif pendant toute cette phase.

**Hors périmètre** :

- ce que le joueur voit pendant l'init (logo, bandeau clignotant, écran « Connecté ») : US-FW-010 ; l'écran « Connexion refusée » : US-FW-015 (ici, seul le fait qu'il est affiché compte, CA-5) ;
- le format JSON de `auth` : US-FW-004 CA-9 ; `auth_refresh`, `token_expiring_soon` et `token_expired` : US-FW-007 ;
- la perte de connexion après `auth_success`, la reconnexion, le compteur et l'élimination : US-FW-006 ; seul le moment où le compteur est armé est ici (CA-10) ;
- le comportement des modes armés : US-FW-002 et US-FW-003 ; le CA-11 ne reprend de US-FW-002 CA-8 que l'effet pendant l'init ;
- la configuration du Wi-Fi, des identifiants et de l'adresse du hub par NFC ou QR code : US-FW-022.

## Critères d'acceptance

Tous les CA sont de la couche **logique**. « Preuve » dit ce qui établit le CA (`constat` : lecture du code, sans test natif, car la séquence vit dans `src/main.cpp` ou dans un adaptateur de `src/`) ; « Statut » dit où il en est dans le code au 2026-11-22. Les tests ne portent pas de numéro de CA : la correspondance se fait par le nom des tests. Aucun smoke test n'est écrit dans cette session.

| # | Étiquette | Critère | Résultat attendu | Preuve | Statut |
|---|---|---|---|---|---|
| CA-1 | logique | Le buzzer est mis sous tension | Les étapes s'enchaînent dans l'ordre du périmètre, chacune déclenchant la suivante sans décision métier : Wi-Fi, login, ouverture du WebSocket, puis `auth` envoyé par l'événement « connexion établie » (CA-9). `setup()` garde la main jusqu'à l'ouverture du WebSocket : `loop()` ne tourne pas avant (dette « `setup()` bloquant ») | constat (`setup()` de `src/main.cpp`) ; smoke test à venir | Implémenté, **non testé** |
| CA-2 | logique | Le Wi-Fi ne se connecte pas | Le buzzer attend sans fin, sans plafond, sans élimination ni changement d'écran : `WiFi.status()` est relu toutes les 500 ms jusqu'à la connexion. Rien n'est rendu pendant l'attente (US-FW-010) | constat (`setup()`) | Implémenté, **non testé** |
| CA-3 | logique | Le Wi-Fi est connecté : le buzzer demande un token | `POST /api/v1/token` avec un corps JSON `{"username":"…","password":"…"}` (deux champs seulement : US-003 CA-6) ; un `200` dont le corps porte un `token` texte non vide donne un login réussi, et ce token est celui du premier `auth` | `test_login_serialization` (`SerializesCredentialsIntoLoginRequestBody`) ; `test_startup` (`ReturnsTheTokenWithoutWaitingWhenTheLoginSucceedsImmediately`) ; `test_login_deserialization` (`DeserializesTokenFromLoginResponseBody`) | Implémenté et testé |
| CA-4 | logique | Le login échoue sans être refusé : réseau injoignable, `400`, `429`, `5xx`, ou `200` sans token exploitable | Aucun arrêt : le buzzer attend 5 s puis redemande, **sans plafond ni recul**, tant que le login n'aboutit pas ni n'est refusé. Il ne s'élimine jamais pendant cette phase (CA-10). Un `400` ne se corrige pas en réessayant : constaté, pas traité (décision #48) | `test_startup` (`WaitsTheRetryDelayThenTriesAgainWhenTheLoginFails`, `KeepsTryingAsLongAsTheLoginFails`, `TreatsASuccessWithoutTokenAsAFailure`) | Implémenté et testé ; classification des statuts : voir CA-8 |
| CA-5 | logique | Le hub refuse le login (`401`, identifiants invalides : US-003 CA-8) | Aucun réessai, que le `401` soit le premier résultat ou vienne après des échecs : `setup()` n'ouvre pas le WebSocket, affiche l'écran « Connexion refusée » (avec le bandeau de l'init, un anneau et un cadenas ; contenu et rendu : US-FW-015 CA-9, décision #58) puis `loop()` ne fait plus rien (`startupHalted`) : plus aucun appui, plus aucune lecture du réseau, jusqu'au redémarrage | `test_startup` (`StopsAtOnceWhenTheCredentialsAreRefused`, `StopsWhenTheCredentialsAreRefusedAfterAFailure`, `LoginOutcomeDisplayTest`, `LoginOutcomeWithoutRefusalTest`) ; arrêt de `loop()` : constat | Implémenté ; testé pour l'acquisition et l'écran, arrêt **non testé** ; rendu sur la dalle jamais constaté (décision #42) |
| CA-6 | logique | Les identifiants du buzzer contiennent un guillemet `"` ou une barre oblique inverse `\` | Le corps du login reste un JSON valide : les deux caractères sont échappés (`\"`, `\\`) et le hub reçoit les identifiants tels que saisis, dans `username` comme dans `password`. Sans échappement, un `"` donnerait un JSON invalide, un `400` du hub (US-003 CA-11) et un réessai à vie (CA-4). Les autres caractères de contrôle JSON (retour à la ligne, tabulation) ne sont pas traités : le CDC se tait, rien n'est décidé | `test_login_serialization` (`EscapesQuotesInCredentials`, `EscapesBackslashesInCredentials`) ; mutations : retrait de l'échappement du `"` puis de celui du `\`, chacune rattrapée par son test | Implémenté et testé |
| CA-7 | logique | Le hub répond `200` avec un corps qui n'est pas un objet portant un `token` texte | Le login est un échec réessayé (CA-4) : texte vide, texte quelconque, `[]`, `null`, `{}`, `token` absent, `null`, nombre, booléen ou texte vide. Un corps tronqué **après** un token complet (`{"token":"abc"`) ou suivi de texte parasite est lu quand même et donne ce token ; un corps tronqué **dans** le token donne un échec. Le hub envoie des corps complets : non constaté | `test_login_deserialization` : cas nominal seulement. Sonde du 2026-11-22 (programme jeté, hors dépôt, ArduinoJson 7.4.3) : quatorze entrées, résultat reporté dans ce CA | Implémenté ; cas d'échec **non testés** |
| CA-8 | logique | L'adaptateur reçoit la réponse HTTP du login | `200` : login réussi, token lu (CA-7) ; `401` : refus (CA-5) ; tout autre statut, y compris un code d'erreur de transport (négatif), et un `200` sans token : échec réessayé (CA-4). Le login est un appel bloquant dont le délai est celui, non vérifié, du framework | constat (`HttpTokenFetcher::Fetch`, `src/`) ; `200` constaté au smoke test du 2026-10-31 (dette « `200` sans token observé une fois »), `401` jamais constaté | Implémenté, **non testé** |
| CA-9 | logique | Le WebSocket s'ouvre | L'événement « connexion établie » déclenche `auth` avec un **token frais** : un nouveau login est demandé à chaque ouverture, la première comprise (deux logins au démarrage, décision #48). Un seul essai : si ce login échoue, ou ne rend pas de token, `auth` part avec le dernier token connu (celui du login de `setup()` au premier démarrage). Le format de `auth` est US-FW-004 CA-9 | `test_session` (`SendsAuthCarryingAFreshlyFetchedToken`, `FetchesAFreshTokenOnEveryAuth`, `SendsAuthCarryingTheLastKnownTokenWhenTheFetchFails`, `SendsAuthCarryingTheLastKnownTokenWhenTheFetchReturnsNoToken`) ; `test_connection` (`EstablishedConnectionTriggersAuth`, `ReconnectionSendsAFreshTokenAfterTheHubConfirmedTheAuthentication`) | Implémenté et testé |
| CA-10 | logique | Le buzzer reçoit `auth_success` | Fin de l'init : le compteur d'élimination est armé et remis à zéro (US-FW-006 le consomme). **Avant** ce message, une connexion perdue n'est pas comptée : un buzzer qui n'a jamais réussi à se connecter tente indéfiniment sans s'éliminer | `test_reconnection` (`UnarmedPolicySurvivesAFailure`, `AuthSuccessResetsTheFailureCount`) ; `test_connection` (`AuthSuccessMessageArmsThePolicy`) | Implémenté et testé |
| CA-11 | logique | Un appui arrive avant `auth_success` | Sans effet : aucun envoi, aucun feedback, aucun armement. Le buzzer est **Inerte dès la mise sous tension** (c'est son mode initial, `auth_success` ne le change pas, écart avec le CDC §9 qui parle d'une « entrée en Inerte » à `auth_success`) ; pendant `setup()` aucune lecture d'appui n'a lieu, car `loop()` ne tourne pas ; après `setup()`, l'Inerte ignore l'appui (US-FW-002 CA-8) | `test_game` (`InertBuzzerDoesNotSendAnswerOnButtonPress`, `InertBuzzerIsNotArmed`) ; absence de lecture pendant `setup()` : constat | Implémenté ; Inerte testé, `setup()` **non testé** |

## Constats reportés

Rien de ce qui suit n'est un CA : ce sont des constats consignés dans `DEBTS.md`, à rouvrir comme indiqué.

- **Deux logins au démarrage** (décision #48) : CA-9 décrit le comportement tel quel ; le second login est une dette existante (« Un `POST /api/v1/token` synchrone à chaque connexion »).
- **`401` pendant une reconnexion** : `TokenSessionMessageSender::SendAuth` ignore un `InvalidCredentials` et envoie le dernier token ; le CDC se tait. Aucun CA, aucun code (décision #48) ; à rouvrir avec US-FW-006 (recul sur refus du hub, élimination).
- **Message de jeu avant `auth_success`** : décision #47, rien de nouveau ici ; un appui avant `auth_success` est sans effet par l'Inerte (CA-11), pas par un état « authentifié ».
- **Séquence bloquante, sans rendu d'init** : `setup()` garde la main pendant le Wi-Fi et le login (CA-1, CA-2, CA-4) ; le logo et le bandeau clignotant sont à US-FW-010. Dette existante.
- **Adresse du hub en dur, HTTP clair** : `192.168.1.183:3000` figure deux fois dans `main.cpp`, les identifiants partent sans TLS. Dette existante, à rouvrir avec US-FW-022.
- **`HttpTokenFetcher` non testé** : CA-8 repose sur le constat ; un port « client HTTP » dans `lib/` rendrait la correspondance testable si elle gagne une branche. Dette existante.
- **Écart de preuve** : arrêt de `loop()` après `401` (CA-5), absence de lecture d'appui pendant `setup()` (CA-11), cas d'échec de la lecture de la réponse (CA-7).
- **Écart de preuve, CA-6** : la preuve est le corps produit par `SerializeLogin` (test natif, chaîne comparée à la valeur attendue). Aucun hub n'a reçu un identifiant contenant `"` ou `\` : l'envoi de ce corps par `HttpTokenFetcher` (`src/`) et sa lecture par le hub ne sont ni testés ni constatés au smoke test. Les identifiants viennent de `secrets.h` ; la checklist manuelle sur matériel reste à exécuter par Olivier.

## Sources

CDC §9 (séquence, compteur, appui pendant l'init) ; `lib/startup/` (`TokenAcquisition`, `TokenFetcher`, `LoginResult`, `LoginOutcomeDisplay`, `Sleeper`), `lib/provisioning/` (`LoginRequestSerializer`, `LoginResponseDeserializer`, `ServerCredentials`), `lib/session/TokenSessionMessageSender.h`, `lib/connection/ConnectionEventHandler.h`, `lib/reconnection/ReconnectionPolicy.h`, `lib/game/BuzzerBehavior.h`, `src/HttpTokenFetcher.*`, `src/main.cpp` ; `test/test_startup`, `test/test_session`, `test/test_connection`, `test/test_reconnection`, `test/test_login_serialization`, `test/test_login_deserialization`, `test/test_game` ; US-003 (CA-1, CA-6, CA-8, CA-11, CA-13) et US-009 (CA-4 à CA-11, CA-22) du hub (`quiz-live-server`, `docs/specs/`) ; décisions #42, #44 à #48 ; `DEBTS.md` (« `setup()` bloquant pendant l'init », « Seul `401` arrête le login », « Écran de login refusé en texte seul », « `HttpTokenFetcher` non testé », « Un `POST /api/v1/token` synchrone à chaque connexion », « `200` sans token observé une fois », « Adresse du hub en dur »).
