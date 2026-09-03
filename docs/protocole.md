# Protocole réseau

`scratchvj` diffuse l'état de la surface et des platines vers Unreal (et vers tout
autre client : TouchDesigner, Resolume) par UDP.

## Pourquoi pas le plugin OSC natif d'Unreal

Il alloue des UObjects par message et dispatche sur le game thread. À ~375 Hz
— un paquet par bloc audio — c'est de la garbage pour rien. On lit donc un socket
UDP sur un thread dédié avec une charge utile de disposition fixe. Un **miroir OSC**
tourne en parallèle pour les outils tiers, qui eux n'ont pas ce problème d'échelle.

## Deux types de paquets

Le flux à haut débit ne transporte **que des valeurs**, jamais de chaînes.

### `Schema` — rare

Envoyé au démarrage et à chaque changement de la liste des contrôles. Il **nomme**
les valeurs : identifiant et type de chaque contrôle, dans l'ordre.

```
u32 magic "SVJ1" | u16 version | u16 kind=2
u32 schema_hash
u16 count
count × { u8 kind | u8 id_len | id_len octets }
```

### `State` — à chaque bloc audio

```
u32 magic "SVJ1" | u16 version | u16 kind=1
u64 t_us                      horloge monotone de l'émetteur
u32 schema_hash               identifie la disposition de `values`
deck A : 7 × f32              pos_s, velocity, acceleration, scratch_rate,
deck B : 7 × f32              confidence, anchor_s, drift_s
u32 gesture_bits
u16 count
count × f32                   valeurs, dans l'ordre du schéma
ceil(count/8) octets          bitset `known` : faux = jamais touché
```

Tout est en **petit-boutien**, écrit octet par octet — pas de `struct` transtypée,
donc pas de surprise d'alignement entre compilateurs.

## Le `schema_hash`

Hash FNV-1a de la liste ordonnée des identifiants et des types. **L'ordre compte** :
c'est ce qui fait du hash une promesse sur la signification de chaque flottant du
paquet d'état. Le récepteur met le schéma en cache et, quand le hash d'un paquet
d'état ne correspond plus, il sait que son cache est périmé et attend le nouveau
schéma plutôt que d'interpréter des valeurs de travers.

## Bits de geste

| Bit | Sens |
|---|---|
| `ScratchingA` / `ScratchingB` | la platine est travaillée, loin de la vitesse nominale |
| `BackspinA` / `BackspinB` | marche arrière rapide et soutenue |
| `HoldingA` / `HoldingB` | **lock timecode perdu, sortie gelée** |
| `Transform` | coupes rapides au crossfader, platine en marche |

Les bits `Holding` méritent une mention particulière : un décrochage du Phase doit
**geler** l'image et la caméra, jamais les téléporter. Le suiveur de gestes ne
décroît même pas vers zéro dans ce cas, parce que cela bougerait encore l'image.
Un client qui ignore ce bit produira un artefact visible en live.

## Robustesse du décodage

Le décodeur rejette : un magic ou une version inconnus, un paquet tronqué à
n'importe quelle longueur, et un champ `count` que le datagramme ne peut pas
satisfaire — cette dernière vérification a lieu **avant** toute allocation, pour
qu'un paquet corrompu ne provoque pas une réservation démesurée.

## Échantillonnage côté Unreal

Le flux arrive vers 375 Hz, Unreal tourne à 60–120 fps. Le plugin conserve les
derniers paquets et échantillonne à `now − offset` avec une interpolation d'Hermite
sur la position — on dispose de la vitesse, autant s'en servir. C'est le modèle
Live Link, sans le coût d'en écrire une source complète.
