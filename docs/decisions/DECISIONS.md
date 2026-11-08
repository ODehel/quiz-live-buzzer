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
- `DisplayState::HighlightedZoneFor(message)` donne l'indice (0 à 3) pour un `question_result` dont `correct_answer` est une lettre A–D, rien sinon. Il reste sans état.
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

