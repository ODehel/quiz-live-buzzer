# Dettes techniques — quiz-live-buzzer

Source de vérité unique des dettes assumées. Une dette **soldée est supprimée** (git garde l'historique), jamais barrée. Chaque ligne indique, quand il existe, le *pas séparé* qui la solderait. Mis à jour à chaque session, dans le commit de la session.

Inventaire initial dressé le 2026-10-31 à partir du diagnostic du dépôt (lecture statique : le build `esp32` et les tests n'ont pas été relancés pour l'établir).

## src/main.cpp

- **Zones tactiles figées en 2×2** — quatre zones codées en dur (A, B, C, D) pour valider la chaîne tactile au smoke test. La disposition réelle change à chaque écran (CDC §8.2) : le vecteur devra être piloté par l'état de jeu. *Pas séparé* : feature « disposition dynamique des zones », après la table de rendu.
- **Pins du retour local `6/7/8/9` provisoires et en collision avec le bus RGB** — le bus RGB utilise déjà les GPIO 6, 7, 8 et 9 (`pin_d6`, `pin_d7`, `pin_d0`, `pin_d3`). `localFeedback.Begin()` est donc commenté dans `setup()`, et le commentaire renvoie à une « décision ci-dessous » qui n'existe pas. *Pas séparé* : plan de câblage du retour (LED/son), puis décision tracée dans `DECISIONS.md`.
- **Adresse du hub en dur, en HTTP clair** — `192.168.1.183:3000` figure deux fois dans `main.cpp` (construction de `HttpTokenFetcher` et `client.begin`), le login (identifiants compris) part sans TLS. Acceptable sur réseau local fermé, à rouvrir dès que le hub sort du réseau domestique. *Pas séparé* : `refactor(config)` vers `secrets.h` (le modèle `secrets.h.example` ne porte que les quatre identifiants).
- **`setup()` bloquant pendant l'init** — l'attente Wi-Fi (`while (WiFi.status() != WL_CONNECTED) delay(500)`) et l'obtention du token (`TokenAcquisition`, réessai toutes les 5 s) gardent la main : tant que l'init dure, `loop()` ne tourne pas. Le comportement « tente indéfiniment, sans jamais s'éliminer » est celui du CDC §9, et un appui pendant l'init est sans effet (§9) ; la limite est l'absence du rendu d'init (logo, LEDs, §8.2) pendant l'attente. *Pas séparé* : démarrage non bloquant avec le rendu d'init, feature à part.
- **Seul `401` arrête le login** — tout autre échec (réseau, `400`, `429`, `5xx`) est réessayé sans fin toutes les 5 s, sans plafond ni recul. Un `400` (corps de login mal formé) ne se corrigera pourtant pas en réessayant. *Pas séparé* : classer les statuts dans `HttpTokenFetcher` (adaptateur) si le cas se présente.
- **Login refusé : arrêt silencieux** — sur `401`, `setup()` écrit une ligne sur le port série, pose `startupHalted` (globale) et `loop()` ne fait plus rien : aucun message à l'écran. Le buzzer paraît figé sans cause visible. *Pas séparé* : rendu d'un écran d'erreur, avec le rendu d'init.
- **`HttpTokenFetcher` non testé** — la correspondance statut HTTP → `LoginResult` (`200`, `401`, autre) vit dans l'adaptateur de `src/`, validé au smoke test seulement ; seule la syntaxe a été vérifiée, avec des stubs d'Arduino. *Pas séparé* : port « client HTTP » dans `lib/` si la correspondance gagne une branche.
- **Configuration de la dalle (`LGFX`) dans `main.cpp`** — une soixantaine de lignes de bus et de panneau RGB à côté du câblage des objets. *Pas séparé* : `refactor(main)` vers `src/` (ex. `src/Lgfx.h`).
- **Globales de câblage** — tous les objets sont des globales de `main.cpp` (racine de composition sans conteneur), ordre d'initialisation implicite. Assumé tant que le graphe reste lisible.
- **Bus RGB sur des broches à double rôle** — le bus utilise les GPIO 0, 3, 45 et 46, que je lis comme des *strapping pins* de l'ESP32-S3 (à confirmer sur le schéma de la CrowPanel). Un niveau forcé au démarrage pourrait modifier le boot. Dette notée dans les récaps précédents, jamais versionnée. *Pas séparé* : vérification sur le schéma, puis décision.

## src/ (adaptateurs)

- **Accents absents à l'écran** — problème de police, pas d'encodage : il faudra une police Latin-étendu, avec un budget Flash à surveiller (l'env `esp32` occupe environ 76 % de la Flash d'après le dernier relevé de session, non remesuré). *Pas séparé* : feature « police Latin-étendu ».
- **Erreurs GPIO à chaque démarrage (`TAMC_GT911`)** — `Gt911TouchReader` construit la bibliothèque avec les broches `int` et `rst` à `static_cast<uint8_t>(-1)` (255), puisque le vrai reset passe par l'expandeur PCA9557 ; `ts.begin()` appelle pourtant `reset()`, qui pilote ces broches : trois `Invalid pin selected` et cinq `gpio_set_level ... error` sur le port série (relevé au smoke test du 2026-10-31, décompte identique à la lecture de `reset()` sur la branche principale de la bibliothèque, non vérifiée sur la `1.0.2`). Aucun effet observé. *Pas séparé* : initialiser le GT911 sans passer par ce `reset()`, à étudier avec les smoke tests tactiles.
- **`200` sans token observé une fois** — premier login du smoke test du 2026-10-31 : le hub a répondu `200`, le buzzer n'a pas obtenu de token et a réessayé 5 s plus tard (le second login a abouti). Cause non établie (corps de réponse vide ou illisible). Le réessai l'absorbe ; `HttpTokenFetcher` écrit maintenant la taille du corps sur le port série. *Pas séparé* : à reprendre si la trace révèle un corps non vide.
- **Adaptateurs validés par smoke test seulement** — `src/` n'est couvert par aucun test natif (frontière voulue : sans état, sans branche, erreur visible au smoke test). À rouvrir si un adaptateur gagne une branche.

## lib/

- **Paramétrage NFC non spécifié** — seul point ouvert du CDC (§10) ; frontière avec l'application mobile à définir. *Pas séparé* : session de cadrage du CDC §10, avant tout code.

- **Renouvellement du token non cadré** — sur `token_expiring_soon`, `SendAuthRefresh()` renvoie le token **courant** (celui du login), alors que la spec du hub (US-021 CA-10) attend un **nouveau JWT** sur `auth_refresh`. Le firmware ne refait jamais `POST /api/v1/token` après le démarrage, et le JWT dure 3600 s par défaut (`JWT_EXPIRATION_TIME`) : une reconnexion après expiration enverrait un token périmé (`4002`, US-009 CA-8). US-021 est spécifiée mais je n'en ai pas trouvé trace dans `ws-route.ts` du hub : le chemin n'est pas éprouvable sur le matériel pour l'instant. *Pas séparé* : session de cadrage (qui redemande un JWT, et quand), puis `fix`.
- **Reconnexion WebSocket sans recul sur refus du hub** — après une fermeture `4001`, le client rouvre une connexion environ toutes les 1,2 s (relevé du 2026-10-31, avant le correctif du token) jusqu'à la limite du hub : 15 par minute, puis `4006`. Le CDC ne dit rien du comportement sur un code de fermeture. *Pas séparé* : cadrage avec le renouvellement du token.

## build/ et CI

- **Env `esp32` hors CI** — la CI ne lance que `pio test -e native`. Un build `esp32` cassé ne se voit qu'en local. Choix assumé (le registre PlatformIO et la dépendance LovyanGFX allongent le run). *Pas séparé* : job `esp32` quand LovyanGFX sera épinglée.
- **PlatformIO non épinglé en CI** — `pip install platformio` prend la dernière version : une nouvelle version peut casser la CI sans changement de code. *Pas séparé* : épingler après un run vert connu.
- **Runner et actions non épinglés** — `ubuntu-latest` (qui migre vers Ubuntu 26) et les actions par tag majeur (`checkout@v5`, `setup-python@v6`, `cache@v5`, premières versions Node 24, non les dernières). *Pas séparé* : `chore(ci)` d'épinglage, ou Dependabot.
- **Dépendances flottantes** — LovyanGFX est tirée depuis `https://github.com/lovyan03/LovyanGFX.git` sans révision ; `ArduinoJson@^7.4.3` et `WebSockets@^2.7.3` acceptent les mises à jour mineures. Seule `TAMC_GT911` est figée (`1.0.2`). *Pas séparé* : épingler LovyanGFX sur un commit ou un tag.
- **Avertissements du compilateur limités à `-Werror=switch`** — pas de `-Wall -Wextra`. *Pas séparé* : `chore(build)` d'activation, puis une session de corrections si le volume l'exige.
- **Aucune mesure de couverture** — les 76 tests natifs ne sont pas rapportés à un taux de lignes ou de branches. *Pas séparé* : `chore(ci)` couverture (gcov/lcov) avec seuil.

## outillage

- **Ni format ni lint** — pas de `.clang-format`, `.clang-tidy`, `.editorconfig` ni `.gitattributes` (le hub en a). Le style existant est Allman. Un `clang-format` en style LLVM par défaut ferait diverger les 69 fichiers : le style est à déduire du code. *Pas séparé* : `chore(format)`, puis `chore(lint)` une famille de règles par session.
- **Pas de hook de commit** — le hub a un `pre-commit` qui refuse un fichier sans retour à la ligne final. *Pas séparé* : à décider avec le format.

## docs/

- **README vide** (0 octet). *Pas séparé* : `docs(readme)` — description, build, tests, secrets.
- **CDC §0 et §11 périmés** — §0 présente le document comme « établi avant implémentation » avec un smoke test vert, §11 propose un premier cycle de reconnexion déjà réalisé et demande de supprimer `Smoke.ToolchainWorks`. À réécrire en état courant.
- **Méthode centralisée en retard** — `CLAUDE.md` et `docs/method/WORKFLOW.md` de `quiz-buzzer-docs` décrivent encore le régime sans pull request (« Claude ne fait jamais `git push` », commit direct puis fast-forward de `main`). Ce dépôt travaille désormais par pull request, relue et fusionnée en `squash` par Olivier. *Pas séparé* : session dans `quiz-buzzer-docs`.
