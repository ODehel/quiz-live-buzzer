# Cahier des charges — Firmware Buzzer (ESP32-S3)

> **Statut** : Spécification de conception, établie avant implémentation.
> **Projet** : Buzz'O'Live — application embarquée PlatformIO / ESP32-S3.
> **Méthode** : décisions prises par questionnement socratique. Ce document ne consigne **que** les décisions tranchées. Les points non tranchés sont marqués **« À DÉCIDER »**.

---

## 0. Note de passation (pour une nouvelle session)

Ce document sert à reprendre le travail sans refaire le cadrage. Contexte de la démarche :

- L'utilisateur (Olivier) a un **background C#/.NET orienté objet** ; les analogies .NET sont les bienvenues.
- La démarche est un **coaching TDD socratique** : Claude pose des questions et ne rédige pas le code de production sans demande explicite. La rédaction de cette **spécification** (pas du code) a été faite par Claude à la demande explicite d'Olivier, son but étant de cadrer le besoin technique et métier **avant** toute implémentation.
- Stack de test : **C++ avec GoogleTest**, exécuté dans l'environnement **`native`** de PlatformIO (compilation et exécution sur PC, pas sur la puce). Environnement déjà fonctionnel (smoke test vert).
- Frontière architecturale fondamentale : **logique pure dans `lib/`** (testable en natif, zéro dépendance matérielle), **adaptateurs matériels dans `src/`** (derrière des abstractions = classes C++ abstraites à méthodes virtuelles pures, équivalent des interfaces .NET ; doublures via gMock).

---

## 1. Principe directeur

Le buzzer est un **exécutant piloté par le serveur** (le hub Node.js est le chef d'orchestre). Le buzzer **n'initie rien de lui-même**, à trois exceptions près, toutes **locales** :

1. Le **feedback visuel immédiat** lors d'un appui armé (évite la latence réseau).
2. L'**auto-verrouillage** après un appui armé.
3. Le passage en mode **Éliminé** après 3 échecs consécutifs de reconnexion WiFi.

---

## 2. Matériel par buzzer

| Composant | Rôle |
|---|---|
| Écran LCD | Affichage piloté par le serveur (question, réponses, classement, etc.) |
| 4 boutons-poussoir (A, B, C, D) | Réponse en mode MCQ |
| 1 gros bouton « buzz » | Réponse en mode SPEED |
| Haut-parleurs | Restitution sonore (sons préchargés) |
| WiFi / WebSocket | Communication avec le serveur (réseau local uniquement) |
| Module NFC | Paramétrage du buzzer via l'app mobile *(hors périmètre de cette session — voir §8)* |

---

## 3. Mécanique générale d'appui

Décision de conception centrale (DRY) : il existe **une seule** écoute d'appui, permanente. Ce n'est pas chaque mode qui décide d'écouter ou non — c'est **l'état courant** qui décide si un appui détecté produit un effet.

- La détection physique (GPIO, anti-rebond) vit dans `src/`, derrière une abstraction.
- La **décision** d'agir vit dans `lib/`, testable en natif.
- Signature pressentie côté logique : quelque chose comme `OnButtonPressed(famille, valeur)`.

Un appui porte **deux informations distinctes** à deux usages distincts :

| Information | Usage |
|---|---|
| **Famille** (A–D, ou buzz) | Déterminer si l'appui est **pertinent** dans le mode courant |
| **Valeur** (la lettre précise, ou rien pour le buzz) | Renseigner le **message envoyé au serveur** |

Règles transverses :

- Appui d'une **famille non armée** dans le mode courant → **ignoré** (rien envoyé, rien affiché).
- Message serveur **non reconnu / invalide / hors contexte** → **ignoré, le buzzer ne fait rien du tout** (YAGNI : pas de remontée d'erreur, pas de log — aucun besoin documenté aujourd'hui).

---

## 4. Modes comportementaux

Le buzzer possède **4 modes** (et non les 26 « états » du premier brouillon CSV — voir §7). Un mode est défini par le critère : *un même appui y produit un effet différent*.

### 4.1 Mode Inerte

- Mode par défaut, le plus fréquent.
- **Aucune décision** côté buzzer : exécution purement mécanique des commandes d'affichage reçues du serveur (via la table de rendu, §8.2).
- Tout appui (bouton ou buzz) est **détecté mais écarté** : aucun effet, aucun envoi.

### 4.2 & 4.3 Modes armés (MCQ et SPEED) — un patron commun

Les deux modes armés sont **deux variantes d'un même patron**. Patron commun :

1. Le **serveur** arme le mode (transition entrante distante).
2. Un appui de la **bonne famille** arrive.
3. **Lock-first** : le verrou est posé **immédiatement et synchroniquement**, *avant* toute autre opération. → garantit qu'un second appui ultra-rapide tombe sur un mode déjà inerte et soit ignoré.
4. **Feedback local immédiat** (illumination locale, sans attendre le serveur).
5. **Envoi du message** au serveur.
6. Retour à l'état inerte localement (auto-verrouillé).
7. Le **serveur** déverrouille / change de phase plus tard (transition sortante distante).

| Aspect | Variante MCQ | Variante SPEED |
|---|---|---|
| Famille armée | Boutons A–D | Gros bouton buzz |
| Valeur transmise | La lettre choisie (A/B/C/D) | Aucune (le buzz n'a pas de variante) |
| Message émis | `answer { value }` | `buzz` |
| Appui hors famille | Ignoré (ex. buzz pendant MCQ) | Ignoré (ex. bouton A pendant SPEED) |
| Second appui après verrou | Ignoré | Ignoré |
| Chemins d'armement | `question_choices` | `question_open`, `buzz_unlocked` |
| Désarmement sans appui | `timer_end` | `timer_end` |

**Test de référence déjà identifié** : « étant donné un mode MCQ armé, quand deux appuis successifs arrivent, alors un seul message est envoyé au serveur » (force l'existence du verrouillage lock-first).

> **Note SPEED réentrant** : le mode SPEED peut être ré-armé plusieurs fois sur une même question. Après un buzze invalidé par le maître, le serveur renvoie `buzz_unlocked` aux buzzers non invalidés (US-012), ce qui les ré-arme (Inerte → SPEED armé). Le cycle `SPEED armé → Inerte → SPEED armé` peut donc se répéter. Les deux chemins d'armement (`question_open`, `buzz_unlocked`) produisent **la même transition** — un seul comportement, deux déclencheurs.

### 4.4 Mode Éliminé

- Atteint **de la propre initiative du buzzer**, après **3 échecs consécutifs** de reconnexion WiFi (voir §5).
- **Terminal et non récupérable** : aucun appui, aucun message serveur ne l'en sort. Seul un **redémarrage physique** du buzzer réinitialise.
- Spécification comportementale : rien ne produit d'effet. (KISS : aucune transition sortante à spécifier.)

---

## 5. Règle de reconnexion WiFi

Règle issue du `VISION.md` (« tentatives 1 à 3 : reconnexion auto ; après 3 échecs : perte définitive → éliminé »), précisée ainsi :

- La **temporisation** et le **déclenchement** des tentatives sont **délégués à la couche réseau** (`src/`), pas à la logique métier. La logique testable **ne sait rien** du WiFi réel.
- La logique reçoit exactement **deux signaux entrants**, qui suffisent :
  - **Échec d'une tentative** de reconnexion → incrémente le compteur.
  - **Succès d'une tentative** → remet le compteur à **0**.
- Le compteur compte les **échecs consécutifs**. La **perte initiale** de connexion ne compte pas comme un échec : ce sont les **tentatives de reconnexion infructueuses** qui sont comptées.
- Au **3ᵉ échec consécutif exactement** → bascule en mode **Éliminé** (terminal).

### Complément — Armement du compteur au premier `auth_success`

Le compteur d'échecs consécutifs est **inactif tant que le buzzer n'a pas reçu son premier `auth_success`**. La phase d'initialisation (mise sous tension → WiFi → WebSocket → première authentification, voir §10) n'est **pas** soumise au compteur : un buzzer qui n'a jamais été connecté ne peut pas être éliminé. Le compteur est armé (et mis à 0) au premier `auth_success`, puis suit la règle ci-dessus : échec de reconnexion → +1 ; succès → remise à 0 ; 3 échecs consécutifs → Éliminé.

> Conséquence pour le TDD : la logique de reconnexion a **trois** entrées et non deux — un signal « connexion réussie » qui arme (et remet à 0), un signal « échec » qui n'incrémente **que si le compteur est armé**, et le seuil (3) qui déclenche Éliminé. Le premier test devient « un compteur non armé ignore les échecs » avant « 3 échecs → éliminé ».

Cette logique est un **pur compteur**, totalement ignorant du matériel → **meilleur candidat pour le premier cycle TDD** (zéro abstraction à mocker, juste des signaux et un compteur).

---

## 6. Synthèse du cycle de vie — Table de transitions

La logique d'état du buzzer se réduit à cette table. Chaque ligne est un comportement testable (`lib/`, sans dépendance matérielle). Les colonnes « Action locale » et « Émission » ne concernent que les transitions ; l'affichage associé relève de la table de rendu (§8.2).

| État source | Déclencheur | État cible | Action locale | Émission |
|---|---|---|---|---|
| Inerte | `question_choices` | MCQ armé | — | — |
| Inerte | `question_open` | SPEED armé | — | — |
| Inerte | `buzz_unlocked` | SPEED armé | — | — |
| MCQ armé | appui A–D | Inerte | lock-first → feedback local | `answer {value}` |
| MCQ armé | `timer_end` | Inerte | — | — |
| MCQ armé | appui buzz | MCQ armé | ignoré (hors famille) | — |
| SPEED armé | appui buzz | Inerte | lock-first → feedback local | `buzz` |
| SPEED armé | `timer_end` | Inerte | — | — |
| SPEED armé | appui A–D | SPEED armé | ignoré (hors famille) | — |
| *(tout mode armé)* | second appui après lock | inchangé | ignoré (déjà verrouillé) | — |
| Inerte | tout appui | Inerte | ignoré | — |
| Inerte | 3ᵉ échec reco. WiFi consécutif | Éliminé | — | — |
| *(tout sauf Éliminé)* | 3ᵉ échec reco. WiFi consécutif | Éliminé | — | — |
| Éliminé | tout | Éliminé | ignoré (terminal) | — |

Règles transverses (rappel) :

- **Armement** : un message arme un mode ssi un appui devient pertinent après réception (§8.1).
- **Désarmement par expiration** : `timer_end` désarme le mode armé courant s'il en existe un ; sinon affichage seul.
- **Lock-first** : le verrou est posé synchroniquement *avant* feedback et émission, garantissant qu'un second appui ultra-rapide tombe sur un buzzer déjà inerte.
- **Élimination** : atteinte par la seule initiative du buzzer (3 échecs WiFi consécutifs, §5), terminale, sortie par reboot physique uniquement.
- Tout message non listé ou hors contexte est ignoré silencieusement (§3).

---

## 7. Recadrage du CSV (origine de la spécification)

Le fichier CSV existant (`Quiz_Buzzer_Game_-_Liste_des_états.csv`) contient **26 lignes**. Recadrage acté :

- **24 lignes** ont pour déclencheur « API » : ce ne sont **pas des états comportementaux** mais des **commandes d'affichage** envoyées par le serveur. Le buzzer les exécute mécaniquement (quelles LEDs, quel texte LCD).
- Seules **2 lignes** ont un déclencheur local (`SimpleQuestionPlayerAnsweredState` = Bouton, `SpeedQuestionPlayerAnsweredState` = Buzzer) : elles correspondent aux **modes armés** du §4.
- Plusieurs « états » ne diffèrent que par une **donnée** et non par un comportement (ex. `DisplayAnswerA/B/C/D` → une seule logique paramétrée par la lettre).

**Conséquence (SRP appliqué à la spec)** : deux préoccupations en deux artefacts distincts —
1. la **logique d'état** (§4–6) ;
2. la **table de rendu** (§8.2 : « message reçu → configuration LEDs + LCD »).

> ⚠️ **Écart CSV vs hub réel acté** : le CSV décrivait un affichage **progressif** des réponses MCQ (`DisplayAnswerA/B/C/D`, une par une). Le contrat hub réel (US-011) envoie les 4 propositions **simultanément** dans `question_choices`. L'affichage progressif est donc **abandonné** : les 4 LEDs passent à leur état cible en une fois.

---

## 8. Protocole de messages et table de rendu

Les quatre points historiquement « À DÉCIDER » du §8 ont été tranchés (voir historique en fin de section). Le protocole est **dérivé** du contrat serveur défini dans les US-009 à US-021 du hub : le firmware ne le définit pas, il s'y conforme.

### 8.1 Protocole de messages WebSocket

> Transport : WebSocket sur `ws://<ip>:<port>/ws`. Sérialisation : JSON (messages texte). Le buzzer s'authentifie par JWT obtenu via `POST /api/v1/token`, transmis en premier message ; le serveur résout lui-même `sub → participant` (US-010), le firmware n'a donc **pas** connaissance de son numéro de participant.

**Principe directeur de la table** : un message **arme** un mode ssi un appui devient pertinent après réception. Un message **désarme** s'il rend un appui de nouveau sans effet. Tout le reste est affichage, son, session ou accusé — sans effet sur la logique d'état (`lib/`). Règle unifiée du désarmement par expiration : `timer_end` désarme le mode armé courant s'il en existe un ; sinon il est affichage seul.

#### Messages reçus (hub → buzzer)

| Message | Mode(s) pertinents | Effet sur le mode | Émission vers le hub | Catégorie |
|---|---|---|---|---|
| `auth_success` | tout | aucun | — | Session |
| `token_expiring_soon` | tout | aucun | `auth_refresh` (renouvellement JWT) | Session |
| `token_expired` | tout | aucun (la connexion est fermée par le serveur) | — | Session |
| `game_resumed` | tout | aucun (resynchro d'affichage) | — | Session |
| `question_title` | Inerte | aucun (affichage seul, reste Inerte) | — | Affichage |
| `question_choices` | Inerte | **Inerte → MCQ armé** | — | Armement |
| `question_open` | Inerte | **Inerte → SPEED armé** | — | Armement |
| `timer_tick` | tout | aucun | — | Affichage |
| `timer_end` | MCQ armé / SPEED armé | **mode armé courant → Inerte** ; sinon aucun | — | Désarmement |
| `answer_received` | Inerte (post-appui) | aucun (accusé de l'`answer` émis) | — | Accusé |
| `buzz_accepted` | Inerte (post-appui) | aucun (accusé du `buzz` émis) | — | Accusé |
| `buzz_locked` | Inerte | aucun (un autre a buzzé ; ce buzzer est déjà passif) | — | Affichage |
| `buzz_invalidated` | Inerte (post-buzz) | aucun — déjà Inerte ; écran d'élimination (rendu §8.2) | — | Affichage |
| `buzz_unlocked` | Inerte | **Inerte → SPEED armé** (ré-armement ; serveur ne l'envoie qu'aux non-invalidés) | — | Armement |
| `question_result` | Inerte | aucun (affichage du résultat) | — | Affichage |
| `play_sound_url` | tout | aucun | requête HTTP GET du fichier | Son |
| `play_system_sound` | tout | aucun | — | Son |

#### Messages émis (buzzer → hub)

| Message | Déclencheur | Mode requis | Contenu |
|---|---|---|---|
| `auth` | connexion établie | tout | `{ type, token }` |
| `auth_refresh` | réception de `token_expiring_soon` | tout | `{ type, token }` |
| `answer` | appui A–D pertinent (lock-first) | MCQ armé | `{ type, value: "A"\|"B"\|"C"\|"D" }` |
| `buzz` | appui buzz pertinent (lock-first) | SPEED armé | `{ type }` |

### 8.2 Table de rendu (message reçu → affichage)

> Rendu mécanique exécuté à la réception de chaque message, indépendamment de la logique d'état (§6).
> **Vocabulaire LED** : `Toutes éteintes` / `Toutes allumées` / `Toutes clignotantes` / `Clignoter LED(x)` (x = contenu du message).
> **Vocabulaire LCD** : description abstraite du contenu, exemple de libellé entre parenthèses (texte exact = détail d'implémentation).
> Seuls `timer_end` et `question_result` ont un rendu **conditionnel** ; tous les autres messages sont inconditionnels.

| Message | Condition | LED | LCD | Son |
|---|---|---|---|---|
| *(init, §10)* | — | Toutes clignotantes (une par une) | Logo | — |
| `auth_success` | — | Toutes éteintes | Attente (« Connecté ») | — |
| `game_resumed` | — | Toutes éteintes | Attente (« Partie en cours ») | — |
| `question_title` | — | Toutes éteintes | Libellé question (« Question N : … ») | — |
| `question_choices` | — | Toutes allumées | Les 4 propositions A/B/C/D | — |
| `question_open` (SPEED) | — | Toutes éteintes | Titre + chrono | — |
| `timer_tick` | — | inchangé | Chrono (« N s ») | — |
| `timer_end` | mode armé courant | Toutes éteintes | Temps écoulé (« Trop tard ») | — |
| `timer_end` | Inerte | inchangé | inchangé | — |
| `answer_received` | — | Toutes éteintes | Réponse prise en compte (« Réponse enregistrée ») | — |
| `buzz_accepted` | — | Toutes clignotantes | Tu as buzzé (« À toi ! ») | — |
| `buzz_locked` | — | Toutes éteintes | Un autre a buzzé (« Buzzé par X ») | — |
| `buzz_invalidated` | — | Toutes éteintes | Éliminé pour la question (« Raté ») | — |
| `buzz_unlocked` | — | Toutes éteintes | À nouveau possible de buzzer | — |
| `question_result` | `correct_answer` = lettre A–D (MCQ) | Clignoter LED(correct_answer) | Résultat (bonne réponse + réponse joueur) | — |
| `question_result` | sinon (SPEED), `correct: true` | Toutes clignotantes | Résultat (validé) | — |
| `question_result` | sinon (SPEED), `correct: false` | Toutes éteintes | Résultat (raté) | — |
| `play_sound_url` | — | inchangé | inchangé | fichier distant (HTTP GET) |
| `play_system_sound` | — | inchangé | inchangé | son préchargé (selon `sound_id`) |

> **Distinction MCQ/SPEED pour `question_result`** : déduite du **contenu** du message (pas d'un état mémorisé). Si `correct_answer` est une lettre A–D → MCQ (clignoter la LED correspondante) ; sinon → SPEED (toutes clignotantes si `correct: true`, toutes éteintes sinon). Le contrat hub garantit cette discrimination : MCQ renvoie une lettre, SPEED renvoie un texte de réponse.

> **Sons et messages métier** : le hub déclenche les sons système via des messages `play_system_sound` **distincts** des messages métier (US-018 : « message métier d'abord, son système ensuite »). La colonne Son n'est donc renseignée que sur les lignes `play_sound_url` / `play_system_sound` ; les sons associés aux événements (buzz, résultat, timer) arrivent par ces messages séparés.

---

## 9. Démarrage et connexion initiale

> Cette phase vit entièrement dans `src/` (adaptateurs matériels et réseau). Elle est **hors de la machine à états de jeu** (§6) : les 4 modes ne s'appliquent qu'à partir du premier `auth_success`. Aucune logique testable en natif n'y réside, à l'exception de l'armement du compteur de reconnexion (§5), couvert par le premier cycle TDD.

### Séquence d'initialisation

1. Mise sous tension. Affichage du rendu d'init (logo + LEDs clignotantes — voir §8.2).
2. Connexion au réseau WiFi local.
3. Ouverture de la connexion WebSocket sur `ws://<ip>:<port>/ws`.
4. Envoi du message d'authentification `{ "type": "auth", "token": "<JWT>" }`.
5. Réception de `auth_success` → **fin de la phase d'init**, entrée en mode Inerte (la machine à états de jeu démarre).

La séquence est linéaire : chaque étape déclenche la suivante, sans décision métier. La temporisation, les tentatives et la gestion des erreurs réseau sont déléguées à la couche réseau (`src/`), cohérent avec §5.

### Articulation avec le compteur d'élimination (§5)

Le compteur d'échecs consécutifs est **inactif pendant toute cette phase**. Il n'est armé qu'au **premier `auth_success`** (voir complément §5). Conséquence : un buzzer qui n'a jamais réussi à se connecter tente indéfiniment sans jamais s'auto-éliminer ; le rendu d'init reste affiché tant que la connexion n'est pas établie. La logique d'armement est testable et fait partie du premier cycle TDD (compteur de reconnexion).

### Appui pendant l'init

Aucune écoute active de mode n'existe avant l'entrée en Inerte : un appui pendant l'init est sans effet (rien à émettre, pas de connexion).

---

## 10. Points encore ouverts (À DÉCIDER)

| # | Sujet | Détail |
|---|---|---|
| 1 | **Paramétrage NFC** | Frontière avec l'app mobile ; non traité dans cette session. |

> Les anciens points « Protocole de messages », « Table de rendu détaillée », « Démarrage / connexion initiale » et la classification de `buzz_invalidated` / `timer_end` sont désormais **tranchés** (§8.1, §8.2, §9) et ne figurent plus comme ouverts.

---

## 11. Prochaine étape TDD

Premier cycle red/green recommandé : la **logique de reconnexion WiFi** (§5) — pur compteur, zéro mock — enrichie de l'**armement au premier `auth_success`**.

Premiers baby-steps pressentis (dans l'ordre) :
1. Un compteur **non armé** ignore un échec (reste à 0, pas d'élimination).
2. Après un premier `auth_success`, le compteur est armé (et à 0).
3. Un échec sur compteur armé incrémente.
4. Un succès remet à 0.
5. 3 échecs consécutifs → Éliminé.

À démarrer en supprimant le smoke test `Smoke.ToolchainWorks` et en créant la première vraie assertion dans `test/test_reconnection/`.

Rappel méthode : Claude **ne code pas** la production ; il guide par questions, valide les pas, et laisse Olivier écrire. Baby-steps stricts (tiniest test → tiniest implémentation → refactor). Principes : SOLID, DRY, KISS, YAGNI.
