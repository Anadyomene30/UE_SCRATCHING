#include "core/destinations.h"

namespace svj {

const std::vector<DestinationSpec>& destination_catalogue() {
    static const std::vector<DestinationSpec> table = {
        {"mix.transition", Dest::MixTransition, false, "la transition du crossfader : cut, fondu, additif, multiplié, screen, wipe luma, wipe, RVB, zoom"},
        {"mix.xfader.curve", Dest::MixXfaderCurve, false, "courbe du crossfader : douce, linéaire, sharp, cut"},
        {"mix.fader.curve", Dest::MixFaderCurve, false, "courbe des faders de voie"},
        {"mix.xfader.reverse", Dest::MixXfaderReverse, false, "crossfader inversé (hamster)"},
        {"mix.fader.reverse", Dest::MixFaderReverse, false, "faders de voie inversés"},
        {"mix.fader.a.curve", Dest::MixFaderACurve, false, "courbe du fader de voie 1 (deck A)"},
        {"mix.fader.b.curve", Dest::MixFaderBCurve, false, "courbe du fader de voie 2 (deck B)"},
        {"mix.fader.a.reverse", Dest::MixFaderAReverse, false, "fader de voie 1 inversé"},
        {"mix.fader.b.reverse", Dest::MixFaderBReverse, false, "fader de voie 2 inversé"},

        {"deck.a.yaw", Dest::DeckAYaw, false, "regard 360 du deck A : lacet (degrés)"},
        {"deck.a.pitch", Dest::DeckAPitch, false, "regard 360 du deck A : tangage (degrés)"},
        {"deck.a.fov", Dest::DeckAFov, false, "champ du deck A (degrés)"},
        {"deck.a.zoom", Dest::DeckAZoom, false, "zoom little planet / fisheye du deck A"},

        {"deck.a.cue.1", Dest::DeckACue1, true, "saut au hot cue 1 du deck A"},
        {"deck.a.cue.2", Dest::DeckACue2, true, "saut au hot cue 2 du deck A"},
        {"deck.a.cue.3", Dest::DeckACue3, true, "saut au hot cue 3 du deck A"},
        {"deck.a.cue.4", Dest::DeckACue4, true, "saut au hot cue 4 du deck A"},
        {"deck.a.cue.5", Dest::DeckACue5, true, "saut au hot cue 5 du deck A"},
        {"deck.a.cue.6", Dest::DeckACue6, true, "saut au hot cue 6 du deck A"},
        {"deck.a.cue.7", Dest::DeckACue7, true, "saut au hot cue 7 du deck A"},
        {"deck.a.cue.8", Dest::DeckACue8, true, "saut au hot cue 8 du deck A"},
        {"deck.b.cue.1", Dest::DeckBCue1, true, "saut au hot cue 1 du deck B"},
        {"deck.b.cue.2", Dest::DeckBCue2, true, "saut au hot cue 2 du deck B"},
        {"deck.b.cue.3", Dest::DeckBCue3, true, "saut au hot cue 3 du deck B"},
        {"deck.b.cue.4", Dest::DeckBCue4, true, "saut au hot cue 4 du deck B"},
        {"deck.b.cue.5", Dest::DeckBCue5, true, "saut au hot cue 5 du deck B"},
        {"deck.b.cue.6", Dest::DeckBCue6, true, "saut au hot cue 6 du deck B"},
        {"deck.b.cue.7", Dest::DeckBCue7, true, "saut au hot cue 7 du deck B"},
        {"deck.b.cue.8", Dest::DeckBCue8, true, "saut au hot cue 8 du deck B"},
        {"deck.a.loop.in", Dest::DeckALoopIn, true, "pose une boucle sur le deck A"},
        {"deck.a.loop.out", Dest::DeckALoopOut, true, "ferme la boucle du deck A ici"},
        {"deck.a.loop.exit", Dest::DeckALoopExit, true, "sort de la boucle du deck A"},
        {"deck.b.loop.in", Dest::DeckBLoopIn, true, "pose une boucle sur le deck B"},
        {"deck.b.loop.out", Dest::DeckBLoopOut, true, "ferme la boucle du deck B ici"},
        {"deck.b.loop.exit", Dest::DeckBLoopExit, true, "sort de la boucle du deck B"},
        {"deck.a.slip", Dest::DeckASlip, false, "slip du deck A tant que c'est tenu"},
        {"deck.b.slip", Dest::DeckBSlip, false, "slip du deck B tant que c'est tenu"},
        {"deck.a.load_next", Dest::DeckALoadNext, true, "charge le suivant de la file sur A"},
        {"deck.b.load_next", Dest::DeckBLoadNext, true, "charge le suivant de la file sur B"},
        {"overlay.load_next", Dest::OverlayLoadNext, true, "charge le suivant de la file sur l'incrustation"},

        {"fx.1.mix", Dest::Fx1Mix, false, "rack, slot 1 : mix"},
        {"fx.1.amount", Dest::Fx1Amount, false, "rack, slot 1 : amount"},
        {"fx.1.time", Dest::Fx1Time, false, "rack, slot 1 : time"},
        {"fx.1.feedback", Dest::Fx1Feedback, false, "rack, slot 1 : feedback"},
        {"fx.1.depth", Dest::Fx1Depth, false, "rack, slot 1 : depth"},
        {"fx.1.tone", Dest::Fx1Tone, false, "rack, slot 1 : tone"},
        {"fx.1.on", Dest::Fx1On, false, "rack, slot 1 : actif"},
        {"fx.2.mix", Dest::Fx2Mix, false, "rack, slot 2 : mix"},
        {"fx.2.amount", Dest::Fx2Amount, false, "rack, slot 2 : amount"},
        {"fx.2.time", Dest::Fx2Time, false, "rack, slot 2 : time"},
        {"fx.2.feedback", Dest::Fx2Feedback, false, "rack, slot 2 : feedback"},
        {"fx.2.depth", Dest::Fx2Depth, false, "rack, slot 2 : depth"},
        {"fx.2.tone", Dest::Fx2Tone, false, "rack, slot 2 : tone"},
        {"fx.2.on", Dest::Fx2On, false, "rack, slot 2 : actif"},
        {"fx.3.mix", Dest::Fx3Mix, false, "rack, slot 3 : mix"},
        {"fx.3.amount", Dest::Fx3Amount, false, "rack, slot 3 : amount"},
        {"fx.3.time", Dest::Fx3Time, false, "rack, slot 3 : time"},
        {"fx.3.feedback", Dest::Fx3Feedback, false, "rack, slot 3 : feedback"},
        {"fx.3.depth", Dest::Fx3Depth, false, "rack, slot 3 : depth"},
        {"fx.3.tone", Dest::Fx3Tone, false, "rack, slot 3 : tone"},
        {"fx.3.on", Dest::Fx3On, false, "rack, slot 3 : actif"},
        {"fx.a.filter", Dest::FxFilterAmount, false, "le passe-bas du rack : cutoff et flou"},
        {"fx.a.kaleidoscope.rotation", Dest::FxKaleidoscopeRotation, false, "le kaléidoscope du rack : rotation (0..360)"},
        {"fx.a.glitch.amount", Dest::FxGlitchAmount, false, "le glitch du rack : intensité"},
        {"fx.a.bloom.amount", Dest::FxBloomAmount, false, "le bloom du rack : intensité"},

        {"overlay.opacity", Dest::OverlayOpacity, false, "opacité de l'incrustation"},
        {"overlay.enabled", Dest::OverlayEnabled, false, "incrustation visible"},

        {"library.next", Dest::LibraryNext, true, "bibliothèque : ligne suivante"},
        {"library.prev", Dest::LibraryPrev, true, "bibliothèque : ligne précédente"},
        {"library.load.a", Dest::LibraryLoadA, true, "bibliothèque : charger la ligne sur A"},
        {"library.load.b", Dest::LibraryLoadB, true, "bibliothèque : charger la ligne sur B"},
        {"library.load.o", Dest::LibraryLoadOverlay, true, "bibliothèque : charger la ligne sur l'incrustation"},
    };
    return table;
}

Dest resolve_destination(std::string_view id) {
    for (const DestinationSpec& spec : destination_catalogue()) {
        if (id == spec.id) return spec.dest;
    }
    return Dest::None;
}

const DestinationSpec* describe_destination(Dest dest) {
    for (const DestinationSpec& spec : destination_catalogue()) {
        if (spec.dest == dest) return &spec;
    }
    return nullptr;
}

bool is_trigger(Dest dest) {
    const DestinationSpec* spec = describe_destination(dest);
    return spec != nullptr && spec->trigger;
}

int cue_of(Dest dest) {
    const auto d = static_cast<std::uint16_t>(dest);
    if (d >= static_cast<std::uint16_t>(Dest::DeckACue1) &&
        d <= static_cast<std::uint16_t>(Dest::DeckACue8)) {
        return d - static_cast<std::uint16_t>(Dest::DeckACue1);
    }
    if (d >= static_cast<std::uint16_t>(Dest::DeckBCue1) &&
        d <= static_cast<std::uint16_t>(Dest::DeckBCue8)) {
        return d - static_cast<std::uint16_t>(Dest::DeckBCue1);
    }
    return -1;
}

int fx_slot_of(Dest dest) {
    const auto d = static_cast<std::uint16_t>(dest);
    const auto first = static_cast<std::uint16_t>(Dest::Fx1Mix);
    const auto last = static_cast<std::uint16_t>(Dest::Fx3On);
    if (d < first || d > last) return -1;
    return (d - first) / 7;
}

}  // namespace svj
