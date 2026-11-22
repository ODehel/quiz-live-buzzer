# US-FW-007 — Garder la session ouverte

- **Couche** : logique
- **Statut** : Partielle, CA-1 à CA-7 rédigés depuis le réel le 2026-11-22 (CA-1 tenu par des tests ; CA-5 et CA-7 implémentés sans test ; CA-2, CA-3, CA-4 et CA-6 non implémentés : `auth_refresh` repart avec l'ancien token, `auth_success` de réponse remplace l'écran). Le hub n'émet pas encore `token_expiring_soon` : chemin mort
- **Dépend du hub** : US-021 (non implémentée au 2026-11-22, `quiz-live-server` à `ef57459`)

## User story

> **En tant que** joueur,
> **quand** la partie dure plus longtemps que la validité du token de mon buzzer,
> **alors** il le renouvelle de lui-même sans couper la connexion, et je ne vois rien changer à l'écran.

## Périmètre

Du premier `auth_success` à la fin de vie du token, sur une connexion ouverte :

1. le hub prévient le buzzer 300 s avant l'expiration (`token_expiring_soon`, US-021 CA-1 à CA-4) ; le buzzer n'a **aucun timer local** et ne lit pas `expires_in` : il obéit au hub (CDC §8.1) ;
2. à ce message, le buzzer redemande un token par un login frais (`POST /api/v1/token`) et l'envoie dans `auth_refresh` ; le hub répond `auth_success` (US-021 CA-10, CA-11) ;
3. sans renouvellement, le hub envoie `token_expired` puis ferme la connexion (`4002`, US-021 CA-6, CA-7) ;
4. aucun de ces messages n'arme ni ne désarme un mode, et le renouvellement ne se voit pas à l'écran.

**Hors périmètre** :

- le token frais redemandé à chaque connexion, reconnexions comprises (`auth`) : US-FW-005 CA-9 (décision #48) ; la dette « Un `POST /api/v1/token` synchrone à chaque connexion » reste inchangée ;
- la fermeture `4002`, son comptage et la reconnexion : US-FW-006 CA-5, CA-6 ; l'arrêt sur `401` à la reconnexion : US-FW-006 CA-11 ;
- le silence de l'Éliminé (aucun `auth_refresh`, aucun login) : US-FW-006 CA-9, CA-10 ;
- le format JSON de `auth_refresh` : US-FW-004 CA-9 ; le rendu d'`auth_success` à la reconnexion (« Connecté », bandeau) : US-FW-010 ;
- l'émission de `token_expiring_soon`, de `token_expired` et le traitement d'`auth_refresh` : US-021 du hub.

## Critères d'acceptance

Tous les CA sont de la couche **logique**. « Preuve » dit ce qui établit le CA (`constat` : lecture du code, sans test natif) ; « Statut » dit où il en est dans le code au 2026-11-22. Les tests ne portent pas de numéro de CA : la correspondance se fait par le nom des tests. Aucun smoke test n'est écrit dans cette session.

| # | Étiquette | Critère | Résultat attendu | Preuve | Statut |
|---|---|---|---|---|---|
| CA-1 | logique | Le buzzer reçoit `token_expiring_soon` | Le message est rendu puis aiguillé vers la session, pas vers le répartiteur : il déclenche le renouvellement (CA-2) et n'a aucun effet sur le mode (un MCQ ou un SPEED armé reste armé, un Inerte reste Inerte) ni sur l'écran (`ScreenUnchanged`) | `test_connection` (`TokenExpiringSoonTriggersAuthRefresh`) ; `test_rendering`, `test_lcd_renderer`, `test_screen_presenter` (écran inchangé pour `token_expiring_soon`) | Implémenté, testé (le déclenchement ; son contenu est CA-2) |
| CA-2 | logique | Le buzzer renouvelle sa session | Un login frais est demandé, puis `{"type":"auth_refresh","token":"<token obtenu>"}` est envoyé avec **ce** token, jamais l'ancien : un token de la connexion en cours a la même `exp`, le hub l'accepterait (US-021 CA-11) et renverrait aussitôt `token_expiring_soon` (CA-1 du hub : délai ≤ 0, envoi immédiat), en boucle jusqu'à `token_expired` | à écrire : `test_session` (login `Success` : `auth_refresh` porte le token frais) ; le test `SendsAuthRefreshCarryingTheUpdatedToken` fige le comportement actuel et sera réécrit | **Non implémenté** : `TokenSessionMessageSender::SendAuthRefresh` n'appelle pas `TokenFetcher` et envoie le dernier token (celui de `SendAuth` à l'ouverture de la connexion) |
| CA-3 | logique | Le login de renouvellement échoue sans être refusé (réseau injoignable, `400`, `429`, `5xx`, ou `200` sans token exploitable) | **Aucun** `auth_refresh` n'est envoyé et rien ne relance le renouvellement : le token expire, le hub envoie `token_expired` puis ferme (`4002`), et le buzzer se reconnecte (US-FW-006). Environ 300 s après le message, au plus. Différence voulue avec `auth` (US-FW-005 CA-9, repli sur le dernier token) : un ancien token reste valable pour ouvrir une connexion, il ne renouvelle rien | à écrire : `test_session` (`Failed` et `Success` sans token : aucun `Write`) | **Non implémenté** : le même `SendAuthRefresh` envoie le dernier token |
| CA-4 | logique | Le login de renouvellement est refusé (`401`, identifiants changés côté hub : US-003 CA-8) | **Aucun** `auth_refresh` n'est envoyé, **rien ne s'arrête** : la connexion reste valable jusqu'à `token_expired` (300 s au plus), le joueur continue de jouer. À la fermeture `4002`, la reconnexion redemande un login, reçoit `401` et s'arrête par US-FW-006 CA-11 (« Connexion refusée », US-FW-015) | à écrire : `test_session` (`InvalidCredentials` : aucun `Write`) ; la suite est US-FW-006 CA-11 | **Non implémenté**, et **dépend de US-FW-006 CA-11, non implémenté lui non plus** : sans lui, la reconnexion renvoie `auth` avec le token expiré, le hub ferme (`4002`) et le buzzer reboucle sans arrêt ni écran |
| CA-5 | logique | Le buzzer reçoit plusieurs `token_expiring_soon` de suite (rafale : le hub envoie immédiatement si `exp - now - 300 ≤ 0`, par exemple avec `JWT_EXPIRATION` ≤ 300) | **Un renouvellement par message**, sans filtre ni état : un login frais puis un `auth_refresh` par message reçu (CA-2). Avec un token frais, l'`exp` suit chaque renouvellement et la boucle ne naît pas côté hub. Coût : un `POST` bloquant par message | constat (`ConnectionEventHandler::OnMessageReceived` : aucune condition sur le type) ; à écrire : `test_connection` (deux messages, deux renouvellements) | Implémenté (un envoi par message), non testé pour une rafale ; le contenu envoyé reste CA-2 |
| CA-6 | logique | Le buzzer reçoit l'`auth_success` qui répond à un `auth_refresh` | Ni l'écran, ni le bandeau, ni le mode ne changent : une question affichée reste affichée avec ses zones, un SPEED armé reste armé. Le compteur d'élimination est remis à 0 (US-FW-006 CA-2) et c'est tout. Seul l'`auth_success` suivant une **ouverture** de connexion s'affiche (« Connecté », US-FW-010) | à écrire : `test_connection` ou `test_lcd_renderer` (`auth_refresh` envoyé, `auth_success` reçu, aucun rendu ; puis `auth_success` d'une nouvelle connexion rendu) | **Non implémenté** : `DisplayState::LcdTextFor` rend « Connecté » et `ScreenPresenter::TopBarFor` le bandeau à **chaque** `auth_success` ; la question en cours serait remplacée par un `TextScreen`, zones masquées, `BuzzerBehavior` restant armé (zones mortes). Distinguer les deux `auth_success` demande un état « `auth_refresh` en attente » (décision #50) |
| CA-7 | logique | Le buzzer reçoit `token_expired` | Rien n'est envoyé, le mode et l'écran ne changent pas : la fermeture `4002` vient du hub (US-021 CA-7) et suit US-FW-006 (perte de la connexion établie, non comptée comme échec : CA-6 du même document). Aucun nouveau login n'est demandé à ce message | constat (`HubMessageDispatcher` ignore le type, `DisplayState` rend une chaîne vide) ; à écrire : `test_connection` (`token_expired` : aucun envoi, aucun changement de mode) | Implémenté, **non testé** : aucun test ne nomme `token_expired` (US-FW-004 CA-5 le liste) |

## Constats reportés

Rien de ce qui suit n'est un CA : ce sont des constats consignés dans `DEBTS.md`, à rouvrir comme indiqué.

- **Hub sans US-021** : au 2026-11-22 (`quiz-live-server` à `ef57459`), `ws-route.ts` ne traite pour un client authentifié que `request_game_state` ; aucun timer, aucun `token_expiring_soon` ni `token_expired`, `auth_refresh` ignoré. CA-1 à CA-7 ne se vérifient pas de bout en bout avant US-021 ; `scripts/fake-hub.ps1` n'émet pas non plus ces messages (non vérifié).
- **`POST` bloquant du renouvellement** : le login se fait dans le callback de réception (`client.loop()` ne tourne pas) ; `HttpTokenFetcher` ne fixe aucun délai (non vérifié). Le hub coupe après 3 pings sans pong, un ping toutes les 30 s (US-015 CA-4, CA-7) : le risque est faible mais non mesuré, et il se répète à chaque message d'une rafale (CA-5).
- **Coupure faute de renouvellement** (CA-3, CA-4) : le message `token_expired` puis la reconnexion perdent le message de jeu d'un mode armé, `game_resumed` ne ré-armant rien (décision #49, constat « Mode armé pendant la coupure »).
- **Durée du token côté hub** : 3600 s par défaut, réglable (`JWT_EXPIRATION`) ; en dessous de 300 s, le hub prévient dès l'`auth_success`. Aucun CA : le comportement du buzzer est le même (CA-5).
- **Écart de preuve** : CA-2 à CA-4 (renouvellement avec token frais, échecs), CA-5 (rafale), CA-6 (écran de l'`auth_success` de réponse), CA-7 (`token_expired`) n'ont aucun test natif ; `HttpTokenFetcher` reste sans test.

## Sources

CDC §8.1 (`token_expiring_soon`, `token_expired`, `auth_refresh`) ; `lib/session/` (`SessionMessageSender`, `TokenSessionMessageSender`), `lib/connection/ConnectionEventHandler.h`, `lib/dispatch/HubMessageDispatcher.h`, `lib/rendering/` (`DisplayState`, `ScreenPresenter`), `lib/startup/` (`LoginResult`, `TokenFetcher`), `src/HttpTokenFetcher.*`, `src/main.cpp` ; `test/test_session`, `test/test_connection`, `test/test_serialization`, `test/test_rendering`, `test/test_lcd_renderer`, `test/test_screen_presenter` ; US-021 (CA-1 à CA-7, CA-10, CA-11), US-009, US-015 (CA-4, CA-7) du hub (`quiz-live-server`, `docs/specs/`), `src/websocket/ws-route.ts` et `src/authentication/jwt-settings` à `ef57459` ; US-FW-004 (CA-4, CA-5, CA-9), US-FW-005 (CA-5, CA-9), US-FW-006 (CA-2, CA-5, CA-6, CA-9 à CA-11) ; décisions #47 à #50 ; `DEBTS.md` (« `auth_refresh` sur `token_expiring_soon` : chemin mort », « Un `POST /api/v1/token` synchrone à chaque connexion », « `401` au login pendant une reconnexion ignoré », « Renouvellement du token : preuves manquantes »).
