# Remontées — scratchvj, tour 01

Vingt points relevés le **2026-09-09** dans le dépôt `UE_SCRATCHING` (branche
`claude/scratch-video-unreal-0oi7dv`, commit `75eee2f`), pendant la phase 0 de
`MAISON.md`. Le relevé complet est dans `docs/ALIGNEMENT.md` du même dépôt ;
**ce fichier-ci se lit seul.** Chaque point rappelle ce que dit la source avec sa
ligne, ce que fait le code avec la sienne, la question posée, et une
recommandation argumentée.

Aucun de ces points n'a été tranché dans le dépôt. Aucun fichier de code n'a été
modifié. Aucun fichier de `Suite 360` n'a été touché.

## Ce qu'il faut savoir avant de lire

**Ce produit est le premier de la « scène ».** Ce n'est pas un neuvième produit
de la suite 360 : c'est le premier d'une seconde salle, celle où il y a un public
(`docs/secteurs.html` § 2). Ce que la maison répondra ici, elle le répondra pour
tous ceux qui suivront — et `secteurs.html:667` dit que le second existe déjà.
**Quinze des vingt points ci-dessous ne sont pas des questions sur ce
logiciel : ce sont les premiers trous de la ligne scène**, et ils sont signalés
comme tels.

**Ce qui n'est pas remonté, et pourquoi.** Six questions que ce relevé aurait
posées ont déjà reçu leur verdict au tour 01 de Diorama. Elles ne sont pas
rejouées, conformément à `remontees/REGISTRE.md:9-11` :

| Question déjà tranchée | La ligne de source qu'elle a produite |
|---|---|
| le repère : `+Z` ou `−Z` ? | `spec/00-vocabulaire.md:80-83`, corrigé et daté en `:85-89` — le code de ce produit est conforme |
| le nom de la maison — *Theama* ou autre chose ? | `design/NOMS.md:181-187` et `:219` — Theama abandonné, nom rouvert |
| un document interne porte-t-il `suite.css` ? | `design/DIRECTION-ARTISTIQUE.md:148-160` — le critère est la circulation |
| `suite.css` sans couverture ni sommaire | `docs/suite.css:181-240` — `.cover`, `.cover.ruled`, `.toc` |
| un produit qui ne lie aucune touche doit-il en lier ? | `design/ERGONOMIE.md:233-238` |
| `docs/` contre `Docs/` | `tools/gen-maison.py`, dossier réel par produit — ce dépôt porte `docs/`, aucun écart |

**Une question de ce produit ne remonte pas non plus** : le repère temporel de la
ligne scène. `remontees/REGISTRE.md:68` l'inscrit déjà comme ouvert et l'attribue
au fondateur « sur recommandation de scratchvj ». C'est la phase 3 de ce produit,
pas une demande à la maison.

**Sur le nom de ce fichier.** `design/NOMS.md:217-227` arrête huit noms de
produits ; aucune ligne pour `UE_SCRATCHING`. Le nom de travail est employé faute
de mieux. Voir **SCRATCHVJ-19**.

---

## Tableau de triage

| # | Objet | Portée | Fichier à modifier dans `Suite 360` | Bloque une phase ? |
|---|---|---|---|---|
| 01 | La forme canonique de « non déclaré » (`auto`) | **toute la maison** | `spec/00-vocabulaire.md` | **oui — phase 2** |
| 02 | La projection d'un fichier n'est pas la reprojection d'une vue | **toute la maison** | `spec/00-vocabulaire.md` | **oui — phase 2** |
| 03 | Un libellé lisible à un mètre, dans le noir | **ligne scène** | `design/ERGONOMIE.md`, `docs/secteurs.html` | **oui — phase 2** |
| 04 | L'échelle typographique de la scène — 15 px contre une échelle fermée | **ligne scène** | `docs/secteurs.html`, `design/tokens.json` | **oui — phase 5** |
| 05 | La densité en trois étages : la variable 7 n'a aucune valeur | **ligne scène** | `docs/secteurs.html`, `design/tokens.json` | **oui — phase 5** |
| 06 | La luminance commune des deux châssis, et la mono unique | **ligne scène** | `design/tokens.json`, `docs/secteurs.html` | **oui — phase 5** |
| 07 | `lines.scene` n'existe pas : ni accent, ni paire A/B, ni classe documentaire | **ligne scène** | `design/tokens.json`, `docs/suite.css` | **oui — phase 5** |
| 08 | L'ambre porte deux rôles : le deck A et le travail en cours | **ligne scène** | `design/tokens.json` | **oui — phase 5** |
| 09 | Les quatre signaux sur un châssis chaud, et le `warn` manquant | **ligne scène** | `design/tokens.json` | **oui — phase 5** |
| 10 | La couleur qui porte du texte — 24 sites, et la paire A/B en fait partie | **ligne scène** | `docs/secteurs.html` | **oui — phase 5** |
| 11 | L'aplat : deux règles que `MAISON.md` croit identiques | **ligne scène** | `docs/secteurs.html`, `tools/gen-maison.py` | **oui — phase 5** |
| 12 | Le rayon des contrôles : 1 px, 3 px, ou « minimal » | toute la maison | `design/tokens.json`, `docs/secteurs.html` | non |
| 13 | La capitale initiale sur un libellé de paramètre, à un mètre | **ligne scène** | `design/ERGONOMIE.md` | **oui — phase 2** |
| 14 | Le format d'un temps et d'un angle sur un instrument scratché | **ligne scène** | `design/ERGONOMIE.md` | **oui — phase 3** |
| 15 | Le clavier de la ligne scène, et `Échap` qui ferme la fenêtre | **ligne scène** | `design/ERGONOMIE.md`, `docs/secteurs.html`, `tools/gen-maison.py` | **oui — phase 4** |
| 16 | Les constantes `viewer` quand le regard est piloté au potard | toute la maison | `design/tokens.json`, `design/ERGONOMIE.md` | non |
| 17 | Le papier sombre : ce que `suite.css` doit porter | **ligne scène** | `docs/suite.css`, `docs/secteurs.html` | **oui — phase 7** |
| 18 | La détection au ratio seul, et `spec/evidence.py` | propre au produit | `spec/evidence.py` | non |
| 19 | Ni `NOMS.md` ni `suite.css` ne portent de ligne pour ce produit | propre au produit | `design/NOMS.md`, `docs/suite.css` | non |
| 20 | Quatre inexactitudes dans les sources | toute la maison | `design/ERGONOMIE.md`, `tools/gen-maison.py` | non |

**Quinze des vingt valent pour toute la ligne scène.** Deux sont propres à ce
produit (18, 19). Trois valent pour la maison entière (01, 12, 20).

---

## SCRATCHVJ-01 — La forme canonique de « non déclaré »

**Portée : toute la maison.** Tout produit qui devine un format aura la même.

**Ce que dit la source.** `spec/00-vocabulaire.md:5-8` : « la chaîne écrite sur
disque ou passée d'une application à l'autre est **toujours** la forme
canonique ». La table `:21-29` donne `equirect_360` pour la sphère complète et
`flat` pour l'image plate ; `:94` bannit « `equirectangular` seul ». La raison est
écrite en `:35-37` : « la projection équirectangulaire ne dit rien de son
étendue ».

**Ce que fait le code.** Le produit écrit trois valeurs dans son fichier de
bibliothèque, dans un tableau de trois lignes :

- `"flat"` — **conforme**, et c'est la seule forme canonique du catalogue
  effectivement écrite sur disque, tous produits confondus
  (`design/DIVERGENCES.md:56` : « Aucun des huit produits n'écrit une seule des
  formes canoniques ») ;
- `"equirect"` — un écart connu, dont la réparation est écrite : `equirect_360` ;
- `"auto"` — **et là, il n'y a pas de forme canonique à écrire.**

`"auto"` signifie : l'utilisateur n'a rien forcé, la valeur vient de l'analyse du
fichier. C'est un troisième état, distinct de `flat` et d'`equirect_360`, et le
vocabulaire ne le nomme pas. Le produit lit la valeur au chargement et refuse
toute chaîne inconnue avec un message nommant la valeur fautive.

**La question.** Comment s'écrit « non déclaré, à déduire » dans un fichier ? Il y
a trois manières, et elles ne sont pas équivalentes :

1. **une valeur canonique de plus**, `auto` ou `undeclared` ;
2. **l'absence de la clé** — ce que fait déjà le lecteur de ce produit, qui
   traite un champ manquant comme non forcé ;
3. **une clé séparée**, du genre `projection_source: declared | inferred`, ce qui
   rejoint la hiérarchie `declared > metadata > inferred` que
   `design/DIVERGENCES.md:110-120` attribue déjà à Vigie.

**Ma recommandation : la deuxième, l'absence de la clé, et l'écrire dans le
vocabulaire.** Trois raisons. (a) C'est ce que le code fait déjà, dans ce produit
et probablement ailleurs : un champ absent n'a jamais été forcé, un champ présent
l'a été — la règle est déjà vraie, il suffit de l'écrire. (b) Une valeur
canonique `auto` mélangerait deux niveaux dans le même champ : *ce que c'est* et
*d'où on le sait*. (c) La troisième voie est meilleure sur le fond mais elle
n'est pas de ce ressort : elle appartient à `spec/evidence.py`, qui reste ouvert
(`REGISTRE.md:66`), et l'ouvrir ici obligerait huit produits à migrer un fichier
pour une question que personne n'a encore tranchée.

**Ce qui la rend urgente ici.** La phase 2 renomme `equirect` en `equirect_360`
et touche donc le fichier de bibliothèque. Si `auto` doit changer aussi, les deux
migrations se font ensemble — c'est exactement le raisonnement de `DIORAMA-02`,
question 2 : « ils touchent le même fichier et se corrigent ensemble ou pas du
tout ».

---

## SCRATCHVJ-02 — La projection d'un fichier n'est pas la reprojection d'une vue

**Portée : toute la maison.** Six produits de la suite portent un viseur, et un
viseur reprojette.

**Ce que dit la source.** `spec/00-vocabulaire.md:41-48` donne six projections
avec leur domaine et leurs paramètres : `equirect_360`, `equirect_180`,
`fisheye`, `cubemap_3x2`, `eac`, `flat`. `design/ERGONOMIE.md:222-225` ajoute que
« le viseur affiche **toujours une reprojection rectiligne** ».

**Ce que fait le code.** Deux énumérations coexistent et ne parlent pas de la
même chose :

- l'une décrit **comment le fichier est stocké** — plate, ou sphère
  équirectangulaire — et c'est celle de l'axe 1 ;
- l'autre décrit **comment la vue est calculée à partir de la sphère**, et elle
  porte trois valeurs : `Perspective`, `LittlePlanet`, `Fisheye`. À l'écran :
  « Perspective », « Little planet », « Fisheye ».

Deux des trois mots n'existent pas dans le vocabulaire : `perspective` et
`little_planet`. Le troisième, `fisheye`, existe — mais avec un tout autre
sens : dans `00-vocabulaire.md:45`, `fisheye` est **la projection d'un fichier**,
paramétrée par `lens`, `fov_deg`, `center`, `radius`, `count`. Ici c'est un
**mode de rendu** d'une équirectangulaire, sans aucun de ces paramètres.

C'est la même collision que le piège 2 de `design/DIVERGENCES.md:87-90`, où
`flat` désigne chez Lacuna un mode d'affichage et non une projection. Le piège
a été vu une fois ; il n'a pas été traité au niveau où il se produit.

**La question.** Le vocabulaire doit-il nommer la **reprojection d'une vue** — et
si oui, sous quels mots, sachant que `fisheye` est déjà pris par la projection
d'un fichier ?

**Ma recommandation : oui, et dans une table séparée du même document.** Une
section « reprojections » à côté de la section « projections », avec trois
formes : `rectilinear`, `little_planet`, `fisheye_view`. Trois raisons.

1. **La confusion est déjà mesurée deux fois** — ici et chez Lacuna. Un concept
   qui se fait confondre dans deux produits indépendants n'est pas un accident
   local.
2. **`rectilinear` plutôt que `perspective`** : c'est le mot que
   `00-vocabulaire.md:48` emploie déjà pour décrire `flat` (« rectiligne, écran
   virtuel »), et il évite de donner le mot le plus générique de l'image de
   synthèse à un cas particulier.
3. **`fisheye_view` plutôt que `fisheye`** : la désambiguïsation doit être portée
   par le mot le plus récent, pas par celui qui est déjà écrit dans une table
   paramétrée.

**Ce que ça coûte à la suite.** Rien pour l'instant : aucun produit n'écrit ces
mots sur disque. Ce sont pour l'heure des identifiants de code et des libellés.
Le jour où un manifeste transporte un point de vue — ce que
`spec/03-ponts.md` prévoit avec le presse-papiers — il en aura besoin.

---

## SCRATCHVJ-03 — Un libellé lisible à un mètre, dans le noir

**Portée : toute la ligne scène.** C'est la conséquence directe de la variable 1
du cadran, et elle n'a jamais été tirée.

**Ce que disent les sources.** `design/ERGONOMIE.md:364-378` donne la table des
libellés lisibles, un mot par concept, dans les deux langues :

| Canonique | Libellé FR | Libellé EN |
|---|---|---|
| `equirect_360` | Équirectangulaire 360 | Equirectangular 360 |
| `flat` | Plate | Flat |

`spec/00-vocabulaire.md:95` bannit `2d` : « préférer `flat` : une plate 360 est
aussi une image à deux dimensions ». `:93` autorise les abréviations `tb`, `sbs`,
`ou` « aux libellés courts d'interface » — mais pas `2d`.

**Et en face**, `docs/secteurs.html:396` fixe la valeur scène de la variable 1 :
« 1 m dans le noir, corps 15 px, cible 44 px », avec la raison en `:397-399` :
« on jette un œil à la scène pendant qu'une main tient le plateau ».

**Ce que fait le code.** Le produit écrit partout `360°` et `2D`, dans six
endroits distincts : un sélecteur sur l'en-tête de chaque deck, un sélecteur dans
l'inspecteur de la bibliothèque, la barre de filtres, le détail sous la vignette
du deck (« equirect 360 » / « plan 2D »), la bande programme
(« PROGRAMME · 360 PROJETÉ ») et un nom de caisse (« 360° ») **écrit sur
disque**. La sortie console emploie « équirectangulaire », le mot que
`00-vocabulaire.md:94` bannit isolé.

**La question.** « Équirectangulaire 360 » fait 21 caractères. Dans un sélecteur
de deux options posé sur l'en-tête d'un deck, à côté d'un bouton de transport de
44 px, dans une fenêtre partagée entre deux decks et un mixeur — il ne rentre
pas, et à un mètre dans le noir il ne se lit pas non plus : l'œil lit une forme,
pas une chaîne.

**La ligne scène a-t-elle droit à une seconde colonne de libellés — courte —
dans la table d'`ERGONOMIE.md` ?**

**Ma recommandation : oui, une colonne « scène », et le critère qui la gouverne
n'est pas la longueur mais le temps de lecture.** Concrètement :

| Canonique | Libellé atelier | Libellé scène |
|---|---|---|
| `equirect_360` | Équirectangulaire 360 | **360** |
| `flat` | Plate | **Plate** |
| `fisheye` | Fisheye | Fisheye |

Quatre raisons, et la troisième est la plus importante.

1. **La raison métier est déjà écrite** — `secteurs.html:397-399`. Ce n'est donc
   pas une préférence : c'est la variable 1 appliquée aux mots au lieu de l'être
   aux pixels. Une cible de 44 px qui porte un libellé de 21 caractères annule sa
   propre raison d'être.
2. **Le cadran ne l'interdit pas.** `secteurs.html:456-460` liste six choses qui
   n'entrent pas dans le cadran ; les libellés lisibles n'en font pas partie.
3. **`2D` doit disparaître quand même.** C'est le point sur lequel je ne
   recommande aucune souplesse : `00-vocabulaire.md:95` le bannit avec une raison
   qui vaut à la scène comme à l'atelier — « une plate 360 est aussi une image à
   deux dimensions » — et sur un instrument qui joue *à la fois* de la plate et de
   la sphère, l'ambiguïté est exactement là où elle coûte. « Plate » fait cinq
   lettres : la contrainte de longueur ne le justifie pas.
4. **`360` seul est acceptable là où `équirectangulaire` ne l'est pas**, parce que
   le domaine est ce qui distingue les deux formats que ce produit manipule
   réellement, et que `00-vocabulaire.md:35-37` dit précisément que nommer le
   domaine est ce qui lève l'ambiguïté.

**Si la réponse est non** — une seule colonne de libellés pour les deux salles —
alors il faut le dire, et le produit écrira « Équirectangulaire 360 » dans un
sélecteur qui devra changer de forme. C'est faisable ; ce n'est pas gratuit, et
ce sera la même dépense pour chaque produit de scène.

---

## SCRATCHVJ-04 — L'échelle typographique de la scène

**Portée : toute la ligne scène.** C'est la contradiction la plus structurelle du
relevé : deux lignes de `secteurs.html` ne peuvent pas être vraies ensemble.

**Ce que dit la source — deux fois, et pas pareil.**

- `docs/secteurs.html:369`, dans **l'invariant de maison** (§ 5.1, « ne diverge
  jamais ») : « **L'échelle typographique.** Un jeu de tailles fermé, pas un
  continuum. » `design/tokens.json:17` donne le jeu : `[11, 12, 13, 16, 28]`.
  `design/DIRECTION-ARTISTIQUE.md:71` : « 11 / 12 / 13 / 16 / 28. **Rien
  d'autre, nulle part.** »
- `docs/secteurs.html:456-458`, juste après le tableau du cadran : « Ce qui
  n'entre pas dans le cadran, et qu'il faut refuser : la famille de fontes,
  **l'échelle typographique**, le rayon des angles… »
- **Et** `docs/secteurs.html:396`, variable 1 du cadran, colonne scène :
  « 1 m dans le noir, **corps 15 px**, cible 44 px ».

15 n'est pas dans `[11, 12, 13, 16, 28]`. La variable 1 impose donc une taille
que l'invariant interdit, et le § 5.2 la range dans le cadran alors que le
paragraphe suivant l'en exclut nommément.

**Ce que fait le code.** Le produit charge trois faces : la fonte de texte à
15 px, la même à 11,5 px pour les sur-titres, la chasse fixe à 15 px. Le
commentaire du code donne la provenance et la mesure : « Sizes read off the
mockup at its native 1440 width: body 15, labels 11.5, numbers 15. »

Deux des trois tailles sont hors du jeu fermé. La feuille documentaire du produit
en emploie **dix-sept** distinctes, de 7,2 pt à 30 pt.

**La question.** La ligne scène a-t-elle sa propre échelle fermée, et laquelle ?

**Ma recommandation : oui, et elle doit être écrite dans `tokens.json` sous
`lines.scene.type.scale`, comme un second jeu fermé — pas comme une exception.**

Le raisonnement en trois temps.

1. **La contradiction se résout dans un seul sens.** Ce qui est invariant, c'est
   la **fermeture** de l'échelle — « un jeu de tailles fermé, pas un continuum »,
   `:369` — pas la liste elle-même. C'est la lecture qui rend les deux lignes
   compatibles : le cadran ne peut pas changer *le principe*, il peut changer *le
   jeu*. La ligne `:457` doit alors se lire « l'échelle typographique [comme
   principe] », et le dire, parce qu'aujourd'hui elle se lit comme interdisant
   `:396`.
2. **La valeur candidate est mesurable, et elle est déjà mesurée.** Le rapport
   entre les deux échelles n'est pas arbitraire : à 1 m au lieu de 50 cm, l'angle
   sous-tendu double. `[11, 12, 13, 16, 28]` × 1,15 environ donne
   `[13, 14, 15, 18, 32]`. Le produit emploie déjà 15 (corps) et n'a pas besoin
   de plus. **Ma proposition concrète : `[13, 15, 18, 24, 40]`** — cinq crans
   comme à l'atelier, le corps à 15 qui est déjà en place et déjà justifié, un
   cran bas à 13 qui remplace le 11,5 actuel (hors barème), et un plafond à 40
   pour un chiffre lu de loin, qu'un instrument de scène finira par vouloir et
   qu'aucune ligne n'autorise aujourd'hui.
3. **11,5 doit disparaître dans tous les cas.** Quelle que soit la réponse, une
   taille à virgule est un continuum de un, et c'est exactement ce que `:369`
   interdit. Ce point-là ne demande pas de décision : il demande seulement de
   savoir vers quel cran arrondir, et c'est la question ci-dessus.

**Ce que ça bloque.** La phase 5 sort les jetons du code dans un fichier unique.
Écrire ce fichier avec 15 et 11,5 dedans, c'est graver une taille qu'aucune
source n'autorise — et `MAISON.md:84-86` demande précisément qu'un jeton que le
toolkit ne sait pas produire se note au lieu de se remplacer en silence. Ici ce
n'est pas le toolkit qui ne sait pas : c'est la source qui ne dit pas.

---

## SCRATCHVJ-05 — La densité en trois étages : la variable 7 n'a aucune valeur

**Portée : toute la ligne scène.**

**Ce que dit la source.** `docs/secteurs.html:443-448`, variable 7 du cadran :

| Atelier | Scène | La raison |
|---|---|---|
| « maximale et uniforme, type Nuke » | « **hiérarchisée en trois niveaux, dont un lisible à un mètre** » | « un instrument de scène qui affiche tout à densité égale est illisible dans l'urgence. Ce n'est pas *moins* dense : c'est dense en trois étages, avec un étage qui se lit de loin » |

C'est **la seule des sept variables dont la colonne scène ne porte aucun
nombre**. La colonne atelier, elle, renvoie à des valeurs mesurables :
`design/tokens.json:20-24` — unité 4, gouttière 16, écart de groupe 20, ligne de
paramètre 28, barre haute 32, cible minimale 20.

**Ce que fait le code, faute de valeurs.** Il a inventé, et il a inventé
raisonnablement — mais rien ne le valide :

- **trois étages typographiques** : 15 px pour les mots, 11,5 px pour les
  sur-titres, 15 px en chasse fixe pour les valeurs. Le bon nombre d'étages, sans
  qu'aucune source ne dise lequel des trois « se lit de loin » ;
- **deux étages de contrôle** : 44 px (transport, pads) et 28 px (tout le reste,
  soit la valeur *atelier*) ;
- **cinq paires d'espacement** écrites une fois dans la fonction de style :
  `(8,4)`, `(10,6)`, `(6,4)`, `(18,16)`, `(8,4)`. Sept des dix nombres sont des
  multiples de l'unité de 4 ; `10`, `6` et `18` ne le sont pas.

**La question.** Que faut-il écrire sous `lines.scene.rhythm` ? Et surtout :
qu'est-ce qu'« un étage » — une taille de texte, une densité d'espacement, une
hauteur de contrôle, ou les trois à la fois ?

**Ma recommandation : définir l'étage comme un triplet, et en écrire trois.** La
raison de `:446-448` porte sur ce qu'on doit pouvoir lire *dans l'urgence*, et
lire dans l'urgence dépend autant de l'air autour du mot que de sa taille.

| Étage | Ce qu'il porte | Corps | Hauteur de ligne | Cible |
|---|---|---|---|---|
| **loin** — lisible à 1 m | ce qui joue : position, état de deck, BPM, armement | 18–24 | 44 | 44 |
| **proche** — lisible à 50 cm | ce qui se règle pendant le set | 15 | 28 | 28 |
| **fin** — lu quand on s'approche | ce qui se configure : mapping, chemins, diagnostics | 13 | 22 | 20 |

Deux arguments pour cette forme plutôt qu'une simple échelle de tailles.

1. **Elle rend la variable vérifiable.** Une liste de contrôle peut demander
   « chaque panneau déclare son étage, et n'en mélange pas deux » ; elle ne peut
   rien demander à « hiérarchisée en trois niveaux ».
2. **Elle est déjà à moitié vraie dans ce produit**, ce qui est le meilleur test
   possible : les deux hauteurs de contrôle existent, les trois tailles existent,
   et la seule chose qui manque est de dire lequel est lequel. Une variable de
   cadran qui décrit ce qu'un produit fait déjà par nécessité est une variable
   solide ; une qui le contredit demanderait à être défendue.

**Une réserve honnête.** Les trois lignes du tableau ci-dessus sont une
proposition à partir d'un seul produit. `secteurs.html:497-499` avertit que le
test des jetons « devient plus dur à chaque produit » : ces valeurs demandent à
être vues sur le second produit de scène avant d'être figées dans `tokens.json`.
Une réponse *reportée*, avec « ce qui manque : un second produit », serait
recevable — à condition que l'étage soit défini comme un triplet dès maintenant,
faute de quoi le second produit inventera son propre découpage.

---

## SCRATCHVJ-06 — La luminance commune des deux châssis, et la mono unique

**Portée : toute la ligne scène.** Ces deux valeurs sont les seules que
`secteurs.html` qualifie lui-même d'accidents, et elles bloquent la même phase.

**Ce que disent les sources — et elles ne disent pas la même chose.**

`docs/secteurs.html:410-416`, variable 3 : le châssis d'atelier est
« noir verdi froid `#0A0C0B` », celui de scène « noir chaud `#141412` », avec la
contrainte « **même luminance, teinte seule différente** — deux finitions d'un
même matériau, pas deux marques ».

`docs/secteurs.html:469-476`, § 5.3 « les accidents » :

- la mono : « **Accident.** Deux monos, c'est deux maisons. Une seule, arbitrée
  sur la disponibilité, pas sur le goût : Fragment Mono n'est présente nulle part
  sur le disque ; DM Mono est présente *et* passe le moteur d'impression de
  Chrome — qu'Archivo, elle, ne passe pas. **Trancher sur ce test.** »
- la luminance : « **Mi-accident** : la teinte est du cadran (variable 3), l'écart
  de *valeur* n'en est pas. Aligner la valeur, garder la teinte. »

`docs/secteurs.html:663` : « les deux accidents (la mono, la luminance du
châssis) **se corrigent aujourd'hui, en un fichier chacun** ».

**Mais `MAISON.md:98-99`** range les deux en **couche 3** — « explicitement
ouverte, ne se touche pas » — et ordonne : « **Ne rien changer** : c'est de la
couche 3. Préparer le changement en un endroit. »
`remontees/REGISTRE.md:62` confirme du côté maison : la famille de chasse fixe
est ouverte, « la maison, arbitrée sur la disponibilité et l'impression, pas sur
le goût ».

**J'ai suivi `MAISON.md` et je n'ai rien changé.** Mais la contradiction entre
deux sources à lire est elle-même une remontée : une session suivante qui lirait
`secteurs.html:663` en premier corrigerait, et aurait une ligne pour se
justifier.

**Ce que fait le code.** Le fond de fenêtre vaut `#141412` — la valeur scène
exacte, écrite deux fois, dans deux fichiers et deux notations. Les six autres
couleurs de châssis sont toutes des versions chaudes des jetons d'atelier ; la
teinte est cohérente d'un bout à l'autre. La chasse fixe est DM Mono, chargée
depuis le dépôt ; DM Mono est aussi la seule fonte que la feuille documentaire
peut employer, pour une raison écrite dans le code de cette feuille : le moteur
d'impression de Chrome refuse le fichier variable d'Archivo.

**Les mesures que ce relevé apporte, et qui manquaient au test de `:415-416`.**

| Valeur | Luminance relative (WCAG) | Écart |
|---|---|---|
| atelier `#0A0C0B` | 0,0035 | — |
| scène `#141412` | 0,0069 | **× 2,0** |

La contrainte « même luminance, teinte seule différente » **n'est tenue par
aucune des deux valeurs actuelles** : le fond de scène est deux fois plus clair
que celui d'atelier. Ce n'est pas une nuance de finition, c'est un autre noir.

**Les questions, et mes recommandations.**

**Q1 — quelle est la luminance commune ?** Recommandation : **prendre celle de
l'atelier et refroidir la scène, pas l'inverse** — `#0D0C0A` ou voisin, qui
garde la dérive chaude R > G > B de `#141412` à la luminance de `#0A0C0B`. La
raison n'est pas esthétique : `DIRECTION-ARTISTIQUE.md:54` fait de `--void` le
fond « derrière tout rendu 3D ou vidéo », et un fond plus clair réduit le
contraste avec l'image projetée — ce qui est précisément ce qu'on ne veut pas
dans une salle noire. Le produit y perd un peu de séparation visuelle entre son
châssis et son puits d'image ; c'est le bon côté du compromis.

**Q2 — DM Mono ou Fragment Mono ?** Recommandation : **DM Mono**, et le test est
déjà passé, ici, avec sa trace. Fragment Mono est absente du disque
(`REGISTRE.md:62`). DM Mono est présente, embarquée, et c'est la seule des deux
qui survit au moteur d'impression de Chrome — ce qui n'est pas un détail : c'est
la chaîne qui fabrique les PDF commerciaux, la première surface que l'acheteur
voit (`DIRECTION-ARTISTIQUE.md:146`). Le critère écrit en `secteurs.html:470-473`
est « la disponibilité, pas le goût » ; sur ce critère, un seul candidat reste
debout.

**Ce que ça bloque.** La phase 5 écrit le fichier unique de jetons. Y graver
`#141412` et « DM Mono » sans réponse, c'est faire de deux accidents une
convention — et `secteurs.html:355-357` dit exactement ce que ça coûte : « un
accident qu'on ne nomme pas devient une convention en six mois, et une convention
se défend ».

---

## SCRATCHVJ-07 — `lines.scene` n'existe pas : ni accent, ni paire A/B, ni classe documentaire

**Portée : toute la ligne scène.** C'est le trou le plus mécanique des vingt, et
le plus facile à combler.

**Ce que dit la source.** `docs/secteurs.html:492-493` : « Et `tokens.json` v3,
qui gagne un niveau au-dessus de `chassis` : `lines: { atelier, scene }`. »
`:418-425`, variable 4 : la scène porte « une teinte produit **plus une paire
opératoire A/B** (ambre / ardoise), constante sur toute la ligne », et la paire
« doit être **identique dans tous les produits de scène**, pour qu'un réflexe
acquis sur l'un serve sur l'autre ».

**Ce qui existe réellement.** `design/tokens.json` est en `"version": 2`,
`"revised": "2026-09-08"`. Il n'y a pas de niveau `lines`. Ses huit accents sont
ceux des huit produits d'atelier. `docs/suite.css:53-61` porte huit classes
produit — `.p-georama` à `.p-mutoscope` — et aucune pour un produit de scène.

**Ce que fait le code.** Le produit emploie trois valeurs qui n'existent nulle
part dans la maison :

| Rôle | Valeur | Ce qu'en dit `MAISON.md` |
|---|---|---|
| accent produit | `#C9762F` | `:18` — « **candidat**, absent de `tokens.json` » |
| deck A, ambre | `#C99A2F` | `:18` — « qui doit devenir une constante de toute la ligne scène » |
| deck B, ardoise | `#6E8696` | idem |

Et un quatrième, dans la feuille documentaire du produit : un accent
`#b45f1c` — **un second orange**, distinct du premier, alors que
`DIRECTION-ARTISTIQUE.md:164-166` écrit qu'« un produit porte une teinte, une
seule » et que « un document qui ajoute un second accent […] invente une identité
de plus ». C'est le même défaut que le second accent doré trouvé chez Diorama
(`DIVERGENCES.md:159-163`), sous une autre forme.

**Les questions.**

**Q1 — la maison entérine-t-elle les trois valeurs ?** Recommandation :
**entériner l'ambre et l'ardoise, reporter l'accent produit.** Elles ne sont pas
du même ordre.

- La **paire A/B** est de l'état, pas de l'identité — `secteurs.html:422-425` le
  dit — et elle doit être constante sur la ligne. Elle ne dépend donc d'aucune
  décision de nom ni de palette : elle dépend seulement d'être écrite une fois.
  Elle n'est pas de la couche 3 pour la même raison que `DIORAMA-04` n'en était
  pas : on choisit lequel des rôles porte une valeur, on ne juge pas une teinte
  sur un écran non calibré.
- L'**accent produit** `#C9762F`, lui, est bien de la couche 3 : c'est une teinte
  de plus dans une palette que personne n'a vue ensemble sur un écran calibré
  (`REGISTRE.md:59`). Et il est lié à un nom de produit qui n'existe pas encore
  (SCRATCHVJ-19). Le figer maintenant, c'est le figer deux fois.

**Q2 — `suite.css` reçoit-elle une classe pour ce produit ?** Recommandation :
**oui, mais après Q1**, et sous le nom définitif du produit. Une classe
`.p-scratchvj` gravée aujourd'hui serait à renommer au premier nom trouvé, dans
une feuille commune que huit autres produits partagent.

**Q3 — le second orange documentaire ?** Ce n'est pas une question : c'est du
travail de produit, il est noté dans le relevé comme tel, et il disparaît en
phase 7 quand la feuille commune prend la place de la feuille locale.

---

## SCRATCHVJ-08 — L'ambre porte deux rôles

**Portée : toute la ligne scène.** La paire A/B étant constante sur la ligne, la
collision le sera aussi.

**Ce que disent les sources.** `design/tokens.json:32` : `signal.running` vaut
`#C9A227`, rôle « travail en cours ». `docs/secteurs.html:377-379`, dans
**l'invariant** : « Les quatre couleurs de signalisation et leur sens. […] un
`fail` rouge veut dire la même chose sur les deux lignes ». `:420` fait de
l'ambre la moitié de la paire opératoire de scène.

**Ce que fait le code.** Une seule constante ambre, `#C99A2F`, employée pour les
deux :

- **le deck A** — la moitié gauche de la paire A/B, posée sur tout ce qui
  appartient au deck A ;
- **le travail en cours** — « analyse en cours », « analyse 47 % », un lien de
  platine faible, un contrôle non vérifié, un retour vidéo détecté.

`#C99A2F` et `#C9A227` diffèrent de trois unités sur le vert et de huit sur le
bleu : à un mètre, dans le noir, c'est la même couleur.

**La question.** Un utilisateur voit un point ambre. Cela veut-il dire « ceci
appartient au deck A » ou « quelque chose est en cours » ?

**Ma recommandation : garder l'ambre pour la paire A/B, et déplacer `running`.**
Trois raisons.

1. **La paire ne peut pas bouger.** `secteurs.html:422-425` la déclare constante
   sur toute la ligne et acquise comme réflexe. Une teinte de deck qui change
   d'un produit de scène à l'autre annule sa raison d'être.
2. **`running` a une porte de sortie que la paire n'a pas.** L'invariant impose
   que les quatre signaux « veulent dire la même chose partout » — pas qu'ils
   soient portés par une pastille de couleur. `ERGONOMIE.md:323-326` exige déjà
   qu'une progression affiche « la fraction faite, l'étape courante **nommée**,
   et le temps restant » : sur la ligne scène, `running` peut être **le fait
   qu'une progression soit affichée**, sans point de couleur du tout. Et cela
   rejoint la conséquence déjà tenue par la maison — un point d'état n'est jamais
   le seul porteur d'une information (`REGISTRE.md:61`).
3. **Il n'y a presque aucun travail en cours à la scène.** `secteurs.html:191-193`
   : le travail y est « immédiat, irréversible, et ne produit aucun fichier ». Ce
   produit n'a qu'un seul travail long — l'analyse d'un clip — et il se fait
   **avant** le set, pas pendant. Charger la teinte la plus employée de la ligne
   pour un état qui n'existe que dans l'écran de préparation est un mauvais
   échange.

**La solution de repli**, si la maison préfère garder un point `running` coloré :
écarter les deux valeurs de façon mesurable — l'ambre de la paire vers le jaune,
`running` vers l'orange — et l'écrire dans `tokens.json` avec le delta minimal
exigé. Je la recommande moins : elle demande de mesurer une différence perceptive
sur un écran calibré, ce qui est justement ce qui reste ouvert
(`REGISTRE.md:59`).

---

## SCRATCHVJ-09 — Les quatre signaux sur un châssis chaud, et le `warn` manquant

**Portée : toute la ligne scène.**

**Ce que dit la source.** `docs/secteurs.html:377-379` range dans **l'invariant
de maison** — « ne diverge jamais » — « les quatre couleurs de signalisation et
leur sens ». `:458` le confirme en creux : « ce qui n'entre pas dans le cadran
[…] les couleurs de signalisation ». `design/tokens.json:30-36` donne les quatre
valeurs :

| Jeton | Valeur | Sens |
|---|---|---|
| `running` | `#C9A227` | travail en cours |
| `done` | `#7FB069` | terminé sans réserve |
| `warn` | `#C48A4B` | terminé avec réserve, ou dérive détectée |
| `fail` | `#D9584B` | échec — la seule couleur autorisée à porter du texte |

**Ce que fait le code.** Trois des quatre existent, toutes retouchées vers le
chaud ; la quatrième n'existe pas.

| Jeton | Maison | Produit | Écart |
|---|---|---|---|
| `running` | `#C9A227` | `#C99A2F` (confondu avec le deck A — voir SCRATCHVJ-08) | faible |
| `done` | `#7FB069` | `#7E946B` — vert désaturé et assombri | net |
| `warn` | `#C48A4B` | **absent** | — |
| `fail` | `#D9584B` | `#B54B3A` — rouge assombri | net |

Le produit n'a donc que trois états à montrer : lié/terminé, en cours/faible,
perdu/échoué. Un lien de platine qui existe mais faiblit est peint en ambre,
c'est-à-dire avec la couleur de « travail en cours » — alors que c'est
exactement le sens de `warn` : « dérive détectée ».

**Les questions.**

**Q1 — un châssis chaud a-t-il droit à ses propres valeurs de signalisation ?**
Recommandation : **non pour les teintes, oui pour un ajustement de luminance
écrit.** L'invariant porte sur *quatre teintes et leur sens*, et une pastille
verte doit être la même verte dans les deux salles — c'est tout l'intérêt d'un
invariant. Mais `docs/suite.css:38-40` a déjà créé un précédent que la maison a
accepté : les quatre signaux y sont **assombris** pour rester lisibles sur fond
clair, avec la raison écrite dans la feuille. La même logique appliquée à la
scène donne : mêmes teintes, un jeu de valeurs par support, et la règle écrite
une fois — pas quatre valeurs libres par produit.

**Q2 — que fait un produit de scène de `warn` ?** Recommandation : **le lui
donner, et lui donner son emploi.** C'est le signal dont un instrument joué
devant un public a le plus besoin, et c'est celui qui manque : « ça marche
encore, mais ça dérive » est précisément l'information qu'on veut voir avant que
le lien de platine lâche. Aujourd'hui le produit ne peut pas la dire, parce qu'il
n'a que trois couleurs et que la troisième veut déjà dire autre chose.

**Q3 — la conséquence d'accessibilité.** `REGISTRE.md:61` la tient déjà : « un
point d'état n'est jamais le seul porteur d'une information ». Ce produit la
respecte : chaque voyant est suivi d'un mot. Aucune décision demandée ; noté pour
que la ligne scène hérite de la règle et non de sa seule conclusion.

---

## SCRATCHVJ-10 — La couleur qui porte du texte

**Portée : toute la ligne scène.** La paire A/B est en cause, et elle est
constante sur la ligne.

**Ce que dit la source.** `docs/secteurs.html:377-379`, **invariant** : « Un
`fail` rouge veut dire la même chose sur les deux lignes, et **reste la seule
couleur autorisée à porter du texte**. »
`design/DIRECTION-ARTISTIQUE.md:98` écrit la même règle.

**Ce que fait le code.** Le produit écrit du texte coloré à **24 endroits** :
17 en ambre, 3 en ardoise, 3 en vert, 2 en rouge, 1 en orange d'accent (le nom du
produit dans la barre haute). Deux seulement sont conformes : les deux rouges,
qui sont les bandes d'erreur.

Parmi les non-conformes, quatre sont d'une nature différente des autres : ce sont
les lettres **« A »** et **« B »**, écrites en ambre et en ardoise, qui
identifient les deux decks au-dessus des faders et sur la bande programme.
C'est la variable 4 du cadran, appliquée de la manière la plus économique
possible — une lettre, une couleur.

**La question.** La paire opératoire A/B a-t-elle le droit de porter du texte,
alors que l'invariant réserve cela à `fail` ?

Le cadran ne le dit pas. `secteurs.html:418-425` crée la paire, écrit sa raison
et impose sa constance sur la ligne — sans dire quelle **surface** elle a le droit
d'occuper. Le silence n'est pas neutre : la variable 2 prend la peine d'autoriser
un aplat, ce qui montre que la question des surfaces est traitée quand elle se
pose.

**Ma recommandation : oui pour la paire, non pour le reste, et l'écrire dans le
cadran.**

Pour la paire, trois raisons.

1. **Une lettre colorée est la plus petite surface possible.** La règle de
   couleur d'atelier autorise déjà « des surfaces de 2 px maximum : filet de rail,
   point d'état, capuchon » (`DIRECTION-ARTISTIQUE.md:76-78`). Un « A » ambre à
   15 px n'occupe pas plus de matière colorée qu'un capuchon de 2 px sur 32.
2. **L'alternative est pire.** Sans le texte coloré, dire « ceci est le deck A »
   demande une pastille **plus** un « A » en craie — soit deux éléments, plus de
   surface colorée, et un mot de plus à lire à un mètre dans le noir. La règle
   produirait le contraire de ce qu'elle protège.
3. **La raison de l'invariant est préservée.** Si `fail` est seul à porter du
   texte, c'est pour qu'un texte coloré soit toujours lu comme une alarme. Un
   « A » et un « B » ne sont pas des phrases : ce sont des étiquettes, et
   personne ne lit une étiquette d'une lettre comme un message d'erreur. La
   clause à écrire porte donc sur ce critère-là, pas sur la couleur : *une
   étiquette d'un ou deux caractères qui nomme une source n'est pas du texte au
   sens de cette règle.*

Pour les dix-huit autres, non, et sans regret : « analyse en cours », « plateau
réel : aucune entrée audio », « pas encore à l'image » sont des phrases, et une
phrase ambre à côté d'une phrase rouge est exactement ce que l'invariant
empêche. C'est du travail de produit, pas une décision de maison.

Reste **le nom du produit en orange dans la barre haute**. `ERGONOMIE.md:104-106`
décrit à cet endroit « le capuchon d'accent du produit — un filet plein de 2 px
[…] puis le nom du produit » en craie. Le produit a fusionné les deux : pas de
filet, le nom lui-même en accent. **Question secondaire : la barre haute de la
scène garde-t-elle le capuchon de 2 px ?** Recommandation : oui. C'est le seul
emploi de l'accent que `DIRECTION-ARTISTIQUE.md:100-106` juge « vraiment utile »,
et rien dans le cadran ne le retire à la scène — la variable 4 ajoute la paire,
elle ne remplace pas le capuchon.

---

## SCRATCHVJ-11 — L'aplat : deux règles que `MAISON.md` croit identiques

**Portée : toute la ligne scène.** C'est la variable la plus visible du cadran, et
sa formulation est ambiguë.

**Ce que disent les sources.** Elles disent deux choses, et `MAISON.md` affirme
qu'elles n'en disent qu'une.

- `docs/secteurs.html:403-404`, variable 2, colonne scène : « **un aplat
  autorisé** — un seul par panneau, **toujours porté par un état**, jamais par
  l'identité ». La démonstration de `:451-454` le montre : trois segments,
  « Boucle · **Armé** · Slip », l'aplat sur *Armé* — un état.
- `design/README.md:33-34` du produit : « Tout ce qui se clique a un fond et un
  bord ; **l'orange plein est réservé à une seule action par panneau**. »
- `MAISON.md:90` : « L'aplat orange n'est pas une infraction. C'est la variable 2
  du cadran : *un aplat autorisé, un seul par panneau, toujours porté par un
  état, jamais par l'identité* — **et c'est déjà, mot pour mot, la règle de
  `design/README.md`.** »

Ce n'est pas mot pour mot, et ce n'est pas la même règle. *Un état* et *une
action* sont deux critères différents : l'un porte sur ce que le contrôle
**montre**, l'autre sur ce qu'il **fait**.

**Ce que fait le code.** Il applique les deux à la fois, ce qui est la
conséquence exacte de l'ambiguïté :

- **portés par un état** — le bouton de lecture quand le deck joue, la sortie de
  boucle quand la boucle est active, un clip présent dans une caisse, un contrôle
  en écoute d'apprentissage MIDI, et les 17 sélecteurs segmentés, dont l'option
  choisie est remplie ;
- **portés par une action** — quatre boutons : « ■ relecture », « Suivant → »,
  « Créer », « + Liaison ». Le commentaire du code les assume : *« `primary`
  fills it with the accent: one per panel, the thing this panel is FOR. »*

**Et le compte n'est pas tenu.** Le panneau d'un deck fait 490 lignes de code et
porte **jusqu'à sept** surfaces remplies d'accent : cinq sélecteurs segmentés —
source d'horloge, mode de lecture, projection, boucle automatique, forme — plus
deux boutons d'état.

**Les questions.**

**Q1 — état ou action ?** Recommandation : **l'état, et corriger `MAISON.md` par
son générateur.** Deux raisons. (a) C'est ce que la source dit, et
`design/README.md` est un document de produit qui ne peut pas amender le cadran.
(b) La raison écrite en `:405-408` tranche à elle seule : « à la scène il faut
*voir avant de lire*. L'aplat est le seul signal préattentif dont on dispose. »
Un signal préattentif sert à répondre à « où en est-on ? », pas à « que puis-je
faire ? » — la seconde question, on se la pose en lisant. Un aplat sur un bouton
d'action au repos consomme le seul canal préattentif de l'écran pour une
information qui n'a pas changé depuis dix minutes.

**Q2 — « un seul par panneau » : qu'est-ce qu'un panneau ?** C'est la moitié qui
rend la règle inapplicable. Le panneau d'un deck contient cinq groupes de
contrôles ; est-ce un panneau, ou cinq ? Recommandation : **définir le panneau
comme la plus petite région délimitée par un filet ou un fond, et non par la
fonction.** Un panneau est ce que l'œil découpe, et l'œil découpe des bordures.
Sur ce critère, chacun des cinq groupes du deck est un panneau, chacun a un
sélecteur, et la règle redevient vraie sans rien changer au code.

**Q3 — un sélecteur segmenté à trois options compte-t-il pour un aplat ?**
Recommandation : **oui, un seul.** Un sélecteur exclusif ne peut afficher qu'un
seul segment rempli ; le compte porte sur les surfaces visibles simultanément,
pas sur les contrôles.

**Ce que ça bloque.** La phase 5 sort les jetons et fige l'emploi de l'accent.
Sans Q1 et Q2, le produit ne peut ni vérifier sa conformité ni la faire vérifier —
et c'est le seul point du cadran où `secteurs.html:655` prévient qu'une
correction faite à l'aveugle « détruirait ce qui rend l'instrument jouable dans
le noir ».

---

## SCRATCHVJ-12 — Le rayon des contrôles : 1 px, 3 px, ou « minimal » ?

**Portée : toute la maison.** La contradiction est entre deux sources d'atelier ;
ce produit ne fait que la révéler.

**Ce que disent les sources — trois fois, et pas pareil.**

- `design/tokens.json:22` : `"radius_control": 3, "radius_panel": 0`.
- `design/DIRECTION-ARTISTIQUE.md:246`, dans la liste des interdits : « Coins très
  arrondis (**le rayon est 3px sur un contrôle**, 0 sur un panneau). »
- `docs/secteurs.html:370-371`, dans **l'invariant de maison** : « Angle vif sur
  les panneaux, **rayon minimal sur les contrôles**. »

Les deux premières donnent un nombre ; la troisième donne un critère, et 1 px
satisfait mieux « minimal » que 3.

**Ce que fait le code.** Le rayon de cadre et le rayon de poignée valent `1.0f` ;
les rayons de fenêtre, d'enfant et d'ascenseur valent `0.0f`. Le bouton dessiné à
la main emploie le même 1 px. Il n'y a donc que deux valeurs dans tout le
produit, 0 et 1, et le panneau est bien à angle vif.

**La question.** Le nombre fait-il foi, ou le critère ?

**Ma recommandation : le nombre — 3 px — et corriger `secteurs.html:371` pour
qu'il le cite au lieu de le paraphraser.**

Trois raisons.

1. **`secteurs.html:457` range le rayon des angles parmi ce qui n'entre pas dans
   le cadran.** Ce n'est donc pas une variable de ligne : il ne peut y avoir
   qu'une valeur, et deux documents sur trois la donnent.
2. **Un critère qualitatif dans un invariant est un accident en attente.** Huit
   produits lisant « minimal » écriront huit valeurs différentes ;
   `DIVERGENCES.md:131` mesure déjà exactement ça sur les rayons — « 3 px contre
   6 px contre 6/8/10 px et une pilule de 12 px ».
3. **Le coût est nul dans ce produit** : deux lignes de la fonction de style, plus
   deux appels de dessin. C'est le point le moins cher des vingt.

**Une réserve, honnêtement.** À 1 m dans le noir, un rayon de 3 px sur une cible
de 44 px se voit un peu plus qu'à 50 cm sur une cible de 28. Si la maison estime
que le rayon devient une variable de cadran, alors il faut l'ajouter au tableau
du § 5.2 avec une raison métier écrite — et je ne trouve pas laquelle. C'est
précisément le test de `secteurs.html:386-389` : « si la raison ne peut pas
s'écrire, la variable retourne à l'invariant ». Je ne peux pas l'écrire, donc je
recommande l'invariant.

---

## SCRATCHVJ-13 — La capitale initiale sur un libellé de paramètre, à un mètre

**Portée : toute la ligne scène.**

**Ce que disent les sources — et elles se croisent.**

- `design/ERGONOMIE.md:382-383` : « **Capitale initiale seulement.** Pas de
  capitales à tous les mots, pas de point final. » La clause ajoutée par
  `DIORAMA-01` précise : « La règle porte sur les libellés que le produit écrit
  lui-même » — ce qui vise ce produit de plein fouet, puisqu'il n'a pas d'hôte.
- `design/DIRECTION-ARTISTIQUE.md:200-201`, en revanche, prescrit les capitales
  pour une catégorie précise : « **titre de panneau à 125 condensé en
  capitales**, chiffre héroïque à 62 étendu ». `docs/suite.css:82-83` fait de même
  pour les titres de section d'un document.

Les deux règles ne se contredisent pas : l'une porte sur les **titres de
panneau**, l'autre sur les **libellés**. Encore faut-il savoir où passe la
frontière.

**Ce que fait le code.** Le produit emploie deux fonctions de libellé, et la
frontière passe entre elles :

| Genre | Sites | Casse | Exemples |
|---|---|---|---|
| sur-titre de panneau | 39 | capitales | `RACK`, `PRISES`, `BANQUES DE PADS`, `SOURCE ÉQUIRECTANGULAIRE — cadre de visée` |
| onglet d'écran | 6 | capitales | `JOUER`, `BIBLIOTHÈQUE`, `EFFETS`, `TABLE`, `SORTIE`, `RÉGLAGES` |
| **libellé de paramètre** | 24, dont **20 en capitales** | capitales | `BOUCLE`, `VITESSE`, `DESTINATION`, `ZONE MORTE`, `VUE 360` |
| libellé de curseur | 5 | **minuscules** | `lacet`, `tangage`, `roulis`, `champ`, `zoom` |
| bouton | 40 | capitale initiale | `Importer…`, `Créer`, `Rescanner`, `Effacer` |

Les 39 premiers et les 6 onglets sont couverts par
`DIRECTION-ARTISTIQUE.md:200-201`. Les 45 boutons sont conformes. **Les 25 du
milieu ne le sont ni dans un sens ni dans l'autre** : vingt en capitales, cinq
tout en minuscules — le produit se contredit lui-même à l'intérieur du même
écran, un `TANGAGE` en capitales dans le résumé et un `tangage` en minuscules
dans le popup qui le règle.

**La question.** Un libellé de paramètre, à un mètre dans le noir, garde-t-il la
capitale initiale ?

**Ma recommandation : oui, capitale initiale, et corriger les 25.** Ce n'est pas
la réponse confortable, et voici pourquoi je la donne quand même.

1. **La capitale intégrale ne se lit pas mieux de loin, elle se lit moins bien.**
   Un mot en capitales perd sa silhouette — les hampes et les jambages — qui est
   précisément ce que l'œil reconnaît à distance sans lire lettre à lettre. C'est
   l'argument qui rendrait la variable de cadran défendable ; il joue à
   l'envers.
2. **La raison métier ne s'écrit pas.** `secteurs.html:386-389` : « si la raison
   ne peut pas s'écrire, la variable retourne à l'invariant ». Le seul argument
   pour les capitales à la scène est « ça fait plus VJ », et `:459` nomme
   exactement celui-là comme la demande à refuser.
3. **La règle qui manque n'est pas une exception, c'est une frontière.** Ce
   produit s'est trompé de fonction, pas de règle : il a employé le sur-titre
   comme libellé de ligne. Ce que la maison doit écrire n'est donc pas « la scène
   a droit aux capitales », mais **où passe la ligne entre un titre de panneau et
   un libellé de paramètre** — et la formulation existe déjà à moitié dans
   `ERGONOMIE.md:135-139` : *le libellé est ce qui a une valeur à sa droite.*
   Écrite ainsi, la règle se vérifie mécaniquement, dans les huit produits comme
   ici.

**Ce que ça coûte.** Vingt-cinq chaînes dans un fichier, en phase 2. Aucun
fichier écrit sur disque n'est touché.

---

## SCRATCHVJ-14 — Le format d'un temps et d'un angle sur un instrument scratché

**Portée : toute la ligne scène.**

**Ce que dit la source.** `design/ERGONOMIE.md:157-167`, « précision d'affichage,
**invariable** » :

| Grandeur | Format | Exemple |
|---|---|---|
| angle | une décimale, signe explicite pour le lacet et le tangage | `−34.2°` |
| timecode | `hh:mm:ss:ff` | `00:04:12:18` |
| durée d'un travail | `h min s`, jamais de décimales | `1 h 04 min` |

`MAISON.md:36` range « la précision d'affichage » dans ce qui « s'applique en
entier » à la scène, sans réserve.

**Ce que fait le code.** Deux formats de temps et un format d'angle :

- la position dans un clip : `mm:ss.d` — par exemple `02:14.3` ;
- une durée courte : `mm:ss` ;
- les angles : zéro décimale, signe explicite — `lacet +12° · tangage −8°`.

**Le désaccord n'est pas une négligence, et c'est ce qui en fait une question.**

Sur le **timecode**. `hh:mm:ss:ff` suppose deux choses qu'un instrument de
scratch n'a pas. (a) Des **heures** : un clip de VJ dure trente secondes à
quelques minutes ; quatre caractères sur onze affichent toujours `00:`. (b) Une
**cadence d'images stable** : le champ `ff` compte des images par seconde, or ce
logiciel est construit sur le principe inverse — son manifeste, dans son propre
README, dit que « tout ce qui se scratche est une *fonction de la position*,
jamais un intégrateur ». À l'arrêt sur un scratch, la position est continue et
l'image est un échantillon ; `ff` y afficherait un nombre qui saute.

Sur l'**angle**. Ici je ne défends pas le code : une décimale coûte deux pixels et
la source est explicite.

**Les questions et mes recommandations.**

**Q1 — le format de temps de la ligne scène.** Recommandation : **`mm:ss.d`, et
l'écrire dans la table comme une seconde ligne, pas comme une exception.**

| Grandeur | Format atelier | Format scène |
|---|---|---|
| position dans un média | `hh:mm:ss:ff` | **`mm:ss.d`**, `hh:` ajouté seulement au-delà d'une heure |

Trois raisons. (a) La raison métier s'écrit, et c'est le test de
`secteurs.html:386-389` : *à la scène, la position est une fonction continue du
geste, pas un numéro d'image ; un compteur d'images y afficherait une valeur qui
saute pendant un scratch.* (b) Le dixième de seconde est ce qu'on peut lire à un
mètre dans le noir ; l'image ne l'est pas. (c) Le format garde la propriété qui
compte — largeur fixe, chiffres tabulaires, comparaison à l'œil entre les deux
decks.

**Q2 — l'angle.** Recommandation : **aucune exception**, une décimale partout.
C'est du travail de produit ; je le note ici seulement pour que la réponse à Q1 ne
soit pas lue comme ouvrant la précision d'affichage en général. Elle ouvre une
ligne, et une seule.

**Un lien avec la phase 3.** Le repère temporel de la ligne scène —
*qui fait autorité : le tempo, l'horloge audio, ou la position du plateau ?* —
est déjà inscrit comme ouvert (`REGISTRE.md:68`) et attendu de ce produit. Le
format d'affichage en dépend : si le tempo fait autorité, l'unité naturelle est
le temps musical et non la seconde, et la ligne « scène » du tableau ci-dessus
devra porter les deux. Je recommande donc de **trancher Q1 après la phase 3**, ou
de la trancher avec la réserve écrite.

---

## SCRATCHVJ-15 — Le clavier de la ligne scène, et `Échap` qui ferme la fenêtre

**Portée : toute la ligne scène.** Plus une correction factuelle à deux sources.

### Le point le plus urgent, et il n'est pas une question

`design/ERGONOMIE.md:284-285` : « **`Échap` ne ferme jamais la fenêtre**, et ne
quitte jamais le plein écran seul. » `MAISON.md:180` le reprend dans la
définition de fini.

Le code fait exactement l'inverse, en trois rangs : le premier `Échap` quitte
« image seule », le deuxième ferme la fenêtre de sortie, **le troisième quitte
l'application**. Le code écrit sa raison : *« One escape gets the picture off the
projector; a second ends the set. Quitting straight to a desktop in front of a
room is the thing this ordering exists to prevent. »* L'argument est bon pour les
deux premiers rangs. Au troisième, on est précisément sur un bureau devant une
salle.

Second défaut, indépendant : la garde ne teste que la saisie de texte. Un popup
ouvert sans champ actif laisse passer **les deux** traitements — la bibliothèque
d'interface ferme le popup, et le gestionnaire d'événements avance d'un rang dans
sa propre pile. La pile de priorité en six rangs de `ERGONOMIE.md:275-285` n'est
pas implémentée : ses cinq premiers rangs n'existent pas, et le sixième (« ne
rien faire ») est remplacé par « quitter ».

**C'est du travail de produit, en phase 4, et il se fait sans réponse.** Il est
écrit ici parce que c'est le seul manquement franc du relevé, et parce que le
premier produit de la scène ne doit pas laisser croire que la pile d'`Échap` est
négociable dans une salle.

### La question qui, elle, se remonte

**Ce que dit la source.** `design/ERGONOMIE.md:245-258` fixe le clavier réservé
de la suite. `MAISON.md:101` en extrait le **noyau** exigible ici : `Espace`,
`Échap` selon la pile, `Ctrl`+`Z`, `?` / `F1`, `Tab`.

**Ce que fait le code.** Quatre touches liées, et seulement quatre : `Espace`
(lecture/pause du deck sous la souris), `F` (image seule), `B` (rail de
bibliothèque), `Échap`. Trois du noyau manquent :

| Touche | Manque parce que |
|---|---|
| `Ctrl`+`Z` | **le produit n'a rien à annuler.** `secteurs.html:191-193` : à la scène « le travail est immédiat, irréversible, et ne produit aucun fichier ». Le test de `spec/04-frontieres.md:19-23` — *que reste-t-il quand on ferme la fenêtre ?* — répond : rien. Un produit sans état persistant n'a pas d'annulation à offrir |
| `Tab` | **son effet existe, sur `F`.** `MAISON.md:138` l'écrit noir sur blanc : « `Échap` […] ne quitte jamais “image seule” — `F` s'en charge ici, comme `Tab` à l'atelier ». La maison a donc déjà accordé la substitution |
| `?` / `F1` | pas de raison. C'est du travail de produit, en phase 4 |

**La question.** `Ctrl`+`Z` fait-il partie du noyau exigible d'un produit de
scène ?

**Ma recommandation : non, et écrire le critère plutôt que l'exception.** La
clause tranchée par `DIORAMA-06` (`ERGONOMIE.md:233-238`) dit déjà qu'« un
produit qui ne lie aucune touche n'en lie pas pour se conformer ». Elle ne couvre
pas ce cas : ce produit lie des touches, il n'a simplement rien à mettre derrière
celle-là. La clause à ajouter est du même genre, et elle s'écrit en une phrase :
*une touche réservée n'est exigible que d'un produit qui a la chose qu'elle
manipule.* Sans ça, la table du clavier redevient ce que `DIORAMA-06` lui
reprochait — « une demande de fonctionnalité déguisée », que `MAISON.md:148`
interdit précisément d'écrire.

**Et la ligne scène gagne-t-elle son propre clavier réservé ?**
`secteurs.html:483-486` le pose sans le trancher : « la ligne scène a droit à son
clavier réservé, les réflexes n'étant pas les mêmes — mais les touches communes
(espace, échap) ne doivent pas se contredire ».

Recommandation : **oui, et ce produit peut en fournir les trois premières
lignes** — `Espace` (lecture/pause, déjà unanime dans les deux salles), `F`
(image seule), `Échap` (la pile, sans le rang qui quitte). Mais **pas
maintenant** : trois touches ne font pas un clavier de ligne, et un second
produit de scène existe (`secteurs.html:667`). Écrire la table à partir d'un seul
produit, c'est refaire ce que `DIVERGENCES.md` a coûté à la suite. Recommandation
opératoire : **noter les trois comme candidates dans `secteurs.html` § 5, et
figer au tour 01 du second produit.**

### Deux corrections factuelles

`MAISON.md:101` et `docs/secteurs.html:483` décrivent tous deux « les pads sur
`1`–`5` » et un conflit potentiel avec `Ctrl`+chiffre. **Cette interface n'existe
plus.** La refonte de septembre 2026 a supprimé les raccourcis chiffrés : les huit
pads sont à l'écran, en cellules de 44 px, et sur la surface MIDI. Recherche
exhaustive des touches `0` à `9`, pavé numérique compris : **aucune occurrence**.
Le conflit annoncé n'existe pas, dans aucun des deux sens.

Conséquence pour la maison : `secteurs.html:486` cite ce conflit comme la preuve
que « ça arrive tout seul », par analogie avec Mutoscope. La preuve tombe ; la
conclusion reste vraie par Mutoscope seul (`REGISTRE.md:67`).

---

## SCRATCHVJ-16 — Les constantes `viewer` quand le regard est piloté au potard

**Portée : toute la maison.** La question se posera à tout produit dont la caméra
est pilotée autrement qu'à la souris.

**Ce que disent les sources.** `design/tokens.json:48-57` porte un bloc `viewer`,
décrit comme « constantes du viseur 360, **partagées par les six produits qui en
portent un** » : champ par défaut 75°, bornes 30–120°, tangage borné à
`[−89.9, 89.9]`, 0,18 °/px au glisser, HUD en bas à droite, taille 11.
`design/ERGONOMIE.md:174-176` nomme les six, et ce produit n'en est pas — il n'est
pas de la suite.

`MAISON.md:109` tranche la moitié gestuelle : « **Là où il est piloté par un
potard, la convention ne s'applique pas.** »

**Ce que fait le code.** Le regard n'est jamais manipulé à la souris — vérifié :
aucun glisser n'est lu sur la vue 360 ni sur la bande programme. Il est piloté par
quatre curseurs dans un popup, et par les potards d'égalisation de la voie 1 via
le mapping. Quatre destinations existent pour cela : lacet, tangage, champ, zoom.

Les valeurs employées :

| Constante | `tokens.json` | Le produit |
|---|---|---|
| champ par défaut | 75° | **90°** |
| bornes du champ | 30–120° | **20–170°** |
| tangage borné | ±89,9° | **±90°** |
| °/px au glisser | 0,18 | sans objet |
| HUD | bas à droite, 11 px | un résumé sur l'en-tête du deck |

**La question.** Un produit qui reprojette une sphère mais ne la manipule pas à la
souris doit-il les constantes `viewer` ?

Le geste est réglé. Les **nombres** ne le sont pas, et ils ne dépendent pas du
geste : le champ par défaut, ses bornes et la borne de tangage sont des propriétés
de la reprojection, pas de la souris.

**Ma recommandation : dissocier le bloc en deux, et n'exiger que la moitié
géométrique.**

- **Géométrie** — `pitch_deg_clamp`, `fov_deg_min/max/default` : exigibles de tout
  produit qui reprojette une sphère, quel que soit le geste. La raison de la borne
  de tangage est écrite en `ERGONOMIE.md:217-218` et elle est purement
  mathématique : « au pôle exact, le lacet devient indéfini et le viseur saute ».
  Un potard poussé à fond tombe sur le pôle exactement comme une souris.
- **Geste** — `drag_deg_per_px` : n'a de sens que là où il y a un glisser.
- **HUD** — position et taille : appartiennent au châssis, donc au niveau de
  conformité, pas au bloc `viewer`.

**Sur les valeurs elles-mêmes, une réserve en faveur du produit.** Les bornes
20–170° ne sont pas une négligence : le mode « little planet » a besoin d'un champ
que 120° ne couvre pas, et le code borne à 170° avec la raison écrite — « une
projection planaire ne peut pas atteindre 180 degrés ». Si la moitié géométrique
devient exigible, **les bornes doivent dépendre de la reprojection choisie**, pas
être uniques. C'est une modification à `tokens.json` que je recommande de faire
en même temps que la réponse, faute de quoi la règle sera inapplicable au premier
produit qui l'applique.

**±89,9 contre ±90**, en revanche, est à corriger côté produit sans discussion :
c'est le « détail que seul ce métier connaît » de
`DIRECTION-ARTISTIQUE.md:218-219`, et ce produit en porte déjà d'autres.

---

## SCRATCHVJ-17 — Le papier sombre : ce que `suite.css` doit porter

**Portée : toute la ligne scène.** C'est la variable 6 du cadran, et c'est le
point où la ligne scène n'a rien du tout.

**Ce que dit la source.** `docs/secteurs.html:435-441`, variable 6 : le papier
d'atelier est « fond clair, corps 16 px, marges (`suite.css`) » ; celui de scène
est « **fond sombre** ». La raison : « le document de scène est lu en coulisse,
sur un téléphone, dans le noir — et le code visuel du marché y est sombre ». Et
la dernière phrase vaut décision : « *Ce PDF-ci est sur fond sombre à votre
demande : c'est donc le premier document de la ligne scène, et il vaut
décision.* »

Le tour 01 de Diorama l'a confirmé du côté maison
(`remontees/diorama-01-reponses.md:406-412`) : le papier sombre « existe, mais il
appartient à la **ligne scène** », et la ligne de `suite.css` qui dit « aucun fond
sombre » n'est pas contredite parce que « ce n'est pas un interdit universel,
c'est la valeur atelier d'une variable de cadran ».

**Ce qui existe réellement.** Rien. `docs/suite.css` porte un papier, un seul, et
il est clair. Sa dernière ligne — la 261 — écrit : « **Aucun thème sombre.** Un
manuel se lit et s'imprime sur du clair ; le sombre appartient à l'application, où
il y a une image à juger. » C'est une règle générale, sans renvoi au cadran, et
elle contredit la variable 6. Le tour 01 n'a corrigé que la ligne 191 ; celle-ci
est restée.

**Ce que fait le produit.** Il a sa propre feuille, `docs/pdf/_style.css`, 254
lignes, employée par un manuel de 33 pages et un argumentaire de 13 pages. Et
elle est **claire** — c'est-à-dire au papier *atelier* — avec une **couverture en
aplat sombre**, que `DIRECTION-ARTISTIQUE.md:168-171` interdit nommément (« Pas
de couverture en aplat. Ni sombre, ni colorée »). La variable 6 n'est donc pas
appliquée non plus dans le produit : le relevé constate qu'elle ne coïncide avec
aucune des deux valeurs.

**Conformément à `MAISON.md:100`, cette feuille n'a pas été réécrite, et je ne
recommande pas de la réécrire.** Ce qui suit est ce qu'un thème sombre de
`suite.css` devrait porter pour la remplacer sans perte.

### Ce qu'un thème sombre de `suite.css` doit porter

**Sa forme.** Un bloc de redéfinition de variables sous une classe de ligne, pas
une seconde feuille : `.line-scene { --paper: …; --ink: …; }`. Tout le reste de
la feuille — structure, échelle, filets, tableaux, encadrés, couverture,
sommaire — reste littéralement le même code. C'est ce qui rend la cible « une
feuille, deux papiers » vraie plutôt que déclarative : le jour où le sommaire
change, il change pour les deux salles.

**Les sept jetons à inverser**, et pour chacun ce que la feuille locale a dû
inventer :

| Jeton | Atelier | Ce que la feuille locale emploie | Ce que le thème sombre doit porter |
|---|---|---|---|
| `--paper` | `#F6F7F4` | `#141412` sur la couverture seulement | le noir chaud de la variable 3, à la luminance qui sortira de SCRATCHVJ-06 |
| `--paper-sunk` | `#EDEFEA` | `#0E0E0C` (blocs de code) | un cran **plus sombre** que le papier, pas plus clair |
| `--ink` | `#141815` | `#f2efe9` | la craie chaude |
| `--ink-dim` | `#5C635B` | `#a8a29a` | — |
| `--ink-off` | `#8D958C` | `#7d786f` | — |
| `--rule` | `#D6DAD3` | `#2e2c28` | — |
| les quatre signaux | assombris pour le fond clair, `:38-40` | inventés localement, dont un `warn` **rouge** | **éclaircis** pour le fond sombre, avec la même raison écrite qu'en `:38-40` |

**Les quatre choses que le thème doit régler et que l'inversion ne suffit pas à
donner.**

1. **L'impression.** C'est le vrai obstacle, et c'est pour lui que la feuille
   locale existe. Un PDF de 33 pages à fond noir plein bord ne s'imprime pas :
   selon le moteur, ou l'encre coule, ou chaque page sort avec un liséré blanc
   dans les marges. `secteurs.html` lui-même a dû contourner le problème par un
   montage explicite — marges de page à zéro, et marges visuelles reprises par un
   tableau à en-tête et pied répétés. **Le thème sombre doit soit embarquer ce
   montage, soit déclarer que la ligne scène ne produit pas de PDF imprimable et
   n'écrire que pour l'écran.** Je recommande la seconde : `:438-441` dit que le
   document de scène est lu « en coulisse, sur un téléphone » — personne ne
   l'imprime, et prétendre le contraire coûte le montage à chaque page.
2. **La couverture.** `suite.css:194-216` est écrite pour du clair, mais elle ne
   contient aucune valeur de couleur : filet d'accent de 2 px, nom, ligne
   descriptive, ligne d'imprint en chasse fixe. **Elle fonctionne telle quelle sur
   fond sombre.** Le remplacement de la couverture en aplat de la feuille locale
   ne coûte donc rien — et il supprime du même coup l'aplat interdit.
3. **La chasse fixe.** La feuille locale emploie DM Mono avec une raison écrite
   dans son propre code : le moteur d'impression de Chrome refuse le fichier
   variable d'Archivo, tandis que DM Mono passe. C'est la même mesure que celle
   que `secteurs.html:470-473` demande d'employer pour arbitrer la mono unique.
   Le thème sombre en hérite : voir SCRATCHVJ-06, question 2.
4. **L'accent.** Il n'y a pas de classe produit pour un produit de scène :
   voir SCRATCHVJ-07.

**Ce qu'on perd si on ne fait rien.** La feuille locale a dix-sept tailles de
texte, vingt-cinq valeurs de couleur écrites en dur, un second orange, trois
étiquettes en aplat coloré et un `warn` rouge. Chacun de ces cinq points est un
écart déjà catalogué chez un produit d'atelier
(`DIVERGENCES.md:155-157`) — c'est-à-dire que **la ligne scène est en train de
refaire, seule, l'écart 22 qui a coûté trois chartes documentaires à la suite
360.** Et elle le refait pour une raison honorable : elle n'a pas de feuille à
employer.

**Une dernière observation, et elle n'est pas anecdotique.** Le document qui
fonde la ligne scène — `secteurs.html` — a un papier de `#0B0D0C`. Son propre
nuancier, deux pages plus loin, donne atelier `#0A0C0B` et scène `#141412`. Le
premier document de la ligne scène n'est donc **à aucune des deux valeurs** qu'il
prescrit — sa luminance relative vaut 0,0039, exactement entre les deux (0,0035
et 0,0069). Ce n'est pas un reproche : c'est la démonstration qu'un papier sombre
sans feuille commune dérive dès le premier document, y compris quand c'est celui
qui écrit la règle.

---

## SCRATCHVJ-18 — La détection au ratio seul, et `spec/evidence.py`

**Portée : propre au produit**, mais elle rejoint un point ouvert de la maison.

**Ce que dit la source.** `design/DIVERGENCES.md:110-120` : quatre
implémentations de la détection automatique du format cohabitent dans la suite,
avec quatre règles de priorité différentes. « Anamorphe et Lacuna déduisent du
seul ratio, Relief interroge ffprobe, **Vigie est le seul à lire les métadonnées
spatiales du fichier** et applique déjà une hiérarchie profil > métadonnées >
ratio. C'est exactement la hiérarchie `declared > metadata > inferred` du
manifeste. **Vigie a donc déjà écrit ce que `spec/evidence.py` doit devenir.** »
`remontees/REGISTRE.md:66` confirme que la règle reste ouverte, et l'attribue à
la maison « après la phase 3 de Mutoscope ».

**Ce que fait le code.** Ce produit est dans le camp d'Anamorphe et de Lacuna :
une image dont la largeur vaut exactement deux fois la hauteur est marquée
sphérique, et rien d'autre n'est consulté. La règle est écrite deux fois, dans
deux fichiers, et elle est commentée : « The 2:1 convention is how
equirectangular footage is recognised without metadata. » Aucune métadonnée
spatiale n'est lue, alors que le produit appelle déjà `ffprobe` pour la durée, la
cadence et les dimensions.

Le produit a en revanche ce que la hiérarchie appelle `declared` : l'utilisateur
peut forcer la projection d'un clip, et ce choix est écrit dans le fichier de
bibliothèque et l'emporte sur la déduction. Il possède donc **deux des trois
rangs**, `declared` et `inferred`, et il lui manque celui du milieu.

**La question.** Ce n'en est pas vraiment une, et c'est pour ça qu'elle est
courte : la règle est déjà attendue, et son implémentation de référence est déjà
identifiée. Ce que ce point apporte est un **cinquième cas mesuré**, et une
précision sur ce que la règle doit prévoir.

**Ma recommandation.** Deux choses.

1. **Quand `spec/evidence.py` s'écrira, il doit dire ce qui se passe quand un
   rang manque**, pas seulement dans quel ordre les trois se battent. Ce produit
   n'a pas `metadata` et n'en aura pas avant longtemps — lire les métadonnées
   spatiales de Google demande une dépendance qu'un instrument temps réel n'a pas
   de raison d'embarquer. Une règle à trois rangs dont un est facultatif est une
   règle différente d'une règle à trois rangs obligatoires, et c'est la
   différence qui décide si ce produit peut s'y conformer.
2. **Le ratio 2:1 doit rester une inférence explicitement faillible.** Ce produit
   le sait déjà et le montre : le sélecteur de l'inspecteur affiche « Auto (360°) »
   plutôt que « 360° », c'est-à-dire *ce que la machine croit, et qu'elle croit*.
   C'est exactement ce que la hiérarchie `declared > metadata > inferred` doit
   produire à l'écran, et c'est un motif réutilisable — je le signale parce qu'un
   document de spécification écrit sans lui produira une règle qui n'a aucune
   conséquence visible.

---

## SCRATCHVJ-19 — Ni `NOMS.md` ni `suite.css` ne portent de ligne pour ce produit

**Portée : propre au produit** — mais chaque produit de scène l'aura à son tour.

**Ce que disent les sources.** `design/NOMS.md:217-227` porte un tableau
« JEU ARRÊTÉ » de neuf lignes : la maison plus huit produits. Aucune ne
correspond à ce dépôt. `docs/suite.css:53-61` porte huit classes produit, une par
accent ; aucune non plus.

`docs/secteurs.html:320-345` explique pourquoi, et l'explication est complète :
le nom de travail décrit la fonction du produit, « ce qui est un motif absolu de
refus au dépôt » — le même défaut que *Relief*. Le registre est retenu : les
appareils de l'image animée d'avant le cinéma, dont quatre finalistes écartés du
nom de maison sont libres. Mais *Praxinoscope* fait douze lettres, et « **aucun
candidat court n'a été vérifié** » (`:342`).
`secteurs.html:664` classe la décision « en cours », à tenir « avant la première
vidéo publique : un nom qui a circulé ne se retire plus ».

**Ce que ça produit ici.** Deux effets concrets, aucun grave :

- ce fichier de remontées s'appelle `REMONTEES-SCRATCHVJ.md`, d'après un nom que
  la maison a déjà décidé de ne pas employer ;
- le produit ne peut pas prendre de classe dans la feuille documentaire commune,
  ce qui est l'un des deux verrous de la phase 7 (l'autre étant le papier sombre,
  SCRATCHVJ-17).

**La question.** Faut-il une ligne provisoire dans `NOMS.md` ?

**Ma recommandation : oui, une ligne, et pas un nom.** Le tableau du jeu arrêté
gagne une neuvième ligne :

| Emplacement | Nom | Ligne descriptive |
|---|---|---|
| `UE_SCRATCHING` | **— en cours** | instrument de scratch vidéo au timecode DVS ; registre retenu, candidat court à vérifier |

Trois raisons.

1. **C'est la forme déjà employée pour la maison.** `NOMS.md:219` porte
   « ~~Theama~~ **— non arrêté** » avec sa raison. Une absence écrite se
   distingue d'un oubli ; une absence muette, non — et c'est exactement le motif
   que `DIORAMA-09` a fait retenir pour `DIVERGENCES.md`.
2. **La ligne descriptive est utile immédiatement**, indépendamment du nom.
   `secteurs.html:176-179` rappelle que c'est elle qui porte les mots-clés
   cherchés, puisqu'un nom propre n'est pas trouvable.
3. **Elle empêche l'erreur qui vient.** Sans ligne, la prochaine session qui
   ouvrira `NOMS.md` conclura que le produit a été oublié, et proposera un nom.
   `secteurs.html:342-345` dit que c'est « un travail à part, à ne pas improviser
   dans ce document ».

**Ce que je ne recommande pas** : réserver `.p-scratchvj` dans `suite.css`. Une
classe posée sous un nom de travail dans une feuille que huit produits partagent
serait à renommer, et le renommage traverserait alors chaque document déjà
publié.

---

## SCRATCHVJ-20 — Quatre inexactitudes dans les sources

**Portée : toute la maison.** Aucune ne bloque quoi que ce soit ; toutes ont fait
perdre du temps à cette session, et la troisième a failli produire un travail
inutile.

**1. `design/ERGONOMIE.md:428` compte dix touches réservées ; la table `:245-258`
en porte douze lignes.** La liste de contrôle demande de vérifier que « les
**dix** touches réservées ont leur effet, et aucun autre ». En comptant les
lignes de la table — `0`, `Ctrl`+`1`/`2`/`3`, `C`, `Espace`, `←`/`→`,
`Maj`+`←`/`→`, `Tab`, `G`, `?`/`F1`, `Échap`, `Ctrl`+`Z`, `Ctrl`+`Maj`+`C`/`V` —
on en trouve douze.

Recommandation : **remplacer le nombre par un renvoi**, « les touches réservées de
la table ci-dessus ». C'est le remède que `DIORAMA-05` a déjà appliqué aux six
viseurs, et pour la raison que la source écrit elle-même en `:175-177` : « le
compte a déjà servi de point d'appui à une session pour croire qu'un produit
devait en avoir un ». Un compte dans une liste de contrôle est un compte qu'on
vérifiera, et qui divergera à la prochaine ligne ajoutée.

**2. La définition de fini de `MAISON.md` dépasse ses phases.** Les lignes
`181-183` exigent quatre choses qu'aucune des huit phases ne couvre : qu'aucune
erreur n'ouvre de modale et que chacune nomme la valeur en cause et l'action qui
répare ; qu'aucune progression n'affiche un pourcentage seul ; qu'aucune unité ne
soit écrite dans un libellé ; qu'un chemin tronqué le soit par le milieu.

Ce n'est pas théorique : dans ce dépôt, trois des quatre ont du travail réel — deux
progressions en pourcentage seul, deux unités dans un libellé, quatre erreurs sur
cinq qui ne nomment pas la réparation. La quatrième est satisfaite sans effort
(aucun chemin n'est tronqué).

Recommandation : **que le générateur ajoute une phase « la liste de contrôle »**,
ou rattache chaque ligne de la définition de fini à une phase existante. Une
définition de fini qui contient du travail non planifié se solde de deux
manières : soit la phase 0 l'invente, ce qui la fait sortir de son rôle, soit
personne ne le fait et la définition n'est jamais atteinte.

**3. `MAISON.md:108` annonce un piège qui n'existe pas.** « Vérifier qu'aucune
transition ImGui n'a été laissée à sa valeur par défaut. » La bibliothèque
d'interface employée ici n'a **aucun champ de durée de transition** : il n'y a
pas de valeur par défaut à corriger, parce qu'il n'y a pas de champ. La variable 5
du cadran est tenue sans que personne n'ait eu à la tenir.

Recommandation : **retirer la ligne**, ou la reformuler en « vérifier qu'aucune
animation d'interface n'existe » — ce qui est vérifiable et se vérifie. Le
signaler parce qu'un piège annoncé est un piège qu'on cherche, et que
`MAISON.md:24` avertit justement contre le fait d'« aligner sur ce qu'on croit
savoir ».

**4. `design/README.md:43-44` du produit désigne le mauvais fichier.** Il dit que
les jetons de la maquette sont ceux de la fonction de style « dans
`scratchvj/ui/main_ui.cpp` » ; cette fonction est dans `scratchvj/ui/panels.cpp`.
C'est du ressort du produit, pas de la maison — noté ici pour mémoire, parce que
la phase 5 déplacera cette fonction et devra corriger le renvoi.

---

## Ce que les réponses débloquent

### Réponses qui débloquent du travail

| Remontée | Ce qui repart |
|---|---|
| 01, 02 | **la phase 2** — la forme canonique à écrire sur disque, et la migration du fichier de bibliothèque, qui se fait une fois ou pas du tout |
| 03, 13 | **la phase 2** — les 25 libellés à réécrire, et les six sélecteurs de projection |
| 04, 05, 06, 07, 08, 09, 10, 11 | **la phase 5** — le fichier unique de jetons. Il ne peut pas s'écrire sans savoir quelle échelle, quelle luminance, quelle mono, quels signaux, quel emploi de l'accent |
| 14 | **la phase 3**, en partie — le format d'un temps dépend de ce qui fait autorité sur le temps |
| 15 | **la phase 4** — le noyau du clavier, et ce qui est exigible d'un instrument sans état persistant |
| 17 | **la phase 7** — elle n'a rien à écrire tant que la feuille commune n'a pas de papier sombre |

### Réponses qui ne débloquent rien, et qu'on peut prendre à loisir

12 (le rayon, deux lignes à changer quel que soit le verdict), 16 (les constantes
du viseur, sans effet tant que le regard reste au potard), 18 (`evidence.py`,
déjà attendu de Mutoscope), 19 (une ligne dans un tableau), 20 (quatre
corrections rédactionnelles).

### Ce qui part sans attendre de réponse

Trois chantiers de produit sont écrits dans le relevé et n'ont besoin de personne :
la pile d'`Échap` et le rang qui ferme la fenêtre (phase 4) ; les deux
progressions en pourcentage seul et les erreurs qui ne nomment pas la réparation
(phase 4) ; les deux documents commerciaux qui présentent la licence GPL comme une
question ouverte alors qu'elle a été tranchée le 9 septembre (phase 6).

---
---

# Tour 02

Six points relevés le **2026-09-10** en appliquant les verdicts du tour 01
(`REPONSES-SCRATCHVJ-01.md`), sur la branche `claude/scratch-video-unreal-0oi7dv`
à partir de `9ea2eba`. La numérotation continue après `SCRATCHVJ-20`. Aucun de
ces points n'a été tranché dans le dépôt ; ce qui a été fait est dans
`docs/ALIGNEMENT.md`, « Application des verdicts du tour 01 ». Les numéros de
ligne sont ceux du disque au 2026-09-10, après application.

| # | Objet | Portée | Ce que ça bloque |
|---|---|---|---|
| 21 | La valeur chaude de `warn`, produite — et sa teinte frôle l'accent | ligne scène | `tokens.json` v4, `lines.scene.signal.warn` |
| 22 | « Plate » ou « Rectiligne » : `SCENE.md` et la table d'`ERGONOMIE.md` ne disent pas le même mot | ligne scène, et la maison | **la phase 2** |
| 23 | L'ambre résiduel : une valeur hors tolérance, et les états de dérive hors du lien de platine | ligne scène | la phase 5 (le fichier de jetons fige les emplois) |
| 24 | Une étiquette de source de plus de deux caractères, et la troisième source sans couleur | ligne scène, et la maison | la phase 5 |
| 25 | Le champ par défaut : `viewer.geometry` selon deux sources, `viewer.plate` selon la troisième | toute la maison | la phase 5 |
| 26 | `MAISON.md` régénéré le 2026-09-10 contredit deux verdicts du tour 01 | ce produit, par le générateur | la phase 4 et la phase 5, à la lecture |

---

## SCRATCHVJ-21 — La valeur chaude de `warn`, produite — et sa teinte frôle l'accent

**Portée : toute la ligne scène.** C'est une valeur remise, pas une question ;
la question est en dessous.

**Ce que disent les sources.** `design/tokens.json:105-108` : `lines.scene.signal`
porte `done` et `fail`, et son `$comment` dit que `warn` « n'a AUCUNE valeur de
scène » et que « qui doit la produire : scratchvj, en donnant `warn` au lien qui
faiblit ». `design/SCENE.md:342` inscrit la même ligne dans « ce qui reste
ouvert ». `tokens.json:44` donne la valeur d'atelier, `#C48A4B`.

**Ce que fait le code.** `scratchvj/ui/panels.cpp:42` : `kWarn = #9C774E`, donné
au lien de platine qui faiblit (`link_colour()`, `:101-108`, et le voyant de la
barre haute). La valeur n'est pas choisie à l'œil ; elle est **dérivée** par la
règle que la table appliquait déjà, sans l'écrire, à `done` et `fail` : même
teinte que la valeur d'atelier, clarté et saturation HSL multipliées par la
moyenne des rapports mesurés sur ces deux jetons.

| Jeton | Atelier | Scène | Clarté | Saturation | Luminance relative |
|---|---|---|---|---|---|
| `done` | `#7FB069` | `#7E946B` | ×0,907 | ×0,519 | ×0,729 |
| `fail` | `#D9584B` | `#B54B3A` | ×0,818 | ×0,790 | ×0,682 |
| **`warn`** | `#C48A4B` | **`#9C774E`** | ×0,863 (moyenne) | ×0,654 (moyenne) | ×0,684 |

La luminance de `warn` tombe entre celles des deux autres. Le calcul est écrit
en commentaire à côté de la valeur, comme `maison/01-LES-TROIS-COUCHES.md` le
demande d'un jeton que le toolkit n'a pas su lire dans la source.

**La question.** `#9C774E` a une teinte de 31,5° ; l'accent candidat du produit,
`#C9762F` (`panels.cpp:36`), en a une de 27,7°. `DIRECTION-ARTISTIQUE.md:267-276`
compte déjà quatre valeurs à deux rôles, dont `#C48A4B` — `warn` **et** l'accent
de Lacuna Motion. Sur la scène, le même voisinage naît entre `warn` et l'accent
du produit, à un mètre dans le noir, où seule la clarté les sépare.

**Ma recommandation : entrer `#9C774E` dans `lines.scene.signal.warn`, et laisser
l'accent bouger s'il le faut, pas le signal.** La paire et les signaux sont de
l'état, jamais de la couche 3 (`SCENE.md`, variable 4) ; l'accent, lui, attend un
nom et un écran calibré (`SCENE.md`, « ce qui reste ouvert », l'accent du
produit). Entre une valeur qui attend deux décisions et une valeur qui n'en
attend aucune, c'est la première qui se déplace. Noter la collision dans le
tableau des rôles doubles, pour que la séance sur écran calibré regarde cinq
voisinages et non quatre.

---

## SCRATCHVJ-22 — « Plate » ou « Rectiligne » : deux sources, deux mots pour `flat`

**Portée : la ligne scène, et la maison.** Bloque la phase 2.

**Ce que disent les sources, et elles se contredisent.**

- `design/SCENE.md:290`, « ce qui s'applique intégralement » : « on y écrit
  « 360 » et « Plate », avec la forme longue accessible à côté ». C'est la
  ligne que le verdict `SCRATCHVJ-03` a produite
  (`REPONSES-SCRATCHVJ-01.md:167-171` : « « Plate » fait cinq lettres ; la place
  ne justifie rien »).
- `design/ERGONOMIE.md:715`, la table du langage : `flat` → « **Rectiligne** » /
  « Flat ». Et `:744-749` : « « Plate » a été retiré de la ligne `flat` le
  2026-09-09 (`LACUNA-17`) : c'est le nom de l'objet que tous les produits
  manipulent, et « une plate en projection Plate » n'est lisible ni à l'écrit ni
  à l'oral. »
- `ERGONOMIE.md:815-821` : une forme courte est bornée par la place, et elle est
  une **troncature de la forme longue**, jamais un autre mot.

Les deux textes sont du même jour. Le verdict de scène a écrit « Plate » pendant
que la table de la maison le retirait, et `SCENE.md` a recopié le verdict.

**Ce que fait le code.** `scratchvj/ui/panels.cpp:883, 1229, 1500-1501, 2073` :
`"2D"` et `"360°"`, ce que la phase 2 doit remplacer — et elle ne peut pas
choisir le mot.

**La question.** Quel est le libellé de `flat` à la scène : « Plate »
(`SCENE.md`) ou « Rectiligne » (la table qui « impose le mot ») ?

**Ma recommandation : « Rectiligne », et corriger `SCENE.md`.** Trois raisons.
(1) `ERGONOMIE.md` dit de sa table qu'elle est « la source des deux colonnes ;
aucun produit n'y ajoute un mot de son cru » — un cadran n'est pas un produit,
mais il n'est pas non plus une seconde table. (2) La raison de `LACUNA-17` vaut
ici aussi : ce produit lit des plates (le manifeste de plate ne le concerne pas,
mais le mot, si — `MAISON.md` l'emploie). (3) Un mot par concept dans les deux
langues est ce que la règle du langage protège ; « Plate » à la scène et
« Rectiligne » à l'atelier en font deux. Ce qui reste vrai du verdict 03 : la
forme courte est bornée par la place, et « Rectiligne » ne tient peut-être pas
dans le même sélecteur que « 360 ». Si c'est le cas, la forme courte est une
troncature de « Rectiligne », pas « Plate » — et laquelle, la maison le dit,
puisque ce serait la première troncature d'un mot de la table hors des trois
abréviations connues.

---

## SCRATCHVJ-23 — L'ambre résiduel : une valeur hors tolérance, et les états de dérive

**Portée : toute la ligne scène.** Bloque la phase 5, qui fige les emplois de
chaque couleur.

**Ce que disent les sources.** `DIRECTION-ARTISTIQUE.md:193` : `fail` est « la
seule couleur autorisée à porter du texte ». `:206-215` : « une étiquette d'un ou
deux caractères qui nomme une source n'est pas du texte au sens de cette règle.
Une phrase l'est toujours. » `tokens.json:44` : `warn` = « terminé avec réserve,
ou dérive détectée ». Le verdict `SCRATCHVJ-08` retire l'ambre des états « en
cours » ; `SCRATCHVJ-09` donne `warn` « au lien de platine qui faiblit ».

**Ce que fait le code, après application.** L'ambre reste à deux endroits qui ne
sont ni le deck A ni « en cours », et que les deux verdicts ne nomment pas.

*Des valeurs colorées hors tolérance* — un nombre, pas une phrase, pas une
étiquette de source :

| Où (`ui/panels.cpp`) | Ce qui est coloré | Quand |
|---|---|---|
| `:1910-1914` | la balance en dB et l'erreur de phase du scope | hors de ±1 dB, hors de ±5° |
| `:1957-1958`, `:2176` | la vitesse du plateau | au-delà de 2,5× |
| `:1964` | la confiance du décodeur, en % | selon l'état du lien (`link_colour`) |

*Des états de dérive portés par un point ou une barre*, pas par du texte :

| Où | Ce que c'est | Couleur |
|---|---|---|
| `:623` | la table MIDI partiellement connectée | ambre |
| `:1871` | la figure de Lissajous quand le scope n'est pas équilibré | ambre |
| `:2027` | la jauge de fraîcheur de l'ancre (`stale > 0,6`) | ambre |
| `:3707` | une liaison activée mais inactive | ambre |

Chacun de ces sites dit « ça marche encore, mais ça dérive », c'est-à-dire le
sens exact de `warn` — et ils portent la couleur du deck A.

**Les questions.**

**Q1 — un nombre coloré est-il du texte au sens de la règle du `fail` ?** La
clause des étiquettes ne le couvre pas (ce n'est pas une source), celle de la
phrase non plus (ce n'est pas une phrase). Recommandation : **oui, c'est du
texte, et il revient à la craie** ; la dérive se dit par le voyant ou la jauge
d'à côté, que ce produit double déjà d'un mot partout. Un chiffre qui change de
couleur est le mécanisme même qu'un instrument de mesure ne fait pas.

**Q2 — les états de dérive prennent-ils `warn` au-delà du seul lien de
platine ?** Recommandation : **oui, par le sens du jeton.** Le verdict 09 a nommé
un site parce qu'un site avait été remonté ; laisser les quatre autres en ambre
reconstruit le double rôle que le verdict 08 vient de retirer.

---

## SCRATCHVJ-24 — Une étiquette de source de plus de deux caractères, et la troisième source sans couleur

**Portée : la ligne scène, et la maison** — la clause des étiquettes est une
clause de maison, et elle vient de servir à Mutoscope.

**Ce que disent les sources.** `DIRECTION-ARTISTIQUE.md:206-215` : « une étiquette
d'**un ou deux caractères** qui nomme une source ». `tokens.json:101-102` :
`lines.scene.pair` porte `a` et `b`, et rien d'autre. `tokens.json:107` :
`done` = `#7E946B`.

**Ce que fait le code.** Deux choses que la clause ne couvre pas.

- **Des mots de source colorés.** `ui/panels.cpp:859, 864` : « Deck A » en ambre,
  « Deck B » en ardoise, sur des boutons ; `:1197-1199`, `:1551-1553`, `:1761` :
  les mêmes sur des puces de cible. Six caractères, pas deux ; mais ils
  **nomment une source**, exactement comme « A » et « B ».
- **Une troisième source, sans couleur dans la paire.** L'incrustation est la
  troisième cible du produit (`DeckTarget::Overlay`) et le code lui a donné le
  **vert de `done`** — `:869`, `:1199`, `:1553`, `:4604`, `:4617`. C'est un
  signal employé comme identité : le troisième rôle de `#7E946B`, et le motif
  que `DIRECTION-ARTISTIQUE.md:267-276` compte comme une collision.

**Les questions.**

**Q1 — la clause tient-elle par le compte de caractères ou par ce que
l'étiquette nomme ?** Recommandation : par ce qu'elle nomme. « Un ou deux
caractères » décrit le cas qui l'a fait naître, pas sa limite ; « Deck A » sur
un bouton n'est pas lu comme une alarme davantage que « A ». Réécrire la clause
par son critère — *un nom de source, jamais une phrase* — comme la maison vient
de le faire pour le clavier et pour la forme courte, qui avaient le même mode de
panne (énoncées par l'exemple, lues sur l'exemple).

**Q2 — l'incrustation a-t-elle droit à une couleur, et laquelle ?** La paire est
« A/B », « constante sur toute la ligne » (`SCENE.md`, variable 4) ; un troisième
membre ne s'invente pas dans un dépôt. Recommandation : soit `lines.scene.pair`
gagne une troisième valeur pour la couche d'incrustation — un produit de scène
qui compose deux sources en a presque toujours une troisième — soit
l'incrustation se nomme à la craie. Dans les deux cas, `done` cesse d'être une
identité : c'est un retrait, il se fera dès la réponse.

---

## SCRATCHVJ-25 — Le champ par défaut : `viewer.geometry` selon deux sources, `viewer.plate` selon la troisième

**Portée : toute la maison.** Trois sources, deux chemins.

**Ce que disent les sources.**

- `design/SCENE.md:304-307` : « Les constantes de `viewer.geometry` restent
  exigibles de tout produit qui reprojette une sphère, y compris quand le regard
  est piloté au potard. »
- `spec/00-vocabulaire.md:237-243` : « Le domaine de champ appartient à la
  reprojection, pas au produit. […] Seul celui de `rectilinear` est écrit à ce
  jour, dans `viewer.geometry`. »
- `design/tokens.json:65-77` : `viewer.geometry` ne porte que
  `pitch_deg_clamp` ; `fov_deg_default` (75), `fov_deg_min` (30) et
  `fov_deg_max` (120) sont sous **`viewer.plate`** — « le viseur qui regarde une
  plate : l'œil est au centre, sa seule profondeur est la focale, donc la molette
  lui revient » — depuis l'arbitrage de `GEORAMA-06`, rendu après
  `SCRATCHVJ-16`.

**Ce que fait le code.** Le tangage est borné à ±89,9° par `kPitchClampDeg`
(`core/sphere.h:41`, fermé). Le champ par défaut est **90°** (`core/sphere.h:47`),
borné à 20–170° (`app/engine.cpp:516`, `ui/panels.cpp:2322`) parce que little
planet a besoin d'un champ que 120° ne couvre pas — raison que `tokens.json:74`
cite et accepte.

**Les questions.**

**Q1 — le bloc `viewer.plate` lie-t-il ce produit ?** Son œil est au centre de la
sphère et sa seule profondeur est le champ : c'est la définition de `plate`.
Mais le regard est un potard, il n'y a pas de molette, et le produit n'est pas
un viseur au sens du châssis (`SCENE.md`, « sans objet »). Recommandation :
séparer ce que `plate` dit de la **molette** (sans objet ici) de ce qu'il dit du
**champ** (qui ne dépend pas du geste, comme le tangage) — et ranger le champ
là où `00-vocabulaire.md` dit qu'il est, avec le domaine : sur la reprojection.
Deux sources sur trois écrivent `viewer.geometry` ; le fichier de jetons, qui
fait foi, écrit `viewer.plate`. L'un des chemins est faux, et la phase 5 doit
en citer un.

**Q2 — 90° ou 75° par défaut ?** 75° est écrit pour un moniteur à 60 cm où l'on
juge une plate ; 90° est ce qu'on projette à un public, et c'est une valeur en
service. Recommandation : ne pas trancher dans le dépôt ; si le champ par défaut
est du cadran (il dépend de la distance de lecture, variable 1), il va sous
`lines.scene` avec sa raison ; sinon il retourne à 75, et la phase 5 le change
en une ligne.

---

## SCRATCHVJ-26 — `MAISON.md` régénéré le 2026-09-10 contredit deux verdicts du tour 01

**Portée : ce produit, par `tools/gen-maison.py`.** `maison/02-INTERDITS.md`
dit qu'un fichier généré faux se répare dans le générateur, et `MAISON.md`
lui-même demande qu'une de ses lignes qui ne se retrouve pas sur le disque se
remonte.

**Ce que dit le talon.**

- `MAISON.md:57` : « Le **noyau** réservé, lui, doit tenir : `Espace`, `Échap`
  selon la pile de priorité, **`Ctrl`+`Z`**, `?` / `F1`, `Tab`. » Et `:96`,
  phase 4 : « … `Ctrl`+`Z`, `?` / `F1`, `Tab` s'il a un sens ». Source :
  `tools/gen-maison.py:642, 657`.
- `MAISON.md:58` : la provenance du fichier de jetons à écrire — « *ligne scène,
  candidat, en attente de `tokens.json` v3 `lines.scene`* ».

**Ce que disent les verdicts, rendus la veille.** `SCRATCHVJ-15`, § 2 : `Ctrl`+`Z`
n'est pas exigible — « un produit sans état persistant n'a pas d'annulation à
offrir », clause désormais écrite dans `ERGONOMIE.md` sous la table du clavier.
`SCRATCHVJ-07` : `tokens.json` **est** en v3 avec `lines.scene`, et l'en-tête
attendu est « ligne scène, tokens.json v3, lines.scene ». `Tab`, enfin, est
rouvert par la suite elle-même (`ERGONOMIE.md`, « La touche qui masque
l'interface est rouverte ») : « `Tab` s'il a un sens » est vrai, mais « comme
`Tab` à l'atelier » ne l'est plus.

**La question.** Aucune : c'est une correction du générateur, signalée pour
qu'une session de phase 4 ne lise pas dans le talon une exigence que le verdict
a levée, ni en phase 5 une attente déjà satisfaite. Recommandation : la ligne du
noyau se réécrit par le critère (« les touches réservées que le produit lie, et
celles qu'il a la chose de lier »), avec `Ctrl`+`Z` **sans objet** pour ce
produit ; l'en-tête des jetons prend la forme du verdict 07.
