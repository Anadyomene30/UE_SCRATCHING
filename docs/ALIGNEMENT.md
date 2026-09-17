# Alignement — le relevé

**Maison lue jusqu'à : ba1e9ba (2026-09-15)**

*C'est le marqueur que `Suite 360\tools\depuis.py` lit pour dire ce qui a changé
dans la maison depuis la dernière lecture de ce dépôt. Il s'écrit en finissant, et
sans lui l'outil ne devine pas : il le dit, et il s'arrête là.*

> **Lecture du 2026-09-17, pendant l'audit d'ergonomie.** Les trente-six commits
> de `d9fa7be..ba1e9ba` ont été lus. Trois touchent ce dépôt et **aucun n'a été
> appliqué** : la session était un relevé, pas un alignement.
>
> 1. **`780e8a0` — la touche `D` est réservée** (masquer l'interface) pour tout
>    produit qui porte un châssis. Ce produit lie quatre touches — `Espace`,
>    `F`, `B`, `F1`, plus `?` lu dans la file de caractères — et **pas `D`**
>    (`ui/panels.cpp:2162, 4778, 4779, 4895`). L'écart est réel et date du
>    2026-09-10.
> 2. **`0891328` — `design/SCENE.md` porte une contradiction non tranchée** :
>    `Espace` et `Échap` y sont dits réservés « intégralement » à la scène, et
>    ailleurs candidats libres pour la ligne scène. Ce produit lie `Espace` et
>    ne lie pas `Échap`. La question appartient à la maison.
> 3. **`68c7469` — un défaut de livraison est signalé** : `distribute.py`
>    descend `DMMono-Regular/Medium.ttf` sans fichier de licence OFL, alors que
>    ce produit est GPL-3 et destiné à la vente.
>
> Et une lecture rectifiée en cours de session, notée parce que l'erreur est
> instructive : `SCRATCHVJ-33` **existe** — c'est la déclaration des PDF
> régénérés, ici même (`docs/REMONTEES.md:2079`). Ce qui manque est de l'autre
> côté : `Suite 360emonteesiloscope.md` passe de 32 à 34, donc la maison
> n'a jamais écrit le « pris acte ». `depuis.py` a raison de la compter comme
> en attente. Chercher une ligne dans le fichier des **verdicts** et conclure
> qu'elle n'a pas été **émise** confond les deux moitiés du canal.

Relevé le **2026-09-09**, en lecture seule, branche `claude/scratch-video-unreal-0oi7dv`
à **`75eee2f`**. Ce qui monte d'ici est dans
[`REMONTEES.md`](REMONTEES.md).

**Ce document a cessé d'être un relevé de phase 0 le 2026-09-10.** Les verdicts du
tour 01 ont été appliqués, puis les phases 1 à 6 du talon, et chaque ligne porte
désormais un état terminal. L'ordre de lecture est celui du travail : les verdicts
d'abord, puis une section par phase, puis le relevé lui-même — qui garde ses
`fichier:ligne`, parce qu'il décrit un état à une date et qu'il porte cette date.
Les numéros de ligne du code sont ceux du 2026-09-09 et ne sont plus vrais ; c'est
ce qu'un relevé daté est.

| Phase | Ce qu'elle a fait | Commit |
|---|---|---|
| 0 | le relevé, onze axes, sept variables du cadran | — |
| 1 | `docs/manifeste.md` | `2e4164c` |
| 2 | le vocabulaire, et la migration de `library.json` | `ef3cbbc` |
| 3 | `docs/repere.md` — écrit, pas tranché | `9f1bc14` |
| 4 | le noyau du clavier, `Échap` compris | `9b88327` |
| 5 | le fichier unique de jetons | `c4fa93c` |
| 6 | la licence dans les deux sens | `4670820` |
| 7 | **rendue par la note** (`SCRATCHVJ-17`) ; reste *remonté* jusqu'à ce que `suite.css` porte le papier sombre | — |

## Application des verdicts du tour 01 — 2026-09-10

Les vingt verdicts (`Suite 360\remontees\filoscope.md`, lus sur place, rendus le 2026-09-09) ont été
appliqués le 2026-09-10, sur la même branche, à partir de `9ea2eba` — l'état
avant application. Chaque ligne du relevé qui portait « remonté SCRATCHVJ-NN »
porte désormais l'état qui en résulte ; ce tableau en est l'index. Un seul état
s'ajoute aux trois du protocole, et il existait déjà dans ce relevé : **ouvert
(phase N) — verdict reçu**, pour une question répondue dont le travail appartient
à une phase que le talon n'a pas encore ouverte. Cette session a fait la phase 1
et rien d'autre ; elle n'a pas pris les phases 2, 4 et 5 sous prétexte qu'un
verdict les débloquait.

| # | Verdict | Ce que ça fait dans ce dépôt | État des lignes | Commit |
|---|---|---|---|---|
| 01 | tranché, source modifiée | `"auto"` devient l'absence de clé, même migration que `equirect` → `equirect_360` | fermé | `ef3cbbc` |
| 02 | tranché, source modifiée | `rectilinear`, `little_planet`, `fisheye_view` | fermé | `ef3cbbc` |
| 03 | tranché, source modifiée | « 360 » là où 44 px ne tiennent pas plus ; `2D` et « équirectangulaire » seul disparaissent | fermé — le mot est celui de la table, et `SCRATCHVJ-22` l'a confirmé | `ef3cbbc` |
| 04 | tranché, source modifiée | corps 15 px, 11,5 arrondi au cran voisin, échelle candidate | fermé | `c4fa93c` |
| 05 | reporté | nommer les trois étages dans le fichier de jetons, sans valeurs | remonté | — |
| 06 | reporté | rien ; isoler la mono et le fond en une ligne chacune en phase 5 | remonté | — |
| 07 | tranché, source modifiée | fichier de jetons en tête « ligne scène, tokens.json v3, lines.scene » | fermé ; le second orange documentaire : phase 7 | `c4fa93c` |
| 08 | tranché | l'ambre au deck A ; « en cours » à la craie | fermé | `3a278f2` |
| 09 | tranché, source modifiée | `kWarn` `#9C774E` dérivé et donné au lien de platine qui faiblit | fermé ; la valeur monte, `SCRATCHVJ-21` | `3a278f2` |
| 10 | tranché, source modifiée | « A » / « B » gardés ; les phrases à la craie ; capuchon 2 px + nom en craie | fermé | `3a278f2` |
| 11 | tranché, source modifiée | l'aplat porte un état ; quatre boutons d'action perdent le remplissage | fermé | `41ca9c5` |
| 12 | tranché, source modifiée | `kControlRadius` 3, d'après `rhythm.radius_control` | fermé | `d138ac3` |
| 13 | tranché, source modifiée | capitale initiale sur les 25 libellés | fermé | `ef3cbbc` |
| 14 | reporté | les angles à une décimale, signe explicite ; `mm:ss.d` reste | angles fermés ; le temps remonté | `b025d6a` |
| 15 | tranché, source modifiée | `Ctrl`+`Z` sans objet ; `Tab` sur `F` ; les pads `1`–`5` corrigés dans les sources ; `Échap` : le rang qui quitte se supprime | fermé | `9b88327` |
| 16 | tranché, source modifiée | tangage borné à ±89,9° (`kPitchClampDeg`) ; aucun HUD | fermé ; le champ par défaut → `SCRATCHVJ-25` | `b025d6a` |
| 17 | reporté | rien ; `_style.css` ne se réécrit pas | remonté | — |
| 18 | tranché, source modifiée | rien : G7, un rang absent ne dégrade rien | fermé | — |
| 19 | tranché, source modifiée | rien : le nom de travail reste interne | fermé | — |
| 20 | tranché, source modifiée | rien sur 1–3 ; le renvoi de `design/README.md` en phase 5 | fermé | `c4fa93c` |

**Ce que ça change à l'écran.** Les contrôles ont un rayon de 3 px au lieu de
1 ; quatre boutons d'action ne sont plus orange ; le nom du produit est en craie
derrière un filet d'accent de 2 px ; les phrases d'état (analyse, plateau,
Spout, apprentissage MIDI, correspondances d'effets) sont en craie, plus en
ambre ni en vert ; le voyant et la figure du lien de platine qui faiblit sont
`warn` et non ambre ; les angles s'écrivent `+12.5°` ; le tangage s'arrête à
89,9°.

**Ce que l'application a révélé, et qui monte au tour 02**
(`REMONTEES.md`, « Tour 02 ») :

1. la valeur chaude de `warn`, produite par dérivation — et sa teinte à 4° de
   l'accent candidat (`SCRATCHVJ-21`) ;
2. « Plate » (`SCENE.md`, verdict 03) contre « Rectiligne » (table
   d'`ERGONOMIE.md`, `LACUNA-17`), le même jour — la phase 2 ne peut pas choisir
   le mot (`SCRATCHVJ-22`) ;
3. l'ambre résiduel : des nombres colorés hors tolérance, et quatre états de
   dérive portés par un point ou une barre (`SCRATCHVJ-23`) ;
4. « Deck A » / « Deck B » colorés sur des boutons, et le vert de `done` employé
   comme identité de l'incrustation (`SCRATCHVJ-24`) ;
5. le champ par défaut sous `viewer.plate` dans `tokens.json` v3, sous
   `viewer.geometry` dans `SCENE.md` et `00-vocabulaire.md` (`SCRATCHVJ-25`) ;
6. `MAISON.md` régénéré le 2026-09-10 exige encore `Ctrl`+`Z` et attend encore
   `tokens.json` v3 (`SCRATCHVJ-26`).

Et deux comptes de ce relevé à corriger sur pièce, sans remontée : le § 6.6
annonçait **24** sites de texte coloré et sa propre table en sommait 26 ; à
l'application, neuf sites de plus ont été trouvés (`share_open`, `connecté`,
`moments lus`, le nom d'écran de sortie, le message des préréglages, la cible de
la file, « régler », `TRANSFORM`, le mot d'état du lien) et traités par la même
règle. Le compte juste est celui que donne
`grep -c 'text_c(k\(Amber\|Sage\|Slate\|Accent\)' scratchvj/ui/panels.cpp`, pas
celui d'un relevé.

**Les tests.** `cmake --build build-ui --config Release` : zéro avertissement ;
`ctest --test-dir build-ui -C Release --output-on-failure` : 6 / 6, les cinq
contrôles de shader compris ; `cmake --build build --config Debug` puis
`ctest --test-dir build -C Debug --output-on-failure` : `scratchvj_core_tests`
passe. **Aucune migration** d'un fichier écrit sur disque dans cette
application — donc aucun fichier réel à nommer ; la première sera celle de
`library.json` en phase 2.

**Phase 1.** `docs/manifeste.md` existe : les deux principes du README, cités
dans ses termes, et la version française que `CLAUDE.md` portait déjà.

---

## Application des verdicts des tours 02 et 03 — 2026-09-10

Neuf verdicts (`Suite 360\remontees\filoscope.md`, lus sur place, section
« Tours 02 et 03 »), plus six décisions rendues ailleurs qui touchent ce dépôt.
**Rien n'a été rejugé.** Deux points seulement n'ont pas été appliqués tels quels,
et les deux sont écrits en bas de cette section avec leur mesure.

**Le commit de toutes les lignes fermées ci-dessous est celui qui porte cette
section** — *« Carry the verdicts of rounds 02 and 03 »*, 2026-09-10. Il est
cité par son titre et non par son empreinte : un commit ne peut pas porter la
sienne, puisque c'est lui qui modifie le relevé où la ligne vit
(`maison/03-DEFINITION-DE-FINI.md`, 2026-09-10). Il est dit une fois ici plutôt
qu'à chaque ligne du tableau.

Deux choses ont changé dans la maison avant même les verdicts : **le nom est
arrêté — Filoscope** — et `MAISON.md` n'existe plus, ni ici ni là-bas. Ce qui
était propre à ce dépôt se lit dans `maison/produits/filoscope.md`, sur place ;
les renvois `MAISON.md:NN` de ce relevé sont ceux d'un document daté qui n'existe
plus, et ils sont laissés tels quels pour la même raison que les numéros de ligne
du code — un relevé décrit un état à une date.

| # | Verdict | Ce que ça fait dans ce dépôt | État |
|---|---|---|---|
| 21 | tranché, source modifiée | `#9C774E` est entré tel quel dans `tokens.json` v5 avec la règle de dérivation ; la constante reste en partie 2, l'outil ne la nomme pas encore | fermé côté produit ; l'outil est signalé plus bas |
| 22 | tranché, source modifiée | rien à écrire : le mot appliqué en phase 2 est celui que la maison a retenu, et `SCENE.md` ne porte plus « Plate » | fermé |
| 23 | tranché, source modifiée | Q1 : les quatre nombres colorés reviennent à la craie, `out_of_tolerance()` et `link_colour()` disparaissent. Q2 : les quatre autres états de dérive étaient déjà en `warn` | fermé |
| 24 | tranché sur Q1 ; tranché et reporté en deux moitiés sur Q2 | Q1 : rien, la clause tenait déjà par son critère. Q2 : les cinq emplois du vert de `done` sur l'incrustation passent à la craie | fermé sur ce qui est tranché ; la troisième valeur de la paire n'est pas inventée |
| 25 | tranché, source modifiée | `core/sphere.h` cite `lines.scene.viewer.plate.fov_deg_default` pour le champ au repos, et dit que `viewer.geometry` ne porte que la borne de tangage ; 90° était déjà la valeur | fermé ; les bornes 20–170 restent ouvertes, et le commentaire dit où elles vont |
| 26 | tranché, source modifiée | rien à écrire : les deux lignes du § 5.3 passent en *hors périmètre* avec la citation de la fiche | hors périmètre |
| 27 | tranché, source modifiée | les trois libellés du sélecteur de vue prennent leur forme construite ; les angles passent sous la ligne | fermé |
| 28 | tranché, source modifiée | rien : la migration est vérifiée, elle ne touche que les trois noms que le produit fabrique | fermé |
| 29 | tranché, source modifiée | l'appui d'un aplat s'éclaircit vers la craie ; `tok::lift()` est l'opération de `tok::veil()` dans l'autre sens | fermé |

**Ce que ça change à l'écran.** Quatre nombres de mesure — balance en dB, erreur
de phase, vitesse de plateau, confiance du décodeur — ne changent plus de
couleur ; la figure de Lissajous, le voyant de liaison et le mot les doublent
comme avant. L'incrustation se nomme en craie aux cinq endroits où elle portait
le vert de `done`, y compris les deux sélecteurs de sa ligne. Un bouton en aplat
s'éclaircit sous le doigt au lieu de virer à l'ambre. Le sélecteur de vue 360
écrit « Vue rectiligne · Vue little planet · Vue fisheye », et les angles sont
passés sous la ligne, faute de place — vérifié sur une capture, pas raisonné :
`pwsh tools/shot.ps1 -Arguments "clips/tokyo_test_360.mp4" -Out captures/x.png -Wait 16`
(`captures/` est ignoré par git ; la commande, elle, ne se périme pas).

**Les six décisions rendues ailleurs.**

| Décision | État ici |
|---|---|
| la pile d'`Échap` a sept rangs (`LACUNA-MOTION-25`, `RELIEF-37`) | **sans objet** — ce produit ne traite plus `Échap` du tout, c'est le rang 6 ; le seul rang qui aurait un objet est le popup ouvert, et ImGui le ferme |
| `viewer.grid` entre dans `tokens.json` (`VIGIE-36`) | **sans objet** — aucune grille de repérage n'est dessinée |
| le pas de base d'un champ à gratter est un millième de la plage (`VIGIE-37`) | **noté** — Dear ImGui ne produit pas de modificateur de pas sur un `SliderFloat` ; le toolkit qui ne sait pas le note et passe, ce qui est le cas écrit dans le verdict |
| les fontes ont une source unique et un mécanisme (`VIGIE-40`) | **appliqué, et la première mesure était fausse** — voir ci-dessous ; les deux familles y sont, et ce qui manquait était un drapeau de `build.py` |
| `print-color-adjust: exact` (`ANAMORPHE-28`) | **sans objet** — `docs/pdf/_style.css` ne l'a jamais porté ; il est dans `docs/pdf/suite.css`, que ces deux documents ne chargent pas encore |
| une infobulle est admise sur un contrôle muet ou pour nommer un raccourci (`MUTOSCOPE-33`) | **noté** — rien à changer, les infobulles de ce produit sont déjà de ces deux formes |

### Les deux points qui n'ont pas été appliqués tels quels

**1. `signal.warn` reste en partie 2 du fichier de jetons, et c'est l'outil.**
La valeur est entrée dans la source, à l'octet près, et le verdict demande de
vérifier que l'outil l'émet. Il ne l'émet pas : la table `noms` de
`Suite 360\tools\distribute.py` descend `signal.done` et `signal.fail` pour ce
produit, et rien d'autre du bloc `signal`. La constante reste donc déclarée après
le marqueur de fin, avec la valeur de la source et la mention de ce défaut, comme
le verdict le demande dans ce cas. Le jour où l'outil la nomme, cette déclaration
s'efface et le bloc généré la porte.

**2. Les mots des deux documents sont en Archivo — et la première mesure de
cette session était fausse.** `docs/pdf/fonts-inline.css` est chargé par
`manuel.html` et par `argumentaire.html`, avant la feuille, comme `VIGIE-40` le
demande. La première vérification a conclu qu'Archivo n'arrivait pas dans le PDF,
et une remontée est partie sur cette base. **Elle était fausse, et la maison
l'avait déjà dit** (`maison/03-DEFINITION-DE-FINI.md`, encadré ajouté le
2026-09-10 sur `LACUNA-29` et `DIORAMA-16`) : un moteur de navigateur émet une
instance de fonte **variable** en `/Subtype /Type3`, sans `/BaseFont`. Chercher
« Archivo » dans les octets répond *absent* alors que la fonte est là, et le
piège est asymétrique — Fragment Mono n'est pas variable, donc elle sort en
`/BaseFont` bien visible. La leçon est plus utile que le résultat : **« ouvre »
est le mot ; ce qui se vérifie sans ouvrir, c'est l'ABSENCE d'un repli.**

**Ce que la mesure refaite a trouvé, et c'était un vrai défaut, ici.**
`docs/pdf/build.py` imprimait dès que Chrome croyait la page prête. Les
`@font-face` de `fonts-inline.css` sont en `font-display: block`, ce qui rend le
texte **invisible** pendant la période de blocage au lieu de le composer dans un
repli : le PDF sortait **sans son corps de texte, et sans aucune erreur** —
l'argumentaire à 14 pages et 65 Ko au lieu de 326. Un drapeau le répare,
`--virtual-time-budget`, et il est dans `build.py` avec sa raison. C'est le seul
changement de cette session sur la chaîne PDF.

| | Manuel | Argumentaire |
|---|---|---|
| objets `Type3` — les instances d'Archivo | 27 | 20 |
| `DMMono` en `/BaseFont` | oui | oui |
| `Times`, `Arial` | aucun | aucun |
| `SegoeUI` | 6 occurrences | aucune |

Les six `SegoeUI` du manuel ne sont pas des mots : ce sont **six caractères** —
`←` `→` `↔` `√` `≥` `ᵉ` — hors des deux `unicode-range` des sous-ensembles de la
maison. **Ce point monte** (`REMONTEES.md`, `SCRATCHVJ-32`), et il est ce qui
reste d'une remontée qui, telle qu'elle était partie, n'avait pas lieu d'être :
`SCRATCHVJ-30` est réécrite en **déclaration** — le drapeau qui manquait, et à
quoi ressemble une chaîne d'impression qui n'attend pas ses fontes.

> *Ce tableau est celui du 2026-09-10 après-midi. **Cinq des six caractères ont
> été réparés le soir même** par le re-sous-ensemblage d'Archivo (`SCRATCHVJ-32`,
> tranché) ; le tableau à jour est dans la section du tour 04, ci-dessous.*

*La réserve DM Mono contre Fragment Mono pour la ligne scène n'est pas touchée :
les deux passent le moteur d'impression, donc cette mesure ne la tranche pas.*

**Les tests.** `cmake --build build-ui --config Release` : zéro avertissement ;
`ctest --test-dir build-ui -C Release` : 6 / 6, les cinq contrôles de shader
compris ; `scratchvj_tests` : 607 passés, 0 échoué. Aucun fichier écrit sur
disque n'est touché par cette session.

---

## Application des verdicts du tour 04 — 2026-09-10, le soir

Trois lignes étaient parties, trois verdicts sont rendus
(`Suite 360/remontees/filoscope.md`, « Tour 04 »). **Le commit de toutes les
lignes fermées ci-dessous est celui qui porte cette section** — *« Regenerate the
PDFs on the widened Archivo, and name the two characters still in fallback »*,
2026-09-10 — cité par son titre, un commit ne pouvant pas porter sa propre
empreinte.

| # | Verdict | Ce que ça fait dans ce dépôt | État |
|---|---|---|---|
| SCRATCHVJ-30 | **pris acte** — « la déclaration la plus utile de la journée » | Rien à faire. L'avertissement est passé dans `Suite 360/tools/fonts_inline.py` et **descend dans les neuf avec le fichier** ; le réglage reste `font-display: block`, « c'est l'attente qui manquait, pas le réglage », et `docs/pdf/build.py` porte déjà cette attente | **fermé** |
| SCRATCHVJ-31 | **pris acte** | Rien à faire. `tok::lift(jeton, fraction)` est « exactement ce que le verdict décrivait sans le nommer » ; la lecture des angles passée sous la ligne est « le bon arbitrage » et reste telle quelle | **fermé** |
| SCRATCHVJ-32 | **tranché, source modifiée** — cinq des six réparés | Archivo re-sous-ensemblée (`1788f09`) ; `docs/pdf/fonts-inline.css` reçu à jour (`3b6fb5c`) ; les deux PDF régénérés et le balayage refait, ci-dessous. Le sixième, `ᵉ` (U+1D49), **reste en repli et c'est attendu** : Archivo ne l'a pas | **fermé** |

### Le balayage refait, et ce qu'il donne

```
python docs/pdf/build.py
```

| | Manuel | Argumentaire |
|---|---|---|
| poids avant | 1 246 Ko | 579 Ko |
| **poids après** | **1 280 Ko** | **603 Ko** |
| objets `Type3` — les instances d'Archivo | 47 | 38 |
| `DMMono` en `/BaseFont` | oui | oui |
| `Times`, `Arial` | aucun | aucun |
| `SegoeUI` | **1 caractère** | aucun |
| `Consolas` | **1 caractère** | aucun |

*Le poids est regardé en premier, et c'est la leçon de `SCRATCHVJ-30` retournée
contre soi : un PDF dont le corps de texte manque ne dit rien, il pèse seulement
65 Ko au lieu de 326. Les deux poids **augmentent**, ce qui est la signature d'un
sous-ensemble élargi et non d'une page vide.*

**Les cinq caractères réparés, un par un.** Ils ne sont plus cherchés par leur
fonte mais par leur point de code, en dépouillant la table `/ToUnicode` de chaque
objet de fonte du PDF — ce qui dit *exactement* quelle fonte compose quel
caractère, au lieu de compter des octets :

| Caractère | Point de code | Avant | Après |
|---|---|---|---|
| `←` | U+2190 | Segoe UI | **Archivo** (Type3) |
| `→` | U+2192 | Segoe UI | **Archivo** (Type3) |
| `↔` | U+2194 | Segoe UI | **Archivo** (Type3) |
| `√` | U+221A | Segoe UI | **Archivo** (Type3) |
| `≥` | U+2265 | Segoe UI | **Archivo** (Type3) |
| `ᵉ` | U+1D49 | Segoe UI | Segoe UI — **attendu**, Archivo ne l'a pas |

`π` (U+03C0) n'apparaît dans **aucun** des deux documents : la réserve du verdict
sur `π` côté Fragment Mono ne porte pas ici, ce dépôt composant sa chasse fixe en
DM Mono.

**Et un sixième caractère qui n'avait jamais été vu**, parce qu'on ne le
cherchait pas : `₀` (U+2080, indice zéro), dans `pos = origine + taux·(t − t₀)`
au chapitre du repère temporel. Il sort en **Consolas**, pas en Segoe UI. Le
contrôle mécanique que la maison nomme — *« que `SegoeUI` ou `Times`
n'apparaisse PAS »* — ne l'aurait jamais attrapé, et c'est ce que le tour 05
remonte (`SCRATCHVJ-34`). Le caractère lui-même n'est pas réparé ici : la plage
des sous-ensembles est une décision de maison, et `≥` vient d'en administrer la
preuve.

*La réserve DM Mono contre Fragment Mono pour la ligne scène n'est toujours pas
tranchée par cette mesure — mais l'argument qui la motivait vient de tomber, et
c'est `SCRATCHVJ-35`.*

**Les tests.** `cmake --build build-ui --config Release` : zéro avertissement.
`ctest --test-dir build-ui -C Release --output-on-failure` : **6 / 6**, les cinq
contrôles de shader compris.

---

## Phase 2 — le vocabulaire · 2026-09-10 · `ef3cbbc`

Les deux moitiés que le talon demande : les formes canoniques de la 360 partout
où une projection ou une disposition est écrite, et `docs/vocabulaire.md` pour
les mots propres à la scène.

| Verdict | Ce qui a été fait | État |
|---|---|---|
| 01 | `equirect` → `equirect_360` ; `"auto"` cesse de s'écrire, la clé s'omet ; les deux anciennes formes se lisent encore | fermé |
| 02 | `Projection::{Rectilinear, LittlePlanet, FisheyeView}`, et les trois libellés | fermé ; les libellés FR des deux dernières → `SCRATCHVJ-27` |
| 03 | « 360 » et « Rectiligne » ; `2D` et « équirectangulaire » seul ont disparu, console et nom de caisse compris | fermé, et `SCRATCHVJ-22` l'a confirmé |
| 13 | les 25 libellés de paramètre en capitale initiale | fermé |
| — | les états d'analyse aux mots canoniques d'`ERGONOMIE.md` | fermé |
| — | `docs/vocabulaire.md` | fermé |

**`SCRATCHVJ-22` : le mot employé est « Rectiligne ».** La question posée au tour 02
opposait « Plate » (`SCENE.md`, et le verdict 03 qui l'a produite) à
« Rectiligne » (la table du langage d'`ERGONOMIE.md`, sur `LACUNA-17`), écrits le
même jour. Le mot appliqué ici est **celui de la table**, pour la raison que la
table donne elle-même : « une plate en projection Plate » n'est lisible ni à
l'écrit ni à l'oral, et la table est la source des deux colonnes. **La ligne est
fermée le 2026-09-10** : la maison a tranché dans ce sens et sur ces arguments,
`design/SCENE.md` ne porte plus « Plate », et la source écrit pourquoi — « **un
cadran re-règle des valeurs ; il n'ouvre pas une seconde table de libellés** ».

Conséquence de forme, notée parce qu'elle n'était pas prévue : « Rectiligne »
tient partout où « 2D » tenait — les sélecteurs de ce produit se dimensionnent
sur leur texte. Il n'y a donc **aucune troncature à demander** ; la seule forme
courte employée est « 360 » pour « Équirectangulaire 360 », que
`ERGONOMIE.md` donne en exemple.

**La migration, vérifiée sur de vrais fichiers.** `library.json` est le premier
fichier écrit sur disque que ce produit migre. Le lecteur accepte encore `"auto"`
et `"equirect"`, l'écriture ne produit plus que `flat` et `equirect_360` — et
l'absence de clé pour ce qui n'est pas forcé. Le lecteur **dit** qu'il a dû
accepter une ancienne forme (`LibraryFile::migrated_on_read`), et l'interface
enregistre une fois au démarrage quand c'est le cas : sans cela un fichier
ancien serait resté dans l'ancienne forme jusqu'à la première modification d'un
set.

Quatre fichiers réels, dérivés du `library.json` de cette machine (huit clips,
dont trois locaux et cinq hors du dépôt), lancés dans
`build-ui\scratchvj\ui\Release\scratchvj_ui.exe --screen bibliotheque` :

| Fichier | Ce qu'il contient | Ce qui en est sorti |
|---|---|---|
| `library-01-ancien.json` | l'ancienne forme, avec **les trois valeurs** `auto`, `flat`, `equirect`, et trois caisses dont `360°` et `2D` | réécrit : `flat`, `equirect_360`, clé absente pour `auto` ; caisses « Équirectangulaire 360 », « Rectiligne », « Set 12 sept. » intacte |
| `library-02-nouveau.json` | la nouvelle forme seule | relu et réécrit à l'identique ; aucun changement de forme |
| `library-03-mixte.json` | les deux formes dans le même fichier, plus une clé absente | normalisé ; les huit décisions conservées une à une |
| `library-04-corrompu.json` | `"projection": "sphere"` sur le troisième clip | **refusé en nommant la valeur** ; le fichier est resté tel quel sur le disque, `"sphere"` compris |

Deux tests capturent la même chose dans la suite, pour que la prochaine session
n'ait pas à refaire ces quatre fichiers : *« the old projection words are still
read, the new ones written »* et *« a crate named after a projection takes the
long form »* (`scratchvj/tests/test_library_io.cpp`).

**Ce que la phase a révélé.** Deux points montent au tour 03 : `SCRATCHVJ-27`
(les libellés FR de `little_planet` et `fisheye_view` n'existent dans aucune
table, et « Fisheye » désigne déjà une projection de fichier) et `SCRATCHVJ-28`
(un nom de caisse est un libellé que l'utilisateur peut changer *et* une chaîne
écrite sur disque ; ce dépôt a dû décider seul ce que la migration touche).

---

## Phase 3 — le repère temporel · 2026-09-10 · `9f1bc14`

**Écrit, pas tranché.** `docs/repere.md` existe : la question en une phrase, les
trois candidats tels qu'ils sont dans le code, les trois endroits où ils se
rencontrent déjà, trois options avec leurs conséquences, une recommandation
argumentée, et la liste de ce qui attend la décision. Le choix est au fondateur,
et ce dépôt ne l'a pas pris.

**Ce que la phase a mesuré, et qui change la forme de la question.** Le tempo est
**une constante** — `kBpm` dans `ui/main_ui`, héritée par tout clip à son
chargement, premier temps à zéro dans chaque grille. Il n'y a ni détection de
tempo, ni saisie, ni tempo par clip. Le tempo ne peut donc pas diverger
aujourd'hui : il ne suit rien. La divergence apparaîtra au premier tempo mesuré,
et c'est pour ça que la règle se laisse écrire maintenant à peu de frais.

**Les trois consommateurs sont nommés**, et ils ne prennent pas le même maître :
la phase d'un LFO synchronisé suit **le plateau** (converti en temps musicaux) ;
la division d'un effet synchronisé suit **le tempo** seul ; le suiveur
d'enveloppe suit **l'horloge**, en secondes. Aucune ligne n'écrivait que c'était
voulu.

**Ce qui reste ouvert, et qui n'appartient pas à ce dépôt** : le choix lui-même,
le nom du deck de référence pour le temps musical, la phrase vérifiable qui dit
ce que fait un backspin, et — par conséquence directe — le format d'affichage
d'un temps, que `SCRATCHVJ-14` a reporté pour cette raison exacte. `mm:ss.d`
reste tel quel et la ligne reste *remontée*.


---

## Phase 4 — le noyau du clavier · 2026-09-10 · `9b88327`

| Ce que le talon demande | État |
|---|---|
| `Espace` | déjà conforme, rien fait |
| la pile d'`Échap` | **fermé** — le gestionnaire ne traite plus `Échap` : c'est le rang 6 |
| `Ctrl`+`Z` | **sans objet**, verdict 15 ; toujours pas liée, et c'est la conformité |
| `?` / `F1` | **fermé** — une carte du clavier |
| `Tab` s'il a un sens | **sans objet** — l'effet est sur `F`, et `Tab` est rouvert par la suite |
| le viseur 360 à la souris | **sans objet** — le regard n'est jamais manipulé à la souris (axe 5.6) |

**`Échap` ne fait plus rien, et c'est la règle.** Les trois rangs que le
gestionnaire SDL portait sont partis, pas seulement celui que le verdict nommait :

- **quitter « image seule »** — `MAISON.md` l'interdit nommément à `Échap` pour ce
  produit (« et ne quitte jamais « image seule » — `F` s'en charge ici ») ;
- **fermer la fenêtre de sortie** — aucun des six rangs de la pile ne porte cet
  effet, et le rang 6 dit alors de ne rien faire. La sortie se ferme depuis
  l'écran SORTIE, qui a déjà le bouton ;
- **quitter l'application** — le verdict, et l'interdit qui n'a pas de source en
  dessous.

Le seul rang qui ait un objet dans ce produit est le rang 1 lu au sens large — un
popup ouvert — et c'est ImGui qui le ferme. L'événement se consomme donc une
fois, ce que le verdict demandait, et il se consomme au premier rang qui
s'applique parce qu'il n'y a plus qu'un traitant. **Rien n'est devenu
inatteignable** : `F` et le basculeur de la barre haute rendent « image seule »,
l'écran SORTIE ferme la sortie, le bouton de légende ferme la fenêtre.

**La carte du clavier.** `?` et `F1` ouvrent un popup non modal qui liste les
cinq gestes — `Espace`, `F`, `B`, `?` / `F1`, `Échap` — et dit que le reste se
joue sur les plateaux et sur la table. Le `?` est lu **dans la file de
caractères** et non sur la touche physique : la suite exige qu'une touche
réservée soit atteignable sur AZERTY comme sur QWERTY sans touche morte, et `?`
est `Maj`+`/` sur l'une, `Maj`+`,` sur l'autre.

**Deux lignes requalifiées plutôt que corrigées**, chacune avec sa raison :

- **la durée `%.1f s`** — la source dit « durée d'un **travail** » ; ces trois
  nombres sont la longueur du tampon de l'anneau live, la grandeur qu'on
  scratche. En `h min s` sans décimale, « 4 s d'historique » ne mesure plus rien ;
- **les trois infobulles de transport** — supprimées, pas gardées. Le glyphe est
  la convention de métier qu'`ERGONOMIE.md` autorise justement pour ces trois
  commandes, et le nommer est le geste que le test de `DIRECTION-ARTISTIQUE.md`
  désigne. Le raccourci, seule information que le glyphe ne portait pas, est
  passé dans la carte.

**Une ligne part au carnet, et c'est la maison qui l'y envoie.** « Aucun
pourcentage seul » est un **ajout** (`SCRATCHVJ-20`) : ni l'étape nommée ni le
temps restant n'existent dans `app/analyze`, et aucune phase du talon ne les
porte. L'entrée est dans `docs/roadmap.md` avec son coût.

**Ce que la phase n'a pas fait, et pourquoi.** Aucune touche n'a été ajoutée au
noyau au-delà de `?` / `F1` : le clavier réservé de la ligne scène attend un
second produit de scène (`SCENE.md`), et `Espace`, `F` et `Échap` y sont notées
comme candidates. Rien n'a été lié « pour se conformer ».


---

## Phase 5 — les jetons · 2026-09-10 · `c4fa93c`

`scratchvj/ui/tokens.h` — **un seul fichier**, provenance et version en tête,
deux parties. Aucune valeur n'est écrite ailleurs dans le code : tout référence
ce fichier par nom de jeton.

**En-tête, à la lettre du verdict 07** : *ligne scène, `design/tokens.json` v3
(`revised` 2026-09-09), bloc `lines.scene` par-dessus l'invariant*. Une copie se
compare par cette ligne, jamais par un `diff` — la source est en CRLF.

| Ce que le talon demande | État |
|---|---|
| le fichier unique extrait d'`apply_style()` | **fermé** — `apply_style()` ne porte plus aucune valeur, seulement des noms |
| provenance en tête | **fermé** — et à la forme du verdict 07, pas à celle du talon (`SCRATCHVJ-26`) |
| valeurs inchangées | **fermé** — une seule valeur bouge, et c'est un verdict : 11,5 → 13 |
| la mono isolée en un endroit | **fermé** — `kMonoFile`, une ligne |
| la luminance isolée en un endroit | **fermé** — sept lignes, les seules du dépôt à porter ces valeurs |

**Les deux fontes.** Archivo pour les mots, DM Mono pour les valeurs, les deux
derrière un nom de jeton. La source prescrit Fragment Mono ; **rien n'a
changé** — c'est de la couche 3, `SCRATCHVJ-06` est reportée, et c'est à ce
produit de fournir la mesure d'impression. Le jour où la maison tranche, c'est
`kMonoFile`, et elle seule.

**L'échelle, verdict 04.** Le corps reste à 15 px, valeur de source. **11,5 px a
disparu** : une taille à virgule est un continuum de un, et l'invariant ferme
l'échelle quelle que soit la ligne. Arrondi au cran voisin du jeu de ce
produit — **13**, premier cran de `scale_candidat` — et le substitut est noté
dans le fichier, comme la maison l'exige. C'est le seul changement de valeur de
la phase, et il se voit : les sur-titres et les libellés de ligne grossissent
d'un cran.

**`warn`, tel que dérivé au verdict 09.** `#9C774E`, avec sa dérivation écrite à
côté d'elle — le `warn` d'atelier à teinte égale, clarté et saturation
multipliées par la moyenne des rapports que la table applique déjà à `done` et à
`fail`. Elle est en partie 2 et déclarée **candidate** : `tokens.json` ne porte
toujours pas `lines.scene.signal.warn`, et sa teinte frôle l'accent
(`SCRATCHVJ-21`). Tant que la maison ne donne pas mieux, c'est celle-là.

**L'ambre résiduel, à la lettre de `SCENE.md`** — et sans trancher
`SCRATCHVJ-23`. La seule chose que la source dise en toutes lettres est que
*l'ambre appartient au deck A*, et que la ligne hérite des **quatre** signaux,
`warn` disant « ça marche encore, mais ça dérive ». Appliqué :

| Site | Ce que c'est | Devient |
|---|---|---|
| la table MIDI partiellement connectée | un voyant | `warn` |
| la figure de Lissajous déséquilibrée | une figure | `warn` |
| la jauge de fraîcheur de l'ancre | une barre | `warn` |
| une liaison activée mais sans valeur | un voyant | `warn` |
| la balance en dB, l'erreur de phase, la vitesse du plateau | **des nombres** | `warn`, par une fonction nommée |

**Ce qui n'est pas tranché, et où il changera.** La question Q1 de
`SCRATCHVJ-23` — *un nombre coloré est-il du texte au sens de la règle du
`fail` ?* — n'a de réponse dans aucune source : la clause couvre une phrase et
une étiquette d'un ou deux caractères, et un nombre n'est ni l'un ni l'autre. Ce
dépôt ne la tranche pas. Les trois nombres passent par une seule fonction,
`out_of_tolerance()`, et le jour où la maison répond « oui, c'est du texte », ils
reviennent à la craie en changeant cette fonction et rien d'autre.

**Les trois étages de la densité, nommés sans valeurs** (verdict 05). Le fichier
de jetons dit quel panneau est à quel étage — ce qui se lit à un mètre d'un coup
d'œil, ce qui se lit en se penchant, ce qui ne se lit pas en jouant — et
**n'invente aucun nombre**. Les dix nombres de rythme d'ImGui, dont `10`, `6` et
`18` qui ne sont pas des multiples de l'unité de 4, sont **nommés et non
régularisés** : régulariser déciderait par la bande ce que la maison doit
trancher au tour 01 d'un second produit de scène.

**Six valeurs de couleur que le relevé de phase 0 n'avait pas comptées.** Il en
annonçait quinze — douze nommées, trois hors table. La transcription en a trouvé
**six de plus**, toutes dans `ui/panels.cpp` : cinq voiles écrits en
`IM_COL32(…, alpha)` — les frames en mémoire sur la bande, l'intervalle bouclé,
la case de banque sous un glisser, la ligne survolée, le masque non armé — et
**un filet de ligne de liste opaque**, `#232220`, qui n'est aucun jeton de
châssis. Les cinq voiles s'expriment désormais **depuis** un jeton par une
fonction `veil()` ; le filet est une valeur propre au produit, nommée et
commentée. Le compte juste est celui que donne
`grep -c 'IM_COL32(0x' scratchvj/ui/panels.cpp`, qui répond **0**, pas celui
d'un relevé.

**La frontière que cette phase a dû tracer, et qui n'est écrite nulle part.**
L'interdit dit « aucune valeur hexadécimale, aucun rayon, aucune taille, aucune
fonte écrite en dur hors du fichier unique de jetons ». Pris à la lettre, toute
largeur de mise en page — un rail de 250 px, une liste de 540 — serait un jeton,
ce qui ferait de ce fichier un fichier de mise en page et non de jetons. La
lecture appliquée ici est celle du vocabulaire de `tokens.json` lui-même : sont
des jetons les **couleurs**, les **fontes et l'échelle**, et les grandeurs que
`rhythm` nomme — unité, gouttière, écart de groupe, ligne de paramètre, barre
haute, rayons, capuchon, cible minimale. Une largeur de composition n'en est
pas une. Écrit ici pour qu'une session suivante ne refasse pas l'arbitrage.

**Ce qui reste ouvert après la phase**, et qui n'appartient pas à ce dépôt : les
sept valeurs de châssis (`SCRATCHVJ-06`), la mono unique, la convention du champ
creusé contre relevé, les cinq crans de l'échelle, les trois étages de la
densité, la valeur chaude de `warn` (`SCRATCHVJ-21`), l'accent du produit — qui
attend son nom. Toutes changent en un endroit chacune, et c'est ce fichier.


---

## Phase 6 — la licence dans les deux sens · 2026-09-10 · `4670820`

**Le premier sens était écrit, le second manquait partout.** Les trois documents
disaient que le binaire est GPL-3 ; aucun ne disait à son lecteur ce que cela lui
donne. C'est cette moitié-là que la phase ajoute, et dans les termes de la
source : *un utilisateur de scène doit savoir qu'il a le droit d'avoir la source —
c'est un argument, pas un aveu.*

| Document | Ce qu'il disait | Ce qu'il dit |
|---|---|---|
| `README.md` | « GPL-3.0 », et la frontière `dvs/` | la même chose, plus le droit à la source, plus ce qui est vendu, plus pourquoi la frontière reste visible dans le code |
| `docs/pdf/manuel.html` | couverture : « la configuration par défaut n'embarque aucun code GPL » | « **GPL-3** — vous avez droit à la source » ; l'annexe E distingue le défaut de **compilation** de la configuration **distribuée** |
| `docs/pdf/argumentaire.html` | « une fois la licence tranchée » | « tranché depuis le 9 septembre 2026 », la voie retenue et sa raison, et ce que l'acheteur y gagne |

**Et rien ne prétend plus vendre autre chose.** Le tableau « Modèles
envisageables » proposait cinq modèles, dont deux qui ne tiennent pas sous
GPL-3 — la licence perpétuelle par poste, qui suppose de pouvoir refuser la
redistribution, et l'édition Pro fermée par-dessus un noyau ouvert. Il est
remplacé par les trois choses que la maison autorise à vendre : **le binaire, les
mises à jour, et que ça marche le soir même**. Les deux modèles qui tombent sont
**nommés** avec la raison, plutôt que retirés en silence.

**Ce qui ne change pas, et qui est demandé par la source.** La frontière de
licence reste une **option de compilation visible dans le code**, et
`dvs/decoder.h` la documente en tête de fichier. Ce n'est plus présenté comme un
moyen de livrer un binaire sans GPL — ça ne l'est pas — mais comme ce qui permet
de répondre en une phrase à « est-ce que votre catalogue contient du GPL ? ».

**Un effet de bord, et c'est une divergence réparée.** Régénérer les deux PDF a
aussi régénéré les annexes du manuel, qui se fabriquent depuis le binaire : le
catalogue imprimé décrivait encore la table à **63 contrôles**, le binaire en
donne **141**. C'est exactement ce que ce mécanisme existe pour empêcher, et le
`README.md` de `docs/pdf/` cesse d'écrire ce nombre — il nomme les deux commandes
qui le donnent.


---

## Définition de fini — l'état au 2026-09-10

Les critères communs aux dix produits sont dans `maison/03-DEFINITION-DE-FINI.md`,
à son adresse. Ceux-ci sont ceux qu'elle demande **de montrer**, plus les quatre
que le talon ajoute pour ce produit.

| Critère | État |
|---|---|
| `ALIGNEMENT.md` existe dans `docs/`, chaque ligne porte un état | **oui** — aucune ligne ne porte plus l'état *ouvert (phase N)* ; les deux occurrences que `grep` en trouve sont celle-ci et la définition de l'état, plus bas |
| chaque phase s'est terminée par un rapport en trois parties | **oui** — une section par phase ci-dessus |
| un seul fichier de jetons, provenance et version en tête | **oui** — `scratchvj/ui/tokens.h` |
| aucune valeur hexadécimale ailleurs dans le code | **oui** — `grep -c "IM_COL32(0x" scratchvj/ui/panels.cpp` répond 0 ; les seules valeurs restantes hors du fichier sont les effacements de vue GPU en noir, qui sont le fond derrière l'image et non une couleur d'interface |
| chaque jeton que le toolkit n'a pas su produire est noté avec son substitut | **oui** — un seul : 11,5 px, arrondi à 13, noté dans le fichier |
| une couleur écrite dans une cible linéaire ou un shader porte sa conversion | **sans objet** — les dix shaders ne portent aucune constante de couleur, et c'est écrit en tête du fichier de jetons |
| aucun libellé n'emploie un mot absent de la table du vocabulaire | **oui** — phase 2 |
| aucune chaîne écrite sur disque n'est autre que la forme canonique | **oui** — phase 2, et la migration est vérifiée sur quatre fichiers réels |
| tout fichier que le produit lisait avant l'alignement se lit encore | **oui** — les deux anciennes formes de `library.json` sont encore lues, vérifié sur un `library.json` réel |
| aucune chaîne affichée n'est écrite dans le code | **non — et c'est la phase de langue**, qui n'est pas un critère d'alignement mais un critère de **vente**, et qui se mesure à part (`FOND-13`). Aucune phase du talon ne la porte ; elle va au carnet |
| les touches réservées liées ont leur effet, et aucun autre | **oui** — `Espace`, et `?` / `F1` depuis la phase 4 ; `F` et `B` sont libres |
| `Échap` suit la pile et ne ferme jamais la fenêtre | **oui** — phase 4, rang 6 |
| un glisser vers la droite fait défiler la scène vers la droite | **sans objet** — le regard n'est jamais manipulé à la souris ; et le critère est de toute façon *suspendu* tant que les viseurs n'ont pas été comparés à l'écran |
| aucune erreur n'ouvre de modale | **oui** — zéro modale dans le produit, mesuré |
| chacune nomme la valeur en cause et l'action qui répare | **oui** — phase 4 |
| aucune progression n'affiche un pourcentage seul | **non — au carnet**, et c'est la maison qui l'y a rangé (`SCRATCHVJ-20`) : c'est un ajout, et aucune phase ne le porte. L'entrée est dans `docs/roadmap.md` avec son coût |
| un produit qui compose un PDF l'**ouvre** et vérifie que les deux familles y sont (`VIGIE-40`) | **oui, avec une réserve nommée** — les mots sont en Archivo, en `/Subtype /Type3` : cette famille-là ne se cherche pas par son nom, et c'est la maison qui l'a mesuré (`maison/03-DEFINITION-DE-FINI.md`, encadré). La chasse fixe de ces deux documents est **DM Mono et non Fragment Mono**, ce qui est une réserve ouverte et non un manquement (`maison/produits/filoscope.md`, « ne rien changer »). Le contrôle mécanique — l'absence d'un repli — est fait au caractère près dans la section du tour 04 |
| les tests existants passent | **oui** — voir ci-dessous |
| ce qui a été corrigé sur la foi d'un écart de `DIVERGENCES.md` est vérifié | **oui** — ce produit n'y figure pas ; les 22 écarts sont confrontés un par un plus bas |

Et les quatre du talon :

| Critère propre à scratchvj | État |
|---|---|
| `docs/manifeste.md`, `docs/vocabulaire.md`, `docs/repere.md` existent ; le troisième expose des options et une recommandation, pas une décision | **oui** |
| l'aplat orange est toujours là, et il n'y en a toujours qu'un par panneau | **oui** — au sens de la plus petite région bordée, fermé en `41ca9c5` |
| aucune couleur, taille ou fonte hors du fichier unique ; la mono et la luminance changent en une ligne chacune | **oui** — `kMonoFile`, et les sept lignes du châssis |
| la suite de tests passe, `*_check` des shaders compris — et le rapport dit **laquelle** | **oui**, ci-dessous |

**Les tests, et laquelle.** `ctest --test-dir build-ui -C Release
--output-on-failure` : six tests, tous passés — `scratchvj_core_tests`,
`gpu_check`, `sphere_check`, `eye_check`, `fx_check`, `taps_check`. C'est celle
qui contient les cinq contrôles de shader. `ctest --test-dir build -C Debug
--output-on-failure` passe aussi (`scratchvj_core_tests` seul). Les deux
compilations sont à **zéro avertissement**. Restent hors suite, comme toujours :
`spout_check` et `net_check`, qui écoutent une application en cours d'exécution,
et `xr_check`, dont le verdict dépend du matériel branché.

**La migration, et sur quels fichiers.** `library.json`, quatre fichiers réels
dérivés de celui de cette machine — ancien avec les trois valeurs
d'énumération, nouveau, mixte, corrompu. Le détail est dans la section de la
phase 2.


---

## Le cadre

**Git.** Le dépôt est sous git, tête `75eee2f`, arbre de travail propre : les
seuls fichiers non suivis sont `MAISON.md`, ce relevé et le fichier de
remontées. Rien à régulariser avant de travailler. *(Signalé parce que
`secteurs.html:671-674` note que trois produits d'atelier et `Suite 360` ne sont
pas sous git ; ce produit-ci l'est entièrement.)*

**Le dossier de documentation.** `git ls-files` ne connaît qu'une casse :
`docs/`. C'est celle qu'écrit `MAISON.md:123`. Aucun écart de casse — le point
tranché par `DIORAMA-10` (le générateur prend désormais le dossier réel de
chaque produit) ne concerne pas ce dépôt, qui est déjà du bon côté.

**Les sources, et leur fraîcheur.** Les neuf sources de `MAISON.md:26-34` ont
été lues en entier, dans l'ordre donné, avant d'ouvrir le code. Six d'entre
elles ont été **révisées le 2026-09-09 après le tour 01 de Diorama** :
`DIRECTION-ARTISTIQUE.md:9-14`, `ERGONOMIE.md:8-11`, `spec/00-vocabulaire.md:10-12`,
`spec/02-repere.md:3-5`, `design/DIVERGENCES.md:6-10`, `docs/suite.css:22-24`.
`remontees/REGISTRE.md` et `remontees/diorama-01-reponses.md` ont été lus
ensuite. `design/NOMS.md` a été lu en plus, pour nommer le fichier de remontées.

`diorama-01-reponses.md:499` dit que le `MAISON.md` de ce produit n'a pas à être
régénéré — « aucune source de la ligne scène n'a changé ». C'est exact au sens
strict : `secteurs.html` n'a pas bougé. Mais `00-vocabulaire.md` et
`02-repere.md`, que `MAISON.md:30-31` met dans la liste de lecture
**obligatoire**, ont bougé — et l'un des deux changements supprime une
contradiction que ce relevé aurait remontée. Voir « Ce que le tour 01 a déjà
tranché ».

**Le nom du produit.** `design/NOMS.md:217-227` arrête huit noms de produits ;
aucune ligne pour `UE_SCRATCHING`. `secteurs.html:320-345` explique pourquoi : le
registre est retenu (les appareils de l'image animée d'avant le cinéma), mais
*Praxinoscope* fait douze lettres et « aucun candidat court n'a été vérifié »
(`:342`). Le fichier de remontées emploie donc le nom de travail. Voir
**SCRATCHVJ-19**.

**Les états portés par chaque ligne.** `MAISON.md:177` en définit trois :
*fermé* (avec le commit), *remonté* (avec l'identifiant), *hors périmètre* (avec
la raison). Ce relevé en emploie **cinq**, et les deux ajouts sont motivés :

- **conforme par le cadran** — la ligne diverge de l'atelier *et c'est ce que
  `secteurs.html` § 5.2 exige d'elle*. Ce n'est pas un écart : c'est une variable
  qui prend sa valeur « scène ». Chaque ligne qui porte cet état cite la ligne de
  `secteurs.html` qui l'autorise. Sans lui, le relevé rangerait en écart ce que
  `secteurs.html:655` interdit nommément de corriger.
- **ouvert (phase N)** — le travail est assigné à une phase de `MAISON.md` et
  n'attend aucune décision extérieure. Marquer « fermé » un travail non fait
  serait faux ; « remonté » une décision déjà prise le serait aussi. Toute ligne
  qui le porte doit atteindre l'un des trois états terminaux avant la fin.

Chemins abrégés : `ui/` = `scratchvj/ui/`, `core/` = `scratchvj/src/core/`,
`app/` = `scratchvj/src/app/`, `config/` = `scratchvj/src/config/`,
`dvs/` = `scratchvj/dvs/`, `unreal/` = `unreal/ScratchLink/`.

---

# Les sept variables du cadran, une par une

C'est le cœur de ce relevé. Pour chaque variable : sa valeur « scène » telle que
`docs/secteurs.html` § 5.2 la donne avec sa ligne, ce que le code fait avec ses
lignes, et si les deux coïncident.

La règle qui gouverne la lecture est à `secteurs.html:386-389` : *« chaque
variable a une raison métier écrite et une valeur par ligne. Si la raison ne peut
pas s'écrire, la variable retourne à l'invariant. »* Et la liste de ce qui
**n'entre pas** dans le cadran est à `:456-460` : la famille de fontes,
l'échelle typographique, le rayon des angles, la règle des filets, la nature des
nombres, les couleurs de signalisation.

## Variable 1 — Distance et corps · **coïncide en partie**

| | |
|---|---|
| Valeur scène | « 1 m dans le noir, corps 15 px, cible 44 px » — `secteurs.html:396` |
| Raison écrite | « on *jette un œil* à la scène pendant qu'une main tient le plateau » — `:397-399` |

Le code :

- **corps 15 px** — `ui/main_ui.cpp:258`, Archivo à `15.0f * dpi` ; `:261`, la
  mono à `15.0f`. Le commentaire `:254-256` donne la mesure : « Sizes read off
  the mockup at its native 1440 width: body 15, labels 11.5, numbers 15. »
  **Coïncide** (`secteurs.html:396`).
- **cible 44 px** — appliquée à **deux** endroits : le bouton lecture/pause
  (`ui/panels.cpp:2096`, largeur `44.0f`) et la cellule de pad (`:2382`,
  `const float pad = 44.0f` ; le nom court d'un pad est coupé « to what fits a
  44 px cell », `:872-878`). Partout ailleurs la hauteur de contrôle est
  **`kControlHeight = 28.0f`** (`ui/panels.cpp:205`), employée par `button()`
  (`:221`), `segmented()` (`:271`), `toggle()` (`:319`) et `row_label()`
  (`:333`) — c'est-à-dire la valeur **atelier** de la même ligne du cadran
  (`secteurs.html:395`, « ligne de paramètre 28 px »).

**Verdict : coïncide, et la source le dit.** Le corps est à la valeur scène ; la
cible de 44 px est là où la main frappe sans regarder (transport, pads). Les 28 px
ne sont pas « l'ancienne valeur » : `tokens.json` v3 garde `rhythm.param_row` à 28
dans l'invariant et ne le redéfinit pas pour la scène, qui ne redéfinit que
`hit_target_min`. Une ligne de paramètre se lit et se vise des yeux ; un bouton de
transport et un pad se frappent à l'aveugle. Les deux valeurs sont désormais
nommées — `kParamRow`, `kHitTargetMin` — et aucune géométrie n'a bougé.
→ **fermé** (`c4fa93c`, phase 5).

Une taille employée n'a la valeur d'aucune des deux lignes : **11,5 px**
(`ui/main_ui.cpp:259`), hors de l'échelle fermée 11/12/13/16/28
(`tokens.json:17`) — et 15 px l'est aussi. Or l'échelle est un invariant
(`secteurs.html:369`) **explicitement exclu du cadran** (`:457`).
→ **fermé** (`c4fa93c`, phase 5, SCRATCHVJ-04) — le corps est à 15 px, valeur de source ; 11,5 est arrondi à **13**, le cran voisin du jeu de ce produit (`scale_candidat`), **substitut noté** dans le fichier de jetons. Les cinq crans restent ouverts côté maison : ce qui manque est la règle de dérivation entre salles.

## Variable 2 — La règle de couleur · **coïncide sur le principe, pas sur le compte**

| | |
|---|---|
| Valeur scène | « **un aplat autorisé** — un seul par panneau, toujours porté par un état, jamais par l'identité » — `secteurs.html:403-404` |
| Raison écrite | « à la scène il faut *voir avant de lire*. L'aplat est le seul signal préattentif dont on dispose » — `:405-408` |
| Démonstration | `:451-454` — « Boucle · **Armé** · Slip » : l'aplat porte un état |

Le code produit l'aplat à deux endroits, et deux seulement :

| Où | Ligne | Ce qui le porte |
|---|---|---|
| `button(..., primary)` | `ui/panels.cpp:229-231` | remplissage `kAccent`, `kAmber` pendant l'appui |
| `segmented(...)` | `ui/panels.cpp:279` | l'option choisie, remplie de l'accent passé |

Aplats portés par un **état** — conformes à `:403-404` :
`ui/panels.cpp:2096` (lecture en cours), `:2338` (boucle active), `:1497` (le
clip est dans la caisse), `:3435` et `:3751` (apprentissage MIDI en écoute), plus
les 17 sélecteurs segmentés (`:1138, 1198, 1472, 2045, 2184, 2217, 2242, 2327,
2478, 3283, 3477, 3764, 3800, 3979, 4454, 4565, 4578`).

Aplats portés par une **action**, pas par un état : `ui/panels.cpp:686`
(« ■ relecture »), `:1013` (« Suivant → »), `:1284` (« Créer »), `:3621`
(« + Liaison »). Le commentaire de `button()` les assume : `:207-208`,
« `primary` fills it with the accent: **one per panel, the thing this panel is
FOR** ».

**Le compte par panneau n'est pas tenu.** `draw_deck()` s'étend de
`ui/panels.cpp:2008` à `:2499` et porte **jusqu'à sept** surfaces remplies
d'accent dans le même panneau : cinq sélecteurs segmentés (`:2045, 2184, 2217,
2242, 2327`) plus deux boutons d'état (`:2096, 2338`).

**Verdict : coïncide sur le principe, pas sur le compte.** L'aplat existe et il
est le plus souvent porté par un état — ce que `MAISON.md:90` demande de ne pas
casser, et ce n'est pas cassé. Mais « un seul par panneau » n'est pas tenu, et
quatre aplats portent une action.

Ce dernier point n'est pas une négligence du code : **les deux documents que
`MAISON.md:90` déclare identiques « mot pour mot » ne disent pas la même
chose.** `secteurs.html:403-404` dit « porté par un état » ;
`design/README.md:33-34` dit « l'orange plein est réservé à une seule action par
panneau ». Le code applique les deux à la fois. → **fermé** (`41ca9c5`, SCRATCHVJ-11) — l'aplat porte un état, jamais une action ; les quatre boutons d'action perdent le remplissage ; un panneau est la plus petite région délimitée par un filet ou un fond propre, et sur ce critère le compte est tenu (`SCENE.md`, variable 2).

## Variable 3 — Le châssis · **coïncide sur la teinte, pas sur la valeur**

| | |
|---|---|
| Valeur scène | « noir chaud `#141412` » — `secteurs.html:412` |
| Contrainte | « **même luminance, teinte seule différente** — deux finitions d'un même matériau, pas deux marques » — `:415-416` |
| Verdict de la source | « Mi-accident : la teinte est du cadran (variable 3), l'écart de *valeur* n'en est pas. Aligner la valeur, garder la teinte » — `:474-476` |

Le code : `kGround = IM_COL32(0x14, 0x14, 0x12, 0xFF)` — `ui/panels.cpp:22`.
La même valeur est écrite **une seconde fois**, dans une autre notation et un
autre fichier : `bgfx::setViewClear(8, …, 0x141412ff, …)` — `ui/main_ui.cpp:227`.

La teinte coïncide exactement. La luminance, non : `#141412` contre `#0A0C0B`
(`tokens.json:6`). Les deux ne sont pas deux finitions d'un même matériau — la
contrainte de `:415-416` n'est tenue par aucune des deux valeurs, et la valeur
commune n'est arrêtée nulle part.

**Verdict : coïncide sur la teinte, ne coïncide pas sur la valeur.** C'est
exactement ce que la source dit d'elle-même. `MAISON.md:99` range l'écart de
valeur en couche 3 et ordonne de ne rien changer ; `secteurs.html:663` demande au
contraire de le corriger « aujourd'hui, en un fichier chacun ». J'ai suivi
`MAISON.md`. → **remonté SCRATCHVJ-06** — *reporté*, et reporté sur la mono, pas sur la luminance. La direction est tranchée : luminance de l'atelier, teinte de la scène, test « luminance relative WCAG à ±5 % du jeton d'atelier correspondant, dérive chaude R > G > B » (`tokens.json` v3, `lines.scene.chassis.$test`) ; `#0D0C0A` passe. Ce qui manque : le moteur d'impression de Chrome sur **Fragment Mono contre DM Mono**, une heure. Qui doit le produire : ce produit, seul à tenir une chaîne PDF réelle. Rien ne change d'ici là ; en phase 5, la mono et le fond s'isolent en une ligne chacune.

## Variable 4 — L'accent · **coïncide**

| | |
|---|---|
| Valeur scène | « une teinte produit **plus une paire opératoire A/B** (ambre / ardoise), constante sur toute la ligne » — `secteurs.html:420-421` |
| Raison écrite | « la paire A/B n'est pas de l'identité, c'est de l'état — donc elle doit être **identique dans tous les produits de scène** » — `:422-425` |

Le code :

| Rôle | Valeur | Où |
|---|---|---|
| accent produit | `#C9762F` | `ui/panels.cpp:29` (`kAccent`) |
| deck A — ambre | `#C99A2F` | `ui/panels.cpp:31` (`kAmber`), posé en `:4711` |
| deck B — ardoise | `#6E8696` | `ui/panels.cpp:33` (`kSlate`), posé en `:4715` |

La paire est employée exactement comme `:422-425` la décrit : elle dit de quelle
source on parle. `ui/panels.cpp:2796,2798` et `:4652,4658` écrivent « A » et
« B » dans les deux teintes ; `:2242` et `:4661` colorent en ardoise ce qui
appartient au deck B ; `:533` écrit un mémo de deck en ardoise.

**Verdict : coïncide.** C'est la variable la mieux tenue des sept.

Trois conséquences n'appartiennent pas au cadran et sortent d'ici :

- aucune des trois valeurs n'existe dans `tokens.json` — il n'y a ni entrée
  `accents.scratchvj` (`:37-47`), ni niveau `lines.scene` que
  `secteurs.html:492-493` annonce pour la v3 ; et `suite.css:53-61` n'a pas de
  classe produit pour la scène. → **fermé** (`c4fa93c`, phase 5, SCRATCHVJ-07) — `scratchvj/ui/tokens.h`, provenance et version en tête, deux parties. L'accent du produit est en partie 2, derrière un nom unique, parce qu'il est attaché à un nom qui n'existe pas ; la paire A / B et les deux signaux de scène sont transcrits de `lines.scene`. Le second orange documentaire `#b45f1c` reste en phase 7, avec la feuille locale ;
- l'ambre `#C99A2F` est à trois unités de `signal.running` `#C9A227`
  (`tokens.json:32`), et porte donc deux sens à la fois — le deck A et le travail
  en cours. → **fermé** (`3a278f2`, SCRATCHVJ-08) — l'ambre appartient au deck A ; `running` garde sa valeur dans `tokens.json` et se montre ici par la progression nommée, jamais par un point ;
- la paire est portée par du **texte** coloré, or `secteurs.html:377-379` range
  dans l'invariant que `fail` « reste la seule couleur autorisée à porter du
  texte ». → **fermé** (`3a278f2`, SCRATCHVJ-10) — « A » et « B » gardent leur couleur (`DIRECTION-ARTISTIQUE.md`, « Ce qui compte comme du texte ») ; les phrases reviennent à la craie ; le nom du produit devient capuchon 2 px + nom en craie (`kAccentCap`, d'après `rhythm.accent_cap`).

## Variable 5 — Le mouvement · **coïncide**

| | |
|---|---|
| Valeur scène | « **0 ms par défaut** ; la seule animation autorisée est celle qui est calée sur le tempo » — `secteurs.html:429-430` |
| Raison écrite | « toute animation d'interface prend de l'attention à l'image [projetée] » — `:431-433` |
| Piège annoncé | `MAISON.md:108` — « Vérifier qu'aucune transition ImGui n'a été laissée à sa valeur par défaut » |

**Le piège n'existe pas, et voici la preuve.** `apply_style()`
(`ui/panels.cpp:4722-4785`) fixe treize métriques (`:4724-4736`) et
quarante-trois couleurs (`:4739-4784`) : aucune n'est une durée. `ImGuiStyle` n'expose aucun champ de transition — il n'y
a pas de « valeur par défaut » à corriger, parce qu'il n'y a pas de champ.
Recherche de `lerp|smooth|anim|ease|transition|fade|interp|DeltaTime *` dans
`ui/`, hors `imgui_impl_bgfx.cpp` : **aucune correspondance** dans un chemin
d'interface (les seules occurrences sont des `Release()` COM dans `audio_in.cpp`
et un uniforme de transition vidéo dans `gpu_compose.cpp:175,183`, qui est une
transition d'**image**, pas d'interface).

Une seule chose varie avec le temps à l'écran, et elle est hors du champ de la
variable : le curseur clignotant qu'ImGui dessine dans un champ de saisie
(`input_string`, `ui/panels.cpp:1559-1569`). Ce n'est pas une transition d'état.

Aucune animation calée sur le tempo non plus. Le tempo est partout dans la
**mesure** — boucles en temps (`ui/panels.cpp:2318-2328`), pads en fractions de
temps (`:2386`), saut au temps (`:2358-2360`) — jamais dans le dessin.

**Verdict : coïncide.** L'axe est vide, et vide dans le bon sens : 0 ms par
défaut, aucune animation, y compris la seule qui serait autorisée.

## Variable 6 — Le papier des documents · **ne coïncide pas**

| | |
|---|---|
| Valeur scène | « **fond sombre** » — `secteurs.html:437` |
| Raison écrite | « le document de scène est lu en coulisse, sur un téléphone, dans le noir — et le code visuel du marché y est sombre » — `:438-441` |
| Confirmé côté maison | `diorama-01-reponses.md:406-412` : le papier sombre « existe, mais il appartient à la **ligne scène** », et `suite.css:191` n'est pas contredit parce que c'est « la valeur atelier d'une variable de cadran » |

Le code. `docs/pdf/_style.css` est la seule feuille documentaire du produit
(liée par `docs/pdf/manuel.html:6` et `docs/pdf/argumentaire.html:6`, et par
elles seules). Elle est **claire** :

- `--ground: #f7f5f1`, `--ink: #171614`, `--body: #2b2926` — `:7-14` ;
- seule la **couverture** est sombre : `.cover { background: #141412 }` — `:42` ;
  et les blocs de code, `pre { background: #141412 }` — `:144`.

C'est-à-dire l'inverse exact de ce qu'attendent les deux lignes : le corps est au
papier **atelier**, et la couverture est un **aplat sombre**, que
`DIRECTION-ARTISTIQUE.md:168-171` interdit nommément (« Pas de couverture en
aplat. Ni sombre, ni colorée »).

**Verdict : ne coïncide pas.** Ni avec la valeur scène, ni avec la valeur
atelier. Et la cible — *une feuille, deux papiers* (`secteurs.html:482`) — n'est
pas atteignable aujourd'hui : `suite.css` n'a pas de papier sombre, et sa
dernière ligne (`:260-261`) écrit « **Aucun thème sombre.** Un manuel se lit et
s'imprime sur du clair » comme une règle générale sans renvoi, alors que
`diorama-01-reponses.md:406-412` vient d'établir que c'est une valeur de cadran.
→ la note est **rendue** (phase 7) ; **remonté SCRATCHVJ-17** — *reporté*. Ce qui manque : les deux valeurs de `SCRATCHVJ-06`. Qui doit le produire : la maison, dans `docs/suite.css` — un bloc `.line-scene` qui redéfinit sept jetons et les quatre signaux (éclaircis), `--paper-sunk` plus **sombre** que `--paper`, non imprimable ; pas de classe `.p-` avant le nom. La phase 7 de ce produit est **rendue** par la note ; `docs/pdf/_style.css` ne se réécrit pas pour ce que la
feuille doit porter.

Conformément à `MAISON.md:100` et à la consigne de ce chantier, `_style.css`
n'est **pas** réécrite et sa réécriture n'est pas recommandée.

## Variable 7 — La densité · **indécidable : la variable n'a pas de valeurs**

| | |
|---|---|
| Valeur scène | « hiérarchisée en trois niveaux, dont un lisible à un mètre » — `secteurs.html:445` |
| Raison écrite | « un instrument de scène qui affiche tout à densité égale est illisible dans l'urgence. Ce n'est pas *moins* dense : c'est dense en trois étages » — `:446-448` |

C'est la seule des sept dont la colonne « scène » ne porte **aucun nombre**. La
colonne atelier renvoie à des valeurs mesurables (`tokens.json:20-24` : unité 4,
gouttière 16, groupe 20, ligne 28, barre 32) ; la colonne scène ne renvoie à
rien.

Ce que le code fait, faute de valeurs : il porte **trois étages de fonte** —
15 px pour les mots (`ui/main_ui.cpp:258`), 11,5 px pour les sur-titres (`:259`),
15 px en chasse fixe pour les valeurs (`:261`) — et **deux étages de contrôle**,
44 px et 28 px (`ui/panels.cpp:2096, 2382` contre `:205`). L'espacement est écrit
une fois, dans `apply_style()` : `CellPadding (8,4)`, `ItemSpacing (10,6)`,
`ItemInnerSpacing (6,4)`, `WindowPadding (18,16)`, `FramePadding (8,4)` —
`ui/panels.cpp:4732-4736`. Six des dix nombres sont des multiples de l'unité de 4
(`tokens.json:21`) ; `10`, `6`, `6` et `18` ne le sont pas, et `ItemSpacing
(10,6)` ne l'est dans aucun de ses deux axes.

**Verdict : la question ne peut pas se poser.** On ne peut pas dire si le code
coïncide avec une valeur qui n'existe pas. Trois étages typographiques existent,
ce qui est le bon nombre, mais rien n'écrit lesquels, ni lequel se lit à un
mètre, ni ce qu'« un étage » recouvre au juste — une taille, une densité
d'espacement, ou les deux. → **remonté SCRATCHVJ-05** — *reporté*. Ce qui manque : la définition d'un étage et ses valeurs, éprouvées sur un **second produit de scène**, que `maison/05-QUI-EST-QUI.md` ne connaît pas encore. Qui doit le produire : la maison, au tour 01 du second, après l'avoir nommé ou retiré la phrase de `secteurs.html`. En attendant, en phase 5 : nommer les trois étages dans le fichier de jetons — quel panneau à quel étage — sans en inventer les valeurs, et ne pas régulariser `10`, `6`, `18`.

## Récapitulatif

| Variable | Coïncide ? | État |
|---|---|---|
| 1 · distance et corps | **oui** — corps 15, cible 44 là où la main frappe à l'aveugle, ligne de paramètre 28 par l'invariant | fermé (`c4fa93c`) |
| 2 · règle de couleur | **oui depuis `41ca9c5`** — l'état, et le compte tenu au sens de la plus petite région bordée | fermé (SCRATCHVJ-11) |
| 3 · châssis | **en partie** — teinte oui, valeur non | remonté SCRATCHVJ-06, reporté sur la mono |
| 4 · accent | **oui** | conforme ; 07 → phase 5, 08 et 10 fermés (`3a278f2`) |
| 5 · mouvement | **oui** | conforme |
| 6 · papier des documents | **non** | remonté SCRATCHVJ-17, reporté — la phase 7 est rendue par la note |
| 7 · densité | **indécidable** — la variable n'a pas de valeurs | remonté SCRATCHVJ-05, reporté |

Deux coïncidences pleines, trois partielles, une non-coïncidence, une
indécidable.

---

# Axe 1 — les chaînes qui nomment une projection

## 1.1 Écrites sur disque

| Fichier | Champ | Valeurs | Où | État |
|---|---|---|---|---|
| `library.json` | `clips[].projection` | `"auto"`, `"flat"`, `"equirect"` | table `config/library_io.cpp:18-22` ; écriture `:63` ; lecture `:120-127` | `"flat"` **conforme**, seule forme canonique du catalogue écrite sur disque (`00-vocabulaire.md:48`) — **fermé** (`9ccce6f`). `"equirect"` et `"auto"` → **fermé** (`ef3cbbc`, phase 2, SCRATCHVJ-01) — `equirect_360` s'écrit, `"auto"` est devenu l'absence de clé, les deux anciennes formes se lisent encore, et le lecteur dit qu'il a dû les accepter pour que le fichier soit réécrit une fois au démarrage. Le refus de chaîne inconnue est intact, vérifié sur un fichier réel |
| `library.json` | `crates[].name` | « Tous les clips », « 360° », « Loops & textures » | `app/engine.cpp`, écrits par `config/library_io.cpp` | **fermé** (`ef3cbbc`, phase 2, SCRATCHVJ-03) — la caisse que le produit crée lui-même porte la forme longue, « Équirectangulaire 360 » ; les trois noms que ce produit a écrits (`360°`, `360`, `2D`) sont migrés à la lecture, un nom tapé par l'utilisateur ne l'est pas. **Ce partage est une décision de ce dépôt** → tour 03, `SCRATCHVJ-28` |
| `.svcache`, en-tête | `flags`, bit 1 | `kCacheEquirect = 1u << 1` | `core/videocache.h:43-46` ; testé `:58` ; posé `app/analyze.cpp:258` et `app/engine.cpp:75` | **hors périmètre** — un bit, pas une chaîne. `00-vocabulaire.md:5-8` ne régit que « la chaîne écrite sur disque ou passée d'une application à l'autre » |
| `settings.json` | — | aucune | preuve ci-dessous | **hors périmètre** — axe vide |
| `mapping.json` | `bindings[].id` | `ch1.trim`, `xfader`, `ch1.eq.hi`… | fichier lu ; `core/layout.h:1-7` | **hors périmètre** — des contrôles, pas des projections |
| `profiles/*.json` | `layout` | groupes de panneau, en millimètres | `config/profile_io.cpp` | **hors périmètre** — homonyme, § axe 4 |
| `.svtake` (prises) | — | aucune | en-tête binaire de 32 octets, `core/take.h:24-26` ; écrit `ui/main_ui.cpp:1456` | **hors périmètre** — axe vide |
| UDP → Unreal | — | aucune | `core/protocol.h:22` ; `unreal/Source/ScratchLink/Private/ScratchLinkReceiver.cpp:9` | **hors périmètre** — le paquet ne porte aucun nom de projection |

Preuve pour `settings.json` : le fichier complet a été lu ; il porte cinq
sections (`library`, `midi`, `mix`, `output`, `platter`) et une version. Aucune
clé, aucune valeur ne nomme une projection, une disposition ou un œil.

## 1.2 Identifiants dans le code

| Identifiant | Où | Ce qu'il désigne | État |
|---|---|---|---|
| `ProjectionOverride::{Auto, Flat, Equirect}` | `core/library.h:28-32` | ce que l'utilisateur a forcé sur un clip | **fermé** (verdict SCRATCHVJ-01, rien à écrire) — un identifiant de code n'est pas « la chaîne écrite sur disque ou passée d'une application à l'autre » (`00-vocabulaire.md`, en tête) ; le verdict ne vise que le fichier de bibliothèque, et renommer un identifiant est une refonte qu'aucune phase ne porte |
| `Projection::{Rectilinear, LittlePlanet, FisheyeView}` | `core/sphere.h` | la reprojection de la **vue**, pas la projection du fichier | **fermé** (`ef3cbbc`, phase 2, SCRATCHVJ-02) — `Rectilinear`, `LittlePlanet`, `FisheyeView` dans le code, « Rectiligne » / « Little planet » / « Fisheye » à l'écran. Les libellés FR des deux dernières ne sont dans aucune table → tour 03, `SCRATCHVJ-27` |
| `kCacheEquirect`, `is_equirect()` | `core/videocache.h:45,58` | le drapeau du cache | **fermé** (verdict SCRATCHVJ-01, rien à écrire) — un identifiant de code n'est pas « la chaîne écrite sur disque ou passée d'une application à l'autre » (`00-vocabulaire.md`, en tête) ; le verdict ne vise que le fichier de bibliothèque, et renommer un identifiant est une refonte qu'aucune phase ne porte |
| `effective_equirect()`, `shown_equirect()` | `core/library.h:37,61` ; `core/library.cpp:19-23` | ce qui est réellement montré | **fermé** (verdict SCRATCHVJ-01, rien à écrire) — un identifiant de code n'est pas « la chaîne écrite sur disque ou passée d'une application à l'autre » (`00-vocabulaire.md`, en tête) ; le verdict ne vise que le fichier de bibliothèque, et renommer un identifiant est une refonte qu'aucune phase ne porte |
| `equirect_from_direction`, `direction_from_equirect`, `sample_equirect` | `core/sphere.h:51,54,60` | la géométrie | **fermé** (verdict SCRATCHVJ-01, rien à écrire) — un identifiant de code n'est pas « la chaîne écrite sur disque ou passée d'une application à l'autre » (`00-vocabulaire.md`, en tête) ; le verdict ne vise que le fichier de bibliothèque, et renommer un identifiant est une refonte qu'aucune phase ne porte |
| `sample_equirect_eye` | `core/headset.h:102` | la même, pour un œil de casque | **fermé** (verdict SCRATCHVJ-01, rien à écrire) — un identifiant de code n'est pas « la chaîne écrite sur disque ou passée d'une application à l'autre » (`00-vocabulaire.md`, en tête) ; le verdict ne vise que le fichier de bibliothèque, et renommer un identifiant est une refonte qu'aucune phase ne porte |
| `View360Gpu` | `ui/gpu_view360.h` | une classe de passe GPU | **hors périmètre** — ne nomme aucune projection |

## 1.3 Libellés à l'écran

| Ligne (`ui/panels.cpp`) | Chaîne | Où | État |
|---|---|---|---|
| `388` | `equirect 360` / `plan 2D` | détail sous la vignette du deck | **fermé** (`ef3cbbc`, phase 2, SCRATCHVJ-03) — « 360 » et « Rectiligne » ; `2D` et « équirectangulaire » seul ont disparu, console comprise ; la forme longue est à côté, dans l'infobulle de l'inspecteur et dans celle du sélecteur d'en-tête. **`SCRATCHVJ-22` est fermé** (2026-09-10) : la maison a tranché dans le sens du produit et `design/SCENE.md` ne porte plus « Plate » — « **un cadran re-règle des valeurs ; il n'ouvre pas une seconde table de libellés** » — et si « Rectiligne » ne tenait pas dans le sélecteur, la forme courte serait « Rectil. », jamais un autre mot |
| `851` | `360°` / `2D` | `projection_word()`, employé en `976` et `1369` | **fermé** (`ef3cbbc`, phase 2, SCRATCHVJ-03) — « 360 » et « Rectiligne » ; `2D` et « équirectangulaire » seul ont disparu, console comprise ; la forme longue est à côté, dans l'infobulle de l'inspecteur et dans celle du sélecteur d'en-tête. **`SCRATCHVJ-22` est fermé** (2026-09-10) : la maison a tranché dans le sens du produit et `design/SCENE.md` ne porte plus « Plate » — « **un cadran re-règle des valeurs ; il n'ouvre pas une seconde table de libellés** » — et si « Rectiligne » ne tenait pas dans le sélecteur, la forme courte serait « Rectil. », jamais un autre mot |
| `1197` | `{"Tous", "2D", "360°", "Alpha"}` | filtres de la bibliothèque | **fermé** (`ef3cbbc`, phase 2, SCRATCHVJ-03) — « 360 » et « Rectiligne » ; `2D` et « équirectangulaire » seul ont disparu, console comprise ; la forme longue est à côté, dans l'infobulle de l'inspecteur et dans celle du sélecteur d'en-tête. **`SCRATCHVJ-22` est fermé** (2026-09-10) : la maison a tranché dans le sens du produit et `design/SCENE.md` ne porte plus « Plate » — « **un cadran re-règle des valeurs ; il n'ouvre pas une seconde table de libellés** » — et si « Rectiligne » ne tenait pas dans le sélecteur, la forme courte serait « Rectil. », jamais un autre mot |
| `1464` | `PROJECTION` | sur-titre de l'inspecteur | **conforme** — sur-titre de panneau, capitales admises (`DIRECTION-ARTISTIQUE.md:200-201`) |
| `1468-1469` | `Auto (360°)`, `Auto (2D)`, `2D`, `360°` | sélecteur de l'inspecteur | **fermé** (`ef3cbbc`, phase 2, SCRATCHVJ-03) — « 360 » et « Rectiligne » ; `2D` et « équirectangulaire » seul ont disparu, console comprise ; la forme longue est à côté, dans l'infobulle de l'inspecteur et dans celle du sélecteur d'en-tête. **`SCRATCHVJ-22` est fermé** (2026-09-10) : la maison a tranché dans le sens du produit et `design/SCENE.md` ne porte plus « Plate » — « **un cadran re-règle des valeurs ; il n'ouvre pas une seconde table de libellés** » — et si « Rectiligne » ne tenait pas dans le sélecteur, la forme courte serait « Rectil. », jamais un autre mot |
| `2039` | `{"2D", "360°"}` | sélecteur sur l'en-tête du deck | **fermé** (`ef3cbbc`, phase 2, SCRATCHVJ-03) — « 360 » et « Rectiligne » ; `2D` et « équirectangulaire » seul ont disparu, console comprise ; la forme longue est à côté, dans l'infobulle de l'inspecteur et dans celle du sélecteur d'en-tête. **`SCRATCHVJ-22` est fermé** (2026-09-10) : la maison a tranché dans le sens du produit et `design/SCENE.md` ne porte plus « Plate » — « **un cadran re-règle des valeurs ; il n'ouvre pas une seconde table de libellés** » — et si « Rectiligne » ne tenait pas dans le sélecteur, la forme courte serait « Rectil. », jamais un autre mot |
| `2236` | `{"Perspective", "Little planet", "Fisheye"}` | sélecteur `VUE 360` | **fermé** (`ef3cbbc`, phase 2, SCRATCHVJ-02) — `Rectilinear`, `LittlePlanet`, `FisheyeView` dans le code, « Rectiligne » / « Little planet » / « Fisheye » à l'écran. Les libellés FR des deux dernières ne sont dans aucune table → tour 03, `SCRATCHVJ-27` |
| `2240` | `VUE 360` | libellé de ligne | **fermé** (`ef3cbbc`, phase 2, SCRATCHVJ-13) — « Vue 360 », capitale initiale |
| `2288` | `SOURCE ÉQUIRECTANGULAIRE — cadre de visée` | sur-titre du popup de regard | **conforme** — sur-titre, et le mot est le bon (`ERGONOMIE.md:366`) |
| `4534` | `PROGRAMME · 360 PROJETÉ` | bande programme | **fermé** (`ef3cbbc`, phase 2, SCRATCHVJ-03) — « 360 » et « Rectiligne » ; `2D` et « équirectangulaire » seul ont disparu, console comprise ; la forme longue est à côté, dans l'infobulle de l'inspecteur et dans celle du sélecteur d'en-tête. **`SCRATCHVJ-22` est fermé** (2026-09-10) : la maison a tranché dans le sens du produit et `design/SCENE.md` ne porte plus « Plate » — « **un cadran re-règle des valeurs ; il n'ouvre pas une seconde table de libellés** » — et si « Rectiligne » ne tenait pas dans le sélecteur, la forme courte serait « Rectil. », jamais un autre mot |

## 1.4 Ligne de commande, démonstration, documentation

| Où | Chaîne | État |
|---|---|---|
| `app/main.cpp:41` | « a 2:1 picture is flagged equirect » (aide) | **fermé** (`ef3cbbc`, phase 2, SCRATCHVJ-01) — la forme canonique s'écrit, `"auto"` est devenu l'absence de clé, dans la même migration que `equirect` → `equirect_360` |
| `app/main.cpp:296` | `  equirect 360` (sortie de `analyze`) | **fermé** (`ef3cbbc`, phase 2, SCRATCHVJ-01) — la forme canonique s'écrit, `"auto"` est devenu l'absence de clé, dans la même migration que `equirect` → `equirect_360` |
| `app/main.cpp:341` | `  360         équirectangulaire` / `non` (sortie de `info`) | **fermé** (`ef3cbbc`, phase 2, SCRATCHVJ-03) — « 360 » et « Rectiligne » ; `2D` et « équirectangulaire » seul ont disparu, console comprise ; la forme longue est à côté, dans l'infobulle de l'inspecteur et dans celle du sélecteur d'en-tête. **`SCRATCHVJ-22` est fermé** (2026-09-10) : la maison a tranché dans le sens du produit et `design/SCENE.md` ne porte plus « Plate » — « **un cadran re-règle des valeurs ; il n'ouvre pas une seconde table de libellés** » — et si « Rectiligne » ne tenait pas dans le sélecteur, la forme courte serait « Rectil. », jamais un autre mot |
| `app/engine.cpp:277` | caisse `360°` de la démonstration | **fermé** (`ef3cbbc`, phase 2, SCRATCHVJ-03) — « 360 » et « Rectiligne » ; `2D` et « équirectangulaire » seul ont disparu, console comprise ; la forme longue est à côté, dans l'infobulle de l'inspecteur et dans celle du sélecteur d'en-tête. **`SCRATCHVJ-22` est fermé** (2026-09-10) : la maison a tranché dans le sens du produit et `design/SCENE.md` ne porte plus « Plate » — « **un cadran re-règle des valeurs ; il n'ouvre pas une seconde table de libellés** » — et si « Rectiligne » ne tenait pas dans le sélecteur, la forme courte serait « Rectil. », jamais un autre mot |
| `app/engine.cpp:75` | `width == height * 2 ? kCacheEquirect` | **fermé** (verdict SCRATCHVJ-18, rien à écrire) — grappe G7 : Mutoscope est la référence, `evidence` gagne `filename`, et **un rang absent est absent, il ne dégrade rien** (`spec/01-manifeste-plate.md`). L'inférence au ratio reste, « Auto (360°) » reste — une règle de détection, pas un mot |
| `app/analyze.cpp:255-258` | la même règle, avec sa raison écrite | **fermé** (verdict SCRATCHVJ-18, rien à écrire) — grappe G7 : Mutoscope est la référence, `evidence` gagne `filename`, et **un rang absent est absent, il ne dégrade rien** (`spec/01-manifeste-plate.md`). L'inférence au ratio reste, « Auto (360°) » reste |
| `core/destinations.cpp:17-20` | « regard 360 du deck A : lacet (degrés) », « zoom little planet / fisheye du deck A » | **fermé** (`ef3cbbc`, phase 2, SCRATCHVJ-02) — `Rectilinear`, `LittlePlanet`, `FisheyeView` dans le code, « Rectiligne » / « Little planet » / « Fisheye » à l'écran. Les libellés FR des deux dernières ne sont dans aucune table → tour 03, `SCRATCHVJ-27` ; l'unité entre parenthèses est traitée à l'axe 10.1 |
| `docs/format-cache.md` | prose : « équirectangulaire (360) », « la 2D et la 360 » | **fermé** (`ef3cbbc`, phase 2) — `equirect_360` dans la table des drapeaux, « la rectiligne et la 360 » dans la prose |
| `docs/roadmap.md` | les mêmes mots, dans deux entrées de journal datées | **hors périmètre** — un journal dit ce qui a été écrit un jour donné ; le réécrire en ferait un faux. Les deux entrées décrivent une interface qui n'existe déjà plus |

## 1.5 Le repère — **conforme, et la contradiction a disparu**

`core/sphere.h:13` : *« right-handed, +Y up, **-Z forward** at yaw and pitch
zero »*. `core/headset.h:44` répète la même base.

- `sphere.cpp:43-48` — `atan2(d.x, -d.z)` : la direction `(0,0,−1)` tombe au
  centre de l'image. Le centre d'une équirectangulaire regarde `−Z`.
  **Conforme** à `02-repere.md:25-27`.
- `sphere.cpp:57-67` — roulis, puis tangage, puis lacet appliqués à la direction,
  soit la composition `yaw ∘ pitch ∘ roll`. **Conforme** à `02-repere.md:17`.
- `sphere.h:38` et `sphere.cpp:62-65` — « positive turns the view to the right »,
  avec la raison écrite. **Conforme** à `02-repere.md:21`.

Ce relevé aurait remonté une contradiction ici. Elle n'existe plus : voir « Ce
que le tour 01 a déjà tranché ». **Fermé** (`9ccce6f`).

---

# Axe 2 — les chaînes qui nomment une disposition stéréo · **axe vide, et voici la preuve**

Recherche exhaustive de `sbs|side.by.side|top.bottom|over.under|\btb\b|anaglyph|stereoscop|\bstereo\b|\bmono\b|packing|eye_order|flip_vertical`
dans `scratchvj/src`, `scratchvj/ui`, `scratchvj/dvs` et `unreal/`, hors
`vendor/` : **aucune occurrence ne nomme une disposition stéréo.** Les 34
correspondances se répartissent en trois sens, tous étrangers à l'axe :

| Sens réel | Occurrences | Exemple |
|---|---|---|
| deux voies audio entrelacées | 14 | `core/quadrature.h:106`, `core/scope.h:126`, `ui/audio_in.h:57`, `dvs/decoder.h:56`, `dvs/generate.h:42` |
| la fonte à chasse fixe | 8 | `ui/panels.h:318` (`ImFont* mono`), `ui/panels.cpp:37-39` |
| divers, sans rapport | 12 | `app/analysis_queue.h:12`, « side by side » dans une phrase anglaise ; `core/profiles_builtin.cpp:160`, `booth.mono` — le bouton mono de la cabine d'un mixeur DJ |

La seule mention de la disposition stéréo du dépôt est un commentaire qui
l'**exclut** : `core/headset.h:31-32` — « Stereoscopic 360 is a different
FORMAT — over/under equirect — and would belong in `core/videocache`, not here. »
C'est la seule occurrence de `over/under` du dépôt, en prose anglaise, dans un
commentaire, pour dire que le produit ne le fait pas.

**État : hors périmètre — le produit ne lit ni n'écrit aucune disposition
stéréo.** Écrit ici parce que, comme `DIVERGENCES.md:69-76` le dit de Diorama,
« une absence muette se prend pour un oubli, et la prochaine session
recommencerait la recherche ».

---

# Axe 3 — les chaînes qui nomment un œil · **axe vide, avec une nuance**

Le produit possède bien une notion d'œil — `core/headset.h`, `EyeView`,
`eye_direction()`, `sample_equirect_eye()`, la passe GPU `ui/gpu_eye.cpp` et son
contrôle `ui/tools/eye_check.cpp`. Mais il s'agit de l'œil **d'un casque OpenXR
qui regarde la sphère**, pas de l'œil gauche ou droit d'une image
stéréoscopique. `core/headset.h:27-32` l'écrit sans ambiguïté : *« The eye
POSITION is deliberately absent. An equirectangular frame is a sphere at
infinity, so no interocular distance can produce parallax from it. »*

| Chaîne | Où | Sens réel | État |
|---|---|---|---|
| `oeil %u` | `ui/tools/xr_check.cpp:178` | la vue d'un casque, numérotée par OpenXR | **hors périmètre** |
| `passe oeil` | `ui/tools/eye_check.cpp:106,202,206` | le nom d'une passe GPU, dans un outil de contrôle | **hors périmètre** |
| `configuration stereo attendue` | `ui/tools/xr_check.cpp:184` | deux vues OpenXR, pas une disposition de fichier | **hors périmètre** |

Aucune chaîne du produit ne nomme un œil au sens de `00-vocabulaire.md:57-63`.
Les mots `left_first` et `right_first` n'apparaissent nulle part.

---

# Axe 4 — les faux positifs, et leur sens réel

C'est la moitié du travail de cet axe : sept mots du vocabulaire canonique
existent dans ce dépôt et **aucun** n'y a le sens du vocabulaire.

| Mot | Où | Sens réel dans ce produit | État |
|---|---|---|---|
| `layout` | `core/layout.h`, `core/layout.cpp`, `config/profile_io.cpp`, `profiles/*.json` | la **disposition physique des contrôles d'un mixeur** — « This is a CHECKLIST, not a driver », `core/layout.h:3` — et, dans un profil, la position en millimètres d'un groupe sur la face avant | **hors périmètre** — homonyme intégral |
| `projection` | `config/library_io.cpp:63`, `core/library.h:31`, `ui/panels.cpp:1464` | presque toujours *la manière de lire un clip*, ce qui **est** l'axe 1 ; mais aussi « géométrie de projection » (`ui/panels.cpp`, écran de sortie) = la déformation d'un vidéoprojecteur sur une surface | le premier sens → axe 1 ; le second **hors périmètre** |
| `flat` | `config/library_io.cpp:20`, `core/library.h:31` | la projection rectiligne — **le seul emploi du dépôt, et il est canonique**. Le piège 2 de `DIVERGENCES.md:87-90` (collision avec le « mode d'affichage » de Lacuna) **n'existe pas ici** | **conforme** |
| `mono` | `core/spectrum.h:67`, `core/profiles_builtin.cpp:160`, `ui/panels.h:318` | l'audio à une voie, le bouton mono d'une cabine, la fonte à chasse fixe. **Jamais** monoscopique | **hors périmètre** |
| `stereo` | 14 sites, tous audio | deux voies entrelacées | **hors périmètre** |
| `360` / `180` numériques | `ui/panels.cpp:2279,2281`, `core/destinations.cpp:72` | des **degrés** : bornes de lacet et de roulis, rotation d'un kaléidoscope | **hors périmètre** |
| `plan` | `ui/panels.cpp:388`, « plan 2D » | le mot de cinéma (un plan), pas celui de géométrie | libellé → axe 1 |

Un faux positif de plus, propre à ce produit : `sphere` / `sphérique`
(`core/sphere.*`, `app/engine.cpp:277`) désigne la **reprojection**, jamais un
format de fichier. → axe 1, ligne `Projection`.

---

# Axe 5 — les touches liées et leur effet

C'est l'axe que `MAISON.md` annonce comme le plus lourd de tous les produits.
**Il ne l'est pas : le produit lie quatre touches.** L'instrument se joue sur des
platines et une table MIDI, pas au clavier — 63 contrôles physiques sont décrits
par `core/layout.h` et par le catalogue imprimé (`docs/pdf/README.md:26`), et
c'est là que passe le geste.

## 5.1 Les quatre touches liées

| Touche | Effet | Où | Garde | État |
|---|---|---|---|---|
| `Espace` | lecture / pause du deck **sous la souris** | `ui/panels.cpp:2093-2099` | `IsWindowHovered(ChildWindows)` et `!WantTextInput` | **conforme** — `ERGONOMIE.md:250`, et couche 1 de `MAISON.md:47` (« unanime »). Le choix du deck survolé est écrit et motivé : `:2091-2092`, « in a set nobody aims at a 28 px button with the other hand on a fader » |
| `F` | image seule (masque toute l'interface) | `ui/panels.cpp:4688` | `!WantTextInput` | **conforme** — `F` est l'une des quatre lettres que `ERGONOMIE.md:260-265` interdit à la **réservation de suite**, donc libre pour le produit ; `MAISON.md:101` et `:138` le confirment. L'effet est celui que `ERGONOMIE.md:253` donne à `Tab` |
| `B` | replier le rail de bibliothèque | `ui/panels.cpp:4689` | `!WantTextInput` | **conforme** — `B` n'est ni dans la table réservée (`ERGONOMIE.md:245-258`) ni dans les quatre lettres interdites ; aucune règle ne s'y oppose |
| `Échap` | 1. quitte « image seule » ; 2. sinon ferme la fenêtre de sortie ; 3. sinon **quitte l'application** | `ui/main_ui.cpp:663-675` | `!WantTextInput` | **fermé** (`9b88327`, phase 4, SCRATCHVJ-15) — le gestionnaire SDL ne traite plus `Échap` du tout : c'est le rang 6. Les trois rangs sont partis — celui qui quittait « image seule » (`F` s'en charge, `MAISON.md`), celui qui fermait la sortie (aucun rang de la pile ne le porte ; l'écran SORTIE le fait) et celui qui quittait l'application. Le seul rang qui ait un objet ici est le popup ouvert, et ImGui le ferme : l'événement se consomme une fois |

`Tab` n'est lié à rien par le produit. ImGui s'en sert pour la navigation entre
champs ; le produit n'y touche pas.

## 5.2 `Échap` — le seul manquement franc du relevé

`ERGONOMIE.md:284-285` : *« `Échap` ne ferme jamais la fenêtre, et ne quitte
jamais le plein écran seul »*. `MAISON.md:180` le reprend dans la définition de
fini. Le code fait exactement les deux :

- `ui/main_ui.cpp:668-669` — le premier `Échap` quitte « image seule » ; or
  `ERGONOMIE.md:284-285` réserve cela à `Tab`, et `MAISON.md:138` désigne `F`
  pour ce produit ;
- `ui/main_ui.cpp:673` — le troisième `Échap` fait `running = false`, ce qui
  **ferme la fenêtre**.

Le code assume l'ordre et écrit sa raison (`:665-667`) : *« One escape gets the
picture off the projector; a second ends the set. Quitting straight to a desktop
in front of a room is the thing this ordering exists to prevent. »* C'est un bon
argument pour les deux premiers rangs ; il ne couvre pas le troisième, où l'on se
retrouve précisément sur un bureau devant une salle.

Second défaut, indépendant : la garde est `!WantTextInput` seulement. Un popup
ouvert (`takes`, `newcrate`, `gaze` — `ui/panels.cpp:696, 1278, 2267`) sans champ
de saisie actif laisse passer les **deux** traitements : ImGui ferme le popup, et
le gestionnaire SDL avance d'un rang dans sa propre pile. La pile de priorité de
`ERGONOMIE.md:275-285` n'est pas implémentée — ses rangs 1 à 5 n'existent pas, et
le rang 6 (« ne rien faire ») est remplacé par « quitter ».

## 5.3 Le noyau réservé que `MAISON.md:101` exige, et son état

| Touche | Exigée par | Liée ? | État |
|---|---|---|---|
| `Espace` | `MAISON.md:101`, `ERGONOMIE.md:250` | oui | **conforme** |
| `Échap`, selon la pile | `MAISON.md:101`, `ERGONOMIE.md:275-285` | oui, et juste | **fermé** (`9b88327`, phase 4, SCRATCHVJ-15) — voir 5.1 |
| `Ctrl`+`Z` / `Ctrl`+`Maj`+`Z` | `MAISON.md:101`, `ERGONOMIE.md:257` | **non** | **hors périmètre** — la fiche produit ne la programme plus : « `Ctrl`+`Z` est **sans objet** — un produit sans état persistant n'a pas d'annulation à offrir » (`maison/produits/filoscope.md`, corrigé le 2026-09-10 sur `SCRATCHVJ-26`). Le critère est celui d'`ERGONOMIE.md` : « une touche réservée n'est exigible que d'un produit qui a la chose qu'elle manipule ». Ce produit ne détient aucun état réversible au sens de `04-frontieres.md:19-23`, et la scène « ne produit aucun fichier » (`secteurs.html:191-193`) |
| `?` / `F1` | `MAISON.md:101`, `ERGONOMIE.md:255` | **oui** | **fermé** (`9b88327`, phase 4) — une carte du clavier en popup non modal, ouverte par `F1` ou par le caractère `?` (lu dans la file de caractères, pour être atteignable sur AZERTY comme sur QWERTY). Elle porte les cinq gestes du clavier et dit que le reste se joue sur les plateaux |
| `Tab` | `MAISON.md:101`, `ERGONOMIE.md:253` | **non** | **hors périmètre** — « et `Tab` ne se programme pas : la touche est **rouverte** » (`maison/produits/filoscope.md`, corrigé le 2026-09-10 sur `SCRATCHVJ-26` ; quatre produits avaient posé la même question sur la même ligne d'un talon généré). L'effet est sur `F`, l'une des quatre lettres libres |

## 5.4 Le reste du clavier réservé de l'atelier

`0`, `Ctrl`+`1`/`2`/`3`, `C` maintenu, `←`/`→`, `Maj`+`←`/`→`, `G`,
`Ctrl`+`Maj`+`C`/`V` : **aucune n'est liée**. Toutes appartiennent au viseur 360
et à la comparaison avant/après, que ce produit n'a pas. **Hors périmètre** —
`ERGONOMIE.md:229` limite la table aux « six produits qui portent un viseur », et
`:174-176` les nomme ; ce produit n'est pas de la suite.

## 5.5 Aucune touche chiffrée — et ce que cela contredit

Recherche de `ImGuiKey_0` à `ImGuiKey_9`, `ImGuiKey_Keypad*`, `SDLK_0`–`SDLK_9` :
**aucune occurrence**. Les huit pads sont à l'écran (`ui/panels.cpp:2382-2470`,
cellules de 44 px) et sur la surface MIDI ; aucun n'a de raccourci clavier.

`MAISON.md:101` et `secteurs.html:483` décrivent pourtant « les pads sur
`1`–`5` » et un conflit potentiel avec le clavier réservé. **Cette interface
n'existe plus** : elle a été remplacée par la refonte de septembre (`9ccce6f`, et
`design/README.md:6-8`). Le conflit annoncé n'existe pas.
→ **fermé** (verdict SCRATCHVJ-15, rien à écrire ici) — les deux sources sont corrigées ; `MAISON.md` et son générateur n'existent plus, et ce qui est propre à ce produit se lit dans `maison/produits/filoscope.md`. La conclusion tient par Mutoscope seul (`MUTOSCOPE-01`).

## 5.6 Le viseur 360 à la souris — **axe vide, et c'est écrit**

`MAISON.md:109` conditionne la convention du viseur : « là où il est manipulé à
la souris (mode 360, plein cadre) […] Là où il est piloté par un potard, la
convention ne s'applique pas. »

Le regard n'est **jamais** manipulé à la souris. Il est piloté :

- par quatre curseurs dans un popup — `ui/panels.cpp:2279-2285` : lacet
  (−180…180), tangage (−90…90), roulis (−180…180), champ (20…170) ou zoom
  (0,2…3) ;
- par les potards d'égalisation de la voie 1, via le mapping — dit à
  l'utilisateur en `ui/panels.cpp:2264-2265` ;
- par les destinations `deck.a.yaw`, `deck.a.pitch`, `deck.a.fov`,
  `deck.a.zoom` — `core/destinations.cpp:17-20`, bornées en
  `app/engine.cpp:512-514`.

Aucun `IsMouseDragging`, `GetMouseDragDelta` ni `MouseDelta` n'est lu sur la vue
360 ni sur la bande programme. Le seul `MouseDelta` du fichier est sur un potard
(`ui/panels.cpp:2614`).

**État : hors périmètre** pour `−Δx` au lacet, `+Δy` au tangage, l'absence
d'inertie et la molette — le geste n'existe pas. Les **constantes** de
`tokens.json:48-57`, elles, ne dépendent pas du geste, et leur sort est ouvert :
→ **fermé** (`b025d6a`, SCRATCHVJ-16) pour le tangage : borné à ±89,9° par `kPitchClampDeg` (`core/sphere.h`, nommé d'après `viewer.geometry.pitch_deg_clamp`), sur le curseur, le mapping par défaut et la destination. Aucun HUD n'est dû. Le champ par défaut (90° contre 75°) et ses bornes (20–170°) ne bougent pas : `tokens.json` v3 a rangé le champ sous `viewer.plate`, et le domaine de little planet / fisheye est ouvert (`00-vocabulaire.md`, « Les modes de vue, ou reprojections ») — question au tour 02, `SCRATCHVJ-25`.

## 5.7 La souris ailleurs

Le produit lie la souris à des contrôles, pas à une vue : `InvisibleButton` en
`ui/panels.cpp:221, 271, 319, 477, 660, 817, 1072, 1247, 1333, 2393, 2605, 2693,
2758, 2843, 3354, 3663, 3877`. Le potard (`rotary`, `:2593-2684`) se règle par un
glisser **vertical** — `:2614`, `MouseDelta.y * 0.005f` — ce qui est la
convention de tous les potards logiciels, et sur quoi aucune source n'écrit quoi
que ce soit. **Hors périmètre** : `ERGONOMIE.md:143-150` décrit le glisser sur le
**libellé** d'une ligne de paramètre, une interaction que ce produit n'a pas.

---

# Axe 6 — les valeurs de couleur écrites dans le code

## 6.1 La table nommée · `ui/panels.cpp:22-33`

Douze valeurs, nommées par rôle. Le commentaire `:20-21` en donne la raison :
« Named by role rather than by hue, so a retune changes one table instead of
every call site. » C'est déjà l'esprit du fichier unique de jetons demandé par
`MAISON.md:74-82` ; il manque la provenance, la version, et le fait d'être un
fichier.

| Nom | Valeur | Rôle | Jeton d'atelier voisin | État |
|---|---|---|---|---|
| `kGround` | `#141412` | fond de fenêtre | `chassis.void` `#0A0C0B` | **conforme par le cadran** — variable 3, `secteurs.html:412` ; la valeur → **remonté SCRATCHVJ-06** — *reporté*, voir « Variable 3 » : la direction est tranchée (luminance de l'atelier, teinte de la scène, test ±5 %), la mono manque |
| `kPanel` | `#1A1917` | fond de panneau | `chassis.panel` `#121614` | **conforme par le cadran** — même teinte chaude, variable 3 ; la valeur → **remonté SCRATCHVJ-06** — *reporté*, voir « Variable 3 » : la direction est tranchée (luminance de l'atelier, teinte de la scène, test ±5 %), la mono manque |
| `kWell` | `#0E0E0C` | champs, fonds de bouton | `lines.scene.chassis.raised` | **fermé par transcription** (`c4fa93c`, phase 5) — la source porte désormais la valeur de scène **et** sa réserve : le champ est **creusé** ici, **relevé** à l'atelier, et la convention de relief reste **ouverte** côté maison. Voir 6.7 |
| `kHair` | `#2E2D28` | filets 1 px | `chassis.line` `#2A302C` | **conforme par le cadran** — variable 3 ; la valeur → **remonté SCRATCHVJ-06** — *reporté*, voir « Variable 3 » : la direction est tranchée (luminance de l'atelier, teinte de la scène, test ±5 %), la mono manque |
| `kInk` | `#E9E6DF` | texte principal | `chassis.chalk` `#E4E7E1` | **conforme par le cadran** — craie chaude contre craie verdie, variable 3 |
| `kMuted` | `#8A867C` | libellés secondaires | `chassis.chalk_dim` `#8D958C` | idem |
| `kFaint` | `#605D56` | indisponible, sur-titres | `chassis.chalk_off` `#5A615A` | idem |
| `kAccent` | `#C9762F` | accent produit | aucun — absent de `tokens.json:37-47` | **fermé** (`c4fa93c`, phase 5, SCRATCHVJ-07) — `scratchvj/ui/tokens.h`, provenance et version en tête, deux parties. L'accent du produit est en partie 2, derrière un nom unique, parce qu'il est attaché à un nom qui n'existe pas ; la paire A / B et les deux signaux de scène sont transcrits de `lines.scene`. Le second orange documentaire `#b45f1c` reste en phase 7, avec la feuille locale |
| `kAmber` | `#C99A2F` | deck A, et « en cours » | `signal.running` `#C9A227` | **conforme par le cadran** pour le deck A (variable 4, `:420`) ; le double rôle → **fermé** (`3a278f2`, SCRATCHVJ-08) : l'ambre est au deck A ; « en cours » se dit à la craie, par la progression nommée |
| `kSlate` | `#6E8696` | deck B | aucun | **conforme par le cadran** — variable 4, `:420` ; l'absence dans `tokens.json` → **fermé** (`c4fa93c`, phase 5, SCRATCHVJ-07) — `scratchvj/ui/tokens.h`, provenance et version en tête, deux parties. L'accent du produit est en partie 2, derrière un nom unique, parce qu'il est attaché à un nom qui n'existe pas ; la paire A / B et les deux signaux de scène sont transcrits de `lines.scene`. Le second orange documentaire `#b45f1c` reste en phase 7, avec la feuille locale |
| `kSage` | `#7E946B` | « terminé », « lié » | `signal.done` `#7FB069` | **fermé** (`3a278f2`, SCRATCHVJ-09) — `kWarn` `#9C774E` entre dans la table nommée, dérivé de `#C48A4B` par les rapports de clarté et de saturation que la table applique déjà à `done` et `fail`, et va au lien de platine qui faiblit (`link_colour()`, voyant de la barre haute) ; la valeur monte au tour 02 (`SCRATCHVJ-21`). La déclaration des quatre valeurs comme jeu de support de la scène attend le fichier de jetons (phase 5) |
| `kAlert` | `#B54B3A` | « échec », « délié » | `signal.fail` `#D9584B` | **fermé** (`3a278f2`, SCRATCHVJ-09) — `kWarn` `#9C774E` entre dans la table nommée, dérivé de `#C48A4B` par les rapports de clarté et de saturation que la table applique déjà à `done` et `fail`, et va au lien de platine qui faiblit (`link_colour()`, voyant de la barre haute) ; la valeur monte au tour 02 (`SCRATCHVJ-21`). La déclaration des quatre valeurs comme jeu de support de la scène attend le fichier de jetons (phase 5) |

Il n'y a **aucune** valeur pour `signal.warn` (`tokens.json:34`). Les quatre
couleurs de signalisation sont un **invariant** (`secteurs.html:377-379`), pas
une variable de cadran ; trois sont retouchées et la quatrième manque.
→ **fermé** (`3a278f2`, SCRATCHVJ-09) — `kWarn` `#9C774E` entre dans la table nommée, dérivé de `#C48A4B` par les rapports de clarté et de saturation que la table applique déjà à `done` et `fail`, et va au lien de platine qui faiblit (`link_colour()`, voyant de la barre haute) ; la valeur monte au tour 02 (`SCRATCHVJ-21`). La déclaration des quatre valeurs comme jeu de support de la scène attend le fichier de jetons (phase 5).

## 6.2 Écrites hors de la table

| Où | Valeur | Ce que c'est | État |
|---|---|---|---|
| `ui/main_ui.cpp` | `0x141412ff` | `kGround`, réécrit en notation bgfx dans un autre fichier | **fermé** (`c4fa93c`, phase 5) — une seule constante, `kGroundRgb`, et deux notations dérivées d'elle : `col()` pour ImGui, `clear_rgba()` pour bgfx |
| `ui/panels.cpp` | `ImVec4(0.79f, 0.46f, 0.18f, 0.35f)` | `#C9752E` à 35 % — **c'est `kAccent`**, réécrit en flottants | **fermé** (`c4fa93c`, phase 5) — l'opacité est posée **sur** le jeton, `kSelectionAlpha` |
| `ui/panels.cpp` | `ImVec4(1.0f, 1.0f, 1.0f, 0.015f)` | blanc à 1,5 %, ligne de tableau alternée | **fermé** (`c4fa93c`, phase 5) — `kRowAlt`, valeur propre au produit, nommée et commentée en partie 2 |
| `ui/gpu_compose.cpp:108` | `0xFF000000u` | un texel noir de remplacement | **hors périmètre** — pas une couleur d'interface |
| `ui/gpu_compose.cpp:124`, `gpu_effects.cpp:108`, `gpu_eye.cpp:97`, `gpu_taps.cpp:91`, `gpu_view360.cpp:96`, `output_window.cpp:249` | `0x000000ff` | six effacements de vue GPU, en noir | **hors périmètre** — le fond derrière l'image, `DIRECTION-ARTISTIQUE.md:54` (`--void`) |
| `ui/output_window.cpp:335` | `0x00FFFFFFu` | blanc, canal alpha composé à part | **hors périmètre** |

**Total dans le code : 15 valeurs de couleur d'interface** (12 nommées + 3 hors
table), plus 8 valeurs utilitaires noir/blanc.

## 6.3 Ce qui n'en porte aucune · **prouvé**

- **Les shaders.** Les dix fichiers `ui/shaders/*.sc` ne contiennent **aucune
  constante de couleur**. Les seuls `vec3(...)` littéraux sont des rampes de
  teinte et des bases géométriques (`fs_program.sc:79`, `fs_view360.sc:29-72`).
- **Le plugin Unreal.** Recherche de `FColor|FLinearColor|#[0-9a-f]{6}` dans
  `unreal/` : **aucune occurrence**. Le plugin ne lit qu'une socket et une
  texture (`secteurs.html:638-640`).
- **`core/`, `config/`, `dvs/`.** Les seules constantes hexadécimales y sont des
  nombres magiques de format (`core/videocache.h:30`, `core/protocol.h:22`,
  `core/take.h:24`, `core/cachemeta.cpp:15`) et des masques
  (`app/analyze.cpp:52`). **Hors périmètre.**

## 6.4 Dans la maquette · `design/*.dc.html`

Sept artboards, **1 166 occurrences hexadécimales pour 37 valeurs distinctes**.
Les douze de la table `ui/panels.cpp:22-33` y sont toutes, et elles portent
l'essentiel : `#2E2D28` ×140, `#605D56` ×96, `#8A867C` ×83, `#E9E6DF` ×75,
`#C9762F` ×57, `#7E946B` ×44, `#C99A2F` ×42, `#141412` ×34, `#6E8696` ×34,
`#0E0E0C` ×32, `#1A1917` ×6, `#B54B3A` ×6.

Les 25 autres sont des valeurs de figure, employées une à cinq fois
(`#E08B3E`, `#2BC46A`, `#D63AC9`, `#101820`…).

**État : hors périmètre**, avec la raison. `design/README.md:3-4` en fait des
fichiers de travail qui « font foi » pour la disposition, et `:43-45` dit que les
jetons y sont ceux du code. Ce ne sont ni du code ni un document qui sort du
dépôt (`DIRECTION-ARTISTIQUE.md:159-160`). Ils ne se retouchent donc pas en phase
5 ; ils se retoucheront le jour où la maquette suivra une valeur changée.

*Une inexactitude locale* : `design/README.md` disait que les jetons sont ceux
d'`apply_style` « dans `scratchvj/ui/main_ui.cpp` », alors qu'`apply_style()` est
dans `ui/panels.cpp`. → **fermé** (`c4fa93c`, phase 5) — le renvoi pointe désormais
sur `scratchvj/ui/tokens.h`, qui est l'endroit juste depuis cette phase :
`apply_style()` ne porte plus aucune valeur, seulement des noms de jetons.

## 6.5 Dans les documents · voir aussi l'axe 9

**963 occurrences hexadécimales pour 61 valeurs distinctes** hors du code, tous
fichiers confondus (maquette comprise). Hors maquette :

| Fichier | Occurrences | Distinctes | État |
|---|---|---|---|
| `docs/pdf/_style.css` | 29 | 25 | **remonté SCRATCHVJ-17** — *reporté*, voir « Variable 6 » : la maison écrira `.line-scene` dans `suite.css` quand `SCRATCHVJ-06` sera rendue ; `_style.css` ne se réécrit pas, et **remonté SCRATCHVJ-17** — la phase 7 est rendue par la note ; rien à faire tant que `suite.css` n'a pas son papier sombre |
| `docs/pdf/build.py:74,76` | 2 | 2 | **remonté SCRATCHVJ-17** — la phase 7 est rendue par la note ; rien à faire tant que `suite.css` n'a pas son papier sombre — le script **écrit** `#eae5dd` et `#6c6862` en style local dans le HTML qu'il régénère |
| `docs/pdf/manuel.html` | 126 | 2 | **remonté SCRATCHVJ-17** — la phase 7 est rendue par la note ; rien à faire tant que `suite.css` n'a pas son papier sombre — les 126 sont la sortie de `build.py`, pas du texte écrit à la main |
| `docs/pdf/argumentaire.html` | 0 | 0 | **conforme** — aucune valeur écrite dans le document, ce que `DIRECTION-ARTISTIQUE.md:172-175` exige |

## 6.6 La couleur qui porte du texte

`secteurs.html:377-379` range dans l'**invariant** que `fail` « reste la seule
couleur autorisée à porter du texte ». `DIRECTION-ARTISTIQUE.md:98` dit la même
chose. Le code écrit du texte coloré à **24 endroits** :

| Couleur | Sites | Où |
|---|---|---|
| `kAmber` | 17 | `ui/panels.cpp:634, 637, 640, 1360, 1956, 1961, 2796, 3125, 3151, 3387, 3494, 3554, 4593, 4599, 4652`, plus deux dans `draw_mapping_list` |
| `kSlate` | 3 | `:533, 2798, 4658` |
| `kSage` | 3 | `:648, 1372, 3553` |
| `kAlert` | 2 | `:644, 3940` |
| `kAccent` | 1 | `:549` — le nom du produit dans la barre haute |

Les deux `kAlert` sont conformes (c'est `fail`). Les quatre « A » / « B »
(`:2796, 2798, 4652, 4658`) sont la paire opératoire de la variable 4 — reste à
savoir si elle a le droit de porter du texte, ce que `secteurs.html:418-425` ne
dit pas. Les dix-huit autres ne sont couvertes par aucune ligne.
→ **fermé** (`3a278f2`, SCRATCHVJ-10) — « A » et « B » gardent leur couleur (`DIRECTION-ARTISTIQUE.md`, « Ce qui compte comme du texte ») ; les phrases reviennent à la craie ; le nom du produit devient capuchon 2 px + nom en craie (`kAccentCap`, d'après `rhythm.accent_cap`).

## 6.7 Le champ creusé contre le champ relevé

`tokens.json:8` nomme `raised` `#1A1F1C` : « champs de saisie, lignes survolées,
état actif » — plus **clair** que le panneau. Le code fait l'inverse : `kWell`
`#0E0E0C` (`ui/panels.cpp:24`) est plus **sombre** que `kPanel` `#1A1917`, et
sert de fond aux champs (`:4746`), aux boutons (`:4749`) et aux options non
choisies (`:279`). Le nom même — *well*, un puits — dit le geste inverse.

Ce n'est aucune des sept variables. `tokens.json` v3 l'écrit désormais lui-même,
sous `lines.scene.chassis.raised` : « HORS TEST, et c'est une question distincte…
une convention de relief, qu'aucune des sept variables du cadran ne couvre.
Ouverte. » La valeur en service est donc transcrite telle quelle, avec cette
réserve écrite à côté d'elle, et elle change en une ligne.
→ **fermé par transcription** (`c4fa93c`, phase 5) ; la **convention** reste
ouverte côté maison, et ce n'est pas à ce dépôt de la trancher.

---

# Axe 7 — les rayons

| Où | Valeur | Ce que dit la source | État |
|---|---|---|---|
| `ui/panels.cpp:4724` | `WindowRounding = 0.0f` | `radius_panel: 0` (`tokens.json:22`) ; « angle vif sur les panneaux » (`secteurs.html:371`) | **conforme** |
| `:4725` | `ChildRounding = 0.0f` | idem | **conforme** |
| `:4728` | `ScrollbarRounding = 0.0f` | — | **conforme** |
| `:4726` | `FrameRounding = 1.0f` | `radius_control: 3` (`tokens.json:22`) ; « rayon minimal sur les contrôles » (`secteurs.html:371`) ; « le rayon est 3px sur un contrôle » (`DIRECTION-ARTISTIQUE.md:246`) | **fermé** (`d138ac3`, SCRATCHVJ-12) — `kControlRadius = 3.0f`, nommé d'après `rhythm.radius_control`, sur les deux champs de style et les six rectangles de contrôle dessinés à la main |
| `:4727` | `GrabRounding = 1.0f` | idem | **fermé** (`d138ac3`, SCRATCHVJ-12) — `kControlRadius = 3.0f`, nommé d'après `rhythm.radius_control`, sur les deux champs de style et les six rectangles de contrôle dessinés à la main |
| `:230-231` | `AddRectFilled(..., 1.0f)`, `AddRect(..., 1.0f)` | le bouton dessiné à la main, au même rayon | **fermé** (`d138ac3`, SCRATCHVJ-12) — `kControlRadius = 3.0f`, nommé d'après `rhythm.radius_control`, sur les deux champs de style et les six rectangles de contrôle dessinés à la main |
| `docs/pdf/_style.css:135, 147, 223` | `1.2mm`, `1.5mm`, `1mm` | rayons de `code`, `pre`, `.tag` dans le document | **remonté SCRATCHVJ-17** — la phase 7 est rendue par la note ; rien à faire tant que `suite.css` n'a pas son papier sombre — `suite.css:118,121` emploie `3px` |

`secteurs.html:457` range le rayon des angles parmi ce qui **n'entre pas** dans
le cadran : ce n'est donc pas une valeur de scène, c'est un écart. Mais
l'invariant tel que `secteurs.html:371` l'écrit dit « rayon minimal », et 1 px
est plus minimal que 3. La contradiction est entre `secteurs.html` et
`tokens.json`, pas entre la source et le code. → **fermé** (`d138ac3`, SCRATCHVJ-12) — `kControlRadius = 3.0f`, nommé d'après `rhythm.radius_control`, sur les deux champs de style et les six rectangles de contrôle dessinés à la main.

Les autres rayons du dépôt sont des **rayons de dessin**, pas des rayons
d'angle : voyants (`ui/panels.cpp:303, 323, 500, 668, 3357, 3667, 4633`), arcs de
potard (`:2585, 2625-2644`), poignée de géométrie (`:3873-3882`), cercle de scope
(`:1858`). **Hors périmètre.**

---

# Axe 8 — les fontes et les tailles

## 8.1 Fontes

| Où | Fonte | Rôle | État |
|---|---|---|---|
| `ui/main_ui.cpp:258` | `Archivo-Variable.ttf` à 15 px | les mots | **conforme** — `DIRECTION-ARTISTIQUE.md:62-63`, `secteurs.html:365-368` |
| `ui/main_ui.cpp:259` | Archivo à 11,5 px | les sur-titres | **conforme** sur la famille ; la taille → **fermé** (`c4fa93c`, phase 5, SCRATCHVJ-04) — le corps est à 15 px, valeur de source ; 11,5 est arrondi à **13**, le cran voisin du jeu de ce produit (`scale_candidat`), **substitut noté** dans le fichier de jetons. Les cinq crans restent ouverts côté maison : ce qui manque est la règle de dérivation entre salles |
| `ui/main_ui.cpp:261` | `DMMono-Regular.ttf` à 15 px | les valeurs | **remonté SCRATCHVJ-06** — *reporté*, et reporté sur la mono, pas sur la luminance. La direction est tranchée : luminance de l'atelier, teinte de la scène, test « luminance relative WCAG à ±5 % du jeton d'atelier correspondant, dérive chaude R > G > B » (`tokens.json` v3, `lines.scene.chassis.$test`) ; `#0D0C0A` passe. Ce qui manque : le moteur d'impression de Chrome sur **Fragment Mono contre DM Mono**, une heure. Qui doit le produire : ce produit, seul à tenir une chaîne PDF réelle. Rien ne change d'ici là ; en phase 5, la mono et le fond s'isolent en une ligne chacune (question 2) — `tokens.json:16` prescrit Fragment Mono |
| `ui/fonts/DMMono-Medium.ttf` | — | **jamais chargée par le code** | **hors périmètre** — elle n'est pas orpheline : `docs/pdf/_style.css` la déclare en `font-weight: 600` pour le document. La règle des deux fontes porte sur la distinction mot / valeur, pas sur le nombre de fichiers ; retirer la graisse casserait la chaîne PDF |
| `docs/pdf/_style.css:28` | `"Segoe UI", system-ui, …` | les mots, dans le document | **remonté SCRATCHVJ-17** — la phase 7 est rendue par la note ; rien à faire tant que `suite.css` n'a pas son papier sombre — la raison est écrite en `:1-3` : Chrome refuse `Archivo-Variable.ttf` à l'impression |
| `docs/pdf/_style.css:4-5, 74, 98, 131, 141, 183, 219, 232` | `"DMMono"` | les valeurs, dans le document | **remonté SCRATCHVJ-17** — la phase 7 est rendue par la note ; rien à faire tant que `suite.css` n'a pas son papier sombre |

**La règle des deux fontes est tenue.** C'est la constante la plus forte de
l'invariant (`secteurs.html:365-368`), et elle est respectée aux deux endroits :
`push_mono()` / `push_small()` / `pop_font()` (`ui/panels.cpp:37-39`) encadrent
chaque valeur, et `readout()` (`:102-115`) impose la structure — sur-titre en
Archivo, valeur en mono, unité en petit à côté. Dans le document,
`_style.css:183-184` fait la même chose (`td.mono`, `td.num`).

La substitution est déclarée partout : `ui/panels.h:322-324` (« a missing font
file degrades to the default face »), `_style.css:74` et suivantes
(`ui-monospace, monospace`). C'est ce que `DIRECTION-ARTISTIQUE.md:66-69`
demande — « la **distinction** survit même quand les fontes exactes ne survivent
pas ».

## 8.2 Tailles

| Taille | Où | Dans l'échelle ? | État |
|---|---|---|---|
| 15 px | `ui/main_ui.cpp:258, 261` | non | **conforme par le cadran** pour le corps — variable 1, `secteurs.html:396` ; hors échelle → **fermé** (`c4fa93c`, phase 5, SCRATCHVJ-04) — le corps est à 15 px, valeur de source ; 11,5 est arrondi à **13**, le cran voisin du jeu de ce produit (`scale_candidat`), **substitut noté** dans le fichier de jetons. Les cinq crans restent ouverts côté maison : ce qui manque est la règle de dérivation entre salles |
| 11,5 px | `ui/main_ui.cpp:259` | non | **fermé** (`c4fa93c`, phase 5, SCRATCHVJ-04) — le corps est à 15 px, valeur de source ; 11,5 est arrondi à **13**, le cran voisin du jeu de ce produit (`scale_candidat`), **substitut noté** dans le fichier de jetons. Les cinq crans restent ouverts côté maison : ce qui manque est la règle de dérivation entre salles |
| ×1,55 | `ui/main_ui.cpp:267` | — | **hors périmètre** — facteur de repli quand les fichiers de fonte manquent |
| 28 px | `kParamRow` | c'est `param_row` | **fermé** (`c4fa93c`, phase 5) — `rhythm.param_row` est de l'invariant et `lines.scene` ne le redéfinit pas ; voir la variable 1 |
| 44 px | `ui/panels.cpp:2096, 2382` | c'est la cible scène | **conforme par le cadran** — `secteurs.html:396` |
| 40 px | la barre haute | — | **fermé** (`c4fa93c`, phase 5) — `tokens.json` v3 donne les deux et dit laquelle décide : `chassis_bar.own_decoration` vaut 40 là où le produit dessine son propre cadre, `host_frame` 32 là où l'hôte le fournit. Ce produit dessine le sien. `kChassisBar` |
| 22 px | absent | — | le pied de fenêtre de `ERGONOMIE.md:117` n'existe pas ; la bande d'erreur est dans la barre haute (`ui/panels.cpp:644`) |
| 7,2 à 30 pt | `docs/pdf/_style.css` | non — dix-sept tailles distinctes | **remonté SCRATCHVJ-17** — la phase 7 est rendue par la note ; rien à faire tant que `suite.css` n'a pas son papier sombre |

Le rythme : `CellPadding (8,4)`, `ItemSpacing (10,6)`, `ItemInnerSpacing (6,4)`,
`WindowPadding (18,16)`, `FramePadding (8,4)` — `ui/panels.cpp:4732-4736`. Unité
de 4 px de `tokens.json:21` : respectée par sept des dix nombres ; `10`, `6` et
`18` ne le sont pas, et ils sont désormais **nommés** dans le fichier de jetons
(partie 2) plutôt que régularisés. La colonne de paramètres de 280 px (`ERGONOMIE.md:112`)
n'existe pas — l'écran est le miroir de la table de mixage
(`ui/panels.cpp:4682-4684`), pas un viseur avec une colonne. → **fermé** (`c4fa93c`,
phase 5) pour les dix nombres, qui vivent en partie 2 du fichier de jetons, chacun
nommé, avec la raison écrite de ne pas les régulariser ; **sans objet** pour la
colonne de 280 px, que ce produit n'a pas. Et la grille de la scène → **remonté SCRATCHVJ-05** — *reporté*. Ce qui manque : la définition d'un étage et ses valeurs, éprouvées sur un **second produit de scène**, que `maison/05-QUI-EST-QUI.md` ne connaît pas encore. Qui doit le produire : la maison, au tour 01 du second, après l'avoir nommé ou retiré la phrase de `secteurs.html`. En attendant, en phase 5 : nommer les trois étages dans le fichier de jetons — quel panneau à quel étage — sans en inventer les valeurs, et ne pas régulariser `10`, `6`, `18`.

## 8.3 La casse des libellés

`ERGONOMIE.md:382-391` : « Capitale initiale seulement. Pas de capitales à tous
les mots, pas de point final. **La règle porte sur les libellés que le produit
écrit lui-même** » — formulation issue de `DIORAMA-01`. Ce produit écrit tous ses
libellés lui-même : la clause de l'hôte ne le protège pas.

| Genre | Sites | Casse | État |
|---|---|---|---|
| sur-titres de panneau — `eyebrow()` | 39 (`ui/panels.cpp:53-59`) | capitales | **conforme** — `DIRECTION-ARTISTIQUE.md:200-201` prescrit « titre de panneau à 125 condensé **en capitales** », et `suite.css:82-83` fait de même en `h2` |
| onglets d'écran | 6 (`ui/panels.cpp:4808-4830`) | capitales | **conforme** — même raison : ce sont des titres de panneau |
| libellés de paramètre — `row_label()` | 24, dont **20 en capitales** | capitale initiale | **fermé** (`ef3cbbc`, phase 2, SCRATCHVJ-13) — les 20 capitales et les 4 phrases prennent la capitale initiale ; `readout()` écrit un sur-titre et garde les siennes, par la frontière mécanique de la source |
| libellés de curseur | 5 : `Lacet`, `Tangage`, `Roulis`, `Champ`, `Zoom` | capitale initiale | **fermé** (`ef3cbbc`, phase 2, SCRATCHVJ-13) |
| boutons | 40 sites | capitale initiale, verbe à l'infinitif (`Importer…`, `Créer`, `Rescanner`, `Effacer`) | **conforme** — `ERGONOMIE.md:392-393` |

Aucun emoji, aucun point d'exclamation, aucun « Oups », aucun « Veuillez », aucun
tutoiement, aucune abréviation `tb` / `sbs` / `ou` dans un libellé
(`ERGONOMIE.md:394-396`). **Conforme.**

---

# Axe 9 — les feuilles de style documentaires

**Une seule**, et elle est propre au produit : `docs/pdf/_style.css`, 254 lignes,
liée par `docs/pdf/manuel.html:6` et `docs/pdf/argumentaire.html:6`. Ni l'un ni
l'autre HTML ne porte de bloc `<style>` local — vérifié, zéro occurrence.

Un second producteur de style existe : `docs/pdf/build.py:74,76` injecte des
attributs `style=` dans les annexes régénérées, avec deux valeurs hexadécimales
écrites en clair.

Écart par écart, contre la feuille commune :

| Ce qui diverge | `_style.css` | `suite.css` | État |
|---|---|---|---|
| le papier | `#f7f5f1` clair, couverture `#141412` en aplat | `--paper #F6F7F4` clair, aucun aplat (`:190-192`) | variable 6 : **ne coïncide pas** ; **remonté SCRATCHVJ-17** — *reporté*, voir « Variable 6 » : la maison écrira `.line-scene` dans `suite.css` quand `SCRATCHVJ-06` sera rendue ; `_style.css` ne se réécrit pas |
| l'accent | `#b45f1c` (`:13`) | posé par la classe produit (`:53-61`) — aucune classe pour ce produit | **remonté SCRATCHVJ-17** — la feuille locale ne se réécrit pas, et le verdict 07 range ce second orange en **phase 7**. Côté interface, la ligne est fermée (`c4fa93c`) : l'accent du produit vit derrière un nom unique dans `scratchvj/ui/tokens.h`, prêt à changer le jour où le produit est nommé. `#b45f1c` reste un **second orange**, distinct de `#C9762F`, ce que `DIRECTION-ARTISTIQUE.md:164-166` interdit |
| les fontes | Segoe UI + DM Mono (`:1-5, 28`) | Archivo + Fragment Mono (`:46-47`) | **remonté SCRATCHVJ-17** — la phase 7 est rendue par la note ; rien à faire tant que `suite.css` n'a pas son papier sombre — la raison est écrite, `:1-3` |
| le corps | 10,2 pt (`:25`) | 16 px (`:69`) | **remonté SCRATCHVJ-17** — la phase 7 est rendue par la note ; rien à faire tant que `suite.css` n'a pas son papier sombre |
| l'échelle | dix-sept tailles de 7,2 à 30 pt | 11/12/13/16/28 (`:78, 82, 87, 116, 132`) | **remonté SCRATCHVJ-17** — c'est l'échelle du **document**, donc la feuille locale, donc la phase 7. Celle de l'interface est fermée (`c4fa93c`) |
| la couverture | `.cover` en aplat sombre (`:40-49`) | `.cover` en filet de 2 px (`:194-216`), ajoutée par `DIORAMA-07` | **remonté SCRATCHVJ-17** — la phase 7 est rendue par la note ; rien à faire tant que `suite.css` n'a pas son papier sombre — la classe existe désormais, elle n'est pas employée |
| le sommaire | `.toc` propre, sur deux colonnes (`:230-233`) | `.toc` commun (`:218-240`), ajouté par `DIORAMA-07` | **remonté SCRATCHVJ-17** — la phase 7 est rendue par la note ; rien à faire tant que `suite.css` n'a pas son papier sombre |
| les aplats colorés | `.tag.ok`, `.tag.part`, `.tag.no` (`:226-228`) : fonds verts, ambrés, gris | `.note` en filet, jamais un aplat (`:151-159`) | **remonté SCRATCHVJ-17** — la phase 7 est rendue par la note ; rien à faire tant que `suite.css` n'a pas son papier sombre — écart 20 de `DIVERGENCES.md:155` |
| les couleurs sémantiques | `.warn` en `#9a2f2f`, un **rouge** pour un avertissement (`:205-206`) | `--warn #8A5C22` (`:40`) | **remonté SCRATCHVJ-17** — la phase 7 est rendue par la note ; rien à faire tant que `suite.css` n'a pas son papier sombre — écart 21 de `DIVERGENCES.md:156` ; `warn` en rouge inverse le sens de `tokens.json:34` |
| les valeurs écrites | 29 valeurs hexadécimales | aucune tolérée (`DIRECTION-ARTISTIQUE.md:172-175`) | **remonté SCRATCHVJ-17** — la phase 7 est rendue par la note ; rien à faire tant que `suite.css` n'a pas son papier sombre |

Les deux HTML **sont** des surfaces de marque au sens tranché par `DIORAMA-08` :
`DIRECTION-ARTISTIQUE.md:153-157`, « tout document qui peut être envoyé à
quelqu'un qui n'est pas l'auteur porte `docs/suite.css` ». Un manuel de 33 pages
et un argumentaire de 13 pages (`docs/pdf/README.md:3-4`) circulent par
définition.

---

# Axe 10 — la précision d'affichage, l'erreur, la progression

`MAISON.md:36` : ces trois-là « s'appliquent en entier », sans réserve de salle.

## 10.1 La précision d'affichage · `ERGONOMIE.md:157-170`

| Grandeur | Ce que dit la source | Ce que fait le code | État |
|---|---|---|---|
| angle | une décimale, signe explicite (`:161`) | `%+.0f°` — `ui/panels.cpp:2253` ; `"%.0f°"` sur les curseurs — `:2279-2283` | **fermé** (`b025d6a`, SCRATCHVJ-14) — `%+.1f°` sur le résumé et les trois curseurs d'angle, `%.1f°` sur le champ ; le zoom, qui n'est pas un angle, garde `%.2f` |
| timecode | `hh:mm:ss:ff` (`:165`) | `mm:ss.d` — `clock_of()`, `ui/panels.cpp:67-74` ; `mm:ss` — `short_clock()`, `:76-81` | **remonté SCRATCHVJ-14** — *reporté*. Ce qui manque : le repère temporel de la ligne scène (tempo, horloge audio ou position du plateau). Qui doit le produire : le fondateur, sur la recommandation que la phase 3 de ce produit a écrite — `docs/repere.md`, rendue le 2026-09-10. `mm:ss.d` reste tel quel ; le format est une conséquence du repère, pas un sujet à part, et il attend la décision |
| durée | `h min s`, jamais de décimales (`:166`) | `%.1f s` — la longueur de l'anneau live, en trois sites | **hors périmètre** — la ligne de la source dit « durée d'un **travail** ». Ces trois nombres sont la **longueur d'un tampon** en secondes, la grandeur qu'on scratche : « 4 s d'historique » écrit sans décimale perdrait la mesure, et en `h min s` la rendrait illisible. Requalifié, pas corrigé |
| chemin | tronqué **par le milieu** (`:167`) | jamais tronqué : `ui/panels.cpp:1229, 1449, 1450, 1591` affichent le chemin entier | **conforme** — la règle porte sur un chemin *tronqué* ; aucun ne l'est. `pad_word()` (`:873-878`) coupe un **nom**, pas un chemin |
| résolution | `×` cadratin, espaces fines (`:164`) | `%ux%u` — `ui/tools/xr_check.cpp:178` | **hors périmètre** — un outil de contrôle en console, pas l'interface |
| unité jamais dans le libellé | `:136-139` | quatre sites, tous corrigés | **fermé** (`9b88327`, phase 4) — « Porteuse » avec `Hz` passé au format de la valeur ; « lacet », « tangage », « champ » dans le registre des destinations perdent « (degrés) ». `ERGONOMIE.md`, « La ligne de paramètre » : l'unité change avec la valeur, le libellé non |

Les nombres sont des mesures et non des scénarios (`secteurs.html:372-374`) : le
BPM à une décimale (`ui/panels.cpp:607`), la mémoire vidéo calculée depuis la
capacité réelle (`:4665-4672`), la confiance du décodeur (`:571-573`), et surtout
les **contrôles fantômes** — un potard absolu dont la position est inconnue est
dessiné en pointillé plutôt qu'à une valeur inventée (`meter(..., ghost)`,
`ui/panels.cpp:117-140` ; `dashed_arc`, `:2579-2591`). **Conforme**, et c'est le
« détail que seul ce métier connaît » que `DIRECTION-ARTISTIQUE.md:218-223`
demande — un par écran.

## 10.2 L'erreur · `ERGONOMIE.md:340-356`

**Aucune modale dans tout le produit.** Recherche de `BeginPopupModal`,
`MessageBox`, `SDL_ShowSimpleMessageBox` : zéro occurrence. Les trois seuls
popups sont non modaux : `takes` (`ui/panels.cpp:696`), `newcrate` (`:1278`),
`gaze` (`:2267`). **Conforme** à `ERGONOMIE.md:340-343` et à `MAISON.md:181`.

L'erreur s'affiche en bande, dans la barre haute : `ui/panels.cpp:644`,
`text_c(kAlert, ...)`, et `:3940` pour la sortie. La couleur est `fail`, la seule
autorisée à porter du texte. **Conforme.**

Le contenu, en revanche, ne dit pas toujours les trois choses de
`ERGONOMIE.md:345-346` :

| Erreur | Nomme la valeur ? | Nomme la réparation ? | État |
|---|---|---|---|
| `app/analyze.cpp:40` — « ffprobe a échoué — ffmpeg est-il installé et dans le PATH ? » | oui | **oui** | **conforme** |
| `ui/main_ui.cpp` — « aucun écran ne correspond à `--output` … » | oui | **oui** | **fermé** (`9b88327`, phase 4) — les écrans vus sont énumérés, et l'action est « choisissez-en un dans SORTIE » |
| `ui/main_ui.cpp` — « <nom> : cache illisible » | oui | **oui** | **fermé** (`9b88327`, phase 4) — relancer l'analyse depuis BIBLIOTHÈQUE, ou supprimer le `.svcache` et réimporter la source |
| `ui/main_ui.cpp` — « rien d'ouvrable dans … » | **oui** | **oui** | **fermé** (`9b88327`, phase 4) — les fichiers déposés sont nommés (par leur nom, jamais par un chemin coupé par la fin), et la phrase dit ce qui s'ouvre |
| `ui/main_ui.cpp` — « `mapping.json` : destination inconnue : <id> » | oui | **oui** | **fermé** (`9b88327`, phase 4) — corriger ou retirer la liaison ; les destinations valides sont dans TABLE |

## 10.3 La progression · `ERGONOMIE.md:323-326`, `MAISON.md:182`

Deux affichages de progression, tous deux **en pourcentage seul** :

- `ui/panels.cpp:969` — `"analyse %.0f %%"`, sur la vignette d'un clip ;
- `ui/panels.cpp:1360` — la même chaîne, dans la liste de la bibliothèque.

Ni la fraction faite, ni l'étape nommée, ni le temps restant.
**Hors périmètre — au carnet du produit**, et c'est la maison qui l'y a rangé :
`SCRATCHVJ-20` classe « aucun pourcentage seul » comme un **ajout** — il faut
produire l'étape nommée et le temps restant, dont ni l'un ni l'autre n'existe
dans `app/analyze` — et « ajouter ne l'est que si une phase le porte ». Aucune
phase du talon ne le porte. L'entrée est écrite dans `docs/roadmap.md`, avec son
coût : trois étapes à déclarer et à remonter par `AnalysisReport`, un débit de
frames à mesurer et à lisser, deux sites d'affichage.

À côté, `ui/panels.cpp:634-640` affiche bien « analyse en cours · N en
attente » : la fraction y est. Les états canoniques d'`ERGONOMIE.md`, « Le travail long » — `pending`,
`running`, `failed` — s'écrivent désormais « En attente », « En cours »,
« Échoué ». **Fermé** (`ef3cbbc`, phase 2). Un clip que personne n'a demandé
n'est pas un travail et n'a pas d'état canonique : il dit « Non analysé », dans
ses propres mots. La **progression** elle-même reste ouverte (phase 4).

## 10.4 Les infobulles

64 appels à `SetTooltip` dans `ui/panels.cpp`, contre 40 boutons, 17 sélecteurs
segmentés et une vingtaine de potards, faders et pads.
`secteurs.html:375-376` interdit « l'infobulle **partout** », pas
l'infobulle. La plupart portent les chiffres qu'un voyant ne montre pas — ce que
`ui/panels.cpp:297-298` écrit comme règle : « The figures behind it belong in a
drawer or a tooltip, not on the bar », ce qui suit
`DIRECTION-ARTISTIQUE.md:218-223`.

Trois décrivaient un bouton dont le glyphe est déjà la convention du métier —
« retour au début », « lecture (espace) », « stop : pause et retour au début ».
**Fermé** (`9b88327`, phase 4) : les trois sont supprimées. Le test de
`DIRECTION-ARTISTIQUE.md` les condamne — nommer un glyphe de transport est
exactement la décision que prendrait quelqu'un qui n'a jamais joué — et la seule
chose que le glyphe ne portait pas, le raccourci, vit désormais dans la carte du
clavier de `?` / `F1`. **Conforme** pour les 61 autres, qui portent les chiffres
qu'un voyant ne montre pas.

## 10.5 Ce qui manque exprès · `secteurs.html:375-376`

Vérifié un par un : aucun accueil guidé, aucune illustration d'état vide (les
puits vides « disent ce qu'ils attendent », `app/engine.cpp:240`), aucun thème
clair, aucune fenêtre flottante (`ui/panels.cpp:4791-4793` : une fenêtre sans
décoration, plein cadre, `NoSavedSettings`), aucun fichier de disposition
(`ui/main_ui.cpp:231`, `IniFilename = nullptr`, avec la raison écrite).
**Conforme**, et c'est aussi le châssis d'`ERGONOMIE.md:81-83`.

---

# Axe 11 — la licence, déclarée dans les deux sens

`MAISON.md:110` demande de vérifier que le README et la vitrine disent que
l'utilisateur a droit à la source, et que rien ne prétend vendre autre chose que
les builds. `secteurs.html:617-621` a tranché le 9 septembre : « **La GPL est
gardée et assumée comme stratégie.** »

| Où | Ce qui est écrit | État |
|---|---|---|
| `README.md` | « **GPL-3.0**, and it is a decision rather than an accident » ; « if you have a build, you have the right to its source » ; ce qui est vendu ; la frontière `dvs/` | **fermé** (`4670820`, phase 6) — le premier sens y était, le second manquait |
| `dvs/decoder.h` | la frontière documentée en tête | **conforme** — `secteurs.html:629-633` |
| `docs/pdf/argumentaire.html`, § 07 | le chapô dit désormais « **tranché depuis le 9 septembre 2026** » ; les trois voies deviennent celle qui a été retenue, avec sa raison commerciale ; l'acheteur lit qu'il a droit à la source, « c'est un argument, pas un aveu » ; « Modèles envisageables » devient « **Ce qui se vend** » — le binaire, les mises à jour, et que ça marche le soir même | **fermé** (`4670820`, phase 6) |
| `docs/pdf/argumentaire.html`, l'encadré des affirmations qui engagent | « Le choix, lui, est fait… ce qui reste à faire relire par un juriste est l'**analyse** qui l'accompagne » | **fermé** (`4670820`, phase 6) — une décision prise n'est pas une décision vérifiée, et les deux ne se rangent pas au même endroit |
| `docs/pdf/manuel.html`, la couverture et l'annexe E | « **GPL-3** — vous avez droit à la source » ; l'annexe E ouvre sur la décision et sur ce qu'elle donne au lecteur ; le tableau distingue le **défaut de compilation** de **la configuration distribuée** | **fermé** (`4670820`, phase 6) — la phrase disait le contraire de la décision : xwax reste |

Les deux documents que l'acheteur lisait avant d'installer quoi que ce soit
contredisaient la décision de la maison. C'était du travail de produit, pas une
question à remonter, et c'est fait (`4670820`, phase 6).

---

# Confrontation avec `design/DIVERGENCES.md`

`DIVERGENCES.md` a été relevé sur **sept dépôts d'atelier** le 2026-09-08, et
complété par Diorama le 2026-09-09 (`:6-10`). Ce produit n'y figure pas : il
n'est pas de la suite. La confrontation demandée par `MAISON.md:127-128` porte
donc sur les 22 écarts, un par un.

## Ce qui y est et qu'on ne retrouve pas ici

| Écart | Pourquoi il est absent |
|---|---|
| 1 — Georama inverse le tangage | pas de viseur à la souris ; le tangage est un curseur (`ui/panels.cpp:2280`) |
| 2 — recentrer sur `0` / `H` / `R` | pas de recentrage : le regard revient par les curseurs |
| 3 — `←`/`→` valent ±1 image | ni `←` ni `→` ne sont liées |
| 5 — la comparaison avant/après | le produit n'en a pas |
| 6 — le bouton d'orbite | pas d'orbite |
| 7 — quatre unités de vitesse de rotation | une seule, le degré, sur quatre destinations (`core/destinations.cpp:17-20`) |
| 9 — deux fonds sombres inconciliables | un seul fond, `#141412`, écrit deux fois (6.2) |
| 10 — trois accents violets presque identiques | un seul accent |
| 11 — `#46465a` en dur quatorze fois | le filet est un jeton nommé, `kHair` |
| 14 — le panneau UXP duplique la palette | pas d'hôte Adobe |
| 15 — Vigie n'a aucun thème | `apply_style()` retune les 44 couleurs d'ImGui, onglets compris (`:4761-4769`) |
| 17 — la feuille QSS de Relief | pas de Qt |
| 19 — quatre modèles d'intégration Adobe | aucun |
| 20 — boutons primaires en aplat coloré | **existe, et c'est une conformité** : variable 2, `secteurs.html:477-479` |

## Ce qu'on trouve ici et qui n'est nulle part dans `DIVERGENCES.md`

Neuf constats propres à ce produit, ou à la ligne scène :

1. **`Échap` ferme la fenêtre** (`ui/main_ui.cpp:673`). L'écart 8 note que la
   pile est réimplémentée trois fois ; nulle part qu'un produit y ajoute un rang
   qui quitte.
2. **La couleur porte du texte à 24 endroits** (6.6). L'écart 21 parle de
   couleurs sémantiques divergentes, pas de leur emploi sur du texte.
3. **Le champ est creusé, pas relevé** (6.7). Aucun des 22 ne le décrit.
4. **`signal.warn` n'a aucune valeur** dans le produit (6.1).
5. **Le pourcentage seul** (10.3). Absent des 22.
6. **Une échelle typographique de deux valeurs hors barème**, 15 et 11,5 (8.2).
   L'écart 13 parle de l'absence de chasse fixe, pas des tailles.
7. **Une couverture en aplat sombre** (`_style.css:42`). L'écart 22 élargi par
   `DIORAMA-09` décrit exactement cela chez Diorama : c'est donc **le second
   produit** où on la trouve, et la première fois hors atelier.
8. **Deux documents commerciaux contredisent une décision de la maison**
   (axe 11).
9. **Un accent documentaire distinct de l'accent d'interface** — `#b45f1c`
   contre `#C9762F`. Chez Diorama c'était un second accent doré
   (`DIVERGENCES.md:159-163`) ; ici c'est le même orange, décalé.

Les points 7 et 9 sont la même maladie que l'écart 22, dans un troisième dépôt.
Cela mérite d'être dit comme tel : ce n'est plus un accident, c'est un motif.

---

# Ce que le tour 01 de Diorama a déjà tranché · questions non reposées

`REGISTRE.md:9-11` : « Si la question qui monte est déjà ici, la réponse existe :
on cite la ligne de source qu'elle a produite, on ne la rejoue pas. » Six
questions que ce relevé aurait posées ont déjà leur verdict.

| Question | Verdict | La ligne de source qui en est sortie | Ce que ça change ici |
|---|---|---|---|
| **Le repère : `+Z` ou `−Z` ?** `00-vocabulaire.md` disait l'inverse de `02-repere.md` | tranché, trouvé en chemin (`diorama-01-reponses.md:512`) | `00-vocabulaire.md:80-83` — « X droite, Y haut, **le regard va vers `−Z`** ; le centre d'une équirectangulaire regarde `−Z` », et la correction est datée en `:85-89` | `core/sphere.h:13` et `sphere.cpp:43-48` sont **conformes**. La question ne se pose plus |
| **Le nom de la maison** : `NOMS.md` annonçait *Theama* arrêté, `secteurs.html:125` dit *Precession* en tête | tranché via `DIORAMA-03` | `NOMS.md:181-187` et `:219` — Theama abandonné, nom de maison **rouvert** ; la contradiction avec `business/01-architecture-de-marque.md` est écrite en `:198-200` | Rien de public ne se rédige avec un nom de maison. Pas de remontée |
| **Un document de travail interne porte-t-il `suite.css` ?** | tranché | `DIRECTION-ARTISTIQUE.md:148-160` — le critère est la **circulation** | `manuel.html` et `argumentaire.html` en sont, sans discussion. Axe 9 |
| **`suite.css` n'a ni couverture ni sommaire** | tranché | `suite.css:181-240` — `.cover`, `.cover.ruled`, `.toc` | La phase 7 n'a plus à les inventer ; il reste le papier sombre. SCRATCHVJ-17 |
| **Un produit qui ne lie aucune touche doit-il en lier ?** | tranché | `ERGONOMIE.md:233-238` | Ce produit en lie quatre : la clause ne le dispense pas. Axe 5.3 |
| **`docs/` contre `Docs/`** | tranché | `diorama-01-reponses.md:459-469` | Ce dépôt porte `docs/`. Aucun écart |

Deux points du même tour ne sont **pas** clos pour la scène, et remontent
autrement :

- le papier sombre : `diorama-01-reponses.md:406-412` établit **à qui il
  appartient**, pas **ce qu'il contient** → SCRATCHVJ-17 ;
- `REGISTRE.md:68` inscrit déjà « le repère temporel de la ligne scène » parmi ce
  qui reste ouvert, et l'attribue au fondateur « sur recommandation de
  scratchvj ». C'est la **phase 3** de ce produit, pas une remontée : il ne faut
  pas demander à la maison ce qu'elle attend d'ici.

---

# Ce qui contredit `MAISON.md` ou ses sources

Sept points, tous instruits dans `REMONTEES-SCRATCHVJ.md`.

1. **`MAISON.md:90` déclare identiques deux règles qui ne le sont pas.**
   « un aplat autorisé, un seul par panneau, toujours porté par un état, jamais
   par l'identité — et c'est déjà, **mot pour mot**, la règle de
   `design/README.md` ». Or `design/README.md:33-34` dit : « l'orange plein est
   réservé à **une seule action** par panneau ». État contre action : ce n'est
   pas le même critère, et le code applique les deux. → SCRATCHVJ-11.

2. **`MAISON.md:101` et `secteurs.html:483` décrivent une interface qui n'existe
   plus.** « les pads sur `1`–`5` » : aucune touche chiffrée n'est liée depuis
   `9ccce6f`. Le conflit annoncé avec `Ctrl`+chiffre n'existe pas.
   → SCRATCHVJ-15.

3. **`secteurs.html` demande de trancher là où `MAISON.md` dit d'attendre.**
   `secteurs.html:663` : les deux accidents « se corrigent **aujourd'hui**, en un
   fichier chacun » ; `:469-473` donne même le test d'arbitrage de la mono.
   `MAISON.md:98-99` les range en couche 3 et ordonne de ne rien changer. J'ai
   suivi `MAISON.md`. → SCRATCHVJ-06.

4. **`secteurs.html` est lui-même hors des deux valeurs de la variable 3.** Il se
   déclare « le premier document de la ligne scène, et il vaut décision »
   (`:440-441`) ; son papier est `#0B0D0C` (`:9`, `:21`), alors que son propre
   nuancier donne atelier `#0A0C0B` et scène `#141412` (`:57-58`, `:121-122`).
   Une troisième valeur. → SCRATCHVJ-17.

5. **`suite.css:260-261` écrit un interdit universel là où il y a une variable de
   cadran.** « Aucun thème sombre. Un manuel se lit et s'imprime sur du clair ;
   le sombre appartient à l'application. » `diorama-01-reponses.md:406-412` vient
   d'établir que le sombre appartient à la **ligne scène**, et n'a corrigé que
   `:191`. → SCRATCHVJ-17.

6. **`ERGONOMIE.md:428` compte dix touches réservées ; la table `:245-258` en
   porte douze lignes.** Le compte a déjà servi de point d'appui fautif une fois
   — `:175-177`, « le compte a déjà servi de point d'appui à une session pour
   croire qu'un produit devait en avoir un ». → SCRATCHVJ-20.

7. **La définition de fini de `MAISON.md` dépasse ses phases.** Quatre exigences
   des lignes `181-183` ne sont couvertes par aucune des huit phases : l'erreur
   qui nomme la valeur et la réparation, la progression sans pourcentage seul,
   l'unité jamais dans un libellé, le chemin tronqué par le milieu. Les trois
   premières ont du travail réel dans ce dépôt (axes 10.1, 10.2, 10.3) et aucune
   phase pour l'accueillir. → SCRATCHVJ-20.

---

# Rapport de phase 0

**Fait.** Les onze axes sont relevés avec `fichier:ligne` pour chaque occurrence.
Les sept variables du cadran sont instruites une par une contre le code. Vingt
remontées partent dans `REMONTEES-SCRATCHVJ.md`. Aucun fichier de code n'a été
modifié ; aucun fichier de `Suite 360` ni d'un autre dépôt n'a été touché.

**Les comptes.**

| | |
|---|---|
| Valeurs de couleur d'interface écrites dans le code | **15** — 12 nommées (`ui/panels.cpp:22-33`), 3 hors table |
| Valeurs de couleur utilitaires (noir, blanc) dans le code | 8 |
| Valeurs hexadécimales hors du code | **963 occurrences, 61 distinctes** — dont 1 166 occurrences / 37 distinctes dans la maquette, et 29 / 25 dans `_style.css` |
| Touches liées | **4** — `Espace`, `F`, `B`, `Échap` |
| Touches du noyau réservé non liées | 3 — `Ctrl`+`Z`, `?` / `F1`, `Tab` |
| Sélecteurs segmentés remplissant un aplat | 17 |
| Boutons remplissant un aplat | 9, dont 4 portés par une action |
| Sites où une couleur porte du texte | 24, dont 2 conformes (`fail`) |
| Feuilles de style documentaires | 1, plus un générateur qui écrit du style local |
| Modales | **0** |
| Animations d'interface | **0** |

**Écarts apparents qui sont des conformités.** **Sept**, et ils portent une part
importante de ce que le produit fait bien : le fond `#141412` et les cinq
couleurs de châssis chaudes (variable 3), l'aplat orange (variable 2), la paire
ambre/ardoise des decks (variable 4), le corps à 15 px et la cible à 44 px
(variable 1), l'absence totale d'animation (variable 5), les capitales sur les
sur-titres de panneau (`DIRECTION-ARTISTIQUE.md:200-201`), et l'emploi de `F`
pour « image seule » (`ERGONOMIE.md:260-265`). Sans le cadran, ce relevé aurait
ouvert sept chantiers dont six auraient **détruit** ce qui rend l'instrument
jouable dans le noir — exactement ce que `secteurs.html:655` interdit.

**Axes vides, et prouvés.** La disposition stéréo (axe 2, 34 faux positifs tous
classés), l'œil (axe 3), le viseur à la souris (axe 5.6), les touches chiffrées
(axe 5.5), les couleurs dans les shaders et dans le plugin Unreal (axe 6.3), les
modales et les animations (axes 10.2 et variable 5). Aucun travail inventé sur
ces sept axes.

**Non fait, et pourquoi.** Le choix entre DM Mono et Fragment Mono, entre
`#141412` et `#0A0C0B` : couche 3 de `MAISON.md:59-66`, et `secteurs.html:663`
demande le contraire — la contradiction est remontée, pas tranchée. Le repère
temporel : c'est la phase 3, et `REGISTRE.md:68` l'attend déjà de ce produit. La
réécriture de `_style.css` : interdite par `MAISON.md:100` et par la consigne du
chantier.

**Ce qui contredit `MAISON.md` ou ses sources.** Sept points, section précédente.
Les trois qui changent le travail à venir :

1. **Le clavier annoncé n'existe plus** — les pads `1`–`5` de `MAISON.md:101` et
   de `secteurs.html:483` ont disparu à la refonte ; le vrai sujet du clavier est
   ailleurs, et c'est `Échap` (SCRATCHVJ-15).
2. **L'aplat repose sur deux règles différentes** que `MAISON.md:90` croit
   identiques ; la phase 5 ne peut pas s'écrire sans savoir laquelle gouverne
   (SCRATCHVJ-11).
3. **L'échelle typographique de la scène n'existe pas**, alors que la variable 1
   impose 15 px et que l'invariant ferme l'échelle à 11/12/13/16/28 : les deux ne
   peuvent pas être vrais ensemble (SCRATCHVJ-04).
