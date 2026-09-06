#include "panels.h"

#include "config/warp_io.h"

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

// Where the picture goes. With an analysed clip loaded this is the real frame
// at the deck's played position; without one it is an empty well that says what
// it is waiting for. Drawing a plausible still would be the one lie this
// interface must not tell -- a performer has to be able to trust that what is
// on screen is what the engine actually has.
void picture_well(const Deck& deck, void* texture, float aspect_override, float width,
                  float height) {
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const ImVec2 corner(origin.x + width, origin.y + height);

    draw->AddRectFilled(origin, corner, kWell);

    if (texture != nullptr && deck.clip.width > 0 && deck.clip.height > 0) {
        // Fit inside the well, letterboxed; stretching would misstate the frame.
        // A reprojected 360 deck shows the VIEW's aspect, not the equirect's.
        const float aspect = aspect_override > 0.0f
                                 ? aspect_override
                                 : static_cast<float>(deck.clip.width) /
                                       static_cast<float>(deck.clip.height);
        float dw = width;
        float dh = dw / aspect;
        if (dh > height) {
            dh = height;
            dw = dh * aspect;
        }
        const ImVec2 lo(origin.x + (width - dw) * 0.5f, origin.y + (height - dh) * 0.5f);
        draw->AddImage(ImTextureRef(reinterpret_cast<ImTextureID>(texture)), lo,
                       ImVec2(lo.x + dw, lo.y + dh));
        draw->AddRect(origin, corner, kHair);
        ImGui::Dummy(ImVec2(width, height));
        return;
    }

    draw->AddRect(origin, corner, kHair);

    push_small();
    const char* waiting = "aucune image — lancer: scratchvj analyze <video>";
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
// The clip end to end -- loop, hot cues, the resident VRAM window, the playhead
// -- and a HANDLE on it: press and the hand takes the clip, drag and it follows.
//
// Scrubbing is not a new mechanism. A hand holding the clip is the fourth kind
// of position source (core/playback's DeckSource::Hand), and letting go returns
// the deck to its platter through the same Grab takeover a real hand landing on
// a moving record uses -- so the picture never jumps on release. The alternative
// (a UI-only "preview position" beside the real one) would have put two truths
// on screen and made the release a lie.
// Where the mouse is pointing, in clip seconds, clamped to the clip.
double position_under(const Deck& deck, float origin_x, float width) {
    const float ratio = std::clamp((ImGui::GetIO().MousePos.x - origin_x) / width, 0.0f,
                                   1.0f);
    return static_cast<double>(ratio) * deck.clip.duration_s();
}

void filmstrip(Deck& deck, Frame& frame, float width, float height = 34.0f) {
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetCursorScreenPos();
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

    // The handle. An InvisibleButton over the strip we have just drawn, so the
    // drawing stays declarative and only the interaction is stateful.
    ImGui::SetCursorScreenPos(origin);
    ImGui::PushID(&deck);
    ImGui::InvisibleButton("scrub", ImVec2(width, height));
    const bool hovered = ImGui::IsItemHovered();
    if (hovered) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

    if (ImGui::IsItemActivated()) {
        std::fprintf(stderr, "GRAB down=%d nav=%d\n",
                     static_cast<int>(ImGui::IsMouseDown(ImGuiMouseButton_Left)),
                     static_cast<int>(ImGui::GetIO().NavActive));
        deck.clock.grab(position_under(deck, origin.x, width), frame.elapsed_s);
    } else if (ImGui::IsItemActive()) {
        deck.clock.scrub(position_under(deck, origin.x, width), frame.elapsed_s);
    } else if (ImGui::IsItemDeactivated()) {
        // Back to the platter, offset so this exact frame is the one that stays.
        deck.clock.hand_over_to_timecode(deck.timecode.state().position_s,
                                         frame.elapsed_s);
    }
    ImGui::PopID();

    // While a hand holds it, say so on the strip itself rather than only in the
    // transport row: the eye is here, not there.
    if (deck.clock.source() == DeckSource::Hand) {
        draw->AddRect(origin, ImVec2(origin.x + width, origin.y + height), kAccent, 0.0f,
                      0, 2.0f);
        push_small();
        draw->AddText(ImVec2(origin.x + 6.0f, origin.y + 4.0f), kAccent, "MAIN");
        pop_font();
    } else if (hovered) {
        draw->AddRect(origin, ImVec2(origin.x + width, origin.y + height), kMuted);
    }

    // The scale under the strip, as the mockup has it: start, playhead, end.
    if (g_fonts.mono != nullptr && g_fonts.small != nullptr) {
        ImGui::PushFont(g_fonts.mono, g_fonts.small->LegacySize);
    }
    dim("00:00");
    const std::string now = clock_of(deck.played.position_s);
    const float now_w = ImGui::CalcTextSize(now.c_str()).x;
    ImGui::SameLine(std::clamp(head - origin.x - now_w * 0.5f, 60.0f, width - 120.0f));
    text_c(kMuted, "%s", now.c_str());
    const std::string end = short_clock(deck.clip.duration_s());
    ImGui::SameLine(width - ImGui::CalcTextSize(end.c_str()).x);
    dim(end.c_str());
    if (g_fonts.mono != nullptr && g_fonts.small != nullptr) ImGui::PopFont();
}

// ---------------------------------------------------------------------------

void draw_status(Engine& engine, Frame& frame) {
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

    // Right-aligned by measurement, and dropped entirely when the bar is too
    // narrow -- overlapping the outputs indicator would be worse than absent.
    const char* quit = "\xC3\x89"
                       "chap pour quitter";
    const float quit_w = ImGui::CalcTextSize(quit).x;
    const float right_x = ImGui::GetContentRegionMax().x - quit_w;
    if (right_x > ImGui::GetCursorPosX() + 40.0f) {
        ImGui::SameLine(right_x);
        dim(quit);
    }
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

void draw_deck(Deck& deck, Engine& engine, Frame& frame, void* texture,
               ImU32 accent, bool is_a, float width, float height) {
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
    const bool projected_360 = is_a && deck.clip.width == deck.clip.height * 2;
    picture_well(deck, texture,
                 projected_360 ? static_cast<float>(engine.view_a().aspect) : 0.0f, inner,
                 std::max(120.0f, height * 0.23f));

    ImGui::Dummy(ImVec2(0.0f, 8.0f));
    push_small();
    ImGui::PushStyleColor(ImGuiCol_Text, rgba(kSlate));
    ImGui::Text("Fen\xC3\xAAtre VRAM \xC2\xB7 %.1f s",
                deck.window.window_seconds(deck.clip.frame_duration_s()));
    ImGui::PopStyleColor();
    pop_font();
    filmstrip(deck, frame, inner);

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

    if (is_a && deck.clip.width == deck.clip.height * 2) {
        ImGui::Dummy(ImVec2(0.0f, 4.0f));
        eyebrow("VUE 360");
        SphereView& gaze = engine.view_a();
        const struct { const char* label; Projection value; } projections[] = {
            {"Perspective", Projection::Perspective},
            {"Little planet", Projection::LittlePlanet},
            {"Fisheye", Projection::Fisheye},
        };
        for (const auto& option : projections) {
            ImGui::SameLine(0.0f, 10.0f);
            if (ImGui::SmallButton(option.label)) gaze.projection = option.value;
            if (gaze.projection == option.value) {
                ImDrawList* draw = ImGui::GetWindowDrawList();
                const ImVec2 lo = ImGui::GetItemRectMin();
                const ImVec2 hi = ImGui::GetItemRectMax();
                draw->AddLine(ImVec2(lo.x, hi.y), ImVec2(hi.x, hi.y), kAccent, 2.0f);
            }
        }
        ImGui::SameLine(0.0f, 14.0f);
        push_small();
        ImGui::PushStyleColor(ImGuiCol_Text, rgba(kFaint));
        ImGui::Text("yaw %+.0f\xC2\xB0  pitch %+.0f\xC2\xB0", gaze.yaw_deg, gaze.pitch_deg);
        ImGui::PopStyleColor();
        pop_font();
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

void draw_mix(Engine& engine, Frame& frame, float width, float height) {
    ImGui::BeginChild("mix", ImVec2(width, height), ImGuiChildFlags_Borders);
    eyebrow("PROGRAM");

    // The program preview, top-right of the panel: the exact pixels the Spout
    // output carries, so what the interface shows is what a receiver gets.
    if (frame.tex_program != nullptr && frame.program_height > 0) {
        const float ph = std::max(90.0f, ImGui::GetContentRegionAvail().y - 14.0f);
        const float pw = ph * static_cast<float>(frame.program_width) /
                         static_cast<float>(frame.program_height);
        const ImVec2 window = ImGui::GetWindowPos();
        const ImVec2 region_max = ImGui::GetWindowContentRegionMax();
        const ImVec2 region_min = ImGui::GetWindowContentRegionMin();
        const ImVec2 lo(window.x + region_max.x - pw, window.y + region_min.y);
        ImDrawList* draw = ImGui::GetWindowDrawList();
        draw->AddImage(ImTextureRef(reinterpret_cast<ImTextureID>(frame.tex_program)), lo,
                       ImVec2(lo.x + pw, lo.y + ph));
        draw->AddRect(lo, ImVec2(lo.x + pw, lo.y + ph), kHair);
        push_small();
        draw->AddText(ImVec2(lo.x + 8.0f, lo.y + 6.0f), kFaint, "SPOUT scratchvj");
        pop_font();
    }

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
    // the row above would say that it did. A chip rather than a checkbox -- the
    // mockup has no checkboxes, and one stock widget is enough to unravel the
    // whole look.
    Layer& overlay = engine.overlay_layer();
    if (ImGui::SmallButton("Incrustation")) overlay.enabled = !overlay.enabled;
    {
        ImDrawList* draw = ImGui::GetWindowDrawList();
        const ImVec2 lo = ImGui::GetItemRectMin();
        const ImVec2 hi = ImGui::GetItemRectMax();
        draw->AddLine(ImVec2(lo.x, hi.y), ImVec2(hi.x, hi.y),
                      overlay.enabled ? kAccent : kHair, 2.0f);
    }
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

// --- the mixer: the one place the mouse is a controller ----------------------
//
// Until MIDI arrives, the mouse IS the control surface, and it writes through
// the same door MIDI will: into the Surface, via the hand-ownership channel.
// The script animates a control only until a hand claims it -- exactly what
// happens on stage when a real fader arrives under a real finger.

// One control looked up by name; kNoControl tolerated everywhere below.
const Control* surface_control(const Surface& surface, const char* id,
                               ControlIndex* index_out) {
    const ControlIndex index = surface.find(id);
    *index_out = index;
    return index == kNoControl ? nullptr : &surface.at(index);
}

void claim(Frame& frame, ControlIndex index, float value) {
    if (frame.hand != nullptr && index != kNoControl) {
        frame.hand->take(index, std::clamp(value, 0.0f, 1.0f));
    }
}

// A dashed arc, for a rotary whose real position is unknown.
void dashed_arc(ImDrawList* draw, ImVec2 centre, float radius, float a0, float a1,
                ImU32 colour, float thickness) {
    const int segments = 14;
    for (int i = 0; i < segments; ++i) {
        const float t0 = a0 + (a1 - a0) * (i + 0.15f) / segments;
        const float t1 = a0 + (a1 - a0) * (i + 0.60f) / segments;
        draw->PathArcTo(centre, radius, t0, t1, 4);
        draw->PathStroke(colour, 0, thickness);
    }
}

// A rotary knob: 270-degree arc, needle, label and value. Returns true when the
// hand moved it this frame; drag is vertical, half a pixel per unit percent, so
// fine moves stay fine.
bool rotary(const char* id, const char* label, const Control* control,
            Frame& frame, ControlIndex index, ImU32 accent) {
    const float diameter = 46.0f;
    const float column = 64.0f;

    ImGui::BeginGroup();
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const ImVec2 centre(origin.x + column * 0.5f, origin.y + diameter * 0.5f + 2.0f);
    ImGui::InvisibleButton(id, ImVec2(column, diameter + 6.0f));

    bool changed = false;
    float value = control != nullptr ? control->value : 0.0f;
    const bool known = control != nullptr && control->known;
    const bool active = ImGui::IsItemActive();
    const bool hovered = ImGui::IsItemHovered();

    if (active && control != nullptr) {
        value = std::clamp(value - ImGui::GetIO().MouseDelta.y * 0.005f, 0.0f, 1.0f);
        claim(frame, index, value);
        changed = true;
        ImGui::SetTooltip("%.2f", static_cast<double>(value));
    }
    if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        claim(frame, index, 0.5f);
        changed = true;
    }

    ImDrawList* draw = ImGui::GetWindowDrawList();
    const float radius = diameter * 0.5f - 3.0f;
    // 270 degrees of throw, opening downwards like a hardware pot.
    const float a0 = 0.75f * 3.14159265f;
    const float a1 = 2.25f * 3.14159265f;

    if (known || active) {
        draw->PathArcTo(centre, radius, a0, a1, 32);
        draw->PathStroke(kHair, 0, 3.0f);
        const float av = a0 + (a1 - a0) * value;
        draw->PathArcTo(centre, radius, a0, av, 24);
        draw->PathStroke(accent, 0, 3.0f);
        const float needle = radius - 5.0f;
        draw->AddLine(centre,
                      ImVec2(centre.x + std::cos(av) * needle,
                             centre.y + std::sin(av) * needle),
                      hovered || active ? kInk : kMuted, 2.0f);
    } else {
        // Ghost: the pot exists, its position does not. Dashes, no needle, no
        // number -- same rule as everywhere else in this interface.
        dashed_arc(draw, centre, radius, a0, a1, kFaint, 2.0f);
    }

    push_small();
    const std::string caption = std::string(label);
    const float caption_w = ImGui::CalcTextSize(caption.c_str()).x;
    ImGui::SetCursorScreenPos(
        ImVec2(centre.x - caption_w * 0.5f, origin.y + diameter + 8.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, rgba(known ? kMuted : kFaint));
    ImGui::TextUnformatted(caption.c_str());
    ImGui::PopStyleColor();
    pop_font();

    push_mono();
    push_small();
    char value_text[16];
    if (known) {
        std::snprintf(value_text, sizeof(value_text), "%.2f", static_cast<double>(value));
    } else {
        std::snprintf(value_text, sizeof(value_text), "?");
    }
    const float value_w = ImGui::CalcTextSize(value_text).x;
    ImGui::SetCursorScreenPos(
        ImVec2(centre.x - value_w * 0.5f, ImGui::GetCursorScreenPos().y));
    text_c(known ? kInk : kFaint, "%s", value_text);
    pop_font();
    pop_font();
    ImGui::EndGroup();
    return changed;
}

// A vertical channel fader: track, cap, absolute drag.
bool vertical_fader(const char* id, const char* label, const Control* control,
                    Frame& frame, ControlIndex index, ImU32 accent) {
    const float height = 96.0f;
    const float column = 46.0f;

    ImGui::BeginGroup();
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton(id, ImVec2(column, height));

    float value = control != nullptr ? control->value : 0.0f;
    const bool known = control != nullptr && control->known;
    const bool active = ImGui::IsItemActive();
    const bool hovered = ImGui::IsItemHovered();
    bool changed = false;

    if (active && control != nullptr) {
        value = std::clamp(1.0f - (ImGui::GetIO().MousePos.y - origin.y) / height, 0.0f,
                           1.0f);
        claim(frame, index, value);
        changed = true;
        ImGui::SetTooltip("%.2f", static_cast<double>(value));
    }
    if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        claim(frame, index, 1.0f);  // a channel fader rests open
        changed = true;
    }

    ImDrawList* draw = ImGui::GetWindowDrawList();
    const float cx = origin.x + column * 0.5f;
    if (known || active) {
        draw->AddRectFilled(ImVec2(cx - 3.0f, origin.y), ImVec2(cx + 3.0f, origin.y + height),
                            kWell);
        draw->AddRectFilled(ImVec2(cx - 3.0f, origin.y + (1.0f - value) * height),
                            ImVec2(cx + 3.0f, origin.y + height), accent);
        // Side ticks every quarter, the glanceable scale.
        for (int i = 0; i <= 4; ++i) {
            const float y = origin.y + height * i / 4.0f;
            draw->AddLine(ImVec2(cx + 8.0f, y), ImVec2(cx + 13.0f, y), kHair);
        }
        const float cap_y = origin.y + (1.0f - value) * height;
        draw->AddRectFilled(ImVec2(cx - 12.0f, cap_y - 5.0f), ImVec2(cx + 12.0f, cap_y + 5.0f),
                            hovered || active ? kInk : kMuted);
        draw->AddLine(ImVec2(cx - 12.0f, cap_y), ImVec2(cx + 12.0f, cap_y), kGround, 1.0f);
    } else {
        for (float y = origin.y; y < origin.y + height; y += 8.0f) {
            draw->AddRectFilled(ImVec2(cx - 2.0f, y), ImVec2(cx + 2.0f, y + 4.0f), kFaint);
        }
    }

    push_small();
    const float label_w = ImGui::CalcTextSize(label).x;
    ImGui::SetCursorScreenPos(ImVec2(cx - label_w * 0.5f, origin.y + height + 8.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, rgba(known ? kMuted : kFaint));
    ImGui::TextUnformatted(label);
    ImGui::PopStyleColor();
    pop_font();
    ImGui::EndGroup();
    return changed;
}

// THE crossfader. Wide, horizontal, with the battle curve stated next to it:
// on SHARP a flick is a cut, and that is the whole reason this control exists.
bool crossfader(const Control* control, Frame& frame, ControlIndex index,
                const Engine& engine, float width) {
    const float height = 34.0f;

    ImGui::BeginGroup();
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("xfader", ImVec2(width, height));

    float value = control != nullptr ? control->value : 0.5f;
    const bool known = control != nullptr && control->known;
    const bool active = ImGui::IsItemActive();
    const bool hovered = ImGui::IsItemHovered();
    bool changed = false;

    if (active && control != nullptr) {
        value = std::clamp((ImGui::GetIO().MousePos.x - origin.x) / width, 0.0f, 1.0f);
        claim(frame, index, value);
        changed = true;
    }
    if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        claim(frame, index, 0.5f);
        changed = true;
    }

    ImDrawList* draw = ImGui::GetWindowDrawList();
    const float cy = origin.y + height * 0.5f;
    draw->AddRectFilled(ImVec2(origin.x, cy - 4.0f), ImVec2(origin.x + width, cy + 4.0f),
                        kWell);
    draw->AddRect(ImVec2(origin.x, cy - 4.0f), ImVec2(origin.x + width, cy + 4.0f), kHair);
    // Centre detent mark.
    draw->AddLine(ImVec2(origin.x + width * 0.5f, cy - 8.0f),
                  ImVec2(origin.x + width * 0.5f, cy + 8.0f), kHair, 1.0f);

    if (known || active) {
        const float cap_x = origin.x + value * width;
        draw->AddRectFilled(ImVec2(cap_x - 7.0f, origin.y), ImVec2(cap_x + 7.0f, origin.y + height),
                            hovered || active ? kInk : kMuted);
        draw->AddLine(ImVec2(cap_x, origin.y), ImVec2(cap_x, origin.y + height), kGround,
                      1.0f);
    }

    // A and B in the decks' own colours, at the ends where the hand aims.
    push_mono();
    ImGui::SetCursorScreenPos(ImVec2(origin.x - 18.0f, cy - ImGui::GetTextLineHeight() * 0.5f));
    text_c(kAmber, "A");
    ImGui::SetCursorScreenPos(ImVec2(origin.x + width + 8.0f, cy - ImGui::GetTextLineHeight() * 0.5f));
    text_c(kSlate, "B");
    pop_font();

    // The caption row: curve, value, and the transform detector where the
    // action is instead of buried in another panel.
    push_small();
    ImGui::SetCursorScreenPos(ImVec2(origin.x, origin.y + height + 6.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, rgba(kFaint));
    ImGui::TextUnformatted("courbe sharp");
    ImGui::PopStyleColor();
    ImGui::SameLine(0.0f, 16.0f);
    push_mono();
    if (known) {
        text_c(kInk, "%.2f", static_cast<double>(value));
    } else {
        text_c(kFaint, "?");
    }
    pop_font();
    ImGui::SameLine(0.0f, 16.0f);
    ImGui::PushStyleColor(ImGuiCol_Text,
                          rgba(engine.cuts().transforming() ? kAmber : kFaint));
    ImGui::Text("coupes %.1f/s%s", static_cast<double>(engine.cuts().cuts_per_second()),
                engine.cuts().transforming() ? "  TRANSFORM" : "");
    ImGui::PopStyleColor();
    pop_font();

    ImGui::EndGroup();
    return changed;
}

void draw_surface(Engine& engine, Frame& frame, float width, float height) {
    ImGui::BeginChild("surface", ImVec2(width, height), ImGuiChildFlags_Borders);
    eyebrow("SURFACE \xE2\x80\x94 Reloop Elite \xC2\xB7 RP-8000 MK2 \xE2\x80\x94 la souris joue en attendant le MIDI");
    ImGui::Dummy(ImVec2(0.0f, 8.0f));

    const Surface& surface = engine.surface();
    ControlIndex idx_hi, idx_mid, idx_f1, idx_fader1, idx_hi2, idx_f2, idx_fader2, idx_xf;
    const Control* hi = surface_control(surface, "ch1.eq.hi", &idx_hi);
    const Control* mid = surface_control(surface, "ch1.eq.mid", &idx_mid);
    const Control* filter1 = surface_control(surface, "ch1.filter", &idx_f1);
    const Control* fader1 = surface_control(surface, "ch1.fader", &idx_fader1);
    const Control* hi2 = surface_control(surface, "ch2.eq.hi", &idx_hi2);
    const Control* filter2 = surface_control(surface, "ch2.filter", &idx_f2);
    const Control* fader2 = surface_control(surface, "ch2.fader", &idx_fader2);
    const Control* xf = surface_control(surface, "xfader", &idx_xf);

    // VOIE 1 -- deck A's colour on its accents, so the strip and the deck read
    // as one instrument.
    ImGui::BeginGroup();
    push_small();
    text_c(kAmber, "VOIE 1");
    pop_font();
    ImGui::Dummy(ImVec2(0.0f, 2.0f));
    rotary("##v1hi", "eq.hi", hi, frame, idx_hi, kAmber);
    ImGui::SameLine(0.0f, 2.0f);
    rotary("##v1mid", "eq.mid", mid, frame, idx_mid, kAmber);
    ImGui::SameLine(0.0f, 2.0f);
    rotary("##v1filter", "filtre", filter1, frame, idx_f1, kAmber);
    ImGui::SameLine(0.0f, 10.0f);
    vertical_fader("##v1fader", "fader", fader1, frame, idx_fader1, kAmber);
    ImGui::EndGroup();

    ImGui::SameLine(0.0f, 48.0f);

    // The crossfader between the two voices, where it lives on the hardware.
    ImGui::BeginGroup();
    push_small();
    ImGui::PushStyleColor(ImGuiCol_Text, rgba(kFaint));
    ImGui::TextUnformatted("CROSSFADER \xC2\xB7 Innofader");
    ImGui::PopStyleColor();
    pop_font();
    ImGui::Dummy(ImVec2(0.0f, 26.0f));
    const float xf_width = std::max(240.0f, ImGui::GetContentRegionAvail().x - 460.0f);
    crossfader(xf, frame, idx_xf, engine, xf_width);
    ImGui::EndGroup();

    ImGui::SameLine(0.0f, 48.0f);

    ImGui::BeginGroup();
    push_small();
    text_c(kSlate, "VOIE 2");
    pop_font();
    ImGui::Dummy(ImVec2(0.0f, 2.0f));
    rotary("##v2hi", "eq.hi", hi2, frame, idx_hi2, kSlate);
    ImGui::SameLine(0.0f, 2.0f);
    rotary("##v2filter", "filtre", filter2, frame, idx_f2, kSlate);
    ImGui::SameLine(0.0f, 10.0f);
    vertical_fader("##v2fader", "fader", fader2, frame, idx_fader2, kSlate);
    ImGui::EndGroup();

    ImGui::SameLine(0.0f, 30.0f);
    ImGui::BeginGroup();
    push_small();
    ImGui::PushStyleColor(ImGuiCol_Text, rgba(kFaint));
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() +
                           std::max(150.0f, ImGui::GetContentRegionAvail().x - 12.0f));
    ImGui::TextUnformatted(
        "Glisser pour jouer, double-clic pour recentrer. Un contr\xC3\xB4le pris "
        "\xC3\xA0 la main quitte la d\xC3\xA9mo. En pointill\xC3\xA9 : jamais "
        "touch\xC3\xA9.");
    ImGui::PopTextWrapPos();
    ImGui::PopStyleColor();
    pop_font();
    ImGui::EndGroup();

    ImGui::EndChild();
}

// --- EFFETS: the whole battery, with its correspondences stated honestly ------

void draw_effects_screen(Engine& engine, Frame& frame) {
    eyebrow("LA BATTERIE — chaque effet audio et son pendant visuel");
    push_small();
    ImGui::PushStyleColor(ImGuiCol_Text, rgba(kFaint));
    ImGui::TextUnformatted(
        "\"analogue\" marque une correspondance perceptive choisie, jamais "
        "pr\xC3\xA9sent\xC3\xA9""e comme identique.");
    ImGui::PopStyleColor();
    pop_font();
    ImGui::Dummy(ImVec2(0.0f, 8.0f));

    if (ImGui::BeginTable("catalogue", 4,
                          ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("effet", ImGuiTableColumnFlags_WidthFixed, 130.0f);
        ImGui::TableSetupColumn("audio", ImGuiTableColumnFlags_WidthFixed, 260.0f);
        ImGui::TableSetupColumn("video", ImGuiTableColumnFlags_WidthFixed, 300.0f);
        ImGui::TableSetupColumn("lien", ImGuiTableColumnFlags_WidthFixed, 110.0f);

        for (const EffectDescriptor& fx : effect_catalogue()) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            text_c(kInk, "%s", fx.id);
            ImGui::TableSetColumnIndex(1);
            push_small();
            text_c(fx.audio != nullptr ? kMuted : kFaint, "%s",
                   fx.audio != nullptr ? fx.audio : "—");
            pop_font();
            ImGui::TableSetColumnIndex(2);
            push_small();
            text_c(kMuted, "%s", fx.video);
            pop_font();
            ImGui::TableSetColumnIndex(3);
            push_small();
            switch (fx.relation) {
                case Correspondence::Identical: text_c(kSage, "identique"); break;
                case Correspondence::Analogue: text_c(kAmber, "analogue"); break;
                case Correspondence::VideoOnly: text_c(kFaint, "vid\xC3\xA9o seule"); break;
            }
            pop_font();
        }
        ImGui::EndTable();
    }

    ImGui::Dummy(ImVec2(0.0f, 14.0f));
    eyebrow("LE RACK — ce qui est charg\xC3\xA9 maintenant");
    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    draw_mix(engine, frame, 0.0f, 0.0f);
}

// --- MAPPING ------------------------------------------------------------------

const char* source_text(SourceKind kind) {
    switch (kind) {
        case SourceKind::Control: return "contr\xC3\xB4le";
        case SourceKind::DeckPosition: return "position deck";
        case SourceKind::DeckVelocity: return "vitesse deck";
        case SourceKind::DeckAcceleration: return "acc\xC3\xA9l\xC3\xA9ration";
        case SourceKind::DeckScratchRate: return "scratch/s";
        case SourceKind::DeckConfidence: return "confiance";
        case SourceKind::Gesture: return "geste";
        case SourceKind::Modulator: return "modulateur";
        case SourceKind::AudioBand: return "bande audio";
    }
    return "?";
}

void draw_mapping_screen(Engine& engine) {
    eyebrow("MAPPING — toute source vers toute destination");
    ImGui::Dummy(ImVec2(0.0f, 8.0f));

    if (ImGui::BeginTable("mappings", 4,
                          ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("nom", ImGuiTableColumnFlags_WidthFixed, 300.0f);
        ImGui::TableSetupColumn("source", ImGuiTableColumnFlags_WidthFixed, 140.0f);
        ImGui::TableSetupColumn("destination", ImGuiTableColumnFlags_WidthFixed, 200.0f);
        ImGui::TableSetupColumn("valeur", ImGuiTableColumnFlags_WidthFixed, 100.0f);

        for (std::size_t i = 0; i < engine.mapping().size(); ++i) {
            const Mapping& mapping = engine.mapping().at(i);
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            text_c(engine.mapping().active(i) ? kInk : kFaint, "%s", mapping.name.c_str());
            ImGui::TableSetColumnIndex(1);
            push_small();
            text_c(kMuted, "%s", source_text(mapping.source.kind));
            pop_font();
            ImGui::TableSetColumnIndex(2);
            push_small();
            text_c(kMuted, "%s", mapping.destination.target.c_str());
            pop_font();
            ImGui::TableSetColumnIndex(3);
            push_mono();
            text_c(kInk, "%9.2f", static_cast<double>(engine.mapping().value(i)));
            pop_font();
        }
        ImGui::EndTable();
    }

    ImGui::Dummy(ImVec2(0.0f, 14.0f));
    eyebrow("APPRENTISSAGE MIDI");
    push_small();
    ImGui::PushStyleColor(ImGuiCol_Text, rgba(kFaint));
    ImGui::TextUnformatted(
        "La table de l'Elite n'est pas documient\xC3\xA9""e publiquement, donc rien "
        "n'est en dur : --midi-learn demande de balayer chaque contr\xC3\xB4le et "
        "n'associe qu'un contr\xC3\xB4le qui bouge vraiment. La checklist compl\xC3\xA8te "
        "est dans `scratchvj layout`.");
    ImGui::PopStyleColor();
    pop_font();
}

// --- SORTIE: corner pin, warp mesh, mask, and the presets that keep them -----

// Where the presets live. Beside the executable's working directory, so a
// venue's mapping travels with the project rather than hiding in an app-data
// folder nobody thinks to copy.
const char* kPresetDirectory = "mappings";

// The state the preset panel keeps between frames: the name being typed, the
// list as last read from disk, and whatever the last save or load had to say.
struct PresetPanel {
    char name[64] = "";
    std::vector<std::string> names;
    std::string message;
    bool listed = false;
};

PresetPanel g_presets;

void refresh_presets() {
    g_presets.names = preset_names(kPresetDirectory);
    g_presets.listed = true;
}

// One draggable handle. Returns true while the hand is on it.
bool handle(const char* id, ImVec2 at, ImU32 colour, float radius) {
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImGui::SetCursorScreenPos(ImVec2(at.x - radius - 3.0f, at.y - radius - 3.0f));
    ImGui::PushID(id);
    ImGui::InvisibleButton("h", ImVec2(radius * 2.0f + 6.0f, radius * 2.0f + 6.0f));
    const bool active = ImGui::IsItemActive();
    const bool hot = active || ImGui::IsItemHovered();
    ImGui::PopID();
    draw->AddRectFilled(ImVec2(at.x - radius, at.y - radius),
                        ImVec2(at.x + radius, at.y + radius), hot ? kInk : colour);
    return active;
}

void draw_output_screen(Engine& engine) {
    if (!g_presets.listed) refresh_presets();

    eyebrow("SORTIE \xE2\x80\x94 g\xC3\xA9om\xC3\xA9trie de projection");
    push_small();
    ImGui::PushStyleColor(ImGuiCol_Text, rgba(kFaint));
    ImGui::TextUnformatted(
        "Le pin est une homographie, pas un \xC3\xA9tirement bilin\xC3\xA9""aire : sous un "
        "vrai projecteur le centre de l'image ne tombe pas au centre du quadrilat\xC3\xA8re. "
        "La grille prend le relais quand la surface n'est pas plane.");
    ImGui::PopStyleColor();
    pop_font();
    ImGui::Dummy(ImVec2(0.0f, 10.0f));

    CornerPin& pin = engine.pin();
    WarpMesh& mesh = engine.mesh();
    const bool mesh_on = engine.mesh_enabled();

    const float width = std::min(760.0f, ImGui::GetContentRegionAvail().x - 320.0f);
    const float height = width * 9.0f / 16.0f;
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetCursorScreenPos();

    draw->AddRectFilled(origin, ImVec2(origin.x + width, origin.y + height), kWell);
    draw->AddRect(origin, ImVec2(origin.x + width, origin.y + height), kHair);

    const auto to_screen = [&](const Point& p) {
        return ImVec2(origin.x + static_cast<float>(p.x) * width,
                      origin.y + static_cast<float>(p.y) * height);
    };
    const auto from_screen = [&](ImVec2 at) {
        return Point{std::clamp(static_cast<double>(at.x - origin.x) / width, -0.2, 1.2),
                     std::clamp(static_cast<double>(at.y - origin.y) / height, -0.2, 1.2)};
    };

    if (mesh_on) {
        // The grid as the surface actually is: every cell edge sampled, so a
        // Bezier mesh shows its curve rather than a polygon that lies about it.
        const int steps = 10;
        for (int row = 0; row < mesh.rows(); ++row) {
            ImVec2 previous;
            for (int col = 0; col < mesh.cols() - 1; ++col) {
                for (int s = 0; s <= steps; ++s) {
                    const double u = mesh.column_u(col) +
                                     (mesh.column_u(col + 1) - mesh.column_u(col)) * s / steps;
                    const ImVec2 point = to_screen(mesh.map(u, mesh.row_v(row)));
                    if (col > 0 || s > 0) draw->AddLine(previous, point, kHair, 1.2f);
                    previous = point;
                }
            }
        }
        for (int col = 0; col < mesh.cols(); ++col) {
            ImVec2 previous;
            for (int row = 0; row < mesh.rows() - 1; ++row) {
                for (int s = 0; s <= steps; ++s) {
                    const double v = mesh.row_v(row) +
                                     (mesh.row_v(row + 1) - mesh.row_v(row)) * s / steps;
                    const ImVec2 point = to_screen(mesh.map(mesh.column_u(col), v));
                    if (row > 0 || s > 0) draw->AddLine(previous, point, kHair, 1.2f);
                    previous = point;
                }
            }
        }
        // The outline last, over the grid, so the shape reads at a glance.
        for (int s = 0; s < 4; ++s) {
            const int steps_edge = 32;
            ImVec2 previous;
            for (int i = 0; i <= steps_edge; ++i) {
                const double t = static_cast<double>(i) / steps_edge;
                const Point p = s == 0   ? mesh.map(t, 0.0)
                                : s == 1 ? mesh.map(1.0, t)
                                : s == 2 ? mesh.map(1.0 - t, 1.0)
                                         : mesh.map(0.0, 1.0 - t);
                const ImVec2 point = to_screen(p);
                if (i > 0) draw->AddLine(previous, point, kAccent, 2.0f);
                previous = point;
            }
        }

        // A handle on every control point. Corners drawn larger: they are the
        // four a hand reaches for first and must never be lost in the grid.
        for (int row = 0; row < mesh.rows(); ++row) {
            for (int col = 0; col < mesh.cols(); ++col) {
                const bool corner = (col == 0 || col == mesh.cols() - 1) &&
                                    (row == 0 || row == mesh.rows() - 1);
                char id[32];
                std::snprintf(id, sizeof(id), "m%d_%d", col, row);
                if (handle(id, to_screen(mesh.at(col, row)),
                           corner ? kAccent : kSlate, corner ? 5.0f : 3.5f)) {
                    mesh.set(col, row, from_screen(ImGui::GetIO().MousePos));
                }
            }
        }
    } else {
        // The pin's own grid, through the real homography from core/warp -- the
        // same matrix the shader will get. Previewing it any other way would
        // show a warp the output does not do.
        const Homography h = homography_from(pin);
        const int kGrid = 8;
        for (int i = 0; i <= kGrid; ++i) {
            const double t = static_cast<double>(i) / kGrid;
            ImVec2 prev_row, prev_col;
            for (int j = 0; j <= kGrid; ++j) {
                const double u = static_cast<double>(j) / kGrid;
                const ImVec2 row = to_screen(apply(h, Point{u, t}));
                const ImVec2 col = to_screen(apply(h, Point{t, u}));
                if (j > 0) {
                    draw->AddLine(prev_row, row, kHair);
                    draw->AddLine(prev_col, col, kHair);
                }
                prev_row = row;
                prev_col = col;
            }
        }
        Point* corners[4] = {&pin.top_left, &pin.top_right, &pin.bottom_right,
                             &pin.bottom_left};
        for (int i = 0; i < 4; ++i) {
            draw->AddLine(to_screen(*corners[i]), to_screen(*corners[(i + 1) % 4]), kAccent,
                          2.0f);
        }
        for (int i = 0; i < 4; ++i) {
            char id[16];
            std::snprintf(id, sizeof(id), "pin%d", i);
            if (handle(id, to_screen(*corners[i]), kAccent, 5.0f)) {
                *corners[i] = from_screen(ImGui::GetIO().MousePos);
            }
        }
    }

    ImGui::SetCursorScreenPos(ImVec2(origin.x + width + 24.0f, origin.y));
    ImGui::BeginGroup();

    // --- which tool -----------------------------------------------------------
    eyebrow("SURFACE");
    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    if (ImGui::SmallButton("Corner pin")) engine.set_mesh_enabled(false);
    if (!mesh_on) {
        const ImVec2 lo = ImGui::GetItemRectMin();
        const ImVec2 hi = ImGui::GetItemRectMax();
        draw->AddLine(ImVec2(lo.x, hi.y), ImVec2(hi.x, hi.y), kAccent, 2.0f);
    }
    ImGui::SameLine(0.0f, 8.0f);
    if (ImGui::SmallButton("Grille")) engine.set_mesh_enabled(true);
    if (mesh_on) {
        const ImVec2 lo = ImGui::GetItemRectMin();
        const ImVec2 hi = ImGui::GetItemRectMax();
        draw->AddLine(ImVec2(lo.x, hi.y), ImVec2(hi.x, hi.y), kAccent, 2.0f);
    }

    if (mesh_on) {
        ImGui::Dummy(ImVec2(0.0f, 12.0f));
        eyebrow("INTERPOLATION");
        ImGui::Dummy(ImVec2(0.0f, 4.0f));
        const bool bezier = mesh.interpolation() == WarpInterpolation::Bezier;
        if (ImGui::SmallButton("Lin\xC3\xA9""aire")) {
            mesh.set_interpolation(WarpInterpolation::Bilinear);
        }
        if (!bezier) {
            const ImVec2 lo = ImGui::GetItemRectMin();
            const ImVec2 hi = ImGui::GetItemRectMax();
            draw->AddLine(ImVec2(lo.x, hi.y), ImVec2(hi.x, hi.y), kAccent, 2.0f);
        }
        ImGui::SameLine(0.0f, 8.0f);
        if (ImGui::SmallButton("B\xC3\xA9zier")) {
            mesh.set_interpolation(WarpInterpolation::Bezier);
        }
        if (bezier) {
            const ImVec2 lo = ImGui::GetItemRectMin();
            const ImVec2 hi = ImGui::GetItemRectMax();
            draw->AddLine(ImVec2(lo.x, hi.y), ImVec2(hi.x, hi.y), kAccent, 2.0f);
        }

        ImGui::Dummy(ImVec2(0.0f, 12.0f));
        eyebrow("GRILLE");
        push_mono();
        text_c(kInk, "%d x %d", mesh.cols(), mesh.rows());
        pop_font();
        ImGui::Dummy(ImVec2(0.0f, 4.0f));
        // Insertion goes in the middle of the grid, which is where a hand
        // reaching for more control is almost always looking.
        if (ImGui::SmallButton("+ colonne")) mesh.insert_column(mesh.cols() / 2);
        ImGui::SameLine(0.0f, 8.0f);
        if (ImGui::SmallButton("\xE2\x88\x92 colonne")) mesh.remove_column(mesh.cols() / 2);
        if (ImGui::SmallButton("+ ligne")) mesh.insert_row(mesh.rows() / 2);
        ImGui::SameLine(0.0f, 8.0f);
        if (ImGui::SmallButton("\xE2\x88\x92 ligne")) mesh.remove_row(mesh.rows() / 2);
        ImGui::SameLine(0.0f, 16.0f);
        if (ImGui::SmallButton("R\xC3\xA9initialiser##mesh")) {
            mesh.reset(mesh.cols(), mesh.rows());
        }

        push_small();
        ImGui::PushStyleColor(ImGuiCol_Text, rgba(kFaint));
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 290.0f);
        ImGui::TextUnformatted(
            "Ajouter une ligne ne bouge pas l'image : les nouveaux points sont "
            "\xC3\xA9""chantillonn\xC3\xA9s sur la surface actuelle. Exact en "
            "lin\xC3\xA9""aire, \xC3\xA0 0,4 % pr\xC3\xA8s en b\xC3\xA9zier.");
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();
        pop_font();
    } else {
        ImGui::Dummy(ImVec2(0.0f, 12.0f));
        if (ImGui::SmallButton("R\xC3\xA9initialiser##pin")) pin = CornerPin{};
        ImGui::SameLine(0.0f, 12.0f);
        push_small();
        ImGui::PushStyleColor(ImGuiCol_Text, rgba(kFaint));
        ImGui::TextUnformatted(pin.is_identity() ? "identit\xC3\xA9 \xE2\x80\x94 plein cadre"
                                                 : "homographie active");
        ImGui::PopStyleColor();
        pop_font();
    }

    // --- presets --------------------------------------------------------------
    ImGui::Dummy(ImVec2(0.0f, 18.0f));
    eyebrow("PRESETS DE MAPPING");
    push_small();
    ImGui::PushStyleColor(ImGuiCol_Text, rgba(kFaint));
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 290.0f);
    ImGui::TextUnformatted(
        "Une g\xC3\xA9om\xC3\xA9trie vise UNE salle. La perdre au red\xC3\xA9marrage, "
        "c'est refaire l'\xC3\xA9""chelle.");
    ImGui::PopTextWrapPos();
    ImGui::PopStyleColor();
    pop_font();
    ImGui::Dummy(ImVec2(0.0f, 6.0f));

    ImGui::SetNextItemWidth(200.0f);
    ImGui::InputTextWithHint("##presetname", "nom de la salle", g_presets.name,
                             sizeof(g_presets.name));
    ImGui::SameLine(0.0f, 8.0f);
    if (ImGui::SmallButton("Enregistrer") && g_presets.name[0] != '\0') {
        OutputPreset preset;
        preset.name = g_presets.name;
        preset.pin = pin;
        preset.mesh = mesh;
        preset.mesh_enabled = engine.mesh_enabled();
        std::string error;
        if (preset_save(preset, preset_path(kPresetDirectory, preset.name), error)) {
            g_presets.message = "enregistr\xC3\xA9 : " + preset.name;
            refresh_presets();
        } else {
            g_presets.message = error;
        }
    }

    ImGui::Dummy(ImVec2(0.0f, 6.0f));
    if (g_presets.names.empty()) {
        push_small();
        dim("aucun preset enregistr\xC3\xA9");
        pop_font();
    }
    for (const std::string& name : g_presets.names) {
        ImGui::PushID(name.c_str());
        if (ImGui::SmallButton("Charger")) {
            OutputPreset preset;
            std::string error;
            if (preset_load(preset_path(kPresetDirectory, name), preset, error)) {
                pin = preset.pin;
                mesh = preset.mesh;
                engine.set_mesh_enabled(preset.mesh_enabled);
                std::snprintf(g_presets.name, sizeof(g_presets.name), "%s", name.c_str());
                g_presets.message = "charg\xC3\xA9 : " + name;
            } else {
                g_presets.message = error;
            }
        }
        ImGui::SameLine(0.0f, 10.0f);
        text_c(kInk, "%s", name.c_str());
        ImGui::PopID();
    }

    if (!g_presets.message.empty()) {
        ImGui::Dummy(ImVec2(0.0f, 6.0f));
        push_small();
        ImGui::PushStyleColor(ImGuiCol_Text, rgba(kSage));
        ImGui::TextUnformatted(g_presets.message.c_str());
        ImGui::PopStyleColor();
        pop_font();
    }

    ImGui::Dummy(ImVec2(0.0f, 16.0f));
    push_small();
    ImGui::PushStyleColor(ImGuiCol_Text, rgba(kFaint));
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 290.0f);
    ImGui::TextUnformatted(
        "Edge blending multi-projecteurs : toujours absent. La sortie part en "
        "Spout vers Resolume ou MadMapper pour ce cas-l\xC3\xA0.");
    ImGui::PopTextWrapPos();
    ImGui::PopStyleColor();
    pop_font();
    ImGui::EndGroup();
}

// --- the layouts -------------------------------------------------------------
//
// One performance screen, five arrangements. They are not five skins: a set has
// phases, and each phase wants a different thing large. Naming them after the
// phase rather than the widget is deliberate -- you pick where you ARE, and the
// arrangement follows.

const char* layout_name(Layout layout) {
    switch (layout) {
        case Layout::Booth: return "CABINE";
        case Layout::Stage: return "SC\xC3\x88NE";
        case Layout::Prepare: return "PR\xC3\x89PA";
        case Layout::Sphere: return "360";
        case Layout::FullFrame: return "PLEIN CADRE";
    }
    return "?";
}

// The program, as large as the space given, letterboxed to its own aspect.
void draw_program_view(const Frame& frame, float width, float height,
                       const char* caption) {
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    draw->AddRectFilled(origin, ImVec2(origin.x + width, origin.y + height), kWell);

    if (frame.tex_program != nullptr && frame.program_height > 0) {
        const float aspect = static_cast<float>(frame.program_width) /
                             static_cast<float>(frame.program_height);
        float w = width;
        float h = w / aspect;
        if (h > height) {
            h = height;
            w = h * aspect;
        }
        const ImVec2 lo(origin.x + (width - w) * 0.5f, origin.y + (height - h) * 0.5f);
        draw->AddImage(ImTextureRef(reinterpret_cast<ImTextureID>(frame.tex_program)), lo,
                       ImVec2(lo.x + w, lo.y + h));
    } else {
        push_small();
        const char* waiting = "aucun program \xE2\x80\x94 charger un clip analys\xC3\xA9";
        const ImVec2 size = ImGui::CalcTextSize(waiting);
        draw->AddText(ImVec2(origin.x + (width - size.x) * 0.5f,
                             origin.y + height * 0.5f - size.y),
                      kFaint, waiting);
        pop_font();
    }
    draw->AddRect(origin, ImVec2(origin.x + width, origin.y + height), kHair);

    if (caption != nullptr) {
        push_small();
        draw->AddText(ImVec2(origin.x + 10.0f, origin.y + 8.0f), kFaint, caption);
        pop_font();
    }
    ImGui::Dummy(ImVec2(width, height));
}

// A deck reduced to what a glance needs: name, position, a scrubbable strip,
// and the transport source. Used where the decks are not the subject.
void draw_deck_strip(Deck& deck, Frame& frame, ImU32 accent, const char* letter,
                     float width, float height) {
    ImGui::PushID(&deck);
    ImGui::BeginChild(letter, ImVec2(width, height), ImGuiChildFlags_Borders);
    const float inner = ImGui::GetContentRegionAvail().x;

    ImGui::PushStyleColor(ImGuiCol_Text, rgba(accent));
    ImGui::TextUnformatted(letter);
    ImGui::PopStyleColor();
    ImGui::SameLine(0.0f, 10.0f);
    text_c(kInk, "%s", deck.name.c_str());

    ImGui::SameLine();
    push_mono();
    const std::string now = clock_of(deck.played.position_s);
    const float right = ImGui::GetContentRegionMax().x - ImGui::CalcTextSize(now.c_str()).x;
    if (right > ImGui::GetCursorPosX()) ImGui::SameLine(right);
    text_c(kInk, "%s", now.c_str());
    pop_font();

    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    filmstrip(deck, frame, inner, 26.0f);
    ImGui::EndChild();
    ImGui::PopID();
}

void draw_layout_booth(Engine& engine, Frame& frame) {
    // The mockup's proportions: a fixed library rail, then the decks, then the
    // surface across the bottom. Fixed because a browser that reflows while a
    // set is running is one nobody can find anything in.
    const float rail = 250.0f;
    const float gap = ImGui::GetStyle().ItemSpacing.x;
    const float surface_h = 208.0f;
    const float body_h =
        std::max(320.0f, ImGui::GetContentRegionAvail().y - surface_h - gap);
    const float decks_w = std::max(520.0f, ImGui::GetContentRegionAvail().x - rail - gap);
    const float deck_w = (decks_w - gap) * 0.5f;
    // The program panel's fixed claim: three effect rows plus the preview, and
    // never the thing that gets clipped.
    const float mix_h = 222.0f;
    const float deck_h = body_h - mix_h - ImGui::GetStyle().ItemSpacing.y;

    draw_library(engine, rail, body_h);
    ImGui::SameLine();

    ImGui::BeginGroup();
    draw_deck(engine.deck_a(), engine, frame, frame.tex_a, kAmber, true, deck_w, deck_h);
    ImGui::SameLine();
    draw_deck(engine.deck_b(), engine, frame, frame.tex_b, kSlate, false, deck_w, deck_h);
    draw_mix(engine, frame, decks_w, mix_h);
    ImGui::EndGroup();

    draw_surface(engine, frame, 0.0f, surface_h);
}

void draw_layout_stage(Engine& engine, Frame& frame) {
    // The show is running and the eyes are on the output. The program takes the
    // room; the decks shrink to the two things a glance actually needs, a
    // position and a strip to grab.
    const float gap = ImGui::GetStyle().ItemSpacing.x;
    const float surface_h = 208.0f;
    const float body_h =
        std::max(300.0f, ImGui::GetContentRegionAvail().y - surface_h - gap);
    const float side = 360.0f;
    const float program_w = std::max(420.0f, ImGui::GetContentRegionAvail().x - side - gap);

    ImGui::BeginChild("stage.program", ImVec2(program_w, body_h), ImGuiChildFlags_Borders);
    draw_program_view(frame, ImGui::GetContentRegionAvail().x,
                      ImGui::GetContentRegionAvail().y, "PROGRAM \xC2\xB7 SPOUT scratchvj");
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginGroup();
    const float strip_h = 86.0f;
    draw_deck_strip(engine.deck_a(), frame, kAmber, "A", side, strip_h);
    draw_deck_strip(engine.deck_b(), frame, kSlate, "B", side, strip_h);
    draw_mix(engine, frame, side,
             body_h - 2.0f * (strip_h + ImGui::GetStyle().ItemSpacing.y));
    ImGui::EndGroup();

    draw_surface(engine, frame, 0.0f, surface_h);
}

void draw_layout_prepare(Engine& engine, Frame& frame) {
    // Before the set: find the moments. No program at all -- nothing is going
    // out yet -- so the room goes to the library and to two tall strips that a
    // hand can land on precisely.
    const float gap = ImGui::GetStyle().ItemSpacing.x;
    const float rail = 380.0f;
    const float body_h = std::max(300.0f, ImGui::GetContentRegionAvail().y);
    const float decks_w = std::max(480.0f, ImGui::GetContentRegionAvail().x - rail - gap);
    const float deck_h = (body_h - gap) * 0.5f;

    draw_library(engine, rail, body_h);
    ImGui::SameLine();

    ImGui::BeginGroup();
    draw_deck(engine.deck_a(), engine, frame, frame.tex_a, kAmber, true, decks_w, deck_h);
    draw_deck(engine.deck_b(), engine, frame, frame.tex_b, kSlate, false, decks_w, deck_h);
    ImGui::EndGroup();
}

// The sight frame: where the current view lands on the equirect source. Four
// corners of the screen pushed through core/sphere -- the same function the
// shader transcribes -- so the outline cannot drift from what is rendered.
void draw_sight_frame(const SphereView& view, ImVec2 origin, float width, float height) {
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const int steps = 24;
    ImVec2 previous;
    bool have_previous = false;

    // Walk the border of the view in screen space; each sample becomes a point
    // on the equirect. Walking the border rather than drawing four straight
    // lines matters: under a wide field of view those edges are curves.
    for (int i = 0; i <= steps * 4; ++i) {
        const int side = i / steps;
        const double t = static_cast<double>(i % steps) / steps;
        Vec2 uv;
        switch (side) {
            case 0: uv = Vec2{t, 0.0}; break;
            case 1: uv = Vec2{1.0, t}; break;
            case 2: uv = Vec2{1.0 - t, 1.0}; break;
            default: uv = Vec2{0.0, 1.0 - t}; break;
        }
        const Vec2 hit = sample_equirect(view, uv);
        const ImVec2 point(origin.x + static_cast<float>(hit.u) * width,
                           origin.y + static_cast<float>(hit.v) * height);
        // The equirect wraps, so a segment that crosses the seam would draw a
        // line straight across the picture. Break it instead.
        if (have_previous && std::fabs(point.x - previous.x) < width * 0.5f) {
            draw->AddLine(previous, point, kAccent, 1.5f);
        }
        previous = point;
        have_previous = true;
    }
}

void draw_layout_sphere(Engine& engine, Frame& frame) {
    // Working spherical material: the projected view large, the gaze under the
    // hand, and the source with a sight frame showing where you are looking.
    const float gap = ImGui::GetStyle().ItemSpacing.x;
    const float side = 420.0f;
    const float body_h = std::max(300.0f, ImGui::GetContentRegionAvail().y);
    const float view_w = std::max(420.0f, ImGui::GetContentRegionAvail().x - side - gap);

    Deck& deck = engine.deck_a();
    SphereView& gaze = engine.view_a();
    const bool is360 = deck.clip.width == deck.clip.height * 2;

    ImGui::BeginChild("sphere.view", ImVec2(view_w, body_h), ImGuiChildFlags_Borders);
    const float inner = ImGui::GetContentRegionAvail().x;
    ImGui::PushStyleColor(ImGuiCol_Text, rgba(kAmber));
    ImGui::TextUnformatted("A");
    ImGui::PopStyleColor();
    ImGui::SameLine(0.0f, 10.0f);
    text_c(kInk, "%s", deck.name.c_str());
    ImGui::SameLine(0.0f, 12.0f);
    push_small();
    ImGui::PushStyleColor(ImGuiCol_Text, rgba(is360 ? kFaint : kAlert));
    ImGui::TextUnformatted(is360 ? "\xC3\xA9quirectangulaire"
                                 : "ce clip n'est pas du 360");
    ImGui::PopStyleColor();
    pop_font();

    ImGui::Dummy(ImVec2(0.0f, 6.0f));
    const float picture_h = std::max(200.0f, body_h * 0.52f);
    picture_well(deck, frame.tex_a, is360 ? static_cast<float>(gaze.aspect) : 0.0f, inner,
                 picture_h);
    ImGui::Dummy(ImVec2(0.0f, 6.0f));
    filmstrip(deck, frame, inner, 30.0f);
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::BeginChild("sphere.gaze", ImVec2(side, body_h), ImGuiChildFlags_Borders);
    eyebrow("REGARD");
    ImGui::Dummy(ImVec2(0.0f, 4.0f));

    const struct { const char* label; Projection value; } projections[] = {
        {"Perspective", Projection::Perspective},
        {"Little planet", Projection::LittlePlanet},
        {"Fisheye", Projection::Fisheye},
    };
    for (const auto& option : projections) {
        if (&option != &projections[0]) ImGui::SameLine(0.0f, 8.0f);
        if (ImGui::SmallButton(option.label)) gaze.projection = option.value;
        if (gaze.projection == option.value) {
            ImDrawList* draw = ImGui::GetWindowDrawList();
            const ImVec2 lo = ImGui::GetItemRectMin();
            const ImVec2 hi = ImGui::GetItemRectMax();
            draw->AddLine(ImVec2(lo.x, hi.y), ImVec2(hi.x, hi.y), kAccent, 2.0f);
        }
    }

    ImGui::Dummy(ImVec2(0.0f, 10.0f));
    // Doubles, because that is what core/sphere speaks; ImGui edits floats, so
    // each one is bounced rather than the geometry being weakened to float.
    const auto slider = [](const char* label, double& value, float lo, float hi,
                           const char* format) {
        float editable = static_cast<float>(value);
        ImGui::SetNextItemWidth(220.0f);
        if (ImGui::SliderFloat(label, &editable, lo, hi, format)) {
            value = static_cast<double>(editable);
        }
    };
    slider("lacet", gaze.yaw_deg, -180.0f, 180.0f, "%.0f\xC2\xB0");
    slider("tangage", gaze.pitch_deg, -90.0f, 90.0f, "%.0f\xC2\xB0");
    slider("roulis", gaze.roll_deg, -180.0f, 180.0f, "%.0f\xC2\xB0");
    if (gaze.projection == Projection::Perspective) {
        slider("champ", gaze.fov_deg, 20.0f, 170.0f, "%.0f\xC2\xB0");
    } else {
        slider("zoom", gaze.planet_zoom, 0.2f, 3.0f, "%.2f");
    }

    push_small();
    ImGui::PushStyleColor(ImGuiCol_Text, rgba(kFaint));
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + side - 24.0f);
    ImGui::TextUnformatted(
        "Le lacet et le tangage suivent aussi le mapping : un potard, un LFO ou "
        "un casque les bougent par la m\xC3\xAAme porte. Bouger un curseur ici est "
        "provisoire jusqu'au prochain pas du mapping.");
    ImGui::PopTextWrapPos();
    ImGui::PopStyleColor();
    pop_font();
    ImGui::EndChild();

    ImGui::BeginChild("sphere.source",
                      ImVec2(side, body_h - ImGui::GetItemRectSize().y -
                                       ImGui::GetStyle().ItemSpacing.y),
                      ImGuiChildFlags_Borders);
    eyebrow("SOURCE \xC3\x89QUIRECTANGULAIRE \xE2\x80\x94 cadre de vis\xC3\xA9""e");
    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    {
        const float w = ImGui::GetContentRegionAvail().x;
        const float h = w * 0.5f;  // an equirect is always 2:1
        ImDrawList* draw = ImGui::GetWindowDrawList();
        const ImVec2 origin = ImGui::GetCursorScreenPos();
        draw->AddRectFilled(origin, ImVec2(origin.x + w, origin.y + h), kWell);
        if (frame.tex_equirect != nullptr) {
            draw->AddImage(ImTextureRef(reinterpret_cast<ImTextureID>(frame.tex_equirect)),
                           origin, ImVec2(origin.x + w, origin.y + h));
        }
        if (is360) draw_sight_frame(gaze, origin, w, h);
        draw->AddRect(origin, ImVec2(origin.x + w, origin.y + h), kHair);
        ImGui::Dummy(ImVec2(w, h));
    }
    ImGui::EndChild();
    ImGui::EndGroup();
}

void draw_layout_full(const Frame& frame) {
    // Nothing but the output. For the screen the audience sees, or for judging
    // the picture with no interface in the way.
    draw_program_view(frame, ImGui::GetContentRegionAvail().x,
                      ImGui::GetContentRegionAvail().y, nullptr);
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
    c[ImGuiCol_HeaderHovered] = rgba(kHair);
    c[ImGuiCol_HeaderActive] = rgba(kHair);
    c[ImGuiCol_ScrollbarBg] = rgba(kWell);
    c[ImGuiCol_ScrollbarGrab] = rgba(kHair);

    // Every widget ImGui ships blue, retuned to the mockup's palette. A single
    // stock-blue tab is enough to make the whole window read as a debug tool.
    c[ImGuiCol_Tab] = rgba(kGround);
    c[ImGuiCol_TabHovered] = rgba(kPanel);
    c[ImGuiCol_TabSelected] = rgba(kPanel);
    c[ImGuiCol_TabSelectedOverline] = rgba(kAccent);
    c[ImGuiCol_TabDimmed] = rgba(kGround);
    c[ImGuiCol_TabDimmedSelected] = rgba(kPanel);
    c[ImGuiCol_TabDimmedSelectedOverline] = rgba(kGround);
    c[ImGuiCol_TableHeaderBg] = rgba(kGround);
    c[ImGuiCol_TableBorderStrong] = rgba(kHair);
    c[ImGuiCol_TableBorderLight] = rgba(kHair);
    c[ImGuiCol_TableRowBg] = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.015f);
    c[ImGuiCol_TextSelectedBg] = ImVec4(0.79f, 0.46f, 0.18f, 0.35f);
    c[ImGuiCol_NavCursor] = rgba(kAccent);
    c[ImGuiCol_DragDropTarget] = rgba(kAccent);
    c[ImGuiCol_ResizeGrip] = rgba(kHair);
    c[ImGuiCol_ResizeGripHovered] = rgba(kAccent);
    c[ImGuiCol_ResizeGripActive] = rgba(kAccent);
    c[ImGuiCol_SeparatorHovered] = rgba(kAccent);
    c[ImGuiCol_SeparatorActive] = rgba(kAccent);
    c[ImGuiCol_TitleBg] = rgba(kGround);
    c[ImGuiCol_TitleBgActive] = rgba(kGround);
}

void draw(Engine& engine, Frame& frame) {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::Begin("scratchvj", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus);

    draw_status(engine, frame);

    // The mockup's five screens, as tabs. Performance is the one a set lives in;
    // the others are preparation and configuration, which is why they can afford
    // to be screens at all instead of fighting for the same pixels.
    // The layout picker, in the status bar's own row: keys 1-5 do the same
    // thing, because in a set nobody aims at a small button.
    for (int i = 0; i < kLayoutCount; ++i) {
        const Layout candidate = static_cast<Layout>(i);
        if (i > 0) ImGui::SameLine(0.0f, 6.0f);
        push_small();
        if (ImGui::SmallButton(layout_name(candidate))) frame.layout = candidate;
        pop_font();
        if (frame.layout == candidate) {
            ImDrawList* draw = ImGui::GetWindowDrawList();
            const ImVec2 lo = ImGui::GetItemRectMin();
            const ImVec2 hi = ImGui::GetItemRectMax();
            draw->AddLine(ImVec2(lo.x, hi.y), ImVec2(hi.x, hi.y), kAccent, 2.0f);
        }
        if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(ImGuiKey_1 + i), false)) {
            frame.layout = candidate;
        }
    }
    ImGui::Dummy(ImVec2(0.0f, 2.0f));

    if (ImGui::BeginTabBar("screens")) {
        if (ImGui::BeginTabItem("PERFORMANCE")) {
            switch (frame.layout) {
                case Layout::Stage: draw_layout_stage(engine, frame); break;
                case Layout::Prepare: draw_layout_prepare(engine, frame); break;
                case Layout::Sphere: draw_layout_sphere(engine, frame); break;
                case Layout::FullFrame: draw_layout_full(frame); break;
                case Layout::Booth:
                default: draw_layout_booth(engine, frame); break;
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("EFFETS")) {
            draw_effects_screen(engine, frame);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("MAPPING")) {
            draw_mapping_screen(engine);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("SORTIE")) {
            draw_output_screen(engine);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}

}  // namespace svj::ui
