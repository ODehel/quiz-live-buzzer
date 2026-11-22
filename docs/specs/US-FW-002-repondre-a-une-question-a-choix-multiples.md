# US-FW-002 — Répondre à une question à choix multiples

- **Couche** : logique
- **Statut** : Partielle, CA-1 à CA-9 rédigés depuis le réel le 2026-11-22 (CA-2 non implémenté ; CA-9 testé mais non câblé)
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
| CA-1 | logique | Un buzzer Inerte reçoit `question_choices` | Le buzzer passe en MCQ armé (`IsArmed()` vrai) : un appui A–D devient pertinent. Vaut aussi après un appui ou un `timer_end` (retour en Inerte) : une nouvelle question réarme | tests natifs `QuestionChoicesArmTheBuzzer` et `InertBuzzerIsNotArmed` ; aiguillage : `test_dispatch` (`QuestionChoicesArmsMcq`) | Implémenté et testé ; le réarmement après un appui ou un `timer_end` n'a pas de test |
| CA-2 | logique | Un message d'armement reçu alors que le buzzer est déjà armé (MCQ non répondu ou SPEED non buzzé) | Ignoré : le mode ne change pas. Vaut pour `question_choices`, `question_open` et `buzz_unlocked` ; seul un buzzer Inerte s'arme (CDC §8.1, source « Inerte » seule). Décision #45 | test natif à écrire avec le correctif : armé en SPEED puis `question_choices`, l'appui A–D n'envoie rien ; armé en MCQ puis `question_open`, l'appui rouge n'envoie rien | **Non implémenté** : `OnQuestionChoices`, `OnQuestionOpen` et `OnBuzzUnlocked` écrasent le mode sans condition. Écart consigné en dette (`DEBTS.md`) |
| CA-3 | logique | Appui A–D sur un MCQ armé : le verrou est posé en premier | Le mode repasse en Inerte de façon synchrone, **avant** le feedback local et avant l'envoi. Un appui qui arriverait pendant le feedback tomberait sur un buzzer déjà Inerte | test natif à écrire : un `LocalFeedback` qui rappelle `OnMcqPressed` pendant `AcknowledgeAnswer` ne provoque qu'un seul `SendAnswer` | Implémenté (`mode = Mode::Inert` est la première instruction) ; **non prouvé** : poser le verrou après l'envoi laisserait tous les tests actuels verts |
| CA-4 | logique | Appui A–D sur un MCQ armé : feedback local puis envoi | `LocalFeedback::AcknowledgeAnswer(lettre)` est appelé, puis `HubMessageSender::SendAnswer(lettre)` avec la même lettre (`A`, `B`, `C` ou `D`). Le contenu du feedback est dans US-FW-011, le format de `answer` dans US-FW-004 | tests natifs `ArmedBuzzerAcknowledgesAnswerOnButtonPress` (`C`), `ArmedBuzzerAcknowledgesBeforeSendingAnswer` (`A`, ordre vérifié), `ArmedBuzzerSendsAnswerOnceOnDoubleButtonPress` (`A`) | Implémenté et testé pour `A` et `C` ; `B` et `D` non testées (même code) |
| CA-5 | logique | Un seul appui par armement | Après l'appui, le buzzer est Inerte (`IsArmed()` faux) : un second appui, quelle que soit la zone, n'envoie rien et ne déclenche aucun feedback. Le hub ne reçoit qu'un `answer` par question | tests natifs `ArmedBuzzerSendsAnswerOnceOnDoubleButtonPress`, `AnswerPressDisarmsTheBuzzer` | Implémenté et testé ; second appui sur une autre lettre non testé, feedback du second appui non asserté |
| CA-6 | logique | Appui du bouton rouge pendant un MCQ armé | Ignoré : aucun envoi, aucun feedback, et le buzzer **reste armé** (un appui A–D ensuite est pris). Les deux familles se distinguent par la méthode appelée (`OnMcqPressed` ou `OnBuzzPressed`) | test natif `McqArmedBuzzerIgnoresBuzzPress` | Implémenté ; testé en partie : seul `SendAnswer` est asserté, ni le feedback, ni `SendBuzz`, ni le maintien de l'armement |
| CA-7 | logique | `timer_end` reçu | Le mode armé courant (MCQ ou SPEED) repasse en Inerte : un appui ensuite est sans effet. Reçu en Inerte, il ne change rien au mode. L'affichage de `timer_end` (« Trop tard » seulement s'il y avait un mode armé) est rendu **avant** le dispatch (décision #5) | tests natifs `TimerEndDisarmsMcqArmedBuzzer`, `TimerEndDisarmsBuzzArmedBuzzer`, `TimerEndDisarmsTheBuzzer` ; aiguillage : `test_dispatch` (`TimerEndDisarmsArmedBuzzer`) ; ordre rendu-puis-dispatch : `test_connection` (`TimerEndIsRenderedWithTheModeOfBeforeTheMessage`) | Implémenté et testé ; `timer_end` reçu en Inerte (sans effet) non testé |
| CA-8 | logique | Appui alors que le buzzer est Inerte | Ignoré : aucun envoi, aucun feedback, quelle que soit la famille. Vaut avant tout armement, après un appui et après un `timer_end` | test natif `InertBuzzerDoesNotSendAnswerOnButtonPress` | Implémenté ; testé en partie : seul l'appui A–D est testé, seul `SendAnswer` est asserté ; l'appui du bouton rouge en Inerte est repris par US-FW-003 |
| CA-9 | logique | Appui alors que le buzzer est Éliminé | Ignoré dans tous les modes, même armé : aucun envoi, aucun feedback. Terminal : seul un redémarrage physique en sort (CDC §4.4 et §6) | tests natifs `EliminatedBuzzerIgnoresButtonPress` et `EliminatedBuzzerIgnoresBuzzPress` (armé puis éliminé) | Testé dans `BuzzerBehavior`, **non câblé** : `OnEliminated()` n'a aucun appelant hors tests, et rien ne lit `ReconnectionPolicy::IsEliminated()`. Un buzzer éliminé répond donc encore à un appui. Écart consigné en dette ; câblage dans US-FW-006 |

## Constats reportés

Rien de ce qui suit n'est un CA : ce sont des constats consignés dans `DEBTS.md`, à rouvrir comme indiqué.

- **Zones affichées mais mortes si le mode n'est pas Inerte** : `LcdRenderer` rend le message avant son dispatch (décision #5). Avec CA-2, un `question_choices` reçu pendant un SPEED armé montrerait les quatre zones alors que les appuis A–D resteraient ignorés. Si le hub n'envoie pas `timer_end` avant la question suivante, le buzzer reste bloqué sur l'ancien mode. À rouvrir quand le hub émet ces messages (US-011, US-012).
- **Appui pendant une coupure** : un MCQ armé garde son mode pendant une reconnexion. Un appui pose le verrou, déclenche le feedback, puis l'envoi échoue sans que rien ne le sache (le retour de `sendTXT` est ignoré) : la réponse est perdue, le joueur voit « Réponse envoyée ». Le CDC §5 et §6 ne disent rien d'un mode armé pendant la coupure. À rouvrir avec la reprise de connexion côté hub (US-021) et US-FW-006.
- **Éliminé armé** : `IsArmed()` reste vrai pour un buzzer éliminé auquel un message d'armement arrive ; `LcdRenderer` s'en sert pour `timer_end`. Sans effet tant qu'Éliminé n'est pas câblé.
- **Armé sans zone affichée** : déjà en dette (« Buzzer armé sans zone tactile à l'écran »), qui vise `timer_tick` et les `question_choices` aux propositions inexploitables.
- **Écarts de preuve** : verrou avant feedback (CA-3), lettres `B` et `D`, réarmement, `timer_end` en Inerte, assertions de feedback et de bouton rouge (CA-1, CA-4 à CA-8).

## Sources

CDC §3 (règles « ignoré »), §4.1, §4.2, §4.4, §6, §8.1 ; `lib/game/` (`BuzzerBehavior`, `ArmedModeReader`, `LocalFeedback`, `HubMessageSender`), `lib/dispatch/HubMessageDispatcher.h`, `lib/connection/ConnectionEventHandler.h`, `lib/reconnection/ReconnectionPolicy.h`, `src/WebSocketMessageSender.cpp`, `src/main.cpp` ; `test/test_game`, `test/test_dispatch`, `test/test_connection` ; décisions #3, #4, #5, #18, #44 et #45 ; `DEBTS.md`.
