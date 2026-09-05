# ScratchLink — le plugin Unreal

Unreal est un **client** de `scratchvj`, pas son cœur. Ce plugin lit le flux de
contrôle UDP de l'app (port **7331**, format décrit dans
[`docs/protocole.md`](../../docs/protocole.md)) sur un thread dédié, et l'expose
aux Blueprints. Le mix vidéo, lui, arrive par **Spout** — via un plugin Spout
tiers (Off World Live ou équivalent), pas par celui-ci.

Le plugin ne lie **aucun code de scratchvj**. Cette frontière est juridique
autant qu'architecturale : l'app sera GPL-3 par le décodeur de xwax, le plugin
reste indépendant par construction.

## Installation

Copier `unreal/ScratchLink/` dans `Plugins/` du projet Unreal, ou empaqueter :

```
RunUAT.bat BuildPlugin -Plugin=...\ScratchLink.uplugin -Package=<dossier> -TargetPlatforms=Win64
```

Vérifié compilé contre **UE 5.7** (Win64).

## Ce qu'il expose

**`UScratchLinkSubsystem`** (GameInstance) — l'appel qui compte est
`SampleState(DelaySeconds)` : le flux arrive vers 375 Hz, Unreal rend à 60–120
fps, donc chaque frame échantillonne le passé proche (20–50 ms) avec une
interpolation d'Hermite sur les positions — la vitesse est sur le fil, autant
s'en servir. `GetControl("ch1.filter")` lit un contrôle par son id de schéma et
répond **faux** pour un potard jamais touché : inventer 0.0 écraserait ce que le
contrôle pilote.

**`UScratchDilationComponent`** — la version pauvre de la machine à remonter le
temps, à câbler en premier exprès : |vitesse plateau| → dilation temporelle
globale, clampée. Poser sur un acteur, scratcher, le monde suit la main. Un
décrochage timecode (`bHolding`) **gèle** le monde — jamais de téléportation.

La vraie `ScratchTimeMachineComponent` (ring buffer 120 Hz, mode SCRUB avec
réinjection des vitesses dérivées) attend une scène pour être réglée — voir le
roadmap.
