# Le manifeste de la ligne scène

Écrit le 2026-09-10, phase 1 du talon (`MAISON.md`, « Le travail, dans
l'ordre »). C'est le `01-manifeste` de la ligne scène, au sens où
`spec/01-manifeste-plate.md` est celui de la suite 360 : ce que tout produit
de la ligne tient pour acquis, écrit une fois.

**Rien n'est rédigé ici.** Les deux principes existaient déjà, dans le `README.md`
de ce dépôt, et ils y étaient depuis le premier commit ; ils n'étaient simplement
pas rangés. Ils sont repris **dans les termes du README**, sans réécriture. Le
README reste l'endroit où ils vivent ; ce fichier est l'endroit où on les cite.

---

## Les deux principes

Tels que le README les écrit, sous « *Why this works at all* » :

> Two design principles run through everything:
>
> 1. **Anything scratchable is a *function* of position, never an integrator.** A
>    simulation moves forward and cannot reverse; a stream parameterised by `t`
>    scratches perfectly. Hence the video engine indexes frames rather than playing a
>    stream, and the audio engine is driven by position rather than by rate.
>
> 2. **Audio acts on the temporal frequencies of a 1D signal; video acts on the
>    spatial frequencies of a 2D one.** Where the correspondence is exact it is
>    implemented as such — a low-pass filter really is a blur, a bitcrusher really is
>    posterisation. Where it is not, the nearest perceptual analogue is chosen and
>    documented. See [docs/fx-correspondances.md](fx-correspondances.md).

Et tels que `CLAUDE.md` les porte déjà en français, sous « Deux principes de
conception non négociables » — c'est la seule autre formulation du dépôt, et
elle est reprise telle quelle :

> 1. **Tout ce qui doit être scratchable est une fonction de la position, jamais un
>    intégrateur.** Le moteur vidéo indexe des frames plutôt que de lire un flux ;
>    le timecode se pilote en position, jamais en vitesse.
> 2. **L'audio agit sur les fréquences temporelles, la vidéo sur les fréquences
>    spatiales.** Quand la correspondance est exacte (passe-bas ↔ flou), on
>    implémente le même calcul dans les deux domaines. Quand elle ne l'est pas
>    (reverb ↔ smear), on choisit l'analogue le plus proche et on le marque comme
>    tel dans `core/effect.cpp` — jamais présenté comme identique.
>
> Toute nouvelle fonctionnalité doit respecter ces deux règles.

---

## Où chacun se vérifie

Ce document ne prescrit rien ; il dit où regarder pour constater que le dépôt
tient ce qu'il dit.

| Principe | Ce qui le porte | Ce qui le contredirait |
|---|---|---|
| 1 — fonction de la position | `core/videocache` indexe des images par position ; `core/timecode` suit une position, pas une vitesse ; `core/modulator` — « a synced LFO follows the platter backwards » (README, *Current state*) | un module qui intègre une vitesse pour obtenir une position, ou une lecture qui ne sait pas reculer |
| 2 — temporel ↔ spatial | `core/effect` et son catalogue ; la table des correspondances dans `docs/fx-correspondances.md`, où chaque paire est dite *identique* ou *analogue* ; à l'écran, la colonne qui écrit « identique » ou « ≈ analogue » | un effet présenté comme identique alors que sa correspondance n'est qu'analogue |

Le second principe a une conséquence d'affichage que ce produit tient déjà :
l'analogie est **dite**, elle n'est pas dissimulée dans un nom d'effet. C'est
le seul endroit du manifeste qui touche l'interface.

---

## Ce que ce manifeste n'est pas

- **Pas le repère temporel de la ligne.** *Qui fait autorité quand ils
  divergent — le tempo, l'horloge audio, ou la position du plateau ?* est la
  phase 3 (`docs/repere.md`), et la décision revient au fondateur. Le premier
  principe dit que tout se pilote en position ; il ne dit pas quelle horloge
  fournit cette position quand plusieurs existent.
- **Pas le vocabulaire de la scène.** Les mots propres à la ligne — deck,
  surface, contrôle fantôme, verrou, prise, ancre, programme — sont la phase 2
  (`docs/vocabulaire.md`).
- **Pas un manifeste de plate.** La scène ne produit aucun fichier ; il n'y a
  rien à faire voyager d'un produit à l'autre, et `spec/01-manifeste-plate.md`
  ne s'applique pas ici (`MAISON.md`, « Ce que les ponts et les frontières
  attendent de ce produit »).
