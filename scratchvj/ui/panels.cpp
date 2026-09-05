#include "panels.h"

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>

#include "imgui.h"

namespace svj::ui {

Fonts g_fonts;

namespace {

// The mockup's palette. Named by role rather than by hue, so a retune changes one
// table instead of every call site.
const ImU32 kGround = IM_COL32(0x14, 0x14, 0x12, 0xFF);
const ImU32 kPanel = IM_COL32(0x1A, 0x19, 0x17, 0xFF);
const ImU32 kWell = IM_COL32(0x0E, 0x0E, 0x0C, 0xFF);
const ImU32 kHair = IM_COL32(0x2E, 0x2D, 0x28, 0xFF);
const ImU32 kInk = IM_COL32(0xE9, 0xE6, 0xDF, 0xFF);
const ImU32 kMuted = IM_COL32(0x8A, 0x86, 0x7C, 0xFF);
const ImU32 kFaint = IM_COL32(0x60, 0x5D, 0x56, 0xFF);
const ImU32 kAccent = IM_COL32(0xC9, 0x76, 0x2F, 0xFF);
const ImU32 kSage = IM_COL32(0x7E, 0x94, 0x6B, 0xFF);
const ImU32 kAmber = IM_COL32(0xC9, 0x9A, 0x2F, 0xFF);
const ImU32 kAlert = IM_COL32(0xB5, 0x4B, 0x3A, 0xFF);
const ImU32 kSlate = IM_COL32(0x6E, 0x86, 0x96, 0xFF);

ImVec4 rgba(ImU32 c) { return ImGui::ColorConvertU32ToFloat4(c); }

void push_mono() { if (g_fonts.mono != nullptr) ImGui::PushFont(g_fonts.mono, 0.0f); }
void push_small() { if (g_fonts.small != nullptr) ImGui::PushFont(g_fonts.small, 0.0f); }
void pop_font() { if (g_fonts.mono != nullptr) ImGui::PopFont(); }

void text_c(ImU32 colour, const char* fmt, ...) IM_FMTARGS(2);
void text_c(ImU32 colour, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    ImGui::PushStyleColor(ImGuiCol_Text, rgba(colour));
    ImGui::TextV(fmt, args);
    ImGui::PopStyleColor();
    va_end(args);
}

// The mockup's one recurring typographic device: small, letterspaced, quiet.
// ImGui has no letter-spacing, so the small face and the faint colour carry it.
void eyebrow(const char* text) {
    push_small();
    ImGui::PushStyleColor(ImGuiCol_Text, rgba(kFaint));
    ImGui::TextUnformatted(text);
    ImGui::PopStyleColor();
    pop_font();
}

void dim(const char* text) {
    ImGui::PushStyleColor(ImGuiCol_Text, rgba(kFaint));
    ImGui::TextUnformatted(text);
    ImGui::PopStyleColor();
}

std::string clock_of(double seconds) {
    const bool negative = seconds < 0.0;
    const double s = std::fabs(seconds);
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%s%02d:%04.1f", negative ? "-" : "",
                  static_cast<int>(s / 60.0), std::fmod(s, 60.0));
    return buffer;
}

std::string short_clock(double seconds) {
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%02d:%02d", static_cast<int>(seconds / 60.0),
                  static_cast<int>(std::fmod(seconds, 60.0)));
    return buffer;
}

const char* link_text(LinkState link) {
    switch (link) {
        case LinkState::Ok: return "OK";
        case LinkState::Degraded: return "FAIBLE";
        case LinkState::Lost: return "PERDUE";
    }
    return "?";
}

ImU32 link_colour(LinkState link) {
    switch (link) {
        case LinkState::Ok: return kSage;
        case LinkState::Degraded: return kAmber;
        case LinkState::Lost: return kAlert;
    }
    return kFaint;
}

// One value with its unit, the readout block the mockup repeats under each deck.
void readout(const char* name, const std::string& value, const char* unit, ImU32 colour) {
    ImGui::BeginGroup();
    eyebrow(name);
    push_mono();
    text_c(colour, "%s", value.c_str());
    pop_font();
    if (unit != nullptr) {
        ImGui::SameLine(0.0f, 4.0f);
        push_small();
        dim(unit);
        pop_font();
    }
    ImGui::EndGroup();
}

// A horizontal meter. `ghost` draws the track dashed instead of filled, which is
// how an absolute knob whose real position is unknown has to be shown: inventing
// a value would be worse than admitting there is none.
void meter(float value01, float width, ImU32 colour, bool ghost, float height = 5.0f) {
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const float line = ImGui::GetTextLineHeight();
    const float mid = origin.y + line * 0.5f;

    draw->AddRectFilled(ImVec2(origin.x, mid - height * 0.5f),
                        ImVec2(origin.x + width, mid + height * 0.5f), kHair);
    if (ghost) {
        for (float x = origin.x; x < origin.x + width; x += 7.0f) {
            draw->AddRectFilled(ImVec2(x, mid - 1.0f), ImVec2(std::min(x + 3.0f, origin.x + width),
                                                              mid + 1.0f),
                                kFaint);
        }
    } else {
        const float filled = width * std::clamp(value01, 0.0f, 1.0f);
        draw->AddRectFilled(ImVec2(origin.x, mid - height * 0.5f),
                            ImVec2(origin.x + filled, mid + height * 0.5f), colour);
    }
    ImGui::Dummy(ImVec2(width, line));
}

// A vertical knob track, for the mixer strip at the bottom.
void knob_strip(const Control& control, float height) {
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const float width = 9.0f;
    draw->AddRectFilled(origin, ImVec2(origin.x + width, origin.y + height), kWell);

    if (control.known) {
        const float filled = height * std::clamp(control.value, 0.0f, 1.0f);
        draw->AddRectFilled(ImVec2(origin.x, origin.y + height - filled),
                            ImVec2(origin.x + width, origin.y + height), kAccent);
    } else {
        for (float y = origin.y; y < origin.y + height; y += 7.0f) {
            draw->AddRectFilled(ImVec2(origin.x + 3.0f, y),
                                ImVec2(origin.x + width - 3.0f, std::min(y + 3.0f,
                                                                         origin.y + height)),
                                kFaint);
        }
    }
    ImGui::Dummy(ImVec2(width, height));
}

// Where the picture goes. There is no decoded frame to draw until the FFmpeg
// analysis pass exists, so this is an empty well that says what it is waiting
// for. Drawing a plausible still here would be the one lie this interface must
// not tell -- a performer has to be able to trust that what is on screen is what
// the engine actually has.
void picture_well(const Deck& deck, float width, float height) {
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const ImVec2 corner(origin.x + width, origin.y + height);

    draw->AddRectFilled(origin, corner, kWell);
    draw->AddRect(origin, corner, kHair);

    push_small();
    const char* waiting = "aucune image — passe d'analyse non écrite";
    const ImVec2 size = ImGui::CalcTextSize(waiting);
    draw->AddText(ImVec2(origin.x + (width - size.x) * 0.5f,
                         origin.y + height * 0.5f - size.y),
                  kFaint, waiting);

    char detail[96];
    std::snprintf(detail, sizeof(detail), "%ux%u  %s", deck.clip.width, deck.clip.height,
                  deck.clip.width == deck.clip.height * 2 ? "equirect 360" : "plan");
    const ImVec2 detail_size = ImGui::CalcTextSize(detail);
    draw->AddText(ImVec2(origin.x + (width - detail_size.x) * 0.5f,
                         origin.y + height * 0.5f + 4.0f),
                  kHair, detail);
    pop_font();

    ImGui::Dummy(ImVec2(width, height));
}

// The clip end to end: the loop, the hot cues, the part resident in video memory,
// and the playhead.
//
// The VRAM bracket is why this is drawn rather than written out. It is the part
// of the clip that is instantly scratchable, so it says how far the platter can
// be thrown before it hits a load -- a question no number answers as fast as a
// mark in the right place.
void filmstrip(const Deck& deck, float width) {
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const float height = 34.0f;
    const std::uint32_t frames = deck.clip.frame_count;

    const auto x_of = [&](std::uint32_t frame) {
        const float ratio = frames <= 1 ? 0.0f
                                        : static_cast<float>(frame) /
                                              static_cast<float>(frames - 1);
        return origin.x + width * std::clamp(ratio, 0.0f, 1.0f);
    };

    draw->AddRectFilled(origin, ImVec2(origin.x + width, origin.y + height), kWell);

    // Sparse tick marks, so the strip reads as a timeline rather than a bar.
    for (int i = 1; i < 8; ++i) {
        const float x = origin.x + width * static_cast<float>(i) / 8.0f;
        draw->AddLine(ImVec2(x, origin.y + height - 5.0f), ImVec2(x, origin.y + height), kHair);
    }

    const FrameRange& resident = deck.window.resident();
    if (!resident.empty()) {
        const float x0 = x_of(resident.first);
        const float x1 = std::max(x_of(resident.last()), x0 + 2.0f);
        draw->AddRectFilled(ImVec2(x0, origin.y), ImVec2(x1, origin.y + height),
                            IM_COL32(0x6E, 0x86, 0x96, 0x28));
        draw->AddLine(ImVec2(x0, origin.y), ImVec2(x0, origin.y + height), kSlate, 1.5f);
        draw->AddLine(ImVec2(x1, origin.y), ImVec2(x1, origin.y + height), kSlate, 1.5f);
    }

    if (deck.transport.loop().active) {
        const float x0 = x_of(deck.clip.frame_at(deck.transport.loop().start_s));
        const float x1 = x_of(deck.clip.frame_at(deck.transport.loop().end_s));
        draw->AddRectFilled(ImVec2(x0, origin.y), ImVec2(x1, origin.y + height),
                            IM_COL32(0xC9, 0x76, 0x2F, 0x33));
    }

    for (int i = 0; i < kHotCueCount; ++i) {
        const HotCue& cue = deck.transport.cue(i);
        if (!cue.set) continue;
        const float x = x_of(deck.clip.frame_at(cue.position_s));
        draw->AddRectFilled(ImVec2(x - 1.0f, origin.y + height - 9.0f),
                            ImVec2(x + 1.0f, origin.y + height), kAccent);
    }

    const float head = x_of(deck.clip.frame_at(deck.played.position_s));
    draw->AddLine(ImVec2(head, origin.y), ImVec2(head, origin.y + height), kInk, 2.0f);

    draw->AddRect(origin, ImVec2(origin.x + width, origin.y + height), kHair);
    ImGui::Dummy(ImVec2(width, height));
}

// ---------------------------------------------------------------------------

void draw_status(Engine& engine, const Frame& frame) {
    ImGui::BeginChild("status", ImVec2(0.0f, 46.0f), ImGuiChildFlags_None,
                      ImGuiWindowFlags_NoScrollbar);
    ImGui::Dummy(ImVec2(0.0f, 2.0f));
    text_c(kAccent, "scratchvj");

    ImGui::SameLine(0.0f, 22.0f);
    push_small();
    text_c(frame.follower_mode ? kInk : kFaint, "Serato");
    ImGui::SameLine(0.0f, 8.0f);
    text_c(frame.follower_mode ? kFaint : kInk, "Autonome");
    pop_font();

    ImGui::SameLine(0.0f, 26.0f);
    eyebrow("Liaison Phase");
    ImGui::SameLine(0.0f, 8.0f);
    push_mono();
    const TimecodeState& a = engine.deck_a().timecode.state();
    const TimecodeState& b = engine.deck_b().timecode.state();
    text_c(link_colour(a.link), "A %3.0f%%", static_cast<double>(a.confidence) * 100.0);
    ImGui::SameLine(0.0f, 6.0f);
    dim("\xC2\xB7");
    ImGui::SameLine(0.0f, 6.0f);
    text_c(link_colour(b.link), "B %3.0f%%", static_cast<double>(b.confidence) * 100.0);
    pop_font();

    ImGui::SameLine(0.0f, 26.0f);
    eyebrow("VRAM");
    ImGui::SameLine(0.0f, 8.0f);
    push_mono();
    const double vram_gb =
        static_cast<double>(engine.deck_a().window.capacity()) *
        static_cast<double>(block_bytes_per_frame(engine.deck_a().clip.width,
                                                  engine.deck_a().clip.height,
                                                  engine.deck_a().clip.format)) /
        (1024.0 * 1024.0 * 1024.0);
    text_c(kInk, "%.2f Go", vram_gb);
    pop_font();

    ImGui::SameLine(0.0f, 26.0f);
    eyebrow("BPM");
    ImGui::SameLine(0.0f, 8.0f);
    push_mono();
    text_c(kInk, "%.1f", engine.bpm());
    pop_font();

    ImGui::SameLine(0.0f, 26.0f);
    push_small();
    // Off, and honestly so: neither output exists yet.
    dim("Spout \xE2\x80\x94 NDI \xE2\x80\x94 hors service");
    pop_font();

    const float right = ImGui::GetContentRegionMax().x - 160.0f;
    if (right > ImGui::GetCursorPosX()) ImGui::SameLine(right);
    push_small();
    dim("\xC3\x89"
        "chap pour quitter");
    pop_font();

    ImGui::EndChild();
}

void draw_library(Engine& engine, float width, float height) {
    ImGui::BeginChild("library", ImVec2(width, height), ImGuiChildFlags_Borders);
    eyebrow("BIBLIOTH\xC3\x88QUE");
    ImGui::Dummy(ImVec2(0.0f, 4.0f));

    const Library& library = engine.library();
    for (int crate = 0; crate < library.crate_count(); ++crate) {
        push_small();
        ImGui::PushStyleColor(ImGuiCol_Text, rgba(crate == 0 ? kInk : kMuted));
        ImGui::Text("%s", library.crate_name(crate).c_str());
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, rgba(kFaint));
        ImGui::Text("%d", static_cast<int>(library.crate_clips(crate).size()));
        ImGui::PopStyleColor();
        pop_font();
    }

    ImGui::Dummy(ImVec2(0.0f, 10.0f));
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0.0f, 6.0f));

    for (std::size_t i = 0; i < library.size(); ++i) {
        const ClipEntry& clip = library.at(static_cast<ClipId>(i));
        // An unanalysed clip is drawn as not loadable. That is the point: loading
        // one would mean real-time decoding, which is the thing this whole design
        // refuses to do, so it must be visibly unavailable before it is clicked.
        text_c(clip.playable() ? kInk : kFaint, "%s", clip.name.c_str());
        push_small();
        ImGui::PushStyleColor(ImGuiCol_Text, rgba(kFaint));
        if (clip.state == AnalysisState::Analysing) {
            ImGui::Text("  analyse %.0f%%", static_cast<double>(clip.progress) * 100.0);
        } else {
            ImGui::Text("  %s  %s", short_clock(clip.duration_s).c_str(),
                        clip.equirect ? "360\xC2\xB0" : "plan");
        }
        ImGui::PopStyleColor();
        pop_font();
    }

    ImGui::Dummy(ImVec2(0.0f, 10.0f));
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0.0f, 6.0f));
    eyebrow("FILE D'ATTENTE");
    ImGui::Dummy(ImVec2(0.0f, 4.0f));

    for (std::size_t i = 0; i < engine.queue().size(); ++i) {
        const QueueItem& item = engine.queue().at(i);
        const ClipEntry& clip = library.at(item.clip);
        push_mono();
        text_c(kFaint, "%d", static_cast<int>(i + 1));
        pop_font();
        ImGui::SameLine(0.0f, 8.0f);
        text_c(kInk, "%s", clip.name.c_str());
        if (item.target != DeckTarget::None) {
            push_small();
            ImGui::PushStyleColor(ImGuiCol_Text, rgba(kAccent));
            ImGui::Text("  \xE2\x86\x92 Deck %s", item.target == DeckTarget::A ? "A" : "B");
            ImGui::PopStyleColor();
            pop_font();
        }
    }

    ImGui::EndChild();
}

void draw_deck(Deck& deck, const Engine& engine, const Frame& frame, ImU32 accent, bool is_a,
               float width, float height) {
    ImGui::PushID(is_a ? "deck.a" : "deck.b");
    ImGui::BeginChild(is_a ? "deckA" : "deckB", ImVec2(width, height), ImGuiChildFlags_Borders);

    const float inner = ImGui::GetContentRegionAvail().x;

    ImGui::PushStyleColor(ImGuiCol_Text, rgba(accent));
    ImGui::Text("%s", is_a ? "A" : "B");
    ImGui::PopStyleColor();
    ImGui::SameLine(0.0f, 12.0f);
    text_c(kInk, "%s", deck.name.c_str());
    ImGui::SameLine(0.0f, 12.0f);
    push_small();
    ImGui::PushStyleColor(ImGuiCol_Text, rgba(kFaint));
    ImGui::Text("%s   %s", short_clock(deck.clip.duration_s()).c_str(),
                deck.clip.width == deck.clip.height * 2 ? "360\xC2\xB0" : "plan");
    ImGui::PopStyleColor();
    pop_font();

    ImGui::Dummy(ImVec2(0.0f, 6.0f));
    picture_well(deck, inner, std::max(120.0f, height * 0.32f));

    ImGui::Dummy(ImVec2(0.0f, 8.0f));
    push_small();
    ImGui::PushStyleColor(ImGuiCol_Text, rgba(kSlate));
    ImGui::Text("Fen\xC3\xAAtre VRAM \xC2\xB7 %.1f s",
                deck.window.window_seconds(deck.clip.frame_duration_s()));
    ImGui::PopStyleColor();
    pop_font();
    filmstrip(deck, inner);

    ImGui::Dummy(ImVec2(0.0f, 8.0f));
    const TimecodeState& state = deck.timecode.state();
    readout("POSITION", clock_of(deck.played.position_s), nullptr, kInk);
    ImGui::SameLine(0.0f, 26.0f);
    {
        char v[16];
        std::snprintf(v, sizeof(v), "%.2f", deck.played.velocity);
        readout("VITESSE", v, "\xC3\x97",
                std::fabs(deck.played.velocity) > 2.5 ? kAmber : kInk);
    }
    ImGui::SameLine(0.0f, 26.0f);
    {
        char v[16];
        std::snprintf(v, sizeof(v), "%.0f", static_cast<double>(state.confidence) * 100.0);
        readout("CONFIANCE", v, "%", link_colour(state.link));
    }
    ImGui::SameLine(0.0f, 26.0f);
    {
        char v[16];
        std::snprintf(v, sizeof(v), "%.1f", static_cast<double>(deck.gestures.scratch_rate()));
        readout("SCRATCH", v, "/s", kInk);
    }
    ImGui::SameLine(0.0f, 26.0f);
    {
        ImGui::BeginGroup();
        eyebrow("LIAISON");
        text_c(link_colour(state.link), "%s", link_text(state.link));
        ImGui::EndGroup();
    }

    ImGui::Dummy(ImVec2(0.0f, 10.0f));
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0.0f, 6.0f));

    // The transport source, which is what core/playback added. A deck that does
    // not follow a platter is not a broken deck, it is a different instrument
    // setting, so it is a first-class control and not a hidden preference.
    eyebrow("TRANSPORT");
    const DeckSource source = deck.clock.source();
    const struct { const char* label; DeckSource value; } sources[] = {
        {"Plateau", DeckSource::Timecode},
        {"Libre", DeckSource::FreeRun},
        {"Tempo", DeckSource::TempoLocked},
    };
    for (const auto& option : sources) {
        ImGui::SameLine(0.0f, 10.0f);
        if (ImGui::SmallButton(option.label)) {
            if (option.value == DeckSource::Timecode) {
                deck.clock.hand_over_to_timecode(state.position_s, frame.elapsed_s);
            } else {
                deck.clock.set_source(option.value, frame.elapsed_s);
            }
        }
        if (source == option.value) {
            ImDrawList* draw = ImGui::GetWindowDrawList();
            const ImVec2 lo = ImGui::GetItemRectMin();
            const ImVec2 hi = ImGui::GetItemRectMax();
            draw->AddLine(ImVec2(lo.x, hi.y), ImVec2(hi.x, hi.y), kAccent, 2.0f);
        }
    }

    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    eyebrow("LECTURE");
    const ClipPlayMode mode = deck.clock.mode();
    const struct { const char* label; ClipPlayMode value; } modes[] = {
        {"Boucle", ClipPlayMode::Loop},
        {"Aller-retour", ClipPlayMode::PingPong},
        {"Une fois", ClipPlayMode::Once},
    };
    for (const auto& option : modes) {
        ImGui::SameLine(0.0f, 10.0f);
        if (ImGui::SmallButton(option.label)) deck.clock.set_mode(option.value);
        if (mode == option.value) {
            ImDrawList* draw = ImGui::GetWindowDrawList();
            const ImVec2 lo = ImGui::GetItemRectMin();
            const ImVec2 hi = ImGui::GetItemRectMax();
            draw->AddLine(ImVec2(lo.x, hi.y), ImVec2(hi.x, hi.y), kAccent, 2.0f);
        }
    }
    if (deck.played.reversed) {
        ImGui::SameLine(0.0f, 12.0f);
        text_c(kAccent, "<< retour");
    }

    ImGui::Dummy(ImVec2(0.0f, 8.0f));
    eyebrow("BOUCLE / SLIP / CUES");
    push_mono();
    text_c(deck.transport.loop().active ? kAccent : kFaint,
           deck.transport.loop().active ? "boucle %.2f s" : "boucle  \xE2\x80\x94",
           deck.transport.loop().length_s());
    ImGui::SameLine(0.0f, 18.0f);
    text_c(deck.transport.slip() ? kAccent : kFaint, deck.transport.slip() ? "SLIP" : "slip");
    pop_font();
    ImGui::SameLine(0.0f, 18.0f);
    for (int i = 0; i < kHotCueCount; ++i) {
        if (i > 0) ImGui::SameLine(0.0f, 4.0f);
        ImDrawList* draw = ImGui::GetWindowDrawList();
        const ImVec2 at = ImGui::GetCursorScreenPos();
        draw->AddRectFilled(ImVec2(at.x, at.y + 4.0f), ImVec2(at.x + 12.0f, at.y + 14.0f),
                            deck.transport.cue(i).set ? kAccent : kHair);
        ImGui::Dummy(ImVec2(12.0f, 16.0f));
    }

    // Deck A carries the anchor, because it is the one lined up against Serato.
    if (is_a) {
        ImGui::Dummy(ImVec2(0.0f, 8.0f));
        eyebrow("ANCRAGE");
        const int jumps = deck.timecode.jump_count();
        const float stale = engine.anchor().armed()
                                ? engine.anchor().staleness(frame.elapsed_s, jumps)
                                : 1.0f;
        // Freshness, never drift. What is observable is how long the anchor has
        // been down and how many timecode discontinuities went past since; a
        // number of seconds of error would be invented, so none is shown.
        meter(1.0f - stale, 130.0f, stale > 0.6f ? kAmber : kSage, false);
        ImGui::SameLine(0.0f, 10.0f);
        push_small();
        ImGui::PushStyleColor(ImGuiCol_Text, rgba(kFaint));
        // The string is split after the escape on purpose: \xAE followed by a
        // hex digit is read as one escape, and "cheur" starts with one.
        ImGui::Text("fra\xC3\xAE"
                    "cheur \xC2\xB7 %d saut%s",
                    jumps, jumps == 1 ? "" : "s");
        ImGui::PopStyleColor();
        pop_font();
    }

    ImGui::EndChild();
    ImGui::PopID();
}

void draw_mix(Engine& engine, float width, float height) {
    ImGui::BeginChild("mix", ImVec2(width, height), ImGuiChildFlags_Borders);
    eyebrow("PROGRAM");
    ImGui::Dummy(ImVec2(0.0f, 6.0f));

    const StackWeights stack = engine.stack();
    push_mono();
    text_c(kAmber, "A");
    pop_font();
    ImGui::SameLine(0.0f, 8.0f);
    meter(stack.a, 130.0f, kAmber, false, 7.0f);
    ImGui::SameLine(0.0f, 14.0f);
    push_mono();
    text_c(kSlate, "B");
    pop_font();
    ImGui::SameLine(0.0f, 8.0f);
    meter(stack.b, 130.0f, kSlate, false, 7.0f);
    ImGui::SameLine(0.0f, 18.0f);
    push_small();
    ImGui::PushStyleColor(ImGuiCol_Text,
                          rgba(engine.cuts().transforming() ? kAmber : kFaint));
    ImGui::Text("coupes %.1f/s%s", static_cast<double>(engine.cuts().cuts_per_second()),
                engine.cuts().transforming() ? "   TRANSFORM" : "");
    ImGui::PopStyleColor();
    pop_font();

    ImGui::Dummy(ImVec2(0.0f, 6.0f));
    // On its own row: the overlay does not answer to the crossfader, and sharing
    // the row above would say that it did.
    Layer& overlay = engine.overlay_layer();
    ImGui::Checkbox("Incrustation", &overlay.enabled);
    ImGui::SameLine(0.0f, 12.0f);
    ImGui::SetNextItemWidth(130.0f);
    ImGui::SliderFloat("##overlay.opacity", &overlay.opacity, 0.0f, 1.0f, "%.2f");
    ImGui::SameLine(0.0f, 12.0f);
    push_small();
    ImGui::PushStyleColor(ImGuiCol_Text, rgba(kFaint));
    ImGui::Text("%s %s  hors crossfader",
                clock_of(engine.overlay().played.position_s).c_str(),
                engine.overlay().played.reversed ? "<<" : ">>");
    ImGui::PopStyleColor();
    pop_font();

    ImGui::Dummy(ImVec2(0.0f, 10.0f));
    eyebrow("EFFETS \xE2\x80\x94 audio et vid\xC3\xA9o sur le m\xC3\xAAme bouton");
    ImGui::Dummy(ImVec2(0.0f, 4.0f));

    for (std::size_t i = 0; i < engine.rack().size(); ++i) {
        const EffectUnit& unit = engine.rack().at(i);
        const EffectDescriptor* info = describe(unit.type);
        if (info == nullptr) continue;

        text_c(kInk, "%-11s", info->id);
        ImGui::SameLine(0.0f, 10.0f);
        // Only the UNLINKED state is signalled. Linked is the norm and the whole
        // idea of the rack; marking it everywhere would be noise, and unlinked is
        // the one state where the two domains say different things.
        push_small();
        if (unit.link) {
            dim("li\xC3\xA9");
        } else {
            text_c(kAlert, "d\xC3\xA9li\xC3\xA9");
        }
        pop_font();
        ImGui::SameLine(0.0f, 12.0f);
        meter(unit.audio_params().mix, 70.0f, kMuted, false);
        ImGui::SameLine(0.0f, 8.0f);
        meter(unit.video_params().mix, 70.0f, kAccent, false);
        ImGui::SameLine(0.0f, 10.0f);
        push_small();
        ImGui::PushStyleColor(ImGuiCol_Text, rgba(kFaint));
        ImGui::Text("%s", info->video != nullptr ? info->video : "");
        ImGui::PopStyleColor();
        pop_font();
    }

    ImGui::EndChild();
}

void draw_surface(Engine& engine, float width, float height) {
    ImGui::BeginChild("surface", ImVec2(width, height), ImGuiChildFlags_Borders);
    eyebrow("SURFACE \xE2\x80\x94 Reloop Elite \xC2\xB7 RP-8000 MK2");
    push_small();
    ImGui::PushStyleColor(ImGuiCol_Text, rgba(kFaint));
    ImGui::TextUnformatted(
        "En pointill\xC3\xA9 : contr\xC3\xB4les non touch\xC3\xA9s depuis le lancement, "
        "position r\xC3\xA9""elle inconnue.");
    ImGui::PopStyleColor();
    pop_font();
    ImGui::Dummy(ImVec2(0.0f, 8.0f));

    const Surface& surface = engine.surface();
    const float column = 76.0f;
    for (std::size_t i = 0; i < surface.size(); ++i) {
        const Control& control = surface.at(static_cast<ControlIndex>(i));
        if (i > 0) ImGui::SameLine(0.0f, 0.0f);

        ImGui::BeginGroup();
        ImGui::Dummy(ImVec2(column, 0.0f));
        ImGui::SameLine(0.0f, -column);
        knob_strip(control, 70.0f);

        push_small();
        ImGui::PushStyleColor(ImGuiCol_Text, rgba(control.known ? kMuted : kFaint));
        ImGui::TextUnformatted(control.id.c_str());
        ImGui::PopStyleColor();
        // A ghost shows no number at all: the Elite's pots are absolute, so their
        // real position is unknown until touched, and printing 0.00 would be a
        // fabrication a performer would read as a measurement.
        ImGui::PushStyleColor(ImGuiCol_Text, rgba(control.known ? kInk : kFaint));
        if (control.known) {
            ImGui::Text("%.2f", static_cast<double>(control.value));
        } else {
            ImGui::TextUnformatted("?");
        }
        ImGui::PopStyleColor();
        pop_font();
        ImGui::EndGroup();
    }

    ImGui::EndChild();
}

}  // namespace

void apply_style() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.ChildRounding = 0.0f;
    style.FrameRounding = 1.0f;
    style.GrabRounding = 1.0f;
    style.ScrollbarRounding = 0.0f;
    style.WindowBorderSize = 0.0f;
    style.ChildBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.CellPadding = ImVec2(8.0f, 4.0f);
    style.ItemSpacing = ImVec2(10.0f, 6.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
    style.WindowPadding = ImVec2(18.0f, 16.0f);
    style.FramePadding = ImVec2(8.0f, 4.0f);

    ImVec4* c = style.Colors;
    c[ImGuiCol_WindowBg] = rgba(kGround);
    c[ImGuiCol_ChildBg] = rgba(kPanel);
    c[ImGuiCol_PopupBg] = rgba(kPanel);
    c[ImGuiCol_Text] = rgba(kInk);
    c[ImGuiCol_TextDisabled] = rgba(kFaint);
    c[ImGuiCol_Border] = rgba(kHair);
    c[ImGuiCol_Separator] = rgba(kHair);
    c[ImGuiCol_FrameBg] = rgba(kWell);
    c[ImGuiCol_FrameBgHovered] = IM_COL32_BLACK_TRANS ? rgba(kHair) : rgba(kHair);
    c[ImGuiCol_FrameBgActive] = rgba(kHair);
    c[ImGuiCol_Button] = rgba(kWell);
    c[ImGuiCol_ButtonHovered] = rgba(kHair);
    c[ImGuiCol_ButtonActive] = rgba(kAccent);
    c[ImGuiCol_CheckMark] = rgba(kAccent);
    c[ImGuiCol_SliderGrab] = rgba(kAccent);
    c[ImGuiCol_SliderGrabActive] = rgba(kInk);
    c[ImGuiCol_Header] = rgba(kHair);
    c[ImGuiCol_ScrollbarBg] = rgba(kWell);
    c[ImGuiCol_ScrollbarGrab] = rgba(kHair);
}

void draw(Engine& engine, const Frame& frame) {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::Begin("scratchvj", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus);

    draw_status(engine, frame);

    // The mockup's proportions: a fixed library rail, then the decks, then the
    // surface across the bottom. Fixed because a browser that reflows while a set
    // is running is a browser nobody can find anything in.
    const float rail = 250.0f;
    const float gap = ImGui::GetStyle().ItemSpacing.x;
    const float surface_h = 150.0f;
    const float body_h = std::max(320.0f, ImGui::GetContentRegionAvail().y - surface_h - gap);
    const float decks_w = std::max(520.0f, ImGui::GetContentRegionAvail().x - rail - gap);
    const float deck_w = (decks_w - gap) * 0.5f;
    const float deck_h = body_h * 0.66f;

    draw_library(engine, rail, body_h);
    ImGui::SameLine();

    ImGui::BeginGroup();
    draw_deck(engine.deck_a(), engine, frame, kAmber, true, deck_w, deck_h);
    ImGui::SameLine();
    draw_deck(engine.deck_b(), engine, frame, kSlate, false, deck_w, deck_h);
    draw_mix(engine, decks_w, body_h - deck_h - ImGui::GetStyle().ItemSpacing.y);
    ImGui::EndGroup();

    draw_surface(engine, 0.0f, surface_h);

    ImGui::End();
}

}  // namespace svj::ui
