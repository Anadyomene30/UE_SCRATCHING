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
ctest --test-dir build --output-on-failure    # 603 tests, doivent tous passer
./build/scratchvj/scratchvj demo              # démo sans matériel
./build/scratchvj/scratchvj effects           # catalogue d'effets
./build/scratchvj/scratchvj layout            # checklist MIDI learn (tout le rig)
./build/scratchvj/scratchvj profile export reloop_elite   # un profil intégré, en JSON
./build/scratchvj/scratchvj analyze clip.mp4  # vidéo → .svcache (ffmpeg dans le PATH)
./build/scratchvj/scratchvj scan D:/rushes    # ce que la bibliothèque verrait dans un dossier
```

Sur Windows le générateur est multi-configuration : `ctest --test-dir build`
**sans `-C Debug`** rapporte « Not Run » et sort en échec alors que tout va bien.
La ligne ci-dessus est celle de la CI (Linux/macOS) ; ici c'est
`ctest --test-dir build -C Debug --output-on-failure`.

L'analyse pilote l'**exécutable** ffmpeg (jamais lié) : tout ce que ce ffmpeg
décode passe — H.264, HEVC, ProRes, HAP, DXV, VP9, AV1 vérifiés. Une source avec
alpha sort en **BC3** (`core/bc3`), le reste en BC1 ; une image fixe donne une
frame tenue, une séquence numérotée (`frame_%04d.png`) une cadence imposée.
Dans l'interface, l'import se fait par glisser-déposer ou par les boutons
« Importer… » / « Dossier… » ; les dossiers surveillés, le dossier des caches et
le rig MIDI sont dans `settings.json` (onglet RÉGLAGES) ; ce qui a été décidé sur
les clips (projection 360/plan forcée, caisses) est dans `library.json`. Les
deux fichiers sont ignorés par git.

**Les contrôleurs sont des données** (`core/profile`) : l'Elite et la RP-8000
sont des tables dans `core/profiles_builtin.cpp`, tout autre appareil est un
fichier `profiles/*.json` (APC40 mk2 et Push 2 livrés, `verified: false` tant
qu'un `midi_probe` ne les a pas confirmés — voir `profiles/README.md`). La
surface à l'écran est **dessinée depuis le profil**, jamais à la main ; le rig
ouvre un port par appareil (`ui/midi_rig`) et chaque adresse MIDI porte
l'index de son appareil.

**Un profil peut porter la géométrie de son panneau**, en millimètres réels.
Celle de l'Elite est *mesurée* sur le rendu officiel de Reloop et recoupée
avec le diagramme du manuel : 290 × 400 mm, chaque section à sa place. L'onglet
**TABLE** la dessine en grand ; la bande de la cabine ne garde que ce qu'on
regarde en jouant. Un profil sans géométrie (la RP-8000, l'APC40, le Push) est
dessiné en rangée — **ne pas inventer de coordonnées** : un panneau faux est
pire qu'une liste honnête. Ce que la mesure a corrigé au passage : l'Elite n'a
**pas** de boutons cue par voie (c'est un slider au centre), le sélecteur
d'entrée est sur le dessus, il y a **un** SHIFT, et la face avant porte
**trois** paires courbe/reverse (voie 1, crossfader, voie 2). Les destinations de mapping sont un **registre**
(`core/destinations`) : une cible inconnue est rapportée au `bind()`, jamais
ignorée en silence.

Avec l'interface (nécessite un GPU, donc jamais en CI) :

```sh
cmake -S . -B build-ui -DSCRATCHVJ_BUILD_UI=ON && cmake --build build-ui --config Release
ctest --test-dir build-ui -C Release --output-on-failure
./build-ui/scratchvj/ui/Release/scratchvj_ui.exe          # decks vides, prêt à recevoir
./build-ui/scratchvj/ui/Release/scratchvj_ui.exe --live   # deck A sur le vrai plateau (MOTU 5/6)
./build-ui/scratchvj/ui/Release/scratchvj_ui.exe --demo   # la performance scriptée d'avant le matériel
./build-ui/scratchvj/ui/Release/scratchvj_ui.exe clip.mp4 # comme un glisser-déposer : analyse et charge
./build-ui/scratchvj/ui/Release/scratchvj_ui.exe --output 2  # programme sur l'écran n° 2
./build-ui/scratchvj/ui/Release/scratchvj_ui.exe --screen bibliotheque  # ouvrir sur cet écran
```

**L'interface a six écrans sur une seule rangée d'onglets** — JOUER,
BIBLIOTHÈQUE, EFFETS, TABLE, SORTIE, RÉGLAGES — et trois familles de
contrôles, pas plus : le bouton (une action), le sélecteur segmenté (un choix
exclusif), le basculeur (on/off). Tout ce qui se clique a un fond et un bord ;
un libellé n'est jamais cliquable. **Le deck est un lecteur** (`Deck::play/
pause/stop`, la platine est une source parmi trois) ; le mixer est *entre* les
decks comme l'Elite entre les platines ; les pads sont à l'écran (cues, clips
depuis les banques de `core/matrix`, boucles) et passent par les mêmes
`DeckCommands` qu'un pad MIDI. Les diagnostics DVS vivent dans un tiroir par
deck, avec le **scope de calibration** (`core/scope`) quand une entrée audio
est ouverte : la figure de Lissajous brute, et les trois défauts qu'elle peut
avoir séparément — centre (offset), balance (dB), erreur de phase (diaphonie).
La figure n'est **pas** recentrée ni décimée, pour les raisons dans le roadmap.
Le raisonnement complet sur l'interface est dans la note « L'interface a été
refaite » du roadmap, et la maquette qui la précède dans `design/`.

**L'écran de sortie** se choisit dans l'onglet SORTIE : le programme part en
plein écran sans bordure sur le moniteur choisi (`ui/output_window`), par une
**seconde chaîne d'échange bgfx sur le même device** — c'est la texture que
l'aperçu montre et que Spout publie, sans relecture ni copie par la mémoire
centrale. La géométrie de SORTIE (coins, grille, masque) est **rendue sur
cet écran** par une grille 32 × 32 placée par `core/warp` et `core/mesh`, les
mêmes fonctions que l'aperçu dessine ; Spout reçoit l'image avant. Un écran
d'une autre forme reçoit des **bandes noires**, jamais une image étirée : un
masque ou un corner pin calé sur une image déformée serait faux d'exactement
cette déformation. L'écran est retenu **par son nom** dans
`settings.json` (les index changent dès qu'on branche quelque chose) ; `--output`
est un remplacement ponctuel qui n'écrit pas le fichier. Échap ferme d'abord la
sortie, puis quitte : sortir sur un bureau devant une salle est ce que cet ordre
évite.

**L'interface démarre sans démo.** Les decks sont vides, la liaison Phase dit
« aucune entrée » plutôt qu'un pourcentage inventé, et rien ne prétend être du
360. La performance scriptée (`app/simulation`) reste l'échafaudage qui a permis
de construire l'instrument avant le matériel : elle est derrière `--demo`, avec
ses clips fabriqués, et `Engine::configure` prend un `DemoContent` pour ça.
Un fichier déposé sur la fenêtre (ou passé en argument) est analysé puis **posé
sur le premier deck libre** ; un deck déjà chargé n'est jamais volé.

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
  Claude Design). Le fichier assemblé (`interface-scratchvj.html`, ~2,5 Mo) est
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

- Le MIDI **portable** (RtMidi) : sur Windows l'entrée est réelle (`ui/midi_in.cpp`
  parle winmm, c'est ce que le rig utilise) ; le `#else` du même fichier est un
  stub qui renvoie « aucun port », donc ce qui manque est macOS et Linux.
  L'audio d'entrée existe (`ui/audio_in`, WASAPI
  partagé, un thread de capture) et le plateau est lu en direct par
  `core/quadrature` — le Phase émet une porteuse nue, pas un timecode, donc le
  décodeur xwax (`dvs/`, vendu et testé) sert un vrai disque de contrôle, pas ce
  matériel. Reste la sortie audio (miniaudio/ASIO) pour le mode autonome.
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

**Les deux tests matériels sont répondus** (détail et mesures dans le roadmap).
Les sondes restent utiles pour re-vérifier après un changement de câblage :
Le matériel est branché et deux sondes existent :

```sh
./build-ui/scratchvj/ui/Release/midi_probe.exe              # les ports vus, sans rien ouvrir
./build-ui/scratchvj/ui/Release/midi_probe.exe all 45      # puis balayer tout
./build-ui/scratchvj/ui/Release/audio_probe.exe all 3        # en tournant un plateau
./build-ui/scratchvj/ui/Release/audio_probe.exe selftest    # le detecteur, sans materiel
./build-ui/scratchvj/ui/Release/input_check.exe             # l'appli peut-elle ouvrir la porteuse ?
```

`input_check` pose exactement la question que pose `--live` (même `settings.json`,
même classe `AudioInput`) mais **depuis une console** : `scratchvj_ui` n'en a pas,
donc c'est le seul endroit d'où l'on voit pourquoi une entrée refuse de s'ouvrir.
Un fragment de nom peut désigner plusieurs entrées — « MOTU » en désigne deux ici —
et c'est `core/endpoint` qui choisit celle qui porte vraiment la paire de voies.

1. ~~Est-ce que l'Elite émet son état MIDI à l'ouverture du port ?~~ **Non**
   (mesuré : 41 contrôles au balayage, 7 messages au démarrage et ce sont des
   encodeurs au repos). **Le mode fantôme reste nécessaire.**
2. ~~Sur quelle entrée arrive le timecode ?~~ **Répondu** : MOTU voies 5/6,
   1000 Hz, lu en WASAPI partagé pendant que Serato tourne. Et ce n'est pas un
   timecode mais une **porteuse nue** (direction + vitesse, pas de position) —
   d'où `core/quadrature`. Tout est dans `docs/cablage.md`.

Ne pas commencer une nouvelle brique sans avoir lu la section correspondante du
roadmap — plusieurs choix (le profil `wireless` du timecode, la fraîcheur plutôt
que la dérive côté ancrage, la courbe de crossfader) viennent de raisons non
évidentes qu'il serait facile de défaire par inadvertance.
