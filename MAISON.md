# scratchvj — le talon

Généré le 2026-09-09 par `Suite 360/tools/gen-maison.py`. **Ne pas éditer à la main** : s'il est faux, c'est le script qu'on corrige — sinon la prochaine génération écrase la correction, et personne ne sait pourquoi le défaut est revenu.

Ce fichier ne porte que ce qui est propre à **scratchvj**. La loi de la maison n'y est pas recopiée : elle se lit à son adresse, et c'est ce qui garantit qu'elle est à jour. Les numéros d'écart renvoient à `design/DIVERGENCES.md`.

## À lire avant ce fichier, et avant le code

**`C:\Users\dimit\Documents\CODE\Suite 360\maison`** — les six documents ci-dessous, à lire en une fois :

| | |
|---|---|
| `00-LIRE-DABORD.md` | l'adresse, les deux étages, les sources dans l'ordre |
| `01-LES-TROIS-COUCHES.md` | **avec quelle confiance** appliquer chaque règle |
| `02-INTERDITS.md` | ce qui ne se fait dans aucun produit |
| `03-DEFINITION-DE-FINI.md` | à quoi on reconnaît que c'est terminé |
| `04-CE-QUI-EST-OUVERT.md` | ce qui ne se décide pas ici |
| `05-QUI-EST-QUI.md` | qui est qui, les salles, les chaînes, les collisions |

**Aucun de ces six documents n'est recopié dans ce dépôt.** Si tu lis une règle de la maison depuis un fichier situé ici, tu lis une copie, et une copie a un jour de retard. Le seul fichier de la maison qui vit ici est celui-ci, et il ne contient aucune règle générale.

Ce produit est de la **scène** : sa liste de sources n'est pas celle de l'atelier. Voir `00-LIRE-DABORD.md`, section « si le produit est de la scène ».

## Ce que ce produit est

| | |
|---|---|
| Produit | **scratchvj** |
| Dossier | `C:\Users\dimit\Documents\GitHub\UE_SCRATCHING` |
| Salle | **scène** — il y a un public dans la pièce : le travail est immédiat, irréversible, et ne produit aucun fichier |
| Nature | **logiciel** — il possède sa fenêtre, son clavier, son thème et son cycle de vie |
| Nom | **provisoire.** `scratchvj` décrit la fonction — indéposable, même défaut que *Relief*. Ne rien publier sous ce nom ; la recherche d'un nom court dans le registre des appareils de l'image animée d'avant le cinéma est en cours. Rien à renommer dans le code tant que le nom n'existe pas |
| Rôle | instrument DJ vidéo scratchable au timecode DVS. **Premier produit de la scène** — la salle où il y a un public |
| Toolkit | C++ ; SDL3 + Dear ImGui + bgfx pour la fenêtre. Une suite de tests fournie, des shaders tenus à une référence CPU (`*_check`), et une CI sur trois systèmes. *Le compte des tests ne s'écrit pas ici : la commande de test le donne, et elle ne se périme pas* |
| Niveau de conformité | **le cadran** (`docs/secteurs.html`, § 5) : l'invariant de maison s'applique en entier ; sept variables changent, chacune pour une raison écrite. Les niveaux A / B / C d'`ERGONOMIE.md` ne s'appliquent pas tels quels |
| Accent | orange `#C9762F` — **candidat**, absent de `tokens.json`. Plus la paire opératoire A / B : ambre `#C99A2F`, ardoise `#6E8696` — qui doit devenir une constante de toute la ligne scène |
| Licence | **GPL-3, assumée** (décision du 2026-09-09). xwax reste dans `scratchvj/dvs/`, `core/` reste sans dépendance, le plugin Unreal reste hors GPL par construction |
| Git | oui |

## Ce qui est déjà juste ici — ne pas le casser

*Cette liste protège ce qui existe, elle n'interdit rien d'autre.* Au tour 01, plusieurs de ses lignes décrivaient un état qui n'existait plus — un réglage « partagé » qui avait été supprimé, une convergence mesurée sur le signe et non sur le geste — et **une session lisait « ne pas casser » là où il y avait du travail à faire**. Une ligne d'ici qui ne se retrouve pas sur le disque n'est pas une protection : c'est un défaut de ce fichier, et il se remonte.

- **L'aplat orange n'est pas une infraction.** C'est la variable 2 du cadran : *un aplat autorisé, un seul par panneau, toujours porté par un **état**, jamais par l'identité.* Le critère est l'état, et un panneau est une surface qui ne bloque pas le reste de l'interface. Ne pas l'« aligner » sur l'atelier.
- **Le corps à 15 px** (`main_ui.cpp`) est la variable 1 du cadran, pas un écart : on jette un œil à la scène pendant qu'une main tient le plateau.
- Archivo pour les mots. Les contrôles fantômes (potards absolus à position inconnue, dessinés en pointillé au lieu d'une valeur inventée) sont un détail que seul ce métier connaît — exactement ce que la DA demande, un par écran.
- Le manifeste de la ligne est **déjà écrit**, dans le README : (1) tout ce qui se scratche est une *fonction de la position*, jamais un intégrateur ; (2) l'audio agit sur les fréquences temporelles d'un signal 1D, la vidéo sur les fréquences spatiales d'un 2D. Il n'est simplement pas rangé.
- La frontière `dvs/` — visible dans le code, documentée en tête de `decoder.h`. C'est le motif à réemployer partout où une dépendance impose sa licence.

## Les écarts déjà relevés dans ce produit

*Déjà relevés, pas tous.* `DIVERGENCES.md` est un relevé daté du 2026-09-08 qui a porté sur les couleurs, les rayons, les fontes et le clavier — **jamais sur les constantes du viseur**, par exemple. Une phase qui transcrit `tokens.json` le vérifie **jeton par jeton**, pas d'après cette liste. Et dans l'autre sens : un écart de cette liste qu'on ne retrouve pas sur le disque **se note comme introuvable, avec la commande passée** — c'est un résultat, pas un échec de recherche.

- **La mono** — DM Mono ici, Fragment Mono dans `tokens.json`. Deux monos, c'est deux maisons — mais **la mono unique de la maison n'est pas tranchée** (les deux familles de la maison sont sur le disque — Archivo et Fragment Mono sont embarquées dans Georama en woff2 avec leurs licences OFL, et Archivo en variable ici ; ce qui n'est pas tranché, c'est **DM Mono contre Fragment Mono pour la scène**, DM Mono passant le moteur d'impression de Chrome qu'Archivo ne passe pas). **Ne rien changer** : c'est de la couche 3. Préparer le changement en un endroit.
- **La luminance du châssis** — `#141412` contre `#0A0C0B`. La *teinte* est du cadran (variable 3 : noir chaud à la scène, noir verdi froid à l'atelier — même luminance, teinte seule différente) ; l'écart de *valeur* n'en est pas. La valeur commune n'est pas arrêtée. **Ne rien changer** ; isoler la valeur en un endroit.
- **La feuille des documents** — `docs/pdf/_style.css` propre au produit, contre `suite.css`. Cible : *une feuille, deux papiers* (clair pour l'atelier, sombre pour la scène — variable 6). **La note est rendue** (`SCRATCHVJ-17`) : le papier sombre sera un bloc de redéfinition de variables sous une classe de ligne, pas une seconde feuille ; il déclare qu'il ne s'imprime pas ; `--paper-sunk` s'y **assombrit** au lieu de s'éclaircir, et les quatre signaux s'éclaircissent. Il **attend deux valeurs qui ne sont pas tranchées** — la mono unique de la maison et la luminance commune des deux châssis. **Ne pas réécrire `_style.css` d'ici là.**
- **Le clavier** — `F` (image seule) est une des quatre lettres que la suite s'interdit de réserver (`F`, `H`, `O`, `I`), donc libre pour le produit. Le **noyau** réservé, lui, doit tenir : `Espace`, `Échap` selon la pile de priorité, `Ctrl`+`Z`, `?` / `F1`, `Tab`.
- **Les jetons** — les constantes vivent dans `apply_style()` (`panels.cpp`). Cible : un fichier unique de constantes, provenance en tête (*« ligne scène, candidat, en attente de `tokens.json` v3 `lines.scene` »*), sans retouche des valeurs.
- **Le vocabulaire 360** — `core/sphere` reprojette (perspective, little planet, fisheye) et le produit lit de la vidéo sphérique. Partout où une projection ou une disposition est nommée ou écrite (`.svcache`, `library.json`, `mapping.json`) : forme canonique de `00-vocabulaire.md`.

## Les pièges propres à ce produit

- **Ne pas aligner scratchvj sur la DA de l'atelier au motif de la cohérence.** Trois de ses écarts sont *justes* et doivent devenir des règles (l'aplat, le corps, le mouvement) ; deux seulement sont des accidents (la mono, la luminance) — et ces deux-là attendent une décision de la maison. Aligner sans trier détruirait ce qui rend l'instrument jouable dans le noir.
- **Le mouvement** (variable 5) : 0 ms par défaut à la scène ; la seule animation autorisée est celle qui est calée sur le tempo. Un changement d'état d'interface qui s'anime prend de l'attention à l'image projetée. Vérifier qu'**aucune animation d'interface n'existe**.
- **Le viseur 360**, là où il est manipulé à la souris (mode 360, plein cadre) : *on tire l'image* — `−Δx` au lacet, `+Δy` au tangage, tangage borné à `±89.9°`, pas d'inertie. Là où il est piloté par un potard, la convention ne s'applique pas.
- **La GPL se déclare dans les deux sens** : un utilisateur de scène doit lire qu'il a droit à la source — c'est un argument, pas un aveu. Vérifier que le README et la vitrine le disent, et que rien ne prétend vendre autre chose que les builds, les mises à jour et le fait que ça marche le soir même.
- **Le second produit de scène existe et n'est pas encore montré.** La frontière entre les deux s'écrira alors, sur le motif de `04-frontieres.md` : *la ligne passe par ce que chacun produit, et tient par deux interdits explicites.* Rien à anticiper dans le code, sauf ne pas fermer une entrée (Spout / NDI / UDP) qui pourrait être son pont.

## Ce que les ponts et les frontières attendent de ce produit

*Lire le temps des verbes.* Ce qui est écrit au **présent** existe et ne se casse pas ; ce qui est écrit au **futur** n'existe pas encore, et c'est une capacité à écrire, pas un manquement. Écrire au présent ce qu'un produit ne fait pas conduit à ne jamais l'écrire, puisque personne ne le voit manquer.

- Sort en Spout et en UDP à 375 Hz vers Unreal, où vit Diorama : **le seul endroit du catalogue où les deux salles se touchent.** Rien à construire — ne pas le rendre impossible.
- Aucun manifeste de plate à lire ou écrire : le manifeste concerne des fichiers, et la scène ne produit qu'un moment.

## Le travail, dans l'ordre

Une phase à la fois. Chaque phase se termine par un rapport court — *fait / non fait et pourquoi / ce qui contredit la maison ou ses sources* — et attend qu'on dise « phase suivante ».

**Phase 0 — le relevé, avant de toucher à quoi que ce soit.** Écrire
`docs/ALIGNEMENT.md` (créer `docs/` s'il n'existe pas) : chaque chaîne qui nomme
une projection ou une disposition, chaque touche liée et son effet, chaque
valeur de couleur, de rayon, de fonte et de taille écrite dans le code, chaque
feuille de style documentaire — **avec `fichier:ligne` pour chaque
occurrence**. Puis confronter au relevé de `DIVERGENCES.md` : ce qui y est et
qu'on ne retrouve pas, ce qu'on trouve et qui n'y est pas. **S'arrêter là et
rendre compte.** Aucun fichier de code ne se modifie avant que le relevé ait
été lu.

**Phase 1 — le manifeste.** Extraire les deux principes du README dans `docs/manifeste.md`, dans les termes du README, sans les réécrire. C'est le `01-manifeste` de la ligne scène.

**Phase 2 — le vocabulaire**, en deux moitiés : les formes canoniques de la 360 partout où une projection ou une disposition est écrite ; et `docs/vocabulaire.md` pour les mots propres à la scène qui existent déjà dans le code (deck, surface, contrôle fantôme, verrou, prise, take, ancre, programme…), un mot par concept, la forme canonique en `snake_case`.

**Phase 3 — le repère temporel.** `docs/repere.md` : *qui fait autorité quand ils divergent — le tempo, l'horloge audio, ou la position du plateau ?* Les trois existent (`core/timecode`, `core/anchor`, `core/modulator`) et rien n'écrit lequel gagne. **Écrire les options et leurs conséquences, recommander, ne pas trancher** : c'est la décision à plus fort levier de la ligne, et elle est au fondateur. C'est le seul document de la liste qu'il faut écrire même si aucun second produit n'arrivait jamais.

**Phase 4 — le noyau du clavier** : `Espace`, la pile d'`Échap` (qui ne ferme jamais la fenêtre et ne quitte jamais « image seule » — `F` s'en charge ici, comme `Tab` à l'atelier), `Ctrl`+`Z`, `?` / `F1`, `Tab` s'il a un sens. Le viseur 360 à la souris.

**Phase 5 — les jetons** : le fichier unique extrait d'`apply_style()`, provenance en tête, valeurs inchangées ; la mono et la luminance isolées en un endroit chacune, prêtes à changer.

**Phase 6 — la licence dans les deux sens** : README et argumentaire.

**Phase 7 — le papier sombre : déjà rendue.** La note que cette phase demandait a été écrite et reçue le 2026-09-09 (`SCRATCHVJ-17`). Il ne reste rien à faire ici : la ligne reste en *remonté* jusqu'à ce que le thème existe dans `docs/suite.css`, ce qui attend deux décisions de la maison. **Ne pas la refaire, et surtout ne pas écrire la feuille en attendant.**

## Ce qui se décide ailleurs — en plus de `maison/04`

Les points ouverts communs aux huit sont dans `04-CE-QUI-EST-OUVERT.md`, qui renvoie au registre. Ceux-ci sont propres à scratchvj, et se remontent de la même façon.

- **La mono unique de la maison** (DM Mono ou Fragment Mono), arbitrée sur la disponibilité et l'impression, pas sur le goût.
- **La luminance commune des deux châssis.**
- **`tokens.json` v3, `lines.scene`** — ce dépôt fournit les valeurs candidates (phase 5), la maison les entérine.
- **Le nom du produit** — **délégué** par le fondateur, contrairement au nom de maison qui reste à lui. Contraintes déjà écrites : pas un mot qui décrit la fonction (motif absolu de refus au dépôt), vérifiable en antériorité, et — c'est la leçon de *Theama* — le fondateur doit le retenir.
- **Le nom de la ligne scène elle-même**, en tant que suite d'outils temps réel. Délégué avec le précédent ; `NOMS.md` ne porte aujourd'hui aucune ligne ni pour l'un ni pour l'autre.
- **Le repère temporel** — recommandé ici, tranché par le fondateur.
- **La frontière avec le second produit de scène**, quand il sera montré.

## Définition de fini — en plus de `maison/03`

Les critères communs aux huit sont dans `03-DEFINITION-DE-FINI.md`. S'y ajoutent, pour scratchvj :

- `docs/manifeste.md`, `docs/vocabulaire.md` et `docs/repere.md` existent ; le troisième expose des options et une recommandation, pas une décision.
- L'aplat orange est toujours là, et il n'y en a toujours qu'un par panneau.
- Aucune couleur, taille ou fonte hors du fichier unique de constantes ; la mono et la luminance changent en une ligne chacune.
- La suite de tests passe, `*_check` des shaders compris — et le rapport de phase dit **laquelle** a été passée, pas combien de tests elle contient.
