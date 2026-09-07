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
ctest --test-dir build --output-on-failure    # 434 tests, doivent tous passer
./build/scratchvj/scratchvj demo              # démo sans matériel
./build/scratchvj/scratchvj effects           # catalogue d'effets
./build/scratchvj/scratchvj layout            # checklist MIDI learn
```

Avec l'interface (nécessite un GPU, donc jamais en CI) :

```sh
cmake -S . -B build-ui -DSCRATCHVJ_BUILD_UI=ON && cmake --build build-ui --config Release
ctest --test-dir build-ui -C Release --output-on-failure
./build-ui/scratchvj/ui/Release/scratchvj_ui.exe --live   # deck A sur le vrai plateau (MOTU 5/6)
```

`--live` démarre le deck A sur le Phase réel via `core/quadrature` ; l'état du
plateau s'écrit chaque seconde dans `platter.log`. **Lire ce fichier, pas
stderr** : `scratchvj_ui` est une application WIN32 sans console, son stderr ne
va nulle part même redirigé — toutes les tentatives de le lire sont revenues
vides avant que ce soit compris.

Ce `ctest`-là ajoute les cinq outils qui tiennent les shaders à leur référence
CPU : `gpu_check`, `sphere_check`, `eye_check`, `fx_check`, `taps_check`. **Les
lancer par `ctest` et non à la main** — lancés à la main ils ont déjà passé depuis
un binaire périmé alors que leur source ne lisait plus rien. Restent en dehors :
`spout_check` et `net_check`, qui écoutent un `scratchvj_ui` en cours
d'exécution, et `xr_check`, qui rapporte ce que la machine offre en OpenXR (son
verdict dépend du matériel branché, pas du code).

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
- Le rendu GPU est complet et vérifié : compositeur (`fs_program.sc` / `gpu_check`),
  360 (`fs_view360.sc` / `sphere_check`), effets une-frame (`fs_effects.sc` /
  `fx_check`) et multi-taps (`fs_taps.sc` / `taps_check`). La FFT audio-réactive
  est faite aussi (`core/spectrum`) et attend une vraie entrée audio ; restent
  les entrées live.
- Les sorties Syphon (macOS) et NDI — Spout est fait et vérifié (ui/share, spout_check)
- La **session** OpenXR pour voir l'équirect scratché dans le Quest. La géométrie
  est faite et vérifiée (`core/headset`, `fs_view360_eye.sc`, `eye_check`) ;
  restent la swapchain et la boucle de frame, qui ne s'exercent pas sans casque
  réveillé — voir la section « Le casque » du roadmap avant d'y toucher
- Le test en scène du plugin Unreal `ScratchLink` — il compile contre UE 5.7 et le flux UDP est vérifié, mais personne n'a encore scratché une scène avec

**Deux tests devant le matériel** (voir le roadmap pour ce qui est déjà établi).
Le matériel est branché et deux sondes existent :

```sh
./build-ui/scratchvj/ui/Release/midi_probe.exe all 45      # puis balayer tout
./build-ui/scratchvj/ui/Release/audio_probe.exe all 3        # en tournant un plateau
./build-ui/scratchvj/ui/Release/audio_probe.exe selftest    # le detecteur, sans materiel
```

1. Est-ce que l'Elite émet son état MIDI à l'ouverture du port ? (`midi_probe`)
2. Sur quelle entrée arrive le timecode, et est-elle lisible en partagé ?
   **Il n'y a pas de disque de contrôle** : le Phase synthétise le signal à
   partir du mouvement de la remote, donc il faut faire tourner un plateau
   pendant tout le balayage. Si le signal arrive sur un point WASAPI ouvrable en
   partagé, la voie « WASAPI partagé » est ouverte et la deuxième machine tombe.

Ne pas commencer une nouvelle brique sans avoir lu la section correspondante du
roadmap — plusieurs choix (le profil `wireless` du timecode, la fraîcheur plutôt
que la dérive côté ancrage, la courbe de crossfader) viennent de raisons non
évidentes qu'il serait facile de défaire par inadvertance.
