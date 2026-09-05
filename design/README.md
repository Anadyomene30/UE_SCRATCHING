# Maquette de l'interface

Les artboards de l'interface de `scratchvj`, en fichiers de travail. Ce sont eux
qui font foi : le canvas publié est régénéré à partir d'eux, jamais l'inverse.

| Fichier | Écran |
|---|---|
| `Main.dc.html` | Mode performance — l'écran principal |
| `Complet.dc.html` | Mode complet — le second écran (effets, mapping, analyse) |
| `Effets.dc.html` | Panneau effets, avec la batterie complète |
| `Mapping.dc.html` | Panneau mapping et apprentissage MIDI |
| `Sortie.dc.html` | Choix de l'écran, corner pin, masque |
| `canvas.json` | Disposition des artboards et notes sur le canvas |

## Ce que la maquette fixe

La disposition et la hiérarchie visuelle ; le code fixe le comportement. Quatre
décisions y sont visibles plutôt qu'écrites :

- **Contrôles fantômes** — les potards de l'Elite sont absolus, donc leur position
  est inconnue au lancement. Ils s'affichent en pointillé au lieu d'une valeur
  inventée.
- **Liaison Phase** dans la barre de statut — le dock émet même plateau arrêté,
  donc un silence veut dire liaison perdue et non plateau arrêté. Sans cet
  indicateur, une batterie vide et un bug sont indiscernables en plein set.
- **Fenêtre VRAM** matérialisée sur le filmstrip — la portion instantanément
  scratchable, donc jusqu'où partir sans déclencher un chargement.
- **Lien des effets** — seul l'état *délié* est signalé, puisque c'est le seul où
  audio et vidéo racontent des choses différentes.

Les valeurs affichées sont un état de jeu plausible, pas des données réelles.

## Régénérer le canvas

Les artboards ne s'ouvrent pas seuls : ils sont assemblés en une page unique par
l'assistant de la compétence `design`, qui y intègre l'éditeur. Depuis ce dossier,
invoquer `/design` puis relancer son helper avec les six fichiers ci-dessus. La
sortie (`maquette-scratchvj.html`, ~2,5 Mo) est ignorée par git : c'est un artefact
de build.
