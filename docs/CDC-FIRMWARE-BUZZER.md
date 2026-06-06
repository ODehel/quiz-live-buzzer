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
- **Aucune décision** côté buzzer : exécution purement mécanique des commandes d'affichage reçues du serveur (via la table de rendu, §7).
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
| Appui hors famille | Ignoré (ex. buzz pendant MCQ) | Ignoré (ex. bouton A pendant SPEED) |
| Second appui après verrou | Ignoré | Ignoré |

**Test de référence déjà identifié** : « étant donné un mode MCQ armé, quand deux appuis successifs arrivent, alors un seul message est envoyé au serveur » (force l'existence du verrouillage lock-first).

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

Cette logique est un **pur compteur**, totalement ignorant du matériel → **meilleur candidat pour le premier cycle TDD** (zéro abstraction à mocker, juste deux signaux et un compteur).

---

## 6. Synthèse du cycle de vie

```
[Inerte] ←─── (commandes d'affichage du serveur, exécution mécanique)
   │
   ├── serveur arme MCQ ──→ [MCQ armé] ──(appui A-D)──→ lock → feedback local → envoi → [Inerte]
   │
   ├── serveur arme SPEED ─→ [SPEED armé] ─(appui buzz)─→ lock → feedback local → envoi → [Inerte]
   │
   └── 3 échecs reco. consécutifs ──→ [Éliminé] (terminal, sortie = reboot physique uniquement)
```

---

## 7. Table de rendu (commande serveur → affichage)

Le fichier CSV existant (`Quiz_Buzzer_Game_-_Liste_des_états.csv`) contient **26 lignes**. Recadrage acté :

- **24 lignes** ont pour déclencheur « API » : ce ne sont **pas des états comportementaux** mais des **commandes d'affichage** envoyées par le serveur. Le buzzer les exécute mécaniquement (quelles LEDs, quel texte LCD).
- Seules **2 lignes** ont un déclencheur local (`SimpleQuestionPlayerAnsweredState` = Bouton, `SpeedQuestionPlayerAnsweredState` = Buzzer) : elles correspondent aux **modes armés** du §4.
- Plusieurs « états » ne diffèrent que par une **donnée** et non par un comportement (ex. `DisplayAnswerA/B/C/D` → une seule logique paramétrée par la lettre).

**Conséquence (SRP appliqué à la spec)** : séparer deux préoccupations en deux artefacts distincts —
1. la **logique d'état** (§4–5),
2. la **table de rendu** (le CSV, à réorganiser en « commande reçue → configuration LEDs + LCD »).

> **À DÉCIDER** : réorganisation effective du CSV au format « commande → affichage ».

---

## 8. Points non tranchés (À DÉCIDER)

| # | Sujet | Détail |
|---|---|---|
| 1 | **Protocole de messages** | Structure exacte des commandes reçues et des événements envoyés (format, champs, sérialisation). À spécifier quand le TDD l'exigera. |
| 2 | **Table de rendu détaillée** | Réorganisation du CSV en « commande → affichage ». |
| 3 | **Paramétrage NFC** | Frontière avec l'app mobile ; non traité. |
| 4 | **Démarrage / connexion initiale** | Comportement avant le premier « connecté » (l'`InitialState` du CSV évoque logo + LEDs clignotantes), non spécifié. |

---

## 9. Prochaine étape TDD

Premier cycle red/green recommandé : la **logique de reconnexion WiFi** (§5) — pur compteur, zéro mock.

Premier baby-step pressenti : l'état initial du compteur, ou le comportement après un unique échec. À démarrer en supprimant le smoke test `Smoke.ToolchainWorks` et en créant la première vraie assertion dans `test/test_reconnection/`.

Rappel méthode : Claude **ne code pas** la production ; il guide par questions, valide les pas, et laisse Olivier écrire. Baby-steps stricts (tiniest test → tiniest implémentation → refactor). Principes : SOLID, DRY, KISS, YAGNI.
