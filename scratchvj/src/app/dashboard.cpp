#include "app/dashboard.h"

#include <cmath>
#include <cstdio>
#include <sstream>

namespace svj {
namespace {

const char* kReset = "\033[0m";
const char* kDim = "\033[38;5;242m";
const char* kAmber = "\033[38;5;173m";
const char* kSlate = "\033[38;5;109m";
const char* kSage = "\033[38;5;108m";
const char* kAlert = "\033[38;5;131m";
const char* kBright = "\033[38;5;253m";

struct Paint {
    bool ansi = false;
    const char* operator()(const char* code) const { return ansi ? code : ""; }
};

std::string clock_of(double seconds) {
    if (seconds < 0.0) seconds = 0.0;
    const int minutes = static_cast<int>(seconds) / 60;
    const double rest = seconds - minutes * 60;
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%02d:%04.1f", minutes, rest);
    return buffer;
}

std::string number(double value, int width, int decimals) {
    char format[16];
    std::snprintf(format, sizeof(format), "%%%d.%df", width, decimals);
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), format, value);
    return buffer;
}

// A horizontal bar. An unknown control is drawn as dots rather than as an empty
// bar, because an empty bar reads as "at zero" and that is a lie.
std::string bar(float value, int width, bool known) {
    std::string out;
    if (!known) {
        out.assign(static_cast<std::size_t>(width), '.');
        return out;
    }
    const int filled = static_cast<int>(value * static_cast<float>(width) + 0.5f);
    for (int i = 0; i < width; ++i) out += (i < filled) ? '#' : '-';
    return out;
}

const char* link_text(LinkState link) {
    switch (link) {
        case LinkState::Ok: return "OK  ";
        case LinkState::Degraded: return "FAIB";
        case LinkState::Lost: return "PERDUE";
    }
    return "?";
}

// The filmstrip: the clip end to end, the resident VRAM window bracketed, the
// playhead, the loop if one is running.
std::string filmstrip(const DeckView& deck, int width) {
    std::string strip(static_cast<std::size_t>(width), '.');
    if (deck.clip == nullptr || deck.clip->frame_count == 0) return strip;

    const auto column = [&](std::uint32_t frame) {
        const double fraction =
            static_cast<double>(frame) / static_cast<double>(deck.clip->frame_count - 1);
        int index = static_cast<int>(fraction * (width - 1) + 0.5);
        if (index < 0) index = 0;
        if (index >= width) index = width - 1;
        return static_cast<std::size_t>(index);
    };

    if (deck.window != nullptr && !deck.window->resident().empty()) {
        const FrameRange r = deck.window->resident();
        const std::size_t first = column(r.first);
        const std::size_t last = column(r.last());
        for (std::size_t i = first; i <= last && i < strip.size(); ++i) strip[i] = '=';
        // A window of a few dozen 4K equirect frames really is sub-pixel against a
        // four minute clip. Collapse it to one mark rather than drawing brackets
        // that overlap into nonsense -- the smallness is the honest message.
        if (first == last) {
            strip[first] = '#';
        } else {
            strip[first] = '[';
            strip[last] = ']';
        }
    }

    if (deck.transport != nullptr && deck.transport->loop().active) {
        const std::uint32_t start = deck.clip->frame_at(deck.transport->loop().start_s);
        const std::uint32_t end = deck.clip->frame_at(deck.transport->loop().end_s);
        for (std::size_t i = column(start); i <= column(end) && i < strip.size(); ++i) {
            strip[i] = 'o';
        }
    }

    if (deck.transport != nullptr) {
        strip[column(deck.clip->frame_at(deck.transport->position_s()))] = 'V';
    }
    return strip;
}

void render_deck(std::ostringstream& out, const Paint& p, const DeckView& deck,
                 const char* accent) {
    if (deck.timecode == nullptr) return;

    out << p(accent) << "  " << deck.name << p(kReset) << "\n";

    out << "    " << p(kDim) << "pos " << p(kReset)
        << clock_of(deck.transport != nullptr ? deck.transport->position_s() : 0.0);

    const float velocity = deck.timecode->velocity;
    out << "   " << p(kDim) << "vit " << p(std::fabs(velocity) > 2.5f ? kAlert : kBright)
        << number(velocity, 6, 2) << "x" << p(kReset);

    if (deck.gestures != nullptr) {
        out << "   " << p(kDim) << "scratch " << p(kReset)
            << number(deck.gestures->scratch_rate(), 4, 1) << "/s";
    }

    out << "   " << p(kDim) << "liaison " << p(kReset);
    switch (deck.timecode->link) {
        case LinkState::Ok: out << p(kSage); break;
        case LinkState::Degraded: out << p(kAmber); break;
        case LinkState::Lost: out << p(kAlert); break;
    }
    out << link_text(deck.timecode->link) << p(kReset);

    if (deck.timecode->platter_stopped) out << p(kDim) << "  [plateau arrêté]" << p(kReset);
    if (deck.timecode->link == LinkState::Lost) out << p(kAlert) << "  << GELÉ" << p(kReset);
    out << "\n";

    out << "    " << p(accent) << filmstrip(deck, 62) << p(kReset) << "\n";

    out << "    ";
    if (deck.transport != nullptr) {
        const Transport& t = *deck.transport;
        out << p(kDim) << "boucle " << p(kReset);
        if (t.loop().active) {
            out << p(kSage) << number(t.loop().length_s(), 4, 2) << "s" << p(kReset);
        } else {
            out << p(kDim) << "  -  " << p(kReset);
        }
        out << p(kDim) << "  slip " << p(t.slip() ? kAmber : kDim)
            << (t.slip() ? "ON " : "off") << p(kReset);
        out << p(kDim) << "  cues " << p(kReset);
        for (int i = 0; i < kHotCueCount; ++i) out << (t.cue(i).set ? '#' : '.');
    }
    if (deck.window != nullptr && deck.clip != nullptr) {
        out << p(kDim) << "   vram "
            << number(deck.window->window_seconds(deck.clip->frame_duration_s()), 5, 1) << "s"
            << p(kReset);
    }
    out << "\n";
}

}  // namespace

std::string render_dashboard(const DashboardView& view, const DeckView& a, const DeckView& b,
                             const Surface& surface, const MappingEngine& mapping, bool ansi) {
    const Paint p{ansi};
    std::ostringstream out;

    if (ansi) out << "\033[H\033[J";  // home, then clear

    out << p(kBright) << "scratchvj" << p(kReset) << p(kDim) << "  demo" << p(kReset) << "   "
        << clock_of(view.elapsed_s) << "   " << p(kAmber) << view.phase << p(kReset) << "\n";

    out << p(kDim) << "mode " << p(kReset) << (view.follower_mode ? "SERATO" : "AUTONOME")
        << p(kDim) << "   bpm " << p(kReset) << number(view.bpm, 5, 1);
    if (view.anchor != nullptr && view.anchor->armed()) {
        const float staleness = view.anchor->staleness(view.elapsed_s, view.jump_count);
        out << p(kDim) << "   fraîcheur " << p(staleness > 0.5f ? kAmber : kSage)
            << bar(1.0f - staleness, 10, true) << p(kReset) << p(kDim) << " sauts "
            << p(kReset) << view.jump_count;
    }
    out << "\n\n";

    render_deck(out, p, a, kAmber);
    out << "\n";
    render_deck(out, p, b, kSlate);
    out << "\n";

    out << p(kDim) << "  SURFACE" << p(kReset) << p(kDim)
        << "   (points = jamais touché depuis le lancement)" << p(kReset) << "\n";
    for (std::size_t i = 0; i < surface.size(); ++i) {
        const Control& c = surface.at(static_cast<ControlIndex>(i));
        out << "    " << p(c.known ? kReset : kDim);
        std::string label = c.id;
        label.resize(14, ' ');
        out << label << " " << bar(c.value, 24, c.known) << " ";
        if (c.known) out << number(c.value, 5, 2);
        else out << p(kDim) << "    ?";
        out << p(kReset) << "\n";
    }
    out << "\n";

    out << p(kDim) << "  MIX" << p(kReset) << "\n";
    out << "    " << p(kDim) << "A " << p(kAmber) << bar(view.weights.a, 16, true)
        << p(kReset) << p(kDim) << "  B " << p(kSlate) << bar(view.weights.b, 16, true)
        << p(kReset);
    if (view.cuts != nullptr) {
        out << p(kDim) << "   coupes " << p(view.cuts->transforming() ? kAmber : kReset)
            << number(view.cuts->cuts_per_second(), 4, 1) << "/s" << p(kReset);
        if (view.cuts->transforming()) out << p(kAmber) << "  TRANSFORM" << p(kReset);
    }
    out << "\n";

    if (view.rack != nullptr) {
        for (std::size_t i = 0; i < view.rack->size(); ++i) {
            const EffectUnit& unit = view.rack->at(i);
            if (!unit.enabled) continue;
            const EffectDescriptor* d = describe(unit.type);
            std::string id = d != nullptr ? d->id : "?";
            id.resize(12, ' ');
            out << "    " << p(kDim) << id << p(kReset)
                << (unit.link ? p(kSage) : p(kAlert)) << (unit.link ? "lié   " : "délié ")
                << p(kReset) << p(kDim) << "audio " << p(kReset)
                << number(unit.audio_params().mix, 5, 2) << p(kDim) << "  vidéo " << p(kReset)
                << number(unit.video_params().mix, 5, 2);
            if (unit.sync.tempo) {
                out << p(kDim) << "  sync " << p(kReset) << number(unit.sync.beats, 4, 2)
                    << p(kDim) << " temps" << p(kReset);
            }
            out << "\n";
        }
    }

    if (view.modulators != nullptr && view.modulators->size() > 0) {
        out << "    " << p(kDim) << "modulateurs " << p(kReset);
        for (const float value : view.modulators->values()) out << number(value, 6, 2);
        out << "\n";
    }
    out << "\n";

    out << p(kDim) << "  MAPPING" << p(kReset) << "\n";
    for (std::size_t i = 0; i < mapping.size(); ++i) {
        const Mapping& m = mapping.at(i);
        std::string label = m.name;
        label.resize(34, ' ');
        out << "    " << p(kDim) << label << p(kReset);
        if (mapping.active(i)) out << number(mapping.value(i), 9, 2);
        else out << p(kDim) << "        -" << p(kReset);
        out << "\n";
    }

    return out.str();
}

}  // namespace svj
