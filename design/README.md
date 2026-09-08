# Maquette de l'interface

Les artboards de l'interface de `scratchvj`, en fichiers de travail. Ce sont eux
qui font foi : le canvas publié est régénéré à partir d'eux, jamais l'inverse.

Cette version est la **refonte de septembre 2026** (voir la section « Refonte de
l'ergonomie » du roadmap) : un seul axe de navigation, le deck comme lecteur, le
mixer entre les deux decks, les pads à l'écran.

| Fichier | Écran |
|---|---|
| `Main.dc.html` | JOUER — l'écran de set (deck A · mixer · deck B, programme en bas) |
| `Montage.dc.html` | JOUER, direction B « montage » — basse fidélité, non retenue, gardée pour mémoire |
| `Bibliotheque.dc.html` | BIBLIOTHÈQUE — dossiers et caisses, liste avec vignettes, inspecteur, banques de pads |
| `Effets.dc.html` | EFFETS — le rack en trois cartes, catalogue, spectre, réactif au son |
| `Table.dc.html` | TABLE — le panneau mesuré dessiné depuis le profil, la face avant, la liste des liaisons |
| `Sortie.dc.html` | SORTIE — écrans, sorties réseau, géométrie appliquée à l'écran de sortie |
| `Composants.dc.html` | Le vocabulaire : bouton, sélecteur segmenté, basculeur, transport, barre de position, pads, voyant, fantôme |
| `canvas.json` | Disposition des artboards et notes sur le canvas |

## Ce que la maquette fixe

La disposition et la hiérarchie visuelle ; le code fixe le comportement. Les
décisions y sont visibles plutôt qu'écrites :

- **Lecteur d'abord** — chaque deck a ⏮ ▶ ⏸ ⏹ et une barre de position qu'on
  glisse. La platine est une source parmi trois (Platine · Lecture · Tempo),
  proposée seulement si elle est branchée. Un clip chargé joue.
- **Un seul axe** — six onglets, plus de dispositions. « Image seule » (F) pour
  la scène. Le 360 est une rangée qui apparaît sur le deck dont le clip est
  sphérique, pas un écran.
- **Trois familles de contrôles** — bouton, sélecteur segmenté, basculeur. Tout
  ce qui se clique a un fond et un bord ; l'orange plein est réservé à une seule
  action par panneau.
- **Contrôles fantômes** — gardés de l'ancienne interface : les potards de
  l'Elite sont absolus, donc leur position est inconnue au lancement. Ils
  s'affichent en pointillé au lieu d'une valeur inventée.
- **Lien des effets** — seul l'état *délié* est signalé en rouge, puisque c'est
  le seul où audio et vidéo racontent des choses différentes.
- **Diagnostics en tiroir** — position, vitesse, confiance, scratch/s vivent
  sous « diagnostic platine », replié sauf quand la platine est en direct.

Les jetons (couleurs, polices, hauteurs) sont ceux d'`apply_style` dans
`scratchvj/ui/main_ui.cpp`, pas des arrondis. Les valeurs affichées sont un
état de jeu plausible, pas des données réelles.

## Régénérer le canvas

Les artboards ne s'ouvrent pas seuls : ils sont assemblés en une page unique par
l'assistant de la compétence `design`, qui y intègre l'éditeur. Depuis ce dossier,
invoquer `/design` puis relancer son helper avec les sept artboards et
`canvas.json`. La sortie (`interface-scratchvj.html`, ~2,5 Mo) est ignorée par
git : c'est un artefact de build.
