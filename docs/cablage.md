# Câblage et modes audio

## La chaîne complète : Phase + Elite + Serato (établie le 2026-09-07)

Documentation constructeur (MWM, Reloop, VirtualDJ) recoupée avec le matériel
réel branché. Deux corrections sur ce qui était supposé avant :

1. **Le récepteur Phase sort au niveau LIGNE, pas phono**, et se branche sur les
   entrées **LINE** du mixeur. Il n'y a pas de disque de contrôle : le dock
   synthétise le signal à partir du mouvement des remotes posées sur n'importe
   quel vinyle.
2. **Le sélecteur de voie de l'Elite choisit ce qu'on écoute, pas ce que
   l'ordinateur reçoit.** En DVS il reste sur **USB A/B** ; le trajet entrée
   analogique → USB est interne, réglé par **Utilities → USB OUT ROUTING**
   (SHIFT + BACK tenus 3 s) : DECK 1 / DECK 2 sur **CD** (= entrée ligne), pas
   PHONO, puisque le Phase entre en ligne.

**Le Phase a deux modes de liaison, et ils s'excluent :**

- **Mode HID** : le récepteur se branche en USB directement au laptop, Serato
  règle le deck sur « WIR », aucun câble RCA. Intégration native, mais fermée —
  Serato (et rekordbox) seulement, et **rien à lire pour nous** : pas de signal
  analogique.
- **Mode DVS/RCA** : le récepteur est alimenté en USB (5 V), ses RCA vont aux
  entrées LINE de l'Elite, les voies restent sur USB A/B, USB OUT ROUTING sur
  CD, et Serato lit le timecode. C'est **le mode qu'il faut à scratchvj** : un
  signal analogique existe, et deux lecteurs peuvent le décoder en parallèle.

**Réglages Serato officiels pour cette chaîne** (doc MWM) : decks virtuels en
**REL**, et — cité tel quel — « For some specific mixers (DJM-S11, **Reloop
Elite**, etc.), you need to go to Serato settings > Audio tab > Click on CDJ. »

> **REL n'est pas un détail : c'est une propriété du signal.** Le timecode
> synthétisé par le Phase n'a pas de position absolue significative — il avance
> depuis un point arbitraire. Serato le prescrit donc en mode relatif, et
> `scratchvj` doit faire pareil : c'est exactement ce que le profil `wireless`
> de `core/timecode` anticipe, et ça confirme que l'ancrage par fraîcheur (et
> non par position absolue) était le bon choix. Un needle drop absolu n'existe
> pas dans cette chaîne.

**La table n'est pas obligatoire.** Le timecode est un signal analogique, donc il
lui faut *une* entrée audio — mais n'importe laquelle. Le récepteur (alimenté en
5 V USB) peut entrer directement dans une interface ligne quelconque, la MOTU du
studio par exemple, sans toucher à l'Elite. Trois câblages selon l'usage :

| Usage | Câblage |
|---|---|
| Vidéo seule | Récepteur Phase → RCA → entrées ligne de n'importe quelle interface (MOTU). Pas de table. |
| Vidéo seule + surface de contrôle | Pareil, plus l'Elite en USB comme simple surface MIDI — aucun routage audio. |
| Serato + vidéo (mode suiveur) | Récepteur → RCA → LINE de l'Elite ; Serato sur USB-B1, scratchvj sur USB-B2. La table ne sert au timecode que parce que Serato doit lire le même signal. |

(Le « zéro câble » serait le mode HID, mais son protocole est fermé — ouvert par
MWM à Serato et rekordbox seulement. Le lire relèverait de la rétro-ingénierie :
projet à part, résultat non garanti, et le RCA marche aujourd'hui.)

**Le format du signal se choisit dans Phase Manager** (Configuration DVS :
Serato DJ, Traktor, rekordbox, VirtualDJ…). Pour ce projet, **rester sur
« Serato DJ » dans tous les cas** :

- en mode suiveur, c'est forcé — Serato ne lit que son propre signal ;
- en mode autonome, le `timecoder.c` de xwax décode le format Serato nativement
  (profils `serato_2a`/`serato_cd` — lequel se verrouille sera mesuré, pas
  deviné) ;
- un seul réglage pour les deux modes = zéro manipulation en passant de l'un à
  l'autre.

Sources : [setup Phase + Serato (MWM)](https://www.phasedj.com/phase-essential/set-up/serato),
[setup Phase + Traktor (MWM)](https://www.phasedj.com/phase-essential/set-up/traktor),
[Elite advanced setup (VirtualDJ)](https://virtualdj.com/manuals/hardware/reloop/elite/advanced.html),
[manuel Reloop Elite](https://www.manualslib.com/manual/1613440/Reloop-Elite.html).

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

Boucle DVS complète : entrées USB 1/2 et 3/4 en CD/LINE (le timecode du Phase entre au niveau ligne), sorties USB 1/2
et 3/4 vers les canaux de l'Elite. Périphérique ouvert en **duplex exclusif**.
Synchronisation parfaite par construction, et possibilité de scratcher un clip qui
n'a aucun équivalent dans Serato.

## Le problème du partage d'entrée sous Windows

En mode suiveur, sur Windows, les pilotes ASIO DJ sont généralement **mono-client** :
si Serato tient le périphérique, `scratchvj` ne peut pas l'ouvrir. Quatre issues,
par ordre de préférence :

| Voie | Détail | Verdict |
|---|---|---|
| **2ᵉ port USB de l'Elite** | Interface double 10×10 : Serato sur USB-B1, l'app sur USB-B2, les deux reçoivent les entrées phono | Matériel déjà possédé — **à tester en premier**. Au 2026-09-07 : toujours pas essayé, un seul câble branché (Windows ne voit qu'une instance de l'Elite). C'est un câble de distance. |
| **Split RCA passif** | Sortie du Phase dupliquée vers une petite interface d'entrée dédiée | Fonctionne toujours, ~50 € |
| **WASAPI partagé** | Si le pilote Reloop expose WASAPI à côté d'ASIO | Dépend du pilote : à tester, pas à présumer |
| **Deux machines** | Laptop Serato + PC vidéo | Le repli qui marche toujours |

**Sur macOS le problème n'existe pas** : CoreAudio est multi-client, l'application
ouvre la même entrée que Serato sans disposition particulière. C'est un avantage
réel du Mac pour ce projet.

## Réglages de l'Elite

- Utilities → USB OUT ROUTING des decks concernés sur **CD** — le Phase entre au niveau ligne. (PHONO ne servirait qu'avec de vraies cellules et un disque de contrôle.)
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
