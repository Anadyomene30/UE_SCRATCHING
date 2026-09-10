# Le vocabulaire de la scène — les mots propres à cet instrument

Écrit le 2026-09-10, phase 2 du talon.

**Ce document ne remplace rien.** Le vocabulaire commun de la maison est dans
`spec/00-vocabulaire.md`, à son adresse, et il fait foi partout où un mot y
figure : tout ce qui nomme une projection, une disposition, un œil, un
conteneur. Ce document-ci nomme **ce que ce vocabulaire ne couvre pas** — les
objets de la scène, qui n'ont pas de mot commun parce qu'aucun autre produit ne
les manipule.

La portée de la table de la maison est écrite dans la table elle-même : elle
nomme ce qui décrit une plate, et « un concept qu'elle ne couvre pas n'est pas
un mot inventé : c'est un mot pour autre chose, et il vit dans la spec de
l'objet qu'il décrit ». C'est ce fichier, pour l'instrument.

**Un concept, un mot.** La forme canonique est en `snake_case` ASCII, comme
celle de la maison ; le libellé français est celui que l'interface écrit. Chaque
ligne renvoie au fichier où le concept vit, sans numéro de ligne : ce document
est fait pour rester vrai après la prochaine révision.

**Aucun de ces mots ne traverse une frontière aujourd'hui.** Ils vivent dans le
code, dans l'interface, dans `library.json`, `settings.json`, `mapping.json` et
dans un `.svtake` — que ce produit est le seul à lire. Le jour où l'un d'eux
entre dans un pont ou dans un manifeste, il monte à la maison et cesse
d'appartenir à ce fichier.

---

## Ce qui joue

| Canonique | Libellé FR | Ce que c'est | Où il vit |
|---|---|---|---|
| `deck` | Deck | un lecteur : un clip, une position, une source d'horloge | `core/playback` |
| `platter` | Plateau | la platine physique, lue en quadrature de phase | `core/quadrature` |
| `deck_source` | Source | ce qui donne sa position au deck : `timecode`, `free_run`, `tempo_locked`, `hand` | `core/playback` |
| `anchor` | Ancre | le décalage entre la position du disque de contrôle et la position dans le clip, en mode suiveur | `core/anchor` |
| `program` | Programme | l'image unique qui part au projecteur, à Spout et au réseau | `core/compose` |
| `overlay` | Incrustation | la troisième couche, au-dessus des deux decks | `core/mixer` |
| `live_ring` | Anneau live | les dernières secondes d'une entrée live, adressées par leur heure de capture — ce qui rend une source sans timeline scratchable | `core/livering` |

## Ce qu'on déclenche

| Canonique | Libellé FR | Ce que c'est | Où il vit |
|---|---|---|---|
| `cue` | Repère | un point posé dans le clip, rejoint d'un pad | `core/transport` |
| `loop` | Boucle | un intervalle rejoué, en temps ou en temps musicaux | `core/transport` |
| `beat_jump` | Saut | un déplacement d'un nombre de temps | `core/transport` |
| `slip` | Slip | la position continue court sous la boucle et se reprend à la sortie | `core/transport` |
| `beat_grid` | Grille | le tempo et le premier temps, sortis de l'analyse | `core/transport` |
| `pad` | Pad | une cellule déclenchable, à l'écran comme sur la surface | `core/matrix` |
| `bank` | Banque | huit pads par couche, et un set en garde plusieurs | `core/matrix` |

## Ce qu'on range

| Canonique | Libellé FR | Ce que c'est | Où il vit |
|---|---|---|---|
| `clip` | Clip | une entrée de bibliothèque : un cache jouable et ce qu'on a décidé de lui | `core/library` |
| `cache` | Cache | le `.svcache` : la vidéo décodée une fois, indexable par position | `core/videocache` |
| `crate` | Caisse | un regroupement de clips, nommé par l'utilisateur | `core/library` |
| `queue` | File | l'ordre de passage — ce qui vient ensuite, et un bouton | `core/library` |
| `take` | Prise | le flux horodaté de tout ce que la surface et les plateaux ont fait | `core/take` |

## Ce qu'on règle

| Canonique | Libellé FR | Ce que c'est | Où il vit |
|---|---|---|---|
| `surface` | Surface | le miroir de la table : un contrôle physique par entrée, avec sa dernière valeur connue | `core/surface` |
| `ghost_control` | Contrôle fantôme | un potard absolu dont la position réelle est inconnue, dessiné en pointillé plutôt qu'à une valeur inventée | `core/surface`, dessiné par `ui/panels` |
| `profile` | Profil | la description d'un appareil : ses contrôles, ses adresses MIDI, et parfois la géométrie de son panneau en millimètres | `core/profile` |
| `mapping` | Correspondance | un lien d'une source à une destination, avec sa courbe, sa zone morte et son lissage | `core/mapping` |
| `destination` | Destination | ce qu'une correspondance peut atteindre — un registre, jamais une cible devinée | `core/destinations` |
| `modulator` | Modulateur | un LFO ou un suiveur d'enveloppe : une source comme un potard | `core/modulator` |
| `rack` | Rack | les emplacements d'effets d'une voie | `core/effect` |
| `gesture` | Geste | les grandeurs dérivées du mouvement du plateau, calculées une fois pour tous les consommateurs | `core/gestures` |

## Ce qui sort

| Canonique | Libellé FR | Ce que c'est | Où il vit |
|---|---|---|---|
| `warp` | Géométrie | la déformation qui pose le programme sur une surface réelle | `core/warp`, `core/mesh` |
| `mask` | Masque | ce que le projecteur ne doit pas éclairer | `core/surface` de sortie, `ui/panels` |
| `scope` | Scope | la figure de Lissajous brute du signal de calibration, et ses trois défauts séparés | `core/scope` |

---

## Ce que ce document n'invente pas

**Les mots de la maison ne sont pas repris ici.** `equirect_360`, `flat`,
`rectilinear`, `little_planet`, `fisheye_view`, `unset`, les états d'un travail
— tout cela se lit dans `spec/00-vocabulaire.md`, et les recopier ferait
exactement ce que `maison/00-LIRE-DABORD.md` interdit : deux exemplaires qui se
mettent à diverger.

**Trois homonymes, et il faut les tenir séparés** — chacun a déjà coûté une
lecture fausse dans ce dépôt :

- **`layout`** ne désigne ici **jamais** une disposition stéréo. C'est la
  disposition physique des contrôles d'un mixeur (`core/layout`), et dans un
  profil la position d'un groupe en millimètres sur la face avant.
- **`mono`** ne désigne ici **jamais** une plate monoscopique. C'est l'audio à
  une voie (`core/spectrum`), le bouton mono d'une cabine
  (`core/profiles_builtin`), ou la fonte à chasse fixe (`ui/panels`).
- **`projection`** a deux sens dans ce produit : la manière de lire un clip —
  et là c'est le mot de la maison qui gouverne — et la géométrie d'un
  vidéoprojecteur sur une surface, qui est `warp` ci-dessus.

**Ce qui reste sans mot, et pourquoi.** Rien de ce que l'instrument fait n'a été
nommé ici pour combler un trou : chaque ligne est un concept qui existe déjà
dans le code, avec son fichier. Un concept que ce document ne porte pas est un
concept que le produit n'a pas.
