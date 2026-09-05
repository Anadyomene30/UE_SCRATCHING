# Feuille de route

Ce document existait jusqu'ici uniquement comme plan de session Claude Code, donc
nulle part sur GitHub. Il est déplacé ici pour que tout ce qui a été décidé et
tout ce qui reste à faire survive à la session qui l'a produit.

## Où on en est, module par module

| Jalon du plan initial | État | Modules |
|---|---|---|
| 1. Voir la table | Logique faite, matériel réel non branché | `core/surface`, `core/learn`, `core/layout`, `core/mapping`, `core/protocol` |
| 2. Suivre le timecode | Logique faite (dont le profil `wireless`), décodeur xwax non intégré | `core/timecode`, `core/anchor`, `core/gestures` |
| 3. Voir la vidéo | Format et fenêtre faits, décodage FFmpeg non fait | `core/videocache`, `core/framewindow` |
| 4. Le Mac tourne | CI verte sur macOS depuis le premier commit ; portage audio/GPU réel non fait | `.github/workflows/ci.yml` |
| 5. Mixer | Courbes, blend modes, détection de transform faits ; rendu GPU non fait | `core/mixer` |
| 6. Transport | Fait en entier : boucles, hot cues, beat jump, slip, ABS/REL/INT | `core/transport` |
| 7. 360 | Géométrie faite (perspective, little planet, fisheye) ; échantillonnage GPU non fait | `core/sphere` |
| 8. Mode autonome | Non commencé (a besoin d'un vrai backend audio) | — |
| 9. Effets et modulateurs | Rack, catalogue et LFO/enveloppes faits ; FFT audio-réactive non faite | `core/effect`, `core/modulator` |
| 10. Entrées live | Non commencé | — |
| 11. Sorties | Corner pin et masque faits ; Spout/NDI/écran réels non faits | `core/warp` |
| 12. Unreal | Non commencé (protocole réseau prêt à l'emploi) | `core/protocol` |

Plus, hors plan initial : `core/library` (bibliothèque et queue), `core/take`
(enregistrement/relecture d'une prise), `app/` (démo et tableau de bord qui font
tourner tout ça sans matériel).

**Ce qui reste, dans les grandes lignes** : tout ce qui touche du matériel ou une
dépendance lourde — le décodeur `timecoder.c` de xwax, un vrai périphérique MIDI et
audio, la passe d'analyse FFmpeg, le rendu GPU (bgfx), l'interface Dear ImGui, les
sorties Spout/NDI, et le plugin Unreal. Rien de tout ça n'est vérifiable sans être
devant le matériel, donc rien n'a été écrit à l'aveugle.

**Deux tests qui reviennent à l'utilisateur, devant le matériel :**
1. Est-ce que l'Elite émet son état MIDI à la connexion ? Ça décide du sort du
   mode fantôme des potards absolus.
2. Est-ce que le **second port USB** de l'Elite reçoit le timecode en parallèle de
   Serato ? C'est la seule inconnue qui pourrait imposer une deuxième machine —
   voir le tableau des quatre voies plus bas.

---

## Contexte

Le projet a changé de nature pendant le cadrage. Point de départ : « faire remonter
le mouvement de mes platines dans Unreal ». Objectif réel : **un logiciel DJ vidéo
autonome, multiplateforme** — charger n'importe quelle vidéo sur des decks, la
scratcher aux platines, mixer au crossfader, gérer une file de clips, piloter tous
les paramètres depuis les potards et boutons de l'Elite, lire et scratcher des
vidéos **360** en dirigeant le regard au potard, et appliquer un **rack d'effets où
chaque effet audio a son pendant visuel piloté par le même bouton**. Unreal devient
un **client** de cette app, pas son cœur.

Décisions actées :

| Sujet | Choix |
|---|---|
| Audio | **Deux modes commutables** : *suiveur* (Serato joue le son) ou *autonome* (l'app joue et scratche le son elle-même). |
| Effets | **Paires audio/vidéo liées par défaut**, déliables unité par unité. |
| Plateformes | **Windows d'abord, architecture portable dès la première ligne** (macOS visé). |
| Rendu GPU | **bgfx** (voir justification plus bas). |
| UI | **Dear ImGui** natif. |
| Unreal | Client : reçoit l'état de la surface (UDP/OSC) et le mix vidéo (Spout). Windows. |

### Faits vérifiés qui déterminent l'architecture

1. **Le Phase sort du timecode DVS standard** en RCA — le dock génère le signal de
   contrôle attendu par n'importe quel DVS. Aucun SDK MWM nécessaire : un décodeur
   donne position absolue + vitesse signée.
2. **La Reloop Elite** a une interface audio **double 10x10 USB** et **tous ses
   contrôles sont MIDI**. Routing réglable dans les Utilities, entrées basculables
   LINE/PHONO par canal.
3. **La RP-8000 MK2** a un port USB type B et 8 pads LED MIDI sur 3 couches —
   surface de contrôle directement exploitable.
4. **Aucun projet open-source** ne fait DVS → vidéo scratchable → Unreal.
5. Côté Unreal, **Spout** est disponible via plusieurs plugins UE5 (Off World
   Live, et des implémentations open-source DX11-on-DX12) : partage de texture
   GPU sans copie ni encodage.

### Les deux modes audio

Le mode se choisit dans la barre de statut. **Le décodeur de timecode et tout le
moteur vidéo sont identiques dans les deux cas** — seule change la question de qui
possède le son.

**Mode suiveur (Serato).** Serato joue et scratche le son ; l'app ne fait que la
vidéo. Comme elle lit le **même** timecode, la position vidéo suit la position
audio. Il faut donc :
- un **ancrage** par deck (`offset = position_timecode − position_vidéo`), posé
  d'un coup de pad au moment voulu, plus un **indicateur de fraîcheur** affiché en
  permanence ;
- un **re-ancrage** après tout ce qui casse la correspondance : needle drop,
  boucle, censor, ou passage en mode relatif dans Serato.

> **Serato n'a pas d'API publique** : impossible de lire sa vraie tête de lecture.
> L'ancrage n'est pas un raccourci, c'est le seul mécanisme possible, et il faut
> donc que le re-ancrage soit à un pad de distance plutôt que caché dans un menu.
> (Écarté : aligner par empreinte audio du master — coûteux et fragile pour un
> gain marginal.)
>
> **La dérive n'est pas mesurable**, et c'est implémenté ainsi (`core/anchor`) : si
> le DJ boucle, censure ou pose l'aiguille dans Serato, l'audio bouge et le
> timecode ne bouge pas — la correspondance casse sans que rien ne soit observable
> de notre côté. Un nombre de secondes affiché serait donc inventé. Ce qui *est*
> observable, c'est ce qui s'est passé depuis la pose : le temps écoulé et le
> nombre de discontinuités de timecode vues. On affiche donc une **fraîcheur** —
> une mesure de risque, pas d'erreur.

**Mode autonome.** L'app possède le son : elle décode le timecode, lit et scratche
l'audio, et le renvoie sur les canaux USB de l'Elite. Boucle DVS complète, sync
parfaite par construction, et possibilité de scratcher un clip sans équivalent
dans Serato.

**Le vrai problème est de partager l'entrée audio en mode suiveur sur Windows**, où
les pilotes ASIO DJ sont généralement mono-client : si Serato tient le device,
l'app ne peut pas l'ouvrir. Quatre issues, par ordre de préférence :

| Voie | Détail | Verdict |
|---|---|---|
| **2ᵉ port USB de l'Elite** | Interface double 10x10 : Serato sur USB-B1, l'app sur USB-B2, les deux reçoivent les entrées phono | Matériel déjà possédé — **à tester en premier**, y compris sur une seule machine |
| **Split RCA passif** | Sortie du Phase dupliquée vers une petite interface d'entrée dédiée | Toujours fonctionnel, ~50 € |
| **WASAPI partagé** | Si le pilote Reloop expose WASAPI à côté d'ASIO | Dépend du pilote, à tester mais pas à présumer |
| **Deux machines** | Laptop Serato + PC vidéo | Le repli qui marche toujours |

**Sur macOS le problème n'existe pas** : CoreAudio est multi-client, l'app ouvre la
même entrée que Serato sans rien partager de spécial.

---

## Deux principes de conception

> **1. Tout ce qui doit être scratchable est une *fonction* de la position, jamais
> un intégrateur.**

Une simulation avance et ne sait pas reculer ; un flux paramétré par un `t` se
scratche parfaitement. Ce principe décide de tout : le moteur vidéo indexe des
frames au lieu de lire un flux, le moteur audio se pilote en position et non en
vitesse, et côté Unreal les effets sont soit paramétriques, soit adossés à un
historique enregistré.

> **2. L'audio agit sur les fréquences temporelles d'un signal 1D, la vidéo sur les
> fréquences spatiales d'un signal 2D.** Quand la correspondance est exacte, on
> l'implémente telle quelle ; quand elle ne l'est pas, on choisit l'analogue
> perceptif le plus proche et on le documente.

C'est ce qui transforme le rack d'effets en système cohérent plutôt qu'en tas
d'effets arbitraires — voir [`fx-correspondances.md`](fx-correspondances.md) pour
la table complète, tenue en code dans `core/effect.h`.

---

## Architecture cible

```
Phase RX ──RCA timecode──> Elite (canaux PHONO) ──USB──┐
Elite : faders, EQ, filtres, FX, 16 pads ──USB MIDI────┤
RP-8000 MK2 : 8 pads x 3 couches ─────────USB MIDI─────┤
                                                       v
                                    ┌──────────────────────────────────┐
                                    │       scratchvj (C++20)          │
                                    │  surface + mapping engine        │
                                    │  timecode  ->  position          │
                                    │  moteur audio DVS                │
                                    │  moteur vidéo (frames GPU)       │
                                    │  RACK D'EFFETS APPAIRÉS          │
                                    │  reprojection 360                │
                                    │  biblio / queue / UI ImGui       │
                                    └──┬─────────┬──────────┬──────────┘
                audio ──USB/ASIO/CoreAudio┘      │          │
                vers les canaux Elite            │          │
                                     écran/projecteur   Spout|Syphon / NDI / fichier
                                                            │
                                                            v
                                         ┌───────────────────────────────┐
                                         │ Unreal + plugin ScratchLink   │
                                         │ état surface via UDP/OSC      │
                                         │ mix vidéo via texture Spout   │
                                         │ rewind 3D, caméra, monde 3D   │
                                         └───────────────────────────────┘
```

**Le flux de contrôle UDP/OSC (`core/protocol`) existe déjà.** Même si le plugin
Unreal est prévu en dernier, on peut brancher Unreal, TouchDesigner ou Resolume
dès que la surface est lue.

### Choix des briques, toutes portables Windows/macOS

| Rôle | Brique | Pourquoi |
|---|---|---|
| Rendu GPU | **bgfx** | Mûr, permissif, backends **Metal** et D3D/Vulkan natifs. Gère depuis longtemps les **tableaux de textures** et les **formats BCn**, dont dépend tout le moteur vidéo. Intégration Dear ImGui établie. |
| Fenêtre / entrées | **SDL3** | Standard, portable, sans surprise. |
| UI | **Dear ImGui** | Dense, immédiat, parfait pour potards et tableaux. |
| Audio | **miniaudio** (WASAPI + CoreAudio), **ASIO** en backend optionnel Windows | Header unique, domaine public, aucun SDK à télécharger pour démarrer. |
| MIDI | **RtMidi** | Windows MM + CoreMIDI. |
| Décodage vidéo | **FFmpeg** | Le seul moyen d'ouvrir « n'importe quelle vidéo ». |
| Timecode | **`timecoder.c` de xwax** | Décodeur DVS éprouvé (GPL-3, voir Licence). |
| Partage texture | **Spout** (Win) / **Syphon** (Mac) / **NDI** (les deux) | Derrière une interface commune. |

**OpenGL a été écarté** : déprécié par Apple et plafonné à 4.1. **SDL3 GPU** est
plus propre sur le papier mais trop jeune, et son écosystème est mince
précisément sur les tableaux de textures et BCn. **Prix à payer de bgfx, annoncé
d'avance** : ses shaders s'écrivent dans un dialecte propre compilé par
`shaderc`, moins agréable que du GLSL brut — le coût de la portabilité, payé une
fois.

> **Une architecture portable qui n'est jamais compilée sur l'autre OS est une
> fiction.** D'où la CI macOS dès le premier commit — déjà en place et vérifiée à
> chaque push, avant même que l'app y fasse quoi que ce soit d'utile.

---

## Les briques restant à écrire

### Timecode et moteur audio (le DVS)

**Décodage** via `timecoder.c` de xwax (`serato_2a/2b`, `traktor_a`, `mixvibes`). À
vérifier en lisant `player.c` : `timecoder_get_position()` renvoie **-1 hors
verrouillage**, et son paramètre `when` donne le nombre d'échantillons écoulés
depuis la mesure — il faut extrapoler `pos += when * pitch / rate`.
`timecoder_get_pitch()` est un ratio sans dimension (1.0 nominal, négatif en
arrière).

**Lecture pilotée en position, jamais en vitesse.** À chaque bloc audio on connaît
la position cible en début et en fin de bloc ; on interpole le pointeur de lecture
sur le bloc et on rééchantillonne (Hermite cubique, sinc fenêtré si le repliement
s'entend sur les scratchs rapides). Piloter en vitesse dériverait ; piloter en
position ne dérive jamais.

**Backend derrière une interface** : **miniaudio** (WASAPI exclusif sur Windows,
CoreAudio sur Mac) pour démarrer sans SDK, ~10 ms aller-retour sur bon matériel.
**ASIO ajouté ensuite** si la latence gêne (128 échantillons @48 kHz ≈ 8-10 ms).

**Le mode décide de ce que fait cette couche**, et l'interface
`DeckAudioSource { Interne | Externe }` doit exister dès le premier jour :
- **Autonome** : boucle DVS complète, device ouvert en **duplex, exclusif**.
- **Suiveur** : **entrée seule, jamais en exclusif**, aucun moteur de lecture
  audio, aucune sortie. Ouvrir le device en exclusif dans ce mode empêcherait
  Serato de fonctionner — c'est l'erreur à ne pas commettre.

### Moteur vidéo — scratcher n'importe quel fichier

L'erreur classique est de faire `Seek()` sur un lecteur vidéo : le décodage
compressé ne suivra jamais un scratch, et même l'`ImgMedia` d'Unreal, pourtant
conçu pour ça et doté d'un cache, rame au scrubbing rapide d'après les retours
communautaires. **Il faut sortir du décodage temps réel.**

**Passe d'analyse hors ligne** (comme l'analyse de morceau de Serato) : FFmpeg
décode le clip une fois, redimensionne, compresse chaque frame en **BCn** (déjà
formalisé dans `core/videocache`), et écrit un `.svcache`. Ensuite, plus jamais de
décodage. File de jobs en arrière-plan avec progression dans l'UI. La même passe
extrait vignette, métadonnées et **beatgrid**.

### Retour LED et mapping en dur

**Retour LED.** Les 16 pads RGB de l'Elite et les 8 de la RP-8000 acceptent
probablement du MIDI entrant pour leur couleur (usage standard en intégration
Serato) — à confirmer devant le matériel. Si oui, **les pads deviennent l'UI de
l'app** : navigation de queue, chargement de deck, next, codés par couleur, sans
regarder l'écran.

### Pont Unreal — plugin `ScratchLink`

`UScratchLinkSubsystem` (`UGameInstanceSubsystem`) lit le socket UDP sur un thread
dédié et expose `GetStateAtTime(double)` : Unreal tourne à 60-120 fps, le flux
arrive vers 375 Hz ; on garde les derniers paquets et on échantillonne à
`now - offset` avec interpolation Hermite sur la position. C'est le modèle Live
Link sans le coût d'en écrire une source.

> On n'utilise pas le plugin OSC natif d'UE pour ce flux : il alloue des UObjects
> par message et dispatche sur le game thread — à 375 Hz c'est de la garbage pour
> rien. Socket UDP + struct POD (déjà le format de `core/protocol`). Le miroir OSC
> reste disponible pour TouchDesigner/Resolume.

- **`ScratchTimeMachineComponent`** — ring buffer à 120 Hz. Mode RECORD normal,
  mode SCRUB avec physique désactivée et transforms interpolés. **Le détail qui
  fait tout est la sortie de scrub** : réinjecter les vitesses dérivées du buffer,
  pas zéro, sinon la scène s'effondre au lieu de repartir quand tu relâches le
  plateau. Version pauvre à câbler d'abord pour valider la chaîne :
  `vel → Set Global Time Dilation` clampé `[0,4]`.
- **`ScratchCameraRigComponent`** — position du plateau → distance sur une spline
  (dolly), angle d'orbite, ou dolly-zoom ; crossfader → blend ou cut entre deux
  caméras ; `accel` → camera shake.

---

## Écarts avec Serato et Resolume

Un instrument qui fait une chose inédite mais à qui manquent les gestes de base
est inutilisable en set. Voici l'inventaire qui a guidé le phasage, et ce qui a
déjà été traité.

### Ajouté depuis (déjà dans le code)

| Manque | Vient de | Traité dans |
|---|---|---|
| Boucles et hot cues | Serato | `core/transport` |
| Modes ABS / REL / INT | Serato | `core/timecode` (`TransportMode`) |
| Slip mode | Serato | `core/transport` |
| Blend modes par couche | Resolume | `core/mixer` (`BlendMode`) |
| Canal alpha | Resolume | `core/videocache` (`kCacheAlpha`, BC7/BC3) |
| Modulateurs : LFO, enveloppes | Resolume | `core/modulator` |
| Corner pin et masque | — | `core/warp` |

### Encore à ajouter

| Manque | Vient de | Pourquoi c'est bloquant |
|---|---|---|
| **Source de transport par deck** | Resolume | Tous les decks ne doivent pas suivre le plateau. Une texture de fond tourne en boucle libre ou calée au tempo. |
| **Modes de lecture du clip** | Resolume | Boucle, aller-retour, une seule fois, sens et vitesse propres au clip. |
| **Une 3ᵉ couche d'incrustation** | Resolume | Deux decks à scratcher plus une couche par-dessus (logo, texte, masque). Pas N couches : on reste un instrument, pas un compositeur. |
| **Réactivité audio (FFT)** | Resolume | Le geste reste le différenciateur, mais l'audio-réactif est attendu et se branche sur la mécanique de `core/modulator` (`SourceKind::AudioBand` existe déjà côté mapping). |
| **Entrées live** | Resolume | Un deck dont la source est une caméra, une entrée NDI ou Spout, au lieu d'un fichier. |
| **Scope de calibration timecode** | Serato | Un `--monitor` en ligne de commande existe déjà (`core/timecode` exposé par la démo) ; il faut le voir à l'écran, en set, une fois l'UI ImGui écrite. |

### À ne pas faire — et pourquoi

| Écarté | Raison |
|---|---|
| **Mapping avancé : warp maillé, edge blending multi-projecteurs, slices** | Projet à soi seul. On sort en **Spout / NDI** vers Resolume ou MadMapper pour ces cas-là. |
| **Compositing N couches, groupes, matrice de clips** | On construit un instrument de scratch, pas un VJ compositeur généraliste. Trois couches suffisent. |
| **Sync, beatmatch, détection de tonalité, mix harmonique** | Antithétique au turntablisme, et couvert par Serato en mode suiveur. |
| **Ableton Link, horloge MIDI** | Reporté. Le mapping OSC ouvre déjà une porte. |
| **DMX / Art-Net** | Reporté. Atteignable plus tard par la couche de mapping. |
| **Intégration de services de streaming, gestion de bibliothèque avancée** | Hors sujet pour de la vidéo. |

### Ce que ni l'un ni l'autre ne fait

À garder en tête quand on arbitre : **les effets appairés audio/vidéo**, la
**vidéo scratchée par indexation** plutôt que par seek, la **360 dont le regard se
pilote pendant qu'on scratche le temps**, et les **grandeurs de geste** (taux de
scratch, backspin) comme sources de modulation de plein droit. C'est là qu'il faut
dépenser l'effort ; le reste, c'est du rattrapage.

---

## Idées d'usage

**Vidéo** — deux decks scratchés indépendamment et mixés comme un vrai mixer
vidéo ; beat juggling visuel où l'écart entre positions A et B pilote la
parallaxe de deux couches ; stutter quantifié pendant qu'un pad est tenu ;
glitch piloté par `scratch_rate` ; inversion de chroma au changement de sens ;
cut stroboscopique au crab/transform.

**360** — pan/tilt au potard pendant qu'on scratche le temps ; crossfader entre
deux regards du même clip ; little planet dont la rotation suit le roll ; backspin
déclenchant un whip pan ; panoramique audio lié au yaw.

**Unreal** — rewind physique réel (verre qui se recompose sous la main) ; scratch
d'un Level Sequence ; dolly sur spline et orbite au plateau ; cut multi-caméra au
crossfader ; mix vidéo Spout plaqué sur des écrans dans une scène 3D et déformé ;
wipe entre deux mondes au crossfader.

**Show** — pads en navigation de queue codée par couleur ; enregistrement d'une
prise et rendu offline propre ; sortie NDI vers régie ou stream.

---

## Vérification, une fois le matériel branché

1. `--midi-learn` puis parcourir la table — le crossfader doit aller de -1 à +1,
   les pads s'allumer. Tester si l'Elite émet son état à la connexion (décide du
   sort du mode fantôme).
2. **Avant toute ligne de vidéo** : `--monitor` doit montrer une position qui
   monte linéairement à vitesse nominale, un sens qui s'inverse proprement, une
   `confidence` haute. Si le lock est instable, essayer les autres définitions de
   timecode. **Puis, Serato lancé en parallèle**, vérifier que l'app lit toujours
   le timecode — c'est le test qui tranche le tableau des quatre voies de partage
   d'entrée.
3. **Ancrage** : poser l'ancrage sur un pad, laisser tourner deux minutes, lire la
   fraîcheur affichée. Puis provoquer un needle drop dans Serato et vérifier qu'un
   seul appui de pad remet tout en place.
4. **Mode autonome** : test d'écoute — scratch, spinback, needle drop. Si le
   feeling n'est pas celui du vinyle, c'est le resampling à revoir, pas la
   latence.
5. **Analyse vidéo** : jeu de clips variés (codecs, fps, résolutions) en
   non-régression ; mesure du temps de recentrage de la fenêtre VRAM après un
   needle drop hors fenêtre.
6. **Effets appairés** : test perceptif, pas seulement fonctionnel — un tiers qui
   n'a pas écrit le code doit reconnaître l'effet audio en ne regardant que
   l'image. C'est le seul critère qui compte.
7. **Décrochage** : forcer `confidence` à 0 doit **geler** proprement — jamais de
   téléportation de l'image ni de la caméra Unreal. (Déjà vérifié en logique pure
   par les tests de `core/timecode` ; à reconfirmer avec le vrai matériel.)
8. **Latence de bout en bout** : filmer main + écran au ralenti, compter les
   frames d'écart. Cible sous 30 ms pour que ce soit bon visuellement, sous 20 ms
   pour que ça « colle » à la main.

---

## Risques

**Risque principal, contenu par le mode suiveur : le moteur audio DVS.** Le mode
suiveur ne demande pas de moteur audio propre : le produit sera jouable dès que
FFmpeg + le décodage vidéo existeront, sans attendre le mode autonome. Il reste un
risque de *qualité* — un scratch audio qui « sonne » mal serait décevant.

**Risque devenu principal : le partage de l'entrée audio sous Windows en mode
suiveur.** Les pilotes ASIO DJ sont souvent mono-client. Le tableau des quatre
voies plus haut est à trancher **devant le matériel**, en testant d'abord le
second port USB de l'Elite.

**Risque secondaire : le mapping et le retour LED de l'Elite** ne sont pas
documentés publiquement. Déjà traité par construction avec `--midi-learn`
(`core/learn`) plutôt que par des valeurs en dur.

**Risque tiers : la VRAM en 360 haute résolution.** Le budget par deck est un
réglage exposé (`core/framewindow`), et [`format-cache.md`](format-cache.md)
donne les compromis. En 4K équirect sur une carte modeste, il faudra réduire la
fenêtre ou la résolution d'analyse.

## Licence

`timecoder.c` est **GPL-3** : `scratchvj` sera donc GPL-3 s'il est distribué.
FFmpeg est LGPL en lien dynamique ; bgfx, SDL3, Dear ImGui, miniaudio et RtMidi
sont permissifs. Le **plugin Unreal reste sous la licence de ton choix** puisqu'il
ne fait que lire un socket UDP et une texture partagée — la frontière entre les
deux processus n'est pas seulement architecturale, elle est aussi juridique. Le
SDK ASIO se compile contre, mais ne se redistribue pas.
