# Le format `.svcache`

Scratcher une vidéo ne peut pas passer par un lecteur classique : chercher dans un
flux compressé ne suivra jamais une main sur un plateau. Le clip est donc décodé
**une seule fois**, hors ligne, en frames compressées par blocs — le format natif
du GPU — et la lecture devient une indexation de tableau. C'est la même idée que
l'analyse d'un morceau par Serato avant de le jouer.

## Frames de taille fixe

La compression par blocs donne des frames de taille constante. **Ce n'est pas une
contrainte subie, c'est le point** : l'offset de la frame *n* est une
multiplication, donc il n'y a pas d'index à consulter, pas de recherche à rater, et
aucune différence de coût entre lire en avant, en arrière, ou sauter n'importe où.
**La marche arrière est gratuite.**

## Disposition

```
[ en-tête, 64 octets ]                 (dont la longueur des métadonnées)
[ métadonnées : autant d'octets ]      chemin source, vignette
[ données : frame_count × frame_bytes ]
```

En-tête :

| Champ | Type | Note |
|---|---|---|
| magic | u32 | `SVC1` |
| version | u16 | rejetée si inconnue |
| flags | u16 | bit 0 alpha, bit 1 équirectangulaire (360) |
| width, height | u32 ×2 | |
| frame_count | u32 | réécrit à la fermeture, quand il est enfin connu |
| fps_num, fps_den | u32 ×2 | rationnel, pour que 29,97 soit exact |
| format | u8 | BC1, BC3 ou BC7 |
| longueur métadonnées | u32 | |

Tout est en petit-boutien, écrit octet par octet (`core/bytes.h`) : pas de surprise
de padding, pas de piège d'alignement, les mêmes octets depuis tous les
compilateurs.

### Le bloc de métadonnées (`core/cachemeta`)

Sa longueur est fixée à l'ouverture, avant la première frame ; son contenu est
réécrit **en place** avant la fermeture (`CacheWriter::rewrite_metadata`),
parce que la vignette vient d'une frame que la passe n'a pas encore décodée
quand elle ouvre le fichier.

```
[ magic u32 "SVM1" ]
[ u32 longueur + chemin source ]
[ u32 largeur, u32 hauteur, u32 longueur + blocs BC1 de la vignette ]
```

La vignette fait 128 px de large, la hauteur garde le ratio arrondi au
multiple de quatre (128 × 72 pour du 16:9, 128 × 64 pour un équirect), prise
au tiers du clip — passé le fondu et le clap d'ouverture. Un bloc **sans** le
magic est lu comme un chemin source nu : c'est ce que tous les caches écrits
avant la vignette contiennent, et ré-analyser une bibliothèque pour gagner une
image serait le mauvais prix. Une vignette dont la taille n'est pas exactement
celle de ses blocs est ignorée, le chemin gardé.

## Choix des formats de bloc

| | Octets / bloc 4×4 | Alpha | Usage |
|---|---|---|---|
| BC1 | 8 | non | le plus compact, pour du contenu opaque |
| BC3 | 16 | oui | alpha à moitié du coût d'encodage de BC7 |
| BC7 | 16 | oui | le choix qualité |

L'analyse tournant hors ligne, la qualité est abordable : BC7 par défaut, BC1
quand la VRAM est le facteur limitant.

## Deux refus délibérés

**Une frame de mauvaise taille est refusée à l'écriture.** Une frame courte
décalerait silencieusement toutes les suivantes, et le dégât n'apparaîtrait que
comme une image fausse, bien plus tard.

**Un fichier dont la taille ne correspond pas à son `frame_count` est refusé à la
lecture** — c'est la signature d'une analyse interrompue. Mieux vaut un message
clair qu'une image de garbage.

De même, lire hors des bornes échoue au lieu de se rabattre sur la frame la plus
proche : demander une frame qui n'existe pas est un bug de l'appelant, et en
renvoyer une autre le masquerait.

## La fenêtre VRAM

Un clip entier ne tient pas forcément en mémoire vidéo. Une plage roulante autour
de la tête de lecture y est maintenue et réalimentée depuis le `.svcache` par
lecture disque brute, sans décodage.

**Elle est dimensionnée par un budget de VRAM, pas par une durée fixe.** C'est ce
choix seul qui fait tenir la 2D et la 360 dans le même mécanisme : le même
gigaoctet contient environ 36 secondes de 720p ou environ 4 secondes de 4K
équirectangulaire, sans cas particulier.

**Règle anti-battement : la fenêtre n'est recentrée que lorsque la tête de lecture
approche d'un bord**, jamais parce que le sens a changé. Un scratch travaille
d'avant en arrière au milieu de la fenêtre — s'il fallait recentrer à chaque
inversion, le cas qui doit absolument ne jamais hoqueter serait précisément celui
qui martèlerait le disque.

Un glissement ne recharge que le nouveau bord. Seul un needle drop **hors** de la
fenêtre recharge tout : c'est le bref temps de chargement assumé, et le seul que
l'utilisateur devrait sentir.
