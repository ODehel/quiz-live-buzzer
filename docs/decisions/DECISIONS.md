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
