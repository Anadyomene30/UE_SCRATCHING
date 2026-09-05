# Câblage et modes audio

## Les deux modes

Le décodeur de timecode et tout le moteur vidéo sont **identiques dans les deux
modes**. Seule change la question de qui possède le son. C'est ce qui rend le
double mode peu coûteux — à condition de l'avoir conçu dès le départ.

### Mode suiveur (Serato joue le son)

Serato fait le DVS ; `scratchvj` ne fait que la vidéo. Comme les deux lisent le
**même** timecode, la position vidéo suit la position audio.

- **Ancrage** par deck : `offset = position_timecode − position_vidéo`, posé d'un
  coup de pad au moment voulu.
- **Fraîcheur** affichée en permanence — et non une dérive en secondes, qui serait
  inventée : voir l'encadré ci-dessous.
- **Re-ancrage** après tout ce qui casse la correspondance : needle drop, boucle,
  censor, ou passage en mode relatif dans Serato.

> Serato n'a **pas d'API publique** : impossible de lire sa vraie tête de lecture.
> L'ancrage n'est pas un raccourci, c'est le seul mécanisme possible. D'où
> l'exigence d'ergonomie : le re-ancrage doit être à un pad de distance, jamais
> caché dans un menu.
>
> **Et pour la même raison, la dérive n'est pas mesurable.** Si le DJ boucle ou
> pose l'aiguille dans Serato, l'audio bouge et le timecode ne bouge pas : la
> correspondance casse sans rien d'observable de notre côté. Un nombre de secondes
> affiché serait donc inventé. On affiche une **fraîcheur** — le temps écoulé
> depuis la pose et le nombre de discontinuités vues — c'est-à-dire un risque et
> non une erreur.

Contrainte technique : l'entrée audio est ouverte **en lecture seule et jamais en
exclusif**. Ouvrir le périphérique en exclusif dans ce mode empêcherait Serato de
fonctionner — c'est l'erreur à ne pas commettre.

### Mode autonome (l'app joue le son)

Boucle DVS complète : entrées USB 1/2 et 3/4 en PHONO (timecode), sorties USB 1/2
et 3/4 vers les canaux de l'Elite. Périphérique ouvert en **duplex exclusif**.
Synchronisation parfaite par construction, et possibilité de scratcher un clip qui
n'a aucun équivalent dans Serato.

## Le problème du partage d'entrée sous Windows

En mode suiveur, sur Windows, les pilotes ASIO DJ sont généralement **mono-client** :
si Serato tient le périphérique, `scratchvj` ne peut pas l'ouvrir. Quatre issues,
par ordre de préférence :

| Voie | Détail | Verdict |
|---|---|---|
| **2ᵉ port USB de l'Elite** | Interface double 10×10 : Serato sur USB-B1, l'app sur USB-B2, les deux reçoivent les entrées phono | Matériel déjà possédé — **à tester en premier** |
| **Split RCA passif** | Sortie du Phase dupliquée vers une petite interface d'entrée dédiée | Fonctionne toujours, ~50 € |
| **WASAPI partagé** | Si le pilote Reloop expose WASAPI à côté d'ASIO | Dépend du pilote : à tester, pas à présumer |
| **Deux machines** | Laptop Serato + PC vidéo | Le repli qui marche toujours |

**Sur macOS le problème n'existe pas** : CoreAudio est multi-client, l'application
ouvre la même entrée que Serato sans disposition particulière. C'est un avantage
réel du Mac pour ce projet.

## Réglages de l'Elite

- Utilities → basculer les canaux concernés en **PHONO** pour recevoir le timecode.
- Utilities → **USB OUT ROUTING** : à vérifier pour savoir si le timecode part vers
  les deux ports USB simultanément. C'est le test qui tranche le tableau ci-dessus.

## Budget de latence

| Étage | Ordre de grandeur |
|---|---|
| Buffer audio (128 échantillons @ 48 kHz) | 2,7 ms |
| Pilote (WASAPI partagé) | ~10 ms |
| UDP vers Unreal | < 1 ms |
| Une frame Unreal (60–120 fps) | 8–16 ms |

Sous **30 ms** l'ensemble est bon visuellement ; sous **20 ms** ça « colle » à la
main. WASAPI exclusif ou ASIO si nécessaire.

**Méthode de mesure** : filmer main et écran au ralenti, compter les frames d'écart.
Rien d'autre ne donne le chiffre réel de bout en bout.
