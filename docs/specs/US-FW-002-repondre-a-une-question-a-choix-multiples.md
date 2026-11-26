# US-FW-002 — Répondre à une question à choix multiples

- **Couche** : logique
- **Statut** : Partielle, CA-1 à CA-9 rédigés depuis le réel le 2026-11-22 (CA-1 à CA-9 implémentés et testés ; CA-9 câblé le 2026-11-23 ; CA-10 décidé le 2026-11-26, décision #91, non implémenté)
- **Dépend du hub** : US-011

## User story

> **En tant que** joueur,
> **quand** une question à choix multiples est ouverte et que je touche l'une des quatre zones,
> **alors** le buzzer enregistre ma réponse une seule fois, me le confirme sans attendre le hub et la transmet au hub.

## Périmètre

Mode MCQ armé par `question_choices` : un appui sur une zone A–D pose le verrou immédiatement (lock-first), déclenche le feedback local puis émet `answer { value }`, avec la lettre touchée. Le buzzer revient en Inerte à l'appui ; `timer_end` désarme aussi.

Cette US porte aussi les règles **communes aux deux modes armés**, formulées pour « tout mode armé » (CA-2, CA-7, CA-8, CA-9) : US-FW-003 y renvoie et ne spécifie que ce qui est propre au bouton rouge.

**Hors périmètre** :

- la détection de l'appui et sa traduction en `OnMcqPressed(lettre)` : US-FW-001 ;
- ce que le joueur voit (« Réponse envoyée », zone mise en évidence, zones masquées, « Trop tard »), l'activation des zones tactiles seulement quand elles sont affichées (décision #18) : US-FW-011 ; ici, seul l'appel du feedback local compte ;
- la lecture du type des messages, leur aiguillage vers `BuzzerBehavior` et la sérialisation de `answer` : US-FW-004 ;
- le mode SPEED (`buzz`, `question_open`, `buzz_unlocked`, appui MCQ ignoré en SPEED armé) : US-FW-003 ;
- le passage en Éliminé (compteur d'échecs, câblage vers `BuzzerBehavior`) : US-FW-006 ;
- l'arbitrage du hub sur la réponse reçue (`answer_received`, `question_result`) : US-011 côté hub, rendu dans US-FW-011 et US-FW-013.

## Critères d'acceptance

Tous les CA sont de la couche **logique** : le rendu du feedback est dans US-FW-011. « Preuve » dit ce qui établit le CA ; « Statut » dit où il en est dans le code au 2026-11-22. Les tests ne portent pas de numéro de CA : la correspondance se fait par le nom des tests (fichier `test/test_game/test_game.cpp` sauf indication).

| # | Étiquette | Critère | Résultat attendu | Preuve | Statut |
|---|---|---|---|---|---|
| CA-1 | logique | Un buzzer Inerte reçoit `question_choices` | Le buzzer passe en MCQ armé (`IsArmed()` vrai) : un appui A–D devient pertinent. Vaut aussi après un appui ou un `timer_end` (retour en Inerte) : une nouvelle question réarme | tests natifs `QuestionChoicesArmTheBuzzer`, `InertBuzzerIsNotArmed`, `QuestionChoicesArmsAgainAfterAnAnswer` et `QuestionChoicesArmsAgainAfterTimerEnd` ; aiguillage : `test_dispatch` (`QuestionChoicesArmsMcq`) | Implémenté et testé, réarmement après un appui ou un `timer_end` compris |
| CA-2 | logique | Un message d'armement reçu alors que le buzzer est déjà armé (MCQ non répondu ou SPEED non buzzé) | Ignoré : le mode ne change pas. Vaut pour `question_choices`, `question_open` et `buzz_unlocked` ; seul un buzzer Inerte s'arme (CDC §8.1, source « Inerte » seule). Décision #45 | tests natifs `SpeedArmedBuzzerIgnoresQuestionChoices` (armé en SPEED puis `question_choices`, l'appui A–D n'envoie rien), `McqArmedBuzzerIgnoresQuestionOpen` (armé en MCQ puis `question_open`, l'appui rouge n'envoie rien), `McqArmedBuzzerIgnoresBuzzUnlocked`, et le maintien du mode d'origine : `SpeedArmedBuzzerStaysSpeedArmedOnQuestionChoices`, `McqArmedBuzzerStaysMcqArmedOnQuestionOpen`, `McqArmedBuzzerStaysMcqArmedOnBuzzUnlocked` | Implémenté et testé (`ArmFromInert` : seul un buzzer Inerte s'arme). Le même message reçu dans le mode qu'il arme (MCQ puis `question_choices`, SPEED puis `question_open` ou `buzz_unlocked`) n'a pas de test : le mode ne change pas, donc rien d'observable à distinguer. Le rendu de `question_choices` hors Inerte est celui de US-FW-011 CA-6 (implémenté et testé) ; celui de `question_open` et `buzz_unlocked`, US-FW-012 CA-13 |
| CA-3 | logique | Appui A–D sur un MCQ armé : le verrou est posé en premier | Le mode repasse en Inerte de façon synchrone, **avant** le feedback local et avant l'envoi. Un appui qui arriverait pendant le feedback tomberait sur un buzzer déjà Inerte | test natif `AnswerLockIsSetBeforeTheLocalFeedback` : un `LocalFeedback` qui rappelle `OnMcqPressed('B')` pendant `AcknowledgeAnswer('A')` ne provoque qu'un seul `SendAnswer` (mutation : verrou posé après l'envoi, rouge) | Implémenté et testé (`mode = Mode::Inert` est la première instruction) |
| CA-4 | logique | Appui A–D sur un MCQ armé : feedback local puis envoi | `LocalFeedback::AcknowledgeAnswer(lettre)` est appelé, puis `HubMessageSender::SendAnswer(lettre)` avec la même lettre (`A`, `B`, `C` ou `D`). Le contenu du feedback est dans US-FW-011, le format de `answer` dans US-FW-004 | tests natifs `ArmedBuzzerAcknowledgesAnswerOnButtonPress` (`C`), `ArmedBuzzerAcknowledgesBeforeSendingAnswer` (`A`, ordre vérifié), `ArmedBuzzerAcknowledgesBeforeSendingAnswerB` et `…D` (ordre vérifié), `ArmedBuzzerSendsAnswerOnceOnDoubleButtonPress` (`A`) | Implémenté et testé pour `A`, `B`, `C` et `D` |
| CA-5 | logique | Un seul appui par armement | Après l'appui, le buzzer est Inerte (`IsArmed()` faux) : un second appui, quelle que soit la zone, n'envoie rien et ne déclenche aucun feedback. Le hub ne reçoit qu'un `answer` par question | tests natifs `ArmedBuzzerSendsAnswerOnceOnDoubleButtonPress`, `AnswerPressDisarmsTheBuzzer`, `SecondPressOnAnotherLetterIsNeitherSentNorAcknowledged` (ni `answer`, ni feedback du second appui) | Implémenté et testé |
| CA-6 | logique | Appui du bouton rouge pendant un MCQ armé | Ignoré : aucun envoi, aucun feedback, et le buzzer **reste armé** (un appui A–D ensuite est pris). Les deux familles se distinguent par la méthode appelée (`OnMcqPressed` ou `OnBuzzPressed`) | tests natifs `McqArmedBuzzerIgnoresBuzzPress` (ni `answer`, ni `buzz`, ni feedback) et `McqArmedBuzzerStaysMcqArmedOnBuzzPress` (`IsArmed()` vrai, puis un appui A–D est pris) | Implémenté et testé |
| CA-7 | logique | `timer_end` reçu | Le mode armé courant (MCQ ou SPEED) repasse en Inerte : un appui ensuite est sans effet. Reçu en Inerte, il ne change rien au mode. L'affichage de `timer_end` (« Trop tard » seulement s'il y avait un mode armé) est rendu **avant** le dispatch (décision #5) | tests natifs `TimerEndDisarmsMcqArmedBuzzer`, `TimerEndDisarmsBuzzArmedBuzzer`, `TimerEndDisarmsTheBuzzer`, `TimerEndWhileInertChangesNothing` ; aiguillage : `test_dispatch` (`TimerEndDisarmsArmedBuzzer`) ; ordre rendu-puis-dispatch : `test_connection` (`TimerEndIsRenderedWithTheModeOfBeforeTheMessage`) | Implémenté et testé, `timer_end` reçu en Inerte compris |
| CA-8 | logique | Appui alors que le buzzer est Inerte | Ignoré : aucun envoi, aucun feedback, quelle que soit la famille. Vaut avant tout armement, après un appui et après un `timer_end` | tests natifs `InertBuzzerDoesNotSendAnswerOnButtonPress`, `FreshInertBuzzerIgnoresEveryPress` (avant tout armement), `InertBuzzerAfterAnAnswerIgnoresEveryPress`, `InertBuzzerAfterABuzzIgnoresEveryPress` et `InertBuzzerAfterTimerEndIgnoresEveryPress` : lettres A–D et bouton rouge, ni envoi, ni feedback, `IsArmed()` faux | Implémenté et testé, bouton rouge compris (aussi US-FW-003 CA-7, sauf après `buzz_locked`) |
| CA-9 | logique | Appui alors que le buzzer est Éliminé | Ignoré dans tous les modes, même armé : aucun envoi, aucun feedback. Terminal : seul un redémarrage physique en sort (CDC §4.4 et §6) | tests natifs `EliminatedBuzzerIgnoresButtonPress` et `EliminatedBuzzerIgnoresBuzzPress` (armé puis éliminé) ; de bout en bout : `test_connection` (`McqArmedBuzzerIgnoresAPressOnceEliminated`, `SpeedArmedBuzzerIgnoresAPressOnceEliminated`) | Implémenté et testé : `ConnectionEventHandler::OnConnectionLost` appelle `OnEliminated()` (via `HubMessageDispatcher`) au passage en Éliminé de `ReconnectionPolicy` ; `OnEliminated()` désarme le mode, `IsArmed()` devient faux |
| CA-10 | logique | Appui A–D sur un MCQ armé, et l'envoi de `answer` échoue : `HubMessageSender::SendAnswer` signale que le message n'est pas parti (retour de `sendTXT` faux), hors d'une coupure détectée | Le buzzer se réarme en MCQ armé (`IsArmed()` vrai) et l'écran de la question revient tel qu'avant l'appui (zones actives, aucune zone sélectionnée, aucun texte d'échec) : le joueur peut rappuyer. Aucun réessai automatique. Sans effet sur l'écran de coupure ni sur le compteur d'élimination (US-FW-015, US-FW-006). Le verrou reste posé en premier (CA-3) jusqu'au retour de l'envoi ; un envoi réussi laisse le buzzer Inerte (CA-5). Même règle pour SPEED : US-FW-003 CA-11. Décision #91 | aucune : à écrire par un `fix(game)` | Décidé, non implémenté |

## Constats reportés

Rien de ce qui suit n'est un CA : ce sont des constats consignés dans `DEBTS.md`, à rouvrir comme indiqué.

- **Zones affichées mais mortes si le mode n'est pas Inerte** : soldé pour `question_choices` par `fix(rendering): keep the screen unchanged when an armed buzzer receives question_choices` (US-FW-011 CA-6, décision #70), puis pour `question_open` et `buzz_unlocked` par `fix(rendering): keep the screen unchanged when an armed buzzer receives question_open or buzz_unlocked` (US-FW-012 CA-13, décision #71) : `LcdRenderer` lit l'état armé avant l'aiguillage (décision #5) et un buzzer déjà armé laisse son écran inchangé, « zones affichées = zones actives ». Plus aucun message d'armement n'a d'écran quand le mode l'ignore. Si le hub n'envoie pas `timer_end` avant la question suivante, le buzzer reste bloqué sur l'ancien mode, sur l'écran précédent (le mode ne change plus : CA-2 est livré). À rouvrir quand le hub émet ces messages (US-011, US-012).
- **Appui pendant une coupure** : tranché. Pendant une coupure détectée, les appuis sont ignorés et le mode conservé (US-FW-015 CA-4 et CA-7, décision #58). Hors coupure détectée, un envoi qui échoue réarme le buzzer sans rien afficher (CA-10, décision #91). Reste, hors de ce périmètre, le mode armé sans question à l'écran après la reconnexion (US-FW-006, US-021).
- **Armé sans zone affichée** : déjà en dette (« Buzzer armé sans zone tactile à l'écran »), qui vise les `question_choices` aux propositions inexploitables (le cas `timer_tick` est soldé : décision #72, US-FW-011 CA-8).

## Sources

CDC §3 (règles « ignoré »), §4.1, §4.2, §4.4, §6, §8.1 ; `lib/game/` (`BuzzerBehavior`, `ArmedModeReader`, `LocalFeedback`, `HubMessageSender`), `lib/dispatch/HubMessageDispatcher.h`, `lib/connection/ConnectionEventHandler.h`, `lib/reconnection/ReconnectionPolicy.h`, `src/WebSocketMessageSender.cpp`, `src/main.cpp` ; `test/test_game`, `test/test_dispatch`, `test/test_connection` ; décisions #3, #4, #5, #18, #44 et #45 ; `DEBTS.md`.
