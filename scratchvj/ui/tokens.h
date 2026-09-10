// Filoscope — le fichier unique de jetons.
//
// LE FICHIER A DEUX PARTIES, et la frontière est un marqueur.
//
//   PARTIE 1, ENTRE LES DEUX MARQUEURS — la transcription de
//   `C:\Users\dimit\Documents\CODE\Suite 360\design\tokens.json`, lue au cadran
//   de la salle scène. Elle est ÉCRITE PAR `Suite 360\tools\distribute.py`, et
//   par rien d'autre. La corriger ici ne sert à rien : la prochaine exécution
//   écrase la correction, et personne ne saura pourquoi le défaut est revenu.
//   Si une valeur est fausse, c'est `tokens.json` qui a tort, et il se corrige
//   à son adresse.
//
//   PARTIE 2, APRÈS LE MARQUEUR DE FIN — LES VALEURS PROPRES À CE PRODUIT,
//   chacune nommée, chacune avec un commentaire qui dit pourquoi elle n'est pas
//   de la suite. L'outil n'y touche jamais.
//
// La source se lit à son adresse et n'est pas recopiée ici : ce qui descend est
// la VALEUR d'un jeton sous le nom de ce jeton, jamais une décision. La version
// se compare par la ligne `SOURCE` du bloc généré, jamais par un `diff` : la
// source est en CRLF, une copie faite par un outil peut être en LF, et un
// `diff` répondrait alors que tout a changé quand rien n'a bougé.
//
// LA RÈGLE QUE CE FICHIER EXISTE POUR TENIR : aucune valeur hexadécimale,
// aucun rayon, aucune taille, aucune fonte n'est écrite en dur ailleurs dans
// le code — pas même « juste pour celle-là ». Tout le reste référence ce
// fichier PAR NOM DE JETON. « Ailleurs » veut dire hors de ce fichier, pas hors
// de l'une de ses deux parties.
//
// UNE COULEUR EST UNE VALEUR sRGB ENCODÉE. Ce produit n'écrit aucune couleur
// d'interface dans une cible linéaire ni dans un shader : les dix `.sc` ne
// portent aucune constante de couleur, et les effacements de vue GPU sont du
// noir. La seule couleur d'interface qui sorte d'ImGui est le fond, effacé par
// bgfx — même valeur, deux notations, et les deux sortent de la même constante
// ci-dessous. Aucune conversion n'est donc due.
#pragma once

#include <cstdint>

#include "imgui.h"

namespace svj::ui::tok {

// Une valeur, deux notations : `0xRRGGBB` pour la source, `IM_COL32` pour
// ImGui, `0xRRGGBBAA` pour un effacement de vue bgfx. Écrire la couleur deux
// fois à la main est l'écart que ce fichier ferme.
constexpr ImU32 col(std::uint32_t rgb) {
    return IM_COL32((rgb >> 16) & 0xFFu, (rgb >> 8) & 0xFFu, rgb & 0xFFu, 0xFF);
}
constexpr std::uint32_t clear_rgba(std::uint32_t rgb) { return (rgb << 8) | 0xFFu; }

// Un voile, une teinte de sélection, un liseré s'expriment DEPUIS un jeton
// nommé : une opacité posée sur un jeton n'est pas une valeur en dur, un
// `IM_COL32(0x6E, 0x86, 0x96, 0x28)` en est une. Cette fonction est la
// différence entre les deux, et elle est la seule façon de voiler une couleur
// dans ce dépôt.
constexpr ImU32 veil(ImU32 token, std::uint32_t alpha) {
    return (token & 0x00FFFFFFu) | (alpha << 24);
}

// =============================================================================
// CE QUE LE BLOC GÉNÉRÉ PORTE, ET DANS QUEL ÉTAT
//
// Les notes de lecture de la partie 1. Elles sont ici, avant le marqueur, parce
// que l'outil écrase tout ce qui est entre les marqueurs : une note posée dans
// le bloc disparaîtrait à la prochaine exécution.
// =============================================================================

// --- chassis -----------------------------------------------------------------
//
// Sept valeurs, pas une. La source les donne AVEC LEUR ÉTAT : deux des sept
// passent son test (« luminance relative WCAG à ±5 % du jeton d'atelier
// correspondant, dérive chaude R > G > B conservée »), les autres non. Elles
// sont en service, jouées dans le noir, et la correction est `SCRATCHVJ-06`,
// reportée. Rien ne change ici tant que la maison n'a pas rendu : une valeur en
// service n'est pas une valeur arrêtée, mais une valeur absente est une
// invitation à en inventer une.
//
// LA LUMINANCE DU CHÂSSIS EST ISOLÉE À UN SEUL ENDROIT, et c'est le bloc
// ci-dessous : les sept lignes du châssis sont les seules du dépôt à porter une
// de ces valeurs. Le jour où `SCRATCHVJ-06` est rendue, la transcription est un
// changement de sept nombres dans `tokens.json` et de rien d'autre. Le candidat
// que la source écrit pour `void` est `#0D0C0A`.
//
// `raised` — et le nom du jeton dit l'inverse de ce que la scène en fait. À
// l'atelier le champ de saisie est RELEVÉ, plus clair que le panneau ; ici il
// est CREUSÉ, plus sombre. La source le note comme « HORS TEST, et c'est une
// question distincte » : ce n'est pas une teinte, c'est une convention de
// relief, qu'aucune des sept variables du cadran ne couvre, et elle est
// OUVERTE. Ne pas la trancher ici ; elle change en une ligne.

// --- pair --------------------------------------------------------------------
//
// La paire opératoire A / B. C'est de l'ÉTAT et non de l'identité : elle dit de
// quelle source on parle, et elle est constante sur toute la ligne scène — deux
// produits de scène qui la choisiraient chacun l'annuleraient. Emploi : une
// étiquette d'un ou deux caractères, « A », « B », qui n'est pas du texte au
// sens de la règle du `fail`.
//
// L'AMBRE APPARTIENT AU DECK A, et à rien d'autre (`SCENE.md`, variable 4).

// --- signal ------------------------------------------------------------------
//
// Mêmes teintes, mêmes sens que l'atelier, valeurs ajustées au châssis chaud.
// La ligne hérite des QUATRE, pas de trois.
//
// `running` n'est pas transcrit, et l'absence est écrite plutôt que tue : la
// source dit qu'il « ne se re-règle PAS et ne se peint pas » — à la scène le
// travail long n'existe pas, l'ambre appartient au deck A, et un travail en
// cours se montre par la progression nommée, jamais par un point.
//
// `warn` n'a pas de valeur dans la source : c'est ce produit qui devait la
// produire. Elle est en partie 2, avec sa dérivation.

// --- accents -----------------------------------------------------------------
//
// L'ACCENT DU PRODUIT, et il n'est plus candidat : le nom est arrêté, et la
// teinte entre dans `tokens.json` v4 sous la clé `filoscope`. Elle descend
// désormais comme les autres, et ne se choisit plus ici.

// --- rhythm ------------------------------------------------------------------
//
// `param_row` reste à 28 : c'est l'invariant, et `lines.scene` ne le redéfinit
// pas. `hit_target_min` passe de 20 à 44, et c'est la scène qui le dit — une
// cible qu'on doit atteindre SANS LA REGARDER, une main sur le plateau, a une
// taille minimale, et ce n'est pas celle qu'on lit. Les deux coexistent sans se
// contredire : une ligne de paramètre se lit et se vise des yeux, un bouton de
// transport et un pad se frappent à l'aveugle.
//
// Le rayon ne bouge pas d'une salle à l'autre : le produit a appliqué le test
// du cadran contre son propre confort, n'a pas su écrire la raison, et la
// variable est retournée à l'invariant (`SCRATCHVJ-12`).
//
// `accent_cap` : un filet plein de 2 px à gauche de la barre haute, le seul
// endroit où la couleur du produit le nomme.
//
// `chassis_bar` vaut 40 et non 32 : la source donne les deux et dit laquelle
// décide — la hauteur dépend de qui dessine le cadre de la fenêtre. Ce produit
// dessine le sien, et l'outil lit donc le cas `own_decoration`.

// --- motion ------------------------------------------------------------------
//
// 0 ms, et non 120. L'interface est À CÔTÉ de l'image projetée, et toute
// animation d'interface prend de l'attention à l'image. La constante descend de
// la source et vaut zéro ; rien ne la consomme, et c'est le bon sens du vide :
// `ImGuiStyle` n'expose aucun champ de durée de transition, et le dépôt ne
// contient aucune animation d'interface. Le jour où l'un des deux change, c'est
// cette constante-là qu'on lit, et pas un nombre écrit sur place.

// JETONS DE LA MAISON — GENERE, NE PAS EDITER
//
// SOURCE   Suite 360/design/tokens.json v4 (2026-09-10)
// OUTIL    Suite 360/tools/distribute.py
// SALLE    scene
//
// Ce bloc est ecrit par l'outil. Le corriger ici ne sert a rien : la
// prochaine execution ecrase la correction. Si une valeur est fausse,
// c'est tokens.json qui a tort, et il se corrige a son adresse.
//
// Ce qui suit le marqueur de fin appartient a ce produit et n'est
// jamais touche par l'outil.
//
// Produit de la salle scene : les valeurs viennent de
// lines.scene quand elle les nomme, de la base sinon.

// --- chassis_brut ---
constexpr std::uint32_t kGroundRgb = 0x141412;  // efface le swapchain

// --- chassis ---
constexpr ImU32 kGround = col(kGroundRgb);  // fond derriere tout rendu 3D ou video
constexpr ImU32 kPanel  = col(0x1A1917);  // fond des panneaux — OPAQUE, ni transparence ni flou
constexpr ImU32 kWell   = col(0x0E0E0C);  // champs, lignes survolees, etat actif
constexpr ImU32 kHair   = col(0x2E2D28);  // filets 1px, jamais deux adjacents
constexpr ImU32 kInk    = col(0xE9E6DF);  // texte principal
constexpr ImU32 kMuted  = col(0x8A867C);  // libelles secondaires, unites
constexpr ImU32 kFaint  = col(0x605D56);  // controle indisponible — jamais l'opacite

// --- signal ---
constexpr ImU32 kSage  = col(0x7E946B);  // termine sans reserve
constexpr ImU32 kAlert = col(0xB54B3A);  // echec — la seule couleur autorisee a porter du texte

// --- pair ---
constexpr ImU32 kAmber = col(0xC99A2F);  // deck A - ambre
constexpr ImU32 kSlate = col(0x6E8696);  // deck B - ardoise

// --- accents ---
constexpr ImU32 kAccent = col(0xC9762F);

// --- type ---
constexpr float kBodyPx = 15.0f;  // la distance de lecture, variable 1 du cadran

// --- rhythm ---
constexpr float kParamRow      = 28.0f;  // hauteur d'une ligne de parametre
constexpr float kControlRadius = 3.0f;
constexpr float kPanelRadius   = 0.0f;
constexpr float kAccentCap     = 2.0f;  // largeur de tout filet d'accent
constexpr float kHitTargetMin  = 44.0f;

// --- chassis_bar ---
constexpr float kChassisBar = 40.0f;  // own_decoration

// --- motion ---
constexpr int kStateMs = 0;

// FIN DES JETONS DE LA MAISON

// =============================================================================
// PARTIE 2 — les valeurs propres à ce produit
//
// Chacune nommée, chacune avec la raison pour laquelle elle n'est pas de la
// suite. Elles sont ici parce qu'un besoin qui n'a qu'une occurrence dans
// toute la maison échoue au test de recevabilité de `design/tokens.json` et
// n'entre pas dans les jetons communs — mais il lui faut un endroit légal où
// vivre, sinon « aucune valeur hexadécimale ailleurs » est inatteignable.
// =============================================================================

// Le fond, en notation bgfx. Ce n'est pas une seconde valeur : c'est la même,
// dérivée du jeton généré par la seule fonction qui sache la ré-encoder. Elle
// est ici et non dans le bloc parce que la suite ne connaît qu'une notation de
// couleur, et que celle-ci appartient à la couche graphique de ce produit.
constexpr std::uint32_t kGroundClearRgba = clear_rgba(kGroundRgb);

// `signal.warn` POUR LA SCÈNE — « ça marche encore, mais ça dérive ». La ligne
// hérite des quatre signaux et n'en avait que trois ; la maison a demandé à ce
// produit de produire la valeur chaude, en donnant `warn` au lien de platine
// qui faiblit (`SCRATCHVJ-09`).
//
// DÉRIVÉE, PAS CHOISIE : le `warn` d'atelier #C48A4B, à teinte égale, avec sa
// clarté et sa saturation HSL multipliées par la moyenne des rapports que cette
// table applique déjà à `done` (#7FB069 → #7E946B) et à `fail` (#D9584B →
// #B54B3A) : L ×0,863, S ×0,654. Sa luminance relative tombe à 0,684 de la
// valeur d'atelier, entre celle de `done` (0,729) et celle de `fail` (0,682).
//
// CANDIDATE tant que `tokens.json` ne porte pas `lines.scene.signal.warn` : sa
// teinte est à 4° de l'accent du produit, ce qui est remonté (`SCRATCHVJ-21`).
constexpr ImU32 kWarn = col(0x9C774E);

// LES DEUX FONTES, et la distinction qu'elles portent : les mots dans la
// famille de texte, TOUTE VALEUR dans la mono. Jamais l'inverse. C'est la
// constante la plus forte de l'invariant. Ce qui est propre au produit n'est
// pas la distinction — elle est de la suite — mais le CHEMIN DU FICHIER
// embarqué : la suite nomme une famille, un exécutable charge un `.ttf`.
//
// LA MONO EST ISOLÉE À UN SEUL ENDROIT, et c'est `kMonoFile`. La source
// prescrit Fragment Mono ; ce produit charge DM Mono, et le choix entre les
// deux pour la ligne scène n'est PAS tranché — c'est de la couche 3
// (`SCRATCHVJ-06`, reporté, et c'est à ce produit de fournir la mesure, seul du
// catalogue à tenir une chaîne PDF réelle). Ne rien changer ; le jour où la
// maison tranche, c'est cette ligne-là, et elle seule.
constexpr const char* kWordsFile = "fonts/Archivo-Variable.ttf";  // type.words — Archivo
constexpr const char* kMonoFile = "fonts/DMMono-Regular.ttf";     // type.values — voir ci-dessus

// L'ÉCHELLE, et ce qu'il en reste ici. Le corps de texte descend de la source :
// `lines.scene.type.body` vaut 15 — « 1 m dans le noir, corps 15 px » — et il
// est dans le bloc généré, sous `kBodyPx`. Les cinq crans du jeu, eux, ne sont
// PAS fixés (`scale` est `null` dans la source, `scale_candidat` porte
// [13, 15, 18, 24, 40] avec la mention « candidat, un seul produit ») : ce qui
// manque est la règle de dérivation entre salles, et c'est la maison qui doit
// l'écrire. Tant qu'elle ne l'a pas écrite, l'outil n'a que le corps à
// descendre, et les deux tailles dérivées vivent ici.
constexpr float kValuePx = kBodyPx;  // les valeurs, en mono, au même corps que les mots
// SUBSTITUT NOTÉ. Les sur-titres étaient à 11,5 px. Une taille à virgule est un
// continuum de un, et l'invariant ferme l'échelle quelle que soit la ligne :
// 11,5 disparaît sans décision de personne. Arrondi au cran voisin du jeu de ce
// produit — 13, le premier cran de `scale_candidat`. C'est le seul changement
// de taille de cette phase, et il est visible : les sur-titres grossissent.
constexpr float kLabelPx = 13.0f;

// Le voile de sélection de texte d'ImGui : l'accent à 35 %. Ce n'est pas une
// valeur en dur — une opacité posée sur un jeton n'en est pas une — mais elle
// était écrite en flottants dans un troisième endroit, ce qui en faisait un
// treizième jeton déguisé. Elle s'exprime désormais DEPUIS le jeton.
constexpr float kSelectionAlpha = 0.35f;
// La ligne de tableau alternée : un blanc à 1,5 %, seule surface du produit qui
// ne sort d'aucun jeton de châssis. Le filet de 1 px ne peut pas rendre une
// alternance de lignes, et le rang suivant du châssis (`panel` sur `void`)
// serait un aplat. Propre au produit : aucun autre n'a de tableau à rangs.
constexpr ImU32 kRowAlt = IM_COL32(0xFF, 0xFF, 0xFF, 0x04);  // 0x04/255 ≈ 1,5 %

// LE FILET D'UNE LIGNE DE LISTE. Plus clair que le panneau, plus sombre que le
// filet du châssis : une liste de clips a besoin d'une séparation qui ne se lise
// pas comme un bord de panneau, et le châssis n'a que sept valeurs. Propre au
// produit — aucun autre n'a de liste à rangs de cette densité — et à retirer le
// jour où les trois étages de la variable 7 donneront la sienne.
constexpr ImU32 kRowLine = col(0x232220);

// LES VOILES, en une table plutôt qu'à chaque site : ce sont des opacités
// posées sur un jeton, et elles n'ont de sens que par leur emploi.
constexpr std::uint32_t kVeilResident = 0x28;   // sur `pair.b` — les frames en mémoire, sur la bande
constexpr std::uint32_t kVeilLoop = 0x33;       // sur l'accent — l'intervalle bouclé, sur la bande
constexpr std::uint32_t kVeilDropTarget = 0x30; // sur l'accent — la case de banque sous un glisser
constexpr std::uint32_t kVeilRowHover = 0x80;   // sur `chassis.panel` — la ligne survolée
constexpr std::uint32_t kVeilMaskIdle = 0x60;   // sur `pair.b` — le masque, outil non armé

// LE RYTHME D'IMGUI. Dix nombres, dont quatre ne sont pas des multiples de
// l'unité de 4 de la suite — `10`, `6`, `6` et `18`. Ils NE SE RÉGULARISENT
// PAS : la variable 7 du cadran, la densité en trois étages, est la seule des
// sept sans aucun nombre, et régulariser aujourd'hui déciderait par la bande ce
// que la maison doit trancher au tour 01 d'un second produit de scène
// (`SCRATCHVJ-05`, reporté). Ils sont nommés ici pour qu'ils changent en un
// endroit le jour où les trois étages existent.
constexpr float kCellPadX = 8.0f;
constexpr float kCellPadY = 4.0f;
constexpr float kItemSpaceX = 10.0f;
constexpr float kItemSpaceY = 6.0f;
constexpr float kItemInnerX = 6.0f;
constexpr float kItemInnerY = 4.0f;
constexpr float kWindowPadX = 18.0f;
constexpr float kWindowPadY = 16.0f;
constexpr float kFramePadX = 8.0f;
constexpr float kFramePadY = 4.0f;

// LES TROIS ÉTAGES DE LA DENSITÉ — nommés, sans valeurs. La variable 7 demande
// « trois niveaux, dont un lisible à un mètre » et ne donne aucun nombre ; un
// produit ne peut pas se conformer à une colonne vide. Ce que la maison a
// demandé en attendant est de NOMMER les étages — quel panneau est à quel
// étage — pour que la réponse se pose sur un découpage déjà écrit au lieu d'en
// trouver un implicite. Le voici, et il n'invente aucune valeur :
//
//   ÉTAGE 1 — ce qui se lit à un mètre, d'un coup d'œil, pendant qu'une main
//             tient le plateau : la barre haute (état des platines, de la
//             table, tempo), l'en-tête de chaque deck (lettre, nom, définition,
//             projection), le transport, les pads, la bande PROGRAMME.
//   ÉTAGE 2 — ce qui se lit en se penchant, entre deux morceaux : les lignes de
//             paramètre des decks (source, lecture, boucle, saut, vue 360), la
//             colonne du mixer, le rail de bibliothèque.
//   ÉTAGE 3 — ce qui ne se lit pas en jouant, et qui n'a pas à être lisible à
//             un mètre : les tiroirs de diagnostic, l'écran TABLE, l'écran
//             SORTIE, l'écran RÉGLAGES, l'inspecteur de la bibliothèque.
//
// Ce découpage est une DESCRIPTION de ce que le produit fait déjà, pas une
// prescription : il n'a pas de valeurs, et il n'en aura pas ici.

}  // namespace svj::ui::tok
