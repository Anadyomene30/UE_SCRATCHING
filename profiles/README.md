# Profils de contrôleurs

Un profil décrit un contrôleur **en données** : ses contrôles (id, nature,
mode des encodeurs), comment l'interface le dessine (groupes, accent, grille),
et — quand un protocole publié le permet — les adresses MIDI de départ.

La Reloop Elite et la RP-8000 MK2 sont compilées dans `core/profiles_builtin.cpp`
(sans adresses : les leurs s'apprennent au `--midi-learn`, elles ne sont pas
publiées). Tout autre contrôleur est un fichier ici, chargé au lancement, et
choisi dans `settings.json` :

```json
"midi": {
  "devices": [
    {"profile": "reloop_elite", "port": "ELITE"},
    {"profile": "rp8000", "port": "RP8000", "ordinal": 0, "deck": "a"},
    {"profile": "rp8000", "port": "RP8000", "ordinal": 1, "deck": "b"},
    {"profile": "apc40_mk2", "port": "APC40"}
  ]
}
```

`ordinal` départage deux ports qui contiennent le même fragment (les deux
platines s'énumèrent « RP8000mk2 » et « 2 - RP8000mk2 ») ; laquelle est à
gauche est un fait de la table, donc un réglage.

## `verified`

`false` veut dire : **écrit depuis un document, jamais balayé sur le matériel**.
L'interface l'affiche tel quel. `apc40_mk2.json` vient du « APC40 Mk2
Communications Protocol » d'Akai, `push2.json` du « Push 2 MIDI and Display
Interface » d'Ableton. Pour passer un profil à `true` :

```sh
./build-ui/scratchvj/ui/Release/midi_probe.exe "APC40" 60    # puis balayer tout
```

et comparer chaque ligne aux `bindings` du fichier ; corriger ce qui diffère,
puis mettre `"verified": true`. Un profil faux ressemble à une table cassée,
d'où la règle : rien n'est déclaré vérifié sans mesure.

## Écrire un profil

```sh
./build/scratchvj/scratchvj profile export reloop_elite > profiles/mon_profil.json
```

écrit le profil intégré comme gabarit. Les ids sont hiérarchiques
(`ch1.eq.hi`, `pad.apc40.r1.c1`) ; un `xfader` sur un autre contrôleur est
**le** crossfader. Les encodeurs sans fin déclarent leur convention
(`relative64` : 64 au repos ; `signed7` : complément à deux sur 7 bits). Un
contrôle `optional` est un contrôle dont on ignore s'il émet du MIDI :
l'apprentissage le passe sans laisser la session inachevée.

Chaque fichier de ce dossier est chargé et validé par la suite de tests.
