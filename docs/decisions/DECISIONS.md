# Décisions de conception — quiz-live-buzzer

Journal append-only des décisions de conception tranchées en session, avec le fait qui les a forcées. Une décision revue donne une **nouvelle** entrée qui référence l'ancienne, jamais une réécriture. Seules les décisions de *conception* y figurent (contrat, emplacement, responsabilité) ; les leçons de méthode restent dans les récaps.

Format : **Contexte** (le fait qui a forcé le choix) · **Décision** · **Conséquences** (ce que ça engage, dettes créées).

L'en-tête de chaque décision porte le **titre du commit** qui l'a livrée, pas son hash : le titre est connu avant le commit, et `git log --grep` le retrouve.

Les décisions antérieures à ce journal sont consignées dans `docs/CDC-FIRMWARE-BUZZER.md` (machine à états §6, protocole §8.1, rendu §8.2, démarrage §9) ; elles n'y sont pas recopiées.

---

## #1 — docs(decisions): drop the physical A-D buttons

**Contexte** — Le matériel est une CrowPanel 5″ (DIS07050H). D'après le wiki d'Elecrow, un seul GPIO du connecteur est libre sans réserve, `GPIO_D` (38) ; les autres broches exposées servent l'I²C du tactile (19, 20), le haut-parleur I²S (17, 18, 42), l'UART de la console (43, 44) ou la carte SD (10 à 13). Les broches 6, 7, 8 et 9 de `ArduinoLocalFeedback` sont sur le bus RGB de la dalle. Olivier a décidé que le buzzer ne compterait plus qu'un bouton rouge pour les questions de rapidité (SPEED), tout le reste passant par l'écran, hors haut-parleurs. Ce bouton n'a pas de LED intégrée.

**Décision** — Le buzzer a **un seul bouton physique**, le bouton rouge (`ButtonInput::Buzz`, `GPIO_D` 38). Les réponses A, B, C et D du mode MCQ sont des **zones tactiles** de l'écran. Il n'y a ni boutons-poussoir A–D ni LED. Le feedback local immédiat d'un appui armé passe par **l'écran** et, éventuellement, le **haut-parleur** intégré.

**Conséquences** —
- Le CDC est aligné sur cette décision (§1, §2, §3, §4, §6). La colonne LED du §8.2 est signalée caduque ; sa réécriture et celle du rendu d'init (§9) restent à faire.
- Le code des boutons physiques A–D (`PhysicalButton` A à D et leur traduction) et `ArduinoLocalFeedback` (quatre broches) sont à retirer, avec leurs tests, dans un `refactor` dédié.
- La dette « pins 6/7/8/9 du retour local » disparaît avec ce retrait ; la dette « zones tactiles figées en 2×2 » devient centrale puisque le tactile porte désormais toutes les réponses MCQ.

## #2 — docs(rendering): move the LED states to the screen

**Contexte** — La décision #1 supprime les LED du buzzer. La colonne LED du CDC §8.2 exprimait pourtant des états utiles : les propositions actives, la bonne réponse, l'attente. Olivier a validé quatre effets d'écran pour les porter, un bandeau seulement (pas tout l'écran) pour le clignotement, et une durée égale à celle du message qui pose l'effet, « on changera si jamais ».

**Décision** — La colonne LED du §8.2 devient une colonne **effet écran**, au vocabulaire fermé : `Zones masquées`, `Zones actives`, `Mettre en évidence zone(x)` et `Bandeau clignotant`. Un effet dure jusqu'au message suivant qui change l'effet. Un effet d'écran porte aussi l'état des zones tactiles : les zones ne sont tactiles que sous `Zones actives`. Le son du feedback local reste un point ouvert (§10 #2).

**Conséquences** —
- La disposition des zones tactiles devient pilotée par le rendu, et non plus figée en 2×2. La dette « zones tactiles figées » est maintenant bornée par ce contrat.
- `DisplayState` devra porter l'effet d'écran en plus du texte LCD, dans une feature à venir (pas dans la session `refactor` suivante).

## #3 — docs(rendering): define the local feedback on the screen

**Contexte** — Le §8.2 ne décrivait que le rendu déclenché par un message du hub. L'appui armé est un déclencheur local (lock-first, §4.2) : entre l'appui et la réponse du hub, le CDC ne disait rien de l'écran, alors que la décision #1 en fait le support du feedback local. Le lock-first rend les zones inactives dès l'appui.

**Décision** — Deux lignes locales sont ajoutées au tableau du §8.2. Un appui sur la zone x (MCQ armé) donne `Mettre en évidence zone(x)` avec la lettre choisie, et le texte « Réponse envoyée ». Un appui sur le bouton rouge (SPEED armé) donne `Zones masquées` et le texte « Buzz envoyé », sans bandeau clignotant : le hub peut avoir accepté un autre buzzer d'abord, donc le bandeau n'est posé qu'à `buzz_accepted`. Le rendu local vaut jusqu'au message suivant du hub. Le son du feedback local reste ouvert (§10 #2).

**Conséquences** —
- `DisplayState`, qui ne reçoit aujourd'hui que des messages entrants, aura besoin d'une seconde entrée pour les appuis locaux (dette ajoutée).
- Pour SPEED, l'appui local n'a qu'un retour textuel ; si c'est trop pauvre à l'usage, une nouvelle décision pourra l'enrichir.

## #4 — fix(feedback): stop driving the display pins on an armed press

**Contexte** — En ouvrant la session de retrait des « boutons physiques A–D » (conséquence de la décision #1), la lecture du code a montré qu'il n'y en a pas : `ButtonInput` A à D est l'énumération partagée que produisent aussi les zones tactiles (`main.cpp`), et seul le bouton rouge est lu en physique. La décision #1 supposait à tort du code mort. Le vrai défaut est ailleurs : `ArduinoLocalFeedback::Acknowledge()` écrivait `HIGH` sur les broches 6, 7, 8 et 9, qui appartiennent au bus RGB de la dalle, sans `Begin()` préalable.

**Décision** — La décision #1 reste valable pour le matériel (un seul bouton physique, le rouge), mais sa conséquence « retirer le code des boutons physiques A–D » est **caduque** : `ButtonInput` A à D est conservé pour le tactile. `ArduinoLocalFeedback` est supprimé et remplacé dans `main.cpp` par `NoLocalFeedback` (implémentation vide). L'interface `LocalFeedback` et `BuzzerBehavior::Acknowledge()` sont conservés : c'est le point d'insertion du rendu local de la décision #3. Les tests de `PhysicalButtonReader` qui décrivaient plusieurs boutons physiques sont supprimés ; les deux tests à un seul bouton sont repris avec le bouton rouge.

**Conséquences** —
- L'appui armé ne touche plus aux broches du bus RGB.
- Le type du commit est `fix` et non `refactor` : l'effet observable change (plus d'écriture sur ces broches).
- La règle multi-boutons de `PhysicalButtonReader` n'est plus testée (dette ajoutée).

## #5 — docs(decisions): choose how the renderer learns the armed mode

**Contexte** — Le §8.2 rend `timer_end` de façon conditionnelle : « Trop tard » et zones masquées si un mode armé est en cours, rien si le buzzer est Inerte. `LcdRenderer.Apply` ne reçoit que le type du message et n'a ni mémoire ni accès au mode. Le mode vit dans `BuzzerBehavior`, et il change aussi sans message du hub : l'appui verrouille (`→ Inert`) avant tout retour du hub. `ConnectionEventHandler.OnMessageReceived` appelle `messageRenderer.Apply` **avant** `hubMessageDispatcher.Dispatch` : au moment du rendu, le mode est celui d'avant le message.

**Décision** — Un port en lecture, `ArmedModeReader`, est implémenté par `BuzzerBehavior` et injecté dans `LcdRenderer`. `BuzzerBehavior` reste la seule source de vérité du mode. Le rendu d'un message se fait **avant** son dispatch : cet ordre est un contrat, non un détail. Deux alternatives écartées : `DisplayState` déduisant le mode des messages vus (duplique la machine du §6 et diverge dès qu'un appui local verrouille) ; `BuzzerBehavior` déclenchant le rendu de `timer_end` via `LocalFeedback` (éclate la table §8.2 en deux chemins et change le sens de `LocalFeedback`).

**Conséquences** —
- Le CDC §8.2 n'est pas modifié : la condition « mode armé courant » y figure déjà.
- Session suivante : `feat(rendering): show "too late" only when a question was armed`, avec un test qui verrouille l'ordre rendu-puis-dispatch.
- Dette ajoutée : l'ordre rendu-puis-dispatch est un contrat implicite, non vérifié par un test existant.


## #6 — docs(decisions): choose how the renderer reads the message payload

**Contexte** — Six messages du §8.2 affichent des données du message, pas un texte fixe. Champs relevés dans les specs du hub (US-011 et US-012, `main` au 2026-11-02) :

| Message | Champs lus |
|---|---|
| `question_title` | `title`, `question_index` |
| `question_choices` | `choices` (tableau de 4 propositions) |
| `question_open` | `title` |
| `timer_tick` | `remaining_seconds` |
| `buzz_locked` | `buzzer_username` |
| `question_result` | `correct_answer`, `correct`, et `player_answer` (MCQ seulement) |

`MessageRenderer::Apply` ne reçoit que le type, et `MessageDeserializer` ne sait extraire que le type. `ConnectionEventHandler` extrait le type puis appelle `Apply(type)` et `Dispatch(type)` : le contenu du message n'est lu nulle part.

**Décision** — `ConnectionEventHandler` analyse le message **une seule fois** en un objet `HubMessage` (le type et l'accès en lecture à ses champs), construit par `MessageDeserializer`, et le passe à `MessageRenderer::Apply`. `LcdRenderer` et `DisplayState` lisent seulement les champs que la table §8.2 demande pour ce type. Le répartiteur (`HubMessageDispatcher`) garde le type seul tant qu'aucun rouge ne lui demande davantage. La dépendance à ArduinoJson reste dans `lib/serialization/`. Un champ requis absent ou de mauvais type laisse l'écran inchangé, comme un type inconnu. La forme exacte de `HubMessage` naît du premier rouge (`timer_tick`, un seul champ numérique), pas d'ici.

Deux alternatives écartées :
- **Passer le JSON brut à `Apply`** : le plus petit changement, mais le message est analysé deux fois et ArduinoJson entre dans `lib/rendering/`. Chaque consommateur répète sa gestion des champs absents.
- **Un second port qui lit « le dernier message reçu »** : crée un couplage temporel caché, du même ordre que celui que la décision #5 a dû rendre explicite.

**Conséquences** —
- La signature de `Apply` change : elle touche `MockMessageRenderer`, `LcdRenderer` et leurs tests. Ce changement mécanique se fait dans une session `refactor` à part, comportement inchangé (tous les rendus restent identiques), avant tout nouveau rouge.
- Ordre des sessions suivantes : `refactor(rendering): pass the whole message to the renderer`, puis `feat(rendering): show the remaining seconds on each timer tick`, puis un message par session.
- Deux points restent à trancher sous leur rouge, non ici : `question_index` est numéroté à partir de 0 dans les exemples du hub, alors que le §8.2 affiche « Question N » (à confirmer côté affichage) ; le texte de `question_choices` doit passer des mots « Question QCM en cours » aux quatre propositions.
- Le §8.2 n'est pas modifié.

## #7 — feat(rendering): wrap long texts on the screen

**Contexte** — `GfxScreen::Show` dessinait une seule ligne en taille 5 : environ 26 caractères tiennent sur les 800 px de la dalle. Le titre de `question_open` peut atteindre 250 caractères (US-005 CA-4), et le nom du buzzer n'a pas de borne relevée. Exigence posée par Olivier : le nombre maximal de caractères d'une question doit tenir sur l'écran, et la taille de la police s'adapte à la longueur du texte.

**Décision** — La mise en page est de la logique pure, dans `lib/rendering/TextLayout` (testée en natif). Elle choisit la plus grande échelle de 5 à 3 pour laquelle le texte, coupé aux espaces, tient en hauteur dans la zone de la dalle (marge de 10 px) ; un mot plus long qu'une ligne est coupé ; un caractère UTF-8 compte pour une colonne et n'est jamais coupé en deux. `GfxScreen` ne fait que mesurer la cellule de la police (`fontWidth()`, `fontHeight()` à l'échelle 1) et dessiner les lignes, centrées. `Screen::Show(text)`, `LcdRenderer` et `DisplayState` ne changent pas.

Garantie : un texte de 250 caractères au plus tient à l'échelle 3 ou plus, quelle que soit la longueur de ses mots. Deux lignes consécutives d'un texte coupé aux espaces contiennent plus de colonnes qu'une ligne (le premier mot de la seconde n'a pas tenu sur la première), soit au plus 2 × ⌈250 / 43⌉ + 1 = 13 lignes pour 19 disponibles ; un test l'éprouve sur des textes dont les mots font de 1 à 60 caractères. Au-delà de la capacité de l'échelle 3, le texte garde l'échelle 3 et déborde (dette).

Alternatives écartées :
- **Une taille unique réduite** : « Raté » ou « Connecté » s'afficheraient aussi petit qu'un titre de 250 caractères.
- **Tronquer avec « ... »** : perd une partie de la question, contraire à l'exigence.
- **Couper dans `GfxScreen`** : le découpage ne serait testable qu'au smoke test.
- **Mesurer la largeur en pixels (`lcd.textWidth`)** : inutile avec une police à chasse fixe, à rouvrir avec la police Latin-étendu.

**Conséquences** —
- L'échelle 3 (cellule de 18×24 px) est la plus petite acceptée : sa lisibilité sur la dalle se juge au smoke test.
- Dettes retirées : « Texte long hors de l'écran » et « Nom du buzzer non borné à l'écran » ; dette ajoutée : `TextLayout` suppose une police à chasse fixe.
- Le rendu de `question_title` (texte composé « Question N : … ») peut venir sans repasser par ce sujet.

## #8 — refactor(rendering): measure text in pixels instead of columns

**Contexte** — `TextLayout` raisonnait en colonnes d'une police à chasse fixe (6×8 px, échelles 5 à 3). Olivier veut remplacer `Font0` par une police de jeu télévisé avec accents (Oswald, graisse normale), dont les glyphes ont des largeurs différentes : le découpage doit se faire sur des largeurs en pixels. Changer la mesure et la police dans la même session mêlerait un refactor (comportement inchangé) et une feature (rendu différent, smoke test obligatoire).

**Décision** — Un port `TextMeasurer` (`SizeCount`, `Width(texte, taille)`, `LineHeight(taille)`, taille 0 = la plus grande) est injecté dans `TextLayout`, qui découpe aux espaces tant que `Width` tient dans la zone et choisit la plus grande taille dont `nombre de lignes × LineHeight` tient en hauteur. `MonospaceTextMeasurer` reproduit exactement l'ancien comportement (cellule × échelle, un caractère UTF-8 = une cellule). `GfxScreen` l'instancie avec `fontWidth()` et `fontHeight()` et dessine à `ScaleOf(taille)`. Le décompte des caractères UTF-8 passe dans `Utf8.h`.

Alternatives écartées :
- **Interroger `lcd.textWidth` depuis `TextLayout`** : la logique redeviendrait invisible aux tests natifs (injection de dépendance plutôt que dépendance à LovyanGFX, comme `Screen` côté C#).
- **Faire le refactor et la police dans une seule session** : un seul commit, mais un diff qui mélange « rien ne change » et « tout change à l'écran ».

**Conséquences** —
- Comportement inchangé : les 21 tests existants de `TextLayout` passent avec les mêmes attendus (le découpage en colonnes s'écrit avec une mesure d'un pixel par caractère) ; le smoke test de la PR #28 doit donner les mêmes écrans.
- Cinq tests sur une police proportionnelle fictive (et deux sur le mesureur à chasse fixe) prouvent que la largeur et la hauteur viennent bien du mesureur.
- Prochaine session : `feat(rendering): show accented characters in a quiz font` (adaptateur de mesure sur les glyphes réels, génération de la police, garantie des 250 caractères recalculée).

## #9 — chore(rendering): provide the quiz font and read its glyph widths

**Contexte** — Le port `TextMeasurer` et `ProportionalTextMeasurer` existent (décision #8) mais aucune police réelle n'est générée. Olivier veut Oswald Regular, lisible depuis 50 cm. Générer la police, la brancher et régler l'écran dans une seule session mêlerait du code dormant et un changement visible à l'écran : Olivier a coupé en deux (`chore` ici, `feat` ensuite).

**Décision** —
- Trois tailles d'Oswald Regular en `lgfx::GFXfont` à 1 bit, corps de 30, 38 et 48 px (cap-height de 24, 31 et 39 px, soit environ 3,3, 4,2 et 5,3 mm sur une dalle de 0,136 mm par pixel), U+0020 à U+00FF. Olivier a confirmé trois tailles. La plus petite vise la limite de lisibilité à 50 cm ; le smoke test de la session `feat` tranche.
- Un script Python (`scripts/generate-gfx-font.py`, FreeType) les génère dans `src/fonts/`, avec la licence OFL, le commit source et l'empreinte de la police (`src/fonts/README.md`).
- `ReadFontMetrics` (`lib/rendering/GfxFontMetrics.h`) remplit `FontMetrics` depuis tout objet qui expose `first`, `last`, `yAdvance` et `glyph[i].xAdvance` : `lgfx::GFXfont` en production, une fausse police dans les tests. Un code que la police ne dessine pas avance comme `?`. Un code de contrôle (0x7F à 0x9F) reçoit le glyphe de `?` à la génération, pour que la mesure et le dessin s'accordent.

Alternatives écartées :
- **`fontconvert` d'Adafruit** : il demande les en-têtes de FreeType, absents de l'environnement de session ; le script Python reproduit son format (bits empaquetés sans remplissage entre les lignes).
- **Police lissée (VLW)** : non évaluée (mesure, Flash) ; Olivier avait validé le GFXfont. À rouvrir si le smoke test juge le 1 bit trop crénelé.
- **Un type commun `GfxFont` dans `lib/`** : il faudrait recopier les données de LovyanGFX ou en dépendre dans les tests natifs ; le gabarit évite les deux.
- **Ajouter `œ`, `Œ`, `’`, `…` et `€` dès maintenant** : ils sont hors Latin-1, donc hors plage contiguë ; il faudrait une plage d'encodage LovyanGFX et étendre `FontMetrics`. Consigné en dette.

**Conséquences** —
- Rien dans `src/` n'inclut ces fichiers : le binaire `esp32` est inchangé (Flash de référence : 76,4 %, 1 001 393 octets sur 1 310 720, mesuré par Olivier sur `main` le 2026-11-03). Branchées, les trois polices ajoutent environ 38 Ko (30 499 octets de bitmaps et 8 064 de tables de glyphes), soit environ 79,3 %.
- `ReadFontMetrics` suppose des glyphes contigus de `first` à `last` (pas de plage d'encodage) et la présence de `?` : vrai pour les polices générées ici.
- Un code hors de la police (par exemple `œ`) n'est pas dessiné par LovyanGFX, qui avance de la largeur d'un espace (lecture de `GFXfont::updateFontMetric`). La mesure lui compte la largeur de `?` (14, 18 et 23 px contre 7, 9 et 11 pour l'espace) : elle surestime, donc le texte se coupe plus tôt et ne déborde jamais. Alignée sur l'espace, elle serait exacte, mais cela change `FontMetrics::Advance`, hors de cette session.
- `yAdvance` d'Oswald vaut environ 1,48 em (44, 56 et 71 px) : l'interligne se règle dans la session `feat`.
- Prochaine session : `feat(rendering): show accented characters in a quiz font` (brancher `ProportionalTextMeasurer` dans `GfxScreen`, recalculer la garantie des 250 caractères, smoke test).


## #10 — feat(rendering): show accented characters in a quiz font

**Contexte** — Les trois polices Oswald (décision #9) et `ProportionalTextMeasurer` (décision #8) existent mais `GfxScreen` dessine encore avec `Font0` agrandie, sans accents lisibles. Le `yAdvance` natif d'Oswald (environ 1,48 em) donne un interligne trop large pour un texte de quiz.

**Décision** —
- `GfxScreen` construit un `ProportionalTextMeasurer` avec `ReadFontMetrics` sur les trois polices (48, 38 et 30 px, de la plus grande à la plus petite), dessine avec la police de la taille que choisit `TextLayout` (`setFont`, échelle 1) et espace les lignes de la hauteur de ligne du mesureur. Son constructeur ne change pas : `main.cpp` est inchangé.
- L'interligne est réglé dans `ReadFontMetrics(font, lineSpacingPercent)` : hauteur de ligne = `yAdvance` × pourcentage, arrondie au pixel le plus proche. La constante `QuizLineSpacingPercent` vaut 90 %, soit 64, 50 et 40 px. Première valeur : 82 %, mais le smoke test du 2026-11-03 a montré les descendants (`q`, `g`, `y`) mordre sur les accents de la ligne suivante (dégagement mesuré de −2, −2 et −3 px) ; à 90 %, il est de 4, 2 et 1 px. Un test natif fige ce dégagement (hauteur d'accent et profondeur de descendant lues dans la police) pour la plus petite taille, la plus serrée.
- La garantie des 250 caractères (choix d'Olivier : un texte réaliste, pas le pire cas) est figée dans deux tests natifs de `TextLayout` : un titre français de 243 caractères, en minuscules et tout en majuscules, tient (4 lignes de 40 px) à la plus petite taille avec les largeurs réelles d'Oswald 30 px. Ces largeurs viennent de `test/fixtures/OswaldRegular30Advances.h`, extraites de l'en-tête de police par `scripts/extract-font-advances.py` (option A).

Alternatives écartées :
- **Fausses largeurs « réalistes » dans les tests (option B)** : plus simple, mais elles ne prouvent rien sur la vraie police.
- **Un interligne en paramètre de `TextMeasurer`** : le port ne gagne rien, la hauteur de ligne est déjà une donnée de `FontMetrics`.
- **Garantir le pire cas** : impossible à la plus petite taille. Mesuré : 250 `¼` ou des mots de 18 `W` donnent 13 lignes, soit 520 px pour 460 (voir `DEBTS.md`).

**Conséquences** —
- LovyanGFX centre, avec une police GFX, la boîte d'encre de toute la police (accents compris, 40, 50 et 62 px) sur le point donné, et non la hauteur de ligne : `fontHeight()` ne sert donc plus d'interligne. Les boîtes d'encre de deux lignes voisines se recouvrent de quelques pixels ; seul compte le dégagement entre l'encre, figé par le test ci-dessus.
- Le build `esp32` n'a pas été lancé dans la session (PlatformIO absent) : `GfxScreen.h` est vérifié en syntaxe contre les vraies structures `GFXfont` de LovyanGFX 1.2.36, et le Flash visé (environ 79,3 %) reste à mesurer au smoke test.
- `MonospaceTextMeasurer` n'est plus utilisé en production (dette).


## #11 — feat(rendering): show œ and typographic symbols in a quiz font

**Contexte** — Les polices Oswald ne couvraient que Latin-1. `Œ`, `œ`, `’`, `…` et `€` n'avaient pas de glyphe : LovyanGFX ne dessine rien pour eux et avance d'un espace (« cœur » s'affichait « c ur »), alors que la mesure leur comptait la largeur d'un `?` (décision #9).

**Décision** —
- Les cinq glyphes sont rangés après les 224 de Latin-1 (indices 224 à 228) et déclarés par cinq `lgfx::EncodeRange` (`{start, end, base}`) : Latin-1, U+0152-U+0153, U+2019, U+2026, U+20AC. La police porte `last` = 0x20AC, `range_num` = 5 et le tableau de plages. Lecture du source de LovyanGFX 1.2.36 (`GFXfont::getGlyph`, `getDefaultMetric`) : le code `start` + k dessine le glyphe d'indice `base` + k.
- `scripts/generate-gfx-font.py` génère ces glyphes (liste `EXTRA_CODES`) et les plages ; un code de la liste absent de la police arrête le script. Les trois polices sont régénérées : les bitmaps et les glyphes de Latin-1 sont inchangés, seuls des glyphes et des plages sont ajoutés.
- `FontMetrics` reçoit, en troisième paramètre optionnel, les avances des codes hors Latin-1 (code -> avance) ; tout autre code avance toujours comme `?`. `ReadFontMetrics` lit `range_num` et `range` comme LovyanGFX (`FindAdvance`) : les codes des plages au-delà de 0xFF gardent leur avance. Sans plage (`range_num` à 0), le comportement est celui de la décision #9.
- `scripts/extract-font-advances.py` copie aussi, dans la fixture de test, le code et l'avance des cinq glyphes (20, 21, 5, 17 et 15 px en Oswald 30 px).

Alternatives écartées :
- **Ne traiter que `œ` et `’`** : le mécanisme (plages) coûte pareil pour cinq glyphes, et la dette ne se solde qu'entière.
- **Une table de codes écrite en dur dans `ReadFontMetrics`** : la lecture des plages de la police suffit, sans liste à tenir en double.
- **Mesurer les codes absents comme un espace** : ne change pas, `?` reste une surestimation sans danger.

**Conséquences** —
- La boîte d'encre de la police ne change pas (33 + 7, 41 + 9 et 51 + 11 px, avec ou sans les cinq glyphes) : l'interligne à 90 % reste valable.
- Poids : les données ajoutées pèsent 1 034 octets (764 de bitmaps, 180 de glyphes, 90 de plages), mais le smoke test du 2026-11-04 a mesuré +3 196 octets au total : Flash 79,8 % (1 046 093 octets sur 1 310 720) contre 79,6 % (1 042 897) avant. Les 2 162 octets en plus de l'estimation s'expliquent à peu près (mesure x86 en `-Os`, indicative, non Xtensa) : environ 1,4 Ko pour la `std::map` de `FontMetrics` (487 octets dans le code de la police, environ 900 dans libstdc++ : `_Rb_tree_insert_and_rebalance`, `_Rb_tree_increment`, `_Rb_tree_decrement`) et environ 0,9 Ko pour le code de lecture des plages (`ReadFontMetrics` et `FindAdvance`, 874 octets), que l'estimation ne comptait pas. Remplacer la `std::map` par un petit vecteur gagnerait environ 1,4 Ko (0,1 point de Flash), comportement inchangé : `refactor` à faire au besoin, si le Flash devient serré. Sans conséquence à ce niveau de remplissage.
- Le générateur Python n'a pas de test : la lecture des plages a été recoupée à la main avec FreeType (avances concordantes pour 195 codes dans chacune des trois polices, aucun code hors plage résolu à tort).


## #12 — feat(rendering): show the four choices when a question is opened

**Contexte** — `question_choices` portait un texte provisoire (« Question QCM en cours »). Le §8.2 demande « les 4 propositions A/B/C/D » avec l'effet `Zones actives`, sans dire comment les composer en un seul texte LCD.

**Décision** —
- Le texte est composé : « A : … » à « D : … », une proposition par ligne (séparateur `\n`, que `TextLayout` coupe depuis la session `break a text at line feeds`).
- `HubMessage` porte `choices` comme un `std::vector<std::string>` ; `MessageDeserializer` le lit seulement si `choices` est un tableau **dont tous les éléments sont du texte**. Sinon le message n'a pas de `choices` et l'écran reste inchangé (règle de la décision #6).
- La cardinalité n'est pas vérifiée : le contrat du hub garantit quatre propositions (choix d'Olivier, « gravé dans le marbre pour l'instant »). `DisplayState` composera pourtant ce qu'on lui donne, avec les lettres A, B, C… selon la position, sans indexer en dur : un tableau plus court n'est pas un comportement indéfini.

Alternatives écartées :
- **Un rendu par zone** (une proposition dans chaque zone tactile) : relève de la feature « disposition dynamique des zones » (dette de `main.cpp`), pas de cette session.
- **Écran inchangé si le nombre de propositions n'est pas quatre** : une validation de plus que le contrat du hub ne demande ; à rouvrir si le hub cesse de garantir quatre.

**Conséquences** —
- `HubMessage::HasChoices()` n'a pas de consommateur de production : `DisplayState` s'en sert, mais le tableau vide donnerait le même texte vide (mutant équivalent). On le garde par cohérence avec les autres champs.
- L'effet `Zones actives` n'est pas porté par cette session (feature « effets d'écran »).
- Non vu à l'écran : le rendu de quatre lignes, et d'une proposition longue qui passe à la ligne, se juge au smoke test (`fake-hub.ps1`, entrées `d` et `f`).

## #13 — feat(rendering): show the result of a question

**Contexte** — Le §8.2 demande pour `question_result` un « Résultat (bonne réponse + réponse joueur) » en MCQ, « validé » ou « raté » en SPEED, sans en fixer les mots. La distinction MCQ/SPEED se déduit du contenu du message (§8.2). Le hub (US-011 CA-25, US-012 CA-31) envoie aussi `points_earned` et `cumulative_score`.

**Décision** (libellés choisis par Olivier à la porte 1) —
- **MCQ** (`correct_answer` est une seule lettre de `A` à `D`) : « Bonne réponse : B », puis, à la ligne suivante, « Votre réponse : C » seulement si `player_answer` est présent et textuel. Sans réponse du joueur, une seule ligne.
- **SPEED** (sinon) : « Validé » si `correct` est `true`, « Raté » s'il est `false`. La bonne réponse en clair n'est pas affichée.
- **Champ requis absent ou de mauvais type** : écran inchangé (décision #6) — `correct_answer` dans tous les cas, `correct` en plus en SPEED. `player_answer` est facultatif.
- `HubMessage` porte `correct_answer` (texte), `correct` (booléen, distinguant `false` d'absent) et `player_answer` (texte). `points_earned` et `cumulative_score` ne sont pas lus : aucun rendu ne les demande.

Alternatives écartées :
- **Afficher la bonne réponse en SPEED** (« Validé » puis « Réponse : Paris ») : le texte de la réponse peut être long et le §8.2 ne la demande pas.
- **Déduire MCQ/SPEED de `player_answer`** : absent aussi pour un MCQ sans réponse du joueur ; le §8.2 fixe la règle sur `correct_answer`.

**Conséquences** —
- Une réponse SPEED qui serait une seule lettre de `A` à `D` (« B ») serait prise pour un MCQ : le contrat du hub (SPEED renvoie un texte de réponse) l'exclut, la limite est assumée.
- `player_answer` à `null` (joueur sans réponse, enregistré avec `answer: null` côté hub, US-011 CA-34) est traité comme absent ; non vérifié contre le hub, qui n'émet pas encore ce message.
- Les effets d'écran (zone mise en évidence, bandeau clignotant, zones masquées) ne sont pas portés par cette session (feature « effets d'écran »).
- Non vu à l'écran : le rendu se juge au smoke test (`fake-hub.ps1`, entrées `g` à `j`).

## #14 — feat(rendering): show the four choices as tappable zones

**Contexte** — `question_choices` s'affichait comme un seul texte de quatre lignes (décision #12). Le §8.2 lui donne l'effet `Zones actives` : les zones A–D sont affichées. `Screen` ne savait que `Show(texte)`.

**Décision** (choix d'Olivier à la porte 1 : chaque quadrant contient la proposition entière) —
- `Screen` gagne `ShowZones(zoneTexts)` : un texte par zone, en grille 2×2 (A en haut à gauche, B en haut à droite, C en bas à gauche, D en bas à droite), le découpage des zones tactiles de `main.cpp`. Chaque texte est mis en page dans son quadrant (`TextLayout`), avec un cadre.
- `DisplayState::ZoneTextsFor(message)` donne les textes (« A : … » à « D : … », lettre selon la position) pour `question_choices`, une liste vide pour tout autre type de message ou sans `choices`. Il reste sans état.
- `LcdRenderer` appelle `ShowZones` quand la liste n'est pas vide, `Show` sinon. `question_choices` n'a plus de texte plat (`LcdTextFor` rend une chaîne vide).

Alternatives écartées :
- **Un paramètre d'effet sur `Show`** (`Show(texte, effet)`) : un seul effet existe ; un type d'effet se justifiera avec le deuxième (« Mettre en évidence »), pas avant.
- **La lettre seule dans le quadrant, le texte ailleurs** : le texte des propositions ne serait plus relié à la zone qu'on touche.

**Conséquences** —
- Le tactile reste actif sur tout l'écran (zones figées de `main.cpp`) : seul le dessin change. Les effets « Zones masquées » et « Mettre en évidence » restent à faire ; un appui MCQ affiche toujours « Réponse envoyée » en plein écran, ce qui efface les quadrants.
- Le découpage 2×2 vit à deux endroits (`GfxScreen`, `main.cpp`) : noté dans `DEBTS.md`.
- Non vu à l'écran : une proposition de 40 caractères dans un quadrant de 400×240 px (zone de texte 380×220), à juger au smoke test (`fake-hub.ps1`, entrées `d` et `f`).

## #15 — feat(rendering): highlight the correct answer in the result

**Contexte** — `question_result` MCQ affichait « Bonne réponse : B » en texte plein écran (décision #12), alors que le §8.2 lui donne l'effet `Mettre en évidence zone(correct_answer)`. Le message ne porte pas les propositions : pour garder les quadrants visibles, quelqu'un doit les retenir.

**Décision** (choix d'Olivier à la porte 1 : texte dans un bandeau en haut, quadrants rétrécis dessous, mise en évidence statique) —
- `Screen` gagne `HighlightZone(zoneIndex, text)` : le texte dans un bandeau (le quart de la hauteur), les quadrants des dernières propositions dessous, la zone `zoneIndex` en blanc plein.
- `DisplayState::HighlightedZoneFor(message)` donne l'indice (0 à 3) pour un `question_result` dont `correct_answer` est une lettre A–D, `DisplayState::NoZone` (−1) sinon. Il reste sans état. Un entier plutôt qu'un `std::optional` : le build `esp32` a refusé `<optional>` (le toolchain n'est pas en C++17), alors que les tests natifs le compilaient.
- `LcdRenderer` appelle `HighlightZone(zone, texte)` quand une zone est désignée, sinon l'ancien chemin (`ShowZones`, `Show`).
- `GfxScreen` retient les textes du dernier `ShowZones` (`lastZoneTexts`) et les redessine.

Alternatives écartées :
- **`DisplayState` retient les propositions** : il deviendrait à état, et les deux instances (`LcdRenderer`, `ScreenLocalFeedback`) devraient être partagées ; c'est la feature « effets d'écran », pas cette tranche.
- **Un seul `ShowZones(textes, zone, bandeau)`** : paramètres facultatifs sur un contrat qui n'a qu'un appelant par cas, YAGNI.
- **Rétrécir aussi les quadrants de `question_choices`** : le tactile couvre 800×480 en dur (`main.cpp`) ; des quadrants dessinés plus petits que les zones tactiles se liraient de travers à l'appui. `question_choices` reste plein écran.

**Conséquences** —
- Le clignotement n'est pas porté (horloge absente) : dette au `DEBTS.md`. La mise en évidence de la lettre choisie à l'appui local non plus (`AcknowledgeAnswer` sans lettre) : elle reste dans « Table de rendu §8.2 incomplète ».
- L'état des propositions vit dans l'adaptateur, non testable nativement, et peut être périmé : voir `DEBTS.md`.
- Non vu à l'écran : la hauteur des quadrants rétrécis (180 px) avec une proposition de 36 caractères, et le contraste du texte noir sur la zone blanche (`fake-hub.ps1`, entrées `d`, `f`, `g`, `h`, `i`).


## #16 — feat(rendering): highlight the chosen answer when the player taps

**Contexte** — La décision #3 donne à l'appui MCQ l'effet `Mettre en évidence zone(x)` et le texte « Réponse envoyée ». Le firmware n'affichait que le texte, en plein écran, ce qui effaçait les quadrants : `AcknowledgeAnswer()` ne portait pas la lettre choisie.

**Décision** (choix d'Olivier à la porte 1 : mise en évidence en plein écran, **sans texte**, « plus proche du rendu final ») —
- `LocalFeedback::AcknowledgeAnswer(char)` porte la lettre ; `BuzzerBehavior::OnMcqPressed` la transmet.
- `Screen` gagne `HighlightChosenZone(zoneIndex)` : les dernières zones affichées, en plein écran (le découpage de `ShowZones`, donc celui des zones tactiles), la zone touchée en blanc plein, aucun texte ajouté.
- `DisplayState::ZoneIndexForAnswer(letter)` (statique, sans état) donne l'indice 0 à 3, `NoZone` hors A–D ; `HighlightedZoneFor` et `IsLetterAnswer` s'appuient sur lui (un seul endroit définit l'intervalle A–D). `ScreenLocalFeedback` ne fait rien pour une lettre inconnue.
- Le texte « Réponse envoyée » est retiré (`LcdTextForAnswerSent` supprimé). Cette décision remplace la partie « texte » de la #3 pour l'appui MCQ ; le texte de l'appui SPEED (« Buzz envoyé ») est inchangé.

Alternatives écartées :
- **La lettre en paramètre avec `HighlightZone` et son bandeau** : ajoute un second écran au dessin rétréci, à 40 % de la hauteur près des zones tactiles ; le texte aurait gardé une place que le rendu final n'a pas.
- **`DisplayState` retient la zone choisie, instance partagée** : YAGNI, aucun rouge ne l'exige. Les deux instances restent sans état.

**Conséquences** —
- Avec le vrai hub, `answer_received` arrive juste après l'appui et affiche « Réponse enregistrée » en plein écran (§8.2) : la mise en évidence ne dure que jusqu'à ce message. Voir `DEBTS.md`.
- `HighlightChosenZone` redessine les textes retenus par `GfxScreen` (`lastZoneTexts`) : périmés si l'appui suit une `question_choices` dont les propositions étaient inexploitables (le buzzer est armé, l'écran n'a pas été redessiné).
- Non vu à l'écran : le blanc plein sous la zone touchée, texte noir (`fake-hub.ps1`, entrée `d`, puis un appui par quadrant).


## #17 — feat(rendering): keep the chosen answer visible when the hub acknowledges it

**Contexte** — `answer_received` affichait « Réponse enregistrée » en plein écran (§8.2 : `Zones masquées`), ce qui effaçait la zone mise en évidence par l'appui local (#16) presque aussitôt avec le vrai hub : le retour visuel du choix ne durait pas.

**Décision** (choix d'Olivier à la porte 1 : zones conservées, texte dans un bandeau **en haut**) —
- Le CDC §8.2 gagne l'effet `Zones conservées` et l'applique à `answer_received` : zones inactives sous un bandeau, zone choisie toujours en évidence. Déroge à la ligne précédente du tableau (`Zones masquées`).
- `Screen` gagne `ShowBanner(text)`. `DisplayState::KeepsZonesFor(message)` (sans état) dit quels messages gardent les zones (aujourd'hui : `answer_received`) ; `LcdRenderer` appelle `ShowBanner` au lieu de `Show`.
- `GfxScreen` retient la zone touchée (`lastChosenZone`, posée par `HighlightChosenZone`, effacée par `ShowZones`) et redessine le même écran que `question_result` (`HighlightZone`) : bandeau en quart de hauteur, quadrants rétrécis dessous.

Alternatives écartées :
- **Bandeau posé par-dessus l'écran courant, sans redessiner** : il aurait recouvert la moitié haute des zones A et B (texte centré à mi-hauteur).
- **`DisplayState` retient la lettre choisie** : YAGNI, aucun rouge ne l'exige ; l'état reste au bord, déjà porteur des textes de zones.

**Conséquences** —
- Les quadrants rétrécissent quand l'accusé arrive (plein écran → sous le bandeau) : saut de mise en page assumé, identique à l'écran de résultat.
- `answer_received` sans appui local préalable (hub qui répond à un `answer` venu d'ailleurs) : aucune zone en évidence, bandeau + zones seulement.
- Un `answer_received` reçu sans `question_choices` préalable redessinerait les propositions de la question précédente (`Show` ne les efface pas) : voir `DEBTS.md`.
- Non vu à l'écran : le bandeau au-dessus du quadrant touché, après un appui puis `a` du faux hub.


## #18 — fix(input): ignore touches on answer zones that are not displayed

**Contexte** — La décision #2 pose que les zones ne sont tactiles que sous `Zones actives`. Le firmware gardait pourtant quatre zones codées en dur (800×480, `main.cpp`), actives sur tous les écrans : seul l'armement de `BuzzerBehavior` empêchait de lire un appui sur une zone absente de l'écran. Le découpage vivait en deux endroits (`main.cpp` pour le tactile, `GfxScreen::DrawZones` pour le dessin, qui divisait `lcd.width()` par deux).

**Décision** (cadrage validé par Olivier à la porte 1, 2026-11-09) —
- `ZoneGrid` (`lib/rendering`) : la grille 2×2 (largeur, haut, hauteur de l'aire → rectangle de chaque zone), pure et testée. `GfxScreen::DrawZones` et les zones tactiles s'en servent : un seul découpage.
- `TappableZonesScreen` décore `Screen` : `ShowZones` pose une zone tactile par texte affiché (quatre au plus, comme le dessin) sur la grille plein écran ; `Show`, `HighlightZone`, `HighlightChosenZone` et `ShowBanner` les retirent. `LcdRenderer` et `ScreenLocalFeedback` reçoivent le décorateur ; `TouchZoneReader` lit son vecteur, inchangé.
- `PanelWidth` et `PanelHeight` (`main.cpp`) : source unique de la configuration LGFX et des zones tactiles.

Alternatives écartées :
- **`DisplayState` porte l'état tactile (effet d'écran)** : il deviendrait à état et les deux instances devraient être partagées, c'est la feature « effets d'écran » (décision #2), pas cette tranche.
- **Zones tactiles sous le bandeau** (`HighlightZone`, `ShowBanner`) : sans objet, les zones y sont inactives (§8.2).
- **Lire la taille par `lcd.width()`** : `lcd` est construit comme global et initialisé dans `setup()` ; non vérifié que la taille est déjà connue à la construction, d'où des constantes partagées avec la configuration.

**Conséquences** —
- Un appui sur une zone qui n'est pas à l'écran est ignoré. Le comportement change dans deux cas limites, acceptés (voir `DEBTS.md`) : `timer_tick` pendant un MCQ armé (`Show` efface les zones et le tactile ; l'appui sur l'écran vide marchait avant) ; `question_choices` aux propositions inexploitables (écran et zones tactiles inchangés).
- Zones retirées : un doigt resté posé sur un quadrant pendant qu'un nouveau `ShowZones` arrive déclencherait un appui (`lastButton` repassé à `Unknown`). Improbable, non vu.
- Non vu à l'écran : tout le smoke test (`TappableZonesScreen`, `GfxScreen::DrawZones` refactoré et `main.cpp` ne sont validés que par lui et par une vérification de syntaxe `gnu++11` sur stub).

## #19 — docs(mockups): draw the MCQ question screens

**Contexte** — Les écrans du buzzer ont été construits un effet à la fois (décisions #2 à #17), en noir et blanc, sans vue d'ensemble. Olivier juge le rendu actuel « pas beau » et veut un rendu cible à mi-chemin entre un jeu télévisé et un écran de smartphone en paysage ; il envisage aussi de changer l'écran pour un plus puissant, voire plus grand, et d'utiliser LVGL pour le rendu (rien n'est décidé).

**Décision** (cadrage validé par Olivier à la porte 1, 2026-11-09 ; le détail visuel est une **proposition de Claude, à valider à la revue de PR**) —
- Les maquettes sont des pages HTML statiques, **une par écran**, dans `docs/mockups/` (`index.html` les montre toutes). Elles représentent le rendu **cible**, pas l'existant, et ne changent ni le CDC ni le code.
- Dimensions en unités relatives à la largeur (1u = 1 % ; 8 px à 800 px), ratio 5:3 : la maquette suit un changement de taille d'écran.
- Formes simples seulement (rectangles arrondis, cercles, arcs, aplats, opacité), réalisables en LVGL comme en LovyanGFX ; le fond est un dégradé vertical à deux teintes, natif en LVGL.
- Police : Barlow Semi Condensed (OFL), graisses 600, 700 et 800, fichiers `woff2` versionnés dans `docs/mockups/fonts/`.
- **Bandeau du haut sur chaque écran** (demande d'Olivier, 2026-11-09) : le nom du joueur à gauche, ses points puis son classement (« 2e/6 ») juste après, « Buzz’O’Live » au centre, à droite une icône d'état de la liaison avec le hub (blanche si la connexion fonctionne, barrée et rouge sinon) puis le taux de charge de la batterie avec une icône dont le remplissage suit la charge. Le bandeau de texte LCD et le chrono passent dessous. Proposés en plus : nom tronqué par « … » pour ne pas toucher le titre, pourcentage et icône en rouge à 20 % ou moins.
- Langage visuel (charte plus gaie, demandée par Olivier le 2026-11-09 : moderne, mais proche des jeux télé) : fond de plateau violet vif en dégradé vertical, bandeau du haut violet foncé ; **un en-tête unique** qui réunit le bandeau du CDC (texte LCD) et le chrono (anneau + secondes) ; réponses en cartes blanches arrondies 2×2, texte indigo, la lettre dans une pastille de couleur propre (A cyan, B rose, C orange, D vert) ; **le jaune doré est le seul marqueur d'état** (choisie, bonne réponse, points), le rouge marque l'erreur (réponse fausse du joueur cerclée, temps écoulé, batterie faible), les autres cartes s'estompent.

**Conséquences** (si la proposition est validée) —
- Le CDC §8.2 n'est **pas** modifié par cette session : voir `DEBTS.md`, « Rendu cible des maquettes non reporté ». Écarts à arbitrer : l'appui local ne se fait plus « en plein écran » mais sous l'en-tête ; le chrono vit dans l'en-tête (le CDC ne dit pas où il va quand les zones occupent l'écran) et passe au rouge sous 5 s (seuil proposé) ; « Question N » reste affiché dans l'en-tête pendant les propositions ; les propositions n'ont plus le préfixe « A : » (la lettre est dans la pastille).
- **Le bandeau du haut demande des données que le firmware n'a pas, sauf l'état de la liaison** : le nom du joueur (le firmware ignore son participant, CDC §8.1 ; `auth_success` porte un `username`, dont on ne sait pas s'il désigne le compte du buzzer ou le joueur), ses points et son classement (aucun message du protocole n'en porte ; à définir côté hub, avec le nombre de joueurs) et le niveau de batterie (mesure matérielle non vérifiée sur la CrowPanel 5″ ; un buzzer sans batterie n'aurait rien à afficher). Aucune n'est spécifiée ni vérifiée ici. L'état de la liaison existe déjà côté firmware (`ConnectionEventHandler`, `ReconnectionPolicy`) ; l'icône reflète la liaison avec le hub, pas seulement le Wi-Fi, et son état « en cours de reconnexion » n'est pas maquetté. Le comportement des zones pendant une coupure n'est pas spécifié (CDC §5).
- Pire cas de texte (`qcm-02e`) : une proposition de 40 caractères (maximum du hub) ne tient pas en trois lignes à 40 px (essai : quatre lignes, qui touchent les bords de la carte). La maquette réduit donc le texte à 30 px, avec **une seule taille pour les quatre cartes**, choisie sur la plus longue ; `GfxScreen` choisit aujourd'hui une taille **par texte** (`TextLayout`), ce qui donnerait des cartes de tailles différentes. Le titre de 250 caractères n'est pas maquetté.
- `DisplayState` ne peut plus se limiter à un texte : l'en-tête demande un modèle (titre de question retenu, chrono, texte d'en-tête, zones, zone choisie). C'est la sum-type déjà prévue pour les messages à contenu variable.
- Candidat pour une session ultérieure : un prototype LVGL (empreinte Flash mesurée : le firmware est à 80,2 %, la dalle et la version de LVGL restent à choisir). Il remplacerait `GfxScreen` et probablement la couche tactile, le port `Screen` restant la couture. Ses animations natives pourraient dispenser du port `Clock` prévu pour le clignotement. Non décidé : le changement d'écran peut le rendre caduc.

## #20 — docs(mockups): show a character reacting to the result

**Contexte** — Olivier veut rendre le quiz plus drôle : à la révélation de la réponse, un petit personnage à son image est heureux si la réponse est bonne, déçu ou en colère si elle est fausse. Il veut un avatar par joueur et une dizaine de réactions, au choix de Claude parmi celles d'un jeu télé. Suite de la décision #19 (maquettes).

**Décision** (choix d'Olivier à la porte 1, 2026-11-09) —
- **Emplacement** : une réaction plein écran d'environ 1,5 s avant l'écran de résultat, le personnage en grand à gauche dans un médaillon, un libellé à droite. Le bandeau du haut reste. Écartés : un médaillon dans l'en-tête (trop petit pour lire l'émotion à 50 cm), une colonne à gauche de l'écran de résultat (rétrécit les cartes, déjà justes au pire cas de texte).
- **Dix réactions**, tirées au hasard dans la catégorie de la situation, sans état côté firmware : bonne réponse (Ravi, Hilare, Amoureux, Malin, Taquin), mauvaise réponse (Déçu, En colère, Dépité, Sonné), sans réponse (Endormi). Chacune a son libellé (« Bravo ! », « Raté… », « Pas de réponse »…) ; le fond du médaillon dit la situation (or, rose, lavande).
- **Style** : avataaars (Pablo Stanley) généré avec DiceBear, d'après les photos d'Olivier (cheveux courts, barbe légère, lunettes). Choisi contre Lorelei (CC0) dont les expressions étaient trop discrètes. Hilare est modifié à la main (larmes de joie) : sa première version, bouche ouverte, était la moins lisible, la barbe cachant la bouche.
- **Format** : une page par réaction (`reaction-NN-*.html`), dix SVG transparents dans `docs/mockups/avatars/` avec leurs paramètres et la licence (`README.md`).

**Conséquences** (si la proposition est validée) —
- Un avatar par joueur ne tient pas dans le Flash : le hub les générerait (DiceBear tourne en Node), le buzzer les téléchargerait au démarrage (précédent : `play_sound_url`) pour les garder en PSRAM. À 300 px en RGB565, une image pèse environ 176 Ko, soit environ 1,76 Mo les dix (1,15 Mo à 240 px). La taille de la PSRAM n'est pas vérifiée. Le protocole (message ou champ d'avatar), le choix de l'avatar dans Angular et la génération côté hub restent à définir : voir `DEBTS.md`.
- La réaction suppose une temporisation (port `Clock`, comme le clignotement) et un tirage au hasard. Le CDC §8.2 ne la décrit pas : elle s'ajoute à `question_result`.
- Licence : avataaars est « libre pour usage personnel et commercial », pas CC0. Le dépôt est public et le personnage reprend le visage d'Olivier, ce qu'il a accepté.
- « Dépité » (yeux au ciel) reste la réaction la moins lisible ; Olivier l'a gardée.

## #21 — docs(mockups): add a female character to the reactions

**Contexte** — Olivier veut aussi un avatar féminin avec les mêmes dix réactions (suite de #20), d'après trois photos qu'il a fournies.

**Décision** (choix d'Olivier à la porte 1, 2026-11-09) —
- Un second personnage avataaars, mêmes dix réactions, mêmes libellés, mêmes fonds de médaillon, mêmes larmes de joie dessinées à la main pour Hilare ; dix SVG `*-f.svg` et **dix pages de plus** `reaction-NN-*-f.html` (vingt pages de réaction au total). Écarté : un paramètre `?avatar=f` en JavaScript sur les dix pages existantes, qui aurait cassé le « HTML statique sans script ».
- Traits retenus : lunettes `prescription02`, peau brune, haut clair à encolure ronde (choix d'Olivier). **Chevelure dessinée à la main** à sa demande (châtain clair, bouclée, mélange de `curly` et `curvy`, raie sur le côté) : la première version (`fro`, orange) ne lui plaisait pas, et aucune coiffure avataaars n'était à la fois longue, bouclée et avec une raie. Les teintes ont été ajustées après un premier essai où peau, cheveux et lunettes se confondaient. Paramètres dans `avatars/README.md`.
- Les pages existantes sont régénérées à l'identique (aucun diff).

**Conséquences** (si la proposition est validée) —
- Le firmware n'a plus « un avatar » mais **un catalogue de personnages** : ce qui est choisi par joueur n'est plus seulement une image mais un modèle de personnage. Cela renforce la piste d'une génération côté hub (voir `DEBTS.md`, « Avatar par joueur sans source ni protocole »). Le coût PSRAM d'environ 1,76 Mo pour dix images concerne **un** joueur : le coût total dépend du nombre de joueurs et de la mise en cache.
- Limites : pas de collier ni de motif rayé ; la chevelure dessinée à la main sort du catalogue avataaars (licence de la partie générée inchangée, la surcouche est de Claude) et son style, plus rond, est un peu différent de celui du personnage masculin. Le personnage ressemble à l'esprit des photos, pas aux détails.
- Le dépôt est public et le personnage s'inspire du visage d'une proche d'Olivier. Aucun prénom n'est écrit dans le dépôt ; à confirmer : que la personne représentée est d'accord.
- Le zoom du médaillon rogne les côtés de la chevelure, plus large que le visage.

## #22 — docs(mockups): add the speed question screens

**Contexte** — Olivier veut modéliser les écrans des questions de rapidité (SPEED) sur le design system des maquettes QCM (suite de #19). Découpe choisie à la porte 1 (2026-11-09) : ouverture, appui local et « À toi ! » dans cette session ; le reste du parcours dans une suivante.

**Décision** (choix d'Olivier à la porte 1, 2026-11-09) —
- **Ouverture** (`question_open`, aussi `buzz_unlocked` : même écran) : en-tête « Question N » et chrono comme en QCM ; le titre est la question, en grand ; en pied, un bouton rouge dessiné et « Appuie sur le bouton rouge », car le bouton physique n'a pas de LED et l'écran est le seul indicateur « tu peux buzzer ». Variantes : titre de 239 caractères (taille réduite) et chrono à 4 s (rouge, même seuil de 5 s que le QCM).
- **Appui local** : en-tête « Buzz envoyé » avec le chrono, le titre disparaît, le bouton rouge s'enfonce et « En attente de confirmation » (texte proposé). Pas de « À toi ! » ici : le hub a pu accepter un autre buzzer d'abord (CDC §8.2).
- **« À toi ! »** (`buzz_accepted`) : bandeau or à la place de l'en-tête (état allumé du clignotement, pas de page pour l'état éteint), la question reste affichée, « Réponds à voix haute ». Pas de chrono : la réponse se dit à l'oral et le maître du jeu tranche.
- **Capsules** (précision d'Olivier, 2026-11-09, valable MCQ et SPEED) : sur l'écran où l'on voit l'intitulé de la question, trois capsules au-dessus du titre : thème, points à gagner (en or), temps imparti. MCQ : `qcm-01-title`, après la pastille « Question N » ; SPEED : `speed-01*`. La capsule du temps reste fixe, seul le chrono de l'en-tête décroît. Non appliquées à « À toi ! » (`speed-03`), où la question n'est qu'un rappel.
- **Pages** : `speed-01-open`, `speed-01b-open-long`, `speed-01c-open-urgent`, `speed-02-pressed`, `speed-03-accepted`, section « Questions de rapidité » dans `index.html`, styles ajoutés à `mockup.css`.

**Conséquences** (si la proposition est validée) —
- Le CDC §8.2 ne change pas dans cette session. Le thème, les points et le temps imparti ne figurent dans aucun message du CDC §8.1 ; non vérifié contre les specs du hub : voir `DEBTS.md`. Écarts à arbitrer au report : le titre persistant sous « À toi ! » et le bouton rouge dessiné ne sont pas dans le CDC ; « En attente de confirmation » et « Réponds à voix haute » sont des libellés proposés.
- Restent à maquetter, avec ce qu'Olivier a déjà dit : `buzz_locked` et `buzz_invalidated` nettement distincts (le second avec une réaction du personnage), `question_result` SPEED avec la réponse attendue et la réaction. Ce dernier contredit la décision #13 (« Validé » / « Raté » seuls) : à trancher au report dans le CDC.
- Le titre de la maquette longue fait 239 caractères (pas 250) et tient sur quatre lignes à 3,5 u de corps ; le maximum exact de 250 n'a pas été rendu, et il n'y a pas de défilement prévu.

## #23 — docs(mockups): add the speed question passive screens

**Contexte** — Suite de #22. Olivier a choisi deux sessions pour finir le parcours SPEED : les écrans passifs (celle-ci), puis le résultat (à venir).

**Décision** (choix d'Olivier à la porte 1, 2026-11-09) —
- **« Un autre a buzzé »** (`buzz_locked`, `speed-04-locked`) : bandeau violet sobre « Julien a buzzé », nom en or, la question atténuée dessous ; sans chrono ni bouton rouge ; le nom seul, sans avatar (l'avatar par joueur n'a ni source ni protocole, voir `DEBTS.md`). Le bandeau n'est pas or, pour se distinguer de « À toi ! ».
- **« Éliminé pour la question »** (`buzz_invalidated`, `speed-05-invalidated`) : « Raté… » du CDC §8.2 avec la précision « Éliminé pour cette question » (proposée), et le personnage déçu en plein écran, fond rose. Distinction forte demandée par Olivier le 2026-11-09, avec une réaction.
- **`timer_end` SPEED** : pas de page, « Trop tard » est identique au QCM (`qcm-05`) ; précisé dans l'index.

**Conséquences** (si la proposition est validée) —
- Le CDC §8.2 ne change pas : il décrit « Zones masquées » avec le texte « Raté » pour `buzz_invalidated`, sans personnage ni précision.
- `buzz_invalidated` utilise une réaction fixe (« Déçu », R6), alors que la décision #20 tire la réaction au hasard à la révélation. À trancher : réaction fixe ou tirage parmi les quatre mauvaises réponses.
- Un nom de joueur très long n'a pas été rendu : le bandeau le tronquerait (comportement du bandeau du haut), non vérifié sur cette page.

## #24 — docs(mockups): add the speed question result screens

**Contexte** — Fin du parcours SPEED (suite de #22 et #23). La décision #13 prévoit pour un SPEED « Validé » ou « Raté » seuls, sans la bonne réponse.

**Décision** (choix d'Olivier à la porte 1, 2026-11-09) —
- **Validé** (`speed-06-result-right`) : bandeau or « Validé » avec une coche, capsule or « +10 pts » (le gain), « Bonne réponse » et la réponse attendue en grand. **Raté** (`speed-07-result-wrong`) : bandeau rose « Raté », « Bonne réponse » et la réponse attendue ; **pas de points perdus**. Variante `speed-07b` : réponse de 40 caractères (maximum choisi par Olivier, comme les propositions du QCM), taille réduite sur deux lignes.
- Le verdict est précédé de la réaction du personnage (R1 à R5 si validé, R6 à R9 si raté), déjà maquettée ; seul le joueur qui a buzzé reçoit un verdict.
- Écarté : la réponse seule sans le libellé « Bonne réponse » (Olivier a validé le libellé).

**Conséquences** (si la proposition est validée) —
- **Contredit la décision #13** (verdict seul) pour le SPEED : à reporter dans le CDC §8.2 et dans le code (`LcdTextFor` : « Validé » / « Raté » aujourd'hui), session `docs(rendering)` à part. #13 n'est pas modifiée ici (le fichier est en ajout seul).
- Le gain et la réponse attendue du SPEED sont des données que le CDC §8.1 ne cite pas pour `question_result` : non vérifié contre le hub (US-011 CA-25, US-012 CA-31).
- Le bandeau du haut garde les points d'avant la question (comme `qcm-07`) : sa mise à jour au gain n'est pas spécifiée.
- Le tronquage des réponses de plus de 40 caractères n'est pas traité.

## #25 — docs(rendering): report the QCM mockups in the render table

**Contexte** — Olivier veut reporter les maquettes validées dans le CDC §8.2, en trois sessions (QCM, SPEED, réactions). Avant d'écrire, les specs du hub (US-009, US-011, US-012) ont été relues pour savoir quelles données existent.

**Décision** (choix d'Olivier à la porte 1, 2026-11-09) —
- **Trois sessions** : QCM (celle-ci), SPEED, puis réactions du personnage.
- **Habillage commun** ajouté au CDC §8.2 : bandeau du haut, en-tête avec chrono, capsules du titre. Le nom du joueur, ses points et son classement sont inscrits « à définir » ; la charge de la batterie est du code interne au firmware. Le thème et les points à gagner des capsules sont « à définir » côté hub ; le temps imparti vient de `time_limit`.
- **Arbitrages de #19 reportés tels que validés** : appui local sous l'en-tête (plus en plein écran), chrono dans l'en-tête et rouge à 5 s ou moins, « Question N » persistant, propositions sans préfixe « A : ».
- **Lignes QCM modifiées** : `question_title` (MCQ), `question_choices`, `timer_tick`, `timer_end`, `answer_received`, `question_result` MCQ et l'appui local MCQ.

**Correction de #24** — #24 affirmait que seul le joueur qui a buzzé reçoit un verdict. C'est faux : US-012 CA-31 envoie `question_result` à chaque buzzer, avec `correct: false` pour un joueur invalidé ou qui n'a pas buzzé. Le libellé « Raté » pour un non-buzzeur est à trancher à la session SPEED (choix d'Olivier). #24 n'est pas modifiée (le fichier est en ajout seul).

**Conséquences** (si la proposition est validée) —
- Le code ne change pas : `LcdTextFor`, `GfxScreen` et le rendu noir et blanc restent tels quels (voir `DEBTS.md`) ; le CDC décrit désormais une cible que le code n'atteint pas.
- Le décompte local du chrono entre deux `timer_tick` est une déduction de Claude (le hub n'en envoie qu'un toutes les 5 s) : à confirmer à l'implémentation.
- Le vocabulaire « Mettre en évidence » parle toujours d'une zone qui clignote, alors que les maquettes montrent une mise en évidence statique (dette « Zone en évidence sans clignotement ») : non touché ici.
- Les lignes SPEED du tableau (`question_open`, `buzz_*`, `question_result` SPEED, appui local SPEED) et la ligne d'init gardent leur ancien libellé jusqu'aux sessions suivantes.

## #26 — docs(rendering): report the speed question mockups in the render table

**Contexte** — Suite de #25. Deuxième des trois sessions de report : les écrans SPEED. Le hub envoie `question_result` à tous les buzzers, avec `correct: false` pour un joueur invalidé, un joueur qui n'a pas buzzé ou un joueur dont un autre a gagné (US-012 CA-31).

**Décision** (choix d'Olivier à la porte 1, 2026-11-09) —
- **Le firmware retient « ce buzzer a buzzé pendant cette question »** (option C) : posé à l'appui local sur le bouton rouge, remis à faux à `question_open`. Avec `correct: false` : s'il a buzzé, bandeau rose « Raté » et réaction « mauvaise réponse » ; sinon, bandeau sobre « Question terminée » et réaction « Endormi ». Écartés : « Raté » pour tous (faux pour qui n'a pas buzzé) ; un libellé neutre pour tous (la réaction déçue y serait étrange).
- **Un écran de plus** : `speed-08-result-no-buzz` (« Question terminée »), ajouté à cette session pour que le CDC et les maquettes restent cohérents.
- **Lignes SPEED du CDC §8.2 réécrites** : `question_open`, `buzz_accepted`, `buzz_locked`, `buzz_invalidated`, `buzz_unlocked` (même écran que l'ouverture, chrono repris à `remaining_seconds`), `question_result` (validé, raté, sans avoir buzzé) et l'appui local.
- **La décision #13** (« Validé » / « Raté » seuls) est dépassée pour le SPEED : réponse attendue et gain affichés (choix d'Olivier, #24).
- La réaction de `buzz_invalidated` est mentionnée sans trancher « fixe » ou « tirée au hasard » : renvoi à la session des réactions.

**Conséquences** (si la proposition est validée) —
- **Un état mémorisé de plus dans le rendu** : le CDC distinguait MCQ et SPEED par le contenu du message, sans état ; l'exception est écrite dans la note de §8.2. Limites : un buzzer qui a appuyé trop tard voit « Raté » ; une reconnexion en cours de question efface l'état (écran « Question terminée »).
- Le code ne change pas : voir `DEBTS.md`.
- Les libellés « Appuie sur le bouton rouge », « En attente de confirmation », « Réponds à voix haute » et « Éliminé pour cette question », proposés dans #22 et #23, sont repris dans le CDC.

## #27 — docs(rendering): report the character reactions in the render table

**Contexte** — Suite de #26. Dernière des trois sessions de report : les réactions du personnage (#20, #21). Deux points laissés ouverts : la réaction de `buzz_invalidated` (« fixe ou tirée au hasard », #26) et l'avatar par joueur, sans source ni protocole (`DEBTS.md`).

**Décision** (choix d'Olivier à la porte 1, 2026-11-09) —
- **`buzz_invalidated` : R9 « Sonné » (yeux en croix), fixe**, puis l'écran « Raté… / Éliminé pour cette question » qui reste. **R9 est réservée à cet écran** (option « réserver ») : le tirage « mauvaise réponse » se fait parmi R6, R7 et R8, pour qu'un joueur éliminé ne revoie pas les yeux en croix au résultat qui suit. Cela corrige #20 (quatre réactions de mauvaise réponse tirées au hasard) et la proposition « Déçu (R6) » de la maquette `speed-05`.
- **Durée : 2000 ms**, dans le CDC, en colonne d'un sous-tableau « Réactions du personnage », la même pour les dix réactions (#20 disait « environ 1,5 s »). Valeur indicative : la colonne permet d'en changer une seule.
- **Avatar** : la table décrit des expressions appliquées au « personnage du joueur » ; son origine n'est pas définie et reste en dette.
- **Port `Clock` et source aléatoire : non cités** dans le CDC (YAGNI) ; ils naîtront sous le premier rouge du `feat(rendering)`.
- **Lignes du §8.2** : `question_result` QCM scindé en trois cas (bonne réponse, mauvaise réponse, sans réponse), chacun précédé de sa réaction ; `buzz_invalidated` réécrit ; la note « Reporté » mise à jour.
- **Maquettes alignées** : 20 pages `reaction-*` (durée de 2 s ; trois réactions au tirage de mauvaise réponse ; R9 fixe), `speed-05` et l'index, dans le même commit.
- Écarté : laisser R9 dans le tirage (simple, mais la même tête deux fois de suite sur un joueur éliminé).

**Conséquences** (si la proposition est validée) —
- Le code ne change pas : voir `DEBTS.md`.
- **Déduction de Claude à confirmer** : la situation d'une réaction QCM se lit dans `correct` et `player_answer`. Le désérialiseur du firmware et le faux hub portent ces champs pour un QCM ; le contrat du vrai hub n'a pas été relu pour ce cas.
- **Non tranché** : un message du hub reçu pendant les 2000 ms (interrompt-il la réaction ou attend-il ?) : dette ajoutée.
- Fin du report des maquettes dans le CDC §8.2 (#25, #26, #27) ; restent sans maquette l'init, le buzz refusé, l'élimination par reconnexion et la connexion perdue.

## #28 — docs(mockups): add the start-up and waiting screens

**Contexte** — Après #27, il restait des écrans du CDC §8.2 sans maquette : l'init (logo et bandeau clignotant, §9), les deux attentes (`auth_success`, `game_resumed`) et les trois écrans de coupure (connexion perdue, mode Éliminé, login refusé). Olivier a validé un découpage en trois sessions : ces trois écrans de démarrage, puis les coupures, puis le report dans le CDC.

**Décision** (choix d'Olivier à la porte 1, 2026-11-09) —
- **Trois pages** : `start-01-init` (D1), `start-02-connected` (D2), `start-03-resumed` (D3), même habillage et même police que les autres maquettes ; index mis à jour (section « Démarrage et attente », en tête).
- **Le bandeau du haut est présent dès l'init**, notamment pour la charge de la batterie (choix d'Olivier) ; au démarrage le nom, les points et le classement sont vides et la liaison au hub est barrée en rouge.
- **D1** : le bouton rouge dessiné au-dessus du logo « Buzz’O’Live », bandeau « Connexion… » en pied d'écran, montré dans son état or (l'autre état est la teinte du bandeau du haut).
- **D2 et D3** : un anneau or avec une coche (« Connecté ») ou un triangle de lecture (« Partie en cours »), le titre en grand, « En attente d’une question » en dessous.
- Lecture de la dette : le « buzz refusé » n'est défini nulle part dans le CDC ; il est lu comme « login refusé (`401`) », avec la dette voisine « Login refusé : arrêt silencieux ».

**Conséquences** (si la proposition est validée) —
- **Libellés proposés par Claude, absents du CDC** : « Connexion… » et « En attente d’une question » ; le CDC ne dit que « Logo », « Connecté » et « Partie en cours ».
- Le logo est un dessin provisoire (le bouton rouge et le nom) : aucune charte de logo n'existe.
- Le nom « Buzz’O’Live » apparaît deux fois à l'init (bandeau du haut et logo), le bandeau étant identique sur tous les écrans.
- Le code ne change pas ; le CDC §8.2 ne décrit pas encore ces écrans (report à la troisième session) : voir `DEBTS.md`.

## #29 — docs(mockups): add the connection failure screens

**Contexte** — Après #28, restaient sans maquette les trois coupures du CDC §8.2 : connexion perdue (reconnexion, §5), mode Éliminé (§4.4) et login refusé (`401`, dette « Login refusé : arrêt silencieux »). La maquette 2d du QCM ne montrait la coupure que par l'icône de liaison barrée.

**Décision** (choix d'Olivier à la porte 1, 2026-11-09) —
- **Reconnexion : un bandeau rouge sous la barre du haut, à la place de l'en-tête** (option B), plutôt qu'un écran plein qui remplace la question : la question reste lisible, atténuée et inactive. Texte « Connexion perdue » et « Tentative N/3 » avec trois points. N est la tentative **en cours** (la perte initiale n'en compte pas ; l'échec de la 3ᵉ mène à l'écran hors jeu). Pages `fail-01-lost` (QCM, 2/3) et `fail-01b-lost-speed` (SPEED, 3/3).
- **Perte définitive : `fail-02-out`**, « Hors jeu », avec la seule sortie (redémarrer le buzzer). Le mot « éliminé » est évité : il désigne déjà l'élimination pour la question (`buzz_invalidated`, R9).
- **Login refusé : `fail-03-refused`**, « Connexion refusée » et « Préviens le maître du jeu » ; bandeau du haut sans nom ni points, comme à l'init.
- Index : section « Coupures » ; même police et même habillage que les autres maquettes.

**Conséquences** (si la proposition est validée) —
- **Libellés proposés par Claude, absents du CDC** : tous (« Connexion perdue », « Tentative N/3 », « Hors jeu », « Redémarre le buzzer pour revenir », « Connexion refusée », « Préviens le maître du jeu »).
- **Déduit par Claude, non spécifié au CDC §5** : pendant la coupure les zones et le bouton rouge sont inactifs, et le chrono disparaît (plus de resynchronisation par `timer_tick`).
- **Le compteur « N/3 » n'existe pas côté rendu** : `ReconnectionPolicy` le porte, aucun message ni port ne l'expose au rendu.
- Le code ne change pas ; le CDC §8.2 ne décrit pas encore ces écrans (report à la session suivante) : voir `DEBTS.md`.

## #30 — docs(rendering): report the start-up and connection failure mockups in the render table

**Contexte** — Après #28 et #29, les maquettes de démarrage, d'attente et de coupure existaient sans report dans le CDC §8.2 (dernière session du découpage en trois, #28). Les coupures ne naissent d'aucun message du §8.1 : elles viennent de la couche réseau (§5, §9).

**Décision** (choix d'Olivier à la porte 1, 2026-11-09) —
- **Trois lignes existantes mises à jour** : init (logo, bandeau « Connexion… », bandeau du haut dès la mise sous tension), `auth_success` (« Connecté » + « En attente d’une question »), `game_resumed` (« Partie en cours » + même précision).
- **Trois lignes ajoutées**, déclenchées par un événement réseau et notées `(réseau : …)` : connexion perdue (tentative N/3, N = tentative en cours, de 1 à 3), mode Éliminé après le 3ᵉ échec (« Hors jeu »), login refusé (`401`).
- Les deux maquettes de connexion perdue (F1 QCM, F1b SPEED) tiennent dans **une seule ligne** : l'effet est le même, seule la question atténuée change.
- Une note sous le tableau nomme les déclencheurs, le sens de N, et ce qui est déduit ou non spécifié.

**Conséquences** —
- **Déduit des maquettes, non spécifié au §5** : zones, bouton rouge et chrono inactifs pendant la reconnexion.
- **Non spécifié** : l'écran après une reconnexion réussie (`auth_success` ramène l'attente « Connecté » selon la table ; le retour à la question en cours n'est pas décrit).
- **Aucun port n'expose le compteur N/3 au rendu** : dette inchangée, voir `DEBTS.md`.
- Le code ne change pas.

## #31 — docs(mockups): add the interim ranking screen

**Contexte** — Olivier a constaté qu'aucun écran de classement n'était maquetté ni mentionné dans le CDC. Lecture du réel : le hub diffuse `intermediate_ranking` à tous les buzzers (US-014 CA-10, rang, nom, score cumulé, temps total) sans interrompre la question en cours (CA-13) ; ce message est absent du CDC §8.1 et §8.2. L'ancienne liste des états (`Liste des états.csv`) portait trois états de classement (`DisplayWinningRankingState`, `DisplayStandardRankingState`, `RevealWinnerState`), jamais repris. Aucun message du hub ne porte un classement définitif vers les buzzers (US-013 est une route REST).

**Décision** (cible et découpage validés par Olivier à la porte 1, 2026-11-09) —
- **Trois sessions** : cette maquette du classement provisoire, puis les écrans du classement définitif, puis le report dans le CDC (§8.1 et §8.2).
- **Trois pages** : `rank-01-interim` (K1, six joueurs, la ligne du joueur en blanc, cercles or, argent et bronze pour les trois premiers), `rank-01b-interim-leader` (K1b, « Tu es en tête ! », bandeau et ligne or, à la place du clignotement des LED du premier de l'ancienne liste), `rank-01c-interim-long` (K1c, plus de joueurs que de lignes : cinq premiers, « … », ligne du joueur avec son vrai rang) ; section « Classements » dans l'index.

**Conséquences** (si la proposition est validée) —
- **Confirmé par Olivier (2026-11-09)** : « provisoire » = « intermédiaire » ; dix joueurs au plus (six lignes tiennent à l'écran, K1c couvre de 7 à 10) ; la reconnaissance de sa propre ligne lui convient ; la durée d'affichage est décidée par le maître du jeu dans son animation (le firmware ne minute rien).
- **Proposé par Claude, non tranché** : l'écran remplace l'écran courant (zones masquées).
- **Libellés proposés par Claude, absents du CDC** : « Classement provisoire », « Tu es en tête ! », « N pts », temps total en secondes.
- **Le firmware reconnaît sa ligne** (principe validé) : par `participant_name` comparé au `username` d'`auth_success` ; la spec du hub ne dit pas qu'ils sont identiques, à garantir côté hub.
- **Non tranché** : ce que devient une question armée pendant que le classement est à l'écran (le hub ne l'interrompt pas, CA-13) ; même famille que la dette « Buzzer armé sans zone tactile à l'écran ».
- Le code ne change pas ; le CDC ne décrit pas encore ce message ni ces écrans : voir `DEBTS.md`.

## #32 — docs(mockups): add the final ranking screens

**Contexte** — Après #31 (classement provisoire), restait le classement de fin de partie. Aucun message du hub ne le porte vers les buzzers : la partie passe en `COMPLETED` sans diffusion, le classement final n'existe qu'en REST (US-013).

**Décision** (choix d'Olivier à la porte 1, 2026-11-09) —
- **Un podium** : les trois premiers, le premier au centre et plus haut, avec **la place du candidat en dessous** ; le classement complet n'est pas affiché, le maître du jeu le donne à voix haute.
- **Deux pages** : `rank-02-final` (K2, le candidat n'est pas sur le podium : « Tu es 5e sur 8 » et son score sur une ligne blanche) et `rank-02b-final-podium` (K2b, il est sur le podium : sa marche est cernée de blanc et son nom passe en pastille blanche) ; section « Classements » de l'index complétée.
- Pas de révélation du gagnant de l'ancienne liste des états (`RevealWinnerState`) : hors périmètre.

**Conséquences** (si la proposition est validée) —
- **Proposé par Claude, non tranché** : la ligne du bas reste la même quand le candidat est sur le podium (le firmware dessine toujours la même chose) ; la couronne sur le premier ; les libellés « Classement final » et « Tu es 5e sur 8 ».
- **Aucun message du hub ne déclenche cet écran** : hypothèse d'un message du même corps que `intermediate_ranking` envoyé à la fin de la partie, à décider côté hub.
- **Non spécifié** : une partie à moins de trois joueurs ; l'effet « gagnant » pour le premier (non demandé).
- La durée est celle du maître du jeu, comme pour le classement provisoire ; la reconnaissance de la ligne suit la décision #31.
- Le code ne change pas ; le CDC ne décrit pas encore ces écrans (report à la session suivante) : voir `DEBTS.md`.

## #33 — docs(mockups): add the waiting and game presentation screens

**Contexte** — Après #32, Olivier a demandé deux écrans absents du CDC §8.2 : « en attente du maître du jeu » et une présentation de la partie (« Quiz #08 », son thème, le nombre de questions et de joueurs). Il a aussi demandé un écran de configuration initiale avec un QR code destiné à l'application mobile ; il a d'abord dit « on verra plus tard », puis a validé un aperçu sans consigne.

**Décision** (choix d'Olivier à la porte 1, 2026-11-09) —
- **Deux pages** : `start-04-waiting-master` (D4) et `start-05-game-intro` (D5), même habillage que D1 à D3 ; index mis à jour (section « Démarrage et attente »). Trois règles nouvelles dans `mockup.css` (`.wait-title.md`, `.intro-title`, `.cap.mid`).
- **D4** : anneau or avec une horloge, « En attente du maître du jeu » sur deux lignes, « La partie va bientôt commencer » en dessous.
- **D5** : « Quiz #08 » en grand, le thème en capsule or, puis le nombre de questions et le nombre de joueurs en capsules ; valeurs d'exemple (Cinéma, 12, 6).
- **L'écran de configuration (QR) est une session séparée** : le canal d'appairage (BLE, point d'accès temporaire, autre) et l'application mobile ne sont pas décidés. Règle déjà tranchée : **aucune consigne à l'écran**, seul le maître du jeu paramètre le buzzer avec son téléphone ; l'aperçu validé montre un engrenage or, « Configuration » et le QR.

**Conséquences** (si la proposition est validée) —
- **Libellés proposés par Claude, absents du CDC** : « En attente du maître du jeu », « La partie va bientôt commencer », « Quiz #08 » ; le bandeau du haut affiche « 0 pt » sans classement avant la première question (le rang n'existe pas encore).
- **Aucun message du hub ne déclenche D4 ni D5** : à définir côté hub (numéro et thème de la partie, nombre de questions et de joueurs). **Non spécifié** : si D4 remplace « Connecté » (D2) ou lui succède, ce que « #08 » numérote (partie globale ou de la soirée), et si le nombre de joueurs se met à jour pendant l'attente.
- Le code ne change pas ; le CDC ne décrit pas encore ces écrans : voir `DEBTS.md`.

## #34 — docs(mockups): add the photo and sound screens

**Contexte** — Après #33, Olivier a indiqué que le déroulé d'une partie permet d'afficher une photo avant une question, ou de faire écouter un son. Aucun de ces deux écrans n'était maquetté ni décrit au CDC §8.2 ; `play_sound_url` et `play_system_sound` y laissent l'écran inchangé.

**Décision** (choix d'Olivier à la porte 1, 2026-11-09) —
- **Deux pages** : `pre-01-photo` (P1) et `pre-02-sound` (P2), même habillage que le reste ; index mis à jour (section « Avant la question »). Quatre règles nouvelles dans `mockup.css` (`.photo`, `.photo .meta`, `.photo .cap`, `.sound-art`), aucune couleur nouvelle.
- **P1** : photo en grande carte arrondie sous le bandeau du haut (marges des cartes de réponse, cadre blanc), capsules « Question N » et thème en surimpression en bas à gauche, sur fond opaque. Pas de points ni de temps : la question n'est pas ouverte. L'image d'exemple est dessinée.
- **P2** : haut-parleur or dans un anneau, barres d'égaliseur symétriques, « Écoute bien ! » (libellé d'Olivier), capsules au-dessus comme à l'écran du titre.
- **Seule la lecture du son est maquettée** : le chargement du fichier et son échec vont en dette (choix d'Olivier, recommandation de Claude).

**Conséquences** —
- **Aucun message du hub ne déclenche P1 ni P2** : à définir côté hub. Pour le son, `play_sound_url` laisse l'écran inchangé (§8.2) : il faudra un message ou un champ qui dise « affiche l'écran Écoute avant la question ».
- **Non spécifié** : ce qui met fin à l'écran (le message suivant, la fin du son, ou les deux) ; si la photo reste visible pendant la question ; la source de l'image (URL à télécharger, comme le son, ou fichier embarqué) et son format.
- Le code ne change pas ; le CDC ne décrit pas ces écrans : voir `DEBTS.md`.

## #35 — docs(mockups): add the game-ordered screen overview

**Contexte** — Après #34, les 59 maquettes existent mais `index.html` les groupe par famille (démarrage, coupures, QCM, rapidité…), pas dans l'ordre où un joueur les voit. Olivier a demandé une plaquette complète des écrans, dans l'ordre d'une partie.

**Décision** (choix d'Olivier à la porte 1, 2026-11-09) —
- **Une page séparée** `docs/mockups/plaquette.html`, imprimable (A4 paysage, export PDF par le navigateur), `index.html` inchangé (option B ; réordonner `index.html` lui-même avait été proposé et écarté).
- **Ordre** : démarrage, attente et présentation, avant la question, question à choix multiple, question de rapidité, réactions, classements, puis les coupures « à tout moment ».
- **Variantes en miniature** à droite de l'écran principal (recommandation de Claude acceptée) ; les réactions en planche compacte (masculin et féminin côte à côte).
- **Écrans manquants en cartes « À maquetter »** : la configuration initiale (QR code) en tête, la fin de partie après le classement final.

**Conséquences** —
- **Choix d'ordre de Claude** : F3 (login refusé) est une variante de D1 ; « bonne réponse » est l'écran principal du résultat, « fausse » et « sans réponse » ses variantes ; le classement provisoire (K1) est rangé avec le classement final, alors qu'il tombe entre deux questions.
- **Non tranché** : la répartition des réponses par proposition (« nombre de personnes par réponse », CSV d'origine) n'a ni maquette ni carte ; la question a été posée à Olivier sans réponse.
- La plaquette est une seconde liste à tenir à jour à côté de `index.html` : voir `DEBTS.md`.

## #36 — docs(mockups): add the configuration screen

**Contexte** — La décision #33 renvoyait l'écran de configuration à une session séparée et citait un aperçu validé (engrenage or, « Configuration », QR). Aucun fichier n'avait été commité, et Olivier n'en avait pas non plus en local : la plaquette (#35) l'affichait donc comme « À maquetter ». Olivier a demandé de le refaire.

**Décision** (choix d'Olivier à la porte 1, 2026-11-09) —
- **Une page** `start-06-config` (D6) ; index mis à jour (section « Démarrage et attente »). **Code D6 plutôt que D0**, pour ne pas renuméroter D1 à D5, déjà cités dans le CDC et les décisions ; la plaquette la place en tête du démarrage.
- **Mise en page** : engrenage plein or dans l'anneau (même gabarit que D2 à D4), « Configuration » à la taille des titres d'attente, QR code dans une carte blanche à droite ; **aucune consigne** (règle de #33 : seul le maître du jeu paramètre le buzzer avec son téléphone). Quatre règles nouvelles dans `mockup.css` (`.center.config`, `.config .side`, `.qr`, `.qr svg`), aucune couleur nouvelle.
- **Bandeau du haut** comme à l'écran de démarrage (D1) : sans joueur, hub injoignable (pas encore configuré), batterie.

**Conséquences** —
- **Le QR est une valeur d'exemple** (`buzzolive://setup?b=7F3A91`, 25 modules, généré avec `segno`) ; son contenu réel dépend du canal d'appairage.
- **Non décidé** : le canal d'appairage (BLE, point d'accès temporaire, autre), le déclencheur de l'écran (premier démarrage, appui long, absence de configuration), l'application mobile, et la sortie de l'écran une fois configuré. Aucun message du §8.1 ne le concerne : l'écran est local, avant toute connexion au hub.
- Le code ne change pas ; le CDC ne décrit pas cet écran : voir `DEBTS.md`.

## #37 — docs(mockups): add the end of game screen

**Contexte** — La plaquette (#35) gardait une dernière carte « À maquetter » : ce que voit le joueur après le classement final. Olivier avait choisi, avant la session, que le personnage de son avatar dise au revoir (version B) avec une réaction à trancher selon le rang.

**Décision** (choix d'Olivier à la porte 1, 2026-11-09) —
- **Une réaction unique, toujours heureuse** : `ravi`, quel que soit le rang du joueur ; jamais de réaction négative (écartées : une réaction selon le rang, une réaction négative pour les derniers). Libellé « Merci d'avoir joué ! », sans sous-texte (« À bientôt ! » n'est pas retenu).
- **Deux pages** : `rank-03-end` (K3, avatar masculin) et `rank-03-end-f` (K3b, avatar féminin, demandée par Olivier dans la même session) ; index et plaquette mis à jour (K3b en miniature à droite de K3), la carte « À maquetter » retirée.
- **K3 vient à la suite de K2** : le maître du jeu décide quand on passe de l'un à l'autre, le firmware ne minute rien (comme pour les classements).
- **Habillage des réactions** : `.react.good`, médaille et `.say` ; aucune règle ni couleur nouvelle dans `mockup.css`.

**Conséquences** —
- **Aucun message du hub ne déclenche K3** : la partie passe en `COMPLETED` sans diffusion (#32) ; à définir côté hub, avec le classement final.
- **Libellé proposé par Claude, validé par Olivier** : « Merci d'avoir joué ! » ; effet écran « Zones masquées » proposé par Claude, non tranché.
- **Non spécifié** : ce que devient l'écran ensuite (retour à l'attente, extinction, nouvelle partie) et la durée d'affichage.
- Le code ne change pas ; le CDC ne décrit pas cet écran (report à la session des classements) : voir `DEBTS.md`.

## #38 — docs(decisions): render the screens through a passive view

**Contexte** — Les maquettes (#19 à #37) fixent un rendu cible (bandeau, capsules, quadrants, personnages) que le code ne reflète pas encore. Avant d'y porter ce design system, Olivier a demandé s'il fallait un équivalent de MVVM ou de MVP sur le firmware. Le découpage existe déjà en partie : `DisplayState` (`lib/rendering/`) calcule ce qu'il faut afficher à partir d'un message, `LcdRenderer` l'applique, et l'interface `Screen` (`Show`, `ShowZones`, `HighlightZone`, `HighlightChosenZone`, `ShowBanner`) est la couture vers `GfxScreen` (adaptateur de `src/`) et vers le décorateur `TappableZonesScreen`. Trois contraintes pèsent sur la suite : le moteur de rendu n'est pas tranché (LovyanGFX aujourd'hui, LVGL candidat, écran peut-être remplacé, #19) ; le build `esp32` a refusé `<optional>` (#15), `platformio.ini` ne fixe aucun `-std` pour cet environnement ; la Flash est à 80,2 %.

**Décision** (choix d'Olivier à la porte 1, 2026-11-09) —
- **MVP en passive view, pas de MVVM.** Le flux est événementiel (un message du hub, un rendu) : un modèle poussé suffit, sans mécanisme de liaison. Seule exception à envisager, quand le bandeau du haut sera posé : la batterie et l'état du lien avec le hub, sources continues, par un petit mécanisme d'observation limité à ces deux valeurs.
- **Modèle d'écran sémantique, dans `lib/`.** Il décrit *quoi* montrer (tons, réactions, textes, zones), jamais des pixels ni des couleurs.
- **Presenter dans `lib/`** : message et état → modèle d'écran, en suivant la table du CDC §8.2. Il se teste en natif par comparaison de valeurs.
- **La couture vers l'écran existe déjà : `Screen`.** Elle est impérative (une méthode par effet) ; son évolution vers « afficher un modèle d'écran » est à décider à la session qui la touche, avec son nom. Aucune nouvelle interface n'est créée avant qu'un rouge la demande.
- **Tokens, composants et moteur de rendu côté `src/`** (couleurs, espacements, tailles, bandeau, capsules, quadrants, avatars), pour que changer de moteur reste un changement d'adaptateur.

**Conséquences** —
- **Ordre des sessions annoncé** : `chore(rendering)` de vérification de `std::variant` sur l'env `esp32` ; `refactor(rendering)` qui place les écrans existants derrière la couture, comportement inchangé, les 76 tests natifs restant verts ; puis `feat(rendering)` du bandeau du haut sur tous les écrans, première brique du design system.
- **YAGNI sur les composants** : bandeau et un seul écran d'abord, on généralise au deuxième écran qui en a besoin.
- **`std::variant` non vérifié** : probablement absent (`<variant>` est du C++17, comme `<optional>`). Repli prévu : un enum et une structure. Voir `DEBTS.md`.
- Le code ne change pas.

## #39 — chore(rendering): check that the esp32 toolchain supports std::variant

**Contexte** — La décision #38 prévoit un modèle d'écran qui est naturellement une somme de types (`std::variant`). Le build `esp32` avait refusé `<optional>` (#15) et `platformio.ini` ne fixait aucun `-std` pour cet environnement. Les tests natifs compilent en C++17 et ne pouvaient pas révéler le problème.

**Décision** (choix d'Olivier à la porte 1, 2026-11-09) —
- **L'env `esp32` compile en `-std=gnu++17`** : `build_unflags = -std=gnu++11` et `-std=gnu++17` dans `build_flags` de `[env:esp32]`. Le changement vaut pour toute la cible (code du projet et libs), pas pour les seuls fichiers qui utilisent `<variant>`.
- **`std::variant` est utilisable sur le firmware.** Le repli enum + structure prévu par #38 n'est plus nécessaire.

**Preuves** (smoke tests lancés par Olivier, 2026-11-09) —
- Sans le flag, une sonde `#include <variant>` avec `std::variant<int, char>` échoue : « `std::variant` is only available from C++17 onwards ».
- Avec le flag, la sonde compile et le build `esp32` est en `SUCCESS`, sans `error:` ni `warning:` relevé.
- Flash : 80,3 % (1 051 905 octets) contre 80,2 % (1 051 433 octets) avant le changement, soit +472 octets.
- La sonde n'est pas commitée : test né rouge, preuve de contrat et non cycle TDD.

**Conséquences** —
- **Le refus de `<optional>` (#15) avait vraisemblablement la même cause** (standard C++ trop bas, pas un manque de la toolchain ; `<optional>` n'a pas été resondé) : sa justification est à considérer comme obsolète. La décision #15 n'est pas réécrite (le fichier est en ajout seul) ; le code concerné est en dette dans `DEBTS.md`.
- **Non prouvé** : le comportement à l'exécution sur la carte (smoke test matériel non lancé) ; le changement n'affecte que la compilation, les 76 tests natifs ne sont pas concernés.
- **Ordre des sessions inchangé** : `refactor(rendering)` qui place les écrans existants derrière la couture, puis `feat(rendering)` du bandeau du haut.

## #40 — refactor(rendering): describe each screen before drawing it

**Contexte** — La décision #38 laissait à cette session la forme de la couture `Screen` : rester impérative (cinq méthodes, une par effet) ou devenir « afficher un modèle d'écran ». Le dispatch message → effet vivait dans `LcdRenderer`, celui de l'appui local dans `ScreenLocalFeedback`.

**Décision** (choix d'Olivier à la porte 1, 2026-11-09, option A) —
- **`Screen` n'a plus qu'une méthode : `Display(const ScreenModel &)`.** `ScreenModel` (`lib/rendering/ScreenModel.h`) est un `std::variant` de valeurs comparables : `TextScreen`, `ZonesScreen`, `HighlightedZoneScreen` (bandeau et évidence de la bonne réponse), `ChosenZoneScreen` (zone touchée), `BannerScreen` (bandeau, zones et zone choisie conservées) et `ScreenUnchanged` (rien à redessiner).
- **`ScreenPresenter` (`lib/rendering/`)** : message et état armé → `ScreenModel` ; `ScreenForTappedAnswer` et `ScreenForBuzzSent` pour le retour local. Il compose `DisplayState` pour les textes et les zones.
- **`LcdRenderer` et `ScreenLocalFeedback` délèguent au presenter** et n'appellent `Display` que si le modèle n'est pas `ScreenUnchanged`.
- **`TappableZonesScreen` et `GfxScreen` visitent le modèle** (`std::visit` avec `Overloaded`). `GfxScreen` est exhaustif : une variante sans dessin ne compile pas.

**Écarts à la proposition de la porte 1** (choisis par Claude, à relire en PR) —
- Cinq variantes d'écran plus `ScreenUnchanged`, et non quatre : `BannerScreen` reste distinct de `HighlightedZoneScreen` parce que la zone choisie qu'il garde en évidence vit dans l'adaptateur (`lastChosenZone`, dette « Textes des zones retenus par l'adaptateur »).
- « Rien à redessiner » est une variante du modèle plutôt qu'un `std::optional` : `std::variant` est prouvé sur l'env `esp32` (#39), `std::optional` n'a pas été resondé.

**Conséquences** —
- **Comportement voulu inchangé.** Les attentes des tests de `LcdRenderer`, `ScreenLocalFeedback`, `TappableZonesScreen` et `test_connection` sont traduites vers `Display(...)` ; le presenter naît sous 19 tests, dont 6 mutations ciblées toutes tuées.
- **`IsArmed()` est lu à chaque message** (avant : seulement sur `timer_end`). C'est un getter `const` sans effet de bord, lu au même instant (avant le traitement du message par le comportement).
- **Non prouvé** : le build `esp32` (Flash, `std::visit` sans exceptions) et le rendu à l'écran, laissés au smoke test d'Olivier ; `GfxScreen` n'a été compilé qu'à blanc contre des stubs LovyanGFX.
- **Ordre des sessions inchangé** : `feat(rendering)` du bandeau du haut sur tous les écrans vient ensuite.

## #41 — feat(rendering): show the player name in a top bar once connected

**Contexte** — La décision #38 place le bandeau du haut après la couture `Screen` (#40). Ses quatre données n'ont pas toutes une source : points et classement sont « à définir côté hub », la charge de la batterie n'est pas mesurée (non vérifié sur la CrowPanel), l'état du lien exige un redessin hors message. Une seule arrive par message : l'`username` d'`auth_success` (US-009 CA-6 du hub).

**Décision** (choix d'Olivier à la porte 1, 2026-11-09, option A ; bandeau à côté du modèle) —
- **`TopBar` (`lib/rendering/TopBar.h`)** : valeur comparable (`visible`, `playerName`), construite par `Hidden()` ou `ForPlayer(nom)`. Elle dit *quoi* montrer, jamais comment.
- **`Screen::Display(const TopBar &, const ScreenModel &)`** : le bandeau vit à côté du modèle, pas dans chaque variante, pour valoir sur tous les écrans sans toucher aux six variantes.
- **`ScreenPresenter::TopBarFor(message)`** : `auth_success` donne `ForPlayer(username)` (champ absent ou non textuel : nom vide, bandeau visible), tout autre message `Hidden()`. Sans état. `ScreenLocalFeedback` passe toujours `Hidden()`.
- **`HubMessage::Username()`**, lu par `MessageDeserializer` ; chaîne vide si absent ou non textuel, sans `HasUsername` (l'absence ne change aucun comportement).
- **`GfxScreen`** dessine le bandeau avec un `TextScreen` : nom à gauche, « Buzz’O’Live » au centre, trait dessous, 60 px de haut ; le texte est centré dans le reste de l'écran. Les autres modèles ignorent le bandeau.

**Choix de Claude, à relire en PR** —
- Le nom affiché est l'`username` du buzzer (« quiz_buzzer_01 »), pas un nom de joueur : le hub n'en fournit pas.
- Aucun état retenu : le bandeau disparaît à l'affichage suivant. Le retenir demande de partager l'état entre `LcdRenderer` et `ScreenLocalFeedback` (dette « Deux instances de `DisplayState` ») ; cela attend le deuxième écran qui le porte.
- Un test de bout en bout (`test_connection`) couvre le trajet JSON → désérialiseur → presenter → `Display`, que ni le test du désérialiseur ni celui du renderer ne couvrent seuls (mutation du désérialiseur : le test du renderer reste vert).

**Conséquences** —
- Tous les écrans existants sont inchangés (`Hidden()`) ; les attentes de test sont traduites vers la nouvelle signature.
- **Prouvé au smoke test d'Olivier (2026-11-09, faux hub)** : build `esp32` SUCCESS (Flash 80,4 %) et bandeau affiché sur la dalle, emplacement des textes conforme ; le design system n'est « pas du tout respecté » (déjà en dette). **Non prouvé** : un nom long qui chevauche le titre.
- Ordre suivant : à choisir (bandeau sur un deuxième écran, avec l'état partagé, ou autres données du bandeau).

## #42 — feat(startup): show why the login was refused

**Contexte** — Sur un `401` au login, `setup()` écrivait une ligne sur le port série puis `loop()` ne faisait plus rien : le buzzer paraissait figé sans cause visible. Le CDC §8.2 décrit l'écran (ligne « réseau : `401` au login ») et la maquette F3 le montre ; il n'était pas affiché.

**Décision** (cadrage validé par Olivier à la porte 1, 2026-11-10, première de trois cibles proposées) —
- **`ScreenPresenter::ScreenForLoginRefused()`** : un `TextScreen` de trois lignes (« Connexion refusée », « Le hub n’a pas accepté ce buzzer », « Préviens le maître du jeu »), libellés de la maquette F3 et du CDC, texte tenu par `DisplayState` comme les autres.
- **`LoginOutcomeDisplay` (`lib/startup/`)** : reçoit le `LoginResult` et n'affiche l'écran que pour `InvalidCredentials`, avec `TopBar::Hidden()`. `Success` et `Failed` ne changent rien à l'écran.
- **`main.cpp`** : un appel `loginOutcomeDisplay.Show(login)` avant l'arrêt, par `tappableZonesScreen` pour que les zones tactiles restent masquées. `startupHalted` est inchangé.

**Choix de Claude, à relire en PR** —
- Un objet dédié plutôt qu'un faux `HubMessage` (`auth_failed`) passé à `LcdRenderer` : ce démarrage n'a pas de message du hub, et en fabriquer un ferait entrer un type inexistant dans le protocole.
- Texte plein écran, sans bandeau ni anneau ni cadenas : un seul rendu `TextScreen` existe aujourd'hui ; le reste de la maquette F3 est consigné en dette (« Écran de login refusé en texte seul »).
- Le CDC n'est pas modifié : sa ligne décrit déjà le rendu cible et les libellés.

**Conséquences** —
- Un `401` n'est plus silencieux. Le comportement « pas de réessai, écran jusqu'au redémarrage » est inchangé.
- **Non prouvé** : le rendu des trois lignes sur la dalle, le build `esp32` et l'appel dans `setup()` (compilé à blanc seulement) ; à prouver par le smoke test d'Olivier avec un hub qui répond `401`.
- Olivier veut aussi un écran d'init avec le logo en grand pendant l'attente du Wi-Fi et du login : session à cadrer ensemble (le logo n'est pas encore dans le dépôt).

## #43 — docs(specs): split the firmware specification into user stories

**Contexte** — Le hub et Angular travaillent par user stories numérotées à critères d'acceptance ; le firmware n'a qu'un cahier des charges en sections (`docs/CDC-FIRMWARE-BUZZER.md`). Sans US, l'ordre numérique n'a pas de sens pour ce dépôt, et la matrice CA × couches des handoffs ne peut pas être produite.

**Décision** (cadrage validé par Olivier, 2026-11-22) —
- Le firmware est spécifié en **user stories `US-FW-001` à `US-FW-022`**, une par fichier dans `docs/specs/`, avec un index et une table de correspondance CDC → US dans `docs/specs/README.md` (principes directeurs et matériel y vivent aussi).
- Ordre : numérique, la logique et la connexion (001 à 007) avant le rendu (008 à 016) ; les US vides (017 à 022, dépendant du hub ou d'un choix matériel) ont un titre, un périmètre et un blocage, et sont sautées tant qu'elles n'ont pas de CA.
- Les US sont **complètes même pour ce qui est déjà implémenté** (CA rédigés depuis le réel, constat de couverture) ; une session `docs(specs)` par US, cadrée par interview.
- Le CDC est **supprimé quand son contenu est dans les US**, avec `docs/Quiz Buzzer Game - Liste des états.csv` (son origine), dans une session dédiée ; il reste en lecture seule d'ici là.

**Choix de Claude, à relire en PR** —
- 22 US, dont 4 couches de rendu fines (bandeau, init, coupure, animations) gardées séparées plutôt que fondues dans les US de question : une session chacune.
- Les principes et le matériel (§1, §2) vont dans le `README` de `docs/specs/` plutôt que dans une US.

**Conséquences** —
- Les numéros d'US du hub cités sont tirés du CDC et de `DEBTS.md` ; à confirmer à la rédaction de chaque US.
- À la suppression du CDC, la phrase de l'en-tête de ce journal (« consignées dans `docs/CDC-FIRMWARE-BUZZER.md` ») sera à rediriger vers les US, par une nouvelle entrée.
- Ordre suivant : à choisir ensemble, en commençant par US-FW-001 (ordre numérique).

## #44 — docs(specs): write US-FW-001, detect a press

**Contexte** — La détection d'appui est répartie sur `lib/input/` (détecteur, lecteurs, traducteur), `src/` (GPIO, GT911) et `main.cpp` (deux détecteurs). Le CDC §3 la décrit en une page, cite un anti-rebond que le code n'a pas, et laisse ouverts le glissement d'un doigt, le multi-touch et la frontière avec le filtrage par mode (`BuzzerBehavior`).

**Décision** (cadrage validé par Olivier, 2026-11-22) —
- **Frontière** : US-FW-001 s'arrête à l'appel de `OnMcqPressed` ou `OnBuzzPressed`. L'appui d'une famille non armée ou en mode Inerte, ignoré par `BuzzerBehavior`, relève de US-FW-002 et US-FW-003 ; l'activation des zones tactiles seulement quand elles sont affichées (décision #18), de US-FW-011.
- **Anti-rebond du bouton rouge** : constat reporté. Ni CA ni code avant branchement du bouton et smoke test ; dette dans `DEBTS.md`.
- **Glissement A→B sans lever le doigt** : le réel est décrit et assumé. Chaque changement de zone est un appui (CA-6).
- **Multi-touch** : le réel est décrit, seul le premier point du GT911 compte (CA-7).
- **Doigt posé à l'apparition des zones** : constat reporté, déjà en dette (décision #18).

**Choix de Claude, à relire en PR** —
- 8 CA, tous « logique » : le CA-8 (deux détecteurs indépendants dans `loop()`) est ajouté pour fixer le câblage réel, alors qu'aucun test ne le couvre.
- Les écarts de preuve (bords inclus, milieu de la projection, source tactile du détecteur) sont regroupés dans une dette plutôt que dans autant de CA.

**Conséquences** —
- Sous le lock-first, le glissement envoie la **première zone effleurée**, pas la zone visée : acceptable tant qu'un appui franc tombe sur la bonne zone ; à rouvrir si le smoke test tactile montre des appuis involontaires.
- Le CDC §3 est migré pour la détection ; les règles « ignoré » restent à rédiger dans US-FW-002, US-FW-003, US-FW-004 et US-FW-011 avant sa suppression.
- Deux dettes ajoutées : anti-rebond du bouton rouge, preuves manquantes de la détection.

## #45 — docs(specs): write US-FW-002, answer a multiple-choice question

**Contexte** — Le CDC §8.1 n'arme un mode que depuis Inerte (colonne « Mode(s) pertinents » de `question_choices`, `question_open`, `buzz_unlocked`) et le §6 n'a de transition d'armement que depuis Inerte. Le code, lui, écrase le mode sans condition (`OnQuestionChoices` pose `McqArmed` quelle que soit la source). La lecture du code a aussi montré que `BuzzerBehavior::OnEliminated()` n'a aucun appelant hors tests : le mode Éliminé du §4.4 n'est pas tenu en production.

**Décision** (cadrage validé par Olivier, 2026-11-22) —
- **Armement hors Inerte** : un message d'armement reçu alors que le buzzer est déjà armé est **ignoré** (US-FW-002 CA-2), comme le dit le CDC. Vaut pour les trois messages, donc pour US-FW-003. Le code ne le fait pas encore : écart consigné en dette, correctif à venir.
- **Éliminé** : un CA « Éliminé ignore tout appui » (CA-9) est écrit dans US-FW-002, testé dans `BuzzerBehavior` mais non câblé ; le câblage et le passage en Éliminé restent à US-FW-006. Écart consigné en dette.
- **Appui pendant une coupure** : constat reporté en dette (envoi sans contrôle du retour de `sendTXT`), pas de CA : le hub ne gère pas encore la reprise (US-021) et le CDC est muet.
- **Périmètre** : les règles communes aux deux modes armés (Inerte, `timer_end`, Éliminé, armement hors Inerte) sont dans US-FW-002, formulées pour « tout mode armé » ; US-FW-003 y renvoie et ne garde que le propre au bouton rouge.

**Choix de Claude, à relire en PR** —
- 9 CA, tous « logique » : le feedback local n'est ici qu'un appel (`AcknowledgeAnswer`), son contenu relève de US-FW-011.
- Les écarts de preuve (verrou avant feedback, lettres `B` et `D`, réarmement, `timer_end` en Inerte, assertions manquantes) sont regroupés dans une dette, comme la décision #44, sauf le verrou avant feedback qui garde son propre CA (CA-3) parce que c'est le cœur du lock-first.
- Statut de US-FW-002 passé de « Implémentée » à « Partielle », à cause de CA-2 et CA-9.

**Conséquences** —
- Avec CA-2, l'écran et le mode divergent : `LcdRenderer` rend le message avant le dispatch (décision #5), donc un `question_choices` reçu pendant un SPEED armé afficherait des zones sur lesquelles les appuis seraient ignorés. Le rendu conditionnel au mode n'est pas décidé ; à trancher avec US-011 et US-012.
- Un hub qui n'enverrait pas `timer_end` avant la question suivante laisserait le buzzer bloqué sur l'ancien mode (voir la dette).
- Quatre dettes ajoutées : armement depuis un mode déjà armé, Éliminé jamais propagé, envoi sans contrôle pendant une coupure, preuves manquantes de la réponse MCQ.
- Le CDC §4.1, §4.2 et §6 sont migrés pour la part MCQ, Inerte et Éliminé-appui ; la part SPEED reste à US-FW-003.

## #46 — docs(specs): write US-FW-003, buzz on a speed question

**Contexte** — Le CDC §8.1 donne à `buzz_locked` « aucun effet sur le mode (un autre a buzzé ; ce buzzer est déjà passif) ». C'est vrai du buzzer qui a appuyé (il est Inerte par le lock-first), faux des autres : le hub envoie `buzz_locked` à tous les autres buzzers (US-012 CA-16), qui sont encore SPEED armés, et n'envoie pas `timer_end` aux buzzers pendant `QUESTION_BUZZED` (CA-11). Le code ignore `buzz_locked` dans `HubMessageDispatcher`.

**Décision** (cadrage validé par Olivier, 2026-11-22) —
- **`buzz_locked` désarme un SPEED armé** (US-FW-003 CA-8) : le buzzer passe en Inerte, `buzz_unlocked` le réarme ensuite (CA-2), comme le §6 le décrit (Inerte → SPEED armé). Écart assumé avec le CDC §8.1, qui reste inchangé jusqu'à sa suppression.
- **`buzz_locked` hors SPEED armé est sans effet** (CA-9) : Inerte reste Inerte, un MCQ armé reste armé (message hors contexte, CDC §3).
- **`question_result` ne désarme pas** : pas de filet de sécurité commun aux deux modes (YAGNI). Un `buzz_locked` ou un `timer_end` perdu relève de la reprise de connexion (US-FW-006, US-021).
- **`buzz_accepted` et `buzz_invalidated` restent sans effet sur le mode** (CA-10) : le buzzer concerné est déjà Inerte.

**Choix de Claude, à relire en PR** —
- 10 CA, tous « logique » ; le bouton rouge physique n'a pas de CA (détection dans US-FW-001, matériel non branché), il est en constat reporté avec sa preuve attendue (smoke test d'Olivier).
- Le perdant d'une course (il appuie en même temps qu'un autre) reste Inerte et affiche « Buzz envoyé » jusqu'à son `buzz_locked` : consigné en constat, sans CA, parce que c'est le lock-first voulu.
- Statut de US-FW-003 passé d'« Implémentée (matériel non testé) » à « Partielle », à cause de CA-8.

**Conséquences** —
- Écart de code : `HubMessageDispatcher` n'aiguille pas `buzz_locked` vers `BuzzerBehavior`. Consigné en dette (« Non-buzzeurs restés armés sur `buzz_locked` »), correctif `fix(game)` à grouper avec l'armement hors Inerte (#45).
- Sans ce correctif, un non-buzzeur reste armé après une question SPEED validée et, avec #45, n'est pas armé par la question MCQ suivante.
- L'écran et le mode restent cohérents : `buzz_locked` est rendu avant le dispatch (#5), le désarmement suit.
- Le hub n'émet pas encore ces messages ; le comportement repose sur US-012 du hub, lu le 2026-11-22.
- Deux dettes ajoutées : non-buzzeurs restés armés, preuves manquantes du bouton rouge SPEED.
- Le CDC §4.1, §4.3, §6 et §8.1 sont migrés pour la part SPEED ; il reste à rédiger US-FW-004 (reste du §8.1), US-FW-005, 006 et 007 avant sa suppression.

## #47 — docs(specs): write US-FW-004, dialogue with the hub

**Contexte** — Le dialogue avec le hub est réparti sur `lib/connection/` (événements, aiguillage session), `lib/dispatch/` (répartiteur), `lib/serialization/` (lecture et écriture JSON) et l'adaptateur `src/WebSocketEventListener`. Le CDC §3 dit qu'un message « non reconnu / invalide / hors contexte » est ignoré, et le §8.1 liste les messages. Une sonde de `MessageDeserializer` (compilée hors dépôt, 2026-11-22) a montré que seul le premier cas est tenu : un `type` absent ou non textuel devient la chaîne « null » ou « 42 » (ignoré par hasard), mais un JSON tronqué ou suivi de texte parasite est accepté et arme le buzzer. Le firmware n'a aucun état « authentifié », alors que le hub n'émet rien avant `auth_success` (US-009 CA-22).

**Décision** (cadrage validé par Olivier, 2026-11-22) —
- **JSON invalide ignoré en entier** (US-FW-004 CA-7) : tout texte qui n'est pas un objet JSON complet à `type` textuel est ignoré, ni rendu ni aiguillage ni envoi. Le CDC §3 fait foi ; le code ne le tient pas, écart consigné en dette, correctif à venir.
- **Message de jeu avant `auth_success`** : constat reporté, ni CA ni code. À rouvrir avec la reprise de connexion (US-FW-006, US-021) ; il faudrait alors un état « authentifié » remis à zéro à chaque reconnexion.
- **Frontière** : US-FW-004 spécifie le **format** des quatre messages émis (`auth`, `auth_refresh`, `answer`, `buzz`) ; quand `auth` part et avec quel token reste à US-FW-005, `auth_refresh` et `token_expired` à US-FW-007.
- **Messages sans effet** : deux CA, un pour les types connus qui ne changent jamais le mode (CA-5), un pour les types inconnus et les `error` du hub (CA-6). Les `error` du hub ne sont pas remontés (CDC §3, YAGNI).

**Choix de Claude, à relire en PR** —
- 12 CA, dont CA-12 (événements WebSocket non textuels ignorés) qui fixe le comportement d'un adaptateur de `src/` jamais testé en natif ; gardé à la demande d'Olivier plutôt que laissé en simple constat.
- CA-2 (rendu avant aiguillage, décision #5) et CA-8 (champ absent ou mal typé = absent, décision #6) sont repris tels quels depuis le code et les tests, sans nouvelle règle.
- Statut de US-FW-004 passé d'« Implémentée » à « Partielle », à cause de CA-7.

**Conséquences** —
- Écart de code : `MessageDeserializer` accepte un JSON incomplet ou suivi de texte et rend un type « null » ; consigné en dette (« JSON invalide accepté »), correctif `fix(serialization)` à faire avec ses neuf tests.
- Deux autres dettes ajoutées : preuves manquantes du dialogue (types sans effet, type inconnu, `error`, `answer` pour `C` et `D`, adaptateur), message de jeu avant `auth_success`.
- Le CDC §3 (règle « ignoré ») et §8.1 (lecture, aiguillage, messages émis) sont migrés ; il reste à rédiger US-FW-005, 006 et 007 avant sa suppression (§8.1 `token_*`, §5, §9).

## #48 — docs(specs): write US-FW-005, start up and authenticate

**Contexte** — Le démarrage vit dans `src/main.cpp` (`setup()` bloquant), `lib/startup/` (`TokenAcquisition`, `LoginOutcomeDisplay`), `lib/session/` (`TokenSessionMessageSender`) et l'adaptateur `src/HttpTokenFetcher`. Le CDC §9 dit « un token frais à chaque connexion, reconnexions comprises », « un seul essai : le dernier JWT connu sinon », et que le compteur est inactif avant `auth_success`. Le code obtient un premier token dans `setup()`, puis `SendAuth()` en redemande un à l'ouverture du WebSocket : deux logins au démarrage. Sonde de `LoginResponseDeserializer` (hors dépôt, 2026-11-22) : tout corps sans `token` texte donne un échec, un corps tronqué après un token complet est lu quand même. `LoginRequestSerializer` n'échappe pas les identifiants.

**Décision** (cadrage validé par Olivier, 2026-11-22) —
- **Deux logins au démarrage** : décrits tels quels (US-FW-005 CA-9), conformes au CDC « à chaque connexion » ; pas de CA « un seul login ». La dette existante reste.
- **Échecs hors `401`** : tout échec du login (réseau, `400`, `429`, `5xx`, `200` sans token) est réessayé toutes les 5 s, sans plafond ni recul (CA-4), comme le CDC « tente indéfiniment ». Un `400` n'arrête pas le buzzer ; dette « Seul `401` arrête le login » conservée.
- **Échappement des identifiants** : CA-6, **non implémenté** ; le corps du login doit rester un JSON valide pour un `"` ou un `\`. Dette `fix(provisioning)` d'une ligne et deux tests.
- **`401` pendant une reconnexion** : constat reporté, ni CA ni code ; à rouvrir avec US-FW-006.
- **Mode Inerte** : le buzzer est Inerte dès la mise sous tension et `auth_success` ne change pas le mode (CA-11) ; le CDC §9 (« entrée en Inerte à `auth_success` ») est lu comme ce comportement, sans état d'init.

**Choix de Claude, à relire en PR** —
- 11 CA, tous « logique » ; trois (CA-1, CA-2, CA-8) n'ont comme preuve qu'un constat, car la séquence et l'adaptateur HTTP sont dans `src/`.
- Le CA-6 est une exigence que le CDC ne contient pas : elle vient d'un défaut de code observé (corps invalide, réessai à vie), pas d'une décision de produit.
- La frontière avec US-FW-010 (rendu d'init) et US-FW-015 (écran de refus) est celle de la décision #43 : ici, seul le fait que l'écran de refus est affiché compte.

**Conséquences** —
- Écart de code : identifiants non échappés (CA-6). Trois dettes ajoutées : identifiants non échappés, preuves manquantes du démarrage, `401` à la reconnexion.
- Le CDC §8.2 (logique du refus `401`) et §9 sont migrés ; il reste à rédiger US-FW-006 et US-FW-007 avant sa suppression (§4.4, §5, §6, §8.1 `token_*`).

## #49 — docs(specs): write US-FW-006, reconnect or be eliminated

**Contexte** — Le compteur d'élimination vit dans `lib/reconnection/ReconnectionPolicy.h` et reçoit `OnReconnectFailed` à chaque `WStype_DISCONNECTED` (`ConnectionEventHandler::OnConnectionLost`). Le CDC §5 compte « les tentatives de reconnexion infructueuses », perte initiale exclue, et parle de « reconnexion WiFi ». Lecture de `WebSockets` 2.7.1 : l'événement n'est émis qu'à la fermeture d'une connexion TCP ouverte, jamais sur un connect raté ; `setReconnectInterval` n'est pas appelé (500 ms par défaut), donc trois tentatives durent environ 1,5 s. `BuzzerBehavior::OnEliminated()` n'a aucun appelant en production et le client continue ses tentatives après l'élimination. Le hub ne ré-arme pas un buzzer à la reconnexion (`game_resumed` sans question : US-019 CA-17, CA-18).

**Décision** (cadrage validé par Olivier, 2026-11-22) —
- **Échec compté** : chaque tentative de reconnexion qui n'aboutit pas à un `auth_success`, perte initiale exclue (CA-5 à CA-7). Le CDC est suivi ; l'écart du code (le compteur compte des fermetures, la première comprise, et ne voit pas un connect raté) est une dette `fix(reconnection)`.
- **Liaison** : le WebSocket vers le hub, Wi-Fi coupé et hub arrêté comptés pareil (CA-7). Le CDC §5 « reconnexion WiFi » est lu ainsi ; le CDC n'est pas modifié dans cette session.
- **Intervalle** : 5 s fixes, sans recul (CA-8), soit environ 15 s de panne avant l'élimination et 12 tentatives par minute au plus, sous la limite du hub (15 par minute).
- **Codes de fermeture** : 4001 à 4006 comptent pareil, aucun recul par code (CA-5).
- **Éliminé** : silence total, réseau compris (CA-9, CA-10) : plus d'appui, d'armement, de rendu ni de message émis, plus de tentative ni de login ; seul un redémarrage physique reprend.
- **`401` en reconnexion** : arrêt comme au démarrage, hors compteur, écran « Connexion refusée » (CA-11), distinct de « Hors jeu ».
- **Mode armé pendant la coupure** : constat reporté, ni CA ni code ; dette conservée.

**Choix de Claude, à relire en PR** —
- 11 CA, tous « logique » : CA-1 à CA-5 sont tenus par des tests existants, CA-6 à CA-11 ne sont pas implémentés. CA-5 reste « testé » alors que son test compte aussi la perte initiale ; le défaut est porté par CA-6, pas par CA-5.
- Les CA-7, CA-8 et CA-10 reposent sur une lecture de la bibliothèque 2.7.1 et du code, pas sur le matériel ; la version 2.7.3 de `platformio.ini` n'a pas de tag dans le dépôt de la bibliothèque et n'a pas été lue.
- L'arrêt sur `401` (CA-11) et le silence de l'Éliminé (CA-9, CA-10) sont deux arrêts terminaux distincts : deux causes, deux écrans (US-FW-015), un seul redémarrage pour en sortir.
- Le statut de l'US dans l'index passe de « Implémentée » à « Partielle » : l'élimination n'était pas câblée.

**Conséquences** —
- Écarts de code : compteur de fermetures (CA-6, CA-7), intervalle (CA-8), propagation de l'élimination et réseau actif (CA-9, CA-10), `401` ignoré (CA-11). Deux dettes ajoutées (compteur de fermetures, preuves manquantes), quatre réécrites (Éliminé non propagé, `401` en reconnexion, reconnexion sans recul, envoi pendant la coupure) et une revue (message de jeu avant `auth_success`).
- Un hub injoignable plus de 15 s élimine les buzzers sans retour avant redémarrage physique : assumé, à rouvrir si le hub redémarre souvent pendant une partie.
- Le CDC §4.4, §5, §6 (passage en Éliminé) et §9 (articulation avec le compteur) sont migrés ; restent à rédiger US-FW-007 (§8.1 `token_*`) et les US de rendu avant la suppression du CDC.


## #50 — docs(specs): write US-FW-007, keep the session open

**Contexte** — Le CDC §8.1 dit que `token_expiring_soon` appelle `auth_refresh` « (renouvellement JWT) » et que `token_expired` est suivi de la fermeture par le hub. Le firmware envoie `auth_refresh` avec le dernier token, celui de la connexion en cours : même `exp`. US-021 du hub (CA-1 : délai ≤ 0 = envoi immédiat, CA-11 : reprogrammation des timers sur le nouvel `exp`) renverrait aussitôt `token_expiring_soon`, en boucle. Le rendu affiche « Connecté » et le bandeau à chaque `auth_success`, y compris celui qui répond à un `auth_refresh`. Le hub n'implémente pas US-021 (`quiz-live-server` à `ef57459`).

**Décision** (cadrage validé par Olivier, 2026-11-22) —
- **Renouvellement** : un login frais avant chaque `auth_refresh`, qui porte ce token et jamais l'ancien (CA-2).
- **Échec du login** (réseau, `400`, `429`, `5xx`, `200` sans token) : aucun `auth_refresh`, aucune relance ; le token expire, `token_expired`, `4002`, reconnexion (CA-3). Différence voulue avec `auth` (décision #48, repli sur le dernier token).
- **`401` au login de renouvellement** : aucun `auth_refresh`, aucun arrêt ; la session continue jusqu'à `token_expired`, puis la reconnexion reçoit `401` et s'arrête par US-FW-006 CA-11 (CA-4).
- **Rafale de `token_expiring_soon`** : un renouvellement par message, sans filtre ni état (CA-5).
- **`auth_success` de réponse** : sans effet sur l'écran, le bandeau et le mode (CA-6), alors que celui d'une ouverture de connexion s'affiche (US-FW-010).
- **Token frais à chaque connexion** : hors périmètre, US-FW-005 CA-9 ; décision #48 inchangée.

**Choix de Claude, à relire en PR** —
- 7 CA, tous « logique » : CA-1 tenu par des tests, CA-5 et CA-7 implémentés sans test, CA-2, CA-3, CA-4 et CA-6 non implémentés.
- CA-6 n'était pas dans le CDC : il vient de la lecture de `DisplayState` et `ScreenPresenter`. Il impose un état « `auth_refresh` en attente » (ou équivalent) côté firmware ; la forme est laissée au `fix`.
- CA-4 dépend de US-FW-006 CA-11, non implémenté : sans lui, la reconnexion qui suit `token_expired` renvoie `auth` avec un token expiré et reboucle.
- Les CA reposent sur la lecture du hub, qui n'émet aucun message `token_*` : rien n'est vérifié de bout en bout.
- Le statut de l'US dans l'index reste « Partielle ».

**Conséquences** —
- Écarts de code : `SendAuthRefresh` sans login frais (CA-2, CA-3, CA-4), rendu de l'`auth_success` de réponse (CA-6). Une dette ajoutée (« Renouvellement du token : preuves manquantes »), une réécrite (« `auth_refresh` sur `token_expiring_soon` »), deux complétées (« Un `POST /api/v1/token` synchrone », « `401` au login pendant une reconnexion »).
- Une panne de login au moment du renouvellement coupe la connexion au plus 300 s plus tard (reconnexion de quelques secondes, message de jeu d'un mode armé perdu : décision #49).
- Le CDC §8.1 (`token_*`, `auth_refresh`) est migré ; restent à rédiger `play_*` (US-FW-021), §8.2 et les US de rendu avant la suppression du CDC.

## #51 — docs(specs): write US-FW-008, show readable texts

**Contexte** — Le rendu des textes (décisions #7 à #11) est livré et testé, mais deux choses n'étaient décidées nulle part. Les polices Oswald ne couvrent que Latin-1 et `Œ œ ’ … €` : LovyanGFX 1.2.36 dessine tout autre code comme un blanc de 7 px (glyphe espace) et ne dessine pas un code < 0x20, alors que la mesure compte 14 px (`?`, décision #9). Or le hub accepte les émojis dans un titre (US-005 CA-4, 10 à 250 caractères) ; une proposition ou une réponse fait 1 à 40 caractères (CA-9, CA-12). Un titre « Quel drapeau : 🇧🇷 ? » s'afficherait avec un trou invisible. Par ailleurs la règle « un signe de ponctuation ne commence pas une ligne » (`? ! : ; » )`, puis `«`) n'a été consignée dans aucune décision (ajoutée par les `fix(rendering)` des PR #35 et #36).

**Décision** (cadrage validé par Olivier, 2026-11-22) —
- **Codes sans glyphe** : tout code d'un texte affiché que la police n'a pas est remplacé par un `?` visible, un par code, avant la mise en page ; les codes < 0x20 sont retirés, sauf `\n`. La mesure et le dessin comptent le même `?` (US-FW-008 CA-10).
- **Signes typographiques** : `–` (U+2013), `—` (U+2014), `“` (U+201C) et `”` (U+201D) rejoignent `EXTRA_CODES` et les trois polices (CA-12). Ils précèdent ou accompagnent CA-10.
- **Ponctuation** : la liste `? ! : ; » )` (mot fait seulement de ces signes) et le guillemet ouvrant `«` sont consignés tels quels (CA-4). `%`, `€` et `…` isolés par une espace restent hors liste (constat reporté).
- **Dessin** : seuls le centrage, la marge de 10 px, la plus grande taille qui tient et les trois corps 30, 38 et 48 px sont des CA (CA-11, CA-13). Le blanc sur noir et l'effacement avant chaque texte n'en sont pas : le rendu cible des maquettes les remplace.
- **Vue passive** (décisions #38, #40) : contrainte de conception notée au périmètre, sans CA.
- **Taille commune aux quatre zones** : renvoyée à US-FW-011. La maquette (décision #19) la veut, le code choisit une taille par texte.

**Choix de Claude, à relire en PR** —
- 13 CA : 8 « logique » tenus par des tests (CA-1 à CA-8), 1 « logique » implémenté sans test (CA-9), 1 « logique » non implémenté (CA-10), 2 « adaptateur » implémentés sans test (CA-11, CA-13), 1 « adaptateur » non implémenté (CA-12). Le statut de l'US passe de « Implémentée » à « Partielle ».
- Un accent décomposé s'affichera « e? » : le remplacement ne normalise pas, par YAGNI (le hub envoie vraisemblablement du NFC, non vérifié). Les marques combinantes ne sont donc pas retirées en silence.
- CA-7 (débordement sans rognage) et CA-9 (zone sous bandeau, 160 px) décrivent le code tel qu'il est, sonde à l'appui ; ni l'un ni l'autre n'a été vu sur la dalle.
- Le CA de lisibilité à 50 cm (CA-11) s'appuie sur les smoke tests d'Olivier des 2026-11-03 et 2026-11-04, non rejoués ici.
- Alternatives écartées au cadrage : tout en `?` sans toucher aux polices ; blanc actuel spécifié tel quel ; demander au hub de refuser les émojis ; étendre la ponctuation à `% € …` ; CA sur le contrat du presenter ; CA sur les couleurs.

**Conséquences** —
- Écarts de code : aucun code sans glyphe n'est remplacé (CA-10), quatre signes manquent aux polices (CA-12). Dettes : une réécrite et renommée (« Codes sans glyphe : un blanc invisible à l'écran », ex-« Signes typographiques français encore absents »), une corrigée (« Garantie des 250 caractères : texte réaliste, pas pire cas » : le pire cas mesuré est 16 lignes, 640 px, avec des mots de 14 `¼`, et 13 lignes, 520 px, avec des mots de 18 `W`), deux complétées (« Largeurs de test copiées de la police », « Dépendances flottantes »), trois ajoutées (« Texte : preuves manquantes », « Une taille par texte, pas par jeu de zones », « Ponctuation `%` `€` `…` isolés »).
- Ordre des `fix` : `feat(rendering)` des quatre signes (CA-12, polices et fixture régénérées), puis `fix(rendering)` du remplacement par `?` (CA-10).
- Le CDC §8.2 (« 250 caractères au plus, taille réduite s'il est long », ligne `question_open`) est migré pour sa part « texte » ; restent à rédiger les US de rendu 009 à 016 avant la suppression du CDC.

## #52 — docs(specs): write US-FW-009, show the top bar

**Contexte** — Le bandeau du haut n'existe que sur l'écran « Connecté » : le nom y est l'`username` du compte buzzer (décision #41), sans état. Le CDC §8.2 et la maquette (#19) le veulent sur chaque écran avec nom, points, rang, liaison et batterie. Le hub n'envoie ni nom de participant (assignation au passage `PENDING → OPEN`, US-010 CA-65, sans message), ni rang ; `cumulative_score` arrive dans `question_result` et `game_resumed` (US-019 CA-18) ; la batterie n'est mesurée nulle part.

**Décision** (cadrage validé par Olivier, 2026-11-22) —
- **Nom** : le bandeau montre le **nom du participant** (US-FW-009 CA-2, non implémentable : aucun message du hub). Tant qu'il n'est pas reçu, **repli sur l'`username` du compte** (CA-1, tenu). Cela remplace la décision #41 (« le nom est l'`username` du buzzer ») pour la cible.
- **Présence** : le bandeau, une fois `auth_success` reçu, reste sur chaque écran jusqu'au redémarrage, feedback local compris (CA-3, CA-7). Il suppose un état partagé entre `LcdRenderer` et `ScreenLocalFeedback`.
- **Liaison** : état « liée » ou « coupée », bandeau redessiné sans message (CA-4, CA-8).
- **Troncature** du nom par « … » dans la colonne de gauche, comme la maquette (CA-5).
- **Reportés** : points, rang, batterie, hauteur et habillage de la maquette, logo animé, variantes de l'init, du `401` et des coupures (US-FW-010, 015).

**Choix de Claude, à relire en PR** —
- 8 CA : 1 « logique » tenu par des tests (CA-1), 5 « logique » non implémentés (CA-2 à CA-5, dont CA-2 bloqué par le hub), 1 « adaptateur » implémenté sans test (CA-6), 2 « adaptateur » non implémentés (CA-7, CA-8). Statut de l'US : « Partielle ».
- La troncature (CA-5) n'a pas été posée en question : la maquette la propose (#19) ; la largeur de la colonne de gauche est un calcul de Claude, pas une mesure.
- La liaison « coupée » est définie sur la perte de la connexion établie : un hub arrêté après la perte ne produit pas d'événement (dette « Compteur d'élimination »), le CA n'est donc pas complet tant que cette dette tient.
- Le calcul « zone de 380 × 190 px sous le bandeau » (constat) n'a pas été sondé.
- Alternatives écartées au cadrage : nom de compte seul, nom absent ; points seuls depuis `cumulative_score`, points et rang avec un message hub à créer ; bandeau limité aux écrans de texte, présence en constat reporté ; liaison et batterie en CA, ou toutes deux reportées ; repli du nom vide ou en tiret.

**Conséquences** —
- Écarts de code : bandeau non retenu (CA-3), non dessiné hors texte (CA-7), liaison absente (CA-4, CA-8), troncature absente (CA-5), nom du participant sans source (CA-2). Dettes : une réécrite (« Bandeau du haut sur le seul écran « Connecté » »), une complétée (« Rendu cible des maquettes partiellement reporté »), trois ajoutées (« Nom du participant absent du protocole », « Bandeau : points, rang et batterie sans source », « Bandeau : preuves manquantes »).
- Les tests qui figent `Hidden()` hors `auth_success` (`MessageWithoutTopBarTest`, attentes de `test_lcd_renderer`, `test_local_feedback`, `test_connection`) contredisent CA-3 : à réécrire avec le `feat`, pas avant.
- Le bandeau de 60 px réduit les zones de US-FW-008 CA-9 (calcul : 380 × 190 px) : à mesurer avec US-FW-011.
- Le CDC §8.2 (habillage commun) est migré pour sa part « bandeau du haut » ; restent à rédiger les US de rendu 010 à 016 avant la suppression du CDC.


## #53 — docs(specs): write US-FW-010, show the start-up and waiting screens

**Contexte** — Le réel n'a ni écran d'init (`setup()` fait `lcd.fillScreen(TFT_BLACK)` puis attend le Wi-Fi et le login : écran noir), ni sous-titre d'attente : « Connecté » et « Partie en cours » sont des textes d'une ligne. Le CDC §8.2 et la maquette D1 (décision #28) veulent un logo, un bandeau « Connexion… » clignotant et le bandeau du haut dès la mise sous tension (batterie seule). US-FW-009 CA-3 et la décision #52 disaient le contraire : aucun bandeau avant le premier `auth_success`. `setup()` est bloquant (dette) : un clignotement exige une horloge et un démarrage non bloquant. Le hub envoie `auth_success` ; il n'envoie pas `game_resumed` (aucune occurrence dans `src/` à `ed6bd31`).

**Décision** (cadrage validé par Olivier, 2026-11-22) —
- **Écran d'init statique** (CA-1, CA-2) : dessiné une fois dans `setup()` avant l'attente du Wi-Fi, il reste jusqu'au premier `auth_success`, sans clignotement ni message d'erreur. Texte de trois lignes : « Buzz’ », « O’Live », « Connexion… ».
- **Bandeau dès la mise sous tension** (CA-1) : titre « Buzz’O’Live », nom vide, liaison « coupée », sans batterie. Cela **revoit** le « Présence » de #52 (bandeau à partir d'`auth_success`) : US-FW-009 CA-1, CA-3 et CA-4 sont corrigés dans la même PR.
- **Deux lignes** (CA-4) : « Connecté » ou « Partie en cours », puis « En attente d’une question », même taille. Anneau et icône en constat reporté.
- **`game_resumed`** (CA-5, CA-6) : texte fixe, champs du message ignorés ; l'enchaînement `auth_success` puis `game_resumed` donne deux rendus, décrits tels quels.
- **Reportés** : clignotement et habillage de D1 à D3, batterie, bandeau du `401`, mode armé pendant la coupure, `auth_success` de réponse à `auth_refresh`.

**Choix de Claude, à relire en PR** —
- 7 CA : 1 tenu (CA-3), 3 implémentés sans test de bout en bout ou de dessin (CA-5 à CA-7), 3 non implémentés (CA-1, CA-2, CA-4) ; 8 constats reportés. Statut de l'US : « Partielle ».
- « Connexion… » est écrit **sous** le logo, trois lignes de même taille, et non en pied d'écran comme la maquette : c'est la seule forme que permet le modèle `TextScreen` (taille unique, texte centré). Le logo en texte et les libellés viennent de #28, pas du CDC.
- US-FW-009 est corrigée sans changer la numérotation de ses CA ; `README` et dettes suivent. Les CA de liaison de US-FW-009 (CA-4, CA-8) restent non implémentés : la liaison « coupée » de l'init n'est qu'un texte de spécification tant que le port n'existe pas.
- Alternatives écartées au cadrage : init clignotant (démarrage non bloquant d'abord), constat reporté sans CA d'init ; pas de bandeau à l'init (US-FW-009 CA-3 inchangé) ; une ligne comme aujourd'hui, ou sous-titre avec anneau et icône ; numéro de question dans « Partie en cours », « Connecté » retenu quand `game_resumed` suit.
- Aucun test, aucun code, aucun smoke test dans cette session.

**Conséquences** —
- Écarts de code : aucun écran d'init (CA-1, CA-2), sous-titre absent (CA-4), `game_resumed` sans bandeau (CA-5, par US-FW-009 CA-3). Dettes : quatre complétées (« `setup()` bloquant pendant l'init », « Bandeau du haut sur le seul écran « Connecté » », « Écran de login refusé en texte seul », « Table de rendu §8.2 incomplète »), une complétée pour les messages jamais émis par le hub (`game_resumed`), une pour le rendu cible, deux ajoutées (« Écrans d'init et d'attente : preuves manquantes », « Écrans d'init et d'attente : habillage de la maquette reporté »).
- Les tests qui figent « Connecté » et « Partie en cours » seuls (`DisplayStateTest`, `ScreenPresenterTest`, `LcdRendererTest`, `ConnectionEventHandlerRenderingTest`) et `Hidden()` pour `game_resumed` (`MessageWithoutTopBarTest`) sont à réécrire avec les `feat`, pas avant.
- Le bandeau du refus `401` est tenu pour celui de l'init (nom vide, liaison « coupée ») : à confirmer par US-FW-015.
- Le CDC §8.2 (lignes init, `auth_success`, `game_resumed`) et §9 (rendu d'init) sont migrés ; restent à rédiger les US de rendu 011 à 016 avant la suppression du CDC.

## #54 — docs(specs): write US-FW-011, show a multiple-choice question on screen

**Contexte** — Le réel affiche les quatre propositions plein écran, sans en-tête ni chrono ; un `timer_tick` y remplace tout par « N s » en plein écran, ce qui efface les zones et désactive le tactile alors que le buzzer reste armé ; un appui local affiche les zones plein écran sans en-tête (#16) et `answer_received` un bandeau de 1/4 de la hauteur avec des zones rétrécies, sans chrono (#17). Le CDC §8.2 et les maquettes `qcm-02` à `qcm-05` veulent un en-tête « Question N » avec chrono sous le bandeau du haut, un `timer_tick` à effet « inchangé » et des zones conservées. Le hub n'émet aucun de ces messages (aucune occurrence dans `src/` à `ed6bd31`) et `question_choices` ne porte pas `question_index` (US-011 CA-10). Le chrono « resynchronisé à chaque tick, décompte local à confirmer » (CDC §8.2) laissait le décompte à US-FW-016.

**Décision** (cadrage validé par Olivier, 2026-11-22, deux salves de questions) —
- **Chrono à chaque seconde** (CA-7, CA-8, CA-15) : « timer_tick ne sert qu'à resynchroniser les comptes à rebours, mais le but c'est qu'on affiche le compte à rebours à chaque seconde » (réponse libre d'Olivier). Le décompte local passe de US-FW-016 à US-FW-011 : départ à `time_limit` à la réception de `question_choices`, recalage à chaque `timer_tick`, arrêt à 0 s. Un tick ne remplace jamais l'écran.
- **`question_index` dans `question_choices`** (CA-3) : demandé au hub, plutôt que retenu par le firmware ; aucun code d'US-011 n'existe, le coût est une ligne de contrat. Champ absent : l'en-tête porte « Question » sans numéro.
- **Zones sous le bandeau et l'en-tête, 60 px chacun** (CA-4) : bande de 800 × 360 px, zones de 400 × 180 px (utiles 380 × 160 px), tactile aligné sur le même découpage. Corrige US-FW-008 CA-9 et le constat de US-FW-009.
- **Hors Inerte, écran inchangé** (CA-6) : `question_choices` n'est rendu que si le buzzer est Inerte (« zones affichées = zones actives »), cohérent avec #45.
- **Même mise en page après réponse** (CA-12) : l'appui local et `answer_received` gardent bandeau, en-tête, zones et chrono ; le texte de l'en-tête devient « Réponse enregistrée ». Cela **remplace** #16 (zones plein écran) et #17 (bandeau de 1/4, zones rétrécies) pour ces deux écrans ; `question_result` garde le bandeau de 1/4 jusqu'à US-FW-013.

**Choix de Claude, à relire en PR** —
- 15 CA : 6 tenus (CA-1, 2, 5, 9, 10, 13), 2 implémentés en partie (CA-11, CA-14), 7 non implémentés (CA-3, 4, 6, 7, 8, 12, 15) ; treize constats reportés. Statut de l'US : « Partielle ».
- Les CA tenus décrivent le réel (« A : x », « Trop tard » plein écran, zone choisie en blanc plein, statique) ; la pastille de lettre, la taille commune, les capsules, l'anneau et les teintes sont des constats reportés (décoration, comme à #53).
- Départ du chrono à `time_limit` à la réception, sans compenser la latence : le buzzer n'a pas d'horloge murale, `started_at` est inutilisable. Sans `time_limit` valide, pas de chrono tant qu'un tick ne l'amorce.
- À l'appui local l'en-tête garde « Question N » (maquette `qcm-03`) ; « Trop tard » reste un texte plein écran sans en-tête (maquette `qcm-05`).
- Les hauteurs (en-tête de 60 px, bandeau de réponse absent) et les chiffres de tenue des propositions viennent d'une sonde hors dépôt (`TextLayout`, largeurs réelles d'Oswald 30 px), pas du matériel.
- Alternatives écartées au cadrage : chrono dès le premier tick seulement, chrono reporté à US-FW-016 ; index retenu par le firmware, pas de numéro ; zones plein écran sans bandeau, constat reporté ; rendre `question_choices` hors Inerte, ou faire réarmer le MCQ (revoir #45) ; garder #16 et #17 avec chrono perdu, ou décrire le réel seulement ; en-tête de 72 px (maquette) ou non fixé.
- Aucun test, aucun code, aucun smoke test dans cette session.

**Conséquences** —
- Écarts de code : aucun en-tête, aucun chrono ni horloge (CA-3, CA-7, CA-15), zones plein écran (CA-4), `timer_tick` en plein écran (CA-8), rendu inconditionnel de `question_choices` (CA-6), mise en page après réponse inchangée (CA-12). Dettes : sept complétées (« Table de rendu §8.2 incomplète », « Buzzer armé sans zone tactile à l'écran », « Armement depuis un mode déjà armé », « Non-buzzeurs restés armés sur `buzz_locked` », « Titre de `question_open` remplacé par le premier `timer_tick` », « Bandeau : preuves manquantes », « Une taille par texte, pas par jeu de zones »), trois autres touchées (« Zone en évidence sans clignotement », « Rendu cible des maquettes partiellement reporté », messages jamais émis par le hub), quatre ajoutées (« Chrono : ni horloge ni rafraîchissement hors message », « Contrat hub : `question_index` absent de `question_choices` », « Écran de question : preuves manquantes et tests à réécrire », « Écran de question : habillage de la maquette reporté »).
- Les tests qui figent l'ancien rendu sont à réécrire avec les `feat`, pas avant : `TimerTickTest`, `DisplayStateTest.TimerTick*`, deux tests de `TappableZonesScreenTest` (zones touchées à y = 100), les modèles `ChosenZoneScreen` et `BannerScreen` de l'appui local et de `answer_received`.
- Hub : `question_index` à ajouter à `question_choices` (US-011 CA-10 et format du message), dans une session `docs(specs)` du hub ; CA-3 en dépend. Le hub reste à implémenter pour tous les messages de cette US.
- US-FW-016 perd le décompte local et ne garde que les clignotements ; US-FW-012 décide du chrono du SPEED et du `timer_tick` sur son écran ; US-FW-013 reprend le bandeau de 1/4 de `question_result` et ses zones de 380 × 130 px sous le bandeau.
- Un non-buzzeur d'un SPEED reste sur l'écran du titre pendant toute la question MCQ tant que le `fix(game)` de US-FW-002 CA-2 et de `buzz_locked` n'est pas livré.
- US-FW-008 CA-9, US-FW-009 (constat des zones), US-FW-016 (périmètre) et le `README` sont corrigés dans la même PR. Le CDC §3 (zones tactiles actives seulement si affichées), §7 (affichage progressif) et §8.2 (lignes `question_title`, `question_choices`, `timer_tick`, `timer_end`, `answer_received`, appui local MCQ ; en-tête et chrono MCQ) sont migrés ; restent à rédiger les US de rendu 012 à 016 avant la suppression du CDC.

## #55 — docs(specs): write US-FW-012, show a speed question on screen

**Contexte** — Le réel affiche une question de rapidité en texte plein écran : le titre seul à `question_open` (rien si le titre est vide, alors que le buzzer s'arme), « N s » à chaque `timer_tick` (qui remplace le titre), « Buzz envoyé », « À toi ! », « X a buzzé », « Raté » et « À nouveau possible de buzzer » chacun en plein écran, sans en-tête, chrono, bouton rouge dessiné, ni titre. Le CDC §8.2 et les maquettes `speed-01` à `speed-05` veulent un en-tête « Question N » avec chrono, un bouton rouge dessiné, la question qui reste affichée sous « À toi ! » et « X a buzzé », une réouverture identique à l'ouverture, et un état « ce buzzer a buzzé » (#26). Les messages `buzz_*` ne portent pas le titre ; `buzz_locked` ne porte que `buzzer_username`. Le hub n'émet aucun de ces messages (aucune occurrence dans `src/` à `ed6bd31`) mais `question_open` porte déjà `question_index` et `time_limit`. US-FW-011 laissait à cette US la règle du `timer_tick` sur un écran SPEED, et la dette « Armement depuis un mode déjà armé » le rendu de `question_open` et de `buzz_unlocked` hors Inerte.

**Décision** (cadrage validé par Olivier, 2026-11-22, une salve de deux questions) —
- **Pied de 80 px** (CA-5) : sous le bandeau du haut et l'en-tête de 60 px, la bande de 800 × 360 px se partage en une zone du titre de 800 × 280 px (utiles 780 × 260 px) et un pied de 80 px (bouton rouge dessiné et « Appuie sur le bouton rouge »). Mesuré hors dépôt (largeurs réelles d'Oswald 30, 38 et 48 px) : 40 caractères en 48 px sur 1 ligne, 121 caractères en 48 px sur 4 lignes, les 239 caractères de la maquette longue en 38 px sur 5 lignes (250 px sur 260). Écartés : 120 px (le titre de 239 caractères passe en 30 px) et 160 px (zone de 800 × 200 px, plus de place au bouton qu'au titre).
- **Hors Inerte, écran inchangé** (CA-13) : `question_open` et `buzz_unlocked` ne sont rendus que si le buzzer est Inerte, comme `question_choices` (#54) ; le message est ignoré en entier. Écartés : rendre toujours (écran SPEED sur un buzzer MCQ armé) ; reporter en dette.

**Choix de Claude, à relire en PR** —
- **Le rendu retient le titre, le numéro et « ce buzzer a buzzé »** (CA-7) : la maquette et le CDC gardent la question sous « À toi ! » et « X a buzzé » et `buzz_unlocked` redessine l'ouverture, or ces messages ne portent pas le titre. C'est un deuxième et un troisième état à côté du chrono (#54) et du nom du bandeau (#52), alors que #26 parlait d'« un seul état mémorisé » : état partagé par `LcdRenderer` et `ScreenLocalFeedback`, effacé à l'`auth_success` d'une connexion (limite de #26) et pas à celui de la réponse à `auth_refresh` (US-FW-007 CA-6). Écarté : écrans sans la question (bandeau seul), réouverture sans titre.
- **L'état « a buzzé » est posé ici, lu par US-FW-013** : le CDC le range dans la ligne de l'appui local ; la fiche de US-FW-013 le mettait dans son périmètre, elle est corrigée.
- **Un `timer_tick` ne remplace jamais l'écran** (CA-4), comme #54 ; le chrono continue après l'appui local (CDC, maquette `speed-02`) alors que le hub suspend le sien en `QUESTION_BUZZED` : seul `buzz_unlocked` recale. Il n'est pas affiché sur « À toi ! », « X a buzzé » ni l'élimination.
- **`question_open` sans titre s'affiche quand même** (CA-2) : le buzzer est armé, il doit voir le bouton ; cela contredit deux tests actuels.
- **`buzz_locked` affiche `buzzer_username` tel quel** (CA-9), repli de #52 : le joueur lit « quiz_buzzer_03 a buzzé » tant que le hub n'envoie pas le nom du participant ; sans troncature, contre la maquette.
- **`buzz_invalidated` : « Raté… » puis « Éliminé pour cette question »** sur deux lignes (CA-10), la réaction R9 restant à US-FW-014 ; **`buzz_unlocked` redonne l'écran d'ouverture** (CA-11), au lieu du libellé « À nouveau possible de buzzer », absent du CDC.
- Écrans monochromes comme #53 et #54 : disque blanc pour le bouton, bandeau d'état cadré, question non atténuée, bandeau « À toi ! » statique ; capsules, anneau, coche et couleurs sont des constats reportés. Les hauteurs et les chiffres de tenue viennent d'une sonde hors dépôt (`TextLayout`), pas du matériel ; les chiffres donnés à Olivier pendant le cadrage comptaient des octets (124 et 246) au lieu de caractères (121 et 239), sans changer les résultats.
- 16 CA : 2 tenus (CA-12, CA-14), 4 implémentés en partie (CA-1, CA-6, CA-8, CA-9), 10 non implémentés (CA-2, CA-3, CA-4, CA-5, CA-7, CA-10, CA-11, CA-13, CA-15, CA-16) ; douze constats reportés. Statut de l'US : « Partielle ».
- Aucun test, aucun code, aucun smoke test dans cette session.

**Conséquences** —
- Écarts de code : aucun en-tête, aucun chrono, aucun bouton dessiné, aucun état retenu (CA-1 à CA-3, CA-5 à CA-9, CA-11, CA-15, CA-16) ; `timer_tick` en plein écran (CA-4) ; titre vide qui laisse l'écran inchangé (CA-2) ; rendu inconditionnel de `question_open` et `buzz_unlocked` (CA-13). Dettes : deux ajoutées (« Écran de rapidité : preuves manquantes et tests à réécrire », « Écran de rapidité : habillage de la maquette reporté »), neuf touchées (« Table de rendu §8.2 incomplète », « `buzz_locked`, `question_open`, `question_title` et `question_result` jamais émis par le hub », « Titre de `question_open` remplacé par le premier `timer_tick` », « Rendu local SPEED sans effet d'écran », « Armement depuis un mode déjà armé », « Non-buzzeurs restés armés sur `buzz_locked` », « Deux instances de `DisplayState` », « Nom du participant absent du protocole », « Chrono : ni horloge ni rafraîchissement hors message »).
- Les tests qui figent l'ancien rendu sont à réécrire avec les `feat`, pas avant : `TimerTickTest`, `DisplayStateTest.TimerTick*`, les cas `buzz_accepted`, `buzz_invalidated` et `buzz_unlocked` de `MessageWithFixedTextTest`, `QuestionOpenWithoutTitle…` et `LeavesTheScreenUnchangedWithoutTitle`, « Buzz envoyé » plein écran (`ScreenLocalFeedbackTest`, `ScreenPresenterLocalFeedbackTest`, `DisplayStateTest.BuzzSent…`).
- Tant que le `fix(game)` de US-FW-003 CA-8 (désarmer sur `buzz_locked`) n'est pas livré, un non-buzzer reste armé, garde « X a buzzé » et ignore `buzz_unlocked` ; de même, un `question_open` reçu pendant un MCQ armé laisse l'écran MCQ alors que le code passe en SPEED, tant que US-FW-002 CA-2 n'est pas livré.
- `LcdRenderer` lira l'état armé pour quatre messages (`timer_end`, `question_choices`, `question_open`, `buzz_unlocked`) ; l'état partagé (chrono, nom du bandeau, titre, numéro, « a buzzé ») se fait en un seul `feat`, avec le solde de « Deux instances de `DisplayState` ».
- Hub : aucun champ à ajouter pour cette US (`question_open` porte `question_index` et `time_limit`, `buzz_unlocked` porte `remaining_seconds`) ; le nom du participant dans `buzz_locked` s'ajoute à la décision attendue pour le bandeau du haut. `scripts/fake-hub.ps1` est à étendre (`question_index`, `time_limit`, `remaining_seconds`).
- US-FW-013 (périmètre : l'état « a buzzé » est lu, pas posé), US-FW-014 (R9 précède l'écran de CA-10) et le `README` sont corrigés ou renvoyés dans la même PR. Le CDC §8.2 (lignes `question_open`, `buzz_accepted`, `buzz_locked`, `buzz_invalidated`, `buzz_unlocked`, appui local SPEED ; en-tête et chrono SPEED ; « zones masquées » SPEED) est migré ; restent à rédiger US-FW-013 à US-FW-016 avant la suppression du CDC.

## #56 — docs(specs): write US-FW-013, show the result of a question

**Contexte** — Le réel affiche `question_result` en texte : un MCQ met en évidence la zone de la bonne réponse sous un bandeau de 1/4 de la hauteur, avec des zones rétrécies, sur deux lignes « Bonne réponse : B » et « Votre réponse : C » (décision #15) ; un SPEED n'affiche que « Validé » ou « Raté » en plein écran, sans réponse attendue ni gain, pour tout `correct: false`, que ce buzzer ait buzzé ou non. Le CDC §8.2 et les maquettes `qcm-06` à `qcm-08` et `speed-06` à `speed-08` veulent un en-tête sous le bandeau du haut, la réponse fausse du joueur cerclée, la réponse attendue, le gain et trois verdicts (#24, #26). Le hub n'émet pas ce message (aucune occurrence dans `src/` à `ed6bd31`) ; `player_answer` d'un joueur sans réponse n'est précisé ni en `null` ni en absent. La décision #54 laissait à cette US le bandeau de 1/4 de `question_result`.

**Décision** (cadrage validé par Olivier, 2026-11-22, une salve de quatre questions) —
- **MCQ : en-tête de 60 px sur une ligne** (CA-2, CA-5) : « Bonne réponse : B · Votre réponse : C » en 30 px (404 px mesurés), zones inchangées à 380 × 160 px, au même endroit que la question, sans saut. Cela **remplace** le bandeau de 1/4 et les zones rétrécies (#15, #17, clause de #54) pour `question_result`. Mesuré hors dépôt (`TextLayout`, largeurs réelles d'Oswald 30, 38 et 48 px) : 40 `W` préfixés « A : » tiennent pile (160 px). Écartés : un en-tête de 100 px sur deux lignes (zones de 380 × 140 px, 40 `W` débordent de 20 px), un bandeau de 120 px (zones de 380 × 130 px, débordement de 30 px).
- **Réponse fausse du joueur : double cadre blanc** (CA-4), à la place du cerclage rouge de la maquette (écran monochrome). Écartée : le texte seul, cerclage reporté avec les couleurs.
- **SPEED : le gain dans le bandeau d'état** (CA-7) : « Validé · +N pts » sur une ligne de 30 px (168 px) ; sans `points_earned` entier d'au moins 1 : « Validé ». Écartée : une ligne sous la réponse attendue.
- **Lettre de A à D : constat reporté** (CA-1) : la règle du CDC (une lettre = MCQ, décision #13) reste, la limite d'une réponse SPEED d'un seul caractère de A à D est consignée en dette. Écartés : un champ `question_type` dans `question_result` (une ligne de contrat côté hub), un type retenu par le firmware.

**Choix de Claude, à relire en PR** —
- **Le rendu retient les propositions** (CA-6) : celles de la dernière `question_choices` rendue, effacées à l'`auth_success` (pas à celui de `auth_refresh`, comme #55) et à chaque `question_open` rendu. Sans propositions, le résultat MCQ s'affiche **sans zones**, en deux lignes plein écran : l'adaptateur redessine aujourd'hui les propositions périmées, ce qui montrerait une bonne réponse fausse. C'est un état de plus à partager entre `LcdRenderer` et `ScreenLocalFeedback`, avec celui de #54 et #55.
- **Texte du résultat MCQ en une ligne avec « · »** (U+00B7, dans les 0x20 à 0xFF de la police) ; `player_answer` repris tel quel même hors A–D ; `correct` non lu en MCQ (US-FW-014 le lira pour la réaction).
- **SPEED sans `player_answer`** : le hub n'en envoie pas ; la mise en page est un bandeau d'état de 60 px, le libellé « Bonne réponse » (800 × 60 px) et la réponse dans le reste (800 × 300 px), mesurée à 220 et 240 px, donc a fortiori à 280 px utiles.
- **`correct: true` fait foi** : l'état « ce buzzer a buzzé » n'est lu que pour `correct: false` (« Raté » ou « Question terminée », limites de #26 reprises).
- **Pas de chrono sur l'écran de résultat** : le décompte de #54 s'arrête.
- 14 CA : 3 tenus (CA-1, CA-3, CA-10), 2 implémentés en partie (CA-2, CA-12), 9 non implémentés (CA-4 à CA-9, CA-11, CA-13, CA-14) ; douze constats reportés. Statut de l'US : « Partielle ». Aucun test, aucun code, aucun smoke test dans cette session.

**Conséquences** —
- Écarts de code : bandeau de 120 px et zones rétrécies, sans bandeau du haut ni en-tête (CA-5, CA-13) ; deux lignes au lieu d'une (CA-2) ; aucun double cadre (CA-4), aucune proposition retenue (CA-6), aucun gain ni réponse attendue (CA-7, CA-11, CA-14), aucun état « a buzzé » donc « Raté » pour tous (CA-8, CA-9).
- Dettes : trois ajoutées (« Écran de résultat : preuves manquantes et tests à réécrire », « Écran de résultat : habillage de la maquette reporté », « Réponse SPEED d'un seul caractère de A à D prise pour un MCQ »), huit complétées ou touchées (« Table de rendu §8.2 incomplète », « jamais émis par le hub », « Dialogue avec le hub : preuves manquantes », « Deux instances de `DisplayState` », « Bandeau : points, rang et batterie sans source », « Rendu cible des maquettes partiellement reporté », « Zone en évidence sans clignotement », « Textes des zones retenus par l'adaptateur »).
- Les tests qui figent l'ancien rendu sont à réécrire avec les `feat`, pas avant (liste dans la dette). CA-8 et CA-9 attendent l'état « a buzzé » de US-FW-012 CA-7.
- Hub : aucun champ à ajouter ; `question_result` à implémenter (US-011 CA-25, US-012 CA-31) ; le format du message d'un joueur sans réponse est à préciser. `scripts/fake-hub.ps1` est à étendre (`points_earned`, SPEED après un appui, réponse de 40 caractères).
- US-FW-011 (CA-12, périmètre) et US-FW-008 (CA-9, géométrie) sont corrigées dans la même PR ; le `README` migre les lignes `question_result` du CDC §8.2. Restent à rédiger US-FW-014 à US-FW-016 avant la suppression du CDC.

## #57 — docs(specs): write US-FW-014, show the reactions of the character

**Contexte** — US-FW-014 était « À cadrer » : les maquettes `reaction-*` (décisions #20, #21, #27) et le CDC §8.2 veulent une réaction plein écran de 2000 ms avant le résultat, avec « le personnage du joueur », mais rien ne dit d'où il vient, ni ce qui arrive à un message reçu pendant la réaction. Le réel n'a ni horloge, ni source aléatoire, ni image, ni état « a buzzé » ; `buzz_invalidated` affiche « Raté » d'emblée. Le hub (`ed6bd31`) n'a aucun avatar : un participant n'est qu'un nom (US-010 CA-9) et les buzzers lui sont assignés par ordre de connexion (CA-65) ; il n'émet ni `question_result` ni `buzz_invalidated`. Le dernier handoff proposait US-FW-015 : la session a donc été un cadrage, limité à ce qui se décide.

**Décision** (cadrage validé par Olivier, 2026-11-22, une salve de deux questions) —
- **Personnage fixé par buzzer, en firmware** (CA-9) : les deux modèles (vingt images) sont embarqués, une constante de configuration choisit masculin ou féminin ; aucun changement du hub, du protocole ni d'Angular. Le joueur voit le personnage du **buzzer** qu'il tient, pas « le sien » : cela **corrige** le CDC §8.2 (« celui du joueur »). Écartés : un seul personnage pour tous (modèle féminin inutilisé), un modèle fourni par le hub par participant (trois dépôts), un personnage généré par le hub et téléchargé (environ 1,8 Mo en RGB565 par joueur à 300 px).
- **Un message qui produit un écran interrompt la réaction** (CA-6), sans file d'attente : sa minuterie est annulée, l'écran en attente n'est pas posé ; un message sans écran ne l'interrompt pas. Cela **précise** le CDC §8.2 (« cet écran est posé à l'issue de la durée », vrai seulement si rien n'interrompt). Écartés : aller au terme et rendre tout dans l'ordre (file d'attente, l'état armé à lire à la réception, résultat après 4 s dans le cas réel), aller au terme en ne gardant que le dernier message (messages intermédiaires perdus). Cas réel : le dernier joueur invalidé reçoit `question_result` quelques ms après `buzz_invalidated` (US-012 CA-25) ; R9 est remplacée sans être vue (constat reporté).

**Choix de Claude, à relire en PR** —
- **Fond du médaillon blanc uni** (CA-10) : les fonds or, rose et lavande sont reportés avec le rendu cible, comme les couleurs de US-FW-011 à US-FW-013.
- **Pas de réaction sans situation exploitable** (CA-7) : `correct` absent ou non booléen en MCQ, ou résultat inexploitable (US-FW-013 CA-10) : l'écran de résultat s'affiche d'emblée ; on n'invente pas une situation.
- **Situation lue à la réception, écran de résultat calculé à la réception** (CA-2, CA-5) : l'état « a buzzé » et les propositions retenues sont ceux du message, pas ceux de l'échéance. La durée est mesurée depuis la réception du message.
- **Tirage sans mémoire** (décision #20) derrière un **port de source aléatoire** de `lib/`, et **port d'horloge partagé** avec le chrono (US-FW-011 CA-7, CA-15) ; ils naissent sous le premier rouge du `feat` (décision #27).
- **Géométrie** (mesurée hors dépôt, pas sur le matériel) : zone de réaction de 800 × 420 px sous le bandeau du haut, médaillon de 300 px à x 48, libellé en 48 px dans une colonne de 364 px (le plus large, « Pas de réponse », mesure 259 px).
- **Stockage des images en constat reporté** : PNG 24 bits à 300 px, 363 470 o les vingt, contre 3,6 Mo en RGB565 brut ; taille du firmware, partitions et PSRAM non lues.
- 11 CA : 0 tenu, 2 implémentés en partie (CA-1, CA-8), 9 non implémentés ; dix constats reportés. Statut de l'US : « Partielle » (les champs `correct` et `player_answer` sont lus et testés). Aucun test, aucun code, aucun smoke test dans cette session.

**Conséquences** —
- Écarts de code : aucune horloge, aucune source aléatoire, aucune image, aucun état « a buzzé » (US-FW-012 CA-7) ; `buzz_invalidated` affiche « Raté » d'emblée ; `ScreenModel` n'a aucune variante pour une réaction.
- Dettes : deux ajoutées (« Réactions du personnage : preuves manquantes et tests à réécrire », « Réactions du personnage : stockage et décodage des images »), huit complétées ou touchées (« Avatar par joueur sans source ni protocole », « Réaction interrompue ou attendue », « Table de rendu §8.2 incomplète », « Rendu cible des maquettes partiellement reporté », « Écran de résultat : habillage de la maquette reporté », « Écran de rapidité : habillage de la maquette reporté », « `buzz_locked`, `question_open`, `question_title` et `question_result` jamais émis par le hub », « Deux instances de `DisplayState` »).
- Les tests `MessageWithFixedTextTest` de `buzz_invalidated` figent l'ancien rendu : à réécrire avec les `feat`, pas avant.
- Hub : aucun champ à ajouter pour le personnage ; `question_result` et `buzz_invalidated` restent à émettre (US-011 CA-25, US-012 CA-23 et CA-31). `scripts/fake-hub.ps1` est à étendre (MCQ `correct: true`, SPEED après un appui).
- US-FW-012 (constat R9) et US-FW-013 (constat des réactions) sont corrigées dans la même PR ; le `README` migre les lignes `question_result`, `buzz_invalidated` et « Réactions du personnage » du CDC §8.2. Il reste US-FW-015 et US-FW-016 à rédiger avant la suppression du CDC.


## #58 — docs(specs): write US-FW-015, show the connection loss, elimination and refusal screens

**Contexte** — US-FW-015 était « Partielle » sans CA : le seul écran de coupure du code est le refus `401` en texte plein écran (décision #42). Les maquettes `fail-01` à `fail-03` (décision #29) et le CDC §8.2 (décision #30) décrivent trois écrans réseau. Le réel : aucun écran de coupure ni de « Hors jeu » (`ConnectionEventHandler::OnConnectionLost` ne fait qu'appeler le compteur, rien ne lit `IsEliminated()`), aucun port d'état de liaison, un refus affiché avec `TopBar::Hidden()`. Bibliothèque `WebSockets` lue à 2.7.2 (dernier tag ; `^2.7.3` épinglé, la version résolue peut différer) : `connectFailedCb` est vide et `connect()` bloque jusqu'à 5000 ms. Hub à `ed6bd31` : aucun message vers le buzzer à la perte ; le `401` est US-003 CA-8. Le dernier handoff annonçait US-FW-015 : confirmé par le `README` et `git log`.

**Décision** (cadrage validé par Olivier, 2026-11-22, une salve de trois questions) —
- **Appuis ignorés pendant la coupure, mode conservé** (CA-4, CA-7) : ni `answer`, ni `buzz`, ni verrou, ni feedback local ; un buzzer armé le reste, les appuis reprennent à l'`auth_success`. Tranche le constat de la décision #49. Écartés : désarmer à la perte (un joueur coupé 1 s perd la question en cours, `game_resumed` ne réarme rien), reporter (le feedback local écraserait le bandeau et le message serait perdu).
- **Question atténuée en gris sous le bandeau rouge** (CA-1, CA-2, CA-11), comme la décision #29 : un décorateur de `Screen` retient le dernier écran affiché. Écartés : un bandeau superposé sans atténuation (la question reste en blanc vif alors qu'elle est inactive), un écran plein écran sans question (écarte la décision #29).
- **Anneau et icônes dessinés** (CA-12, CA-13) : anneau de 128 px, symbole marche/arrêt, cadenas. Écartés : du texte seul à tailles distinctes (la perte définitive et le refus se ressemblent), du texte seul à une seule taille (état actuel).

**Choix de Claude, à relire en PR** —
- **Écran retenu à la couture `Screen`** (CA-2), partagé par `LcdRenderer` et `ScreenLocalFeedback`, plutôt que dans `GfxScreen` (état non testable) ou dans `DisplayState` (deux instances, dette).
- **Coupure pendant une réaction** (CA-5) : elle remplace la réaction, et l'écran atténué est celui qui l'attendait (résultat ou élimination), pour ne griser aucune image. Confirme la règle d'interruption de la décision #57.
- **Les appuis reprennent à l'`auth_success`**, pas à l'ouverture de la connexion (CA-4, CA-7) ; **avant le premier `auth_success`, aucun écran de coupure** : l'init reste (CA-6, US-FW-010 CA-2).
- **Bandeau du refus identique au démarrage et en reconnexion** (CA-9, CA-10 : nom vide, liaison coupée, comme l'init) ; **« Hors jeu » garde le dernier nom**, liaison coupée (CA-8). Remplace le « sans bandeau » de la décision #42.
- **Géométrie** (mesurée hors dépôt en caractères, pas sur le matériel) : bandeau rouge de 60 px à y 60 (la place de l'en-tête de US-FW-011), texte en 38 px car la boîte de 48 px mesure 62 px ; pile des écrans à anneau de 362 px sur 420 ; gris `#666666` (40 % de blanc, l'opacité de la maquette).
- **Rouge et gris dérogent au monochrome** de US-FW-011 à US-FW-014, pour ces seuls écrans : le rouge dit « coupure ».
- 13 CA : 0 tenu, 5 implémentés en partie (CA-3, CA-6, CA-7, CA-9, CA-13), 8 non implémentés ; sept constats reportés. Aucun test, aucun code, aucun smoke test dans cette session.

**Conséquences** —
- Écarts de code : aucun port d'état de liaison (le même que US-FW-009 CA-4), aucun écran retenu, aucun écran de coupure ni de « Hors jeu », appuis non ignorés pendant la coupure, `401` de reconnexion ni affiché ni arrêté, refus sans bandeau ni icône.
- N de « Tentative N/3 » et le passage en « Hors jeu » dépendent des `fix(reconnection)` de US-FW-006 (CA-6, CA-7) : un hub arrêté laisserait « Tentative 1/3 » à vie.
- `LoginOutcomeDisplayTest.ShowsTheRefusalScreenWithoutTopBarWhenTheCredentialsAreRefused` et `ScreenPresenterStartupTest.ARefusedLoginPresentsTheReasonAndWhoToWarn` figent l'ancien rendu : à réécrire avec les `feat`, pas avant.
- Dettes : deux ajoutées (« Écrans de coupure : preuves manquantes et tests à réécrire », « Écrans de coupure : habillage de la maquette reporté »), douze complétées ou touchées.
- Hub : rien à ajouter. `scripts/fake-hub.ps1` est à étendre (arrêt du hub en cours de question, réponse `401`).
- US-FW-005, 006, 009, 010 et 014 sont corrigées d'une phrase dans la même PR ; le `README` migre les lignes « Lignes réseau », « Zones masquées » et §4.4. Il reste US-FW-016 à rédiger avant la suppression du CDC.

## #59 — docs(specs): write US-FW-016, animate the screen

**Contexte** — US-FW-016 était « À cadrer » : le CDC §8.2 demande une zone qui clignote et un « bandeau clignotant » (« alterne entre deux teintes »), sans cadence, sans état éteint ni durée ; la maquette ne montre que l'état allumé (« 1 Hz proposé »). Le réel n'a ni horloge (aucune occurrence de `Clock` ni de `millis` dans `lib/` et `src/`), ni rafraîchissement hors message (`loop()` lit le WebSocket et deux appuis), ni écran retenu, et chaque dessin de `GfxScreen` commence par `fillScreen(TFT_BLACK)`. LovyanGFX 1.3.0 (lue hors dépôt, `Panel_RGB` et `Bus_RGB` seulement) donne à la dalle un tampon d'image unique en PSRAM. La décision #54 a retiré le décompte du chrono de cette US, mais les décisions #54, #55 et #57 attendent toutes la même horloge, « à définir une seule fois ». Le dernier handoff proposait cette session comme un cadrage seul.

**Décision** (cadrage validé par Olivier, 2026-11-22, une salve de quatre questions) —
- **Port d'horloge et animateur à la couture `Screen`** (CA-1 à CA-3, CA-7) : un port `Clock` de `lib/`, lu une fois par tour de `loop()` ; le décorateur de `Screen` de la décision #58 (dernier écran retenu) porte aussi la phase et sert d'animateur. Écartés : une horloge lue par chaque consommateur (la phase vivrait dans `GfxScreen`, non testable en natif), `millis()` appelé directement dans `GfxScreen` (la fin de réaction et le chrono n'auraient plus de test à horloge factice).
- **Clignotement à 1 Hz, 500 ms allumé et 500 ms éteint** (CA-4, CA-9), valeur du CDC et de la maquette. Écartés : 2 Hz (quatre redessins par seconde), 700 ms allumé et 300 ms éteint (écart au CDC).
- **Sans fin, jusqu'au message suivant** (CA-4), fidèle au CDC §8.2. Écartés : 5 s puis fixe, ou sans fin pour les bandeaux et 5 s pour les zones.
- **Init statique** (constat reporté) : le bandeau « Connexion… » clignotant et le démarrage non bloquant sortent de cette US (US-FW-010 CA-2, décision #53 inchangés). Écartés : un `Sleeper` qui rafraîchit l'écran pendant les pauses (l'écran se fige pendant l'appel HTTP), un `setup()` en machine d'états dans cette US.

**Choix de Claude, à relire en PR** —
- **Quatre écrans clignotent** (CA-5) : la zone de la bonne réponse d'un résultat MCQ, la zone choisie (appui local et `answer_received`), le bandeau « À toi ! », le bandeau « Validé ». « X a buzzé », « Raté », « Trop tard », le double cadre de la réponse fausse, les réactions et les écrans réseau restent fixes.
- **Monochrome** (CA-9) : allumé, blanc plein et texte noir (la mise en évidence actuelle) ; éteint, cadre blanc et texte blanc, l'aspect d'une zone ordinaire. Le cadre statique du bandeau d'état (US-FW-012 CA-15, US-FW-013 CA-7) devient l'état éteint : le premier affichage d'un bandeau passera du cadre au plein blanc avec le `feat`, les CA statiques restant vrais jusque-là.
- **Phase repartie de zéro à chaque écran affiché, départ allumé** (CA-3, CA-4), calculée sur l'heure et non comptée par tour ; un `timer_tick` ou un redessin du chrono ne la remet pas à zéro (CA-7).
- **Pas de clignotement pendant la coupure, « Hors jeu » ou le refus** (CA-6) : l'écran atténué de US-FW-015 CA-11 est figé à l'état allumé.
- **Redessin d'une région, sans `fillScreen`** (CA-8), à cause du tampon unique ; **aucune attente dans un tour de `loop()`** (CA-10). Durées par différence non signée (CA-1) : le cœur Arduino n'est pas lu.
- **Points de tentative animés et logo qui vibre : constats reportés**, faute de définition (maquettes statiques).
- 10 CA : 0 tenu, 2 implémentés en partie (CA-5, CA-9), 8 non implémentés ; sept constats reportés. Statut de l'US : « Partielle ». Aucun test, aucun code, aucun smoke test dans cette session.

**Conséquences** —
- Écarts de code : aucun `Clock`, aucun animateur ni écran retenu, aucun redessin de région, aucun bandeau d'état ni état éteint ; « À toi ! » et « Validé » sont du texte plein écran.
- Le temps de dessin d'une région et le noir visible sur un tampon unique ne sont pas mesurés ; LovyanGFX n'est pas épinglée (1.3.0 lue, version résolue possiblement différente).
- Dettes : deux ajoutées (« Animation : preuves manquantes et tests à écrire », « Animation : coût d'un redessin et tampon unique non mesurés »), onze complétées.
- Hub : rien à ajouter. `scripts/fake-hub.ps1` n'a rien à changer pour cette US.
- US-FW-009, 010, 011, 012, 013 et 015 sont corrigées d'une phrase dans la même PR ; le `README` migre les lignes « Vocabulaire des effets d'écran » et « Habillage commun » (chrono) du CDC §8.2. US-FW-016 est la dernière US de rendu à cadrer ; la suppression du CDC attend les lignes `play_*` (US-FW-021) et le paramétrage (US-FW-022), dans des US encore « Vides ».

## #60 — docs(specs): write US-FW-017, show the interim ranking

**Contexte** — US-FW-017 était « Vide » (décisions bloquantes) et le dernier handoff proposait une session de code, non cette US : session menée en **cadrage seul**. Le hub diffuse `intermediate_ranking` à tous les buzzers, identique pour tous (US-014 CA-10, CA-12), même pendant une question (CA-2, CA-13) ; le firmware n'en lit aucun champ (aucune occurrence de `ranking` dans `lib/`, `src/`, `test/`, `scripts/`) et le hub ne l'émet pas encore (aucune occurrence dans `src/` à `ed6bd31`). La décision #31 fondait la reconnaissance de la ligne du joueur sur `participant_name` = `username` d'`auth_success` ; le hub la contredit : noms de participants libres et uniques (US-010 CA-9, CA-9a), assignés par ordre de connexion (CA-65), `username` du compte (`quiz_buzzer_01`, US-009 CA-6), aucun message qui dise au buzzer quel participant il est (même blocage que #52). Mesures (hors dépôt, Oswald 30 px) : 7 lignes de 44 px tiennent dans les 340 px utiles de la bande de 800 × 360 px, 10 en demandent 476 ; colonne du nom de 470 px, 21 `W` ou 36 `e`.

**Décision** (cadrage validé par Olivier, 2026-11-22, une salve de trois questions, les trois recommandations retenues) —
- **Reconnaissance de la ligne reportée** (constat 1) : le classement s'affiche sans ligne en évidence, sans « Tu es en tête ! », sans variante « plus de joueurs que de lignes », sans points ni rang au bandeau, jusqu'à un message du hub qui donne au buzzer son participant (il servirait aussi US-FW-009 CA-2 et US-FW-018). Remplace #31 pour la cible ; #31 et #32 ne sont pas réécrites. Écartés : garder l'égalité du nom et de l'`username` (aucune ligne reconnue, sans que rien le dise), un champ propre à chaque buzzer (contredit US-014 CA-12).
- **Sept premières lignes seulement** (CA-6, constat 2) : sans pagination ni « … ». Écartée : deux pages alternées toutes les 5 s (le classement bouge, une horloge est requise) ; écarté : aucune règle au-delà de 7.
- **Un buzzer armé ignore le classement en entier** (CA-4, constat 3) : « zones affichées = zones actives » reste vrai, comme `question_choices` reçu armé (US-FW-011 CA-6, US-FW-012 CA-13). Écartés : l'afficher en restant armé (MCQ : zones perdues, appuis ignorés jusqu'à `timer_end` ; SPEED : « Buzz envoyé » le remplace), l'afficher en désarmant (contredit le hub et US-FW-002).

**Choix de Claude, à relire en PR** —
- Message **inexploitable** (`ranking` absent, non tableau ou vide ; une ligne qui n'est pas un objet ou dont un champ n'a pas le bon type) : **écran inchangé en entier**, plutôt qu'un classement amputé de lignes (CA-1).
- `participant_order` n'est ni lu ni affiché ; ordre et rangs sont ceux du hub, jamais recalculés (CA-1).
- Temps arrondi au dixième le plus proche (demi vers le haut), virgule décimale, « 83,2 s » ; score « N pts » sans singulier ; classement sans question jouée affiché tel quel, « 0 pts » et « 0,0 s » (CA-3).
- En-tête de 60 px « Classement provisoire », bandeau du haut avec le nom retenu et sans points ni rang (CA-2) ; le classement **remplace** l'écran, **reste** jusqu'au prochain écran produit, sans minuterie (décision #31), **ne clignote pas** (US-FW-016 CA-5) (CA-2, CA-5).
- Un classement pendant une réaction de 2000 ms **la remplace** : l'écran de résultat en attente n'est pas posé (US-FW-014 CA-6) ; `timer_tick` sur un classement : inchangé (US-FW-011 CA-8, US-FW-012 CA-4) (CA-5).
- Dessin monochrome : 7 lignes de 44 px à 5 px d'écart (338 px), tout en 30 px, **temps à 30 px et non 24 px** (aucune police 24 px) ; couleurs reportées (CA-7, constat 4).
- 7 CA : 0 tenu, 1 en partie (CA-4, tenu par défaut, non testé), 6 non implémentés ; sept constats reportés. Statut de l'US : « Partielle ». Aucun test, aucun code, aucun smoke test dans cette session.

**Conséquences** —
- Écarts de code : aucun champ `ranking` lu, aucun modèle d'écran de liste, aucun dessin ; `LcdRenderer` lira l'état armé pour un cinquième message.
- Le hub n'émet ni `intermediate_ranking` ni `trigger_intermediate_ranking` ; `scripts/fake-hub.ps1` n'a aucune touche de classement : à étendre avec le `feat`.
- Hub : un message qui donne au buzzer son `participant_order` ou son nom de participant (une décision côté hub, à rouvrir avec US-FW-009 CA-2).
- Dettes : une ajoutée (« Classement intermédiaire : preuves manquantes et tests à écrire »), six complétées (« Classements maquettés sans report ni message pour le définitif », « Nom du participant absent du protocole », « Bandeau : points, rang et batterie sans source », « Buzzer armé sans zone tactile à l'écran », « `buzz_locked`, `question_open`, `question_title` et `question_result` jamais émis par le hub », « Table de rendu §8.2 incomplète »).
- US-FW-009 (constat « Points et classement ») et US-FW-012 (CA-13 : cinq messages) sont corrigées d'une phrase dans la même PR ; le `README` passe US-FW-017 en « Rendu », « Partielle ». Les US-FW-018 à 022 restent « Vides ».

## #61 — docs(specs): write US-FW-018, show the final ranking and the end of the game

**Contexte** — US-FW-018 était « Vide » (décisions bloquantes) et le dernier handoff (`2026-11-22-1823`) proposait une session de code, non cette US : session menée en **cadrage seul**. Le hub n'a aucun message de classement final vers les buzzers : `intermediate_ranking` est refusé en `COMPLETED` (US-014 CA-4), `GET /api/v1/games/:id/results` est une route REST de l'administrateur (US-013 CA-1, CA-11, CA-12), et `play_system_sound` `GAME_END` (US-018 CA-8) ne porte ni classement ni effet d'écran. La dette disait « aucun message à la fin d'une partie » : ce son existe dans le contrat du hub, sans écran. Le hub accepte 1 à 10 participants (US-010 CA-8). La place du joueur dépend de l'identité du participant, que le hub ne donne pas (décision #60, constat 1). Mesures (hors dépôt, Oswald 30 et 48 px) : colonne du podium de 250 px, 230 px utiles, nom à 30 px 10 `W`, 17 `e` ou 32 `i` ; première colonne de 244 px sur 256 (pas de nom sur deux lignes) ; « Merci d’avoir joué ! » 327 px sur 344 à 48 px.

**Décision** (cadrage validé par Olivier, 2026-11-22, une salve de quatre questions, les quatre recommandations retenues) —
- **`final_ranking` : nouveau message** au corps de `intermediate_ranking`, diffusé à tous les buzzers ; le firmware réutilise la lecture de US-FW-017 CA-1 et n'affiche que les trois premières lignes (CA-1 à CA-4). Écartés : `intermediate_ranking` avec un champ `final` (rouvre US-014 CA-4, un message sans le champ afficherait « Classement provisoire »), rien côté firmware (US reste « Vide »).
- **Fin de partie : second message du hub**, nom `game_end` **proposé**, sans champ, sur décision du maître du jeu (CA-8) : le passage du podium à « Merci d’avoir joué ! » reste sa décision (#37), le firmware ne minute rien. Écartés : `play_system_sound` `GAME_END` comme déclencheur (arrive à la transition `COMPLETED`, couple l'écran à un son, adapte `MessageWithoutDisplayTest.LeavesTheScreenUnchanged`), une minuterie de 10 s au firmware (contredit #37).
- **Moins de trois joueurs : autant de marches que de lignes** (CA-4) : une ligne, la première seule au centre ; deux lignes, colonne de droite vide. Écartés : une liste à la place du podium (second modèle d'écran), le cas hors périmètre.
- **Buzzer armé : affiché et désarme** (CA-6), contrairement au classement provisoire (US-FW-017 CA-4, ignoré si armé) : la partie est finie. `LcdRenderer` ne lit pas l'état armé pour ces deux types, le dispatcher les aiguille par leur type seul. Écartés : ignoré si armé (un joueur resté armé ne voit jamais le podium), affiché en restant armé (zones perdues ou « Buzz envoyé » qui remplace le podium).

**Choix de Claude, à relire en PR** —
- **Place du joueur reportée** (décision #60 reprise, constat 1) : pas de « Tu es Ne sur N », pas de marche cernée, pas de pastille, pas de points ni de rang au bandeau ; la bande de 100 px du bas (y 380 à 480) est **réservée** et **laissée vide**, pour que la ligne s'y glisse sans refaire le podium. Aucune ligne « N joueurs » à la place.
- **Trois premières lignes lues dans l'ordre reçu** : première au centre, deuxième à gauche, troisième à droite ; le **rang affiché sur la marche est celui du hub**, jamais recalculé ; le temps est lu et **non affiché** (CA-1, CA-3).
- **Message inexploitable** : même règle que US-FW-017 CA-1 (écran inchangé en entier) ; il **désarme quand même**, car le dispatcher ne reçoit que le type (constat 5, comme `question_choices` aux propositions inexploitables).
- **Géométrie de la maquette reprise** (8 px l'unité) : marches de 128, 88 et 60 px, couronne de 32 × 28 px, nom et score à 30 px (aucune police entre 30 et 48 px ; la maquette met le nom à 35 px), nom tronqué par « … » à 230 px, **sans pastille** ; monochrome : marches blanches, rang en noir.
- **En-tête « Classement final »** de 60 px, comme US-FW-017 CA-2 ; le podium **ne clignote pas** (US-FW-016 CA-5) et reste jusqu'au prochain écran, sans minuterie.
- **Fin de partie** : personnage du buzzer en **R1** (« ravi »), sans tirage, une seule réaction quel que soit le rang (#37), géométrie de US-FW-014 CA-11 ; affichée même sans podium préalable. L'image est celle déjà embarquée (US-FW-014).
- **Aucun écran de transition** entre le podium et la fin, aucune suite après la fin (constat 4).
- 10 CA : 0 tenu, 1 en partie (CA-5), 9 non implémentés (2 adaptateur) ; sept constats reportés. Statut de l'US : « Partielle ». Aucun test, aucun code, aucun smoke test dans cette session.

**Conséquences** —
- Écarts de code : aucun champ `ranking` lu pour ce type, aucun modèle de podium ni de fin, aucun dessin, aucun aiguillage de fin dans `HubMessageDispatcher` ; `BuzzerBehavior` gagnera une transition vers Inerte à la fin.
- Hub : une session `docs(specs)` ajoute `final_ranking` et `game_end` (corps, deux déclencheurs Angular, état de la partie à l'émission) ; le firmware ne se smoke-teste qu'avec `scripts/fake-hub.ps1`, à étendre avec le `feat`.
- Hub, déjà noté : un message qui donne au buzzer son participant (US-FW-009 CA-2, US-FW-017, et la place du joueur ici).
- Dettes : une ajoutée (« Classement final et fin de partie : preuves manquantes et tests à écrire »), huit complétées (« Classements maquettés sans report ni message pour le définitif », « Nom du participant absent du protocole », « Bandeau : points, rang et batterie sans source », « Table de rendu §8.2 incomplète », « `buzz_locked`, `question_open`, `question_title` et `question_result` jamais émis par le hub », « Non-buzzeurs restés armés sur `buzz_locked` », « Avatar par joueur sans source ni protocole », « Rendu cible des maquettes partiellement reporté »).
- Aucune US voisine à corriger : US-FW-017 renvoyait déjà le classement final et la fin de partie à US-FW-018 ; le `README` passe US-FW-018 en « Rendu », « Partielle », les US-FW-019 à 022 restent « Vides ».

## #62 — docs(specs): write US-FW-019, show the wait for the game master and the game introduction

**Contexte** — US-FW-019 était « Vide » (décisions bloquantes) et le dernier handoff (`2026-11-22-1840`) proposait une session de code, non cette US : session menée en **cadrage seul**. Les maquettes `start-04` (D4, attente du maître du jeu) et `start-05` (D5, présentation : « Quiz #08 », thème, questions, joueurs) viennent de la décision #33, qui laissait ouvert : D4 remplace-t-il « Connecté » ou lui succède-t-il, ce que numérote « #08 », la mise à jour du nombre de joueurs. Le hub n'envoie **rien** au buzzer au démarrage : `game_state_sync` part vers Angular (US-010 CA-61), US-019 CA-16 et CA-20 n'envoient aucun message pour une partie `PENDING` ou `COMPLETED`, et `game_resumed` n'est pas émis. Le hub n'a ni numéro de partie ni thème de partie ; il a le nom du quiz (3 à 40 caractères, US-008 CA-3), son nombre de questions (au moins dix, CA-1) et 1 à 10 participants (US-010 CA-8) ; il assigne les buzzers au `POST /start` (CA-65 à CA-67). Mesures (hors dépôt, `TextLayout` du dépôt, Oswald 48 px, zone 780 × 400 px) : D4 en deux lignes de 472 et 548 px, 128 px de haut ; D5 en trois lignes, 192 px, nom de 40 caractères 716 px (une ligne), 40 « W » sur deux lignes (256 px au pire) : tout tient à 48 px.

**Décision** (cadrage validé par Olivier, 2026-11-22, une salve de quatre questions, les quatre recommandations retenues) —
- **D4 remplace « Connecté »** : le texte de chaque `auth_success` (ouverture d'une connexion) devient « En attente du maître du jeu » puis « La partie va bientôt commencer » (CA-1). Aucun message hub à ajouter. Écartés : un message `game_pending` après `auth_success` (contredit US-019 CA-16 et CA-20, « Connecté » reste lisible un temps non mesuré), D4 reporté (« Connecté » resterait le seul écran d'attente).
- **D5 : nom du quiz, questions, joueurs**, par un nouveau message `game_started` (nom **proposé**), émis au `POST /games/:id/start` à chaque buzzer connecté, avec `quiz_name`, `question_count` et `player_count` (CA-2, CA-3). « Quiz #08 » et le thème sont abandonnés (aucune source côté hub). Écartés : le nom du quiz seul (le hub connaît les nombres), la maquette entière (le hub créerait un numéro et un thème de partie : D5 serait restée en constat).
- **`player_count` = buzzers assignés** : le plus petit du nombre de buzzers connectés et du nombre de participants (US-010 CA-65 à CA-67), donc ceux qui peuvent jouer. Écartés : le nombre de participants créés (« 6 joueurs » avec 4 buzzers), aucun nombre de joueurs.
- **`game_started` ne porte pas le nom du participant** : décision hub reportée (constat 4). Écarté : `participant_name` dans `game_started` (perdu à toute reconnexion, `game_resumed` n'en ayant pas ; trois US voisines à corriger).

**Choix de Claude, à relire en PR** —
- **`game_started` rendu dans tous les modes sauf Éliminé et remet le buzzer Inerte** (CA-4) : une nouvelle partie repart d'un état propre, comme `final_ranking` (#61) ; contraire du classement provisoire (#60, ignoré si armé). Un message inexploitable désarme quand même (constat 7).
- **Libellés de D4 repris de la maquette** (« En attente du maître du jeu », « La partie va bientôt commencer », décision #33, proposés par Claude, absents du CDC) ; « La partie va bientôt commencer » est optimiste hors partie (constat 5).
- **Écrans sans clignotement ni minuterie** (CA-5), restent jusqu'au prochain écran (#37, #61) ; retenus comme tout écran (US-FW-015 CA-2).
- **Bandeau sans points ni rang** (CA-7) : le « 0 pt » de la maquette n'a aucune source côté hub.
- **Monochrome, 48 px, trois lignes de même taille** (CA-3, CA-8) : ni anneau, ni capsule, ni couleur (constat 2). « questions » toujours au pluriel (au moins dix), « 1 joueur » au singulier.
- **Aucun nombre à tenir à jour** : D4 n'en montre pas, D5 est figé au démarrage (constat 5).
- **Les mentions de « Connecté »** des US-FW-011, 016, 017 et 018 ne sont pas réécrites : elles se lisent « D4 » (le `README` le dit à la ligne §8.2 init).
- 8 CA : 0 tenu, 4 en partie (CA-1, CA-6, CA-7, CA-8), 4 non implémentés (CA-2 à CA-5) ; 7 logique, 1 adaptateur ; huit constats reportés. Statut de l'US : « Partielle ». Aucun test, aucun code, aucun smoke test dans cette session.

**Conséquences** —
- Écarts de code : `DisplayState::LcdTextFor` rend « Connecté » seul ; aucun champ de `game_started` lu (`HubMessage`, `MessageDeserializer`), aucun modèle ni texte de D5, aucun aiguillage ni transition `BuzzerBehavior` pour ce message, `ScreenPresenter::TopBarFor` ne montre le bandeau que pour `auth_success`. Quatre tests figent l'ancien « Connecté » : `DisplayStateTest.AuthSuccessShouldDisplayConnecteOnScreen`, `ScreenPresenterTest.AuthSuccessPresentsTheTextConnecte`, `LcdRendererTest.AuthSuccessMessageShowsConnecteOnScreen`, `ConnectionEventHandlerRenderingTest.AuthSuccessIsRenderedWithTheUsernameOfTheHubInTheTopBar`.
- Hub : une session `docs(specs)` ajoute `game_started` (corps, diffusion à tous les buzzers connectés, émission **après** l'assignation des buzzers) et tranche le chemin `PUT`/`PATCH` du statut vers `OPEN` (US-010 CA-23, CA-36, CA-42) ; le firmware ne se smoke-teste qu'avec `scripts/fake-hub.ps1`, à étendre avec le `feat`.
- Hub, déjà noté : un message qui donne au buzzer son participant (`game_started` en est le porteur naturel, avec `game_resumed`).
- Dettes : une ajoutée (« Attente du maître du jeu et présentation de la partie : preuves manquantes et tests à écrire »), sept complétées (« Table de rendu §8.2 incomplète », « `buzz_locked`, `question_open`, `question_title` et `question_result` jamais émis par le hub », « Bandeau du haut sur le seul écran « Connecté » », « Nom du participant absent du protocole », « Écrans d'init et d'attente : preuves manquantes », « Rendu cible des maquettes partiellement reporté », « Attente du maître du jeu et présentation de la partie maquettées sans message ni report », recentrée sur le reste).
- Trois US voisines corrigées d'une phrase : US-FW-010 (CA-3, CA-4), US-FW-015 (CA-7), US-FW-007 (CA-6) ; le `README` passe US-FW-019 en « Rendu », « Partielle », les US-FW-020 à 022 restent « Vides ».

## #63 — docs(specs): write US-FW-020, show the photo or the sound before a question

**Contexte** — US-FW-020 était « Vide » (décisions bloquantes) et le dernier handoff (`2026-11-22-1855`) proposait une session de code (`fix(game): arm only from the inert mode`), non cette US : session menée en **cadrage seul**. Les maquettes `pre-01-photo` et `pre-02-sound` viennent de la décision #34, qui laissait ouvert : le message déclencheur, ce qui met fin à l'écran, la photo pendant la question, la source et le format de l'image. Le hub n'envoie **rien** au buzzer pour un média : `question_title` ne porte que l'index, le type, le titre et `time_limit` (US-011 CA-5), `play_sound_url` diffuse un jingle (US-017 CA-22, CA-29), `image_path` et `audio_path` ne sont que des colonnes de la question et des champs de `PATCH` (US-005, US-007 CA-14, CA-15) ; il **ne sert aucun fichier** (US-016 non implémentée à `ed6bd31`). Il accepte JPEG, PNG, GIF et WebP jusqu'à 10 Mo (US-016 CA-1, CA-11) ; LovyanGFX 1.3.0 (`dd228d1`) décode BMP, PNG et JPEG baseline.

**Décision** (cadrage validé par Olivier, 2026-11-22, une salve de quatre questions, les quatre recommandations retenues) —
- **Déclencheur : un message dédié `question_media`** (nom **proposé**), avec `question_index`, `media` (`image` ou `sound`) et `url` (CA-1). Écartés : des champs ajoutés à `question_title` (même instant que l'écran du titre, US-FW-011 et US-FW-012 à réécrire, une règle d'ordre pour deux médias), aucun déclencheur (deux CA seulement).
- **Photo : JPEG baseline et PNG**, sinon échec de chargement (CA-8, CA-9). Écartés : un hub qui convertit en JPEG baseline de 764 × 400 px (une dépendance et une session hub de plus), un format non fixé.
- **Pendant le chargement : l'écran « Question N » aussitôt**, la photo le remplace une fois décodée, un échec le laisse (CA-3, CA-8). Écartés : l'écran courant gardé jusqu'à la photo (le joueur ne comprend rien pendant l'attente), chargement et échec reportés.
- **Son : l'écran seul** ; la lecture (HTTP, décodage, haut-parleur) est celle de US-FW-021 (constat 8). Écartés : écran et lecture ici (CA non décidables : le haut-parleur reste un point ouvert, CDC §10 n°2), la photo seule.

**Choix de Claude, à relire en PR** —
- **`url` complète**, comme `play_sound_url` (US-017 CA-29), composée par le hub ; une question avec photo **et** son = deux messages, le second remplace le premier (CA-5).
- **Libellés** : photo « Question N » (une ligne), son « Question N » puis « Écoute bien ! » (deux lignes), N = `question_index` + 1 comme `question_title` (CA-2, CA-3, CA-10).
- **Un buzzer armé ignore le message** (CA-4, comme US-FW-017 CA-4) ; Éliminé : silence total ; il remplace une réaction ; le mode ne change pas (CA-4, CA-6). Contraire de `game_started` (#62) et de `final_ranking` (#61), qui désarment : ce message arrive entre deux questions, un buzzer armé est l'anomalie, et ne pas le désarmer protège la question en cours.
- **L'écran reste jusqu'au message suivant, sans minuterie** (CA-5, #37) ; la photo n'est pas gardée pendant la question (constat 6).
- **Abandon à 10 s** (CA-8, CA-9) : un tiers du ping du hub (30 s), valeur provisoire non mesurée ; une photo qui arrive après un autre écran n'est jamais dessinée (CA-8).
- **L'écran retenu est l'attente, pas la photo** (CA-5, constat 5) : le décorateur de US-FW-015 retient un modèle, une photo exigerait un nouveau téléchargement hors réseau.
- **Photo recadrée au centre dans 764 × 400 px, cadre blanc ; texte monochrome de 48 px** ; ni capsules, coins arrondis, thème, haut-parleur ni barres d'égaliseur (CA-9, CA-10, constat 7) ; bandeau sans points ni rang (CA-7).
- 10 CA : 0 tenu, 2 en partie (CA-6, CA-10), 8 non implémentés ; 8 logique, 2 adaptateur ; neuf constats reportés. Statut de l'US : « Partielle ». Aucun test, aucun code, aucun smoke test dans cette session.

**Conséquences** —
- Écarts de code : `HubMessage` ne porte ni `media` ni `url` et `MessageDeserializer::Parse` ne les lit pas ; aucun modèle ni texte pour ces écrans ; `ScreenPresenter::ScreenFor` ne lit l'état armé que pour `timer_end` ; `HubMessageDispatcher` n'a rien à changer ; `GfxScreen` ne dessine aucune image et aucun `GET` d'image n'existe (`src/HttpTokenFetcher.cpp` : un `POST` seulement). Aucun test ne fige un ancien rendu à réécrire : les trois `MessageWithoutDisplayTest` ne portent que sur `play_sound_url` et `play_system_sound`, qui restent sans écran.
- Hub : une session `docs(specs)` ajoute `question_media` (corps, déclencheur Angular, composition de l'`url`) et implémente US-016 (service des fichiers) avant tout `feat` ; le firmware ne se smoke-teste qu'avec `scripts/fake-hub.ps1`, à étendre (touches et petit serveur de fichiers).
- Décision #34 : son « à définir côté hub » reste vrai côté hub ; côté firmware, le déclencheur, la fin de l'écran, la photo pendant la question et la source de l'image sont tranchés.
- Dettes : une ajoutée (« Photo ou son avant la question : preuves manquantes et tests à écrire »), huit complétées (« Photo et son avant la question maquettés sans message ni report », « Table de rendu §8.2 incomplète », « `buzz_locked`, `question_open`, `question_title` et `question_result` jamais émis par le hub », « Un `POST /api/v1/token` synchrone à chaque connexion », « Dépendances flottantes », « `fake-hub.ps1` validé au smoke test seulement », « Rendu cible des maquettes partiellement reporté », « Bandeau du haut sur le seul écran « Connecté » »).
- Deux US voisines corrigées d'une phrase : US-FW-017 (CA-4 : « cinq » devient « six » messages lus avec l'état armé) et US-FW-012 (CA-13) ; le `README` passe US-FW-020 en « Rendu », « Partielle », les US-FW-021 et 022 restent « Vides ».

## #64 — docs(specs): write US-FW-021, play the sounds of the game

**Contexte** — US-FW-021 était « Vide » (le haut-parleur et le son du feedback local restaient ouverts, CDC §10 n°2) et le dernier handoff (`2026-11-22-1913`) proposait une session de code (`fix(game): arm only from the inert mode`), non cette US : session menée en **cadrage seul**. Le réel : aucun code audio (`lib/`, `src/`, `platformio.ini`), `HubMessage` sans `sound_id` ni `url`, `HubMessageDispatcher` à quatre branches, aucun `GET` de fichier (`src/HttpTokenFetcher.cpp` : un `POST` seulement). Le CDC §8.1 et §8.2 laissent l'écran inchangé pour `play_sound_url` et `play_system_sound` et les donnent valables en « tout » mode. Le hub **n'émet aucun son et ne sert aucun fichier** : `src/` n'a aucune occurrence de `SYSTEM_SOUNDS`, `sendSystemSound`, `play_system_sound`, `play_sound` ni de `uploads` à `ed6bd31` ; US-018 (dix `sound_id`, CA-17 « joue le son préchargé », CA-18 « séquentiel, file d'attente côté firmware ») et US-017 (jingles mp3, wav, ogg de 10 Mo au plus, CA-29 `url` absolue) ne sont pas implémentées. Le matériel (décision #1) : CrowPanel 5″, haut-parleur I²S sur les GPIO 17, 18 et 42 d'après le wiki d'Elecrow, schéma non lu. Mesures (hors dépôt) : WAV 16 bits mono, 31,2 Kio/s à 16 kHz, dix sons de 1,5 s = 469 Kio (646 Kio à 22,05 kHz, 1 292 Kio à 44,1 kHz) ; 253 Kio libres dans l'application, 1 408 Kio de partition de données supposés (`default.csv` 4 Mo, table non lue) ; PSRAM non lue (8 Mo supposés, tampon d'image 768 000 o) ; 10 Mo en mp3 à 128 kbit/s = 625 s. `ESP32-audioI2S` lue au tag 4.0.1 : wav, mp3, vorbis (ogg), flux HTTP ou fichier, tâche audio propre, PSRAM requise, GPL-3.0.

**Décision** (cadrage validé par Olivier, 2026-11-22, une salve de quatre questions, les quatre recommandations retenues) —
- **Sons système : un WAV 16 kHz mono par identifiant en LittleFS**, `<sound_id>.wav` (CA-11). Écartés : des tableaux C dans le binaire (469 Kio pour 253 Kio libres), un téléchargement au démarrage depuis le hub (aucun fichier servi, dix requêtes de plus à chaque démarrage).
- **Jingle : flux HTTP lu au fil du téléchargement** (CA-12). Écartés : un téléchargement complet en PSRAM (10 Mo contre 7,6 Mo libres supposés), un constat reporté sans lecture.
- **Aucun son local à l'appui armé** (CA-9) : le son vient du hub, après un aller-retour réseau non mesuré ; un appui MCQ reste muet. Tranche CDC §10 n°2. Écartés : un son local SPEED avec le doublon du hub ignoré (une règle de plus), un constat reporté.
- **Ambiance interruptible, file de quatre** (CA-3, CA-4) : `WAITING` et `SUSPENSE` sont coupés par tout autre son ; les autres restent séquentiels (US-018 CA-18). Écartés : séquentiel strict (un `SUSPENSE` de 20 s retarderait `BUZZ_LOCKED`), un nouveau son qui coupe toujours le courant (contredit CA-18).

**Choix de Claude, à relire en PR** —
- **Dans tout mode sauf Éliminé** (CA-1, CA-5) : colonne mode « tout » du CDC ; l'Éliminé coupe le son en cours et vide la file, comme le silence total de US-FW-006 CA-9.
- **Message inexploitable, échec ou abandon à 10 s : silence** (CA-2, CA-8), aucun écran d'erreur, aucune relance, la file passe au suivant ; 10 s reprend la valeur provisoire de US-FW-020 CA-9.
- **Perte de liaison : file vidée, flux coupé, son préchargé terminé** (CA-6) : un son d'événement passé est périmé.
- **Un jingle suit la file et n'est pas interruptible** (CA-3, constat 7) ; **sons lus une fois**, sans boucle ; **volume fixe et unique** (CA-13, constat 5).
- **Le son d'un `question_media` de type son est un jingle** (CA-7), muet si le buzzer est armé (US-FW-020 CA-4) ; l'écran « Écoute bien ! » n'attend pas le son (US-FW-020 CA-2).
- **Lecture non bloquante** (CA-10) : un appui armé pendant un son est pris comme sans son.
- **Bibliothèque candidate `ESP32-audioI2S` 4.0.1, hors CA** : GPL-3.0, PSRAM requise, non épinglée dans `platformio.ini`, mémoire et chemin réseau non mesurés (constat 4) ; à confirmer avant le `feat`.
- **Couche « Son », statut « Partielle »** ; 13 CA : 0 tenu, 2 en partie (CA-1, CA-9), 11 non implémentés ; 10 logique, 3 adaptateur ; huit constats reportés. Aucun test, aucun code, aucun smoke test dans cette session.

**Conséquences** —
- Écarts de code : aucun champ `sound_id` ni `url` lu (`HubMessage`, `MessageDeserializer::Parse`), aucun lecteur ni file, aucune branche pour les `play_*` dans `HubMessageDispatcher`, aucun appel dans `ConnectionEventHandler::OnMessageReceived`, aucune bibliothèque audio dans `platformio.ini`, aucun I²S dans `setup()`, aucun fichier audio, aucun port de liaison (US-FW-009 CA-4) pour vider la file. Aucun test ne fige un ancien rendu à réécrire : `MessageWithoutDisplayTest.LeavesTheScreenUnchanged` (`test_lcd_renderer`, `test_screen_presenter`) et `MessageWithoutDisplayTest.ShouldHaveNoTextToDisplay` (`test_rendering`) restent vrais, les sons n'ayant pas d'écran.
- Hub : US-018 et US-017 sont à implémenter avant tout `feat` ; US-018 CA-18 est précisée côté firmware (ambiance interruptible), à reporter au hub ; `question_media` reste à spécifier (décision #63) ; le firmware ne se smoke-teste qu'avec `scripts/fake-hub.ps1`, à étendre (touches de sons, petit serveur de fichiers audio).
- Dépôt : partition de données partagée avec les personnages (469 Kio de sons + 363 470 o d'images, environ 59 % de 1 408 Kio supposés) ; schéma de la carte à lire pour le haut-parleur et pour les broches du bus RGB.
- Dettes : une ajoutée (« Sons : preuves manquantes et tests à écrire »), sept complétées (« Dialogue avec le hub : preuves manquantes », « `buzz_locked`, `question_open`, `question_title` et `question_result` jamais émis par le hub », « Dépendances flottantes », « Réactions du personnage : stockage et décodage des images », « `fake-hub.ps1` validé au smoke test seulement », « Photo et son avant la question maquettés sans message ni report », « Bus RGB sur des broches à double rôle »).
- Trois US voisines corrigées d'une phrase : US-FW-004 (CA-5 : les `play_*` jouent un son), US-FW-006 (CA-9 : ni joué, son coupé) et US-FW-020 (constat 8 : lecture du son tranchée) ; le `README` passe US-FW-021 en couche « Son », « Partielle » ; seule US-FW-022 reste « Vide ».

## #65 — docs(specs): write US-FW-022, show the configuration screen

**Contexte** — US-FW-022 était « Vide, bloquée » (canal d'appairage, déclencheur et contenu du QR non décidés, application mobile inexistante) et le dernier handoff (`2026-11-22-1929`) proposait une session de code (`fix(game): arm only from the inert mode`), non cette US : session menée en **cadrage seul**. Le réel : `ScreenModel` à six variantes sans QR ni engrenage, aucune occurrence de `qrcode`, `nfc`, `nvs` ou `Preferences` dans `lib/`, `src/`, `test/`, Wi-Fi, compte et adresse du hub compilés (`secrets.h`, `main.cpp`) : un binaire par buzzer pour les dix comptes du hub (US-002 CA-1). Le hub (`ed6bd31`) n'a aucun point d'accès ni message de configuration ; sa VISION mentionne une application mobile Android et NFC. La CrowPanel 5″ n'a pas de NFC d'après Elecrow (carte TF, 2 GPIO, haut-parleur, UART0, 2 I²C, batterie), et l'I²C 19 et 20 sert au tactile. Mesures (hors dépôt, `segno`, niveau bas, en caractères) : QR de 26 caractères = version 2 (25 modules, 9 px dans les 248 px de la carte), 41 (`WIFI:`) = version 3 (8 px), 140 = version 7 (5 px), 239 = version 10 (4 px) ; version 11 = 321 octets au plus (61 modules, 4 px). `LGFXBase::qrcode` (`dd228d1`, 1.3.0) : niveau bas fixe, `alloca` à chaque version essayée, 2 541 octets cumulés jusqu'à la version 11, 59 700 jusqu'à la 40.

**Décision** (cadrage validé par Olivier, 2026-11-22, une salve de quatre questions, les quatre recommandations retenues) —
- **Périmètre : l'écran D6 dormant, le reste en constats reportés** (CA-1 à CA-6). Écartés : un dossier de décision sans CA (D6, pourtant décidé par #33 et #36, resterait non cadré), D6 plus des règles de déclencheur et de sortie (des CA sans stockage ni canal, non vérifiables).
- **Cible de la configuration : Wi-Fi, adresse du hub et compte du buzzer, reçus à l'exécution** (constat 3), pour un seul binaire pour les dix buzzers. Écartés : le Wi-Fi seul (la VISION du hub ; dix binaires), un report.
- **Canal d'appairage : reporté** (constat 1). Écartés : un point d'accès temporaire avec un QR `WIFI:` (le buzzer coupe son Wi-Fi station, serveur HTTP non mesuré en flash), le BLE (pile non mesurée, cohabitation avec le Wi-Fi non vérifiée), le NFC (absent de la carte).
- **Déclencheur et sortie de D6 : reportés** (constat 5). Écartés : l'absence de configuration enregistrée (exige un stockage, modifie US-FW-005 CA-1), l'appui long (l'init est bloquant, US-FW-005 CA-11), le `401` (modifie US-FW-015 CA-9 et US-FW-005 CA-5).

**Choix de Claude, à relire en PR** —
- **Borne de 321 octets** (CA-2) : version 11, 61 modules, 4 px par module ; au-delà, la pile de la tâche est exposée à l'`alloca` en boucle de LovyanGFX (59 700 octets cumulés au pire, pile non lue). Elle couvre le pire contenu mesuré (239 caractères).
- **Repli lisible** (CA-2) : une chaîne vide, de plus de 321 octets ou contenant un octet nul donne D6 **sans carte**, le titre seul, plutôt qu'un écran inchangé ou un QR partiel.
- **Rendu sans engrenage ni anneau** (CA-5, constat 6) : titre blanc de 48 px (la plus grande des trois polices embarquées, 235 px), comme l'habillage de D4 et D5 reporté par US-FW-019 constat 2. C'est un écart avec la maquette validée en #36.
- **Bandeau d'init sans batterie, de 60 px** (CA-1) : le réel gagne sur la maquette (batterie 78 %, 44 px).
- **Géométrie reprise de la maquette** (CA-5) : carte de 288 px, remplissage de 20 px, écart de 48 px, ligne de 571 px centrée.
- **D6 local et statique** (CA-1, CA-4) : aucun message, aucun mode, aucune animation.
- **Couche « Rendu », statut « Partielle »** ; 6 CA : 0 tenu, 1 en partie (CA-4), 5 non implémentés ; 4 logique, 2 adaptateur ; sept constats reportés. Aucun test, aucun code, aucun smoke test dans cette session. Le CDC §2 (module NFC) n'est pas modifié.

**Conséquences** —
- Écarts de code : aucune variante d'écran de configuration, aucune méthode de `ScreenPresenter`, aucun masquage des zones pour D6, aucun dessin de QR, aucun stockage de configuration. Aucun test ne fige D6 ; `LoginOutcomeDisplayTest.ShowsTheRefusalScreenWithoutTopBarWhenTheCredentialsAreRefused` fige encore un écran de refus sans bandeau (constat 5).
- Hub : aucun changement ; le point d'accès ou le message de configuration dépendent du canal. Le CDC §2 contredit le matériel (pas de NFC) : à corriger à la suppression du CDC.
- Dettes : une ajoutée (« Configuration (écran D6) : preuves manquantes et tests à écrire »), cinq complétées (« Paramétrage NFC non spécifié », « Configuration maquettée sans déclencheur, canal ni report », « Adresse du hub en dur, en HTTP clair », « Attente du maître du jeu et présentation de la partie… » dont la phrase « non maquetté » était fausse depuis #36, « Dépendances flottantes »).
- Aucune US voisine contredite ; le `README` passe US-FW-022 en couche « Rendu », « Partielle » : plus aucune US n'est « Vide ».

## #66 — feat(reconnection): show the out-of-game screen after the third failed reconnection

**Contexte** — Le smoke test du 2026-11-24 (C03) a constaté qu'après l'élimination (US-FW-006 CA-3, réseau arrêté par la décision de `fix(reconnection): stop all network activity once the buzzer is eliminated`) l'écran resté affiché était « Connecté » : `IsEliminated()` devenait vrai et `ConnectionEventHandler` arrêtait le réseau, mais rien ne dessinait l'écran « Hors jeu » de US-FW-015 CA-8 et CA-12 (décision #58). Le CDC §4.4 et §8.2 (ligne `(réseau : 3ᵉ échec consécutif…)`) donnent le contenu : anneau avec le symbole marche/arrêt, « Hors jeu », « Connexion perdue après 3 tentatives », « Redémarre le buzzer pour revenir », zones masquées, terminal. Un bandeau du haut sans état n'existe pas dans le code : `TopBarFor` ne le rend qu'avec `auth_success`, et aucun port d'état de liaison n'existe (US-FW-009 CA-4).

**Décision** (cadrage validé par Olivier, 2026-11-24) —
- **Bandeau de « Hors jeu » : le dernier nom connu, sans icône de liaison** : le nom est l'`username` du dernier `auth_success` (US-FW-009 CA-1), conservé jusqu'à l'élimination ; l'icône de liaison barrée n'est pas dessinée (US-FW-009 CA-8, hors périmètre, la dette « Bandeau du haut sur le seul écran « Connecté » » reste). Écarté : un port d'état de liaison dans cette session (c'est le port de la coupure, CA-1, à écrire avec elle).
- **Type de commit `feat`** : comportement observable modifié (un écran de plus à l'élimination).

**Choix de Claude, à relire en PR** —
- **Un port `OutOfGameDisplay` (`lib/connection/`, `ShowOutOfGame()`), séparé de `MessageRenderer`**, plutôt qu'une méthode de plus sur `MessageRenderer` : l'élimination n'est pas un message du hub et les doubles de `MessageRenderer` n'ont pas à la connaître. `LcdRenderer` réalise les deux interfaces ; `main.cpp` lui passe le même objet deux fois. `ConnectionEventHandler` prend le port en paramètre (avant `NetworkHalt`).
- **Le dernier nom est retenu par `LcdRenderer`** (`lastVisibleTopBar`, mis à jour à chaque message dont le bandeau est visible, donc aujourd'hui au seul `auth_success`), pas par `ConnectionEventHandler` ni par `ScreenPresenter` (sans état) : c'est lui qui connaît déjà le rendu des messages. Sans `auth_success` jamais reçu (impossible en production : la politique n'est armée qu'au premier), le bandeau est visible, nom vide (US-FW-009 CA-1). Le `feat` qui retient le nom pour tous les écrans (US-FW-009 CA-3) reprend cet état.
- **Le texte vit dans le modèle** : `OutOfGameScreen{title, cause, wayOut}`, rempli par `ScreenPresenter::ScreenForOutOfGame()` d'après `DisplayState` ; `GfxScreen` ne porte aucun libellé et reste un simple dessin. Les trois chaînes sont donc testées en natif. Écarté : un modèle sans champ avec les libellés dans `GfxScreen` (non testables).
- **Affichage une seule fois, jamais sur un réseau déjà arrêté** : `PropagateEliminationIfAny` n'appelle `ShowOutOfGame()` que si `NetworkHalt` n'est pas arrêté, avant d'arrêter le réseau. Une élimination repassée après l'arrêt (échecs reçus ensuite) ne redessine pas ; un réseau arrêté par un `401` en reconnexion (US-FW-006 CA-10, écran de refus, terminal) ne peut pas être recouvert par « Hors jeu » (US-FW-015 CA-10 : le refus ne passe pas par « Hors jeu »).
- **Zones masquées par le repli générique** de `TappableZonesScreen` (aucun code ajouté, un test le fige). Terminal : les messages ignorés (US-FW-006 CA-9) et le mode Éliminé qui ignore les appuis (US-FW-002 CA-9) suffisent ; rien n'est ajouté.
- **Dessin au pinceau rond** : anneau de 128 px = un disque rose et un disque noir de 9 px de moins (`fillCircle`, rayon 64 : 129 px au pixel près) ; symbole marche/arrêt de `fail-02-out.html` mis à l'échelle 128/36 (arc de rayon 29 px, centre 3 px sous celui de l'anneau, ouverture de 86° ; barre de 30 px), tracé par disques de 9 px plutôt que par `fillArc`, dont le sens des angles n'a pas pu être vérifié ici. Écart de 1 px sur l'anneau assumé, à voir au smoke test.
- **Mesures de la spec reprises telles quelles** (titre 62 px, lignes 50 px, écarts de 24 px, pile de 362 px) et non les hauteurs de ligne de `measurer` (64 et 50 px à 90 % d'interligne) : la pile ne ferait plus 362 px.

**Conséquences** —
- Écarts de code restants : pas de liaison barrée au bandeau, pas d'écran de coupure ni de remplacement d'une réaction, son non coupé (US-FW-021 CA-5).
- Le bandeau n'est dessiné par `GfxScreen` qu'avec un `TextScreen` et « Hors jeu » ; les autres modèles l'ignorent toujours.
- `LcdRenderer` a deux responsabilités de rendu (messages et élimination) : à rediscuter si la coupure (CA-1) ajoute un troisième port.
- Dettes : « Éliminé : écran « Hors jeu » et son coupé non implémentés » réduite au son coupé (renommée « Éliminé : son coupé non implémenté »), « Écrans de coupure : preuves manquantes et tests à réécrire » et « Bandeau du haut sur le seul écran « Connecté » » ajustées. Aucune dette ajoutée.
- Vérifié : suites natives (voir le handoff) ; `src/GfxScreen.h` et `src/main.cpp` non compilés en local (PlatformIO absent, registre bloqué) : la CI de la PR fait foi.

## #67 — feat(reconnection): show the cut-off screen when the established connection is lost

**Contexte** — Après #66 (« Hors jeu »), la perte de la connexion établie ne produisait toujours aucun écran : `ConnectionEventHandler::OnConnectionLost` n'appelait que le compteur, un buzzer armé envoyait encore `answer` ou `buzz` et affichait son feedback pendant la coupure, le bandeau du haut n'avait aucune liaison (US-FW-009 CA-4, CA-8) et `GfxScreen` gardait lui-même les propositions et la zone choisie (dette « Textes des zones retenus par l'adaptateur »). La décision #58 avait tranché le comportement (écran retenu à la couture `Screen`, appuis ignorés, mode conservé) ; la décision #66 avait écarté le port d'état de liaison « à écrire avec la coupure ».

**Décision** (périmètre fixé par la demande de session, 2026-11-24 ; les CDC font foi, aucun trou constaté sur les CA visés) —
- **Visés** : US-FW-015 CA-1, CA-2, CA-4, CA-6, CA-7, CA-11 ; US-FW-009 CA-4 et CA-8 ; la dette des textes de zones retenus par l'adaptateur. **Hors périmètre** : CA-5 de US-FW-015 (aucune réaction n'existe), le reste de US-FW-009, CA-3 et CA-8 à CA-10 de US-FW-015 (déjà faits ; la liaison barrée de « Hors jeu » vient avec la liaison du bandeau).
- **Type de commit `feat`** : comportement observable modifié (un écran de coupure, des appuis ignorés, une icône de liaison).

**Choix de Claude, à relire en PR** (aucun ne vient des CDC) —
- **Un port `LinkStateDisplay` (`lib/connection/`, `ShowLinkLost(int attempt)`)** réalisé par `LcdRenderer`, appelé par `ConnectionEventHandler` à la perte de la connexion établie et à chaque échec compté (jamais avant le premier `auth_success`, jamais une fois le réseau arrêté : l'élimination l'arrête, donc jamais « 4/3 »). **Un seul sens** : « liée » est l'`auth_success` lui-même (`TopBarFor`), un second appel de port n'aurait aucun effet visible. Écarté : un port à deux sens (mort aujourd'hui, à ajouter avec US-FW-009 CA-3 quand tout écran portera le bandeau).
- **`RetainingScreen` (`lib/rendering/`)** : décorateur de `Screen` entre les deux producteurs et `TappableZonesScreen`. Il retient le dernier écran affiché, ignore `ScreenUnchanged`, et complète le `ConnectionLostScreen` que `LcdRenderer` émet avec ce qu'il atténue (`dimmed`) et si la coupure était déjà à l'écran (`bannerOnly`). Il retient aussi les propositions et la zone choisie (la règle de l'ancien état de `GfxScreen`, déplacée telle quelle) et les met dans `HighlightedZoneScreen`, `ChosenZoneScreen` et `BannerScreen` : `GfxScreen` ne garde plus aucun état, ce qui traite la dette. Écartés : garder l'état dans `GfxScreen` et ne retenir que le modèle (la coupure d'après une reconnexion redessinerait les propositions de la question précédente), le mettre dans `DisplayState` (deux instances, #58). **Défaut assumé** : un même type de modèle est « partiel » côté producteur et « complet » après le décorateur (`zoneTexts`, `chosenZone`, `dimmed`, `bannerOnly` vides à la production) ; des types distincts auraient doublé le visiteur de `GfxScreen`.
- **`bannerOnly` décidé en logique** (testable) plutôt que par un état de `GfxScreen` : à chaque échec compté, seul le bandeau rouge est repeint (CA-11).
- **Sous la coupure, le texte d'en-tête d'un résultat n'est pas redessiné** : les propositions sont dessinées de y 120 à la fin de la dalle (380 × 180 px par zone, 380 × 160 px utiles, comme la spec) et le bandeau rouge prend la place de l'en-tête ; un texte plein écran est centré dans la même zone. La spec ne dit pas comment redessiner un écran qui portait un texte d'en-tête et des zones.
- **Icône de liaison** (US-FW-009 CA-8) : la maquette `qcm-02-choices.html` (viewBox 24 × 18) à l'échelle 1,5 (36 × 27 px), à 10 px du bord droit, tracée au pinceau rond (comme l'anneau de #66) ; coupée : **rose `#ff8fa6`**, arcs et point à 55 %, barrée. Le critère dit « rouge » ; la maquette (`.link.lost`) et la géométrie de US-FW-015 donnent ce rose aux icônes, le rouge `#e8174f` étant celui du bandeau de coupure. Taille et tracé sont des choix de session.
- **Couleurs en RVB 888 (`uint32_t`)** dans `GfxScreen` : lu dans les en-têtes de LovyanGFX (`LGFXBase.hpp`, `colortype.hpp`, dépôt cloné hors du dépôt, `dd228d1`), un `int` (`TFT_WHITE`) est lu en RVB 565 et un `uint32_t` en RVB 888 ; `DrawTextIn` et `DrawZones` prennent donc un `uint32_t`. Les constantes sont `static constexpr` (un `static const` lié à un `const T&` demanderait une définition hors classe).
- **Les appuis** : `BuzzerBehavior::OnLinkLost` / `OnLinkEstablished` (un indicateur `isLinkCut`, « liée » par défaut : aucune coupure avant le premier `auth_success`), joints par `HubMessageDispatcher` comme `OnEliminated`. La coupure commence à la perte de la connexion établie, **se rétablit à l'`auth_success`**, pas à l'ouverture de la connexion (CA-4, CA-7).
- **`ReconnectionPolicy` expose `IsArmed()` et `CurrentAttempt()`** (échecs comptés + 1). La garde `IsEliminated()` de `ShowLinkLostIfAny` a été retirée : le mutant qui la supprimait survivait (l'élimination arrête toujours le réseau avant, `networkHalt.IsHalted()` suffit).
- **`ArmedBuzzerStillAnswersAfterThreeDisconnections` réécrit** (`…OnceTheAuthSuccessIsReceived`) : il figeait l'appui qui part pendant la coupure, que CA-4 interdit. Les attentes de « Hors jeu » passent à la liaison coupée (CA-8), comme le dit l'US.
- **Les libellés sont dans le modèle** (`ConnectionLostScreen{title, attemptLabel, attempt, attemptCount, …}`, `DisplayState`), comme « Hors jeu » en #66 ; le « 3 » de « Tentative N/3 » est une constante de `DisplayState`, doublon assumé du seuil de `ReconnectionPolicy`.
- **Nouveau répertoire de test** `test/test_retaining_screen` (27 répertoires). `LoginOutcomeDisplay` reste branché directement sur `TappableZonesScreen` (écran de refus terminal).

**Conséquences** —
- US-FW-015 : CA-1, CA-2, CA-3, CA-4, CA-6 tenus par des tests natifs ; CA-7 en partie (l'écran de l'`auth_success` reste « Connecté » : D4 est US-FW-019) ; CA-8 et CA-12 en partie (liaison dessinée, non constatée) ; CA-11 implémenté, non constaté ; CA-5 inchangé. US-FW-009 : CA-4 en partie, CA-8 implémenté, non constaté.
- Dettes : « Textes des zones retenus par l'adaptateur » renommée « Propositions retenues jamais effacées (`RetainingScreen`) » et réduite ; « Envoi d'une réponse ou d'un buzz sans contrôle pendant une coupure » renommée « Retour de `sendTXT` non lu… » ; sept autres ajustées (bandeau, preuves, habillage, éliminé, reconnexion, instances de `DisplayState`, table de rendu, visiteur générique). Aucune dette ajoutée.
- Risque levé par le smoke test du 2026-11-24 (voir « Correction » plus bas) : la bibliothèque émettait bien un échec **et** une fermeture par tentative ; « Tentative N/3 » avançait de deux (1/3 puis 3/3) et « Hors jeu » arrivait à la 2ᵉ tentative.

**Correction du 2026-11-24 (second commit de la PR, `fix(reconnection): count a failed reconnection attempt once`)** —
- **Constat** (dalle, hub arrêté puis relancé, 5 coupures d'environ 12 s) : « Tentative 1/3 » puis directement « 3/3 », sans « 2/3 ». Le CDC (US-FW-015 CA-1 ; US-FW-006 CA-3, CA-6, CA-7, CA-8) veut N = échecs comptés + 1, un redessin à chaque échec compté, jamais « 4/3 ».
- **Cause établie par le code** (`WebSockets` 2.7.3, dépôt à `1c8b8de`, `WebSocketsClient.cpp` : `loop()` l. 238-340 dont le connect raté l. 338, `clientDisconnect` l. 554-610, `clientIsConnected` l. 616-640 dont le nettoyage l. 636) : un connect raté laisse un `WiFiClient` non nul et non connecté ; la lecture suivante de `clientIsConnected` appelle `clientDisconnect(…, "TCP connection cleanup")`, qui émet `WStype_DISCONNECTED`. `ObservableWebSocketsClient::LoopAndTellIfTheConnectFailed` appelait `clientIsConnected` dans son propre `return` : cette fermeture arrivait donc dans le même tour que le signal `ConnectionAttemptFailed`, avant lui. Deux échecs comptés pour une tentative : 2/3 redessiné puis aussitôt 3/3 (quelques millisecondes), et élimination à la 2ᵉ tentative au lieu de la 3ᵉ. Reproduit hors dépôt avec les sources réelles de la bibliothèque (réseau simulé) : 1/3, 2/3 et 3/3 dans le même tour, puis « Hors jeu » à la 2ᵉ tentative ; après correction : 1/3, 2/3, 3/3, « Hors jeu » à la 3ᵉ.
- **Décision** : une tentative ratée = la seule fermeture « TCP connection cleanup ». `ObservableWebSocketsClient` est supprimée (plus de lecture de l'état protégé) ; `WebSocketEventListener` transmet la raison de fermeture ; `BuzzerEventTranslator` traduit cette raison en `ConnectionAttemptFailed`, toute autre en `OnConnectionLost`. Les autres fermetures d'une tentative (« HTTP 4xx », « WebSocket handshake failed », délai d'en-tête) produisent déjà un seul événement et restent comptées par `OnConnectionLost` (perte établie consommée).
- **Écarté** : ne garder que la fermeture et supprimer `BuzzerEventType::ConnectionAttemptFailed` (une soixantaine d'appels de tests à migrer, et le nom d'un échec de tentative disparaîtrait du modèle) ; filtrer le signal côté adaptateur (la fermeture arrive de toute façon au `loop()` suivant) ; relire `_lastConnectionFail` autrement (même dépendance à l'état protégé).
- **Preuves** : 3 tests natifs ajoutés dans `test_connection` (le premier était rouge avant la correction) ; 5 mutants sur le traducteur et 1 sur la politique, tous tués. Aucun test natif ne peut reproduire la chaîne réelle (bibliothèque et adaptateur) ; ni compilée pour l'`esp32` ici, ni constatée sur la dalle après correction. À challenger : la dépendance à la chaîne « TCP connection cleanup » (dette « L'échec d'une tentative se lit dans la raison de fermeture de la bibliothèque »).
- **Autre constat de la dalle, non lié** : `connect()` bloque jusqu'à 5000 ms quand l'hôte ne répond pas et l'intervalle de 5 s court depuis la fin de la tentative (`_lastConnectionFail`) : jusqu'à 10 s entre deux comptes, ce qui laisse « 1/3 » à l'écran plus longtemps que prévu par US-FW-006 CA-8 ; déjà dans la dette « Reconnexion : preuves manquantes ».
- Vérifié (voir le handoff) : les 27 répertoires natifs compilés à la main ; 57 mutants sur la logique ajoutée, tués sauf un équivalent (retiré) ; `src/GfxScreen.h` contrôlé en syntaxe contre les en-têtes réels de LovyanGFX (backend framebuffer Linux), **ni compilé pour l'`esp32`, ni vu sur la dalle** ; `src/main.cpp` non compilé. La CI de la PR fait foi pour `pio test -e native`.

## #68 — refactor(startup): make the start-up sequence testable

**Contexte** — La séquence de démarrage vivait dans `setup()` et `loop()` de `src/main.cpp`, avec la globale `startupHalted` : ni l'arrêt de `loop()` après un `401` (US-FW-005 CA-5), ni l'absence de lecture d'appui pendant le démarrage (CA-11), ni l'ordre « token, intervalle de reconnexion, ouverture du WebSocket » n'avaient de test natif (dette « Démarrage : preuves manquantes »). Le comportement est tranché par US-FW-005 et ne change pas.

**Décision** (périmètre fixé par la demande de session, 2026-11-24 ; aucun trou du CDC ni des US constaté sur CA-5 et CA-11) —
- **Type de commit `refactor`** : comportement observable inchangé. Les tests ajoutés caractérisent le comportement existant ; ils sont nés rouges (API absente, échec de compilation) puis verts.
- **Extraire vers `lib/startup/`** la séquence d'après le Wi-Fi et le tour de `loop()` ; `main.cpp` ne fait plus que câbler. Hors périmètre, intacts : l'attente du Wi-Fi et l'init matérielle dans `setup()` (dette « `setup()` bloquant »), l'écran d'init statique, « un seul statut arrête le login », tout comportement observable.

**Choix de Claude, à relire en PR** (aucun ne vient des CDC) —
- **`StartupSequence::Run()`** (login, écran, puis token, intervalle et ouverture du WebSocket, ou arrêt) rend un `StartupOutcome` (`Started` ou `Halted`) et porte l'état d'arrêt (`IsHalted()`), à la place de `startupHalted`. Elle dépend de `TokenAcquisition` et de `LoginOutcomeDisplay` **concrets** (déjà testés avec leurs faux de dessous : `MockTokenFetcher`, `MockSleeper`, `MockScreen`), pas d'interfaces de plus. Écarté : un port « connexion Wi-Fi » dans la séquence (de la logique en moins, un port en plus : la dette « `setup()` bloquant » est à part) ; fondre l'arrêt du démarrage dans `NetworkHalt` (il fermerait un client jamais ouvert : un appel de plus à `disconnect()`, ce n'est plus un refactor).
- **`BuzzerLoop::Tick()`** reprend `loop()` à l'identique (réseau, lecteur physique, lecteur tactile ; rien si le démarrage ou le réseau est arrêté). Il lit `StartupSequence` et `NetworkHalt` directement et prend les deux `ButtonPressDetector` **concrets** : c'est le lecteur d'appui (`ButtonReader`) qui prouve qu'aucune lecture n'a lieu. Écarté : un port « état d'arrêt » unique (un port pour deux booléens déjà lisibles).
- **Trois ports minces**, chacun réalisé par une classe existante ou un adaptateur : `TokenUpdater` (`UpdateToken`, réalisé par `TokenSessionMessageSender`, dont la méthode gagne `override`), `NetworkLoop` (`Loop`, réalisé par `WebSocketEventListener`, `override` sur `Loop`) et `ConnectionOpener` (`SetReconnectInterval` puis `Open`, réalisé par le nouvel adaptateur `src/WebSocketConnectionOpener`). **Deux méthodes** pour l'ouverture, pas une : l'ordre « intervalle avant `begin` » est ainsi testable.
- **L'adresse du hub reste dans `main.cpp`** : `WebSocketConnectionOpener` reçoit l'hôte, le port et le chemin de ses arguments. La dette « Adresse du hub en dur » garde ses deux occurrences et son pas séparé.
- **Messages série** : « Login OK… » et « Login refused… » sont écrits par `setup()` d'après le `StartupOutcome`, après l'ouverture du WebSocket et non avant. `client.begin()` n'écrit rien et ne se connecte pas (la connexion se fait dans `loop()`), donc l'ordre observable sur le port série est inchangé ; lu dans `WebSockets` (master), pas constaté sur le matériel.
- **Tests dans `test_startup`** (pas de nouveau répertoire), fakes locaux au fichier de test, une fixture qui câble séquence et boucle avec les mêmes lecteurs d'appui, comme `main.cpp`.

**Conséquences** —
- Deux états d'arrêt coexistent toujours (`StartupSequence::IsHalted()` au démarrage, `NetworkHalt` pour l'élimination et le `401` en reconnexion), lus ensemble par `BuzzerLoop`. À rediscuter si un troisième cas d'arrêt apparaît.
- Preuves : US-FW-005 CA-5 et CA-11, US-FW-001 CA-8 (tour de `loop()`), US-FW-006 CA-10 et CA-11 (arrêt de `loop()`) passent de « constat » à « testé en natif » pour la logique ; le câblage de `main.cpp` et `WebSocketConnectionOpener` restent sans test natif.
- Dette « Démarrage : preuves manquantes » réduite : il reste le constat sur la dalle du chemin du `401` au démarrage. Dette « Écran de login refusé en texte seul » : l'arrêt est porté par `StartupSequence`.
- Non exécutés ici : `pio test -e native` et `pio run -e esp32` ; `src/main.cpp` contrôlé en syntaxe contre les en-têtes réels de `WebSockets` et de LovyanGFX (`-DLGFX_LINUX_FB`), hors la classe `LGFX` (bus RGB propre à l'ESP32-S3, déjà non compilable sur Linux avant ce commit).

## #69 — feat(startup): show the init screen while the buzzer connects

**Contexte** — Constaté sur la dalle le 2026-11-24 (hub éteint au démarrage) : `setup()` bloque sur le Wi-Fi et le login, l'écran reste vide. US-FW-010 CA-1 et CA-2 (décision #53) spécifient un écran d'init **statique**, dessiné dans `setup()` avant l'attente réseau, avec le bandeau du haut (nom vide, liaison « coupée ») et trois lignes de même taille (« Buzz’ », « O’Live », « Connexion… », libellés de la décision #28). Aucun trou de CDC constaté : le bandeau (US-FW-009 CA-3, CA-4), les libellés, les zones masquées et le chemin `tappableZonesScreen` sont dits ; l'absence de tout autre rendu avant le premier message était déjà tenue par `ConnectionEventHandler` (rien avant le premier `auth_success`, US-FW-015 CA-6) et par `LoginOutcomeDisplay` (seul un `401` affiche).

**Décision** (périmètre et choix figés avec Olivier : logo en texte et « Connexion… » de la décision #28 ; écran dessiné dans `setup()` juste après `lcd.init()` et avant `WiFi.begin`) —
- **Type de commit `feat`** : capacité observable (l'écran d'init apparaît à la mise sous tension).
- **Hors périmètre, intacts** : clignotement du bandeau, habillage D1, batterie, CA-4 (sous-titre d'attente), `game_resumed`, bandeau du `401` (`LoginOutcomeDisplay` passe toujours `Hidden()`).

**Choix de Claude, à relire en PR** (aucun ne vient des CDC) —
- **`InitScreenDisplay`** (`lib/startup/`, `Show()`), sur le modèle de `LoginOutcomeDisplay` : il prend un `Screen&` (`main.cpp` lui passe `tappableZonesScreen`, comme à l'écran de refus) et appelle une fois `Display(bandeau, modèle)`. Écarté : une méthode de plus sur `LoginOutcomeDisplay` (deux raisons de changer pour une classe) ; l'appeler depuis `StartupSequence::Run()` (le Wi-Fi s'attend avant `Run()` : l'écran serait trop tard).
- **Modèle d'écran** : `ScreenPresenter::ScreenForInit()` rend un `TextScreen` d'un seul texte à retours à la ligne (`DisplayState::LcdTextForInit()` : `"Buzz’\nO’Live\nConnexion…"`), comme l'écran de refus ; `ScreenPresenter::TopBarForInit()` rend `TopBar::ForPlayer("").WithLinkCut()`. La taille unique des trois lignes est celle que `GfxScreen` donne à tout `TextScreen` (US-FW-010 CA-7), non constatée sur la dalle.
- **Pas de `RetainingScreen`** : l'init n'est pas un écran à atténuer sous une coupure (aucune coupure avant le premier `auth_success`) ; `tappableZonesScreen` seul, comme le brief et CA-2.
- **`lcd.fillScreen(TFT_BLACK)` conservé** avant l'écran d'init (effacement initial inchangé).
- **CA-2 sans nouvelle logique** : aucun test nouveau ne prétend le porter seul ; il est tenu par composition (tests existants cités dans US-FW-010). Un test composé vérifie en revanche que l'init, passé par `TappableZonesScreen`, désactive des zones actives.
- **Tests** : deux dans `test_screen_presenter` (texte, bandeau), deux dans `test_startup` (`InitScreenDisplay` : appel exact attendu avec des littéraux, pas via le présenteur ; zones masquées). Les tests de texte, de bandeau et d'appel exact sont nés rouges (API absente) ; le test des zones n'a connu qu'un rouge de compilation (classe absente), jamais un rouge de comportement : il a été muté (17 mutants, tous tués).

**Conséquences** —
- L'ordre « écran d'init avant `WiFi.begin` » vit dans `setup()` : aucun test natif ne le prouve (dette « Démarrage : preuves manquantes »). Le dessin des trois lignes sous le bandeau et l'icône « coupée » de l'init ne sont pas constatés sur la dalle.
- Dette « `setup()` bloquant » : l'absence de rendu d'init est levée pour le statique ; reste le clignotement. Dette « Écran de login refusé en texte seul » : `TopBarForInit()` existe maintenant, mais le refus passe encore `Hidden()` (US-FW-015 CA-9).
- Non exécutés ici : `pio test -e native` (suites natives compilées à la main, voir le handoff) et `pio run -e esp32` ; `src/main.cpp` contrôlé en syntaxe seulement, mêmes erreurs `LGFX` qu'avant.

## #70 — fix(rendering): keep the screen unchanged when an armed buzzer receives question_choices

**Contexte** — US-FW-002 CA-2 (décision #45, livré) fait ignorer par le mode tout message d'armement reçu hors Inerte. `LcdRenderer::Apply` rend le message avant son dispatch (décision #5) et lisait déjà l'état armé (`presenter.ScreenFor(message, armedModeReader.IsArmed())`), mais `ScreenPresenter::ScreenFor` n'utilisait ce paramètre que pour `timer_end` : un `question_choices` reçu par un buzzer déjà armé (MCQ non répondu, SPEED non buzzé) dessinait quatre zones tactiles mortes, les appuis A–D étant ignorés. C'est la dette « zones affichées mais mortes » de US-FW-002 et le CA-6 de US-FW-011 (« zones affichées = zones actives »). Aucun trou de CDC constaté : le CA-6 dit l'observable (écran inchangé, ni zones, ni en-tête, tactile intact, mode inchangé).

**Décision** (périmètre figé avec Olivier : US-FW-011 CA-6 seul) —
- **Type de commit `fix`** : correction d'un comportement incorrect (des zones affichées qui ne répondent pas).
- Un buzzer déjà armé qui reçoit `question_choices` laisse son écran inchangé : `ScreenFor` rend `ScreenUnchanged` ; `LcdRenderer` n'appelle pas `Screen::Display`, donc les zones tactiles ne sont pas touchées (US-FW-011 CA-13). Un buzzer Inerte rend les zones comme avant.
- **Hors périmètre, intacts** : `question_open` et `buzz_unlocked` reçus armés (US-FW-012 CA-13, même écart, encore ouvert), `timer_tick` (CA-8), l'appui pendant une coupure (US-021), les propositions inexploitables (CA-5), l'en-tête et le chrono.

**Choix de Claude, à relire en PR** (aucun ne vient du CDC) —
- **Où vit la règle** : dans `ScreenPresenter::ScreenFor`, à côté de `timer_end`, qui prend déjà l'état armé en paramètre ; `LcdRenderer` n'a pas changé. Écarté : un test dans `LcdRenderer` qui court-circuite avant le présenteur (deux endroits pour la même règle d'écran, et le présenteur est le seul à connaître quels messages arment).
- **Condition** : `questionArmed` seul, sans regarder si les propositions sont exploitables. Des propositions inexploitables rendent déjà `ZonesScreen` vide donc écran inchangé (CA-5) ; armé, c'est inchangé aussi, sans distinction à faire.
- **Tests** : trois niveaux, comme `timer_end`. `test_lcd_renderer` et `test_connection` sont ceux que la spec demande ; `test_screen_presenter` est ajouté (un test) parce que la règle y vit. Dans `test_connection`, deux tests de bout en bout avec le vrai `BuzzerBehavior` : MCQ armé par un premier `question_choices` puis un second non rendu ; SPEED armé, `question_choices` non rendu, un appui A n'envoie rien, `timer_end` rend « Trop tard » (le mode est resté SPEED). Les quatre tests sont nés rouges (écran dessiné), aucun né vert ; `QuestionChoicesShowsTheFourChoicesAsZonesOnScreen` fixe désormais explicitement `IsArmed()` à faux (ce qu'il supposait par défaut).
- **Dette « zones affichées mais mortes »** : réécrite dans le constat de US-FW-002 (soldée pour `question_choices`, reste pour `question_open` et `buzz_unlocked`) ; la clause CA-6 est retirée de « Écran de question : preuves manquantes et tests à réécrire ».

**Conséquences** —
- Un buzzer resté armé (hub qui n'envoie pas `timer_end` avant la question suivante) garde le titre de la question précédente à l'écran jusqu'à `timer_end` : c'est ce que dit le CA-6, pas une régression. Le joueur ne voit pas les nouvelles propositions.
- `ScreenFor` lit maintenant l'état armé pour deux messages ; US-FW-012 CA-13 en ajoutera deux (`question_open`, `buzz_unlocked`), US-FW-017 CA-4 et US-FW-020 CA-4 un chacun.
- Non constaté sur la dalle : le hub (`main`, `54a320e`) n'émet pas `question_choices`, pas de smoke test. `pio test -e native` non exécuté ici (suites compilées à la main, voir le handoff) ; la CI de la PR fait foi.


## #71 — fix(rendering): keep the screen unchanged when an armed buzzer receives question_open or buzz_unlocked

**Contexte** — US-FW-012 CA-13 : un buzzer déjà armé (SPEED non buzzé, ou MCQ non répondu) qui reçoit `question_open` ou `buzz_unlocked` laisse son écran inchangé, « seul un message qui arme est rendu ». Le mode les ignore déjà (US-FW-002 CA-2, décision #45), et `LcdRenderer::Apply` lit l'état armé avant l'aiguillage (décision #5) et le passe à `ScreenPresenter::ScreenFor` ; mais celui-ci ne l'utilisait que pour `timer_end` et, depuis la décision #70, `question_choices`. Un buzzer armé rendait donc encore le titre de `question_open` ou « À nouveau possible de buzzer » alors que son mode n'avait pas bougé : l'écran affichait un état que le buzzer n'avait pas. Même écart, même mécanisme que la décision #70. Aucun trou de CDC constaté : le CA dit l'observable (écran inchangé, message ignoré en entier, mode conservé).

**Décision** (périmètre figé avec Olivier : US-FW-012 CA-13 seul) —
- **Type de commit `fix`** : correction d'un comportement incorrect (un écran qui ne reflète pas l'état armé).
- Un buzzer déjà armé qui reçoit `question_open` ou `buzz_unlocked` laisse son écran inchangé : `ScreenFor` rend `ScreenUnchanged` ; `LcdRenderer` n'appelle pas `Screen::Display`. Un buzzer Inerte rend ces écrans comme avant (texte actuel ; l'écran d'ouverture de CA-1 et CA-11 attend leurs `feat`).
- **Hors périmètre, intacts** : CA-7 (l'état retenu : titre, numéro, « a buzzé » n'existe pas, il n'y a rien à « ne pas retenir »), `timer_tick`, le chrono, `intermediate_ranking`, `question_media`, tout autre CA.

**Choix de Claude, à relire en PR** (aucun ne vient du CDC) —
- **Où vit la règle** : dans `ScreenPresenter::ScreenFor`, comme `timer_end` et `question_choices`, avant `ZoneTextsFor` ; `LcdRenderer` et `BuzzerBehavior` n'ont pas changé. Écarté : une règle dans `LcdRenderer` (deux endroits pour la même règle d'écran).
- **Forme** : une condition explicite sur les deux types (`question_open`, `buzz_unlocked`) en tête de `ScreenFor`, avec le commentaire de la règle. Écarté : un prédicat « message qui arme » dans `DisplayState` (une abstraction pour trois messages ; à reconsidérer quand US-FW-017 CA-4 et US-FW-020 CA-4 s'ajouteront, voir « Conséquences »).
- **Condition** : `questionArmed` seul, sans regarder le titre ni `remaining_seconds` : armé, le message est ignoré en entier, même s'il serait inexploitable.
- **Tests** : trois niveaux, comme `timer_end` et `question_choices`, tous deux paramétrés sur les deux messages (`question_open` avec titre, `buzz_unlocked`). `test_screen_presenter` et `test_lcd_renderer` : un test armé (écran inchangé, `Display` jamais appelé) et un test Inerte (texte d'ouverture actuel). `test_connection` : deux tests de bout en bout avec le vrai `BuzzerBehavior`, SPEED armé puis MCQ armé ; rien n'est rendu, et le mode conservé se prouve par l'appui (un appui rouge envoie le buzz en SPEED ; un appui A envoie la réponse en MCQ, un appui rouge n'envoie rien). Les huit tests armés sont nés rouges (écran dessiné), les quatre tests Inerte sont nés verts et ont été mutés. Aucun test existant ne figeait l'ancien comportement : aucun test réécrit.
- **CA-7** : la trace du CA-13 le dit en toutes lettres (« hors de cette trace ») ; le `feat` de CA-7 devra laisser un message ignoré hors de son état, ce que « Un message ignoré (CA-13) ne le touche pas » (CA-7) exige déjà.

**Conséquences** —
- La dette « zones affichées mais mortes si le mode n'est pas Inerte » (constat de US-FW-002) est soldée pour les trois messages d'armement ; le constat est réécrit dans la spec de US-FW-002.
- `ScreenFor` lit maintenant l'état armé pour quatre messages ; US-FW-017 CA-4 (`intermediate_ranking`) et US-FW-020 CA-4 (`question_media`) en ajouteront deux. Le moment de factoriser en un prédicat est là.
- Même conséquence que la décision #70 : un buzzer resté armé (hub qui n'envoie pas `timer_end` avant la question suivante) ne voit ni la nouvelle ouverture ni la réouverture. À confirmer à l'intégration US-011 et US-012.
- Non constaté sur la dalle : le hub n'émet ni `question_open` ni `buzz_unlocked` (`DEBTS.md`). `pio test -e native` non exécuté ici (suites compilées à la main, voir le handoff) ; la CI de la PR fait foi.


## #72 — fix(rendering): keep the screen unchanged when a buzzer receives timer_tick

**Contexte** — US-FW-011 CA-8 et US-FW-012 CA-4 : un `timer_tick` **ne remplace jamais l'écran**, en MCQ comme en SPEED, armé ou non ; le mode ne change pas (US-FW-004 CA-5) et rien n'est émis. `DisplayState::LcdTextFor` rendait pourtant « N s » en texte plein écran : sur un MCQ armé, `Show` effaçait les zones tactiles alors que le buzzer restait armé (constat du smoke test du 2026-11-09 : aucun `answer` après le tick) ; sur un SPEED, le premier tick remplaçait le titre de `question_open`. Le chrono d'en-tête que ces CA recalent n'existe pas encore (US-FW-011 CA-3 et CA-7, US-FW-012 CA-2 et CA-3) : le recalage est dormant, un `timer_tick` laisse donc toujours l'écran inchangé, y compris avec `remaining_seconds` absent ou non entier. Aucun trou ni contradiction de CDC constaté : les deux CA disent l'observable (écran inchangé, zones et tactile intacts, mode conservé, rien d'émis).

**Décision** (périmètre figé avec Olivier : US-FW-011 CA-8 et US-FW-012 CA-4, hors recalage du chrono) —
- **Type de commit `fix`** : correction d'un comportement incorrect (des zones tactiles effacées sous un buzzer armé, un titre remplacé par « N s »).
- Un `timer_tick` ne produit plus de texte : `DisplayState::LcdTextFor` ne le traite plus, il rend la chaîne vide, `ScreenPresenter::ScreenFor` rend `ScreenUnchanged` et `LcdRenderer` n'appelle pas `Screen::Display`. Aucune lecture de l'état armé : la règle vaut armé ou non.
- **Hors périmètre, intacts** : le chrono d'en-tête et son recalage (CA-7 de US-FW-011, CA-3 de US-FW-012), `question_open` et `buzz_unlocked` armés (décision #71), `question_choices` armé (décision #70), tout autre CA.

**Choix de Claude, à relire en PR** (aucun ne vient du CDC) —
- **Pas de condition en tête de `ScreenFor`** : la décision #70 et la décision #71 ajoutent une condition sur l'état armé, parce que ces messages ont un texte qu'il faut supprimer à certains moments. Ici le message n'a plus de texte du tout : retirer la branche de `LcdTextFor` suffit, et le chemin existant « texte vide : écran inchangé » de `ScreenFor` fait le reste. Une condition `timer_tick` en tête de `ScreenFor` a été écrite d'abord, puis retirée : la mutation « condition supprimée » ne cassait aucun test, c'était un mutant équivalent, donc du code mort. Écarté : la garder « pour l'intention » (le nom des tests et ce paragraphe la portent).
- **Suppression du texte « N s »** : `LcdTextForRemainingSeconds` est retirée avec la branche (code mort sinon). Le chrono à venir écrira son propre libellé avec l'horloge (US-FW-011 CA-7) ; `HubMessage::RemainingSeconds()` reste, le désérialiseur le teste et le recalage le lira.
- **Tests** : trois niveaux, comme #70 et #71. `test_screen_presenter` et `test_lcd_renderer` : un test paramétré armé ou non (présentateur : `remaining_seconds` à 25, à 0 ou absent ; `LcdRenderer` : 25, 5 et 0 s, `StrictMock<MockScreen>` donc `Display` jamais appelé). `test_connection` : trois tests de bout en bout avec le vrai `BuzzerBehavior` (MCQ armé par un vrai `question_choices` : zones dessinées une seule fois, puis le tick, puis un appui A envoie encore `answer` et un appui rouge rien ; SPEED armé par un vrai `question_open` : titre dessiné une seule fois, puis le tick, puis l'appui rouge envoie encore `buzz` et un appui A rien ; Inerte : rien n'est rendu, rien n'arme) et un test paramétré sur quatre formes de `remaining_seconds` inutilisable (absent, texte, décimal, `null`). `test_rendering` : `DisplayStateTest.TimerTickShouldDisplayTheRemainingSecondsOnScreen` est réécrit en `TimerTickWithRemainingSecondsShouldHaveNoTextToDisplay`.
- **Tests réécrits** : `TimerTickTest.ShowsTheRemainingSecondsOnScreen` (`test_lcd_renderer`, « 25 s », « 5 s », « 0 s » en plein écran) et `DisplayStateTest.TimerTickShouldDisplayTheRemainingSecondsOnScreen` (`test_rendering`), les deux seuls qui figeaient l'ancien rendu ; `test_dispatch`, `test_deserialization` et `test_connection` n'en dépendaient pas. `test_tappable_zones` garde un `TextScreen{"12 s"}`, simple texte quelconque, sans lien avec `timer_tick`.
- **Rouge avant le correctif** : 14 tests échouaient (`Display` appelé avec « 25 s », ou texte non vide) : 4 au présentateur (25 et 0 s, armé ou non), 6 à `LcdRenderer`, 3 de bout en bout (MCQ armé, SPEED armé, Inerte), 1 à `DisplayState`. Nés verts, donc mutés : le cas `remaining_seconds` absent au présentateur (2) et les quatre formes inutilisables de `test_connection` (la branche de texte n'existait que pour `HasRemainingSeconds()`).
- **Mutations** : 9 mutants, 9 tués (voir le handoff) : texte rétabli (M1), texte seulement armé (M2), seulement Inerte (M3), valeur inventée sans `remaining_seconds` (M4), cas 0 s (M5), un tick qui désarme au répartiteur (M6), un `LcdRenderer` qui appelle `Display` pour `ScreenUnchanged` (M7), texte rétabli avec un désérialiseur trop permissif (M8, texte et décimal) ou qui lit `null` comme 0 (M9).
- **Dettes** : « Titre de `question_open` remplacé par le premier `timer_tick` » est supprimée (soldée, US-FW-012 CA-4) ; le cas `timer_tick` de « Buzzer armé sans zone tactile à l'écran » est retiré (reste les propositions inexploitables) ; « Table de rendu §8.2 incomplète », « Chrono : ni horloge ni rafraîchissement hors message », « Écran de question » et « Écran de rapidité » sont ajustées (CA-8 et CA-4 tenus pour l'écran, le recalage du chrono reste à écrire avec CA-7 et CA-3).

**Conséquences** —
- Un `timer_tick` ne change plus jamais l'écran ni les zones : le joueur d'un MCQ armé garde ses zones actives, le joueur d'un SPEED garde le titre. Il ne voit aucun compte à rebours tant que le chrono n'existe pas (aucun écran ne le montrait avant le tick non plus).
- Le `feat` de CA-7 (US-FW-011) et de CA-3 (US-FW-012) portera le chrono et son recalage par `remaining_seconds` ; ce chemin aura besoin d'un état et d'une horloge, que `DisplayState` n'a pas (dette « Chrono : ni horloge ni rafraîchissement hors message »).
- Non constaté sur la dalle : le hub n'émet pas `timer_tick` (`DEBTS.md`), pas de smoke test dans cette session. `pio test -e native` et `pio run -e esp32` non exécutés ici (suites compilées à la main, voir le handoff) ; la CI de la PR fait foi.


## #73 — feat(rendering): keep the top bar visible on every screen

**Contexte** — US-FW-009 CA-3 et CA-7 (décision #52) : le bandeau du haut reste visible, même nom, sur chaque écran qui change (question, zones, évidence, bandeau de texte, zone choisie, feedback local d'un appui MCQ ou SPEED), dès l'init, et `GfxScreen` le dessine au-dessus de chaque modèle d'écran, le contenu occupant la dalle moins 60 px. Le code faisait le contraire : `ScreenPresenter::TopBarFor` rendait `Hidden()` pour tout autre message que `auth_success`, `ScreenLocalFeedback` passait toujours `Hidden()`, `GfxScreen` ne dessinait le bandeau qu'avec un `TextScreen`, « Hors jeu » et la coupure, `LcdRenderer` gardait le dernier nom dans `lastVisibleTopBar` (état à lui seul), et `TappableZonesScreen` découpait les zones tactiles sur toute la dalle. Aucun trou de CDC constaté sur un comportement observable : le nom (CA-1, repli), la liaison (CA-4), la hauteur (60 px, CA-6), la géométrie des zones (380 × 190 px, constat de US-FW-009) sont dits.

**Décision** (périmètre figé avec Olivier : CA-3 et CA-7 ; hors CA-2, CA-5, CA-6, CA-8, l'en-tête de question et le chrono, tout ce qui demande une horloge ; le bandeau visible est un état partagé entre `LcdRenderer` et `ScreenLocalFeedback`, dette « Deux instances de `DisplayState` ») —
- **Type de commit `feat`** : le comportement observable change (le bandeau et le nom restent à l'écran quand l'écran change, y compris sur les zones, l'évidence, « Trop tard », le feedback local).
- **Un bandeau partagé** : `VisibleTopBar` (`lib/rendering/VisibleTopBar.h`) retient le bandeau visible (`Current()`, `Set()`). `main.cpp` en construit un et le passe à `LcdRenderer` (qui l'écrit) et à `ScreenLocalFeedback` (qui le lit). Avant tout `auth_success` : le bandeau de l'init, nom vide, liaison « coupée ».
- **Le présentateur reste sans état** : `ScreenPresenter::TopBarFor(message, visibleTopBar)` rend `ForPlayer(username)` pour un `auth_success` (CA-1, liaison « liée ») et le bandeau visible tel quel pour tout autre message (même nom, même liaison). `LcdRenderer::Apply` l'enregistre avant de rendre ; un message qui laisse l'écran inchangé n'appelle toujours pas `Screen::Display`.
- **La perte de liaison est retenue** : `ShowLinkLost` met le bandeau partagé en liaison coupée (CA-4 : « coupée dès la perte, liée à l'`auth_success` suivant »), donc un écran qui change pendant la coupure la garde barrée. `lastVisibleTopBar` est supprimé.
- **Les zones tactiles suivent le dessin** : `TopBar::Height` (60 px, trait compris) est partagé entre `GfxScreen` (qui l'avait en constante privée) et `TappableZonesScreen`, qui construit son `ZoneGrid` à l'activation : `ZoneGrid(largeur, 60, hauteur − 60)` sous un bandeau visible, `ZoneGrid(largeur, 0, hauteur)` sinon. Aucun point du bandeau n'est tactile.
- **`GfxScreen::Display` dessine le bandeau sur tous les modèles** : un assistant `ClearAndDrawTopBar` (fond noir, bandeau s'il est visible, rend le haut de la place du contenu) sert les zones, la zone choisie, la zone en évidence et le bandeau de texte ; les zones passent à 400 × 210 px (380 × 190 utiles).
- **Hors périmètre, intacts** : le refus du login (`LoginOutcomeDisplay` passe encore `Hidden()`, US-FW-015 CA-9), la troncature du nom (CA-5), le nom du participant (CA-2), l'en-tête et le chrono, le débordement connu de 40 `¼` (10 px, constat reporté).

**Choix de Claude, à relire en PR** (le découpage n'est pas observable ; aucun ne vient du CDC) —
- **Un petit objet partagé plutôt qu'un état dans `DisplayState` ou `RetainingScreen`** : `VisibleTopBar` est un porteur de valeur de dix lignes, injecté par référence aux deux producteurs, qui rend le partage visible dans `main.cpp`. Écarté : porter le bandeau dans `DisplayState` (il faudrait d'abord partager l'instance, tout l'objet de la dette, pour un seul champ) ; le porter dans `RetainingScreen` (il retient déjà « le dernier écran », mais les producteurs doivent *produire* le bandeau avant `Display` : le présentateur le choisit et l'enregistre, le décorateur le verrait trop tard pour le feedback local) ; un constructeur qui crée l'objet par défaut (le partage serait implicite, deux instances se perdraient sans erreur de compilation).
- **L'état initial est celui de l'init (liaison coupée), pas « liée »** : l'ancien `lastVisibleTopBar` valait `ForPlayer("")` (liée). CA-4 dit « coupée dès la mise sous tension, liée à l'`auth_success` » : le bandeau d'avant tout `auth_success` est donc `TopBarForInit()`, relié par un test (`TheTopBarBeforeAnyAuthSuccessIsTheInitTopBar`). Non observable en jeu : le hub envoie `auth_success` en premier ; seuls les tests de `LcdRenderer` qui rendent un message sans `auth_success` l'attendent.
- **`ShowOutOfGame` ne retient pas la coupure** : « Hors jeu » est terminal (US-FW-015 CA-8), aucun écran n'est dessiné après ; il affiche le bandeau partagé en liaison coupée sans l'enregistrer (une ligne de moins, un mutant non tuable en moins). `ShowLinkLost`, lui, l'enregistre, parce qu'un appui ou un message peut suivre.
- **Bande de résultat inchangée à 120 px** : `HighlightZone` garde un quart de la hauteur de la **dalle** pour le texte, décalé sous le bandeau (y 60 à 180) ; les zones occupent le reste (800 × 300 px, 150 par rangée, 130 utiles : le chiffre du constat de US-FW-009 et de US-FW-013, « 380 × 130 »). Écarté : un quart de la place du contenu (105 px), qui changerait la bande sans que le CDC le dise. L'en-tête de 60 px de US-FW-013 CA-5 remplacera cette bande.
- **`TappableZonesScreen` garde `panelWidth` et `panelHeight`** au lieu d'un `ZoneGrid` membre : la grille dépend du bandeau de chaque affichage, recréée à l'activation (aucune allocation).
- **Tests** (cinq suites) : `test_screen_presenter` (`MessageKeepingTheTopBarTest`, ex-`MessageWithoutTopBarTest`, avec un bandeau coupé pour prouver la liaison gardée), `test_lcd_renderer` (`ChangingScreenKeepsTheTopBarTest` : onze écrans de quatre modèles après un `auth_success` nommé ; nom remplacé, nom vide, liaison coupée gardée, coupure partagée, message inchangé sans effet), `test_local_feedback` (nom et liaison lus, état initial, bandeau changé après construction), `test_connection` (`SharedTopBarRenderingTest` : le vrai câblage, `LcdRenderer` et `ScreenLocalFeedback` sur un même `VisibleTopBar`, feedback MCQ et SPEED et séquence d'écrans), `test_tappable_zones` (six tests : bandeau non tactile, zones dès la ligne 60, deux rangées de 210 px, deux colonnes de 400 px, liaison coupée, retour au plein écran). Les attentes `Display(TopBar::Hidden(), …)` de `test_lcd_renderer`, `test_local_feedback` et `test_connection` sont réécrites (bandeau de l'init avant tout `auth_success`, ou nom de l'`auth_success`) ; les deux constructeurs prennent un `VisibleTopBar` (quarante-quatre sites de test mis à jour mécaniquement).
- **Mutations** : 18 mutants, 18 tués (voir le handoff), dont les tests nés verts (zones dès la ligne 60, deux colonnes, retour au plein écran, message inchangé).

**Conséquences** —
- Le nom et la liaison restent à l'écran sur tout écran qui change ; le feedback local d'un appui les porte aussi. Les zones des propositions sont à 380 × 190 px utiles : 40 `¼` débordent de 10 px (constat reporté de US-FW-008 CA-9, inchangé). Sous l'en-tête de US-FW-011 CA-4, ce sera 380 × 160 px.
- `GfxScreen` n'est ni testé en natif ni compilé pour l'`esp32` ici ; contrôle de syntaxe contre les en-têtes de LovyanGFX (backend framebuffer Linux) seulement. Le dessin sous le bandeau (zones, zone choisie, évidence, bandeau de texte) est à constater sur la dalle : CA-7 « implémenté, non constaté ». `src/main.cpp` n'est lu que : deux lignes de câblage.
- Dette « Deux instances de `DisplayState` » : le bandeau n'en fait plus partie (soldé par `VisibleTopBar`) ; reste l'état de la question, de « a buzzé », des propositions retenues et de la minuterie. Dette « Bandeau du haut sur le seul écran « Connecté » » : renommée « Bandeau du haut : nom du participant, troncature, habillage et écran de refus » (reste CA-2, CA-5, le bandeau du refus, l'habillage). Une dette ajoutée : « `TouchZoneReader` : bords droit et bas inclus » (un pixel de recouvrement entre zones voisines, relevé en écrivant les essais aux limites).
- Les écrans à venir (D5, photo, son, chrono, classement) auront le bandeau par le même chemin : `TopBarFor` garde le bandeau visible quel que soit le type de message.
- Non constaté sur la dalle ; pas de smoke test dans cette session. `pio test -e native` et `pio run -e esp32` non exécutés ici (suites compilées à la main, voir le handoff) ; la CI de la PR fait foi.

## #74 — feat(rendering): draw dashes and curly quotation marks in texts

**Contexte** — US-FW-008 CA-12 (décision #51) : `–` (U+2013), `—` (U+2014), `“` (U+201C) et `”` (U+201D) n'avaient pas de glyphe. LovyanGFX 1.2.36 les dessinait comme un blanc de 7 px, alors que la mesure leur comptait la largeur d'un `?` (14 px à 30 px). Un titre ou une proposition qui les contient (« Paris — la capitale », « “Germinal” ») s'affichait avec un trou.

**Décision** (périmètre figé avec Olivier : CA-12 seul ; hors CA-10, le remplacement par `?` des codes sans glyphe, qui en dépend et vient à une session suivante) —
- **Type de commit `feat`** : le comportement observable change (un blanc de 7 px devient le signe, avec son avance).
- Les quatre codes rejoignent `EXTRA_CODES` de `scripts/generate-gfx-font.py`, dans l'ordre des codes (`Œ`, `œ`, `–`, `—`, `’`, `“`, `”`, `…`, `€`). Les trois polices (30, 38 et 48 px) sont régénérées avec le fichier épinglé de `src/fonts/README.md` (`Oswald-Regular.ttf`, SHA-256 `0d9980da…78ab826`, FreeType 2.13.2) : 233 glyphes (229 avant), 7 plages d'encodage (5 avant : U+2013-U+2014 et U+201C-U+201D se regroupent chacune en une plage). Bitmaps : 6 244, 9 647 et 15 530 octets (6 215, 9 600 et 15 448 avant). Les bitmaps et les métriques des 229 codes précédents sont inchangés ; la boîte d'encre de chaque police (33/7, 41/9 et 51/11 px au-dessus et au-dessous de la ligne de base) l'est aussi, donc le centrage vertical de LovyanGFX ne bouge pas.
- Avances (px, 30 / 38 / 48) : `–` 17 / 21 / 27, `—` 31 / 39 / 49, `“` 11 / 13 / 17, `”` 10 / 12 / 16, recoupées entre la table `hmtx` de la police (1000 unités par em) et le rendu FreeType monochrome.
- `scripts/extract-font-advances.py` régénère la copie de test de la plus petite police : quatre avances de plus, rien d'autre ne change. Seul le commentaire de l'en-tête généré est réécrit dans les deux scripts pour nommer les neuf codes.
- Un test : `TextLayoutRealFontTest.DashesAndCurlyQuotationMarksShouldBeMeasuredWithTheirRealWidth` (17, 31, 11 et 10 px à 30 px). Rouge d'abord pour la bonne raison (les quatre signes mesuraient 14 px, l'avance du `?`), vert après régénération.
- `src/fonts/README.md` : nombre de glyphes, plages et octets recalculés d'après le réel ; la ligne « hors Latin-1 » (qui disait que `Œ œ ’ … €` n'avaient pas de glyphe, périmée depuis la décision #11) dit désormais que tout code hors `EXTRA_CODES` est dessiné comme un blanc.

**Choix de Claude, à relire en PR** (aucun ne vient du CDC) —
- **`EXTRA_CODES` écrit une ligne par code, avec le signe en commentaire**, comme le dit CA-12 (« une ligne par code »), au lieu de la liste sur une seule ligne : la liste passe à neuf codes, dont quatre qu'on ne distingue pas à l'œil (`–` et `—`). Écarté : la garder sur une ligne.
- **Un seul test, quatre assertions**, sur le modèle du test des cinq autres glyphes hors Latin-1. Les valeurs attendues viennent de `hmtx` et de FreeType, pas du générateur : le test ne se confirme pas lui-même. Écarté : un test par signe (même ligne de code testée, quatre noms de plus).
- **Avance testée à 30 px seulement** : la fixture ne copie que la plus petite police (dette « Largeurs de test copiées de la police »). Les avances de 38 et 48 px sont recoupées à la main (ci-dessus) et par un décodage des en-têtes contre FreeType, sans test versionné. Écarté : étendre la fixture aux trois polices (hors CA-12, qui dit « à 30 px »).
- **Régénération déterministe, vérifiée** : deux passes de génération donnent des en-têtes identiques au bit près ; avant la modification, la régénération reproduisait déjà les trois en-têtes versionnés (le fichier épinglé et FreeType 2.13.2 donnent les mêmes pixels).
- **Mutation** : aucun test né vert (le seul est né rouge), mais la chaîne générateur, fixture et test est mutée : `EXTRA_CODES` privé de chacun des quatre codes (4 tués), `“` remplacé par `‘` (tué), indice de base des glyphes décalé (refusé par `extract-font-advances.py`, qui vérifie que les plages couvrent les glyphes : tué par l'outil, pas par le test), arrondi de l'avance vers le bas (**équivalent** : FreeType rend en mode monochrome des avances déjà entières, `advance.x % 64 = 0` pour les 233 codes des trois tailles). Fichiers restaurés, sha256 vérifié.
- **Pas de test natif du dessin** : `GfxScreen` n'est pas touché ; le dessin des quatre signes (position, épaisseur du tiret, crénelage de `“` et `”` à 30 px, 8 px de haut) se juge au smoke test, qui n'est pas dans cette session.

**Conséquences** —
- Dette « Codes sans glyphe : un blanc invisible à l'écran » : la part des quatre signes est soldée (réécrite sans eux) ; reste CA-10 (émojis, marques combinantes, espace fine insécable, codes < 0x20). `– — “ ”` ne deviendront pas des `?` quand CA-10 sera livré : c'est la raison de l'ordre des sessions.
- Dette « Largeurs de test copiées de la police » : la phrase qui annonçait quatre avances de plus est retirée (fait) ; le reste de la dette est inchangé. Dette « Polices en 1 bit » : complétée (le dessin des quatre signes n'a pas de smoke test).
- Statut de US-FW-008 : toujours « Partielle » (CA-10 non implémenté ; CA-9, CA-11 et CA-13 sans test) ; CA-12 : implémenté, avance testée, dessin non constaté. L'index des specs ne change pas.
- LovyanGFX non épinglée (dette « Dépendances flottantes ») : la lecture d'`EncodeRange` (`GFXfont::getGlyph`, `getDefaultMetric`) est celle du tag 1.2.36 de la décision #11 ; les en-têtes sont contrôlés en syntaxe et en `static_assert` (233 glyphes, 7 plages) contre LovyanGFX à `dd228d1` (backend framebuffer Linux, pas l'`esp32`). `pio test -e native` et `pio run -e esp32` non exécutés ici (suites compilées à la main, voir le handoff) ; la CI de la PR fait foi.

## #75 — fix(rendering): replace characters without a glyph with a question mark

**Contexte** — US-FW-008 CA-10 (décision #51) : un code que la police n'a pas (émoji, marque combinante d'un accent décomposé, espace fine insécable U+202F, tout caractère hors Latin-1 autre que `Œ œ – — ’ “ ” … €`) était mesuré comme un `?` (14 px à 30 px) mais dessiné par LovyanGFX 1.2.36 comme un blanc de 7 px, et un code < 0x20 n'était pas dessiné du tout. « Quel drapeau : 🇧🇷 ? » s'affichait avec un trou, et la coupe des lignes surestimait la largeur. CA-12 (décision #74) a livré `– — “ ”` : ils ne deviennent pas des `?`.

**Décision** (périmètre figé avec Olivier : CA-10 seul ; hors périmètre, CDC muet : octets UTF-8 invalides, U+007F, U+0080 à U+009F, U+00AD et normalisation NFC) —
- **Type de commit `fix`** : le comportement observable change (un blanc invisible devient un `?` visible).
- **Un filtre pur dans `lib/rendering/GlyphFilter.h`** (`GlyphFilter::ReplaceCodesWithoutGlyph`), appelé à l'entrée de `TextLayout::Compute`. Tout code est remplacé par **un** `?` (un drapeau, deux codes, donne `??` ; `e` + U+0301 donne « e? ») sauf : U+0020 à U+00FF (la plage Latin-1 du CDC, bornes comprises), les neuf codes de `EXTRA_CODES` de `scripts/generate-gfx-font.py` (`Œ œ – — ’ “ ” … €`) et `\n` ; les autres codes < 0x20 (`\r`, tabulation) sont retirés.
- **Pourquoi à l'entrée de `Compute`** : `GfxScreen::DrawTextIn` dessine les lignes que `Compute` rend et rien d'autre, donc la mesure (choix de la taille, coupe) et le dessin voient le même texte, le même `?` de 14 px à 30 px. Aucun code de `src/` ne change. `Wrap` (public, utilisé par les tests de coupe) n'est pas filtré : seul `Compute` est le point d'entrée du dessin.
- **La liste des codes dessinés vit dans `GlyphFilter`**, copie à la main des neuf codes de `EXTRA_CODES` (un `switch` commenté code par code) : rien ne détecte une police qui gagnerait un code sans le filtre (dette « Largeurs de test copiées de la police », même famille).
- **Tests** : `test_glyph_filter` (nouvelle suite, 12 tests : émoji, drapeau, U+0301, U+202F, codes de contrôle, Latin-1 gardé, première borne, voisins des neuf signes, plage Latin-1 sans glyphe utile gardée) et trois tests dans `test_text_layout` (`?` de 14 px avec les largeurs réelles d'Oswald 30 px, `\r` et tabulation sans ligne, un drapeau qui tient pile en « ?? »). Quatre tests du filtre (émoji, signes gardés, les deux des codes de contrôle) et les trois du layout sont nés rouges ; les huit autres du filtre sont nés verts (le premier code avait remplacé tout ce qui dépasse 0xFF) et ont été mutés : 33 mutants, tous tués (voir le handoff).

**Choix de Claude, à relire en PR** (aucun ne vient du CDC) —
- **Filtre dans `lib/` et appel dans `Compute`, pas dans `ScreenPresenter` ni dans `GfxScreen`** : un seul point pour tous les textes mis en page, testé en natif ; dans `ScreenPresenter` il aurait fallu le répéter pour chaque écran, dans `GfxScreen::DrawTextIn` il n'aurait pas été testable en natif.
- **Les codes < 0x20 sont retirés, pas remplacés** (CA-10 le dit) ; `\r` d'un `\r\n` disparaît donc sans laisser de ligne vide.
- **Un code = un `?`, jamais d'agrégat** : un émoji composé donne un `?` par code (CA-10 : « un drapeau, deux codes, donne deux `?` »).
- **Octets invalides** : le filtre s'appuie sur `Utf8::ContinuationCount` et `Utf8::CodePoints` sans les valider ; un octet isolé est gardé, une séquence tronquée ou fausse est décodée telle quelle. Aucune assertion sur ce comportement (non décidé), mais un contrôle jetable hors dépôt (2 millions de chaînes aléatoires de 0 à 7 octets, ASan et UBSan) ne trouve aucun plantage. Écarté : remplacer chaque octet invalide par `?` (décision du CDC, pas de la session).
- **Plage Latin-1 gardée telle quelle** : U+007F, U+0080 à U+009F et U+00AD ne sont ni remplacés ni retirés (le CDC dit que la plage est dessinée) ; le générateur de polices leur donne le glyphe de `?`, ce qui s'accorde avec CA-10 pour les contrôles. Un test les fige (`CodesInsideLatin1WithoutAUsefulGlyphShouldBeKeptAsTheyAre`) : à changer avec la règle, si le CDC en fixe une autre.
- **Le nom du bandeau du haut n'est pas filtré** : `GfxScreen::DrawTopBar` le dessine par `drawString` sans `TextLayout` (en une ligne, sans mise en page). Il est dans le périmètre de la US (« nom du joueur ») mais hors de l'emplacement choisi ; sans effet aujourd'hui (comptes `quiz_buzzer_NN`). Consigné en dette, avec la troncature du bandeau (US-FW-009 CA-5). Écarté : filtrer aussi dans `DrawTopBar` (code d'adaptateur non testable en natif, hors du périmètre décidé).

**Conséquences** —
- Dette « Codes sans glyphe : un blanc invisible à l'écran » : soldée (supprimée). Remplacée par « Remplacement des codes sans glyphe : trous du CDC et texte hors filtre » : octets invalides, U+007F à U+009F et U+00AD, NFC, nom du bandeau du haut, dessin du `?` non constaté.
- Statut de US-FW-008 : toujours « Partielle » (CA-9, CA-11 et CA-13 sans test natif) ; CA-10 : implémenté, testé en natif, dessin non constaté sur la dalle (smoke test à jouer). L'index des specs ne change pas.
- `src/fonts/README.md` et US-FW-019 (« Nom de quiz hors police ») : phrases mises à jour (un `?` pour tout texte mis en page par `TextLayout`).
- LovyanGFX non épinglée (dette « Dépendances flottantes ») : sans effet ici, aucun code de `src/` n'est touché. `pio test -e native` et `pio run -e esp32` non exécutés (suites compilées à la main, voir le handoff) ; la CI de la PR fait foi.

## #76 — test(rendering): pin the room of 40-character choices and the line spacing of the three fonts

**Contexte** — US-FW-008 CA-8 (le descendant d'une ligne laisse de la place à l'accent de la suivante) n'était figé qu'à 30 px, et CA-9 (une proposition ou une réponse de 40 caractères, préfixée « A : », dans une zone) ne l'était pas du tout : des sondes jetables de la session du 2026-11-22. La fixture `test/fixtures/OswaldRegular30Advances.h` ne copiait que la plus petite police (dette « Largeurs de test copiées de la police »).

**Décision** (type `test` : aucun code de `lib/` ni de `src/` modifié ; comportement observable inchangé) —
- **Fixtures** : `scripts/extract-font-advances.py` régénère `OswaldRegular38Advances.h` et `OswaldRegular48Advances.h` depuis `src/fonts/OswaldRegular38.h` et `OswaldRegular48.h` (même format et mêmes noms de tableaux, préfixés par la police) ; la fixture de 30 px, régénérée au passage, est identique au bit près. Un en-tête par police, pas un seul en-tête à trois tableaux. `src/fonts/README.md` régénère les trois.
- **Tests de CA-8** (`test_text_layout`, 2) : à 38 px, pas de 50 px et dégagement de 2 px ; à 48 px, pas de 64 px et dégagement de 4 px (`LineHeightShouldLeaveRoomBetweenADescenderAndTheAccentOfTheNextLineAt38Pixels` et `…At48Pixels`). Mesure : 1, 2 et 4 px à 30, 38 et 48 px, ce que le spec disait d'après `DEBTS.md` ; figés en égalité (le spec dit « au moins 1 px » à 30 px, le test de 30 px reste en `EXPECT_GE`).
- **Tests de CA-9** (`test_text_layout`, 8) avec les trois polices réelles (tailles 0, 1 et 2 = 48, 38 et 30 px, comme `GfxScreen`) : 40 `W`, 40 `¼` et 40 `—` préfixés « A : » dans 380 × 220, 380 × 190 et 380 × 160. Résultats mesurés, **tous conformes au spec** : 40 `W` : 4 lignes (« A : » seul, puis 17, 17 et 6 `W`), 160 px, dans les trois zones, pile dans 380 × 160 ; 40 `¼` : 5 lignes, 200 px, dans 380 × 220 ; 40 `¼` dépassent de 10 px 380 × 190 et de 40 px 380 × 160. Les trois tests des débordements sont dans une suite à part, `TextLayoutPinnedFindingTest` : ils **figent un constat reporté** (rien n'est corrigé).

**Choix de Claude, à relire en PR** (aucun ne vient du CDC) —
- **Hauteurs données à `TextLayout`, pas aux zones** : les « 380 × 220 », « 380 × 190 » et « 380 × 160 » du spec sont les zones de `ZoneGrid` (400 × 240, 400 × 210 et 400 × 180) **moins la marge de 10 px** que `GfxScreen::DrawTextIn` retire de chaque côté avant d'appeler `TextLayout`. Lu dans `ZoneGrid.h` et `GfxScreen.h`, d'où les chiffres du test ; la soustraction elle-même est dans l'adaptateur, donc hors de portée d'un test natif (dette « Texte : preuves manquantes »).
- **« A : » + 40 caractères, soit 44 caractères** : la borne du hub porte sur la proposition, le préfixe vient en plus (c'est ce que le spec mesure). Le préfixe reste seul sur sa ligne dans les trois cas (le mot de 40 glyphes est coupé à la lettre) ; il est donc indifférent à son propre avance : un mutant sur l'avance de `A` ou de `:` est équivalent.
- **Assertions exactes** (lignes, largeur de la plus large ligne, hauteur) et non « ça tient » : « ça tient » laisse passer une avance de `W` à 23 px au lieu de 22. Écarté : ne tester que la hauteur.
- **Un cas de plus que le brief, 40 `—`** : la mesure des fixtures montre que l'em dash (ajouté par la décision #74) est le glyphe le plus large des trois polices (31, 39 et 49 px), plus large que `¼` (28, 36 et 45 px) que le spec donnait pour le plus large. Le résultat ne change pas (12 tirets par ligne, 5 lignes, 200 px) ; le spec et la dette le consignent. Écarté : ne pas en parler, le « pire cas » du spec étant alors faux.
- **Helper de test** : `RealFont` (gabarit sur les tableaux de la fixture) remplace le corps de `SmallestRealFont`, qui le réutilise ; `ThreeRealFonts` ajoute les trois polices dans l'ordre de `GfxScreen`. Les 65 tests existants du fichier restent verts sans changement d'attente. Écarté : copier le corps trois fois.
- **Avance de `– — “ ”` à 38 et 48 px non assertée** : hors CA-8 et CA-9 (CA-12 dit « à 30 px »). La fixture existe maintenant, un test serait trivial : session suivante possible.
- **Mutation** : les 10 tests sont nés verts (ils figent un comportement existant) ; 35 mutants, 32 tués et 3 équivalents expliqués dans le handoff (fixtures, `QuizLineSpacingPercent`, `TextLayout`, `ProportionalTextMeasurer`, `FontMetrics`, `GlyphFilter`, helper de test) ; fichiers restaurés, sha256 vérifié. Les mutants de `Compute` (`<=` en `<`, plus grande taille sautée) et de `Fits` (`<=` en `<`) ne sont tués que par des tests existants, pas par les nouveaux : la plus petite taille n'est jamais jugée par `Compute`, et aucune largeur des cas de CA-9 ne tombe pile sur 380 px.

**Conséquences** —
- Dette « Texte : preuves manquantes » : CA-8 et CA-9 retirés ; restent `GfxScreen::DrawTextIn` et les polices (smoke test), la marge non reliée à `ZoneGrid`, et les débordements de CA-9 comme constats reportés. Dette « Largeurs de test copiées de la police » : trois copies au lieu d'une.
- Statut de US-FW-008 : toujours « Partielle » (CA-11 et CA-13 sans test natif) ; CA-1 à CA-10 tenus en natif, CA-12 : avance testée à 30 px seulement. L'index des specs ne change pas.
- Pas de smoke test (type `test`). `pio test -e native` et `pio run -e esp32` non exécutés ici (suites compilées à la main, voir le handoff) ; la CI de la PR fait foi.

## #77 — feat(token): renew the session with a fresh token when the hub announces its expiry

**Contexte** — La décision #50 a spécifié US-FW-007 (CA-1 à CA-7) sur la lecture du code : à `token_expiring_soon`, `TokenSessionMessageSender::SendAuthRefresh` renvoyait le **dernier token** (même `exp`, donc une boucle de `token_expiring_soon` côté hub, US-021 CA-1 et CA-11), et l'`auth_success` de réponse était rendu comme une connexion (« Connecté », bandeau : la question en cours aurait été remplacée, mode armé conservé). CA-5 et CA-7 étaient implémentés sans test. Le hub (`quiz-live-server` à `2050a2d`, relu le 2026-11-25) ne contient toujours aucune occurrence de `token_expiring_soon`, `token_expired` ni `auth_refresh` : le chemin reste mort de bout en bout.

**Décision** (cadrage validé par Olivier ; choix du temps : preuve en tests natifs, ni smoke test, ni modification du hub ou de `fake-hub.ps1`) —
- **CA-2** : `SendAuthRefresh` demande un login frais (`TokenFetcher`) et n'envoie `auth_refresh` qu'avec le token obtenu, jamais l'ancien.
- **CA-3** : échec du login (`Failed` : réseau, `400`, `429`, `5xx`) ou `200` sans token : aucun `Write`, aucune relance.
- **CA-4** : `401` : aucun `Write`, et `LoginRefusalListener` n'est **pas** prévenu (contrairement à `SendAuth`) : rien ne s'arrête.
- **CA-6** : `ConnectionEventHandler` compte les `auth_refresh` envoyés dont l'`auth_success` n'est pas revenu ; un `auth_success` reçu avec un compte non nul le décrémente, remet le compteur d'élimination à 0 (`ReconnectionPolicy::OnAuthSuccess`) et s'arrête là : ni rendu, ni `OnLinkEstablished`, ni aiguillage.
- **CA-5 et CA-7** : testés, sans changement de code ; nés verts, mutés (voir le handoff).
- **Statut de US-FW-007** : « Implémentée » dans l'index (CA-1 à CA-7 tenus en natif), avec le chemin mort de bout en bout consigné au spec et dans la dette « `auth_refresh` sur `token_expiring_soon` : chemin mort ».

**Choix de l'agent, à relire en PR** (aucun ne vient du CDC ; les trois premiers sont observables dans un cas limite, les suivants non) —
- **Un compteur, pas un drapeau**, pour « `auth_refresh` en attente » : CA-5 envoie un `auth_refresh` par message d'une rafale, donc le hub répondra autant de fois ; un drapeau afficherait la deuxième réponse. Le spec dit « un renouvellement par message » (CA-5) et « l'`auth_success` qui répond à un `auth_refresh` » (CA-6) sans les croiser : c'est leur conjonction, pas un silence. Écarté : un drapeau (plus simple, faux en rafale).
- **Le compteur ne monte que si l'`auth_refresh` est parti** (`SendAuthRefresh` retourne un booléen, vrai si le `Write` a eu lieu) : un login échoué ou refusé n'attend aucune réponse, l'`auth_success` suivant est donc rendu. Écarté : un compteur incrémenté à chaque `token_expiring_soon` (il silencierait un `auth_success` de reconnexion).
- **Remis à 0 à `OnConnectionEstablished`** : la réponse à un `auth_refresh` d'une connexion fermée ne viendra jamais ; sans cela, l'`auth_success` suivant l'ouverture d'une nouvelle connexion (D4, US-FW-019 CA-1) resterait muet (CA-6 : « seul l'`auth_success` suivant une ouverture s'affiche »). Pas remis à 0 à `OnConnectionLost` : l'ouverture suivante le fait toujours, un seul endroit. Limite : un hub qui répondrait à `auth_refresh` par autre chose qu'un `auth_success` laisserait le compteur à 1 jusqu'à la prochaine ouverture, et le prochain `auth_success` de la connexion serait silencieux (consigné en dette).
- **Le token du renouvellement devient le dernier token connu** (repli de `auth` quand son login échoue, US-FW-005 CA-9) : comme `SendAuth`, il est plus récent que celui de l'ouverture. Non exigé par un CA ; un test le fige (`SendsAuthCarryingTheRenewedTokenWhenTheFetchFailsAfterARenewal`). Écarté : ne rien retenir (le repli renverrait un token plus ancien, donc plus près de son `exp`).
- **L'`auth_success` de réponse ne passe pas par `OnLinkEstablished`** : le spec dit « c'est tout » après la remise à 0 ; la méthode ne fait que lever la coupure de liaison, impossible ici (l'ouverture a remis le compteur à 0, une coupure précède une ouverture).
- **L'état vit dans `ConnectionEventHandler`, pas dans `TokenSessionMessageSender`** : c'est lui qui voit les messages reçus et décide du rendu ; l'émetteur reste sans état de réponse. Conséquence : `SessionMessageSender::SendAuthRefresh()` passe de `void` à `bool` (interface et mock). Écarté : un second port « `AuthRefreshTracker` » (YAGNI), ou un état dans l'émetteur interrogé par le gestionnaire (couplage dans l'autre sens).
- **Pas de factorisation `SendAuth` / `SendAuthRefresh`** : les deux appellent `Fetch()` et retiennent le token, mais leurs replis diffèrent (`auth` retombe sur le dernier token, le renouvellement n'envoie rien et `401` n'arrête pas le même chemin) ; une fonction commune demanderait des drapeaux. Refactor possible, non fait : trois lignes dupliquées.
- **Tests de `lib/` seulement ; `src/` inchangé** : `main.cpp` passe déjà `tokenSessionMessageSender` au gestionnaire. Le build `esp32` n'est donc pas indispensable à cette session, mais il n'a pas été exécuté (PlatformIO absent) ; seul le changement de signature de `SendAuthRefresh` touche une classe que `main.cpp` instancie, et `TokenSessionMessageSender` la redéfinit avec `override`.
- **Mutant survivant équivalent** (M19) : retirer le test du statut (`fresh.status != Success`) de `SendAuthRefresh` ne change aucun test, car `HttpTokenFetcher` ne renvoie un token non vide qu'avec `Success`. Un test avec `Failed` et un token non vide serait spéculatif : l'état n'existe pas. Le test du token vide est gardé (tué).
- **Nommage** : `pendingAuthRefreshReplies`, `AnAuthSuccess()` (matcher de test) ; le test qui figeait l'ancien token est renommé (`SendsAuthRefreshCarryingAFreshlyFetchedTokenNeverTheOldOne`), la référence de US-FW-004 CA-9 suit.

**Conséquences** —
- Dette « Renouvellement du token : preuves manquantes » soldée (supprimée) ; « `auth_refresh` sur `token_expiring_soon` : chemin mort » réécrite (le firmware est prêt, le hub et le bout en bout restent) ; « Un `POST /api/v1/token` synchrone à chaque connexion » : le renouvellement sert maintenant le même `POST` bloquant, dans le callback de réception, une fois par message d'une rafale.
- Spec US-FW-007 : statuts et preuves des CA-1 à CA-7, « Écart de preuve » réécrit ; index : « Implémentée ». US-FW-004 CA-9 : nom du test renommé.
- Quand le hub implémentera US-021, le smoke test d'intégration (rafale, login en échec puis refusé, `token_expired` puis `4002`) reste à faire, avec une touche de `fake-hub.ps1` pour les trois messages.
- 23 tests de plus (760 → 783) : `test_session` 9 → 14, `test_connection` 102 → 120. `pio test -e native` et `pio run -e esp32` non exécutés ici (suites compilées à la main, voir le handoff) ; la CI de la PR fait foi.

## #78 — docs(specs): frame the token login without blocking the receive loop

**Contexte** — Le login `POST /api/v1/token` est un appel bloquant (`ArduinoHttpPoster`, délai du framework non vérifié) : il gèle `client.loop()` dans le callback de connexion (login frais avant `auth`, US-FW-005 CA-9, décision #48) et dans le callback de réception (renouvellement, US-FW-007 CA-2, CA-5, décision #77). Le spec décrivait ce blocage comme un fait (US-FW-005 CA-8 et notes, US-FW-007 note « `POST` bloquant du renouvellement »), sans cible. La dette « Un `POST /api/v1/token` synchrone à chaque connexion » ne disait pas ce qu'on voulait à la place. Session de cadrage seule : aucun code, aucun test.

**Décision** (cadrage validé par Olivier, 2026-11-25) —
- **Périmètre** : seuls les `POST` du token « en jeu » deviennent non bloquants : le login à l'ouverture du WebSocket (US-FW-005 CA-9) et le renouvellement (US-FW-007 CA-2). Le login de `setup()` reste bloquant (init statique, décision #53 ; US-FW-005 CA-1, CA-2, CA-4, CA-11 inchangés). La photo (US-FW-020, décision #63) reste hors périmètre : sa dette demeure.
- **Délai** : 5 s au plus pour un `POST` du token en jeu. Au-delà, c'est un échec, avec les règles existantes : au login d'ouverture, `auth` part avec le dernier token connu (US-FW-005 CA-9) ; au renouvellement, aucun `auth_refresh` (US-FW-007 CA-3).
- **Un seul login de renouvellement à la fois** : un `token_expiring_soon` reçu pendant qu'un login de renouvellement est en vol est ignoré (le login en cours rend déjà un token frais). Cela **modifie** US-FW-007 CA-5 : « un renouvellement par message » devient « un renouvellement par login terminé » (CA-5, CA-8).
- **Résultat tardif** : un login en vol qui aboutit après la fermeture de la connexion qui l'a demandé n'envoie rien, ni `auth` ni `auth_refresh` ; le token frais est conservé comme dernier token connu (US-FW-005 CA-9, US-FW-007 CA-9).
- **Appui pendant le login d'une reconnexion, avant `auth`, en mode armé** : même règle que la coupure (décision #49, « Mode armé pendant la coupure » : constat reporté, ni CA ni code, dette conservée). Au premier démarrage, l'Inerte ignore l'appui (US-FW-005 CA-11) : rien à changer.
- **Login frais à chaque ouverture** : reste la règle (US-FW-005 CA-9, décision #48 inchangée). « Ne redemander que près de l'expiration (`expires_in`) » reste une piste de la dette, non traitée ici.

**Choix de Claude, à relire en PR** (hors des décisions ci-dessus) —
- **Trois CA pour la décision 3 et la décision 4 côté renouvellement** : CA-5 (la rafale : un renouvellement par login terminé), CA-8 (un message reçu pendant un login de renouvellement en vol est ignoré), CA-9 (résultat après la fermeture). CA-5 et CA-8 se recouvrent en partie : CA-5 est le constat observable d'une rafale, CA-8 la règle unitaire. Écarté : tout mettre dans CA-5 (un CA, deux comportements). La décision 4 côté login d'ouverture est dans US-FW-005 CA-9, pas dans un CA propre : le CA existe déjà.
- **Le non-blocage est décrit par son effet observable** (`client.loop()` continue de tourner : pings, messages du hub), sans nommer de mécanisme (tâche, machine à états, port) : le choix est celui de la session de code.
- **CA-3 de US-FW-007 et CA-1 complétés** : le délai de 5 s dépassé est un échec de CA-3 ; CA-1 renvoie à CA-8 (« sauf si un login de renouvellement est déjà en vol »). Aucune valeur ni règle nouvelle.
- **Statut de US-FW-007 : « Implémentée » devient « Partielle »** (spec et index `docs/specs/README.md`) : CA-5 décrit un comportement que le code n'a pas, CA-8 et CA-9 n'existent pas. Il redeviendra « Implémentée » quand le code aura suivi.
- **Numéro** : #78, le dernier numéro du journal étant #77 (le brief annonçait #74 comme dernier connu).
- **Le titre de la dette est conservé** (« Un `POST /api/v1/token` synchrone à chaque connexion ») : il est cité par les specs US-FW-005, US-FW-007, US-FW-020 et des décisions ; la dette est réécrite en « cadré / reste ».
- **Statut « Non implémenté : écart de code » sans nom de test** : aucun test n'existe pour les comportements cadrés ; les tests cités de CA-2, CA-3, CA-5 sont conservés avec la mention de ce qu'ils prouvent encore (le contenu envoyé) et de ce qui changera (CA-5).
- **Points ouverts non tranchés, consignés aux specs** : une nouvelle connexion ouverte pendant qu'un login est en vol ; un `token_expiring_soon` pendant le login d'ouverture ; un `401` reçu après la fermeture ; un login qui se termine alors que le buzzer est Éliminé ; ce que mesurent les 5 s (connexion, lecture, ou les deux) ; la justification « rafale » du compteur de US-FW-007 CA-6.

**Alternatives écartées** (options refusées pendant l'interview) —
- **Périmètre** : rendre aussi le login de `setup()` non bloquant (rouvrirait la décision #53 et US-FW-005 CA-1, CA-2, CA-11) ; un port HTTP commun au login et à la photo (cadrerait US-FW-020, hors de cette session).
- **Délai** : 10 s ; aucun délai fixé (non testable).
- **Rafale** : un renouvellement par message, mis en file ; un login de suivi regroupé.
- **Réponse tardive** : jeter entièrement le résultat (le token frais serait perdu) ; l'envoyer sur la connexion courante.
- **Appui pendant le login d'une reconnexion** : l'ignorer tant que `auth` n'est pas parti ; le retenir puis l'envoyer après `auth_success`.
- **Login frais** : ne redemander que près de l'expiration (`expires_in`).

**Conséquences** —
- Écarts de code : US-FW-005 CA-8 (délai de 5 s du login en jeu) et CA-9 (non-blocage, délai, résultat après fermeture) ; US-FW-007 CA-2 (non-blocage, délai), CA-3 (délai), CA-5 (un par login terminé, remplace un par message), CA-8, CA-9. Tests à adapter : `ABurstOfTokenExpiringSoonTriggersOneAuthRefreshPerMessage` et `ConnectionEventHandlerRenewalSessionTest.ABurstOfTokenExpiringSoonRenewsTheSessionOncePerMessageWithEachFreshToken` décrivent l'ancien comportement ; les tests de `test_session` supposent un login synchrone ; le commentaire de `ConnectionEventHandler.h` sur la rafale change aussi.
- Dette « Un `POST /api/v1/token` synchrone à chaque connexion » réécrite : cadrée, pas soldée ; restent le code, le login de `setup()`, la photo, « ne redemander que près de l'expiration » et l'appui en mode armé (constat « Mode armé pendant la coupure »).
- Spec US-FW-007 : « Partielle » ; spec US-FW-005 : reste « Partielle », CA-8 et CA-9 en écart de code. Index mis à jour.
- Pas de smoke test, pas de code, pas de test (type `docs`) ; ni `pio test` ni `pio run` : la CI de la PR fait foi.

## #79 — docs(specs): settle the open points of the non-blocking token login

**Contexte** — La décision #78 a cadré un login du token non bloquant (délai de 5 s, un seul login de renouvellement à la fois, résultat tardif sans envoi) mais a consigné six points muets sans les trancher (US-FW-005 et US-FW-007, « Constats reportés »). Ils conditionnent le port du login et ses tests. Dernier numéro du journal relu avant d'écrire : #78, donc #79 (aucun écart). Session de cadrage seule : aucun code, aucun test.

**Décision** (cadrage validé par Olivier, 2026-11-25) —
1. **Les 5 s mesurent la durée totale du login**, de l'envoi du `POST` à la fin de lecture de la réponse (une seule borne). Raison : fidèle au « dure 5 s au plus » de US-FW-007 CA-2 ; pire cas 5 s, et non 10 s. (US-FW-005 CA-8 ; US-FW-007 CA-2, CA-3)
2. **Une nouvelle connexion s'ouvre pendant qu'un login est en vol** (ouverture ou renouvellement) : il est **abandonné** et un login frais le remplace. Au plus un login en vol, même à l'ouverture (cohérent avec US-FW-007 CA-8 et la décision #48). Un login déjà terminé avant la réouverture reste le dernier token connu (US-FW-007 CA-9). (US-FW-005 CA-12 ; US-FW-007 CA-9)
3. **Un login qui se termine par `401` après la fermeture de la connexion qui l'avait demandé** : résultat **ignoré**, rien ne s'arrête (cohérent avec US-FW-007 CA-9 et la décision 2). Le login de la connexion suivante recevra le `401` et arrêtera le buzzer par US-FW-006 CA-11, au prix d'un cycle de plus. US-FW-007 CA-4 (refus d'un renouvellement : rien ne s'arrête) reste vrai.
4. **Le buzzer devient Éliminé pendant qu'un login est en vol** : le login est **abandonné**, son résultat jeté, aucun token gardé ; rien n'est lu ni envoyé après l'élimination (silence total, US-FW-006 CA-9, CA-10).
5. **Un `token_expiring_soon` reçu pendant le login d'ouverture** (avant l'envoi de `auth`) est **ignoré** : US-FW-007 CA-8 vaut pour tout login en vol, d'ouverture ou de renouvellement (un seul login à la fois ; le login d'ouverture donne déjà un token frais).
6. **Le compteur `pendingAuthRefreshReplies` (US-FW-007 CA-6) est gardé**, comportement inchangé (tests de CA-6 intacts). Sa justification « rafale » est reformulée : « deux renouvellements successifs avant la première réponse » (un login se termine et envoie `auth_refresh`, puis un nouveau `token_expiring_soon` arrive avant l'`auth_success`).

**Choix de Claude, à relire en PR** (hors des six décisions ci-dessus ; aucun ne vient d'Olivier) —
- **Numérotation des CA** : US-FW-005 garde CA-8 et CA-9 (modifiés) et gagne **CA-12** (les cas limites du login d'ouverture : nouvelle connexion, `401` tardif, élimination, `token_expiring_soon`), sans renuméroter l'existant ; US-FW-007 modifie CA-2, CA-3, CA-6, CA-8 et CA-9 en place, sans CA nouveau (le brief laissait le choix). Écarté : tout mettre dans CA-9 de US-FW-005 (quatre comportements dans un CA déjà dense).
- **Un login abandonné par une nouvelle connexion ne garde aucun token** : la décision 2 dit « abandonné » sans dire ce que devient son résultat ; la décision 4 dit « aucun token gardé » pour l'élimination, et j'ai lu « abandonné » de la même façon. La règle de #78 (résultat tardif : token frais conservé) ne vaut donc que pour un login qui aboutit après la fermeture et **avant** toute réouverture, ce que la décision 2 (« un login déjà terminé avant la réouverture reste le dernier token connu ») confirme. Écarté : garder le token d'un login abandonné.
- **Le `401` ignoré de la décision 3 est écrit dans les deux specs** (US-FW-005 CA-12 pour le login d'ouverture, US-FW-007 CA-9 pour le renouvellement) : la décision 3 ne distingue pas les deux logins. Un `401` reçu alors que la connexion est encore ouverte garde la règle existante (US-FW-006 CA-11, mentionnée seulement en contraste, rien de changé).
- **Les 5 s sont recopiés avec la formule d'Olivier**, sans nommer de mécanisme ni dire si l'établissement de la connexion TCP qui précède l'envoi y entre : le brief ne le dit pas, c'est le choix de la session de code. Si cela doit être tranché, c'est un point à challenger.
- **US-FW-006 inchangé** : aucune référence croisée ne devient fausse (CA-9 et CA-10 n'évoquent aucun login en vol ; « plus de `POST /api/v1/token` » à l'élimination reste vrai, le login en vol étant abandonné ; CA-11 s'applique au login de la connexion suivante). Index `docs/specs/README.md` inchangé : US-FW-005 et US-FW-007 restent « Partielle ».
- **Statuts** : CA-8, CA-9 et CA-12 de US-FW-005, CA-2, CA-3, CA-5, CA-8 et CA-9 de US-FW-007 restent « Non implémenté : écart de code ». Aucun test nouveau ni preuve inventée : CA-12 n'a aucune preuve ; CA-6 de US-FW-007 garde ses statuts et ses tests (seule la justification du compteur change).
- **La décision #78 n'est pas réécrite** : son point « Un seul login de renouvellement à la fois » est étendu à « un seul login à la fois » par la décision 5, et son « résultat tardif » est précisé par les décisions 2 à 4. Le journal reste append-only ; ses « points ouverts non tranchés » sont soldés ici.

**Conséquences** —
- US-FW-005 : CA-8 (durée totale), CA-9 (précisé), **CA-12** (nouveau) ; la puce « Points ouverts du cadrage du 2026-11-25 » renvoie à cette décision. US-FW-007 : CA-2, CA-3, CA-6 (justification), CA-8, CA-9 ; même renvoi.
- Écarts de code (inchangés dans leur nature, plus précis) : login non bloquant, 5 s de durée totale, abandon par une nouvelle connexion ou l'élimination, `401` tardif ignoré, `token_expiring_soon` ignoré pendant le login d'ouverture. Tests à écrire ou adapter : ceux de rafale de US-FW-007 CA-5 (`ABurstOfTokenExpiringSoonTriggersOneAuthRefreshPerMessage`, `ConnectionEventHandlerRenewalSessionTest.ABurstOfTokenExpiringSoonRenewsTheSessionOncePerMessageWithEachFreshToken`) ; ceux de CA-6 restent. Le commentaire de `ConnectionEventHandler.h` sur la rafale change à la session de code.
- Dette « Un `POST /api/v1/token` synchrone à chaque connexion » : cadrage complété, plus de point ouvert ; reste le code.
- Hub toujours sans US-021 : rien de bout en bout. Pas de smoke test, pas de code, pas de test (type `docs`) ; ni `pio test` ni `pio run` : la CI de la PR fait foi.

## #80 — feat(token): log in without blocking the receive loop

**Contexte** — Les décisions #78 et #79 ont cadré le login du token « en jeu » non bloquant (US-FW-005 CA-8, CA-9, CA-12 ; US-FW-007 CA-2, CA-3, CA-5, CA-8, CA-9) sans le coder : `TokenSessionMessageSender` appelait `TokenFetcher::Fetch()` (un `POST` bloquant) dans le callback de connexion (`auth`) et dans le callback de réception (`auth_refresh`), `client.loop()` ne tournait pas pendant ce temps. Session de code : d'abord le login d'ouverture, puis le renouvellement. Dernier numéro du journal relu avant d'écrire : #79 (aucun écart). Hors périmètre, inchangés : le login de `setup()` (bloquant, `TokenAcquisition`), le `GET` de la photo (US-FW-020), « ne redemander un token que s'il approche de son expiration ».

**Décision** (cadrage figé par Olivier ; ce qui suit est la réalisation) —
- **Port `AsyncLogin`** (`lib/session/`) : `Start()` rend la main, `Poll()` rend le résultat une fois le login terminé, `Abandon()` le jette. Adaptateur `src/TaskAsyncLogin.cpp` : une tâche FreeRTOS par login exécute le `HttpTokenFetcher` existant (inchangé) et pose son résultat dans un état partagé (`LoginRun`, drapeau atomique) ; `Abandon()` lâche cet état.
- **`TokenSessionMessageSender`** (`lib/session/`) tient tout l'état du login : un seul login en vol, d'ouverture ou de renouvellement (CA-8, CA-12), la borne de 5 s (port `Clock`, `ArduinoClock` sur `millis()`, comparaison par différence non signée), « la connexion qui l'a demandé est-elle encore ouverte ». `PollLogin()`, appelé à chaque tour de `loop()`, recueille la fin : ouverture → `auth` (token frais, sinon dernier token connu), `401` → `LoginRefusalListener` ; renouvellement → `auth_refresh` seulement avec un token frais ; connexion fermée → rien n'est envoyé, `401` ignoré, token frais conservé comme dernier token connu ; nouvelle connexion ou élimination → `Abandon()`, aucun token gardé.
- **`ConnectionEventHandler`** : transmet `OnConnectionClosed()` (perte de la connexion) et `Abandon()` (élimination) à la session ; implémente `SessionPoller::PollSession()`, qui compte les `auth_refresh` partis (compteur de CA-6, comportement inchangé). Nouveau `SessionPollingNetworkLoop` : le tour de réseau (`client.loop()`) puis `PollSession()` ; `BuzzerLoop` est inchangé et `main.cpp` le câble.
- **Interface `SessionMessageSender`** : `SendAuth()` et `SendAuthRefresh()` (void) lancent le login ; `PollLogin()` (bool : un `auth_refresh` est parti), `OnConnectionClosed()`, `Abandon()`.

**Choix de Claude, à relire en PR** (aucun ne vient du CDC ni d'Olivier, sauf le premier qui vient de #79) —
- **Un login abandonné par une nouvelle connexion a son résultat jeté**, comme à l'élimination : « Choix de Claude » de #79, repris tel quel à la demande d'Olivier. Test : `KeepsNoTokenFromTheLoginAbandonedByANewConnection` (résultat prêt, jamais lu).
- **Observable : un résultat prêt est pris même s'il est observé après 5 s.** `PollLogin()` interroge le port d'abord ; le délai ne s'applique qu'à un login **encore en vol** (à partir de 5 000 ms, borne comprise). Le CDC dit « au-delà de 5 s, c'est un échec » sans dire si c'est l'heure de fin ou l'heure d'observation. Raison : l'heure de fin n'est pas connue de `lib/`, et un tour long (un `connect()` bloque jusqu'à 5 s) ne doit pas jeter un token frais déjà reçu. Conséquence : le seuil est celui du premier tour qui voit le login non terminé à 5 000 ms ou plus. Écarté : tester le délai avant d'interroger le port (jette un résultat prêt) ; horodater la fin dans l'adaptateur (de la logique dans `src/`).
- **Observable : les 5 s courent du lancement** (`Start()` : création de la tâche, connexion TCP, `POST`, lecture), pas de « l'envoi du `POST` » au sens strict (#79 laisse ce point à la session de code). Une borne unique, côté logique ; le temps de création de la tâche et de connexion TCP y est compté, ce qui rend le pire cas plus court, jamais plus long.
- **Observable : le `POST` d'un login abandonné n'est pas interrompu sur le réseau** : `HTTPClient` bloquant ne s'annule pas depuis l'extérieur de la tâche. La tâche finit seule, son résultat est jeté (CA-12 : « résultat jeté »). Deux `POST` peuvent donc être en vol côté hub pendant quelques secondes ; consigné en dette. Écarté : annuler en fermant le socket depuis la boucle (état partagé avec la tâche, `WiFiClient` non thread-safe), ou une pile HTTP asynchrone (nouvelle dépendance, réécriture de `HttpPoster`).
- **Mécanisme : une tâche FreeRTOS autour de l'appel bloquant existant**, plutôt qu'un client HTTP non bloquant écrit à la main (machine à états sur `WiFiClient`, qui réécrirait `HttpTokenFetcher`/`ArduinoHttpPoster`, testés) ou une bibliothèque asynchrone. Pile de 8 192 octets (non mesurée), priorité 1, non épinglée à un cœur. Un échec de création de tâche est un login échoué (une branche dans `src/`, consignée en dette).
- **L'état « un seul login en vol » vit dans `TokenSessionMessageSender`**, pas dans le gestionnaire : le login d'ouverture et celui du renouvellement partagent la même case (CA-8). Le gestionnaire continue de transmettre **chaque** `token_expiring_soon` ; c'est la session qui ignore. Conséquence : le test de rafale du gestionnaire avec une session simulée est supprimé (il ne décrit plus rien d'observable) et remplacé par des tests avec la vraie session.
- **Écart avec la consigne « CA-6 et ses tests gardés tels quels »** : le comportement de CA-6 et les assertions de ses tests n'ont pas changé, mais l'interface a dû changer (un login asynchrone ne peut plus dire « envoyé » au moment de la demande, `SendAuthRefresh()` ne rend plus de booléen) : dans les sept tests de CA-6 du gestionnaire et dans le fixture `ConnectionEventHandlerRenewalRenderingTest`, **l'arrangement** a suivi (`ON_CALL(…PollLogin())` et un `handler.PollSession()` après chaque `token_expiring_soon`) ; les noms et les assertions n'ont pas bougé, et le fixture du gestionnaire passe d'une session simulée « naggy » à `NiceMock`. Écarté : garder `bool SendAuthRefresh()` (il ne serait vrai que dans les tests, code mort en production) ; un port « compteur de réponses » partagé (changerait tous les constructeurs du gestionnaire). À valider par Olivier : c'est un écart à une consigne, pas à un CA.
- **Un compteur CA-6 qui compte à l'envoi**, pas à la demande (comme #77) : `PollLogin()` rend vrai seulement quand `auth_refresh` est écrit ; un login échoué, refusé, dépassé, ou terminé sur une connexion fermée n'attend aucune réponse.
- **`OnConnectionClosed()` n'est transmis que si le réseau n'est pas arrêté** (le gestionnaire ignore déjà toute fermeture voulue, CA-11) ; `Abandon()` est appelé à l'élimination (`PropagateEliminationIfAny`), pas au refus d'un `401` (aucun login n'est alors en vol).
- **Une nouvelle connexion abandonne même un login déjà terminé mais pas encore recueilli** : son résultat, prêt mais jamais lu, est jeté (conforme à la décision 2 de #79 : « un login déjà terminé avant la réouverture » est un login recueilli ; l'écart de quelques ms entre la fin et le tour de `loop()` n'est pas distingué).
- **Nommage** : `AsyncLogin`, `SessionPoller`, `SessionPollingNetworkLoop`, `Clock`, `PollLogin`, `OnConnectionClosed`, `Abandon`, `TaskAsyncLogin`, `LoginRun` ; doubles de test `FakeAsyncLogin` et `FakeClock` (`test/mocks/LoginFakes.h`, à état, plus lisibles que des attentes gmock en séquence). Les noms de tests cités par les specs (`SendsAuthCarryingAFreshlyFetchedToken`, `…NeverTheOldOne`, etc.) sont conservés.
- **Statut de US-FW-007 : « Implémentée »** (spec et index), le chemin restant mort de bout en bout (US-021 du hub). US-FW-005 reste « Partielle ». **Dette renommée** : « Un `POST /api/v1/token` synchrone à chaque connexion » devient « Login du token : ce que le non-blocage ne couvre pas » (le titre n'était plus vrai) ; les renvois des specs US-FW-005 et US-FW-007 suivent, l'historique des décisions et des handoffs n'est pas réécrit.
- **Mutant survivant équivalent** : retirer le test du statut dans `hasFreshToken` (`result.status == Success && !token.empty()`) ne change aucun test, car `HttpTokenFetcher` ne rend un token non vide qu'avec `Success` (même raison que le M19 de #77). Un test avec `Failed` et un token non vide serait spéculatif.

**Alternatives écartées** — voir les « Écarté » ci-dessus ; en plus : un seul login « d'ouverture » regroupé avec celui de `setup()` (rouvrirait #48), un délai dans `ArduinoHttpPoster` (`setTimeout` de `HTTPClient` borne chaque lecture, pas la durée totale), horodatage de la fin dans la tâche.

**Conséquences** —
- Code : `lib/session/` (`AsyncLogin.h`, `SessionPoller.h`, `SessionPollingNetworkLoop.h`, `SessionMessageSender.h`, `TokenSessionMessageSender.h`), `lib/startup/Clock.h`, `lib/connection/ConnectionEventHandler.h` ; `src/` (`TaskAsyncLogin.*`, `ArduinoClock.*`, `main.cpp`).
- Tests : 783 → 826 (`test_session` 14 → 45, `test_connection` 120 → 132) ; 40 mutants, 39 tués (dont 3 seulement après l'ajout de tests : débordement du compteur avant la bascule, état non remis à zéro après `Abandon`, `PollLogin` sans garde), 1 équivalent. Suites compilées à la main (g++ 13, C++17, googletest et googlemock clonés hors dépôt) ; `pio test -e native` et `pio run -e esp32` **non exécutés** (registre PlatformIO bloqué) : la CI de la PR fait foi pour `native`, le build `esp32` n'a jamais été fait pour `src/TaskAsyncLogin.cpp`.
- Specs : US-FW-005 CA-8, CA-9, CA-12 et US-FW-007 CA-2, CA-3, CA-5, CA-8, CA-9 : « implémenté, testé en natif » ; « Écart de preuve » réécrits ; index : US-FW-007 « Implémentée ».
- Dette : renommée et réduite (voir ci-dessus) ; « Adaptateurs validés par smoke test seulement » prend note de la branche de `TaskAsyncLogin`.
- Smoke test à prévoir (non fait ici) : hub arrêté ou lent à l'ouverture (le buzzer continue de répondre aux pings, `auth` part au bout de 5 s avec le dernier token), puis, avec US-021, un renouvellement.

## #81 — feat(rendering): show the wait for the game master after auth_success

**Contexte** — La décision #62 (US-FW-019 CA-1) remplace l'écran « Connecté » de chaque `auth_success` qui suit l'ouverture d'une connexion par D4 : deux lignes de même taille, « En attente du maître du jeu » puis « La partie va bientôt commencer », sous le bandeau du haut portant l'`username`. Le code rendait encore « Connecté » seul (écart constaté sur la dalle au smoke test du 2026-11-25, O01), et quatre tests figeaient l'ancien texte. Session de code, un seul CA : US-FW-019 CA-1. Dernier numéro du journal relu avant d'écrire : #80 (aucun écart), donc #81. Hors périmètre, inchangés : `game_started` et D5, CA-2 à CA-5 de US-FW-019, le texte de `game_resumed` (« Partie en cours », US-FW-010 CA-4 et CA-5), le cadenas de l'écran de refus.

**Décision** —
- **Le texte de `auth_success` est celui de D4** : `DisplayState::LcdTextFor` rend « En attente du maître du jeu\nLa partie va bientôt commencer ». Le remplacement de l'écran, le bandeau avec l'`username`, les zones inactives, le mode inchangé, le silence vers le hub et vers le répartiteur, et l'`auth_success` de réponse à `auth_refresh` (écran inchangé, US-FW-007 CA-6) existaient déjà : le seul changement de `lib/` est ce texte.
- **Les quatre tests qui figeaient « Connecté » sont réécrits pour D4** (rouges d'abord pour la bonne raison : l'ancien texte), et renommés quand leur nom disait « Connecté » : `DisplayStateTest.AuthSuccessShouldDisplayTheWaitForTheGameMasterOnTwoLines`, `ScreenPresenterTest.AuthSuccessPresentsTheWaitForTheGameMasterOnTwoLines`, `LcdRendererTest.AuthSuccessMessageShowsTheWaitForTheGameMasterOnScreen` ; `ConnectionEventHandlerRenderingTest.AuthSuccessIsRenderedWithTheUsernameOfTheHubInTheTopBar` garde son nom (il reste vrai).
- **Deux tests nouveaux, nés verts, mutés** : `ConnectionEventHandlerRenderingTest.AuthSuccessThenGameResumedAreRenderedAsTwoDisplaysInOrderWithTheSameTopBar` (US-FW-010 CA-6 et US-FW-019 CA-1 : D4 puis « Partie en cours », deux `Display` dans l'ordre, même bandeau) et `TappableZonesScreenTest.TheWaitForTheGameMasterScreenOfAnAuthSuccessDeactivatesTheZones` (D4 sans zones tactiles).

**Choix de Claude, à relire en PR** (aucun ne vient du CDC) —
- **Les deux lignes sont un seul `TextScreen` séparé par `\n`**, comme l'écran d'init (US-FW-010 CA-1, décision #28) : le dessin respecte déjà les retours à la ligne (US-FW-019 CA-8) et donne une seule taille aux deux lignes. Écarté : un nouveau modèle d'écran à deux champs (YAGNI, le dessin devrait le visiter).
- **Nommage** : `DisplayState::LcdTextForWaitingForGameMaster()`, sur le modèle de `LcdTextForInit()`. Non observable.
- **Quatre tests réécrits en plus des quatre cités par l'US**, parce que leur attente dérive du texte de l'`auth_success` : `LcdRendererLinkLostTest.AnAuthSuccessAfterTheLossShowsALinkedTopBarAgain` (`test_lcd_renderer`), `ConnectionLostRenderingTest.EachCountedFailureRepaintsOnlyTheBannerWithTheNextAttemptAndTheThirdOneShowsTheOutOfGameScreen`, `…AnAuthSuccessDuringTheLossReplacesTheConnectionLostScreenWithTheLinkedTopBar` et `…TheQuestionIsNotRestoredByTheAuthSuccessAndTheNextLossDimsTheScreenOfTheAuthSuccess` (`test_connection`, l'écran atténué est celui de D4). Aucune assertion n'a changé hormis ce texte.
- **Le test des zones passe par `ScreenPresenter::ScreenFor`**, pas par un `TextScreen` écrit à la main : il échoue si le présenteur rend un `ZonesScreen` pour `auth_success`, ce que le test générique `ATextScreenDeactivatesTheZones` ne voit pas. Le comportement testé (CA-6 : hors périmètre) existait ; D4 reste sans zones, c'est ce que le test fige.
- **Le CDC §8.2 n'est pas modifié** : sa ligne `auth_success` dit encore « Connecté » puis « En attente d'une question ». Il est en lecture seule et la décision #62 et US-FW-010 CA-3, CA-4 l'ont corrigée ; la dette « Table de rendu §8.2 incomplète » le note. Les mentions de « Connecté » des US-FW-011, 016, 017 et 018 ne sont pas réécrites non plus (#62 : elles se lisent « D4 »).
- **Mutant survivant équivalent** : faire passer `auth_success` par `HubMessageDispatcher::Dispatch` ne change rien d'observable : le répartiteur ignore ce type (`Dispatch` ne connaît que cinq types), l'absence d'aiguillage est donc garantie par la structure, pas par un test ; un test avec un répartiteur simulé serait spéculatif.

**Alternatives écartées** — voir les « Écarté » ci-dessus ; en plus : laisser « Connecté » le temps de D5 (la décision #62 dit « pour tout `auth_success` »), un texte différent après une reconnexion (le CDC n'en distingue pas).

**Conséquences** —
- Code : `lib/rendering/DisplayState.h` seul. Tests : 826 → 828 (`test_connection` 132 → 133, `test_tappable_zones` 25 → 26), huit tests réécrits.
- Specs : US-FW-019 CA-1 « implémenté, testé en natif » (statut de l'US inchangé : « Partielle », CA-2 à CA-5 non implémentés), US-FW-010 CA-3, CA-4 (pour `auth_success` seulement) et CA-6 mis à jour ; index (README) à jour.
- Dettes : « Table de rendu §8.2 incomplète », « Écrans d'init et d'attente : preuves manquantes », « Attente du maître du jeu et présentation de la partie : preuves manquantes et tests à écrire » (et « contrat hub… »), « `TappableZonesScreen` : visiteur à repli générique », « Bandeau… » et mentions de « Connecté » tenues à jour du réel. D4 reste **non constaté sur la dalle** (le dessin de deux lignes sous le bandeau n'a jamais été vu) : smoke test à prévoir, `auth_success` seul avec `scripts/fake-hub.ps1` ; le faux hub n'a toujours pas de touche `game_started`.
- Vérifications : suites compilées à la main (g++ 13, C++17, googletest et ArduinoJson hors dépôt), 19 mutants. `pio test -e native` et `pio run -e esp32` non exécutés (registre PlatformIO bloqué) ; aucun fichier de `src/` ni `main.cpp` modifié : la CI de la PR fait foi pour `native`.

## #82 — feat(rendering): show the wait for a question after game_resumed

**Contexte** — Le CDC §8.2 (ligne `game_resumed`) donne la même mise en page que `auth_success` : « Partie en cours » puis « En attente d’une question » ; US-FW-010 CA-4 et CA-5 la reprennent (libellé de la seconde ligne proposé par la décision #28). Le code rendait « Partie en cours » seul, et deux tests figeaient ce texte court. Session de code, un seul CA : US-FW-010 CA-4, partie `game_resumed`. Dernier numéro du journal relu avant d'écrire : #81 (aucun écart), donc #82. Hors périmètre, inchangés : D4 et `auth_success`, CA-5 (champs `question_index`, `cumulative_score` et `status` ignorés) et CA-6 (enchaînement en deux rendus) dans leur comportement, tout dessin sur la dalle. Le hub n'émet pas `game_resumed` (US-019 du hub seulement) : le chemin se prouve par les tests natifs, sans émetteur de bout en bout (précédent : `feat(token)`, d'où le type `feat`, le firmware réagissant déjà à ce message en production).

**Décision** —
- **Le texte de `game_resumed` porte deux lignes** : `DisplayState::LcdTextFor` rend « Partie en cours\nEn attente d’une question », via `LcdTextForGameResumed()` (même convention que D4 et l'init : un seul `TextScreen`, retour à la ligne, une seule taille). Le bandeau gardé, les zones désactivées, le mode inchangé, le silence vers le hub, l'enchaînement `auth_success` puis `game_resumed` existaient déjà : le seul changement de `lib/` est ce texte. Ni `src/` ni `main.cpp` ne bougent.
- **Les deux tests qui figeaient le texte court sont réécrits** (rouges d'abord pour la bonne raison) : `DisplayStateTest.GameResumedShouldDisplayTheWaitForAQuestionOnTwoLines` (renommé : l'ancien nom disait « Partie en cours » seul) et l'attente de `ConnectionEventHandlerRenderingTest.AuthSuccessThenGameResumedAreRenderedAsTwoDisplaysInOrderWithTheSameTopBar` (nom inchangé, il reste vrai). Les autres suites étaient vertes après le premier vert : aucun autre test ne dérivait de ce texte.
- **Quatre tests nouveaux, nés verts, mutés** : `ScreenPresenterTest.GameResumedPresentsTheWaitForAQuestionOnTwoLines`, `ScreenPresenterTest.GameResumedPresentsTheWaitForAQuestionEvenWhenAQuestionIsArmed`, `LcdRendererTest.GameResumedMessageShowsTheWaitForAQuestionOnScreenUnderTheTopBarOfTheAuthSuccess`, `TappableZonesScreenTest.TheWaitForAQuestionScreenOfAGameResumedDeactivatesTheZones`.

**Choix de Claude, à relire en PR** (aucun ne vient du CDC) —
- **Nommage** : `DisplayState::LcdTextForGameResumed()`, sur le modèle de `LcdTextForWaitingForGameMaster()`. Non observable. Écarté : laisser la chaîne en ligne dans `LcdTextFor` (D4 et l'init ont chacun leur méthode ; les deux lignes se lisent mieux nommées).
- **Le test « buzzer armé » du présenteur** n'est pas demandé par le brief : il est né du mutant « `game_resumed` laisse l'écran inchangé quand une question est armée », qui survivait. CA-5 dit que l'écran est remplacé et que le mode reste armé, sans zones ; le test fige la première moitié.
- **Le test des zones passe par `ScreenPresenter::ScreenFor`**, pas par un `TextScreen` écrit à la main (même raison que pour D4, décision #81).
- **Le bandeau gardé n'a pas de test nouveau côté présenteur** : `MessageKeepingTheTopBarTest` couvre déjà `game_resumed` (décision #73) ; côté `LcdRenderer`, le test nouveau pose le bandeau par un `auth_success` puis vérifie que `game_resumed` le reprend.
- **Mutant survivant, non équivalent, laissé en dette** : un `ConnectionEventHandler` qui traiterait `game_resumed` comme `auth_success` (appel de `OnLinkEstablished`, donc le mode repartirait) ne fait échouer aucun test : le « mode inchangé » de CA-5 n'est testé qu'au répartiteur (`KnownTypeWithoutEffectOnTheModeTest`). Comportement existant, hors du texte de CA-4 : pas élargi ici. Dette « Écrans d'init et d'attente : preuves manquantes » mise à jour.
- **Le CDC §8.2 n'est pas modifié** (lecture seule) : sa ligne `game_resumed` est déjà juste ; seule celle de `auth_success` est périmée (voir #81).

**Alternatives écartées** — un type `chore` (code dormant) : le firmware réagit déjà à `game_resumed` en production, le texte est visible dès que le hub l'émettra ; un nouveau modèle d'écran à deux champs (YAGNI, comme en #81) ; un texte différent de D4 pour la seconde ligne (le CDC dit « même mise en page que `auth_success` »).

**Conséquences** —
- Code : `lib/rendering/DisplayState.h` seul. Tests : 828 → 832 (`test_screen_presenter` 54 → 56, `test_lcd_renderer` 60 → 61, `test_tappable_zones` 26 → 27), deux tests réécrits.
- Specs : US-FW-010 CA-4 « implémenté, testé en natif » pour `auth_success` et `game_resumed`, CA-5 pour l'écran ; statut de l'US inchangé (« Partielle » : dessin jamais constaté, hub sans émetteur) ; index (README) à jour.
- Dettes : « Table de rendu §8.2 incomplète », « Écrans d'init et d'attente : preuves manquantes » (mutant survivant consigné) et « Attente du maître du jeu et présentation de la partie : preuves manquantes et tests à écrire » (même dessin à constater pour `game_resumed`) tenues à jour du réel. `game_resumed` **jamais constaté sur la dalle** (le hub ne l'émet pas : à constater avec `scripts/fake-hub.ps1` s'il l'émet, non vérifié).
- Vérifications : suites compilées à la main (g++, C++17, googletest et ArduinoJson hors dépôt), 23 mutants : 22 tués, 1 survivant (ci-dessus). `pio test -e native` et `pio run -e esp32` non exécutés (registre PlatformIO bloqué) ; aucun fichier de `src/` ni `main.cpp` modifié : la CI de la PR fait foi pour `native`.

## #83 — docs(specs): settle the space before the ellipsis of a truncated top bar name

**Contexte** — Le `feat(rendering): truncate a long name in the top bar` (US-FW-009 CA-5) s'est arrêté avant d'écrire du code : CA-5 dit que le nom est tronqué par « … » et tient dans la colonne de gauche, mais ne dit pas ce qui arrive quand la coupure tombe juste après une espace (« Jean-Baptiste Dupont-Martin » : « Jean-Baptiste… » ou « Jean-Baptiste … » ?). US-FW-008 CA-1, US-FW-017 CA-3, US-FW-018 CA-3, `DECISIONS.md` et `DEBTS.md` ne le disaient pas non plus. Dernier numéro du journal relu avant d'écrire : #82, donc #83 (aucun écart). Session de cadrage seule : aucun code, aucun test.

**Décision** (validée par Olivier, 2026-11-26) —
- **Quand la troncature du nom du bandeau tombe juste après une espace, l'espace est gardée** : le rendu est le plus long début du nom qui tient avec « … », espace comprise, puis « … ». « Jean-Baptiste Dupont-Martin », trop large, donne « Jean-Baptiste … » (et non « Jean-Baptiste… »).
- Raison donnée par Olivier : c'est le rendu de la maquette (`docs/mockups/mockup.css`, `text-overflow: ellipsis` sur `.who .name`) et la règle la plus simple, sans cas particulier.
- Écarté : retirer l'espace avant « … ».
- **Périmètre : le nom du bandeau du haut seulement** (US-FW-009 CA-5).

**Choix de Claude, à relire en PR** (aucun ne vient d'Olivier) —
- **Formulation du CA-5** : « le plus long début du nom qui tient avec « … » », reprise de la décision ; l'exemple est donné sous condition (« si « Jean-Baptiste » et l'espace tiennent avec « … » mais pas le « D » suivant ») car aucune largeur n'est mesurée ici ; je n'invente pas de pixels. La colonne « Preuve » reste « à écrire » et gagne « dont une coupure juste après une espace » ; le statut reste « Non implémenté ».
- **US-FW-017 CA-3 et US-FW-018 CA-3 ne sont pas modifiés** (hors périmètre) : ils disent « tronqué par « … » » sans préciser le cas de l'espace ; la maquette utilise aussi `text-overflow: ellipsis` pour les lignes du classement (`.row .nm`) et les marches du podium (`.pod .nm`), donc la même lecture est probable, mais **non décidée** : à trancher pour ces deux écrans avant leur `feat`.
- **En-tête de US-FW-009 et index (README) inchangés** : le statut « Partielle » et sa liste « restent non implémentés : … CA-5 (troncature) » restent vrais ; l'historique de l'en-tête rend compte des sessions de code, pas des décisions de cadrage seules.
- **Cas non tranchés, laissés** : un nom dont la partie qui tient n'a aucun caractère avec « … », et un nom qui commence par des espaces ; aucun n'est décidé par Olivier ni par le CDC.

**Alternatives écartées** — retirer l'espace avant « … » (un cas particulier de plus, et un écart avec la maquette).

**Conséquences** —
- US-FW-009 : CA-5 complété (une phrase et un cas de preuve) ; rien d'autre.
- `DEBTS.md` : la dette « Bandeau du haut : nom du participant, troncature, habillage et écran de refus » mentionne la règle ; elle reste ouverte (la troncature n'est pas implémentée).
- Code à écrire plus tard : `feat(rendering): truncate a long name in the top bar`, avec ce CA-5 complet.
- Pas de code, pas de test, pas de smoke test (type `docs`) ; ni `pio test` ni `pio run` : sans objet.

## #84 — feat(rendering): truncate a long name in the top bar

**Contexte** — US-FW-009 CA-5 (complété par la décision #83) : le nom du bandeau est tronqué par « … » pour tenir dans la colonne de gauche, de largeur (dalle − titre) ÷ 2 moins la marge de 10 px, avec les largeurs réelles de la police ; un nom qui tient est affiché entier ; sinon c'est le plus long début du nom qui tient avec « … », espace gardée. Le code dessinait le nom tel quel (`drawString` direct) : un nom long chevauchait le titre. Session de code, un seul CA : US-FW-009 CA-5. Dernier numéro du journal relu avant d'écrire : #83 (aucun écart), donc #84. Hors périmètre, inchangés : CA-2 (bloqué par le hub), CA-4, CA-6, CA-8, US-FW-017 CA-3 et US-FW-018 CA-3 (même question de l'espace, non décidée). Type `feat` : comportement observable modifié (un nom trop large est tronqué au lieu de chevaucher le titre).

**Décision** —
- **La troncature vit dans `lib/rendering/TopBarName.h`** : `TopBarName::Fit(measurer, size, name, panelWidth)` rend le nom entier s'il tient, sinon le plus long début (au caractère UTF-8 près, jamais au milieu d'un caractère) qui tient avec « … », tel quel. `TopBarName::ColumnWidth` donne la colonne : `(dalle − largeur du titre) / 2 − 10` (division entière). À 800 px avec Oswald 30 px : titre 119 px, colonne 330 px.
- **`GfxScreen::DrawTopBar` dessine le résultat de `Fit`**, mesuré avec le `ProportionalTextMeasurer` qu'il possède déjà, à la taille 2 (la police du bandeau, Oswald 30 px). Rien d'autre ne change dans `src/` ; `main.cpp` ne bouge pas.
- **Le titre et la marge deviennent des constantes de `TopBarName`** (`Title`, `Margin`), utilisées par `Fit` pour mesurer et par `GfxScreen` pour dessiner : la mesure et le dessin ne peuvent plus diverger.
- **Dix tests nés dans `test_text_layout`** (`TopBarNameTest.*`, largeurs réelles d'Oswald 30 px, taille 2 de `ThreeRealFonts` comme `GfxScreen`) : la colonne (119 et 330 px), un nom qui tient (« quiz_buzzer_01 », « Jean-Baptiste Dupont-Martin » à 310 px), un nom vide, un nom qui tient pile et un nom d'un pixel de trop (« Camille » à 81 px, dalles de 301 et 299 px), le plus long début (« Marie-Charlotte Delacroix d… »), une limite incluse (un début avec « … » de 330 px exactement), une coupure juste après une espace (« Jean-Baptiste Alexandre Du … », 321 px, le « W » suivant porte à 343 px), un « É » jamais coupé en deux, un « — » de trois octets écarté en entier. Rouges d'abord pour la bonne raison (sur un squelette qui rend le nom tel quel) : 6 des 10 ; **les 4 autres sont nés verts** (nom qui tient, nom vide, nom qui tient pile, et le « — », écrit après un mutant survivant) et ont été mutés.

**Choix de Claude, à relire en PR** (aucun ne vient du CDC) —
- **Division entière de la colonne** : « ÷ 2 » du CA ne dit pas si l'on arrondit ; `(800 − 119) / 2 = 340` (680,5 arrondi vers le bas), colonne de 330 px. Observable à un pixel près pour un nom dont le début avec « … » mesure 331 px. Écarté : arrondir vers le haut (331 px ; un mutant le fait échouer, test de la colonne).
- **Nommage et emplacement** : `TopBarName` (espace de noms, comme `GlyphFilter` et `Utf8`), dans `lib/rendering/` ; les tests dans `test_text_layout` plutôt que dans un répertoire dédié, pour réutiliser `RealFont30` / `ThreeRealFonts` sans les dupliquer. Non observable. Écarté : un répertoire `test_top_bar_name` (copie des helpers de police).
- **Cas que le CDC (#83) laisse non décidés, non figés par un test** : (1) un nom dont aucun caractère ne tient avec « … » rend « … » seul (la règle « plus long début » avec un début vide), même si « … » dépasse la colonne : impossible à 800 px (« … » mesure 17 px pour 330 px) ; (2) un nom qui commence par des espaces : la règle s'applique telle quelle, sans cas particulier (les espaces de tête sont gardées, tronquées ou non) ; (3) un code que la police n'a pas : il est mesuré comme un `?` (`FontMetrics::Advance`), jamais plus étroit que ce que LovyanGFX dessine, donc la coupe se fait plus tôt, jamais plus tard ; le nom n'est pas passé par `GlyphFilter` (le CDC ne dit pas que le nom l'est : US-FW-008 CA-11 parle des textes de quiz). Aucun de ces trois cas n'est atteignable avec `quiz_buzzer_NN`.
- **Les largeurs des tests** (119, 310, 321, 343, 331 px…) viennent d'une sonde jetable construite sur la fixture `OswaldRegular30Advances.h`, pas d'un calcul à la main ; la sonde n'est pas versionnée.

**Alternatives écartées** — tronquer dans `ScreenPresenter` / `TopBar` (le nom du modèle resterait entier, le bandeau retenu aussi : la largeur dépend de la police, un fait de l'adaptateur) ; tronquer dans `TextLayout` (il coupe aux espaces en plusieurs lignes, ici une seule ligne) ; retirer l'espace avant « … » (écarté par la décision #83).

**Conséquences** —
- Code : `lib/rendering/TopBarName.h` (nouveau) et `src/GfxScreen.h` (`DrawTopBar`). Tests : 832 → 842 (`test_text_layout` 75 → 85).
- Specs : US-FW-009 CA-5 « implémenté, testé en natif ; non constaté sur la dalle » ; statut de l'US inchangé (« Partielle » : CA-2, CA-4, CA-6 et CA-8).
- Dettes : « Bandeau du haut : nom du participant, troncature, habillage et écran de refus » (la troncature n'est plus une dette de code ; son rendu n'est pas constaté) et « Bandeau : preuves manquantes » tenues à jour.
- **Constat reporté (choix du temps (b), acté avec Olivier)** : le smoke test se fait contre le vrai hub, qui n'envoie que `quiz_buzzer_NN` ; seul le cas « le nom tient, affiché entier » y sera observé. La troncature à l'écran reste **non constatée** ; aucun constat n'est inventé.
- Vérifications : suites compilées à la main (g++, C++17, googletest 1.14 et ArduinoJson 7.4.3 clonés hors dépôt) : 28 suites vertes, 842 tests ; 7 mutants, 7 tués (un survivant au premier passage : la coupure à l'octet ; le test du « — » l'a tué). `pio test -e native` : le registre PlatformIO est bloqué ici (installation de la plateforme `native` refusée) ; `pio run -e esp32` impossible ; la CI de la PR fait foi.
