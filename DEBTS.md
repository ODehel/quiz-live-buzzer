# Dettes techniques — quiz-live-buzzer

Source de vérité unique des dettes assumées. Une dette **soldée est supprimée** (git garde l'historique), jamais barrée. Chaque ligne indique, quand il existe, le *pas séparé* qui la solderait. Mis à jour à chaque session, dans le commit de la session.

Inventaire initial dressé le 2026-10-31 à partir du diagnostic du dépôt (lecture statique : le build `esp32` et les tests n'ont pas été relancés pour l'établir).

## src/main.cpp

- **Zones tactiles figées en 2×2** — quatre zones codées en dur (A, B, C, D) pour valider la chaîne tactile au smoke test. La disposition réelle change à chaque écran (CDC §8.2) : le vecteur devra être piloté par l'état de jeu. *Pas séparé* : feature « disposition dynamique des zones », après la table de rendu.
- **Pins du retour local `6/7/8/9` provisoires et en collision avec le bus RGB** — le bus RGB utilise déjà les GPIO 6, 7, 8 et 9 (`pin_d6`, `pin_d7`, `pin_d0`, `pin_d3`). `localFeedback.Begin()` est donc commenté dans `setup()`, et le commentaire renvoie à une « décision ci-dessous » qui n'existe pas. *Pas séparé* : plan de câblage du retour (LED/son), puis décision tracée dans `DECISIONS.md`.
- **Adresse du hub en dur, en HTTP clair** — `192.168.1.183:3000` figure deux fois (`fetchToken` et `client.begin`), le login (identifiants compris) part sans TLS. Acceptable sur réseau local fermé, à rouvrir dès que le hub sort du réseau domestique. *Pas séparé* : `refactor(config)` vers `secrets.h` (le modèle `secrets.h.example` ne porte que les quatre identifiants).
- **Attente Wi-Fi sans limite** — `while (WiFi.status() != WL_CONNECTED) delay(500)` ne rend jamais la main : un Wi-Fi absent fige le buzzer, hors de `ReconnectionPolicy` (qui ne gouverne que la reconnexion après un premier `auth_success`). Cf. CDC §9 (démarrage hors machine à états). *Pas séparé* : `fix(startup)`, avec une abstraction testable pour l'attente.
- **Échec du login ignoré** — `fetchToken` renvoie `""` si le statut n'est pas `200`, et `setup()` poursuit : `UpdateToken("")` puis `client.begin(...)`. Seule trace : une ligne sur le port série. *Pas séparé* : même session que la ligne précédente.
- **`fetchToken` non testable** — la logique de login (sérialisation, statut, désérialisation) est mêlée à `HTTPClient`, donc à `src/`. Les deux sérialiseurs sont testés, l'enchaînement non. *Pas séparé* : extraire un port « client HTTP » dans `lib/provisioning/`.
- **Configuration de la dalle (`LGFX`) dans `main.cpp`** — une soixantaine de lignes de bus et de panneau RGB à côté du câblage des objets. *Pas séparé* : `refactor(main)` vers `src/` (ex. `src/Lgfx.h`).
- **Globales de câblage** — tous les objets sont des globales de `main.cpp` (racine de composition sans conteneur), ordre d'initialisation implicite. Assumé tant que le graphe reste lisible.
- **Bus RGB sur des broches à double rôle** — le bus utilise les GPIO 0, 3, 45 et 46, que je lis comme des *strapping pins* de l'ESP32-S3 (à confirmer sur le schéma de la CrowPanel). Un niveau forcé au démarrage pourrait modifier le boot. Dette notée dans les récaps précédents, jamais versionnée. *Pas séparé* : vérification sur le schéma, puis décision.

## src/ (adaptateurs)

- **Accents absents à l'écran** — problème de police, pas d'encodage : il faudra une police Latin-étendu, avec un budget Flash à surveiller (l'env `esp32` occupe environ 76 % de la Flash d'après le dernier relevé de session, non remesuré). *Pas séparé* : feature « police Latin-étendu ».
- **Adaptateurs validés par smoke test seulement** — `src/` n'est couvert par aucun test natif (frontière voulue : sans état, sans branche, erreur visible au smoke test). À rouvrir si un adaptateur gagne une branche.

## lib/

- **Paramétrage NFC non spécifié** — seul point ouvert du CDC (§10) ; frontière avec l'application mobile à définir. *Pas séparé* : session de cadrage du CDC §10, avant tout code.

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
