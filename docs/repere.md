# Le repère temporel de la ligne scène

Écrit le 2026-09-10, phase 3 du talon.

**Ce document ne tranche pas, et c'est sa consigne.** La question est au
fondateur : `maison/04-CE-QUI-EST-OUVERT.md` range le repère temporel de la ligne
scène parmi ce qui ne se décide pas dans un dépôt, et le registre de la maison
l'inscrit déjà comme ouvert, « sur recommandation de scratchvj ». Ce qui suit est
donc la recommandation : les options, leurs conséquences mesurées sur ce code, et
un avis argumenté. Rien n'a été implémenté sur la foi de cet avis.

C'est le pendant temporel de `spec/02-repere.md` — qui se lit à l'adresse de la
maison, jamais recopié ici, et qui ne règle que la **géométrie** : OpenXR, main
droite, `−Z` devant. Un instrument joué a un second repère, et personne ne l'a
écrit.

---

## La question, en une phrase

> **Quand le tempo, l'horloge audio et la position du plateau ne disent pas la
> même chose, lequel fait autorité ?**

Elle n'est pas théorique : les trois existent dans ce dépôt, chacun est déjà
consommé par une partie du moteur, et **aucune ligne n'écrit lequel gagne**.
Trois consommateurs les mélangent aujourd'hui sans dire pourquoi — la phase d'un
LFO, la division d'un effet, et la longueur d'une boucle en temps musicaux.

---

## Les trois candidats, tels qu'ils existent

### 1. Le tempo — `core/transport`, `BeatGrid`

Un BPM et un premier temps. Il sert à quantifier : une boucle de 4 temps, un saut
d'un demi-temps, une division d'effet, la phase d'un LFO synchronisé.

**Ce qu'il est aujourd'hui, et c'est le point le plus important de ce
document : un nombre constant.** Le tempo du poste est fixé une fois au
démarrage (`kBpm`, dans `ui/main_ui`), tout clip l'hérite à son chargement, et
la grille de chaque deck est construite avec un **premier temps à zéro**. Il
n'existe ni détection de tempo dans la passe d'analyse, ni saisie de tempo dans
l'interface, ni tempo par clip lu depuis un fichier. Le « 124.0 BPM » de la barre
haute est donc exact au sens où c'est bien la valeur employée, et sans objet au
sens où elle ne mesure rien.

Cela veut dire qu'aujourd'hui **le tempo ne peut pas diverger** des deux autres :
il ne suit rien, donc il ne se désynchronise de rien. La divergence apparaîtra le
jour où il sera mesuré ou saisi — c'est-à-dire au premier set réel avec deux
clips de tempos différents.

### 2. L'horloge audio — `core/spectrum`, `ui/audio_in`

Le temps du flux d'entrée : les blocs WASAPI qui arrivent, leur cadence, et la
FFT qui en sort les bandes réactives. C'est aussi elle qui porte la porteuse du
plateau avant que `core/quadrature` ne la démodule.

Elle a une propriété qu'aucune des deux autres n'a : **elle est la seule à être
imposée de l'extérieur.** Une carte son avance à son rythme, et rien dans
l'application ne peut la ralentir ni la rattraper. Le moteur la voit aujourd'hui
sous la forme du temps mur (`frame.time_s`, `frame.dt_s`), qui est le temps de la
boucle de rendu et non celui de la carte — les deux dérivent l'un par rapport à
l'autre, lentement, et personne ne mesure de combien.

### 3. La position du plateau — `core/quadrature` → `core/timecode` → `core/playback`

La position que la main demande. C'est la seule des trois qui soit une **fonction
de la position** au sens du premier principe de l'instrument : elle ne s'intègre
pas, elle se lit. Elle peut reculer, s'arrêter, sauter, et le moteur est
construit pour que cela reste normal.

Elle a la propriété inverse de l'horloge audio : **elle est la seule que le corps
de l'utilisateur produise**, et donc la seule dont un écart se voit
immédiatement dans la salle.

---

## Où les trois se rencontrent déjà, dans ce code

Trois endroits, et ils ne font pas le même choix. C'est ce désaccord silencieux
qui rend la question urgente plutôt qu'académique.

| Où | Ce qui fait autorité aujourd'hui | Ce que ça donne |
|---|---|---|
| La phase d'un LFO synchronisé (`app/engine`, `core/modulator`) | **le plateau**, converti en temps musicaux par le tempo | un LFO tempo-synchronisé suit le plateau, y compris à l'envers. C'est écrit et motivé dans le code : « la phase est *dérivée* plutôt que comptée » |
| La division d'un effet synchronisé (`core/effect`, `Sync`) | **le tempo** seul | un délai calé sur le tempo garde sa durée pendant qu'on scratche : il ne suit pas la main |
| Le suiveur d'enveloppe (`core/modulator`, `EnvelopeFollower`) | **l'horloge** — un `dt` en secondes | il suit le son qui arrive, pas la position jouée |

Les trois choix sont défendables séparément. Ensemble ils décrivent une machine
où, pendant un backspin, le LFO recule, le délai continue tout droit et
l'enveloppe suit l'audio de Serato : trois temps dans la même image, et rien
n'écrit que c'est voulu.

Un quatrième point mérite d'être nommé parce qu'il a déjà été tranché, et dans
l'autre sens : `core/anchor` refuse de mesurer une dérive. Serato n'expose pas sa
tête de lecture, donc un nombre de secondes appelé « dérive » serait inventé, et
le module rapporte une **péremption** — un risque — au lieu d'une erreur. C'est
le seul endroit du dépôt où la question « lequel a raison ? » a reçu une réponse
écrite, et la réponse est *aucun, et on le dit*.

---

## Les trois options

### Option A — le plateau fait autorité

Tout ce qui est temporel se dérive de la position jouée du deck de référence. Le
tempo devient une **unité de conversion** (combien de secondes vaut un temps), et
l'horloge audio un simple fournisseur d'échantillons.

**Ce que ça donne.** Tout suit la main. Un backspin fait reculer le délai, le
LFO, la grille. L'instrument devient littéralement une fonction de la position —
le premier principe étendu à tout ce qui bouge.

**Ce que ça coûte.** Un effet temporel qui recule n'a pas d'équivalent audio :
un délai dont la ligne de retard se lit à l'envers n'est plus un délai. Il
faudrait donc soit accepter que la correspondance audio/vidéo se rompe pendant le
scratch — ce que `core/effect` marque déjà comme un cas nommé quand la
correspondance n'est pas exacte — soit exempter les effets, ce qui rouvre la
question pour eux seuls. Et le deck de référence doit être choisi : il n'y en a
qu'un, et il y a deux platines plus une incrustation.

### Option B — le tempo fait autorité

Une horloge musicale unique, posée sur le poste, à laquelle tout se raccroche.
Le plateau donne la position de l'image, l'horloge audio donne les
échantillons, mais **le temps musical, lui, ne recule jamais**.

**Ce que ça donne.** C'est le modèle des logiciels de VJing et des séquenceurs, et
c'est celui qu'Ableton Link parlerait si on l'ouvrait un jour — le roadmap le
range en « reporté ». Un set à plusieurs machines devient possible sans rien
réécrire. Les effets synchronisés se comportent comme leurs équivalents audio,
puisque c'est déjà ce qu'ils font.

**Ce que ça coûte.** Le temps musical devient un **intégrateur**, et le premier
principe de l'instrument l'interdit nommément : le roadmap écrit qu'une horloge
libre « ne se scratche pas, ne se rejoue pas à l'identique et dérive sur la durée
d'un set », et `core/playback` a été construit pour l'éviter. Il faudrait la
forme close du temps absolu appliquée au temps musical, ce qui est faisable, et
accepter que la grille et l'image se décalent visiblement pendant un long
scratch.

### Option C — chacun fait autorité sur son domaine, et la frontière s'écrit

Trois domaines déclarés, une règle par frontière :

- **la position de l'image** appartient au plateau, sans exception ;
- **le temps musical** appartient au tempo ;
- **les grandeurs réactives** appartiennent à l'horloge audio.

Et une règle de conversion écrite une fois : *ce qui se convertit d'un domaine à
l'autre le fait en un seul endroit, et cet endroit dit dans quel sens.*

**Ce que ça donne.** C'est, à peu de chose près, ce que le code fait déjà — mais
sans l'avoir écrit, donc sans pouvoir le vérifier ni l'expliquer. Formaliser
coûte peu et rend chacun des trois comportements ci-dessus défendable au lieu
d'accidentel.

**Ce que ça coûte.** Ça ne répond pas à la question posée. Le jour où deux
domaines se contredisent *sur le même objet* — un LFO synchronisé pendant un
backspin, exactement — il faut quand même dire lequel gagne, et l'option C
repousse cette décision au lieu de la prendre.

---

## Ma recommandation

**L'option C, avec l'option A comme règle d'arbitrage aux frontières.**

Trois raisons, dans l'ordre de leur poids.

1. **Le premier principe de l'instrument désigne déjà un vainqueur.** « Tout ce
   qui doit être scratchable est une fonction de la position, jamais un
   intégrateur » : là où un objet est scratchable, la position gagne, et ce n'est
   pas un arbitrage nouveau, c'est le principe appliqué. L'option B demanderait
   de l'affaiblir pour un bénéfice — la synchronisation entre machines — dont
   aucun set n'a encore eu besoin.

2. **La divergence n'existe pas encore, et c'est le bon moment pour écrire la
   règle et non le mécanisme.** Le tempo est constant, donc rien ne dérive
   aujourd'hui. Écrire la frontière maintenant coûte un document ; l'écrire après
   la détection de tempo coûterait de défaire trois consommateurs qui auront
   chacun pris leur habitude.

3. **Une salle voit une image, pas une grille.** La preuve à fournir sur cette
   ligne est la tenue, jamais l'exactitude — c'est ce que `design/SCENE.md` oppose
   à l'atelier. Un temps musical juste sous une image en retard est un défaut
   visible ; l'inverse ne l'est pas.

**Ce que la décision doit produire, quel qu'en soit le sens** — et c'est ce qui
manque le plus, plus encore que le choix lui-même :

- **le nom du deck de référence**, ou la règle qui le désigne. Deux platines et
  une incrustation, un seul temps musical : quelque chose doit choisir, et
  aujourd'hui c'est le deck A par une constante dans `app/engine`, sans une ligne
  pour le dire ;
- **le comportement pendant un backspin**, énoncé comme une phrase vérifiable en
  trente secondes devant l'écran, sur le modèle de celle que la maison emploie
  pour le glisser du viseur ;
- **le format d'affichage d'un temps**, qui en est une conséquence directe et non
  un sujet à part — voir ci-dessous.

---

## Ce qui dépend de cette décision, et attend

**Le format d'un temps.** `ERGONOMIE.md` prescrit `hh:mm:ss:ff` ; ce produit
écrit `mm:ss.d`. La maison a **reporté** la question (`SCRATCHVJ-14`) en disant
exactement pourquoi : le format d'un temps est une conséquence du repère
temporel, pas un sujet à part. Un `ff` est un numéro d'image, donc une position
dans un clip ; un `mm:ss.d` est une durée. Les deux ne mesurent pas la même
chose, et lequel s'affiche dépend de ce qui fait autorité. **Rien ne change
d'ici là** : `mm:ss.d` reste tel quel, et la ligne reste *remontée*.

**La détection de tempo.** Elle n'existe pas, et elle n'est programmée par aucune
phase du talon : c'est une fonctionnalité, elle va au carnet du produit, pas à un
alignement. Mais elle est le premier consommateur de cette décision — une grille
mesurée par clip n'a de sens que si l'on sait ce qu'elle gouverne.

**Le clavier réservé de la ligne scène.** Sans rapport direct, sauf sur un point :
si la décision introduit un geste (poser un temps, resynchroniser), il lui
faudra une touche, et la ligne scène n'a pas encore de clavier réservé à elle —
`design/SCENE.md` le range dans ce qui attend un second produit.

---

## Ce qui ne dépend pas de la décision, et qui est fait

- **Ce document existe.** C'est ce que la phase 3 demandait, et le talon note que
  c'est le seul document de sa liste à écrire même si aucun second produit
  n'arrivait jamais.
- **Les trois consommateurs sont nommés**, avec ce que chacun prend pour
  autorité : la phase d'un LFO, la division d'un effet, le suiveur d'enveloppe.
  La prochaine session n'a pas à les retrouver.
- **Le tempo est constant, et c'est écrit ici.** C'était la seule chose à
  mesurer avant de pouvoir recommander quoi que ce soit, et elle change la forme
  de la question : il ne s'agit pas de réparer une divergence, il s'agit
  d'écrire une règle avant qu'elle n'apparaisse.
- **Le refus de `core/anchor`** — rapporter une péremption plutôt qu'une dérive
  inventée — est le précédent de la maison sur ce sujet, et il est cité ici pour
  qu'on ne le redécouvre pas.

**Ce qui n'est pas fait, et pourquoi.** Le choix. Il est au fondateur, et une
décision prise dans ce dépôt n'atteindrait pas le produit de scène suivant.
