# scratchvj

Instrument DJ vidéo scratchable, piloté par timecode DVS (MWM Phase, Reloop
Elite, RP-8000 MK2). Unreal Engine est un **client** de cette app, pas son cœur.

**Avant de faire quoi que ce soit, lire [`docs/roadmap.md`](docs/roadmap.md)** :
c'est le document qui fait foi sur l'état d'avancement, ce qui reste à faire, et
pourquoi chaque décision de conception a été prise. Ce fichier `CLAUDE.md` ne fait
que résumer les conventions ; le roadmap contient le raisonnement.

## Deux principes de conception non négociables

1. **Tout ce qui doit être scratchable est une fonction de la position, jamais un
   intégrateur.** Le moteur vidéo indexe des frames plutôt que de lire un flux ;
   le timecode se pilote en position, jamais en vitesse.
2. **L'audio agit sur les fréquences temporelles, la vidéo sur les fréquences
   spatiales.** Quand la correspondance est exacte (passe-bas ↔ flou), on
   implémente le même calcul dans les deux domaines. Quand elle ne l'est pas
   (reverb ↔ smear), on choisit l'analogue le plus proche et on le marque comme
   tel dans `core/effect.cpp` — jamais présenté comme identique.

Toute nouvelle fonctionnalité doit respecter ces deux règles.

## Build et tests

```sh
cmake -S . -B build && cmake --build build
ctest --test-dir build --output-on-failure    # 329 tests, doivent tous passer
./build/scratchvj/scratchvj demo              # démo sans matériel
./build/scratchvj/scratchvj effects           # catalogue d'effets
./build/scratchvj/scratchvj layout            # checklist MIDI learn
```

Compiler avec gcc **et** clang avant de pousser (`-DCMAKE_CXX_COMPILER=clang++`) :
la CI tourne sur Linux, macOS et Windows à chaque push, mais les deux compilateurs
locaux attrapent déjà l'essentiel des warnings avant même d'y arriver.

## Conventions du code

- **C++20**, zéro avertissement (`-Wall -Wextra -Wpedantic` / `/W4 /permissive-`).
- **`scratchvj_core` n'a aucune dépendance externe.** C'est délibéré : ça permet
  à la logique d'être testée intégralement sans matériel, sur les trois OS, dès
  le premier commit. Ne pas introduire de dépendance dans `core/` sans y réfléchir
  à deux fois — les dépendances lourdes (FFmpeg, bgfx, RtMidi, miniaudio) sont
  prévues pour des couches séparées, pas encore écrites (voir le roadmap).
  `config/mapping_io` est la seule exception déjà en place, isolée exprès pour
  que la bibliothèque JSON ne se compile qu'une fois.
- **Un test par comportement, pas par fonction.** Regarder les fichiers
  `tests/test_*.cpp` existants pour le style : chaque test a un nom qui décrit ce
  qu'il vérifie en une phrase, et les tests qui capturent un piège trouvé en
  cours de route disent explicitement pourquoi dans un commentaire.
- **Écrire le test qui aurait attrapé le bug, pas juste corriger le bug.** Le
  projet a trouvé quatre défauts réels en construisant la démo d'intégration
  (voir le roadmap et l'historique des commits) : seuil de saut de timecode fixe,
  reprise après coupure comptée comme un saut, lacet 360 inversé, arrondi de
  frame à 29,97 fps. Dans chaque cas le correctif est venu avec un test qui
  énonce la propriété violée, pas juste un chiffre magique changé.
- **Documenter le pourquoi, pas le quoi.** Le code est commenté pour expliquer
  une contrainte cachée ou une décision non évidente (voir n'importe quel fichier
  de `core/` pour le ton), jamais pour paraphraser ce que fait la ligne suivante.

## Repo

- Branche de travail : `claude/scratch-video-unreal-0oi7dv`. `main` est une
  branche de base quasi vide, créée uniquement pour que GitHub ait un point de
  comparaison pour les pull requests — ne pas la confondre avec une branche
  stable.
- PR en cours : [#1](https://github.com/Anadyomene30/UE_SCRATCHING/pull/1).
- `design/` contient les fichiers source de la maquette d'interface (canvas
  Claude Design). Le fichier assemblé (`maquette-scratchvj.html`, ~2,5 Mo) est
  ignoré par git — c'est un artefact généré, voir `design/README.md` pour le
  régénérer.
- `docs/` contient tout le raisonnement de conception : `roadmap.md` (la feuille
  de route complète), `cablage.md` (branchement et modes audio), `protocole.md`
  (format réseau vers Unreal), `format-cache.md` (format `.svcache`),
  `fx-correspondances.md` (table des correspondances audio/vidéo).

## Ce qui reste à faire

Résumé — le détail et l'état module par module sont dans
[`docs/roadmap.md`](docs/roadmap.md). Tout ce qui suit a besoin de matériel réel
ou d'une dépendance lourde pour être vérifiable, donc rien n'a été écrit à
l'aveugle :

- Le décodeur de timecode `timecoder.c` (xwax, GPL-3)
- Un vrai backend MIDI (RtMidi) et audio (miniaudio/ASIO)
- La passe d'analyse vidéo FFmpeg → `.svcache`
- Le rendu GPU (bgfx). L'interface Dear ImGui existe (`ui/`), mais elle ne
  dessine encore aucune image — il n'y a pas de frame décodée à afficher
- Les sorties Spout/Syphon/NDI
- Le plugin Unreal `ScratchLink`

**Deux tests qui reviennent à l'utilisateur, devant le matériel** (voir le
roadmap pour le détail) :
1. Est-ce que l'Elite émet son état MIDI à la connexion ?
2. Est-ce que le second port USB de l'Elite reçoit le timecode en parallèle de
   Serato ?

Ne pas commencer une nouvelle brique sans avoir lu la section correspondante du
roadmap — plusieurs choix (le profil `wireless` du timecode, la fraîcheur plutôt
que la dérive côté ancrage, la courbe de crossfader) viennent de raisons non
évidentes qu'il serait facile de défaire par inadvertance.
