# Les deux PDF

- `scratchvj-manuel.pdf` — le manuel de l'instrument.
- `scratchvj-argumentaire.pdf` — le dossier de positionnement commercial.

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

Les deux tableaux d'annexe — les contrôles et les effets — sont produits par
`build.py` depuis la sortie de `scratchvj layout` et `scratchvj effects`. *Leur
nombre ne s'écrit pas ici : ce sont ces deux commandes qui le donnent, et elles ne se
périment pas. Le 2026-09-10, la régénération a fait passer l'annexe des contrôles de
63 lignes à 141 — le catalogue imprimé avait divergé du code, ce que ce mécanisme
existe pour empêcher.* C'est
délibéré : le catalogue imprimé ne peut pas diverger du code, pour la même raison que
`core/effect.h` tient la prose et l'implémentation dans le même fichier.

Le script a donc besoin du binaire. Il cherche `scratchvj.exe` dans les répertoires
de build habituels ; **recompiler d'abord**, sinon les annexes décrivent un état
périmé — c'est exactement le piège que `CLAUDE.md` documente pour les `*_check`.

## Le corps de texte est en Archivo — et ce qui l'en empêchait

**Ce n'était ni la fonte ni le chemin.** Cette page portait, jusqu'au
2026-09-10, la conclusion inverse : « Archivo-Variable.ttf est refusé par le
moteur d'impression de Chrome ». Elle était fausse, et elle l'était pour la
raison qui compte le plus ici — **la façon de vérifier**.

**Ce qui se passait vraiment.** `build.py` imprimait dès que Chrome croyait la
page prête. Les `@font-face` de `fonts-inline.css` sont en `font-display: block`,
ce qui rend le texte **invisible** pendant la période de blocage au lieu de le
composer dans un repli. Le PDF sortait donc sans le corps de texte, sans aucune
erreur : l'argumentaire faisait 14 pages et **65 Ko au lieu de 326**. Un seul
drapeau le répare, et il est dans `build.py` avec sa raison :
`--virtual-time-budget=10000`.

**Pourquoi la vérification avait menti.** Chercher `/BaseFont` dans les octets du
PDF répond *absent* pour une fonte **variable** : un moteur de navigateur en émet
une instance en `/Subtype /Type3` — des procédures de dessin — **sans
`/BaseFont`**. Le piège est **asymétrique**, et c'est ce qui le rend
convaincant : Fragment Mono n'est pas variable et sort en `/BaseFont` bien
visible, donc une session qui trouve l'une et pas l'autre conclut logiquement que
la seconde manque. `maison/03-DEFINITION-DE-FINI.md` porte désormais la règle :
« **ouvre** » est le mot, et ce qui se vérifie sans ouvrir, c'est l'**absence**
d'un repli — jamais la présence d'une variable.

**Ce que les deux PDF portent aujourd'hui**, mesuré le 2026-09-10 au soir, après
le re-sous-ensemblage d'Archivo :

| | Manuel | Argumentaire |
|---|---|---|
| poids | 1 280 Ko | 603 Ko |
| objets `Type3` — les instances d'Archivo | 47 | 38 |
| `DMMono` en `/BaseFont` | oui | oui |
| `Times`, `Arial` | aucun | aucun |
| `SegoeUI` | **1 caractère** | aucun |
| `Consolas` | **1 caractère** | aucun |

Cinq des six caractères qui tombaient hors des sous-ensembles sont réparés :
`←` `→` `↔` `√` `≥` sortent maintenant en Archivo (`SCRATCHVJ-32`, tranché).
Restent **deux** caractères, et ni l'un ni l'autre n'est un mot :

| Caractère | Point de code | Repli | Pourquoi |
|---|---|---|---|
| `ᵉ` | U+1D49 | Segoe UI | Archivo n'a pas l'exposant — **attendu**, c'est le verdict |
| `₀` | U+2080 | Consolas | jamais vu jusqu'ici : on cherchait `SegoeUI` et `Times`, pas `Consolas` (`../REMONTEES.md`, `SCRATCHVJ-34`) |

**Le poids est la première chose à regarder**, et avant les fontes : un PDF dont
le corps de texte manque ne proteste pas, il maigrit — 65 Ko au lieu de 326, quatorze
pages blanches. Un chiffre qui s'effondre est le seul symptôme.

**Vérifier soi-même**, sans rien installer — et en cherchant ce qui NE doit PAS
être là, pas ce qui doit y être. Noter que la liste des replis à chercher est
**plus longue que celle que la maison nomme** : Chromium retombe aussi sur
`Consolas` pour une chasse fixe et sur `CambriaMath` pour un symbole.

```sh
python -c "d=open('docs/pdf/scratchvj-manuel.pdf','rb').read();print(len(d),*[(n,d.count(n.encode())) for n in ('/Type3','SegoeUI','Times','Arial','Consolas','CambriaMath')])"
```

Ce compte-là reste **grossier** : il compte des octets, et il ne voit pas les
descripteurs qui vivent dans les flux d'objets compressés. Le compte juste se
lit en dépouillant la table `/ToUnicode` de chaque objet de fonte, ce qui donne
le caractère exact au lieu d'une occurrence — c'est ce qui a trouvé `₀`.

## Tenir les deux documents à jour

Les deux partagent la même échelle d'état — *vérifié* / *écrit, non éprouvé* / *à
venir* — et elle doit rester alignée sur `docs/roadmap.md`, qui fait foi. Quand un
module change d'état dans le roadmap, les deux PDF sont à régénérer.
