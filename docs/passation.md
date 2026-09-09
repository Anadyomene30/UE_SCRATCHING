# Reprendre ce projet sur un autre ordinateur

Ce dépôt a été développé dans une session Claude Code qui tournait dans un
conteneur cloud éphémère — c'est pour ça qu'il n'y a aucune trace de Claude Code
sur ta machine locale : le code n'a jamais transité par un disque local, il a été
écrit directement dans le dépôt Git. Tout ce qui compte est déjà sur GitHub ;
cette page explique comment le récupérer et relancer une session Claude Code
dessus.

## 1. Récupérer le code

```sh
git clone https://github.com/Anadyomene30/UE_SCRATCHING.git
cd UE_SCRATCHING
git checkout claude/scratch-video-unreal-0oi7dv
```

C'est la branche de travail — `main` n'est qu'une branche de base quasi vide
créée pour que GitHub ait un point de comparaison pour les pull requests.

Vérifier que ça compile et que les tests passent :

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure    # 290 tests
./build/scratchvj/scratchvj demo              # tourne sans matériel
```

Prérequis : un compilateur C++20 (gcc, clang ou MSVC) et CMake 3.20+.

## 2. Installer Claude Code sur la nouvelle machine

```sh
npm install -g @anthropic-ai/claude-code
```

Puis, dans le dossier du dépôt cloné :

```sh
claude
```

La première fois, Claude Code demande de se connecter (compte Claude.ai ou clé
API Anthropic). Suivre l'invite affichée dans le terminal.

## 3. Ce qui se passe automatiquement au démarrage

Claude Code lit **`CLAUDE.md`** à la racine du dépôt à chaque démarrage de
session dans ce dossier — c'est déjà en place, rien à faire. Ce fichier résume
les conventions du projet et pointe vers `docs/roadmap.md` pour le détail complet
: raisonnement de conception, état d'avancement module par module, ce qui reste à
faire.

Tu n'as donc **pas besoin de réexpliquer le projet** à la nouvelle session : elle
le lit elle-même. Une bonne première instruction est simplement :

> Lis CLAUDE.md et docs/roadmap.md, puis dis-moi où on en est.

## 4. Continuer sur la pull request existante

Le travail est sur la [PR #1](https://github.com/Anadyomene30/UE_SCRATCHING/pull/1).
Pour que la nouvelle session pousse ses commits au même endroit, reste sur la
branche `claude/scratch-video-unreal-0oi7dv` (déjà fait à l'étape 1) — tout push
dessus s'ajoute automatiquement à la PR ouverte.

Si Claude Code a besoin d'agir sur GitHub (lire les commentaires de la PR,
merger, etc.), il utilisera soit `gh` (CLI GitHub, à installer et authentifier
avec `gh auth login` si pas déjà fait), soit demandera l'accès nécessaire.

## 5. Le point qui compte le plus pour la suite

Le prochain vrai jalon (voir `docs/roadmap.md`, jalon 2) a besoin du **matériel
branché** : Phase, Reloop Elite, RP-8000 MK2. C'est probablement la vraie raison
de basculer de machine — si "l'autre ordinateur" est celui qui a le matériel
branché, c'est là qu'il faut être pour :
1. Vérifier si l'Elite émet son état MIDI à la connexion.
2. Vérifier si le second port USB de l'Elite reçoit le timecode en parallèle de
   Serato — la seule inconnue qui pourrait imposer une deuxième machine pour de
   bon.

Ces deux tests sont documentés en détail dans `docs/roadmap.md`, section
« Vérification ».

## 6. Si tu veux que je (Claude) me souvienne de cette conversation

Je ne peux pas transférer l'historique de conversation d'une session à l'autre —
chaque session Claude Code démarre sans mémoire de la précédente. Ce que je *peux*
transmettre, et que j'ai transmis, c'est tout ce qui compte pour reprendre le
travail : le code, les tests, la documentation de conception, et ce fichier. Une
nouvelle session qui lit `CLAUDE.md` et `docs/roadmap.md` a accès à l'essentiel
de ce qu'on a décidé ensemble ici.
