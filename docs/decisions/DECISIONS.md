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
