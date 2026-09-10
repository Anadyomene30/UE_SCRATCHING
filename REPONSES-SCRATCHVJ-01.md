# Réponses de la maison — scratchvj, tour 01

Rendues le **2026-09-09** depuis `Suite 360`, sur
`GITHUB/UE_SCRATCHING/REMONTEES-SCRATCHVJ.md` et son relevé
`docs/ALIGNEMENT.md`.

Vingt remontées, vingt verdicts. **Quinze « tranché, source modifiée », un
« tranché », quatre « reporté ».** Aucun « refusé », aucun « hors maison » — et
c'est un résultat, pas de la complaisance : ce relevé ne demande nulle part une
fonctionnalité. Il demande une source qui n'existe pas.

---

## Ce que ce tour est, et pourquoi il ne ressemble à aucun des huit autres

**Ce produit n'est pas le neuvième de la suite. C'est le premier de la scène**,
et les niveaux A / B / C ne s'y appliquent pas : ce qui le gouverne est le
cadran, sept variables qui prennent une autre valeur, chacune pour une raison
écrite (`maison/05-QUI-EST-QUI.md:96-108`, `MAISON.md:34` du produit).

**Le cadran a fonctionné, et c'est le résultat le plus important de ce tour.**
Le relevé a trouvé **sept écarts apparents qui sont des conformités écrites** :
le fond `#141412` et les cinq couleurs de châssis chaudes (variable 3), l'aplat
orange (variable 2), la paire ambre / ardoise des decks (variable 4), le corps à
15 px et la cible à 44 px (variable 1), l'absence totale d'animation
(variable 5), les capitales sur les sur-titres de panneau
(`DIRECTION-ARTISTIQUE.md:219-220`), et l'emploi de `F` pour « image seule »
(`ERGONOMIE.md:265-270`). Sans le cadran, six chantiers auraient été ouverts et
auraient détruit ce que `secteurs.html:655` interdit nommément de casser. **Une
session qui aligne sans cadran ne se trompe pas de règle : elle se trompe de
salle.**

**Et le cadran a montré ce qui lui manque.** Treize des vingt remontées ne sont
pas du travail de produit : ce sont les trous de la ligne scène. La scène n'a
pas d'échelle typographique fermée, pas de valeurs de densité, pas de jeton de
filet, pas de couleur d'avertissement, pas de papier de document, et **sa
variable 7 est la seule des sept dont la colonne ne porte aucun nombre**.

> **Un produit ne peut pas se conformer à une colonne vide.**

La cause est une, et elle explique les treize : **l'atelier a trois sources
(`DIRECTION-ARTISTIQUE.md`, `ERGONOMIE.md`, `tokens.json`), le terrain en a une
(`design/TERRAIN.md`), la scène n'en a aucune.** Son cadran vit au § 5 d'un
document commercial, `docs/secteurs.html`, qui n'est pas dans `design/`, qui
n'est pas dans la liste des sources de `maison/00-LIRE-DABORD.md:54-70`, et qui
se lit à travers du HTML. `maison/00:86` a raison de dire qu'il gouverne ; c'est
l'adresse qui est fausse.

**La réponse générale de ce tour tient donc en un fichier :
`design/SCENE.md`**, le second cadran écrit comme une source, sur le modèle
exact de `TERRAIN.md` — qui est le troisième, qui a été écrit le même jour, et
qui a la place que la scène n'a pas. Treize verdicts ci-dessous y déposent
quelque chose. Aucun n'invente : chacun écrit ce que le produit fait déjà par
nécessité, ou ce qu'une source dit déjà ailleurs.

---

## Ce qu'il faut savoir avant de lire

**Sur les numéros de ligne.** Les renvois placés sous « ce que les sources
disent » sont ceux de l'état **d'aujourd'hui**, revérifiés un par un.
`design/ERGONOMIE.md` a été révisé le 2026-09-09 après le tour de Diorama et les
grappes : ses numéros ont glissé de trois à cinq lignes depuis ceux que la
remontée cite. Le décalage est signalé là où il change quelque chose ; ailleurs
il ne change rien.

**Sur `design/tokens.json`.** Il est modifié, et c'est le premier tour où ça
arrive : `"version": 2` passe à `3`, `"revised"` au 2026-09-09. **Aucun produit
n'a de fichier de jetons à recopier** — `maison/01-LES-TROIS-COUCHES.md:108-112`
constate qu'au 2026-09-09 aucune de ces copies n'existe encore. C'est
précisément la fenêtre où toucher le fichier ne coûte rien, et elle se refermera
au premier produit qui écrit la sienne. **scratchvj est ce produit-là, en
phase 5.**

**Sur les grappes.** Cinq des seize grappes du 2026-09-09 touchent ce produit ;
elles ne sont pas rejouées, elles sont citées. **Trois affectations du registre
sont fausses et sont corrigées plus bas** — `SCRATCHVJ-01` n'est pas dans G5,
`SCRATCHVJ-20` n'est pas dans G1, et `SCRATCHVJ-23` n'existe pas. Le produit
avait raison : il n'a reposé **ni** le repère **ni** le nom de maison, et il l'a
écrit (`REMONTEES-SCRATCHVJ.md:27-34`). C'est la première fois qu'un relevé
prouve qu'il a lu le registre avant de remonter, et ça se voit à ce qu'il ne
remonte pas.

**Un seul manquement franc dans tout le relevé**, et il est tranché sans
discussion : `Échap` ferme la fenêtre au troisième appui. Voir `SCRATCHVJ-15`.

---

## Tableau de triage

| # | Objet | Portée | Verdict | Source touchée |
|---|---|---|---|---|
| 03 | Un libellé lisible à un mètre | scène | **tranché, source modifiée** | `ERGONOMIE.md` |
| 04 | L'échelle typographique de la scène | scène | **tranché, source modifiée** | `secteurs.html`, `SCENE.md`, `tokens.json` |
| 05 | La densité — la variable 7 sans valeur | scène | **reporté** | — |
| 06 | La luminance commune, et la mono unique | scène | **reporté** | `secteurs.html` (fait corrigé) |
| 07 | `lines.scene` n'existe pas | scène | **tranché, source modifiée** | `tokens.json`, `SCENE.md` |
| 08 | L'ambre porte deux rôles | scène | **tranché** | — |
| 09 | Les quatre signaux, et le `warn` manquant | scène | **tranché, source modifiée** | `DIRECTION-ARTISTIQUE.md` |
| 10 | La couleur qui porte du texte | scène | **tranché, source modifiée** | `DIRECTION-ARTISTIQUE.md` |
| 11 | L'aplat : état ou action, et qu'est-ce qu'un panneau | scène | **tranché, source modifiée** | `secteurs.html`, `gen-maison.py` |
| 13 | La capitale initiale sur un libellé de paramètre | scène | **tranché, source modifiée** | `ERGONOMIE.md` |
| 14 | Le format d'un temps et d'un angle | scène | **reporté** | — |
| 15 | Le clavier de la scène, et `Échap` | scène | **tranché, source modifiée** | `ERGONOMIE.md`, `secteurs.html`, `gen-maison.py` |
| 17 | Le papier sombre | scène | **reporté** | `secteurs.html` (fait corrigé) |
| 01 | La forme canonique de « non déclaré » | maison | **tranché, source modifiée** | `spec/00-vocabulaire.md` |
| 02 | Projection d'un fichier ≠ reprojection d'une vue | maison | **tranché, source modifiée** | `spec/00-vocabulaire.md`, `spec/03-ponts.md` |
| 12 | Le rayon des contrôles | maison | **tranché, source modifiée** | `secteurs.html` |
| 16 | Les constantes `viewer` au potard | maison | **tranché, source modifiée** | `tokens.json`, `spec/00-vocabulaire.md` |
| 20 | Quatre inexactitudes dans les sources | maison | **tranché, source modifiée** | `ERGONOMIE.md`, `gen-maison.py` |
| 18 | La détection au ratio seul, `evidence.py` | produit | **tranché, source modifiée** | `spec/01-manifeste-plate.md` |
| 19 | Aucune ligne pour ce produit dans `NOMS.md` | produit | **tranché, source modifiée** | `design/NOMS.md` |

---
---

# Partie I — Les treize trous de la ligne scène

Ces treize réponses ne sont pas des réponses à scratchvj. **Le copilote de
captation les reposera dans les mêmes termes, et tout produit de scène futur
aussi.** Elles sont donc écrites comme des règles de ligne, à déposer dans
`design/SCENE.md`, et pas comme des autorisations données à un produit.

---

## SCRATCHVJ-03 — Un libellé lisible à un mètre, dans le noir

**Ce que disent les sources.** `ERGONOMIE.md:377-391` donne la table des
libellés lisibles, un mot par concept, dans les deux langues :
`equirect_360` → « Équirectangulaire 360 », `flat` → « Plate ».
`spec/00-vocabulaire.md:95` bannit `2d` — *« préférer `flat` : une plate 360 est
aussi une image à deux dimensions »* — et `:94` bannit « `equirectangular`
seul ». En face, `secteurs.html:396` fixe la valeur scène de la variable 1 :
« 1 m dans le noir, corps 15 px, cible 44 px », raison en `:397-399`.

**Ce que la remontée demande.** Une seconde colonne de libellés — « scène » —
dans la table d'`ERGONOMIE.md`, avec `equirect_360` → « 360 ».

### Verdict : **tranché, source modifiée** — la substance est accordée, la forme est refusée

**La règle existe déjà, et elle a été écrite il y a un jour**, par la grappe
G12, sur la remontée de Georama, Relief et Vigie
(`GRAPPES-01-reponses.md:324-343`). Elle est à `ERGONOMIE.md:409-416` :

> Les abréviations `tb`, `sbs`, `ou` ne sont pas interdites — elles sont
> **bornées**. Jamais dans un fichier, une valeur ou une clé. Dans l'interface,
> seulement là où la forme lisible ne tient pas — une étiquette de vignette, une
> colonne étroite — et le libellé complet reste accessible à côté.

**Le critère est la place, pas la salle.** Un sélecteur de 44 px sur l'en-tête
d'un deck, à un mètre, dans le noir, *est* le cas « la forme lisible ne tient
pas » — au même titre que l'étiquette de vignette de Mutoscope, qui a produit
cette clause. La ligne scène n'a donc pas besoin d'une colonne : elle a besoin
que la clause cesse de nommer trois abréviations et nomme son critère.

**Pourquoi je refuse la seconde colonne, et ce n'est pas un détail de forme.**
Une table de libellés à deux colonnes est une table qui se périme deux fois. Un
concept ajouté demande deux traductions, un concept renommé en demande deux, et
la ligne scène finira par avoir des mots que l'atelier n'a pas — ce qui est
exactement la seconde direction artistique que `secteurs.html:648` interdit,
sous une autre forme. **Un libellé court n'est pas un autre mot : c'est le même
mot, tronqué par la place.**

**Sur `2D`, aucune souplesse, et le produit a raison de le dire lui-même.**
`00-vocabulaire.md:95` le bannit pour une raison qui vaut plus à la scène qu'à
l'atelier — sur un instrument qui joue *à la fois* de la plate et de la sphère,
l'ambiguïté est là où elle coûte. « Plate » fait cinq lettres ; la place ne
justifie rien.

**Ce que le produit doit faire.** Écrire « 360 » et « Plate » là où 44 px ne
tiennent pas plus, avec la forme longue accessible à côté — l'inspecteur de la
bibliothèque, qui l'est ; supprimer `2D` et « équirectangulaire » seul partout, y
compris dans la sortie console et dans le nom de caisse écrit sur disque.

---

## SCRATCHVJ-04 — L'échelle typographique de la scène

**Ce que disent les sources — trois fois, et pas pareil.**

- `secteurs.html:369`, **invariant de maison** (§ 5.1, « ne diverge jamais ») :
  « **L'échelle typographique.** Un jeu de tailles fermé, pas un continuum. »
- `tokens.json:17` : `"scale": [11, 12, 13, 16, 28]`.
  `DIRECTION-ARTISTIQUE.md:90` : « 11 / 12 / 13 / 16 / 28. **Rien d'autre, nulle
  part.** » `ERGONOMIE.md:445`, liste de contrôle : « Aucune taille de texte hors
  de 11 / 12 / 13 / 16 / 28. »
- `secteurs.html:457` : « Ce qui n'entre pas dans le cadran, et qu'il faut
  refuser : la famille de fontes, **l'échelle typographique**, le rayon des
  angles… »
- **Et** `secteurs.html:396`, variable 1, colonne scène : « corps **15 px** ».

15 n'est pas dans le jeu. La remontée a raison : deux lignes du même document ne
peuvent pas être vraies ensemble.

### Verdict : **tranché, source modifiée**

**La contradiction est déjà résolue ailleurs, et par une source — mais personne
n'a rapporté la résolution ici.** `design/TERRAIN.md:81-85`, écrit le
2026-09-09, déclare pour la troisième salle :

> L'échelle de la suite — 11 / 12 / 13 / 16 / 28 — est écrite pour un moniteur à
> 60 cm et une attention soutenue. Au terrain elle se **décale de deux crans**,
> en gardant les mêmes rapports pour rester le même système : **16 / 20 / 24 /
> 32 / 56.**

`TERRAIN.md:4-7` dit de lui-même : « **Ce document est une source.** » Un cadran
a donc déjà changé le jeu, personne ne l'a traité comme une infraction, et
`secteurs.html:457` interdit ce que `TERRAIN.md:85` fait. **Ce n'est pas une
question ouverte : c'est une règle appliquée deux fois et écrite une fois de
travers.**

**Ce qui est invariant, c'est la fermeture, pas la liste.** « Un jeu de tailles
fermé, pas un continuum » (`:369`) est la règle ; `[11, 12, 13, 16, 28]` en est
la valeur atelier. Le cadran ne peut pas changer le principe ; il change l'ancre,
et l'ancre est la distance de lecture, qui est la variable 1. C'est la seule
lecture qui rende les trois lignes vraies ensemble, et c'est celle que le
produit a trouvée seul.

**11,5 px disparaît sans décision de personne.** Une taille à virgule est un
continuum de un, et `:369` l'interdit quelle que soit la ligne. Ce point ne
demandait aucune réponse : il demandait seulement de savoir vers quel cran
arrondir.

**Ce qui reste ouvert, et qui prend sa ligne au registre.** Les cinq crans de la
scène. **Je ne les fixe pas, et pas par prudence — par une mesure.** La
remontée propose `[13, 15, 18, 24, 40]` et l'appuie sur « les mêmes rapports ».
Or *les mêmes rapports* n'ont jamais été appliqués : le jeu du terrain rapporté
à celui de l'atelier donne ×1,45, ×1,67, ×1,85, ×2,00, ×2,00 — cinq facteurs
différents. **La règle de dérivation que `TERRAIN.md:83` déclare suivre n'existe
pas**, donc la scène ne peut pas la suivre non plus, et deux propositions
concurrentes ne se départagent pas au goût.

**Ce qui manque pour fixer les cinq crans** : la règle de dérivation, écrite une
fois, et appliquée aux trois salles ensemble — faute de quoi la maison aura
trois échelles fermées et aucun système. **Qui doit le produire** : la maison, au
moment où `design/SCENE.md` s'écrit, en re-dérivant celle du terrain ou en
corrigeant la phrase qui la revendique.

**Ce que le produit doit faire.** Écrire son fichier unique de jetons avec le
corps à **15 px**, qui est une valeur de source (`secteurs.html:396`), et
**arrondir 11,5 au cran voisin de son propre jeu**, en notant le substitut
comme `maison/01-LES-TROIS-COUCHES.md:104-106` l'exige. Ne pas figer les cinq
crans dans `tokens.json` : les écrire sous `lines.scene.type.scale` avec la
mention « candidat, un seul produit ».

---

## SCRATCHVJ-05 — La densité en trois étages : la variable 7 n'a aucune valeur

**Ce que dit la source.** `secteurs.html:443-448`, variable 7 : atelier
« maximale et uniforme, type Nuke », scène « hiérarchisée en trois niveaux, dont
un lisible à un mètre », raison en `:446-448`. La colonne atelier renvoie à des
nombres (`tokens.json:20-24` : unité 4, gouttière 16, écart de groupe 20, ligne
de paramètre 28, barre haute 32, cible minimale 20). **La colonne scène est la
seule des sept qui ne porte aucun nombre.**

### Verdict : **reporté**

La question est bonne, et elle est la meilleure du relevé : *qu'est-ce qu'un
étage — une taille de texte, une densité d'espacement, une hauteur de contrôle,
ou les trois ?* Sans réponse, « hiérarchisée en trois niveaux » n'est pas une
règle, c'est une intention : rien ne peut la vérifier, et
`03-DEFINITION-DE-FINI.md:6-7` exige qu'un critère se **montre**.

**Ce qui manque pour trancher** : les valeurs, et un **second produit de scène**
pour les éprouver. La remontée le dit elle-même et elle a raison de le dire :
trois triplets tirés d'un seul produit seraient une réponse particulière, et
`04-CE-QUI-EST-OUVERT.md:66-70` rappelle qu'une réponse particulière est une
dette que le produit suivant paie.

**Et il y a une difficulté de plus, que la remontée ne pouvait pas voir.**
`secteurs.html:258`, `:274-275` et `:667` affirment trois fois que le second
produit de scène **existe**. Il n'est nulle part dans `maison/05-QUI-EST-QUI.md`,
qui fait foi pour la place de chaque produit et qui en connaît dix — dont **un
seul** de scène. Tant que cette phrase n'est pas ou bien vérifiée ou bien
retirée, « attendre le second produit de scène » est une attente sans date.
C'est le premier point à régler, et il ne coûte qu'une ligne.

**Qui doit le produire** : la maison, dans `design/SCENE.md`, au tour 01 du
second produit de scène — après avoir nommé celui-ci dans `05-QUI-EST-QUI.md`
ou retiré la phrase.

**Ce que le produit doit faire.** Laisser la ligne en *remonté*, garder ses trois
étages tels qu'ils sont, et **les nommer dans son fichier de jetons** — quel
panneau est à quel étage — pour que la réponse, quand elle viendra, se pose sur
un découpage déjà écrit au lieu d'en trouver un implicite. Ne pas inventer les
valeurs, et ne pas régulariser les cinq paires d'espacement hors unité de 4
avant : `10`, `6` et `18` sont peut-être ce qu'un étage veut dire.

---

## SCRATCHVJ-06 — La luminance commune des deux châssis, et la mono unique

**Ce que disent les sources, et elles se contredisent.**

`secteurs.html:410-416`, variable 3 : châssis d'atelier « noir verdi froid
`#0A0C0B` », de scène « noir chaud `#141412` », **contrainte** : « même
luminance, teinte seule différente — deux finitions d'un même matériau, pas deux
marques ».

`secteurs.html:474-476`, § 5.3 : « **Mi-accident** : la teinte est du cadran
(variable 3), l'écart de *valeur* n'en est pas. **Aligner la valeur, garder la
teinte.** » Et `:663` : les deux accidents « se corrigent **aujourd'hui**, en un
fichier chacun ».

`MAISON.md:49-50` du produit range les deux en couche 3 et ordonne « ne rien
changer ». **Le produit a suivi `MAISON.md`, et il a eu raison** : entre une
source qui dit « corrige aujourd'hui » et la loi de la maison qui dit
« attends », c'est la loi qui gouverne, et la contradiction se remonte
(`maison/00-LIRE-DABORD.md:32-34`).

### Verdict : **reporté**

**Sur la luminance (Q1) — la source a déjà tranché, et il ne manque qu'un
calcul.** « Aligner la valeur, garder la teinte » (`:475-476`) est une décision,
pas une question. Le produit apporte la mesure qui manquait, et elle est
accablante :

| Valeur | Luminance relative WCAG | |
|---|---|---|
| atelier `#0A0C0B` | 0,0035 | — |
| scène `#141412` | 0,0069 | **× 2,0** |

La contrainte « même luminance » n'est tenue par aucune des deux valeurs. La
direction de l'alignement est écrite ailleurs et n'est pas une préférence :
`DIRECTION-ARTISTIQUE.md:73` fait de `--void` le fond « derrière tout rendu 3D ou
vidéo », et un fond plus clair réduit le contraste avec l'image projetée — ce
qu'on ne veut nulle part et moins encore dans une salle noire. **Donc :
luminance de l'atelier, teinte de la scène.** La contrainte s'écrit comme un
test au lieu d'une phrase — *luminance relative à ±5 % de `void`, dérive chaude
R > G > B conservée* — et la valeur en sort par le calcul, pas par l'œil. Le
`#0D0C0A` que la remontée propose mesure 0,0037, soit +5 % : il passe.

**Ce n'est pas cela qui reporte la remontée. C'est la mono.**

**Sur la mono (Q2) — le test n'a jamais été passé sur les deux bons candidats.**
La grappe G6 a fermé la famille de chasse fixe pour la ligne atelier
(`GRAPPES-01-reponses.md:155-183`) et l'a laissée ouverte pour la seule scène,
« arbitrée sur le passage au moteur d'impression et non sur le goût ». Or le
critère écrit en `secteurs.html:470-473` repose sur deux jambes, et **G6 en a
cassé une** :

> « Fragment Mono n'est présente nulle part sur le disque » — `secteurs.html:471-472`

C'est faux depuis le 2026-09-09 : `DimGS/apps/ui/src/fonts/fragment-mono-latin.woff2`
et son `OFL-FragmentMono.txt` sont embarqués dans Georama depuis le début.
`secteurs.html` est corrigé sur ce point (voir la section finale), et **la
disponibilité cesse de départager quoi que ce soit**.

Reste l'autre jambe, le moteur d'impression. **Et la mesure que le produit
rapporte ne porte pas sur elle** : il a mesuré qu'**Archivo variable** ne passe
pas le moteur d'impression de Chrome, et que DM Mono passe. C'est une mesure
juste, et elle ne dit rien de Fragment Mono, qui n'est pas une fonte variable et
que rien ne condamne par le même mécanisme.

**Ce qui manque pour trancher** : rendre le même document dans le moteur
d'impression de Chrome avec `fragment-mono-latin.woff2` d'un côté et DM Mono de
l'autre, et regarder. **Une heure.** **Qui doit le produire** : ce produit, et
c'est le seul qui puisse — il tient la seule chaîne du catalogue qui fabrique
réellement des PDF, et c'est ce qui rend sa mesure recevable là où un avis ne le
serait pas.

**Ce que le produit doit faire.** Ne rien changer, comme `MAISON.md:49-50` le
dit — et, en phase 5, isoler la famille de chasse fixe et la valeur du fond
**en une ligne chacune**, comme `04-CE-QUI-EST-OUVERT.md:49-52` l'exige d'une
couche 3 : « on s'arrange pour que la trancher plus tard coûte une ligne ». La
phase 5 n'est donc pas bloquée, et `MAISON.md:91` l'avait déjà prévu ainsi.

---

## SCRATCHVJ-07 — `lines.scene` n'existe pas : ni accent, ni paire A/B, ni classe documentaire

**Ce que dit la source.** `secteurs.html:492-493` : « Et `tokens.json` v3, qui
gagne un niveau au-dessus de `chassis` : `lines: { atelier, scene }`. »
`:418-425`, variable 4 : la scène porte « une teinte produit **plus une paire
opératoire A/B** (ambre / ardoise), constante sur toute la ligne », et cette
paire « doit être **identique dans tous les produits de scène**, pour qu'un
réflexe acquis sur l'un serve sur l'autre ».

**Ce qui existe.** `tokens.json` est en `"version": 2`. Il n'y a pas de niveau
`lines`. Ses huit accents sont ceux des huit produits d'atelier.
`suite.css:53-61` porte huit classes produit et aucune de scène.

### Verdict : **tranché, source modifiée**

**`tokens.json` passe en v3 et gagne `lines`.** C'est le trou le plus mécanique
des vingt, la source dit déjà quoi faire, et le moment est le bon : au
2026-09-09 **aucun produit n'a encore de fichier de jetons**
(`maison/01-LES-TROIS-COUCHES.md:110-112`), donc le changement coûte zéro
transcription. Il en coûtera huit dans un mois.

**Q1 — la paire A/B entre, l'accent produit non**, et la remontée a raison de
distinguer les deux. Ce ne sont pas deux teintes de plus : ce sont deux objets
différents.

- **La paire ambre / ardoise est de l'état, pas de l'identité** —
  `secteurs.html:422-425` l'écrit. Elle ne rejoint donc pas la palette d'accents
  qui attend un écran calibré (`REGISTRE.md:201`) : elle rejoint la famille des
  couleurs de signalisation, qui n'a jamais été de la couche 3. Et elle doit
  être écrite **une fois** pour que « constante sur toute la ligne » veuille dire
  quelque chose — deux produits de scène qui la choisissent chacun l'annulent.
  Elle entre aux valeurs que le seul produit de scène emploie, `#C99A2F` et
  `#6E8696`, **parce que ce sont des mesures, pas des propositions** : elles sont
  en service, jouées dans le noir, et aucune autre n'a jamais été essayée.
- **L'accent produit `#C9762F` reste dehors.** C'est de l'identité, il rejoint
  la palette non vue sur écran calibré, et il est attaché à un nom de produit qui
  n'existe pas (`SCRATCHVJ-19`). Le figer aujourd'hui, c'est le figer deux fois.

**`lines.scene` ne se limite pas à ces deux valeurs, et c'est le point que la
remontée sous-estime.** Le relevé montre que **sept** couleurs de châssis sont
déjà réchauffées, pas une (`ALIGNEMENT.md:587-593`) : fond, panneau, champ,
filet, craie, craie atténuée, craie éteinte. **La scène n'a pas de jeton de
filet** — `chassis.line` `#2A302C` est verdi, et le produit emploie `#2E2D28`.
Un cadran qui déclare « noir chaud » sans donner les sept valeurs laisse chaque
produit réchauffer les six autres à sa main. `lines.scene.chassis` porte les
sept, ou il n'en porte aucune.

**Et une divergence de plus, que personne n'a remontée et qui n'est pas une
couleur** : à l'atelier le champ de saisie est **relevé** (`chassis.raised`
`#1A1F1C`, plus clair que le panneau) ; à la scène il est **creusé** (`#0E0E0C`,
plus sombre). Ce n'est pas une teinte, c'est une convention de relief, et aucune
variable du cadran ne la couvre. Elle se tranche en écrivant `lines.scene`, ou
elle divergera en silence.

**Q2 — pas de classe `.p-` dans `suite.css` avant le nom.** Tranché, et pour la
raison que la remontée donne : une classe posée sous un nom de travail dans une
feuille que huit produits partagent traverserait chaque document déjà publié le
jour du renommage. Voir `SCRATCHVJ-19`.

**Q3 — le second orange documentaire `#b45f1c`** n'est pas une question :
`DIRECTION-ARTISTIQUE.md:183-186` interdit qu'un produit porte deux teintes, et
il disparaît en phase 7 avec la feuille locale.

**Ce que le produit doit faire.** Écrire son fichier de jetons avec en tête
`« ligne scène, tokens.json v3, lines.scene »`, référencer la paire A/B par nom
de jeton, garder son accent produit derrière un nom unique prêt à changer, et
supprimer le second orange documentaire.

---

## SCRATCHVJ-08 — L'ambre porte deux rôles

**Ce que disent les sources.** `tokens.json:32` : `signal.running` = `#C9A227`,
rôle « travail en cours ». `secteurs.html:377-379`, **invariant** : « Les quatre
couleurs de signalisation et leur sens » ne divergent jamais. `:420` fait de
l'ambre la moitié de la paire opératoire de scène.

**Ce que fait le code.** Une seule constante `#C99A2F` porte les deux. Trois
unités d'écart sur le vert, huit sur le bleu : à un mètre dans le noir, c'est la
même couleur, et un point ambre ne veut plus rien dire.

### Verdict : **tranché** — la règle existe, et elle ne dit pas ce qu'on croit

**L'invariant porte sur la teinte et son sens, jamais sur sa surface.**
`DIRECTION-ARTISTIQUE.md:106-110` : quatre teintes « qui veulent dire la même
chose partout — l'état d'un travail. Mêmes règles que les accents : jamais un
aplat, jamais un fond de bouton. » Rien n'oblige un produit à peindre un point.

**Trois lignes déjà écrites tranchent ensemble, et aucune n'est nouvelle :**

1. **`ERGONOMIE.md:328-331`** exige d'une progression trois choses — « la
   fraction faite, l'étape courante **nommée**, et le temps restant estimé » — et
   aucune n'est une couleur. Un travail en cours qui affiche une progression
   *est* signalé ; le point est redondant.
2. **`DIRECTION-ARTISTIQUE.md:145-148`**, tenu par la maison depuis le début :
   « un point d'état ne doit jamais être le **seul** porteur de l'information ».
   Retirer le point ne retire donc rien qui portait quelque chose.
3. **`ERGONOMIE.md:306-341`, « Le travail long », est une section de niveaux A
   et B** — et `maison/00-LIRE-DABORD.md:87` a déjà écrit que ces sections
   « ne s'appliquent que là où le produit a réellement un viseur ou un travail
   long ». `secteurs.html:191-193` dit que le travail de scène est « immédiat,
   irréversible, et ne produit aucun fichier ». Le seul travail long de ce
   produit — l'analyse d'un clip — se fait **avant** le set, dans un écran de
   préparation qui n'est pas l'instrument.

**Donc : à la scène, l'ambre appartient au deck A ; `running` garde sa valeur et
son sens dans `tokens.json`, et se montre par la progression, pas par un
point.** La paire, elle, ne peut pas bouger : `secteurs.html:422-425` la déclare
acquise comme réflexe sur toute la ligne, et un réflexe qui change d'un produit
à l'autre n'en est pas un.

**Je ne retiens pas la solution de repli** — écarter les deux teintes de façon
mesurable. Elle demande de juger une différence perceptive sur un écran non
calibré, ce qui est le point ouvert nommément (`REGISTRE.md:201`), pour résoudre
un problème qui se résout sans juger quoi que ce soit.

**Ce que le produit doit faire.** Garder `#C99A2F` pour le deck A, retirer le
point ambre des cinq états « en cours », et laisser la progression nommée porter
l'information — ce que `ERGONOMIE.md:328-331` demandait déjà.

---

## SCRATCHVJ-09 — Les quatre signaux sur un châssis chaud, et le `warn` manquant

**Ce que dit la source.** `secteurs.html:377-379` et `:458` rangent les quatre
couleurs de signalisation dans l'**invariant** : elles n'entrent pas dans le
cadran. `tokens.json:30-36` donne les quatre valeurs et leurs sens.

**Ce que fait le code.** Trois des quatre existent, toutes retouchées vers le
chaud — `done` `#7E946B` contre `#7FB069`, `fail` `#B54B3A` contre `#D9584B` —
et **`warn` n'existe pas**. Le produit n'a que trois états à montrer, et il peint
en ambre un lien de platine qui faiblit : c'est-à-dire avec la couleur de
« travail en cours », alors que c'est mot pour mot le sens de `warn` — « dérive
détectée » (`tokens.json:34`).

### Verdict : **tranché, source modifiée**

**Q1 — mêmes teintes, un jeu de valeurs par support, la règle écrite une fois.**
Le précédent existe et la maison l'a déjà accepté : `suite.css:38-40` porte les
quatre signaux **assombris** pour le fond clair, avec la raison écrite dans la
feuille — « mêmes rôles que dans l'application, assombris pour le fond clair afin
de garder un contraste lisible ». Personne n'a appelé ça une divergence, parce
que ce n'en est pas une : c'est la même teinte au même sens, portée à un
contraste tenable sur un autre fond.

Ce précédent n'est écrit nulle part ailleurs que dans un commentaire de CSS, et
c'est pour ça que ce produit a cru devoir demander. **Il monte dans
`DIRECTION-ARTISTIQUE.md`, sous les couleurs de signalisation :**

> **Les quatre teintes et leurs sens sont invariants ; leurs valeurs s'ajustent
> par support.** Un jeu par support — l'application sombre, le papier clair, le
> papier sombre — écrit une fois dans `tokens.json`, avec la raison. Jamais
> quatre valeurs libres par produit : un `fail` qu'on ajuste dans un dépôt est un
> `fail` qui ne veut plus dire la même chose que celui d'à côté.

Les trois valeurs chaudes du produit rentrent donc dans `lines.scene.signal`, et
elles cessent d'être un écart.

**Q2 — `warn` entre, et il entre avec son emploi.** Ce n'est pas une
fonctionnalité nouvelle : le produit **détecte déjà** un lien de platine qui
faiblit et le peint. Lui donner la bonne couleur est une correction d'affichage,
donc un retrait au sens de `maison/00-LIRE-DABORD.md:157-161`, donc dans le
périmètre.

Et c'est le signal dont un instrument joué devant un public a le plus besoin.
« Ça marche encore, mais ça dérive » est précisément ce qu'on veut voir **avant**
que le lien lâche, et un produit qui n'a que lié / en cours / perdu ne peut pas
le dire. La ligne scène hérite des quatre, pas de trois.

**Q3 — l'accessibilité.** Aucune décision demandée, et le produit tient déjà la
règle : chaque voyant est suivi d'un mot. Noté pour que la ligne scène hérite de
la **règle** (`DIRECTION-ARTISTIQUE.md:145-148`) et non de sa seule conclusion.

**Ce que le produit doit faire.** Ajouter `warn` à sa table nommée, le donner au
lien de platine qui faiblit, et déclarer ses quatre valeurs chaudes comme le jeu
de support de la scène — pas comme des valeurs à lui.

---

## SCRATCHVJ-10 — La couleur qui porte du texte

**Ce que dit la source.** `secteurs.html:377-379`, **invariant** : un `fail`
rouge « reste la seule couleur autorisée à porter du texte ».
`DIRECTION-ARTISTIQUE.md:117` écrit la même règle.

**Ce que fait le code.** Du texte coloré à **24 endroits** ; deux conformes, les
deux bandes d'erreur. Parmi les vingt-deux autres, quatre sont d'une autre
nature : les lettres **« A »** et **« B »**, en ambre et en ardoise, qui
identifient les decks au-dessus des faders et sur la bande programme. C'est la
variable 4 appliquée de la manière la plus économique possible.

### Verdict : **tranché, source modifiée**

**Le cadran crée une couleur et ne dit pas quelle surface elle occupe.** C'est un
silence, pas une autorisation : la variable 2 prend la peine d'autoriser un aplat
(`:403-404`), ce qui prouve que la question des surfaces se traite quand elle se
pose. Elle ne s'est pas posée pour la variable 4, et elle se pose ici.

**La clause à écrire porte sur ce que le texte est, pas sur la couleur qui le
porte** — et c'est ce qui la rend générale au lieu d'être une exception de
scène :

> **Une étiquette d'un ou deux caractères qui nomme une source n'est pas du
> texte au sens de cette règle.** Une phrase l'est toujours. La règle protège le
> fait qu'un texte coloré se lise comme une alarme ; un « A » et un « B » ne
> sont pas des phrases, personne ne lit une étiquette d'une lettre comme un
> message d'erreur.

Trois raisons de la retenir, et la troisième est celle qui décide.

1. **Une lettre colorée est la plus petite surface possible.**
   `DIRECTION-ARTISTIQUE.md:95-97` autorise déjà la couleur sur « des surfaces de
   2 px maximum : filet de rail, point d'état, capuchon ». Un « A » ambre à 15 px
   ne porte pas plus de matière colorée qu'un capuchon de 2 px sur 32.
2. **L'alternative est pire.** Sans texte coloré, dire « ceci est le deck A »
   demande une pastille **plus** un « A » en craie : deux éléments, plus de
   surface colorée, et un mot de plus à lire à un mètre. La règle produirait le
   contraire de ce qu'elle protège.
3. **Elle sert immédiatement ailleurs.** `GRAPPES-01-reponses.md:340-342`, sur
   `MUTOSCOPE-16`, vient de constater que « Œil sur œil » ne tient pas dans une
   pastille de vignette. Une pastille de vignette est exactement une étiquette
   d'un ou deux caractères qui nomme une source. **La clause n'est pas une clause
   de scène : c'est une clause de la maison, que la scène a fait apparaître.**

**Pour les dix-huit autres, non, et sans regret.** « Analyse en cours »,
« plateau réel : aucune entrée audio », « pas encore à l'image » sont des
phrases. Une phrase ambre à côté d'une phrase rouge est exactement ce que
l'invariant empêche. C'est du travail de produit.

**Sur le nom du produit en orange dans la barre haute — le capuchon reste.**
`ERGONOMIE.md:104-106` décrit à cet endroit « le capuchon d'accent du produit —
un filet plein de 2 px […] puis le nom du produit » en craie, et
`DIRECTION-ARTISTIQUE.md:121-123` en fait le seul emploi vraiment utile des
accents. La variable 4 **ajoute** la paire, elle ne remplace pas le capuchon.
Le produit a fusionné les deux : il rend le nom à la craie et reprend le filet.
C'est un retrait de texte coloré plus deux pixels de filet, sur une barre qu'il
a déjà — donc dans le périmètre, et pas un ajout au sens de G4.

**Ce que le produit doit faire.** Garder « A » et « B » colorés, rendre les
dix-huit phrases à la craie, et remplacer le nom orange par capuchon 2 px + nom
en craie.

---

## SCRATCHVJ-11 — L'aplat : deux règles que `MAISON.md` croit identiques

**Ce que disent les sources.** `secteurs.html:403-404`, variable 2, colonne
scène : « un aplat autorisé — un seul par panneau, **toujours porté par un
état**, jamais par l'identité », démontré en `:451-454` — « Boucle · **Armé** ·
Slip », l'aplat sur *Armé*. `MAISON.md:41` du produit affirme que c'est « déjà,
**mot pour mot**, la règle de `design/README.md` », lequel écrit : « l'orange
plein est réservé à une seule **action** par panneau ».

Ce n'est pas mot pour mot, et ce n'est pas la même règle. *Un état* et *une
action* sont deux critères différents : l'un porte sur ce que le contrôle
**montre**, l'autre sur ce qu'il **fait**. Le code applique les deux, ce qui est
la conséquence exacte de l'ambiguïté.

### Verdict : **tranché, source modifiée**

**Q1 — l'état, sans hésitation, et pour deux raisons de rang différent.**

(a) C'est ce que la source dit. `design/README.md` est un document de **produit**
et ne peut pas amender un cadran : `maison/00-LIRE-DABORD.md:26-34` place la
source au-dessus de ce qui en dérive, et un README de dépôt n'est même pas dans
la chaîne.

(b) La raison écrite en `:405-408` tranche à elle seule : « à la scène il faut
*voir avant de lire*. L'aplat est le seul signal préattentif dont on dispose. »
**Un signal préattentif répond à « où en est-on ? », pas à « que puis-je
faire ? »** — la seconde question, on se la pose en lisant. Un aplat sur un
bouton d'action au repos consomme le seul canal préattentif de l'écran pour une
information qui n'a pas changé depuis dix minutes.

**Et le générateur est corrigé, pas le produit.**
`maison/02-INTERDITS.md:62-65` : un fichier généré qui est faux se répare dans
le générateur, sinon la prochaine génération écrase la correction. La ligne
fautive est dans `tools/gen-maison.py`.

**Q2 — ce qu'est un panneau, et c'est la moitié qui rendait la règle
inapplicable.** Le panneau d'un deck fait 490 lignes et porte jusqu'à sept
surfaces remplies : est-ce un panneau, ou cinq ? Sans réponse, « un seul par
panneau » ne se vérifie pas, et `03-DEFINITION-DE-FINI.md:6-7` exige qu'un
critère se montre.

> **Un panneau est la plus petite région délimitée par un filet ou par un fond
> propre** — pas une région définie par sa fonction. Le panneau est ce que l'œil
> découpe, et l'œil découpe des bordures.

Je retiens la définition que la remontée propose, et pour un motif qui n'est pas
son élégance : **elle rend la règle vraie sans rien changer au code.** Sur ce
critère, chacun des cinq groupes du deck est un panneau, chacun porte un
sélecteur, et le compte est tenu. Une définition qui décrit ce qu'un produit fait
déjà par nécessité est solide ; une qui le contredit demanderait à être défendue,
et personne ici n'aurait pu la défendre.

**Q3 — un sélecteur segmenté compte pour un aplat, un seul.** Un sélecteur
exclusif n'affiche qu'un segment rempli ; le compte porte sur les surfaces
**visibles simultanément**, pas sur les contrôles. Tranché.

**Ce que le produit doit faire.** Retirer l'aplat des quatre boutons d'action —
« ■ relecture », « Suivant → », « Créer », « + Liaison » — et le garder partout
où il porte un état ; vérifier le compte panneau par panneau au sens de Q2.

---

## SCRATCHVJ-13 — La capitale initiale sur un libellé de paramètre, à un mètre

**Ce que disent les sources, et elles ne se contredisent pas.**
`ERGONOMIE.md:395-404` : « Capitale initiale seulement. Pas de capitales à tous
les mots, pas de point final. **La règle porte sur les libellés que le produit
écrit lui-même** » — clause ajoutée par `DIORAMA-01`, et qui vise ce produit de
plein fouet puisqu'il n'a pas d'hôte.
`DIRECTION-ARTISTIQUE.md:219-220` prescrit au contraire les capitales pour une
catégorie précise : « titre de panneau à 125 condensé en capitales ».

Les deux règles portent sur deux objets. **Ce qui manque, c'est la frontière.**

**Ce que fait le code.** 39 sur-titres de panneau et 6 onglets en capitales —
couverts par `DIRECTION-ARTISTIQUE.md:219-220`, **conformes**. 40 boutons en
capitale initiale, conformes. Et **25 libellés de paramètre qui ne sont conformes
dans aucun sens** : vingt en capitales, cinq tout en minuscules — un `TANGAGE`
dans le résumé et un `tangage` dans le popup qui le règle.

### Verdict : **tranché, source modifiée**

**Capitale initiale. Aucune exception de scène.** Et c'est la remontée elle-même
qui donne les deux arguments décisifs, contre son propre confort :

1. **La capitale intégrale ne se lit pas mieux de loin, elle se lit moins bien.**
   Un mot en capitales perd sa silhouette — hampes et jambages — qui est
   précisément ce que l'œil reconnaît à distance sans lire lettre à lettre.
   L'argument qui rendrait la variable défendable joue à l'envers.
2. **La raison métier ne s'écrit pas**, et `secteurs.html:386-389` en fait le
   test du cadran : « si la raison ne peut pas s'écrire, la variable retourne à
   l'invariant ». Le seul argument pour les capitales à la scène est « ça fait
   plus VJ », et `:459` nomme exactement celui-là comme la demande à refuser.

**C'est le passage le plus important de tout ce relevé, et il faut le dire.** Un
produit a appliqué le test de son propre cadran contre son propre code, a trouvé
qu'il ne passait pas, et l'a écrit. C'est ce que le cadran existe pour permettre,
et c'est ce qu'aucune liste d'interdits ne produit.

**Ce que la maison écrit, c'est la frontière, pas une exception.** Le produit ne
s'est pas trompé de règle : il s'est trompé de fonction, en employant le
sur-titre comme libellé de ligne. La formulation existe déjà à moitié en
`ERGONOMIE.md:135-139` — le libellé est à gauche, la valeur à droite — et elle
devient un critère :

> **Un libellé est ce qui a une valeur à sa droite. Un titre de panneau est ce
> qui a des libellés en dessous.** Le premier porte la capitale initiale, le
> second peut porter les capitales à l'axe de largeur 125. Un texte qui n'est ni
> l'un ni l'autre est un libellé.

Elle se vérifie mécaniquement, dans les huit produits comme ici.

**Ce que le produit doit faire.** Réécrire les 25 en capitale initiale, en
phase 2. Aucun fichier écrit sur disque n'est touché.

---

## SCRATCHVJ-14 — Le format d'un temps et d'un angle sur un instrument scratché

**Ce que disent les sources.** `ERGONOMIE.md:162-172`, « précision d'affichage,
**invariable** » : angle à une décimale avec signe explicite, timecode
`hh:mm:ss:ff`, durée d'un travail en `h min s`.
`maison/00-LIRE-DABORD.md:95-97` range « la précision d'affichage » dans ce qui
« s'applique **en entier** » à la scène, sans réserve.

**Ce que fait le code.** Position dans un clip en `mm:ss.d`, durée courte en
`mm:ss`, angles à zéro décimale.

### Verdict : **reporté**

**Le désaccord n'est pas une négligence, et c'est ce qui en fait une question.**
`hh:mm:ss:ff` suppose deux choses qu'un instrument de scratch n'a pas. Des
**heures** : un clip de VJ dure trente secondes à quelques minutes, et quatre
caractères sur onze afficheraient toujours `00:`. Et une **cadence d'images
stable** : le champ `ff` compte des images par seconde, or ce logiciel est bâti
sur le principe inverse — son manifeste dit que « tout ce qui se scratche est une
*fonction de la position*, jamais un intégrateur ». Pendant un scratch, la
position est continue et l'image est un échantillon ; `ff` y afficherait un
nombre qui saute.

**La raison métier s'écrit donc, et elle passe le test de
`secteurs.html:386-389`.** Ce n'est pas ce qui reporte la remontée.

**Ce qui la reporte, c'est qu'elle dépend d'une décision qui n'est pas prise et
qui n'appartient pas à la maison.** `REGISTRE.md:210` inscrit déjà comme ouvert
« le repère temporel de la ligne scène — qui fait autorité : le tempo, l'horloge
audio, ou la position du plateau », attribué **au fondateur, sur recommandation
de scratchvj**, et `MAISON.md:87` en fait la phase 3 de ce produit — « la
décision à plus fort levier de la ligne ».

Si le tempo fait autorité, l'unité naturelle n'est pas la seconde mais le temps
musical, et la ligne du tableau devra porter les deux formes, pas une. **Trancher
`mm:ss.d` aujourd'hui, c'est trancher le repère temporel par la bande, en
écrivant un format d'affichage.** C'est exactement ce que
`04-CE-QUI-EST-OUVERT.md:44-48` interdit — « ne pas la trancher, même quand la
réponse paraît évidente ; surtout alors ».

**Ce qui manque pour trancher** : le repère temporel de la ligne scène.
**Qui doit le produire** : le fondateur, sur la recommandation que la phase 3 de
ce produit doit écrire — options, conséquences, recommandation, pas décision.

**Sur l'angle, aucune exception, et ce n'est pas reporté.** Une décimale partout,
signe explicite : `ERGONOMIE.md:166` est explicite et le coût est de deux pixels.
`DIRECTION-ARTISTIQUE.md:237-238` en fait même un des « détails que seul ce
métier connaît » dont ce produit porte déjà d'autres. C'est du travail de
produit, et il se fait sans réponse.

**Ce que le produit doit faire.** Passer les angles à une décimale ; garder
`mm:ss.d` et laisser la ligne en *remonté* jusqu'à la phase 3 ; y traiter le
format d'affichage comme une conséquence du repère temporel et non comme un
sujet à part.

---

## SCRATCHVJ-15 — Le clavier de la ligne scène, et `Échap` qui ferme la fenêtre

### Verdict : **tranché, source modifiée**

Trois choses dans une remontée. La première n'est pas une question.

### 1. `Échap` — c'est un manquement, il est franc, et il n'a jamais été négociable

`maison/02-INTERDITS.md:35`, sous « Le clavier » :

> **`Échap` ne ferme jamais la fenêtre.** Il suit la pile de priorité.

`03-DEFINITION-DE-FINI.md:35` le répète. `ERGONOMIE.md:289-290` l'écrit avec la
pile. **Ce document-là fait foi et n'a pas de source en dessous**
(`02-INTERDITS.md:3`). Il n'y a pas de variable de cadran ici, il n'y a pas de
lecture de scène, et il n'y a rien à arbitrer.

Le code fait l'inverse en trois rangs : le premier `Échap` quitte « image
seule », le deuxième ferme la fenêtre de sortie, **le troisième quitte
l'application**. Le commentaire du code écrit son propre argument — *« One escape
gets the picture off the projector; a second ends the set. Quitting straight to a
desktop in front of a room is the thing this ordering exists to prevent. »* —
**et l'argument condamne le troisième rang** : au troisième appui, on est
précisément sur un bureau devant une salle. La règle de la maison et l'intention
du code disent la même chose ; seul le code ne la tient pas.

**C'est un retrait, donc dans le périmètre**, au sens exact de
`maison/00-LIRE-DABORD.md:157-161` : retirer se fait, ajouter demande une phase.
Retirer un rang est un retrait. **Il se fait en phase 4, sans réponse, et il ne
se discute pas.**

**Le second défaut est du même ordre.** La garde ne teste que la saisie de
texte : un popup ouvert sans champ actif laisse passer les deux traitements — la
bibliothèque ferme le popup **et** le gestionnaire avance d'un rang. Un `Échap`
qui produit deux effets n'est pas une pile de priorité, c'est deux piles. La
correction est que l'événement se consomme au premier rang qui s'applique.

**Et une lecture qui manquait à la pile elle-même, et qui vaut pour les dix
produits.** La pile de `ERGONOMIE.md:284-290` a six rangs, et ce produit n'en a
que deux qui ont un objet — il n'a ni outil armé, ni sélection. Elle se lit comme
la table du clavier, dont `DIORAMA-06` a déjà donné la lecture
(`ERGONOMIE.md:238-243`) :

> **La pile dit ce qu'un rang fait *si* le produit a la chose qu'il manipule, pas
> qu'il doit l'avoir.** Un rang sans objet est sans objet, pas manqué. **Le rang 6
> n'est jamais facultatif** : c'est lui la règle — ne rien faire, et ne jamais
> fermer la fenêtre.

### 2. `Ctrl`+`Z` est-il exigible d'un produit de scène ?

**Non, et la clause existe déjà — il faut seulement la lire jusqu'au bout.**
`ERGONOMIE.md:238-243` se termine par : « **La règle ne s'applique qu'aux touches
que le produit lie réellement.** » `Ctrl`+`Z` n'est pas liée, la règle ne s'y
applique pas, et le noyau n'a pas été enfreint.

La remontée a cru le contraire parce que la clause **s'annonce** par son cas
particulier — « un produit qui ne lie **aucune** touche » — et que ce produit en
lie quatre. C'est le même mode de panne que G12 et G15 : un document qui énonce
sa règle par un exemple, et qu'on lit sur l'exemple. La clause est donc réécrite
dans l'ordre inverse, le critère d'abord :

> **Une touche réservée n'est exigible que d'un produit qui a la chose qu'elle
> manipule.** Un produit sans état persistant n'a pas d'annulation à offrir ; un
> produit sans comparaison avant/après ne lie pas `C`. La table dit ce qu'une
> touche signifie *si* le produit en lie une, jamais combien il doit en lier — et
> le cas limite est celui d'un produit qui n'en lie aucune.

Le raisonnement du produit est juste et il faut le garder écrit :
`spec/04-frontieres.md:10-23` pose le test « que reste-t-il quand on ferme la
fenêtre », et `secteurs.html:191-193` y répond pour la scène — rien. **Un produit
qui ne détient rien n'a rien à annuler.** Sans cette clause, la table du clavier
redevient ce que `DIORAMA-06` lui reprochait : une demande de fonctionnalité
déguisée.

`Tab` est déjà réglé par `MAISON.md:89` du produit — « `F` s'en charge ici, comme
`Tab` à l'atelier » — et `F` est l'une des quatre lettres que la suite s'interdit
de réserver (`ERGONOMIE.md:265-270`), donc libre. `?` / `F1` n'a pas d'excuse :
travail de produit, phase 4.

### 3. La ligne scène gagne-t-elle son propre clavier réservé ?

`secteurs.html:483-486` le pose sans le trancher. **La réponse est oui sur le
principe et pas maintenant sur la table**, et je retiens la recommandation
opératoire de la remontée : `Espace`, `F` et `Échap` sont **notées comme
candidates** dans le cadran, et la table se fige au tour 01 du second produit de
scène. Trois touches ne font pas un clavier de ligne, et écrire une table à
partir d'un seul produit est exactement ce que `DIVERGENCES.md` a coûté à la
suite.

**Avec la même réserve qu'à `SCRATCHVJ-05`** : le second produit de scène est
affirmé trois fois dans `secteurs.html` et absent de `maison/05-QUI-EST-QUI.md`.
Cette ligne se règle d'abord.

### 4. Deux corrections factuelles — la preuve tombe, la conclusion tient

`MAISON.md:52` du produit et `secteurs.html:483` décrivent tous deux « les pads
sur `1`–`5` » et un conflit potentiel avec `Ctrl`+chiffre. **Cette interface
n'existe plus** : recherche exhaustive des touches `0` à `9`, pavé numérique
compris, aucune occurrence. Les deux sources sont corrigées.

Conséquence pour la maison : `secteurs.html:486` cite ce conflit comme la preuve
que « ça arrive tout seul ». **La preuve tombe ; la conclusion reste vraie par
Mutoscope seul** (`MUTOSCOPE-01`, `REGISTRE.md:209`). Une conclusion qui survit à
la perte d'une de ses deux preuves est une conclusion qu'on garde en changeant sa
citation, pas en la défendant.

**Ce que le produit doit faire.** Supprimer le rang qui quitte l'application,
consommer l'événement au premier rang applicable, et ne pas lier `Ctrl`+`Z`.

---

## SCRATCHVJ-17 — Le papier sombre : ce que `suite.css` doit porter

**Ce que dit la source.** `secteurs.html:435-441`, variable 6 : papier d'atelier
« fond clair, corps 16 px, marges (`suite.css`) », papier de scène « **fond
sombre** », raison en `:438-441` — « le document de scène est lu en coulisse, sur
un téléphone, dans le noir ». Et la dernière phrase vaut décision : « *Ce PDF-ci
est sur fond sombre à votre demande : c'est donc le premier document de la ligne
scène, et il vaut décision.* »

**Ce que la maison a déjà fait.** `diorama-01-reponses.md:406-412` a établi que
le papier sombre appartient à la ligne scène ; `DIRECTION-ARTISTIQUE.md:196-198`
le porte ; et `docs/suite.css:260-275` vient d'être corrigée, sur ce relevé, pour
dire que l'absence de thème sombre est **la valeur atelier d'une variable de
cadran** et non un interdit universel. La remontée avait raison : le tour 01
n'avait repris qu'une des deux occurrences.

**Ce qui existe.** Rien. Un papier, un seul, et il est clair.

### Verdict : **reporté**

Le papier sombre n'attend aucune mesure et aucun arbitrage de goût. Il attend
**deux valeurs qui sont ailleurs dans ce même tour** : la mono unique
(`SCRATCHVJ-06`, Q2 — non tranchée, une heure de test nommée) et la luminance
commune (`SCRATCHVJ-06`, Q1 — la contrainte est écrite, le calcul reste à faire).
Écrire `--paper` et `--vals` avant les deux, c'est graver ce que `secteurs.html:355-357`
décrit exactement : « un accident qu'on ne nomme pas devient une convention en
six mois, et une convention se défend ».

**Qui doit le produire** : la maison, dans `docs/suite.css`, dès que
`SCRATCHVJ-06` est répondue.

**Ce qu'il doit porter — et ce n'est pas écrit ici, c'est spécifié.**

**Sa forme.** Un bloc de redéfinition de variables sous une classe de ligne —
`.line-scene { --paper: …; --ink: …; }` — et **pas une seconde feuille.** Tout le
reste du code reste littéralement le même : structure, échelle, filets, tableaux,
encadrés, couverture, sommaire. C'est ce qui rend « une feuille, deux papiers »
(`secteurs.html:482`) vrai plutôt que déclaratif : le jour où le sommaire change,
il change pour les deux salles. Une seconde feuille est la seconde direction
artistique de `secteurs.html:648`, sous une autre forme.

**Les sept jetons à inverser** : `--paper`, `--paper-sunk`, `--ink`, `--ink-dim`,
`--ink-off`, `--rule`, et les quatre signaux. Deux points qui ne se déduisent pas
de l'inversion :

- `--paper-sunk` doit être **plus sombre** que `--paper`, pas plus clair. Sur du
  clair, un encadré s'enfonce en s'assombrissant ; sur du sombre, il s'enfonce en
  s'assombrissant aussi. L'inverser mécaniquement produit un encadré qui saille,
  ce qui n'est pas ce que la classe veut dire — c'est le même piège que le champ
  *creusé* contre *relevé* de `SCRATCHVJ-07`.
- Les quatre signaux s'**éclaircissent** pour le fond sombre, avec la même raison
  écrite qu'en `suite.css:38-40` pour le fond clair — c'est le troisième jeu de
  support de la règle écrite en `SCRATCHVJ-09`, et il n'y en aura pas de
  quatrième avant longtemps.

**Les quatre choses que l'inversion ne donne pas.**

1. **L'impression, et c'est le vrai obstacle.** Un PDF de 33 pages à fond noir
   plein bord ne s'imprime pas : selon le moteur, l'encre coule ou chaque page
   sort avec un liséré blanc dans les marges. `secteurs.html` lui-même a dû
   contourner par un montage explicite — marges de page à zéro et marges
   visuelles reprises par un tableau à en-tête et pied répétés. **Le thème doit
   trancher, et la réponse est écrite dans la variable 6 elle-même** : `:438-441`
   dit que le document de scène est lu « en coulisse, sur un téléphone ».
   Personne ne l'imprime. **Le papier sombre déclare qu'il n'est pas imprimable
   et n'écrit que pour l'écran** — un bloc `@media print` qui rend le papier
   clair, ou rien. Prétendre le contraire coûte le montage à chaque page, pour un
   usage que la source dit inexistant.
2. **La couverture fonctionne telle quelle.** `suite.css:194-216` ne contient
   aucune valeur de couleur : filet d'accent de 2 px, nom, ligne descriptive,
   ligne d'imprint en chasse fixe. Elle tient sur fond sombre sans une ligne de
   plus — et remplacer la couverture en aplat de la feuille locale supprime du
   même coup l'aplat que `DIRECTION-ARTISTIQUE.md:187-190` interdit. **C'est le
   seul endroit de tout ce tour où la maison rend au produit plus qu'elle ne lui
   prend.**
3. **La chasse fixe** — `SCRATCHVJ-06`, Q2.
4. **L'accent** — il n'y a pas de classe produit pour un produit de scène, et il
   n'y en aura pas avant le nom : `SCRATCHVJ-19`.

**Une observation qui n'est pas anecdotique, et que la maison doit garder.** Le
document qui **fonde** la ligne scène a un papier de `#0B0D0C` ; son propre
nuancier, deux pages plus loin, donne atelier `#0A0C0B` et scène `#141412`. Sa
luminance relative vaut 0,0039 — exactement entre les deux. **Le premier document
de la ligne scène n'est à aucune des deux valeurs qu'il prescrit.** Ce n'est pas
un reproche : c'est la démonstration qu'un papier sombre sans feuille commune
dérive dès le premier document, y compris quand c'est celui qui écrit la règle.
`secteurs.html` prend le papier de la ligne scène le jour où il existe.

**Ce que le produit doit faire.** Ne pas réécrire `docs/pdf/_style.css`. Sa
phase 7 est **déjà rendue** : cette remontée *est* la note que `MAISON.md:95` lui
demandait — « ce que `suite.css` devrait porter, une note, pas une feuille ». La
ligne reste en *remonté* jusqu'à ce que le thème existe.

---
---

# Partie II — Les cinq remontées qui valent pour toute la maison

---

## SCRATCHVJ-01 — La forme canonique de « non déclaré »

**Ce que dit la source.** `spec/00-vocabulaire.md:5-8` : « la chaîne écrite sur
disque ou passée d'une application à l'autre est **toujours** la forme
canonique ». La table `:21-29` et les projections `:41-48` donnent les formes.
`:94` bannit « `equirectangular` seul ».

**Ce que fait le code.** Trois valeurs écrites dans le fichier de bibliothèque :
`"flat"`, **conforme** — et c'est la seule forme canonique du catalogue
effectivement écrite sur disque, tous produits confondus ; `"equirect"`, écart
connu dont la réparation est écrite ; et `"auto"`, **pour lequel il n'y a aucune
forme canonique à écrire**.

### Verdict : **tranché, source modifiée**

**L'absence de la clé est la forme canonique de « non déclaré ». Le vocabulaire
le dit désormais.**

Trois raisons, et la deuxième vient d'être écrite par une autre grappe.

1. **C'est ce que le code fait déjà, ici et probablement ailleurs.** Le lecteur
   de ce produit traite un champ manquant comme non forcé. La règle est vraie
   avant d'être écrite ; il suffit de l'écrire, et
   `03-ponts.md:45` — « ne construire que ce qui manque » — dit que c'est le seul
   travail légitime.
2. **Une valeur `auto` mélangerait deux niveaux dans le même champ : *ce que
   c'est* et *d'où on le sait*.** Et la seconde a désormais un endroit à elle :
   **`evidence`**, à qui la grappe G7 vient d'ajouter un quatrième rang
   (`filename`) et d'imposer que la résolution porte sur la plate entière
   (`GRAPPES-01-reponses.md:209-223`). Un champ qui porterait `auto` ferait le
   travail d'`evidence` en moins bien et à un endroit où huit produits
   devraient migrer.
3. **Une valeur canonique de plus est une migration de plus.** Elle coûterait à
   tout produit qui écrit un descripteur, pour une information qu'un champ absent
   porte déjà sans ambiguïté.

La clause est courte et elle ferme la question pour les dix produits :

> **Ce qui n'est pas déclaré ne s'écrit pas.** Une clé absente signifie « non
> forcé par l'utilisateur, à déduire » ; c'est la seule écriture de cet état, et
> aucune valeur canonique ne le nomme. La provenance d'une valeur déduite est
> portée par `evidence` (`01-manifeste-plate.md`), jamais par une valeur du champ
> lui-même. Un lecteur qui rencontre une clé inconnue refuse le fichier en
> nommant la valeur fautive ; un lecteur qui rencontre une clé absente déduit.

**Ce que le produit doit faire.** Cesser d'écrire `"auto"` et omettre la clé, dans
la **même migration** que `equirect` → `equirect_360` — les deux touchent le même
fichier et se corrigent, selon le mot de `DIORAMA-02`, « ensemble ou pas du
tout ». Garder le refus de chaîne inconnue, qui est déjà juste.

---

## SCRATCHVJ-02 — La projection d'un fichier n'est pas la reprojection d'une vue

**Ce que disent les sources.** `spec/00-vocabulaire.md:41-48` donne six
projections avec leur domaine et leurs paramètres. `ERGONOMIE.md:227-230` : « le
viseur affiche **toujours une reprojection rectiligne** ». Le mot *rectiligne*
est déjà employé par `00-vocabulaire.md:48` pour décrire `flat`.

**Ce que fait le code.** Deux énumérations coexistent : l'une décrit **comment le
fichier est stocké**, l'autre **comment la vue est calculée depuis la sphère** —
`Perspective`, `LittlePlanet`, `Fisheye`. Deux de ces trois mots n'existent pas
dans le vocabulaire. **Le troisième existe et veut dire autre chose** : en
`00-vocabulaire.md:45`, `fisheye` est la projection d'un fichier, paramétrée par
`lens`, `fov_deg`, `center`, `radius`, `count`. Ici c'est un mode de rendu d'une
équirectangulaire, sans aucun de ces paramètres.

### Verdict : **tranché, source modifiée**

**Le vocabulaire nomme les reprojections, dans une table séparée du même
document.** Le motif est celui que la maison a déjà retenu deux fois : quand un
concept se fait confondre dans deux produits indépendants, ce n'est pas un
accident local.

**Et il l'est.** C'est exactement la collision du piège 2 de
`DIVERGENCES.md:87-90`, où `flat` désigne chez Lacuna un mode d'affichage et non
une projection. **Le piège a été vu une fois et n'a jamais été traité au niveau
où il se produit** : le vocabulaire n'a de mots que pour les fichiers, alors que
six produits sur dix portent un viseur, et qu'un viseur reprojette.

Trois formes, et je retiens les trois noms de la remontée avec ses raisons :

| Canonique | Ce que c'est |
|---|---|
| `rectilinear` | la vue par défaut d'un viseur — `ERGONOMIE.md:227-230` |
| `little_planet` | la vue depuis le nadir, champ très large |
| `fisheye_view` | la vue en projection circulaire |

- **`rectilinear` plutôt que `perspective`** : c'est le mot que
  `00-vocabulaire.md:48` emploie déjà, et il évite de donner le mot le plus
  générique de l'image de synthèse à un cas particulier.
- **`fisheye_view` plutôt que `fisheye`** : la désambiguïsation se porte par le
  mot le plus récent, jamais par celui qui est déjà écrit dans une table
  paramétrée et lu par des produits en service.

**Deux bornes, qui sont ce qui rend l'ajout légitime plutôt que spéculatif.**

**(a) Ce sont des identifiants de code et des libellés, pas des chaînes de
disque** — aujourd'hui. La table le dit d'elle-même. Aucun produit ne les écrit,
et la règle de `00-vocabulaire.md:5-8` ne les atteindra que le jour où un pont
les transporte.

**(b) Ce jour-là est déjà identifié, et il est proche.**
`spec/03-ponts.md:136-138` fait circuler `plate-view/1 yaw=… pitch=… roll=…
fov=…` dans le presse-papiers, entre sept produits. **Cette ligne ne dit pas dans
quelle reprojection le champ est exprimé** — or un `fov=75` rectiligne et un
`fov=75` en little planet ne montrent pas la même chose. `03-ponts.md` le note,
et le champ s'ajoutera avec sa valeur par défaut `rectilinear` le jour où un
second produit sait le produire, pas avant.

**Leurs paramètres ne sont pas spécifiés, et c'est volontaire.** Personne ne les
transporte ; les inventer serait concevoir au lieu de spécifier, ce que
`DIORAMA-02` a déjà refusé sur l'espace colorimétrique. Ce qui est spécifié,
c'est le **domaine de champ** de chacune, parce que celui-là est demandé
maintenant — voir `SCRATCHVJ-16`.

**Ce que le produit doit faire.** Renommer ses trois identifiants et ses trois
libellés en phase 2. Aucun fichier écrit sur disque n'est touché.

---

## SCRATCHVJ-12 — Le rayon des contrôles : 1 px, 3 px, ou « minimal » ?

**Ce que disent les sources — trois fois, et pas pareil.**

- `tokens.json:22` : `"radius_control": 3, "radius_panel": 0`.
- `DIRECTION-ARTISTIQUE.md:265` : « Coins très arrondis (**le rayon est 3px sur
  un contrôle**, 0 sur un panneau). »
- `secteurs.html:370-371`, **invariant de maison** : « Angle vif sur les
  panneaux, **rayon minimal sur les contrôles**. »

Deux donnent un nombre, la troisième un critère — et 1 px satisfait « minimal »
mieux que 3.

### Verdict : **tranché, source modifiée**

**3 px, et `secteurs.html:371` cesse de paraphraser une valeur pour la citer.**

Trois raisons, dont la première suffit.

1. **Le fichier de jetons fait foi.** `maison/00-LIRE-DABORD.md:61` : « C'est ce
   fichier qui fait foi, pas les nombres recopiés ailleurs. » Un invariant qui
   paraphrase `tokens.json` en le déformant est un nombre recopié, et il perd.
2. **`secteurs.html:457` range le rayon des angles parmi ce qui n'entre pas dans
   le cadran.** Il ne peut donc pas y avoir deux valeurs, et deux documents sur
   trois donnent la même.
3. **Un critère qualitatif dans un invariant est un accident en attente.** Dix
   produits lisant « minimal » écriront dix valeurs. `DIVERGENCES.md:131` mesure
   déjà exactement cela — « 3 px contre 6 px contre 6/8/10 px et une pilule de
   12 px ». C'est le mode de panne que ce chantier existe pour arrêter, et le
   voir apparaître **dans le document qui l'interdit** est le signe qu'il faut
   corriger la formulation et pas seulement la valeur.

**La réserve de la remontée est honnête et elle est retenue telle quelle.** À un
mètre dans le noir, 3 px sur une cible de 44 se voient un peu plus qu'à 50 cm sur
une cible de 28. Le produit a alors appliqué le test du cadran contre son propre
confort : « si la raison ne peut pas s'écrire, la variable retourne à
l'invariant » (`secteurs.html:386-389`) — et il n'a pas pu l'écrire. **C'est le
test qui fonctionne, employé exactement comme il a été conçu.** Rien à ajouter au
cadran.

**Ce que le produit doit faire.** Passer les deux rayons de `1.0f` à `3.0f`.
Deux lignes de la fonction de style et deux appels de dessin — le point le moins
cher des vingt.

---

## SCRATCHVJ-16 — Les constantes `viewer` quand le regard est piloté au potard

**Ce que disent les sources.** `tokens.json:48-57` porte un bloc `viewer`,
« constantes du viseur 360, partagées par les six produits qui en portent un » :
champ par défaut 75°, bornes 30–120°, tangage borné à `±89,9°`, 0,18 °/px au
glisser, HUD en bas à droite, taille 11. `ERGONOMIE.md:55-56` nomme les six, et ce
produit n'en est pas. `MAISON.md:60` du produit tranche la moitié gestuelle :
« là où il est piloté par un potard, la convention ne s'applique pas ».

**Ce que fait le code.** Le regard n'est jamais manipulé à la souris — vérifié,
aucun glisser n'est lu sur la vue 360. Il est piloté par quatre curseurs et par
les potards d'égalisation via le mapping. Champ par défaut 90° contre 75°, bornes
20–170° contre 30–120°, tangage à ±90° contre ±89,9°.

### Verdict : **tranché, source modifiée**

**Le bloc se scinde, et n'exige que sa moitié géométrique.** La remontée a raison
sur le fond : le geste est réglé, les **nombres** ne le sont pas, et ils ne
dépendent pas du geste.

- **`viewer.geometry`** — `pitch_deg_clamp`, `fov_deg_default`, `fov_deg_min`,
  `fov_deg_max` : exigibles de **tout produit qui reprojette une sphère**, quel
  que soit le geste. La raison du tangage borné est écrite en
  `ERGONOMIE.md:222-223` et elle est purement mathématique : « au pôle exact, le
  lacet devient indéfini et le viseur saute ». Un potard poussé à fond tombe sur
  le pôle exactement comme une souris.
- **`viewer.drag`** — `drag_deg_per_px` : n'a de sens que là où il y a un
  glisser.
- **Le HUD** sort de `viewer`. Position et taille appartiennent au châssis
  (`ERGONOMIE.md:292-304`, section de niveau A), donc au niveau de conformité, pas
  aux constantes de reprojection. Il n'aurait jamais dû être dans ce bloc : c'est
  ce qui a fait croire à ce produit qu'il devait un HUD en bas à droite alors
  qu'il n'a pas de viseur au sens du châssis.

**Sur les bornes de champ, la remontée a raison contre `tokens.json`, et la
correction n'est pas celle qu'elle propose.** Le mode little planet a besoin d'un
champ que 120° ne couvre pas, et le code borne à 170° avec sa raison écrite —
« une projection planaire ne peut pas atteindre 180 degrés ». Faire dépendre les
bornes du produit ferait dix bornes ; **les faire dépendre de la reprojection en
fait autant qu'il y a de reprojections, et c'est la bonne cardinalité.**

`00-vocabulaire.md:41-48` donne déjà une colonne **Domaine** à chaque projection.
La table des reprojections créée par `SCRATCHVJ-02` en reçoit une aussi, et
`tokens.json` cesse de porter une borne unique : `viewer.geometry.fov_deg` donne
le défaut, et le domaine vient de la reprojection choisie. **Sans cela la règle
est inapplicable au premier produit qui l'applique**, ce qui est la définition
d'une règle à ne pas écrire.

**±89,9 contre ±90 est à corriger côté produit, sans discussion.** C'est le
« détail que seul ce métier connaît » de `DIRECTION-ARTISTIQUE.md:237-238`, et ce
produit en porte déjà d'autres — les contrôles fantômes dessinés en pointillé
plutôt qu'à une valeur inventée. Celui-là lui manque.

**Ce que le produit doit faire.** Borner le tangage à ±89,9°, référencer
`viewer.geometry` par nom de jeton, et ne pas fabriquer de HUD.

---

## SCRATCHVJ-20 — Quatre inexactitudes dans les sources

### Verdict : **tranché, source modifiée**

**1. Dix touches réservées contre douze.** **Déjà corrigé** par la grappe G15
avant l'arrivée de ce relevé : `ERGONOMIE.md:448` porte « les **douze** lignes du
clavier réservé ».

**Mais la remontée demande mieux que la correction, et elle a raison.** Un
compte dans une liste de contrôle est un compte qu'on vérifiera, et qui divergera
à la treizième ligne. C'est exactement la règle que G10 a portée dans
`maison/00-LIRE-DABORD.md:179-188` — « on nomme ce qu'on compte » — et la liste
de contrôle est le seul endroit qui ne l'a pas encore appliquée à elle-même. Le
nombre devient un renvoi.

**Et le même défaut existe trois lignes plus haut, non remonté.**
`ERGONOMIE.md:367-369` : « **Les trois cas** d'aujourd'hui : écraser un fichier
existant, et quitter alors qu'un travail est en cours. **Tous deux** parce qu'ils
sont irréversibles. » Trois annoncés, deux listés, « tous deux » pour conclure —
le troisième cas, la suppression définitive de Mutoscope, a été accordé par G9 et
a disparu de la phrase le même jour. **Le verdict qui a remplacé une liste par un
critère a laissé le compte de la liste derrière lui.** Corrigé avec.

**2. La définition de fini dépasse ses phases.** **Répondu par la grappe G4**,
`maison/00-LIRE-DABORD.md:144-163`, et le partage y est écrit : *retirer est
toujours dans le périmètre, ajouter ne l'est que si une phase le porte.*
Appliqué aux quatre exigences que la remontée relève :

| Exigence | Ce que c'est | Où elle va |
|---|---|---|
| aucune erreur n'ouvre de modale | retrait | dans le périmètre — le produit en a **zéro**, sans objet |
| chaque erreur nomme la valeur et l'action qui répare | réécriture de chaînes | dans le périmètre, avec les libellés de phase 2 |
| aucune unité dans un libellé | retrait | dans le périmètre, phase 2 |
| aucun pourcentage seul | **ajout** — il faut produire l'étape nommée et le temps restant | **au carnet du produit**, pas à la définition de fini |
| un chemin tronqué par le milieu | — | satisfait sans effort |

**Je refuse la phase « liste de contrôle » que la remontée propose.** Ajouter une
phase pour accueillir ce que les phases ne couvrent pas réimporte exactement la
contradiction que G4 vient de retirer à cinq produits : une définition de fini
qui programme du travail. Trois des quatre exigences sont des retraits et
s'attachent à des phases qui existent ; la quatrième est une fonctionnalité, elle
va au carnet, et elle n'entre dans une définition de fini que le jour où une
phase la porte.

**3. Un piège annoncé qui n'existe pas.** `MAISON.md:59` du produit : « Vérifier
qu'aucune transition ImGui n'a été laissée à sa valeur par défaut. » La
bibliothèque employée **n'a aucun champ de durée de transition** : il n'y a pas
de valeur par défaut à corriger parce qu'il n'y a pas de champ. La variable 5 du
cadran est tenue sans que personne ait eu à la tenir — le relevé compte zéro
animation.

La ligne est reformulée dans le générateur en un contrôle qui se vérifie :
« vérifier qu'aucune animation d'interface n'existe ». **Un piège annoncé est un
piège qu'on cherche**, et `MAISON.md` avertit lui-même contre le fait d'aligner
sur ce qu'on croit savoir.

**4. `design/README.md:43-44` du produit désigne le mauvais fichier.** Hors
maison — c'est un document de produit. Noté pour que la phase 5 corrige le renvoi
en déplaçant la fonction.

**Ce que le produit doit faire.** Rien sur 1, 2 et 3 : ce sont les sources qui
changent. Corriger le renvoi de son propre README en phase 5.

---
---

# Partie III — Les deux remontées propres à ce produit

---

## SCRATCHVJ-18 — La détection au ratio seul, et `spec/evidence.py`

**Ce que dit la source.** `DIVERGENCES.md:110-120` recense quatre
implémentations de la détection automatique, avec quatre règles de priorité.
`REGISTRE.md:208` inscrit la règle d'arbitrage comme ouverte.

**Ce que fait le code.** Une image dont la largeur vaut deux fois la hauteur est
marquée sphérique, et rien d'autre n'est consulté — alors que le produit appelle
déjà `ffprobe` pour la durée, la cadence et les dimensions. Il possède en
revanche `declared` : l'utilisateur peut forcer la projection, le choix est écrit
et l'emporte. **Deux rangs sur trois, et c'est celui du milieu qui manque.**

### Verdict : **tranché, source modifiée**

**La question est répondue par la grappe G7** (`GRAPPES-01-reponses.md:187-226`) :
Mutoscope est l'implémentation de référence, `evidence` gagne un quatrième rang
`filename` entre `metadata` et `declared`, et la résolution porte sur la plate
entière et non champ par champ. Le produit apporte un **cinquième cas mesuré** et
deux précisions. La première n'est pas dans G7, et elle décide.

**1. `evidence.py` doit dire ce qui se passe quand un rang manque.** G7 a écrit
l'ordre des rangs ; il n'a pas écrit s'ils sont obligatoires. Ce produit n'a pas
`metadata` et n'en aura pas de sitôt — lire les métadonnées spatiales demande une
dépendance qu'un instrument temps réel n'a pas de raison d'embarquer, et
`maison/02-INTERDITS.md:12-14` interdit d'en ajouter une pour un alignement.

> **Une règle à quatre rangs dont deux sont facultatifs est une règle
> différente d'une règle à quatre rangs obligatoires** — et c'est la différence
> qui décide si un produit peut s'y conformer ou non.

Sans cette clause, `spec/evidence.py` naîtra en décrivant Mutoscope, et tout
produit plus léger sera non conforme par construction. Elle entre dans
`spec/01-manifeste-plate.md`, à côté de l'échelle des rangs : **un rang absent est
absent, il ne dégrade rien ; c'est l'ordre entre les rangs présents qui fait la
règle.**

**2. Le ratio 2:1 reste une inférence explicitement faillible, et le produit en
donne le motif d'affichage.** Son sélecteur affiche **« Auto (360°) »** plutôt que
« 360° » — c'est-à-dire *ce que la machine croit, et le fait qu'elle le croit*.
C'est exactement ce que G7 a retenu pour choisir Mutoscope : « il affiche
l'origine de la décision et la laisse corriger ». **Le motif est réutilisable et
il vaut d'être écrit** : une spécification d'`evidence` rédigée sans lui produirait
une hiérarchie sans aucune conséquence visible, c'est-à-dire une hiérarchie que
rien ne vérifie.

**Ce que le produit doit faire.** Rien. Garder l'inférence au ratio et garder
« Auto (360°) ». Fermer la ligne du relevé en citant G7.

---

## SCRATCHVJ-19 — Ni `NOMS.md` ni `suite.css` ne portent de ligne pour ce produit

**Ce que disent les sources.** `NOMS.md:217-227` porte un tableau « JEU ARRÊTÉ »
de neuf lignes : la maison plus huit produits. Aucune ne correspond à ce dépôt.
`suite.css:53-61` porte huit classes produit ; aucune non plus.
`secteurs.html:320-345` explique pourquoi et l'explication est complète : le nom
de travail décrit la fonction, ce qui est un motif absolu de refus au dépôt — le
même défaut que *Relief* ; le registre est retenu, mais *Praxinoscope* fait douze
lettres et « aucun candidat court n'a été vérifié ».

### Verdict : **tranché, source modifiée**

**Une ligne, et pas un nom.** La recommandation est retenue telle quelle, et
pour les trois raisons qu'elle donne — dont la première est déjà un précédent de
ce dépôt : `NOMS.md:219` porte « ~~Theama~~ **— non arrêté** » avec sa raison.
**Une absence écrite se distingue d'un oubli ; une absence muette, non**, et
c'est le motif exact que `DIORAMA-09` a fait retenir pour `DIVERGENCES.md`.

**Et la table en manque deux, pas une.** Le tableau s'intitule « JEU ARRÊTÉ » et
omet silencieusement les deux produits dont le nom ne l'est pas :

| Emplacement | Nom | Ligne descriptive |
|---|---|---|
| `UE_SCRATCHING` | **— en cours** | instrument de scratch vidéo au timecode DVS ; registre retenu, candidat court à vérifier |
| *(hors machine)* | **— en cours** | copilote de captation 360° sur téléphone ; *Osmo Spatial* écarté, famille en `-orama` |

Le copilote a exactement le même problème (`maison/05-QUI-EST-QUI.md:121-126`), il
n'a personne pour le remonter puisque son dépôt ne peut pas lire la maison, et la
prochaine session qui ouvrira `NOMS.md` conclura que **deux** produits ont été
oubliés et proposera **deux** noms. La règle est donc générale et pas
particulière : **le tableau porte tous les produits du catalogue ; un nom non
arrêté s'y écrit comme tel.**

**Sur `suite.css`, la remontée recommande contre son propre intérêt et elle a
raison.** Une classe `.p-scratchvj` posée sous un nom de travail dans une feuille
que huit produits partagent serait à renommer au premier nom trouvé, et le
renommage traverserait chaque document déjà publié. **Pas de classe avant le
nom.** C'est le second verrou de la phase 7, l'autre étant le papier sombre.

**Sur le nom lui-même, la maison ne tranche pas, et pour la même raison que
G1.** `secteurs.html:342-345` dit que c'est « un travail à part, à ne pas
improviser dans ce document », et `secteurs.html:664` le classe « à tenir avant
la première vidéo publique : un nom qui a circulé ne se retire plus ».

**Ce que le produit doit faire.** Continuer à employer le nom de travail dans ses
fichiers internes, ne rien publier sous ce nom, et attendre pour la classe
documentaire.

---
---

# Ce que ce tour apprend à la maison

**Une salle sans source finit par avoir un cadran sans valeurs.** Treize des
vingt remontées disent la même chose sous treize formes : la variable existe, sa
raison est écrite, et sa valeur n'est nulle part. Ce n'est pas une négligence de
rédaction. `secteurs.html` est un document de **stratégie commerciale** ; le § 5
y a été écrit pour montrer qu'une seconde ligne ne coûtait pas une seconde
maison, et il l'a montré. Un document qui démontre n'a pas besoin de nombres ;
un document qui gouverne, si. Le cadran a changé de rôle en cours de route, et
personne n'a déplacé son adresse.

**Le contraste avec `TERRAIN.md` est ce qui rend le diagnostic sûr.** Le
troisième cadran a été écrit le même jour, pour une salle dont le produit n'est
même pas sur cette machine, et il porte des nombres : 16/20/24/32/56, 48 px de
cible, 24 px de gouttière, quatre états doublement encodés avec leur trame et
leur glyphe. La différence entre les deux n'est pas l'ancienneté ni l'importance
— c'est qu'un des deux vit dans `design/` et l'autre dans `docs/`.

**Une règle appliquée deux fois et écrite une fois de travers reste une règle.**
`secteurs.html:457` interdit à un cadran de changer l'échelle typographique, et
`TERRAIN.md:85` la change, et personne n'a appelé ça une infraction. La ligne
scène s'est trouvée seule à buter dessus parce qu'elle est la seule à avoir eu un
produit qui a lu les deux documents et compté les tailles.

**Une preuve peut tomber sans que la conclusion tombe.** Deux fois dans ce tour :
le conflit de clavier `1`–`5` de ce produit n'existe plus, et la conclusion tient
par Mutoscope seul ; « Fragment Mono n'est présente nulle part sur le disque »
est faux depuis G6, et le test de la mono reste à passer sur son autre jambe.
Dans les deux cas, la bonne réaction est de changer la citation, pas de défendre
la phrase.

**Un relevé qui refuse son propre confort vaut mieux qu'un relevé qui obéit.**
`SCRATCHVJ-12` et `SCRATCHVJ-13` recommandent tous deux contre le code du
produit, en appliquant le test du cadran — *si la raison ne peut pas s'écrire, la
variable retourne à l'invariant* — et en constatant qu'ils ne peuvent pas
l'écrire. C'est la première fois qu'un produit emploie ce test contre lui-même.
C'est ce qui rend recevables les onze fois où il l'emploie en sa faveur.

**Et un chiffre, pour finir.** Sept écarts apparents étaient des conformités,
sur un relevé qui en compte une quarantaine. **Le cadran a évité six chantiers
destructeurs, et il a coûté treize questions.** C'est le bon échange, et c'est
l'argument à opposer à la prochaine session qui proposera de simplifier en
ramenant tout à un seul jeu de règles.

---
---

# Les modifications de source que ces verdicts imposent

**Aucune source n'a été modifiée par cette session.** Sept autres travaillent en
parallèle sur les mêmes fichiers ; ce qui suit est la liste des changements, avec
le passage actuel et son remplacement, à appliquer par la session qui tient les
sources.

## Vue d'ensemble

| Fichier | Ce qui change | Remontées |
|---|---|---|
| **`design/SCENE.md`** | **à créer** — le second cadran, comme source, sur le modèle de `TERRAIN.md` | 03 à 15, 17 |
| `design/tokens.json` | **v3** — `lines: { atelier, scene }` ; `viewer` scindé | 04, 06, 07, 08, 09, 16 |
| `docs/secteurs.html` | § 5.1 l'échelle et le rayon ; § 5.2 variables 2 et 7 ; § 5.3 la mono et le clavier ; le papier du document lui-même | 04, 05, 11, 12, 06, 15, 17 |
| `design/ERGONOMIE.md` | la forme courte ; la frontière libellé / titre ; la clause du clavier ; la pile d'`Échap` ; deux comptes ; la scène dans le tableau des niveaux | 03, 13, 15, 20 |
| `design/DIRECTION-ARTISTIQUE.md` | un jeu de signalisation par support ; l'étiquette d'un ou deux caractères ; Mutoscope absent du tableau des accents | 09, 10, *(en chemin)* |
| `spec/00-vocabulaire.md` | ce qui n'est pas déclaré ; la table des reprojections | 01, 02, 16 |
| `spec/01-manifeste-plate.md` | la clause du rang absent | 18 |
| `spec/03-ponts.md` | `plate-view/1` et la reprojection ; « sept des huit » | 02, *(en chemin)* |
| `design/NOMS.md` | deux lignes au jeu arrêté ; « 21 effets » | 19, *(en chemin)* |
| `tools/gen-maison.py` | « mot pour mot » ; les pads `1`–`5` ; la transition ImGui | 11, 15, 20 |
| `maison/05-QUI-EST-QUI.md` | le second produit de scène : le nommer ou retirer la phrase | 05, 15 |
| `remontees/REGISTRE.md` | vingt lignes ; trois affectations de grappe fausses ; quatre points ouverts | tous |

---

## 1. `design/SCENE.md` — à créer

**C'est le changement qui porte les douze autres.** Il ne contient aucune
décision nouvelle : il rassemble à une adresse ce que les verdicts ci-dessus
tranchent et ce que `secteurs.html` § 5 dit déjà.

Sa forme est celle de `TERRAIN.md`, dans le même ordre, et son en-tête dit la
même chose : *ce document est une source ; il ne remplace pas
`DIRECTION-ARTISTIQUE.md`, il en dérive, en déclarant les sept variables du
cadran pour une seconde salle, et en justifiant chaque écart par une raison
métier.* Il porte :

- **pourquoi une seconde salle** — repris de `secteurs.html:182-226`, et non
  réécrit ;
- **ce qui ne change pas** — l'invariant de `secteurs.html:359-381` ;
- **les sept variables réglées pour la scène**, avec pour chacune sa valeur et sa
  raison. Quatre reçoivent des valeurs de ce tour (03, 04, 07, 09, 10, 11) ;
  **la variable 7 reçoit un trou écrit** : « valeurs non écrites — voir
  `REGISTRE.md` ; ce qui manque, ce qui le produit » ;
- **ce que la scène doit à `ERGONOMIE.md` et ce qui y est sans objet** — le
  noyau du clavier, la pile d'`Échap`, le langage, la précision d'affichage,
  l'erreur sans modale ; sans objet : le châssis, le travail long, le viseur ;
- **la lecture scène de la liste de contrôle** (voir § 4 ci-dessous) ;
- **ce qui reste ouvert**, sur le modèle de `TERRAIN.md:230-242` : les cinq crans
  de l'échelle, les trois étages de la densité, la mono, la luminance, le clavier
  de ligne, le nom, le repère temporel — chacun avec *ce qui manque* et *qui doit
  le produire*.

**Et `maison/00-LIRE-DABORD.md` change de renvoi.** Sa section « si le produit
est de la scène » (`:80-97`) désigne `docs/secteurs.html` § 5 en rang 2 ; elle
désignera `design/SCENE.md`, et `secteurs.html` redeviendra ce qu'il est — un
document commercial que le cadran cite pour sa raison d'être, pas la source qu'on
lit avant le code. Le rang 12 de la liste générale (`:70`) accueille la scène à
côté du terrain.

---

## 2. `design/tokens.json` — version 3

**Passage actuel** — `:3-4` :

```json
  "version": 2,
  "revised": "2026-09-08",
```

**Remplacement** :

```json
  "version": 3,
  "revised": "2026-09-09",
```

**Ce que la v3 ajoute**, et rien d'autre :

- un niveau **`lines: { atelier, scene }`** au-dessus de `chassis`, comme
  `secteurs.html:492-493` le prévoit. `lines.atelier` reçoit les valeurs
  actuelles sans changement — **aucun produit d'atelier ne voit de différence** ;
- **`lines.scene.chassis`** : les **sept** valeurs chaudes, pas une. `void`,
  `panel`, `raised`, `line`, `chalk`, `chalk_dim`, `chalk_off`. Avec la
  contrainte de la variable 3 écrite comme un test : *luminance relative à ±5 %
  de la valeur atelier correspondante, dérive chaude R > G > B*. `void` porte la
  valeur qui sort du calcul de `SCRATCHVJ-06`, Q1 ; `raised` tranche le champ
  **creusé** contre **relevé** ;
- **`lines.scene.pair`** : `a` = `#C99A2F` (ambre), `b` = `#6E8696` (ardoise),
  avec le rôle — *état, pas identité ; constante sur toute la ligne* ;
- **`lines.scene.signal`** : les quatre teintes au jeu de valeurs de la scène,
  `warn` compris, sous la règle « un jeu par support » de `SCRATCHVJ-09` ;
- **`lines.scene.type.scale`** : le jeu fermé, marqué **« candidat — un seul
  produit »**, avec `15` comme corps ;
- **`lines.scene.rhythm`** : `hit_target_min` à 44 ; les trois étages **absents**,
  avec un `$comment` qui dit ce qui manque et qui le produit ;
- **`viewer` scindé** en `viewer.geometry` (`pitch_deg_clamp`, `fov_deg_default`)
  et `viewer.drag` (`drag_deg_per_px`) ; `hud_corner` et `hud_size` **sortent** du
  bloc et rejoignent le châssis ; `fov_deg_min` / `fov_deg_max` sortent aussi, le
  domaine de champ étant porté par chaque reprojection dans
  `00-vocabulaire.md` ;
- la **règle de recevabilité** du `$comment` d'en-tête s'étend comme
  `secteurs.html:496-500` le demande : un jeton n'entre dans l'invariant que si
  les toolkits des **trois salles** savent le produire — ce qui en fait **six**,
  Dear ImGui sur bgfx et le téléphone compris.

> **Produits qui doivent recopier leur fichier de jetons : aucun.**
> `maison/01-LES-TROIS-COUCHES.md:110-112` constate qu'au 2026-09-09 aucune de ces
> copies n'existe encore. **scratchvj écrira la première, en phase 5**, et c'est
> la dernière occasion de toucher ce fichier à coût nul.

---

## 3. `docs/secteurs.html`

**a) § 5.1, l'échelle — `:369`.**

Actuel : « **L'échelle typographique.** Un jeu de tailles fermé, pas un
continuum. »

Remplacement : « **La fermeture de l'échelle typographique.** Un jeu de tailles
fermé, pas un continuum — **le jeu, lui, est du cadran** : il s'ancre sur la
distance de lecture (variable 1). L'atelier tient 11/12/13/16/28, le terrain
16/20/24/32/56 (`design/TERRAIN.md`). Ce qui ne diverge jamais, c'est qu'il y en
ait cinq et qu'on n'en sorte pas. »

**b) § 5.1, le rayon — `:370-371`.**

Actuel : « Angle vif sur les panneaux, **rayon minimal sur les contrôles**. »

Remplacement : « Angle vif sur les panneaux (`radius_panel` 0), **rayon de 3 px
sur les contrôles** (`radius_control`, `design/tokens.json`). La valeur est
citée, pas paraphrasée : « minimal » a déjà produit quatre rayons différents dans
le catalogue. »

**c) § 5.2, ce qui n'entre pas dans le cadran — `:457`.**

Actuel : « La famille de fontes, l'échelle typographique, le rayon des angles, la
règle des filets, la nature des nombres, les couleurs de signalisation. »

Remplacement : « La famille de fontes, **la fermeture** de l'échelle
typographique, le rayon des angles, la règle des filets, la nature des nombres,
les teintes de signalisation et leurs sens. **Deux nuances, apprises en réglant
les deux autres salles** : le *jeu* de tailles est du cadran, sa fermeture ne
l'est pas ; les *valeurs* de signalisation s'ajustent par support, leurs teintes
et leurs sens ne bougent pas. »

**d) § 5.2, variable 2 — `:403-408`.** La colonne scène gagne les deux
définitions qui rendaient la règle inapplicable :

> un aplat autorisé — un seul par panneau, toujours porté par **un état** (ce que
> le contrôle *montre*), jamais par une action (ce qu'il *fait*), jamais par
> l'identité. **Un panneau est la plus petite région délimitée par un filet ou
> par un fond propre** — l'œil découpe des bordures, pas des fonctions. Un
> sélecteur exclusif compte pour un aplat : le compte porte sur les surfaces
> visibles en même temps.

Et la dernière phrase de la raison — « c'est déjà, mot pour mot, la règle écrite
dans la maquette de scratchvj » — **est retirée** : elle est fausse, la maquette
dit « une seule action par panneau », et c'est elle qui a produit
`MAISON.md:41`.

**e) § 5.2, variable 7 — `:445`.** La colonne scène gagne, après « hiérarchisée
en trois niveaux, dont un lisible à un mètre » : « — **valeurs non écrites ; voir
`design/SCENE.md`, ce qui reste ouvert.** C'est la seule des sept variables sans
nombre, et un produit ne peut pas se conformer à une colonne vide. »

**f) § 5.3, la mono — `:470-473`.**

Actuel : « …arbitrée sur la disponibilité, pas sur le goût : Fragment Mono n'est
présente nulle part sur le disque ; DM Mono est présente *et* passe le moteur
d'impression de Chrome — qu'Archivo, elle, ne passe pas. Trancher sur ce test. »

Remplacement : « …arbitrée sur le passage au moteur d'impression, pas sur le
goût. **Correction du 2026-09-09** : Fragment Mono *est* sur le disque, embarquée
dans Georama en woff2 avec sa licence OFL (grappe G6) — la disponibilité ne
départage plus. La mesure passée à ce jour compare **Archivo variable** à DM
Mono, pas Fragment Mono à DM Mono. **Le test reste à passer sur les deux bons
candidats**, et c'est une heure. »

**g) § 5.3, le clavier — `:483` et `:486`.** « réserve les siennes (F, 1–5 sur les
pads) » devient « réserve les siennes (`F`, `B`) » ; et la dernière phrase — « Le
conflit déjà noté sur Mutoscope (1–5) montre que ça arrive tout seul » — perd sa
seconde preuve : « Le conflit noté sur Mutoscope (`MUTOSCOPE-01`) montre que ça
arrive tout seul. *Les pads sur `1`–`5` de scratchvj n'existent plus depuis la
refonte de septembre 2026 ; la conclusion tient par Mutoscope seul.* »

**h) Le papier du document lui-même — `:9`, `:21`.** `#0B0D0C` n'est ni la valeur
atelier ni la valeur scène. Il prend la valeur scène de `lines.scene.chassis.void`
le jour où elle existe, et une note le dit d'ici là.

---

## 4. `design/ERGONOMIE.md`

**a) La forme courte, bornée par la place — `:409-416`.** La clause de G12
s'énonce par son critère avant ses trois exemples :

> **Une forme courte n'est pas interdite — elle est bornée par la place.** Jamais
> dans un fichier, une valeur ou une clé. Dans l'interface, seulement là où la
> forme lisible ne tient pas — une étiquette de vignette, une colonne étroite,
> **une cible de 44 px lue à un mètre** — et le libellé complet reste accessible à
> côté. Cela vaut pour les abréviations (`tb`, `sbs`, `ou`,
> `00-vocabulaire.md:93`) comme pour une forme longue tronquée (« 360 » pour
> « Équirectangulaire 360 »). **Ce qui reste interdit partout, c'est un autre
> mot** : `2d` ne devient pas acceptable parce qu'il est court.

**b) La frontière libellé / titre — sous `:404`.**

> **Un libellé est ce qui a une valeur à sa droite ; un titre de panneau est ce
> qui a des libellés en dessous.** Le premier porte la capitale initiale, le
> second peut porter les capitales à l'axe de largeur 125
> (`DIRECTION-ARTISTIQUE.md:219-220`). Un texte qui n'est ni l'un ni l'autre est
> un libellé. La frontière se vérifie mécaniquement ; c'est ce qui la rend
> applicable dans dix produits.

**c) La clause du clavier — `:238-243`**, réécrite critère d'abord (texte donné
en entier dans `SCRATCHVJ-15`, § 2).

**d) La pile d'`Échap` — sous `:290`** : la lecture qui manquait, texte donné
dans `SCRATCHVJ-15`, § 1. Le rang 6 est déclaré non facultatif.

**e) Le compte des touches — `:448`.**

Actuel : « Les **douze** lignes du clavier réservé ont leur effet, et aucun
autre… »

Remplacement : « **Chaque ligne de la table du clavier réservé** a son effet, et
aucun autre… » (le reste de la ligne inchangé).

**f) Le compte des modales — `:367-369`.**

Actuel : « Les trois cas d'aujourd'hui : écraser un fichier existant, et quitter
alors qu'un travail est en cours. Tous deux parce qu'ils sont irréversibles. »

Remplacement : « Les cas d'aujourd'hui : **écraser un fichier existant, quitter
alors qu'un travail est en cours, supprimer définitivement** (la corbeille de
Mutoscope, grappe G9). Tous parce qu'ils sont irréversibles. »

**g) Le tableau des niveaux — `:48-52`.** Il n'a pas de ligne pour la scène, ce
qui fait qu'un produit de scène s'y cherche. Une ligne sous le tableau :

> **Un produit de scène ou de terrain n'a pas de niveau : il a un cadran.**
> `design/SCENE.md` et `design/TERRAIN.md` disent ce qui s'applique et ce qui est
> sans objet. Les niveaux A / B / C mesurent ce qu'un toolkit d'atelier laisse
> atteindre ; ils ne mesurent pas une salle.

**h) La liste de contrôle — `:441-458`.** Deux de ses lignes sont **fausses pour
un produit de scène**, par les variables 1 et 2 du cadran lui-même : `:442`
(« Aucune couleur ne remplit un aplat ni un fond de bouton ») et `:445` (« Aucune
taille de texte hors de 11 / 12 / 13 / 16 / 28 »). Un produit qui passe le cadran
échoue à la liste. Les deux lignes reçoivent leur lecture de salle — *à l'atelier
aucun aplat ; à la scène un par panneau, porté par un état* ; *aucune taille hors
du jeu fermé de la ligne* — et `design/SCENE.md` porte la liste complète pour la
scène.

---

## 5. `design/DIRECTION-ARTISTIQUE.md`

**a) Un jeu de signalisation par support — sous `:117`.** Texte donné dans
`SCRATCHVJ-09`.

**b) L'étiquette d'un ou deux caractères — à `:117`**, sur la ligne du `fail`.
Texte donné dans `SCRATCHVJ-10`. Elle sert immédiatement à `MUTOSCOPE-16`.

**c) Trouvé en chemin — `:131-139`, le tableau des accents.** Il porte **sept**
produits ; `tokens.json:37-47` en porte huit et `maison/05-QUI-EST-QUI.md:19`
donne à Mutoscope `#8E9BC4`. **La ligne Mutoscope manque.** Rien à arbitrer, la
valeur existe dans le fichier qui fait foi.

---

## 6. `spec/00-vocabulaire.md`

**a) Une section « Ce qui n'est pas déclaré »**, après les projections. Texte
donné dans `SCRATCHVJ-01`.

**b) Une section « Reprojections »**, à côté de « Projections », avec les trois
formes, une colonne **Domaine** de champ par forme, et la borne explicite : *ce
sont des identifiants de code et des libellés ; aucun produit ne les écrit sur
disque à ce jour, et la règle de `:5-8` ne les atteindra que le jour où un pont
les transporte.* Texte donné dans `SCRATCHVJ-02`.

## 7. `spec/01-manifeste-plate.md`

La clause du rang absent, à côté de l'échelle des rangs. Texte donné dans
`SCRATCHVJ-18`.

## 8. `spec/03-ponts.md`

**a) `:133-138`** — une phrase après `plate-view/1` : la ligne ne dit pas dans
quelle reprojection le champ est exprimé, un champ `view=` de valeur par défaut
`rectilinear` s'y ajoutera le jour où un second produit sait le produire.

**b) Trouvé en chemin — `:117`** : « Deux commandes, deux raccourcis, dans **sept
des huit** produits ». C'est un décompte dans une source, ce que
`maison/00-LIRE-DABORD.md:179-188` interdit depuis G10. **On nomme les sept.**

## 9. `design/NOMS.md`

**a) `:217-227`** — les deux lignes données dans `SCRATCHVJ-19`, et le titre
« JEU ARRÊTÉ » qui cesse de laisser croire que la table ne porte que des noms
arrêtés.

**b) Trouvé en chemin — `:222`** : « Anamorphe | 21 effets GPU ». G10 a mesuré
**78** et l'a corrigé ailleurs ; la ligne descriptive de `NOMS.md` est restée. Et
c'est un décompte : la ligne descriptive n'a pas besoin du nombre.

## 10. `tools/gen-maison.py`

- la ligne « et c'est déjà, **mot pour mot**, la règle de `design/README.md` » →
  supprimée ; la règle du cadran est citée seule, avec le critère *état* et la
  définition du panneau ;
- « les pads sur `1`–`5` n'entrent en conflit avec rien de réservé » → supprimée :
  l'interface n'existe plus ;
- « Vérifier qu'aucune transition ImGui n'a été laissée à sa valeur par défaut »
  → « Vérifier qu'aucune animation d'interface n'existe » ;
- le renvoi de la ligne scène pointe vers `design/SCENE.md`.

**Le `MAISON.md` de scratchvj est à régénérer et à transporter** — c'est le seul
des dix pour ce tour, plus ceux que `tokens.json` v3 et `SCENE.md` obligent à
relire (tous, mais sans changement de contenu propre).

## 11. `maison/05-QUI-EST-QUI.md`

`secteurs.html:258`, `:274-275` et `:667` affirment trois fois que **le second
produit de scène existe**. `05-QUI-EST-QUI.md` fait foi pour la place de chaque
produit et n'en connaît qu'un. **Deux verdicts de ce tour (`SCRATCHVJ-05`,
`SCRATCHVJ-15`) reportent explicitement jusqu'à lui.** Il se nomme dans la table,
ou la phrase se retire de `secteurs.html` — mais pas les deux.

## 12. `remontees/REGISTRE.md`

- **vingt lignes** pour scratchvj ;
- **trois affectations de grappe corrigées** : `SCRATCHVJ-01` n'est pas dans G5
  (le repère n'a pas été reposé), `SCRATCHVJ-20` n'est pas dans G1 (le nom de
  maison n'a pas été reposé — c'est G10), et **`SCRATCHVJ-23` n'existe pas** : ce
  produit a vingt remontées, et la colonne « identifiants émis » du registre le
  dit déjà (`01`–`20`), ce qui rend la ligne G10 contradictoire avec sa propre
  table d'en-tête. **`GRAPPES-01-reponses.md` porte la même erreur en toutes
  lettres** — G1 « *posée par […] scratchvj* » et G5 « *posée par […] scratchvj* »
  — et se corrige avec : ce produit n'a reposé ni le nom de maison ni le repère,
  et il a écrit pourquoi (`REMONTEES-SCRATCHVJ.md:27-34`) ;
- **G6** gagne `SCRATCHVJ-06` et **G7** garde `SCRATCHVJ-18` ; **G4** gagne
  `SCRATCHVJ-20` ; **G12** gagne `SCRATCHVJ-03` ;
- **quatre points ouverts nouveaux** dans la table « ce qui reste ouvert » :

| Point ouvert | Ce qui manque pour trancher | Qui doit le produire |
|---|---|---|
| Les cinq crans de l'échelle de scène | la **règle de dérivation** entre salles, écrite une fois — celle que `TERRAIN.md:83` revendique et n'applique pas | la maison, en écrivant `design/SCENE.md` |
| Les trois étages de la variable 7 | les valeurs, et **un second produit de scène** pour les éprouver | la maison, au tour 01 du second — après l'avoir nommé |
| La mono de la ligne scène | le moteur d'impression de Chrome, sur **Fragment Mono contre DM Mono** — une heure | scratchvj : seul produit avec une chaîne PDF réelle |
| Le clavier réservé de la ligne scène | un second produit ; trois candidates connues (`Espace`, `F`, `Échap`) | la maison, au tour 01 du second |

Et une ligne à rayer : la famille de chasse fixe, restée ouverte « pour la seule
ligne scène », est désormais **la mono de la ligne scène** ci-dessus, avec son
test corrigé.
