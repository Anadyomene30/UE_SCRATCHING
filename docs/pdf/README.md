# Les deux PDF

- `scratchvj-manuel.pdf` — le manuel de l'instrument (33 pages).
- `scratchvj-argumentaire.pdf` — le dossier de positionnement commercial (13 pages).

Les sources sont les `.html` de ce dossier, plus `_style.css`. Ce sont eux qu'on
modifie ; les PDF sont des artefacts.

## Régénérer

```sh
python docs/pdf/build.py
```

Le script imprime les deux HTML avec **Chrome en mode headless** — la seule chaîne
PDF disponible sur cette machine (ni pandoc, ni weasyprint, ni reportlab). Il faut
donc Chrome installé ; le chemin est cherché aux emplacements Windows habituels et
peut être forcé par la variable d'environnement `CHROME`.

`--allow-file-access-from-files` est nécessaire : sans lui, le `<link>` vers
`_style.css` et les `@font-face` en `file://` sont ignorés en silence, et le PDF sort
en Times New Roman sans qu'aucune erreur ne soit émise.

## Les annexes du manuel sont générées, pas recopiées

Les deux tableaux d'annexe — les 63 contrôles et les 32 effets — sont produits par
`build.py` depuis la sortie de `scratchvj layout` et `scratchvj effects`. C'est
délibéré : le catalogue imprimé ne peut pas diverger du code, pour la même raison que
`core/effect.h` tient la prose et l'implémentation dans le même fichier.

Le script a donc besoin du binaire. Il cherche `scratchvj.exe` dans les répertoires
de build habituels ; **recompiler d'abord**, sinon les annexes décrivent un état
périmé — c'est exactement le piège que `CLAUDE.md` documente pour les `*_check`.

## Pourquoi le corps de texte n'est pas en Archivo

`scratchvj/ui/fonts/Archivo-Variable.ttf` est refusé par le moteur d'impression de
Chrome : le fichier est valide et l'application le charge sans problème, mais
`DMMono-Regular.ttf` du même dossier passe alors qu'Archivo retombe sur la police
système. Le corps est donc composé dans la pile système (Segoe UI sur Windows), que
Chrome incorpore en sous-ensemble dans le PDF — le rendu est donc identique partout.
DM Mono, lui, est bien incorporé et porte l'identité du projet sur les chiffres, le
code et les intertitres.

## Tenir les deux documents à jour

Les deux partagent la même échelle d'état — *vérifié* / *écrit, non éprouvé* / *à
venir* — et elle doit rester alignée sur `docs/roadmap.md`, qui fait foi. Quand un
module change d'état dans le roadmap, les deux PDF sont à régénérer.
