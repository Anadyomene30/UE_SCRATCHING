# Correspondances audio → vidéo

Le rack d'effets de `scratchvj` n'est pas une collection d'effets vidéo posée à
côté d'une collection d'effets audio. **Une unité d'effet est une paire** : un DSP
audio et son implémentation visuelle, pilotés par les mêmes paramètres. Le lien est
actif par défaut et se coupe unité par unité.

## La règle

> L'audio agit sur les **fréquences temporelles** d'un signal 1D.
> La vidéo agit sur les **fréquences spatiales** d'un signal 2D.

Quand la correspondance est exacte, on implémente le même calcul dans l'autre
domaine. Quand elle ne l'est pas, on choisit l'analogue perceptif le plus proche et
on l'écrit ici comme tel. C'est ce qui fait la différence entre un système cohérent
et un tas d'effets arbitraires — et c'est la règle à appliquer pour tout effet
ajouté plus tard.

## La table

| Effet | Audio | Vidéo | Relation |
|---|---|---|---|
| Passe-bas | retire les hautes fréquences temporelles | flou gaussien : retire les hautes fréquences spatiales | **identique** |
| Passe-haut | retire les basses fréquences | rehaussement de contours, gris médian | **identique** |
| Filtre DJ (1 potard) | LPF ← neutre → HPF | flou ← neutre → netteté | **identique** |
| Bitcrusher | réduction de profondeur de bits | postérisation, réduction de palette | **identique** |
| Décimation (SRR) | baisse de fréquence d'échantillonnage | gel / décimation de frames (stutter) | **identique** |
| Delay / Echo | `out = in + fb · retardé` | traînées d'images à contre-réaction, même équation | **identique** |
| Distortion / Drive | écrêtage d'amplitude | écrêtage des couleurs, saturation dure | **identique** |
| Ring mod | multiplication par une porteuse | multiplication par un motif spatial | **identique** |
| Tremolo / Gate | modulation d'amplitude | strobe, gate de luminosité au même taux | **identique** |
| Reverse | lecture inversée | frames à l'envers | **identique** |
| Loop roll / beat repeat | boucle de N temps | boucle de frames de même durée | **identique** |
| Noise | bruit additif | grain, neige | **identique** |
| Reverb | diffusion + décroissance | smear diffus, bloom persistant | analogue |
| Flanger / Phaser | filtre en peigne balayé | déplacement UV ondulant, moiré chroma | analogue |
| Pitch shift | transposition | zoom, échelle spatiale | analogue |
| Panoramique | position stéréo | translation horizontale — **et en 360, le yaw** | analogue |

La dernière ligne mérite d'être relevée : en vidéo 360, panoramiquer le son *est*
tourner la tête. Le même paramètre pilote les deux, et ce n'est pas une métaphore.

## Synchronisation au tempo

Elle ne coûte presque rien parce qu'on possède déjà les deux moitiés : le beatgrid
sort de la passe d'analyse, la position exacte sort du timecode. La phase se dérive
de ces deux-là — aucune détection de tempo temps réel à écrire.

## Le lien, et quand le couper

`link` est vrai par défaut : un seul potard d'écho fait échoer le son et l'image.
Le couper sert à deux choses — un effet purement visuel sur un son qu'on veut
laisser intact, ou des réglages volontairement divergents entre les deux domaines.
C'est le seul état où audio et vidéo racontent des choses différentes, donc c'est
le seul que l'interface signale explicitement.

## Critère de validation

Fonctionnel ne suffit pas. **Un tiers qui n'a pas écrit le code doit reconnaître
l'effet audio en ne regardant que l'image.** Si ce n'est pas le cas, c'est
l'analogue qui est mauvais, pas le réglage.
