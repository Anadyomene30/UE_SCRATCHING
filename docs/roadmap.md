# Feuille de route

Ce document existait jusqu'ici uniquement comme plan de session Claude Code, donc
nulle part sur GitHub. Il est déplacé ici pour que tout ce qui a été décidé et
tout ce qui reste à faire survive à la session qui l'a produit.

## Où on en est, module par module

| Jalon du plan initial | État | Modules |
|---|---|---|
| 1. Voir la table | **Fait sur le vrai rig** : trois ports ouverts à la fois (Elite + deux RP-8000, `ui/midi_rig`), chaque adresse MIDI porte son appareil, les encodeurs sans fin sont lus en relatif, la surface est **dessinée depuis un profil de données** (`core/profile`, Elite complète : 2 voies, 2 unités FX, boucles, 16 pads + modes, browse, sorties, face avant en `optional`), APC40 mk2 et Push 2 livrés en JSON non vérifié, courbes et reverse du crossfader **et** des faders de voie pilotables (`MixSettings`), destinations de mapping en registre (`core/destinations`) | `core/surface`, `core/learn`, `core/profile`, `core/destinations`, `core/mapping`, `core/protocol`, `ui/midi_rig` |
| 2. Suivre le timecode | **Fait sur le vrai matériel** : le Phase émet une porteuse nue (direction + vitesse, pas de position), lue par `core/quadrature` depuis la MOTU en WASAPI partagé, verrou et suivi de la main vérifiés dans la fenêtre (`--live`). Le décodeur xwax est vendu et testé (`dvs/`) pour un vrai disque de contrôle. Le **scope de calibration** (`core/scope`) mesure la figure de Lissajous et la sépare en centre, balance et erreur de phase, dessinée dans le tiroir de diagnostic | `core/timecode`, `core/quadrature`, `core/scope`, `core/anchor`, `core/gestures`, `ui/audio_in`, `dvs/` |
| 3. Voir la vidéo | Fait de bout en bout : `scratchvj analyze` décode via l'exécutable ffmpeg, compresse en BC1 (`core/bc1`, testé) — ou en **BC3** quand la source a un canal alpha (`core/bc3`, testé, la moitié couleur est le bloc BC1 lui-même) — et écrit le `.svcache` ; images fixes et séquences numérotées passent par la même passe. L'interface **importe** (glisser-déposer, dialogues SDL3, dossiers surveillés de `settings.json`) et **analyse en arrière-plan** (`app/analysis_queue`, un worker, rapports à la frontière de frame) | `core/videocache`, `core/framewindow`, `core/bc1`, `core/bc3`, `app/analyze`, `app/analysis_queue`, `app/library_scan`, `config/library_io` |
| 4. Le Mac tourne | CI verte sur macOS depuis le premier commit ; portage audio/GPU réel non fait | `.github/workflows/ci.yml` |
| 5. Mixer | Courbes, blend modes, détection de transform faits ; le program est composité sur le GPU (`fs_program.sc`), tenu conforme à sa référence `core/compose` par l'outil `gpu_check` (écart max 1/255) | `core/mixer`, `core/compose` |
| 6. Transport | Fait en entier : boucles, hot cues, beat jump, slip, ABS/REL/INT, plus la source de position et les modes de lecture par deck | `core/transport`, `core/playback` |
| 7. 360 | Fait à l'image : la passe GPU (`fs_view360.sc`) reprojette l'équirect en perspective / little planet / fisheye, tenue conforme à `core/sphere` par `sphere_check` (écart max 1/255) ; le regard suit le mapping (potards EQ), le program composite la vue projetée | `core/sphere`, `ui/gpu_view360` |
| 8. Mode autonome | Non commencé (a besoin d'un vrai backend audio) | — |
| 9. Effets et modulateurs | Rack, catalogue, LFO/enveloppes faits ; les effets **une-frame** sont dessinés (`fs_effects.sc` / `fx_check`) et les **multi-taps** aussi — traînées et slit scan lisent le clip à huit moments par tableau de textures (`core/videotaps`, `fs_taps.sc`, `taps_check`), calculés depuis position et vitesse donc scratchables et réversibles ; la **FFT audio-réactive** est faite (`core/spectrum`, bandes log affichées dans l'onglet EFFETS) et attend une vraie entrée audio | `core/effect`, `core/videofx`, `core/videotaps`, `core/modulator`, `core/spectrum`, `ui/gpu_effects`, `ui/gpu_taps` |
| 10. Entrées live | Non commencé | — |
| 11. Sorties | Corner pin, **warp maillé bézier** avec ajout/retrait de lignes et **presets de mapping** sauvegardables (`core/mesh`, `config/warp_io`, testés), masque, compositeur GPU, et **sortie Spout vérifiée** par un récepteur indépendant (`spout_check`) ; Syphon/NDI restants | `core/warp`, `core/mesh`, `config/warp_io`, `ui/share` |
| 12. Unreal | L'app émet le flux UDP (vérifié par `net_check`) et le plugin `ScratchLink` existe — subsystem + échantillonnage Hermite + dilation temporelle — compilé contre UE 5.7 ; le test en scène reste à faire | `core/protocol`, `ui/netout`, `unreal/ScratchLink` |

Plus, hors plan initial : `core/playback` (source de transport et modes de
lecture par deck), `core/library` (bibliothèque et queue), `core/take`
(enregistrement/relecture d'une prise), `app/` (démo et tableau de bord qui font
tourner tout ça sans matériel), et `ui/` — l'interface ImGui réelle : six
écrans sur une seule rangée d'onglets (JOUER, BIBLIOTHÈQUE, EFFETS, TABLE,
SORTIE, RÉGLAGES), le deck comme lecteur, scrub à la souris sur la barre de
position, mixer entre les decks, pads à l'écran, éditeur de warp et de masque,
et **chargement d'un clip de la bibliothèque sur un deck ou sur l'incrustation**
en un clic.

> **Le plateau live n'ouvrait rien, et ne le disait à personne** (2026-09-09).
> Trouvé en lançant `--live` pour regarder le scope ci-dessous : aucun
> `platter.log`, aucun message, le deck A repassé sur son horloge sans un mot.
> La cause est dans `AudioInput::open` : le point d'entrée était choisi sur le
> **nom seul** — le premier dont le nom contient le fragment de `settings.json`
> — et le nombre de canaux n'était vérifié qu'ensuite, pour refuser l'ouverture.
> Or « MOTU » désigne ici **deux** entrées de capture, `In 1-2` (2 canaux) et
> `In 1-24` (24 canaux), et la première est celle que Windows énumère d'abord.
> La porteuse est sur les voies 5/6, qui n'existent que sur la seconde. Le
> nombre de canaux n'est pas une validation à faire après coup : **il fait
> partie de la question posée**.
>
> La règle est maintenant dans `core/endpoint`, pure et testée sur les trois
> OS sans carte son — sept tests, dont celui qui énonce le bug (« un fragment
> qui désigne deux entrées prend celle qui porte la paire ») et celui qui
> empêche la sur-correction (« la petite entrée est la bonne quand la paire
> demandée y tient »). Un huitième est né d'une erreur du fichier de test
> lui-même : un micro mono correspond au nom et ne peut pas porter deux
> jambes ; « correspond au nom » n'est pas « peut porter le signal », ce qui
> est tout le propos du module.
>
> Le deuxième défaut est plus grave que le premier : l'échec partait en
> `fprintf(stderr)`, et `scratchvj_ui` est une application WIN32 **sans
> console** — CLAUDE.md le documentait déjà comme piège, et le code le
> commettait quand même. La raison remonte maintenant jusqu'au tiroir de
> diagnostic, en ambre, avec ses trois cas distincts : entrée absente, entrée
> trouvée mais trop étroite (avec le nombre de canaux qu'elle a vraiment), et
> ouverture refusée. Et `input_check` pose la même question depuis une console,
> avec le même `settings.json` et la même classe — une réponse là-bas *est* la
> réponse dans l'application ; réimplémenter la recherche n'aurait testé
> qu'elle-même.

> **Le panneau de la RP-8000 n'aura pas lieu, et c'est une décision**
> (2026-09-09). Il figurait depuis des semaines comme « reste à faire », bloqué
> sur l'absence d'un rendu officiel comparable au `238128_Reloop_TP.jpg` de
> l'Elite. En cherchant à le débloquer, deux choses sont apparues, et aucune
> n'est l'image manquante.
>
> **Un.** Le modèle de panneau (`ProfileGroup`, un rectangle par section, les
> contrôles s'écoulant dedans) suppose **un identifiant = une place physique**.
> Le profil de la RP-8000 déclare 24 identifiants —
> `pad.rp8000.<deck>.l<1..3>.<1..8>` — pour **huit** pads réels : les trois
> couches sont un état de la platine, pas trois rangées de boutons. Les placer
> demanderait soit trois groupes superposés, que le test « aucune section n'en
> recouvre une autre » refuse à juste titre, soit une notion de couche active
> que rien ici ne mesure — le profil ne déclare aucun contrôle pour le bouton
> de couche, et personne ne sait s'il émet du MIDI.
>
> **Deux, et c'est celle qui tranche.** Même le problème résolu, le gain serait
> nul. Le panneau de l'Elite gagne sa place parce qu'on y **cherche un potard
> parmi quarante**, répartis en sections qu'une main connaît par leur position.
> La surface MIDI de la RP-8000 est huit pads dans une seule bande, déjà
> dessinés quatre par rangée. Un panneau de platine serait aux neuf dixièmes un
> dessin de plateau. Ce n'est donc pas reporté : c'est **écarté**, et la ligne
> est passée dans « À ne pas faire ».
>
> Ce qui restait à réparer était ailleurs, et l'erreur le prouve : trois
> groupes de huit pads dessinés côte à côte **sans rien qui dise que ce sont
> les mêmes huit**. La liste est complète et se lit quand même de travers —
> assez pour qu'on planifie un panneau pour 24 boutons qui n'existent pas.
> D'où `DeviceProfile::note` : une ligne que la liste de contrôles ne peut pas
> porter, affichée sous l'appareil dans TABLE, écrite dans le JSON seulement
> quand il y en a une (sinon un profil écrit à la main serait réécrit à chaque
> sauvegarde). Celle de la RP-8000 dit que les trois couches sont les mêmes
> huit pads, commutés sur la platine.

> **La porteuse a une figure, et la figure a trois défauts** (2026-09-09).
> Le dernier manque de la colonne « Serato » : de quoi régler une cellule à
> l'œil. `core/quadrature` savait déjà que la paire n'est jamais un cercle
> parfait — il en fait la correction de Heydemann — mais ne le montrait à
> personne, et niveau, verrou et vitesse peuvent tous les trois être au vert
> pendant que la chaîne est à 3 dB près ou penchée par la diaphonie. Ce qui
> est faux dans ces cas-là est la **forme**, donc c'est la forme qu'on dessine.
>
> `core/scope` mesure une fenêtre de 100 ms et en sort les **trois** manières
> qu'une paire en quadrature a de cesser d'être un cercle centré, parce que
> chacune est une pièce de matériel différente : un **centre** hors de zéro
> est un offset continu, une **balance** hors de 0 dB est une voie plus forte
> que l'autre, une **erreur de phase** hors de l'angle droit est de la
> diaphonie. Les deux dernières font toutes les deux une ellipse — les
> rapporter ensemble enverrait tourner le mauvais bouton, et c'est
> `phase_error_deg` qui les distingue. Le calcul est exact plutôt qu'ajusté :
> pour `x = A·cos(t)` et `y = B·sin(t + φ)`, la covariance des deux voies
> normalisées vaut `sin(φ)/2`, donc la phase tombe des sommes déjà tenues.
>
> Trois décisions qui font la différence entre un affichage et une mesure :
>
> 1. **Le scope mesure avant correction, pas après.** Réutiliser le centre et
>    le gain que le `QuadratureTracker` tient déjà (en min/max glissant)
>    montrerait un cercle quelle que soit l'entrée — puisqu'il corrige. Et
>    min/max, c'est deux échantillons, qu'un seul clic ruine ; une RMS sur une
>    fenêtre, non.
> 2. **La figure est tracée brute, jamais recentrée sur son propre centre.**
>    La recentrer dessinerait un beau cercle pour un offset, c'est-à-dire
>    cacherait un des trois défauts qu'elle existe pour montrer.
> 3. **Le tracé n'est pas décimé.** Prendre un échantillon sur N d'un signal
>    périodique est un stroboscope : un pas qui divise la période de la
>    porteuse dessine un point — ou un triangle, qui ressemble à une panne
>    grave — à partir d'un cercle parfait. Garder les 512 derniers
>    échantillons tels quels coûte quelques kilo-octets et ne peut pas aliaser.
>
> Et une propriété propre à ce matériel : sur cette chaîne **la fréquence de
> la porteuse est la vitesse du plateau**. À l'arrêt la figure dégénère en un
> point, et sous quatre tours par fenêtre c'est un arc, pas une boucle — une
> ellipse ajustée sur un arc est une devinette, et une devinette imprimée en
> degrés à côté d'une vraie mesure ne s'en distingue pas. D'où le troisième
> verdict, `TooSlow`, qui dit « laisser tourner à vitesse nominale » plutôt
> que d'inventer trois chiffres. Neuf tests, tous contre
> `generate_quadrature`, auquel un défaut de phase a été ajouté pour que la
> mesure de phase ait quelque chose à vérifier.
>
> Ce qui n'est pas vérifié : le **dessin**. Le calcul est tenu par les tests,
> mais la figure elle-même ne s'affiche qu'avec une entrée audio ouverte et un
> plateau qui tourne — personne ne l'a encore regardée.

> **Le programme a un écran à lui** (2026-09-08). Jusque-là la seule sortie
> était Spout, ce qui est juste pour nourrir Resolume ou Unreal et inutile
> pour le cas ordinaire : un projecteur sur la seconde sortie du bureau.
> `ui/output_window` ouvre une fenêtre sans bordure en plein écran sur
> l'écran choisi et y dessine la texture du rack d'effets par une **seconde
> chaîne d'échange bgfx sur le même device** — pas de relecture, pas de copie
> par la mémoire centrale, donc rien de plus que la vsync de ce moniteur. Les
> trois sorties (aperçu, Spout, écran) partagent la même texture et ne peuvent
> pas diverger. Un écran d'une autre forme reçoit des bandes noires : un
> projecteur est un instrument de mesure pour un VJ, et une image étirée en
> silence rendrait faux tout masque calé dessus. L'écran est retenu **par son
> nom**, jamais par son index — ceux-ci sont renumérotés dès qu'on branche
> quoi que ce soit, et un set qui s'ouvre sur le mauvais écran est pire qu'un
> set qui ne s'ouvre sur aucun. Échap ferme la sortie avant de quitter.
> La géométrie (corner pin, grille, masque) est **appliquée sur cet écran**
> depuis la refonte de l'interface (voir plus bas) ; restent Syphon/NDI.

> **La table est dessinée où elle est, pas en liste** (2026-09-08). Un profil
> peut porter la géométrie de son panneau en millimètres réels ; celle de
> l'Elite est **mesurée** sur le rendu officiel de Reloop, lue au pixel et
> recoupée avec le diagramme de callouts du manuel (290 × 400 mm, portrait).
> Une main trouve le potard de filtre par sa *place* ; une interface qui montre
> les mêmes contrôles autrement est un second panneau à apprendre plutôt qu'un
> miroir du premier. Un profil sans géométrie mesurée reste dessiné en rangée —
> la RP-8000, l'APC40 et le Push en sont là, et inventer leurs coordonnées
> serait pire que la liste. Deux tests gardent le panneau : aucune section n'en
> recouvre une autre, et aucun contrôle déclaré n'est laissé hors du dessin.
>
> La mesure a corrigé quatre suppositions : l'Elite n'a **aucun** bouton cue
> par voie (le monitoring est un slider de sélection au centre), le sélecteur
> d'entrée est sur le dessus et non à l'arrière, il y a **un** SHIFT et non un
> par côté, et la face avant porte **trois** paires courbe/reverse — voie 1,
> crossfader, voie 2. Le mixeur n'applique pour l'instant qu'**une** courbe de
> voie aux deux canaux là où le matériel en a deux : simplification connue,
> à lever quand `MixSettings` séparera les deux voies.
>
> Un panneau portrait ne tient pas dans une bande en bas de l'écran de jeu :
> il a son onglet, **TABLE**, et JOUER garde le mixer entre les deux decks —
> les deux faders, le crossfader et les courbes, ce pour quoi l'œil quitte
> l'image pendant un set.

> **L'interface a été refaite** (2026-09-08). Trois lots avaient corrigé une
> pièce chacun et l'ensemble restait incompréhensible : pas de bouton lecture,
> deux axes de navigation (six onglets *et* cinq dispositions), des boutons qui
> ressemblaient à des libellés, le jargon du DVS sous chaque deck, les pads
> absents de l'écran alors qu'ils sont l'interface principale du matériel. La
> maquette Claude Design (`design/`) a été refaite d'abord, puis le code en six
> lots :
>
> 1. **Le deck est un lecteur.** `Deck::play/pause/stop` (façade sur
>    `DeckClock`), un clip chargé sans platine *joue*, la barre de position se
>    glisse et rend le deck à la source d'où la main l'a pris (`Deck::grab/
>    scrub/release` — le clock seul l'oubliait et rendait tout à la platine,
>    ce qui gelait un deck qui jouait : c'était le « je ne peux pas scrubber »).
>    `Deck::load` prend le temps mur, sinon une horloge libre reconstruite à
>    t = 0 ouvrait un clip à *temps-mur modulo durée*. 2D | 360 sur l'en-tête
>    du deck, source Platine | Lecture | Tempo en sélecteur segmenté, Platine
>    proposée seulement s'il y a une platine.
> 2. **Un seul axe.** Les cinq dispositions et les touches 1–5 disparaissent ;
>    F = image seule, B = rail replié. JOUER = rail | deck A | mixer | deck B,
>    programme en bas. MAPPING fusionne dans TABLE. La barre d'état passe aux
>    voyants (Phase, Table) et perd « Serato | Autonome » (rien ne l'écrivait).
> 3. **La bibliothèque est un écran** : dossiers et caisses (création,
>    ajout/retrait — `Library::create_crate` n'était appelé par rien),
>    liste avec **vignettes** (BC1 128 px, rangées par la passe d'analyse dans
>    le bloc de métadonnées du cache, `core/cachemeta`, lisible aussi depuis les
>    caches d'avant), inspecteur, glisser-déposer vers les **banques de pads**
>    (`core/matrix`, dans `library.json`).
> 4. **Les pads sont à l'écran**, huit par deck, trois modes : cues (Maj + clic
>    pose, clic droit efface — `set_cue`/`clear_cue` n'étaient appelés que par
>    la démo), clips (la banque), boucles (1/8 à 16 temps). Boucle entrée/
>    sortie/×, saut de temps, quantisé. Un pad à l'écran et un pad de l'Elite
>    passent par les mêmes `DeckCommands`.
> 5. **La géométrie est rendue sur l'écran de sortie** : grille 32 × 32
>    placée par `homography_from` ou `WarpMesh::map`, alpha par
>    `Mask::coverage`, `vs_warp`/`fs_warp` — les mêmes fonctions que l'aperçu,
>    donc le projecteur et l'aperçu ne peuvent pas diverger. Spout reçoit
>    l'image *avant* la géométrie. Le masque a enfin un éditeur, et les presets
>    l'écrivent (ils l'écrivaient vide).
> 6. **Le jargon en tiroir** (« diagnostic platine », ouvert seul quand une
>    platine est en direct), et ce que le moteur savait faire sans qu'aucun
>    bouton n'y mène : fusion de l'incrustation, budget de mémoire vidéo,
>    hôte/port UDP (RÉGLAGES), **Ancrer** (deck A), **REC** d'une prise
>    (`core/take`, `takes/*.svtake`), éditeur de liaisons dans TABLE (ajout,
>    « bouger un contrôle », destination depuis le registre, courbe, zone
>    morte, lissage, suppression — `MappingEngine::remove` est nouveau). Les
>    liaisons OSC par défaut (`/ue/shake`, `/ue/whippan`) sont retirées : aucun
>    transport ne les portait, et une liaison qui ne fait rien apprend à se
>    méfier des autres.
>
> Trois familles de contrôles et pas plus — bouton, sélecteur segmenté,
> basculeur — avec un fond et un bord sur tout ce qui se clique ; l'orange
> plein pour une seule action par panneau ; le rouge réservé à « délié ». Les
> contrôles fantômes restent. `--screen <nom>` ouvre sur un écran donné.
>
> Puis, le même jour : la **relecture** d'une prise (« Relire… » sur la barre
> d'état ; la table et les platines sont rejouées depuis `takes/*.svtake`, le
> schéma de la prise est rapproché de la surface par identifiant), **une
> courbe par voie** (`MixSettings::channel_b`, trois paires dans TABLE comme
> sur la face avant ; les destinations `mix.fader.a/b.*` s'ajoutent, et
> `mix.fader.*` écrit les deux), et **EFFETS en cartes** : sélecteur, lien,
> actif, six potards avec « assigner » (le prochain contrôle touché sur la
> table devient la source, par une liaison ordinaire), sync tempo.
>
> Et les **neuf transitions du crossfader** ont enfin leur code : la référence
> CPU est `compose_decks` (`core/compose`), le shader `fs_program.sc` la
> transcrit, et `gpu_check` tient les neuf à 2/255 près, à mi-course et aux
> deux bouts. Fondu et Additif lisent les *poids* (la courbe) ; Cut, Multiplié,
> Screen, wipe luma, wipe, RVB décalé et Zoom lisent la *position*, pour qu'une
> courbe sharp ne réduise pas un wipe à un cut. Le menu est dans la colonne du
> mixer, `mix.transition` en liaison, `mix.transition` dans `settings.json`.
> Le « zoom blur » est un zoom : le flou serait l'affaire des taps du rack, et
> il est nommé ainsi à l'écran.
>
> Ce qui n'est pas fait : la géométrie de la RP-8000 — et depuis le 2026-09-09
> ce n'est plus une dette mais une décision, voir la note plus haut. Ni gcc ni
> clang n'étant installés sur ce bureau, la vérification multi-compilateurs est
> celle de la CI.

> **La démo a cessé d'être l'état par défaut** (2026-09-08). Elle était
> l'échafaudage qui a permis de construire tout l'instrument avant qu'il y ait
> du matériel ou une passe d'analyse : deux decks aux noms inventés, dont un
> équirectangulaire, et une bibliothèque de fichiers qui n'existent pas. Vue
> par quelqu'un qui ouvre le logiciel, cette scène *est* le logiciel — d'où
> « on est en mode simulation » et « on ne peut faire que de la 360 », deux
> reproches qui décrivaient exactement ce qui était à l'écran. `--demo` la
> rallume. Sans elle les decks sont vides, et un fichier déposé est analysé
> puis **posé sur le premier deck libre** : rendre un `.svcache` sans rien
> montrer n'est pas un import, c'est un devoir à faire.
>
> Corollaire trouvé du même coup : un fader de voie jamais touché comptait
> **zéro**, donc charger un clip sur une installation neuve donnait un
> programme noir. La règle du fantôme porte sur ce qu'on *affiche* ; le mixeur,
> lui, doit calculer, et « fermé » est aussi inventé qu'« ouvert » tout en
> étant le seul des deux qui fait croire que le logiciel est cassé. Un fader
> inconnu compte donc comme **ouvert**, le crossfader comme **au centre**, et
> la vraie valeur gagne dès qu'on y touche.

> **La surface a cessé d'être un dessin de huit potards** (2026-09-08). Le
> profil dit ce qu'un appareil a et comment on le dessine ; le rig ouvre un
> port par appareil et stampe chaque événement de son index, ce qui fait de
> « CC 7 sur l'Elite » et « CC 7 sur la platine » deux adresses. Les encodeurs
> sans fin (browse, beats, loop) sont lus comme des distances (`relative64` ou
> `signed7`), plus comme des positions à 0,49. Les courbes et reverse du
> crossfader et des faders sont des réglages du moteur, des cibles de mapping
> et des puces à l'écran ; leurs boutons en face avant de l'Elite sont
> déclarés `optional` tant que leur MIDI n'est pas mesuré. Le dispatch des
> mappings par comparaison de chaînes (quatre cibles vivantes sur huit) est
> remplacé par le registre `core/destinations` : un pad sur un hot cue saute
> sur le front, une cible inconnue est nommée au `bind()`. **Le hash de schéma
> change avec le roster** — c'est le comportement prévu par le protocole, un
> client Unreal doit s'y attendre. Correction au passage : l'Elite est une
> table **2 voies** ; l'exploration qui l'avait décrite en quatre se trompait.

> **La bibliothèque a cessé d'être un coup d'œil à `clips/`** (2026-09-07). Elle
> se remplit par glisser-déposer, par un dialogue de fichiers ou de dossier, et
> par les dossiers de `settings.json` parcourus récursivement au lancement. Ce
> qui arrive par un geste explicite (drop, dialogue) s'analyse tout de suite ;
> ce qu'un dossier révèle n'est que listé, à analyser à la demande — un dossier
> de 4K analysé sans qu'on l'ait demandé, en plein set, saturerait la machine.
> L'analyse tourne sur **un** thread (`app/analysis_queue`) et ne touche jamais
> la `Library` : elle rapporte, et la boucle principale applique à la frontière
> de frame, comme un chargement de clip. Deux corrections trouvées en chemin :
> `Deck::load` remettait la source, le mode et la politique de prise en main du
> deck à zéro (un logo déposé sur l'incrustation en faisait un deck timecode
> bouclé qui suivait le plateau gauche), et « ce clip est du 360 » était dérivé
> de l'aspect en cinq endroits de l'interface — c'est maintenant une décision
> par clip (`ProjectionOverride` dans `library.json`) que le deck porte dans son
> drapeau, et tout le reste demande au deck.

Et `core/headset` : la géométrie de la sphère vue à travers un casque, avec sa
passe GPU (`ui/gpu_eye`, `fs_view360_eye.sc`) tenue à sa référence par
`eye_check`. Voir [Le casque](#le-casque--scratcher-une-sphère-quon-regarde-de-lintérieur).

**Ce qui reste, dans les grandes lignes** : le MIDI **portable** (RtMidi) — sur
Windows l'entrée est réelle depuis longtemps, `ui/midi_in.cpp` parle winmm et
c'est ce que le rig utilise ; ce qui manque est macOS et Linux, où le fichier
est un stub qui renvoie « aucun port » (vérifié le 2026-09-09 : le `#else` de
`midi_in.cpp`). Ce n'est donc pas « pas de backend MIDI » mais « pas de backend
MIDI hors Windows », et ça ne se vérifie pas sans une machine de chaque.
Restent aussi la sortie audio du mode autonome (miniaudio/ASIO — l'entrée existe),
Syphon/NDI, la **session** OpenXR (la géométrie est faite, la boucle de frame
attend le casque), `DeckSource::Live` pour scratcher une entrée vidéo live, et
le test en scène du plugin Unreal. Le reste du plan initial est fait et vérifié
— voir le tableau ci-dessus. Les réglages propres au bureau (entrée, paire de
canaux, porteuse) sont dans `settings.json` via `config/settings_io`.

> **NDI : bloqué sur une licence, pas sur du code.** La machine a bien le
> *runtime* NDI (v5 et les NDI 6 Tools, avec `NDI_RUNTIME_DIR_V6` posée), donc le
> chargement dynamique de `Processing.NDI.Lib.x64.dll` — la voie que NDI lui-même
> recommande, sans DLL à redistribuer — fonctionnerait. Il manque les **en-têtes
> du SDK**, dont le téléchargement suppose d'accepter l'EULA NDI. C'est à
> l'utilisateur de le faire, pas à l'outil. Le SDK ne doit pas être versionné
> dans ce dépôt (il sera GPL-3 via xwax) : le motif à suivre est celui d'`obs-ndi`
> — une option CMake pointant sur un SDK installé localement, et la sortie NDI
> compilée hors du binaire quand il est absent. Les NDI Tools fournissent au
> passage **Studio Monitor**, donc un récepteur indépendant pour la vérifier,
> exactement comme `spout_check` vérifie Spout.

**Les deux tests qui revenaient à l'utilisateur sont répondus.** Le matériel a
été branché le 2026-09-06 et deux outils existent maintenant pour y répondre —
`midi_probe` et `audio_probe`, tous deux sous `ui/tools`. Ce qui est déjà établi :

| Fait | Établi par |
|---|---|
| Windows voit `ELITE`, `Phase`, et **deux** `RP8000mk2` (chemins USB parents distincts : ce sont les deux platines, pas deux ports d'une) | `midi_probe` sans argument |
| Aucun port MIDI n'est pris par une autre application | `midi_probe all` les ouvre tous |
| L'Elite expose un pilote **ASIO** *et* un point de terminaison **WASAPI** « Entrée ligne » | énumération Windows |
| Cette entrée s'ouvre en **mode partagé**, 48 kHz, stéréo, flottant | `audio_probe "Reloop ELITE"` |
| L'Elite n'expose à WASAPI qu'**une seule paire stéréo** — sa nature 10x10 vit du côté ASIO | énumération des points de terminaison |
| Les 11 entrées de capture s'ouvrent toutes en partagé, aucune n'est prise en exclusif | `audio_probe all` |
| La MOTU expose **24 canaux sur un seul point de terminaison** — d'où le balayage de toutes les paires adjacentes | `audio_probe all` |
| **Serato en marche ne bloque rien** : l'entrée WASAPI de l'Elite s'ouvre toujours (Ploytec multi-client ASIO/WDM), et les 7 ports MIDI restent libres — Serato parle à l'Elite en HID, pas en MIDI | sondes lancées pendant une vraie session Serato, 2026-09-07 |
| En session HID réelle, **aucun timecode analogique n'existe** : la musique est vue (webcam, câble virtuel), le geste des remotes non | `audio_probe all` pendant la session |

**Comment l'Elite achemine réellement le timecode** (documentation constructeur +
inspection du pilote, 2026-09-07). Corrige une erreur de raisonnement : le
sélecteur de voie choisit ce qu'on **écoute**, pas ce que l'ordinateur
**reçoit**.

- **Le sélecteur de voie doit être sur USB A/B en DVS**, pas sur PHONO. Le trajet
  entrée analogique → USB est interne et permanent ; mettre la voie sur PHONO
  reviendrait à écouter le timecode au lieu de la musique que Serato renvoie.
- **Utilities → USB OUT ROUTING** (SHIFT + BACK tenus 3 s) désigne, pour DECK 1
  et DECK 2, si l'envoi USB prend l'entrée **PHONO** ou **CD/LINE**. C'est là que
  se décide ce qui part vers l'ordinateur.
- Le pilote est un **Ploytec** et n'expose que des noms génériques — `ELITE In
  1..10`, `ELITE Out 1..10`. Aucun panneau de contrôle logiciel : tout se règle
  sur le mixeur.
- Côté WASAPI, une **seule paire stéréo** (« Entrée ligne »), très probablement
  `ELITE In 1/2`. Donc le timecode y est lisible **si et seulement si** USB OUT
  ROUTING met le deck concerné sur cette paire.

> **La voie n° 1 du tableau des quatre voies n'a jamais été essayée.** L'Elite a
> **deux ports USB-B** et se présente comme deux interfaces 10×10 indépendantes.
> Windows n'en voit **qu'une seule instance** (`VID_26AD&PID_94F0\201709`) : un
> seul câble est branché. Brancher le second câble donne à cette application sa
> propre interface, portant les mêmes entrées, **sans rien partager avec
> Serato** — ni ASIO, ni WASAPI, ni exclusivité. C'est le plan d'origine, il est
> à un câble de distance, et il rend la question du partage sans objet.

**Ce que dit Phase Manager (V 2.4.6), et qui oriente le décodeur :**

- **Configuration DVS : « Serato DJ (default) ».** Le dock ne synthétise pas un
  signal maison : il **émule le disque de contrôle Serato**. C'est exactement ce
  que le `timecoder.c` de xwax décode avec son profil Serato existant. La note du
  plan sur un « profil `wireless` » reste pertinente pour la *qualité* du signal
  (synthétisé, donc sans bruit de surface ni usure) mais pas pour son *format*.
- **Vitesse des Remotes : 33 RPM.** La vitesse de référence attendue par le
  décodeur.
- **Un mode HID existe** (« Connectez Phase avec Serato DJ Pro en HID »), qui se
  passe entièrement des câbles RCA. À vérifier : si le Phase est en HID, la
  sortie RCA peut être inactive — ce qui expliquerait qu'aucune entrée n'ait
  jamais reçu de signal pendant les mesures. Ce mode est propre à Serato et ne
  nous est pas accessible, donc le DVS reste la voie.

Mesuré avec des mains sur le matériel :

1. ~~**Est-ce que l'Elite émet son état MIDI à la connexion ?**~~ **RÉPONDU le
   2026-09-07 : NON.** `midi_probe all 45` avec balayage complet : 329 messages,
   **41 contrôles distincts**, mais seulement **7 dans la première demi-seconde**
   — et ce sont des CC d'encodeur au repos (ch2/ch3 n25 à 64), pas une
   description de la surface.
   **Donc le mode fantôme de `core/surface` reste nécessaire** : un potard
   absolu n'a pas de valeur connue tant qu'on ne l'a pas bougé, et l'interface a
   raison de le dessiner en pointillé plutôt que de mentir avec un zéro. C'est
   la justification mesurée d'un choix fait à l'aveugle au premier commit.

   Ce que l'Elite envoie, pour mémoire (le nommage reste à `--midi-learn` : rien
   n'est câblé en dur, c'est la règle du projet) — CC sur ch2/ch3 (n22-n26, n28,
   n52), ch7 (n52), ch10/ch11 (n0, n3), ch16 (n8, n10, n123, n127) ; notes sur
   ch6, ch7, ch11, ch14, ch16. Aucun autre port ne parle : le Phase n'est plus
   énuméré (récepteur débranché de l'USB, il est sur son chargeur) et les
   RP-8000 étaient éteintes.
2. **Sur quelle entrée arrive le timecode, et est-elle lisible en partagé ?**
   C'est la reformulation concrète de la question du second port : si le signal
   arrive sur un point de terminaison WASAPI ouvrable en partagé, la voie
   « WASAPI partagé » est ouverte et la deuxième machine tombe.
   `audio_probe all 3` **en faisant tourner un plateau pendant tout le
   balayage** — il n'y a pas de disque de contrôle avec un Phase, le dock
   synthétise le signal à partir du mouvement de la remote, donc à l'arrêt il
   n'y a rien à mesurer. Mesuré à vide : toutes les entrées silencieuses.

> **Le détecteur ne se contente pas d'un niveau, et il a fallu deux essais.**
> « L'entrée n'est pas silencieuse » est vrai d'un micro dans une pièce. La
> première version cherchait la **quadrature** entre les deux voies — et a
> déclaré « TIMECODE » sur le micro d'une webcam captant une pièce calme : sur du
> bruit, la fréquence et la phase estimées sont aléatoires, donc environ une
> paire sur quatre tombe près de ±90°. Le niveau ne sauve pas non plus (la pièce
> était à 0,012, un niveau plausible pour une cellule faible).
>
> Ce qui discrimine est la **tonalité** : le rapport entre la magnitude à la
> porteuse et l'énergie totale du signal, qui vaut ~0,71 pour une sinusoïde pure
> et ~1/√N pour du bruit. `audio_probe selftest` construit les six cas (timecode
> fort, faible, dans les deux sens, bruit, musique, silence) et les juge sans
> matériel — c'est le test qui aurait attrapé le faux positif du premier coup.
>
> **Le signe du déphasage n'est pas interprété.** Les deux sens de rotation
> donnent des signes opposés, et le selftest l'exige ; mais lequel veut dire
> « avant » dépend de la convention du dock et du câblage L/R. Le nommer sans
> l'avoir mesuré serait refaire le défaut du lacet 360 inversé. C'est un tour de
> plateau dans un sens connu qui le tranchera.

---

## Contexte

Le projet a changé de nature pendant le cadrage. Point de départ : « faire remonter
le mouvement de mes platines dans Unreal ». Objectif réel : **un logiciel DJ vidéo
autonome, multiplateforme** — charger n'importe quelle vidéo sur des decks, la
scratcher aux platines, mixer au crossfader, gérer une file de clips, piloter tous
les paramètres depuis les potards et boutons de l'Elite, lire et scratcher des
vidéos **360** en dirigeant le regard au potard, et appliquer un **rack d'effets où
chaque effet audio a son pendant visuel piloté par le même bouton**. Unreal devient
un **client** de cette app, pas son cœur.

Décisions actées :

| Sujet | Choix |
|---|---|
| Audio | **Deux modes commutables** : *suiveur* (Serato joue le son) ou *autonome* (l'app joue et scratche le son elle-même). |
| Effets | **Paires audio/vidéo liées par défaut**, déliables unité par unité. |
| Plateformes | **Windows d'abord, architecture portable dès la première ligne** (macOS visé). |
| Rendu GPU | **bgfx** (voir justification plus bas). |
| UI | **Dear ImGui** natif. |
| Unreal | Client : reçoit l'état de la surface (UDP/OSC) et le mix vidéo (Spout). Windows. |

### Faits vérifiés qui déterminent l'architecture

1. **Le Phase sort du timecode DVS standard** en RCA — le dock génère le signal de
   contrôle attendu par n'importe quel DVS. Aucun SDK MWM nécessaire : un décodeur
   donne position absolue + vitesse signée.
2. **La Reloop Elite** a une interface audio **double 10x10 USB** et **tous ses
   contrôles sont MIDI**. Routing réglable dans les Utilities, entrées basculables
   LINE/PHONO par canal.
3. **La RP-8000 MK2** a un port USB type B et 8 pads LED MIDI sur 3 couches —
   surface de contrôle directement exploitable.
4. **Aucun projet open-source** ne fait DVS → vidéo scratchable → Unreal.
5. Côté Unreal, **Spout** est disponible via plusieurs plugins UE5 (Off World
   Live, et des implémentations open-source DX11-on-DX12) : partage de texture
   GPU sans copie ni encodage.

### Les deux modes audio

Le mode se choisit dans la barre de statut. **Le décodeur de timecode et tout le
moteur vidéo sont identiques dans les deux cas** — seule change la question de qui
possède le son.

**Mode suiveur (Serato).** Serato joue et scratche le son ; l'app ne fait que la
vidéo. Comme elle lit le **même** timecode, la position vidéo suit la position
audio. Il faut donc :
- un **ancrage** par deck (`offset = position_timecode − position_vidéo`), posé
  d'un coup de pad au moment voulu, plus un **indicateur de fraîcheur** affiché en
  permanence ;
- un **re-ancrage** après tout ce qui casse la correspondance : needle drop,
  boucle, censor, ou passage en mode relatif dans Serato.

> **Serato n'a pas d'API publique** : impossible de lire sa vraie tête de lecture.
> L'ancrage n'est pas un raccourci, c'est le seul mécanisme possible, et il faut
> donc que le re-ancrage soit à un pad de distance plutôt que caché dans un menu.
> (Écarté : aligner par empreinte audio du master — coûteux et fragile pour un
> gain marginal.)
>
> **La dérive n'est pas mesurable**, et c'est implémenté ainsi (`core/anchor`) : si
> le DJ boucle, censure ou pose l'aiguille dans Serato, l'audio bouge et le
> timecode ne bouge pas — la correspondance casse sans que rien ne soit observable
> de notre côté. Un nombre de secondes affiché serait donc inventé. Ce qui *est*
> observable, c'est ce qui s'est passé depuis la pose : le temps écoulé et le
> nombre de discontinuités de timecode vues. On affiche donc une **fraîcheur** —
> une mesure de risque, pas d'erreur.

**Mode autonome.** L'app possède le son : elle décode le timecode, lit et scratche
l'audio, et le renvoie sur les canaux USB de l'Elite. Boucle DVS complète, sync
parfaite par construction, et possibilité de scratcher un clip sans équivalent
dans Serato.

**Le vrai problème est de partager l'entrée audio en mode suiveur sur Windows**, où
les pilotes ASIO DJ sont généralement mono-client : si Serato tient le device,
l'app ne peut pas l'ouvrir. Quatre issues, par ordre de préférence :

| Voie | Détail | Verdict |
|---|---|---|
| **2ᵉ port USB de l'Elite** | Interface double 10x10 : Serato sur USB-B1, l'app sur USB-B2, les deux reçoivent les mêmes entrées | Matériel déjà possédé — **à tester en premier**, y compris sur une seule machine |
| **Split RCA passif** | Sortie du Phase dupliquée vers une petite interface d'entrée dédiée | Toujours fonctionnel, ~50 € |
| **WASAPI partagé** | Si le pilote Reloop expose WASAPI à côté d'ASIO | Dépend du pilote, à tester mais pas à présumer |
| **Deux machines** | Laptop Serato + PC vidéo | Le repli qui marche toujours |

**Sur macOS le problème n'existe pas** : CoreAudio est multi-client, l'app ouvre la
même entrée que Serato sans rien partager de spécial.

---

## Deux principes de conception

> **1. Tout ce qui doit être scratchable est une *fonction* de la position, jamais
> un intégrateur.**

Une simulation avance et ne sait pas reculer ; un flux paramétré par un `t` se
scratche parfaitement. Ce principe décide de tout : le moteur vidéo indexe des
frames au lieu de lire un flux, le moteur audio se pilote en position et non en
vitesse, et côté Unreal les effets sont soit paramétriques, soit adossés à un
historique enregistré.

> **2. L'audio agit sur les fréquences temporelles d'un signal 1D, la vidéo sur les
> fréquences spatiales d'un signal 2D.** Quand la correspondance est exacte, on
> l'implémente telle quelle ; quand elle ne l'est pas, on choisit l'analogue
> perceptif le plus proche et on le documente.

C'est ce qui transforme le rack d'effets en système cohérent plutôt qu'en tas
d'effets arbitraires — voir [`fx-correspondances.md`](fx-correspondances.md) pour
la table complète, tenue en code dans `core/effect.h`.

---

## Architecture cible

```
Phase RX ──RCA timecode (niveau ligne)──> Elite (entrées LINE) ──USB──┐
Elite : faders, EQ, filtres, FX, 16 pads ──USB MIDI────┤
RP-8000 MK2 : 8 pads x 3 couches ─────────USB MIDI─────┤
                                                       v
                                    ┌──────────────────────────────────┐
                                    │       scratchvj (C++20)          │
                                    │  surface + mapping engine        │
                                    │  timecode  ->  position          │
                                    │  moteur audio DVS                │
                                    │  moteur vidéo (frames GPU)       │
                                    │  RACK D'EFFETS APPAIRÉS          │
                                    │  reprojection 360                │
                                    │  biblio / queue / UI ImGui       │
                                    └──┬─────────┬──────────┬──────────┘
                audio ──USB/ASIO/CoreAudio┘      │          │
                vers les canaux Elite            │          │
                                     écran/projecteur   Spout|Syphon / NDI / fichier
                                                            │
                                                            v
                                         ┌───────────────────────────────┐
                                         │ Unreal + plugin ScratchLink   │
                                         │ état surface via UDP/OSC      │
                                         │ mix vidéo via texture Spout   │
                                         │ rewind 3D, caméra, monde 3D   │
                                         └───────────────────────────────┘
```

**Le flux de contrôle UDP/OSC (`core/protocol`) existe déjà.** Même si le plugin
Unreal est prévu en dernier, on peut brancher Unreal, TouchDesigner ou Resolume
dès que la surface est lue.

### Choix des briques, toutes portables Windows/macOS

| Rôle | Brique | Pourquoi |
|---|---|---|
| Rendu GPU | **bgfx** | Mûr, permissif, backends **Metal** et D3D/Vulkan natifs. Gère depuis longtemps les **tableaux de textures** et les **formats BCn**, dont dépend tout le moteur vidéo. Intégration Dear ImGui établie. |
| Fenêtre / entrées | **SDL3** | Standard, portable, sans surprise. |
| UI | **Dear ImGui** | Dense, immédiat, parfait pour potards et tableaux. |
| Audio | **miniaudio** (WASAPI + CoreAudio), **ASIO** en backend optionnel Windows | Header unique, domaine public, aucun SDK à télécharger pour démarrer. |
| MIDI | **RtMidi** | Windows MM + CoreMIDI. |
| Décodage vidéo | **FFmpeg** | Le seul moyen d'ouvrir « n'importe quelle vidéo ». |
| Timecode | **`timecoder.c` de xwax** | Décodeur DVS éprouvé (GPL-3, voir Licence). |
| Partage texture | **Spout** (Win) / **Syphon** (Mac) / **NDI** (les deux) | Derrière une interface commune. |

**OpenGL a été écarté** : déprécié par Apple et plafonné à 4.1. **SDL3 GPU** est
plus propre sur le papier mais trop jeune, et son écosystème est mince
précisément sur les tableaux de textures et BCn. **Prix à payer de bgfx, annoncé
d'avance** : ses shaders s'écrivent dans un dialecte propre compilé par
`shaderc`, moins agréable que du GLSL brut — le coût de la portabilité, payé une
fois.

> **Une architecture portable qui n'est jamais compilée sur l'autre OS est une
> fiction.** D'où la CI macOS dès le premier commit — déjà en place et vérifiée à
> chaque push, avant même que l'app y fasse quoi que ce soit d'utile.

---

## Les briques restant à écrire

### Timecode et moteur audio (le DVS)

**Décodage** via `timecoder.c` de xwax (`serato_2a/2b`, `traktor_a`, `mixvibes`). À
vérifier en lisant `player.c` : `timecoder_get_position()` renvoie **-1 hors
verrouillage**, et son paramètre `when` donne le nombre d'échantillons écoulés
depuis la mesure — il faut extrapoler `pos += when * pitch / rate`.
`timecoder_get_pitch()` est un ratio sans dimension (1.0 nominal, négatif en
arrière).

**Lecture pilotée en position, jamais en vitesse.** À chaque bloc audio on connaît
la position cible en début et en fin de bloc ; on interpole le pointeur de lecture
sur le bloc et on rééchantillonne (Hermite cubique, sinc fenêtré si le repliement
s'entend sur les scratchs rapides). Piloter en vitesse dériverait ; piloter en
position ne dérive jamais.

**Backend derrière une interface** : **miniaudio** (WASAPI exclusif sur Windows,
CoreAudio sur Mac) pour démarrer sans SDK, ~10 ms aller-retour sur bon matériel.
**ASIO ajouté ensuite** si la latence gêne (128 échantillons @48 kHz ≈ 8-10 ms).

**Le mode décide de ce que fait cette couche**, et l'interface
`DeckAudioSource { Interne | Externe }` doit exister dès le premier jour :
- **Autonome** : boucle DVS complète, device ouvert en **duplex, exclusif**.
- **Suiveur** : **entrée seule, jamais en exclusif**, aucun moteur de lecture
  audio, aucune sortie. Ouvrir le device en exclusif dans ce mode empêcherait
  Serato de fonctionner — c'est l'erreur à ne pas commettre.

### Moteur vidéo — scratcher n'importe quel fichier

L'erreur classique est de faire `Seek()` sur un lecteur vidéo : le décodage
compressé ne suivra jamais un scratch, et même l'`ImgMedia` d'Unreal, pourtant
conçu pour ça et doté d'un cache, rame au scrubbing rapide d'après les retours
communautaires. **Il faut sortir du décodage temps réel.**

**Passe d'analyse hors ligne** (comme l'analyse de morceau de Serato) : FFmpeg
décode le clip une fois, redimensionne, compresse chaque frame en **BCn** (déjà
formalisé dans `core/videocache`), et écrit un `.svcache`. Ensuite, plus jamais de
décodage. File de jobs en arrière-plan avec progression dans l'UI. La même passe
extrait vignette, métadonnées et **beatgrid**.

### Le casque — scratcher une sphère qu'on regarde de l'intérieur

L'idée : lire un équirect en live, le scratcher aux platines, et le voir dans le
Quest branché en Link. Le moteur vidéo n'y change rien — un clip 360 est déjà un
clip comme un autre, indexé par position. Ce qui change est **qui regarde**.

**La décision structurante : le casque est la tête, le performeur est le monde.**
En VR la tête *est* le regard, donc le potard de lacet ne peut pas être le regard
aussi sans que les deux se disputent le même degré de liberté. Le potard tourne
donc la **sphère** : le spectateur regarde librement autour de lui pendant que le
DJ fait tourner le monde. Un rayon se compose `monde = R_regard(R_œil(rayon))`,
et `R_regard` est exactement la rotation que `core/sphere` applique déjà — la vue
plate et la vue casque restent un seul mécanisme, pas deux qui peuvent diverger.

Deux propriétés d'OpenXR qui ont façonné l'interface de `core/headset` :

- **Le champ de vision est asymétrique** : quatre demi-angles indépendants, pas
  un `fov_deg` symétrique. Une lentille voit plus loin du côté du nez, donc le
  pixel central n'est pas l'axe de vue. Une projection qui met à l'échelle un
  seul demi-angle vise faux de cet écart, silencieusement.
- **L'orientation est par œil, pas partagée.** Les Quest 3 et Quest Pro ont des
  dalles inclinées : prendre une seule pose de tête pour les deux yeux penche une
  des deux images, et un horizon penché en VR se sent bien avant de se voir.

**La position de l'œil est délibérément absente.** Un équirect est une sphère à
l'infini : aucun écart interoculaire ne peut en tirer de parallaxe. Y injecter
l'IPD n'ajouterait pas de profondeur, seulement de l'erreur. Le 360 monoscopique
dans un casque est réellement plat-mais-enveloppant, et c'est une propriété du
format, pas une insuffisance ici. Le 360 **stéréoscopique** est un *format*
différent (équirect haut/bas) : il relèverait de `core/videocache`, pas de la
géométrie.

État : `core/headset` est écrit et testé (9 tests), dont un qui prouve qu'une
tête immobile redonne **exactement** la vue plate déjà validée — le chemin casque
hérite ainsi de la preuve que `sphere_check` apporte au shader plutôt que d'en
demander une seconde. Le shader `fs_view360_eye.sc` est tenu à cette référence
par `eye_check` (écart max 1/255). `xr_check` établit ce que la machine offre
vraiment : ici bgfx en **Direct3D 11**, runtime **Oculus 1.117.0**, extension
`XR_KHR_D3D11_enable` présente.

**Ce qui reste et pourquoi ça attend le matériel** : la session OpenXR
proprement dite — swapchain partagée avec le device D3D11 de bgfx, puis la
boucle `xrWaitFrame` / `xrLocateViews` / `xrEndFrame`. Rien de tout ça ne peut
être exercé sans casque réveillé : `xrGetSystem` répond
`XR_ERROR_FORM_FACTOR_UNAVAILABLE` et il n'existe aucun runtime de simulation
côté Oculus. Écrire cette boucle à l'aveugle irait contre la règle du projet,
donc elle attend que le Quest soit branché — tout ce qu'elle composera est déjà
démontré.

> **La latence, puisque la question s'est posée** : le Quest en Link ajoute
> l'encodage et le transport USB au chemin, mais le reprojection asynchrone du
> runtime compense le mouvement de tête indépendamment de l'app. Ce qui reste
> exposé à la latence est le **scratch**, pas le regard — et c'est la même
> latence qu'à l'écran, celle que la cible « sous 30 ms » du plan de test
> mesure déjà.

### Retour LED et mapping en dur

**Retour LED.** Les 16 pads RGB de l'Elite et les 8 de la RP-8000 acceptent
probablement du MIDI entrant pour leur couleur (usage standard en intégration
Serato) — à confirmer devant le matériel. Si oui, **les pads deviennent l'UI de
l'app** : navigation de queue, chargement de deck, next, codés par couleur, sans
regarder l'écran.

### Pont Unreal — plugin `ScratchLink`

`UScratchLinkSubsystem` (`UGameInstanceSubsystem`) lit le socket UDP sur un thread
dédié et expose `GetStateAtTime(double)` : Unreal tourne à 60-120 fps, le flux
arrive vers 375 Hz ; on garde les derniers paquets et on échantillonne à
`now - offset` avec interpolation Hermite sur la position. C'est le modèle Live
Link sans le coût d'en écrire une source.

> On n'utilise pas le plugin OSC natif d'UE pour ce flux : il alloue des UObjects
> par message et dispatche sur le game thread — à 375 Hz c'est de la garbage pour
> rien. Socket UDP + struct POD (déjà le format de `core/protocol`). Le miroir OSC
> reste disponible pour TouchDesigner/Resolume.

- **`ScratchTimeMachineComponent`** — ring buffer à 120 Hz. Mode RECORD normal,
  mode SCRUB avec physique désactivée et transforms interpolés. **Le détail qui
  fait tout est la sortie de scrub** : réinjecter les vitesses dérivées du buffer,
  pas zéro, sinon la scène s'effondre au lieu de repartir quand tu relâches le
  plateau. Version pauvre à câbler d'abord pour valider la chaîne :
  `vel → Set Global Time Dilation` clampé `[0,4]`.
- **`ScratchCameraRigComponent`** — position du plateau → distance sur une spline
  (dolly), angle d'orbite, ou dolly-zoom ; crossfader → blend ou cut entre deux
  caméras ; `accel` → camera shake.

---

## Écarts avec Serato et Resolume

Un instrument qui fait une chose inédite mais à qui manquent les gestes de base
est inutilisable en set. Voici l'inventaire qui a guidé le phasage, et ce qui a
déjà été traité.

### Ajouté depuis (déjà dans le code)

| Manque | Vient de | Traité dans |
|---|---|---|
| Boucles et hot cues | Serato | `core/transport` |
| Modes ABS / REL / INT | Serato | `core/timecode` (`TransportMode`) |
| Slip mode | Serato | `core/transport` |
| Blend modes par couche | Resolume | `core/mixer` (`BlendMode`) |
| Canal alpha | Resolume | `core/videocache` (`kCacheAlpha`, BC7/BC3) |
| Modulateurs : LFO, enveloppes | Resolume | `core/modulator` |
| Corner pin et masque | — | `core/warp` |
| Source de transport par deck | Resolume | `core/playback` (`DeckSource`) |
| Modes de lecture du clip | Resolume | `core/playback` (`ClipPlayMode`) |
| Une 3ᵉ couche d'incrustation | Resolume | `core/mixer` (`StackWeights`), `app/engine` |

> **Une horloge libre est un intégrateur, et le principe n° 1 l'interdit.**
> `pos += rate * dt` ne se scratche pas, ne se rejoue pas à l'identique et dérive
> sur la durée d'un set. `core/playback` la remplace donc par une **forme close du
> temps absolu** — `pos = origine + rate·(t − t₀)` — où tout changement de vitesse
> rebase l'origine plutôt que d'accumuler. Trois heures plus tard la position est
> encore exacte, et un test l'énonce ainsi plutôt que par un chiffre.
>
> Le mode de lecture n'est alors pas un lecteur mais un **repliement de la
> position** : la même fonction pure sert au fond en aller-retour et au deck
> scratché qui dépasse la fin du clip. Elle s'applique **après** `core/transport`,
> jamais avant — une boucle utilisateur posée à cheval sur la fin du clip serait
> sinon repliée avant que le transport ne la voie.
>
> **La couche d'incrustation ne répond pas au crossfader** (`stack_weights`). Un
> logo qui disparaît à chaque transition est pire que pas de logo, et un masque
> qui s'ouvre en plein transform montre exactement ce qu'il existe pour cacher.
> La 3ᵉ couche est la part de l'image qui **survit** à la transition en dessous
> d'elle. C'est aussi le premier consommateur de `core/playback` dans le moteur :
> elle n'a pas de plateau, donc son horloge est sa seule entrée.
>
> **Prise en main du plateau** (`TakeoverMode`) : trois politiques, `Grab` par
> défaut. Le plateau reprend la main **là où le clip est déjà**, ancré par un
> offset, donc on peut attraper une boucle en vol pour la scratcher sans que
> l'image saute. Le prix, assumé : la position du plateau ne veut plus rien dire
> en absolu — le même compromis que `core/anchor` fait déjà en mode suiveur, et
> pour la même raison. La couche d'incrustation, elle, est en `Ignore` : une main
> qui traîne sur un plateau ne doit jamais capturer le masque.

### Encore à ajouter

| Manque | Vient de | Pourquoi c'est bloquant |
|---|---|---|
| ~~**Réactivité audio (FFT)**~~ | Resolume | **Fait** : `core/spectrum` (fenêtre de Hann, FFT radix-2, bandes log) remplit le tableau `bands` que `SourceKind::AudioBand` lisait déjà. Reste à lui donner du vrai son plutôt que celui du script — c'est-à-dire le backend audio. |
| **Entrées live** | Resolume | Un deck dont la source est une caméra, une entrée NDI ou Spout, au lieu d'un fichier. |
| ~~**Scope de calibration timecode**~~ | Serato | **Fait** : `core/scope` mesure la figure de Lissajous et la sépare en ses trois défauts (centre, balance en dB, erreur de phase en degrés), `ui/panels` la dessine dans le tiroir « diagnostic platine » du deck A. Voir la note ci-dessous. |

### À ne pas faire — et pourquoi

| Écarté | Raison |
|---|---|
| **Edge blending multi-projecteurs, slices** | Projet à soi seul. On sort en **Spout / NDI** vers Resolume ou MadMapper pour ces cas-là. |
| **Compositing N couches, groupes** | On construit un instrument de scratch, pas un VJ compositeur généraliste. Trois couches suffisent. La **matrice de clips** n'est plus dans cette ligne : décidée le 2026-09-07 sous la forme de banques de clips déclenchées par les pads **sur ces trois couches** (montage live par cut, pas compositing), elle respecte les deux principes et arrive avec la surface complète. |
| **Sync, beatmatch, détection de tonalité, mix harmonique** | Antithétique au turntablisme, et couvert par Serato en mode suiveur. |
| **Ableton Link, horloge MIDI** | Reporté. Le mapping OSC ouvre déjà une porte. |
| **DMX / Art-Net** | Reporté. Atteignable plus tard par la couche de mapping. |
| **Intégration de services de streaming, gestion de bibliothèque avancée** | Hors sujet pour de la vidéo. |
| **Un panneau dessiné pour la RP-8000** | Écarté le 2026-09-09, deux raisons. Le modèle donne une place par identifiant, et la platine en déclare 24 pour 8 pads réels (les couches sont un état de l'appareil, invisible d'ici). Et surtout : le panneau de l'Elite sert à trouver un potard parmi quarante ; huit pads en une seule bande, déjà dessinés quatre par rangée, n'ont rien à y gagner. La rangée reste, avec une note qui dit que les trois couches sont les mêmes huit pads. |

### Ce que ni l'un ni l'autre ne fait

À garder en tête quand on arbitre : **les effets appairés audio/vidéo**, la
**vidéo scratchée par indexation** plutôt que par seek, la **360 dont le regard se
pilote pendant qu'on scratche le temps**, et les **grandeurs de geste** (taux de
scratch, backspin) comme sources de modulation de plein droit. C'est là qu'il faut
dépenser l'effort ; le reste, c'est du rattrapage.

---

## Idées d'usage

**Vidéo** — deux decks scratchés indépendamment et mixés comme un vrai mixer
vidéo ; beat juggling visuel où l'écart entre positions A et B pilote la
parallaxe de deux couches ; stutter quantifié pendant qu'un pad est tenu ;
glitch piloté par `scratch_rate` ; inversion de chroma au changement de sens ;
cut stroboscopique au crab/transform.

**360** — pan/tilt au potard pendant qu'on scratche le temps ; crossfader entre
deux regards du même clip ; little planet dont la rotation suit le roll ; backspin
déclenchant un whip pan ; panoramique audio lié au yaw.

**Unreal** — rewind physique réel (verre qui se recompose sous la main) ; scratch
d'un Level Sequence ; dolly sur spline et orbite au plateau ; cut multi-caméra au
crossfader ; mix vidéo Spout plaqué sur des écrans dans une scène 3D et déformé ;
wipe entre deux mondes au crossfader.

**Show** — pads en navigation de queue codée par couleur ; enregistrement d'une
prise et rendu offline propre ; sortie NDI vers régie ou stream.

---

## Vérification, une fois le matériel branché

1. `--midi-learn` puis parcourir la table — le crossfader doit aller de -1 à +1,
   les pads s'allumer. Tester si l'Elite émet son état à la connexion (décide du
   sort du mode fantôme).
2. **Avant toute ligne de vidéo** : `--monitor` doit montrer une position qui
   monte linéairement à vitesse nominale, un sens qui s'inverse proprement, une
   `confidence` haute. Si le lock est instable, essayer les autres définitions de
   timecode. **Puis, Serato lancé en parallèle**, vérifier que l'app lit toujours
   le timecode — c'est le test qui tranche le tableau des quatre voies de partage
   d'entrée.
3. **Ancrage** : poser l'ancrage sur un pad, laisser tourner deux minutes, lire la
   fraîcheur affichée. Puis provoquer un needle drop dans Serato et vérifier qu'un
   seul appui de pad remet tout en place.
4. **Mode autonome** : test d'écoute — scratch, spinback, needle drop. Si le
   feeling n'est pas celui du vinyle, c'est le resampling à revoir, pas la
   latence.
5. **Analyse vidéo** : jeu de clips variés (codecs, fps, résolutions) en
   non-régression ; mesure du temps de recentrage de la fenêtre VRAM après un
   needle drop hors fenêtre.
6. **Effets appairés** : test perceptif, pas seulement fonctionnel — un tiers qui
   n'a pas écrit le code doit reconnaître l'effet audio en ne regardant que
   l'image. C'est le seul critère qui compte.
7. **Décrochage** : forcer `confidence` à 0 doit **geler** proprement — jamais de
   téléportation de l'image ni de la caméra Unreal. (Déjà vérifié en logique pure
   par les tests de `core/timecode` ; à reconfirmer avec le vrai matériel.)
8. **Latence de bout en bout** : filmer main + écran au ralenti, compter les
   frames d'écart. Cible sous 30 ms pour que ce soit bon visuellement, sous 20 ms
   pour que ça « colle » à la main.

---

## Risques

**Risque principal, contenu par le mode suiveur : le moteur audio DVS.** Le mode
suiveur ne demande pas de moteur audio propre : le produit sera jouable dès que
FFmpeg + le décodage vidéo existeront, sans attendre le mode autonome. Il reste un
risque de *qualité* — un scratch audio qui « sonne » mal serait décevant.

**Risque devenu principal : le partage de l'entrée audio sous Windows en mode
suiveur.** Les pilotes ASIO DJ sont souvent mono-client. Le tableau des quatre
voies plus haut est à trancher **devant le matériel**, en testant d'abord le
second port USB de l'Elite.

**Risque secondaire : le mapping et le retour LED de l'Elite** ne sont pas
documentés publiquement. Déjà traité par construction avec `--midi-learn`
(`core/learn`) plutôt que par des valeurs en dur.

**Risque tiers : la VRAM en 360 haute résolution.** Le budget par deck est un
réglage exposé (`core/framewindow`), et [`format-cache.md`](format-cache.md)
donne les compromis. En 4K équirect sur une carte modeste, il faudra réduire la
fenêtre ou la résolution d'analyse.

## Licence

`timecoder.c` est **GPL-3** : `scratchvj` sera donc GPL-3 s'il est distribué.
FFmpeg est LGPL en lien dynamique ; bgfx, SDL3, Dear ImGui, miniaudio et RtMidi
sont permissifs. Le **plugin Unreal reste sous la licence de ton choix** puisqu'il
ne fait que lire un socket UDP et une texture partagée — la frontière entre les
deux processus n'est pas seulement architecturale, elle est aussi juridique. Le
SDK ASIO se compile contre, mais ne se redistribue pas.
