#include "panels.h"

#include "config/warp_io.h"
#include "core/videofx.h"
#include "core/videotaps.h"

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


// --- the three families of control ------------------------------------------
//
// A button, a segmented selector and a toggle, and nothing else. Every choice
// on screen is one of these: an action, an exclusive choice, an on/off. They
// all have a face -- a fill and a border -- so a control and a caption can
// never be mistaken for one another, which is what the underlined-word chips
// this replaces kept doing.

enum class Icon { None, SkipStart, Play, Pause, Stop };

void draw_icon(ImDrawList* draw, Icon icon, ImVec2 centre, float size, ImU32 colour) {
    const float h = size * 0.5f;
    switch (icon) {
        case Icon::Play:
            draw->AddTriangleFilled(ImVec2(centre.x - h * 0.8f, centre.y - h),
                                    ImVec2(centre.x + h, centre.y),
                                    ImVec2(centre.x - h * 0.8f, centre.y + h), colour);
            break;
        case Icon::Pause:
            draw->AddRectFilled(ImVec2(centre.x - h, centre.y - h),
                                ImVec2(centre.x - h * 0.25f, centre.y + h), colour);
            draw->AddRectFilled(ImVec2(centre.x + h * 0.25f, centre.y - h),
                                ImVec2(centre.x + h, centre.y + h), colour);
            break;
        case Icon::Stop:
            draw->AddRectFilled(ImVec2(centre.x - h, centre.y - h),
                                ImVec2(centre.x + h, centre.y + h), colour);
            break;
        case Icon::SkipStart:
            draw->AddRectFilled(ImVec2(centre.x - h, centre.y - h),
                                ImVec2(centre.x - h + 2.0f, centre.y + h), colour);
            draw->AddTriangleFilled(ImVec2(centre.x + h, centre.y - h),
                                    ImVec2(centre.x - h + 3.0f, centre.y),
                                    ImVec2(centre.x + h, centre.y + h), colour);
            break;
        case Icon::None:
            break;
    }
}

constexpr float kControlHeight = 28.0f;

// An action. `primary` fills it with the accent: one per panel, the thing
// this panel is FOR. `tint` colours the word (a deck's colour says where a
// clip goes). Returns true on the click.
bool button(const char* label, Icon icon = Icon::None, bool primary = false,
            float width = 0.0f, ImU32 tint = kInk, bool enabled = true) {
    const ImVec2 text = label != nullptr && label[0] != '\0'
                            ? ImGui::CalcTextSize(label)
                            : ImVec2(0.0f, 0.0f);
    const float icon_w = icon == Icon::None ? 0.0f : 12.0f;
    const float gap = icon != Icon::None && text.x > 0.0f ? 6.0f : 0.0f;
    const float w = width > 0.0f ? width : text.x + icon_w + gap + 20.0f;
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::PushID(label);
    if (!enabled) ImGui::BeginDisabled();
    const bool pressed = ImGui::InvisibleButton("btn", ImVec2(w, kControlHeight));
    if (!enabled) ImGui::EndDisabled();
    const bool hovered = enabled && ImGui::IsItemHovered();
    const bool held = enabled && ImGui::IsItemActive();
    ImGui::PopID();

    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 corner(origin.x + w, origin.y + kControlHeight);
    const ImU32 fill = primary ? (held ? kAmber : kAccent) : (held || hovered ? kHair : kWell);
    draw->AddRectFilled(origin, corner, fill, 1.0f);
    draw->AddRect(origin, corner, primary ? kAccent : kHair, 1.0f);
    const ImU32 ink = !enabled ? kFaint : primary ? kGround : tint;
    float x = origin.x + (w - text.x - icon_w - gap) * 0.5f;
    if (icon != Icon::None) {
        draw_icon(draw, icon, ImVec2(x + icon_w * 0.5f, origin.y + kControlHeight * 0.5f),
                  11.0f, ink);
        x += icon_w + gap;
    }
    if (text.x > 0.0f) {
        draw->AddText(ImVec2(x, origin.y + (kControlHeight - text.y) * 0.5f), ink, label);
    }
    return pressed;
}

// An exclusive choice. One block, the chosen option filled with the accent.
// `enabled[i]` false greys an option out and refuses it -- "Platine" with no
// turntable on the desk. Returns true when `value` changed.
bool segmented(const char* id, const char* const* labels, int count, int& value,
               const bool* enabled = nullptr, ImU32 accent = kAccent,
               bool vertical = false, float fixed_width = 0.0f) {
    ImGui::PushID(id);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    bool changed = false;
    float x = origin.x;
    float y = origin.y;
    float widest = fixed_width;
    if (vertical && widest <= 0.0f) {
        for (int i = 0; i < count; ++i) {
            widest = std::max(widest, ImGui::CalcTextSize(labels[i]).x + 20.0f);
        }
    }
    for (int i = 0; i < count; ++i) {
        const bool on = value == i;
        const bool ok = enabled == nullptr || enabled[i];
        const ImVec2 text = ImGui::CalcTextSize(labels[i]);
        const float w = vertical ? widest : fixed_width > 0.0f ? fixed_width : text.x + 20.0f;
        ImGui::SetCursorScreenPos(ImVec2(x, y));
        ImGui::PushID(i);
        if (!ok) ImGui::BeginDisabled();
        if (ImGui::InvisibleButton("opt", ImVec2(w, kControlHeight)) && !on) {
            value = i;
            changed = true;
        }
        if (!ok) ImGui::EndDisabled();
        const bool hovered = ok && ImGui::IsItemHovered();
        ImGui::PopID();
        const ImVec2 corner(x + w, y + kControlHeight);
        draw->AddRectFilled(ImVec2(x, y), corner, on ? accent : hovered ? kHair : kWell);
        if (i > 0) {
            if (vertical) draw->AddLine(ImVec2(x, y), ImVec2(corner.x, y), kHair);
            else draw->AddLine(ImVec2(x, y), ImVec2(x, corner.y), kHair);
        }
        draw->AddText(ImVec2(x + (vertical ? (w - text.x) * 0.5f : 10.0f),
                             y + (kControlHeight - text.y) * 0.5f),
                      on ? kGround : !ok ? kFaint : kMuted, labels[i]);
        if (vertical) y += kControlHeight; else x += w;
    }
    const ImVec2 extent = vertical ? ImVec2(origin.x + widest, y) : ImVec2(x, origin.y + kControlHeight);
    draw->AddRect(origin, extent, kHair, 1.0f);
    ImGui::SetCursorScreenPos(origin);
    ImGui::Dummy(ImVec2(extent.x - origin.x, extent.y - origin.y));
    ImGui::PopID();
    return changed;
}

// A state read at a glance: a dot, and the word beside it. The figures behind
// it belong in a drawer or a tooltip, not on the bar.
void light(ImU32 colour, const char* label) {
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 at = ImGui::GetCursorScreenPos();
    const float line = ImGui::GetTextLineHeight();
    draw->AddCircleFilled(ImVec2(at.x + 4.0f, at.y + line * 0.5f), 4.0f, colour);
    ImGui::Dummy(ImVec2(8.0f, line));
    ImGui::SameLine(0.0f, 8.0f);
    push_small();
    ImGui::PushStyleColor(ImGuiCol_Text, rgba(kMuted));
    ImGui::TextUnformatted(label);
    ImGui::PopStyleColor();
    pop_font();
}

// An on/off. A dot, sage when on, and the word. Returns true on the click.
bool toggle(const char* label, bool on) {
    const ImVec2 text = ImGui::CalcTextSize(label);
    const float w = 10.0f + 8.0f + text.x;
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::PushID(label);
    const bool pressed = ImGui::InvisibleButton("tog", ImVec2(w, kControlHeight));
    ImGui::PopID();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 dot(origin.x + 5.0f, origin.y + kControlHeight * 0.5f);
    draw->AddCircleFilled(dot, 5.0f, on ? kSage : kHair);
    draw->AddCircle(dot, 5.0f, on ? kSage : kFaint);
    draw->AddText(ImVec2(origin.x + 18.0f, origin.y + (kControlHeight - text.y) * 0.5f),
                  on ? kInk : kMuted, label);
    return pressed;
}

// An eyebrow sitting on the same row as a 28 px control, vertically centred.
void row_label(const char* text) {
    push_small();
    const float dy = (kControlHeight - ImGui::GetTextLineHeight()) * 0.5f;
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + dy);
    ImGui::PushStyleColor(ImGuiCol_Text, rgba(kFaint));
    ImGui::TextUnformatted(text);
    ImGui::PopStyleColor();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - dy);
    pop_font();
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
    const char* waiting = "aucune image \xE2\x80\x94 choisir un clip dans la biblioth\xC3\xA8que";
    const ImVec2 size = ImGui::CalcTextSize(waiting);
    draw->AddText(ImVec2(origin.x + (width - size.x) * 0.5f,
                         origin.y + height * 0.5f - size.y),
                  kFaint, waiting);

    if (deck.clip.frame_count > 0) {
        char detail[96];
        std::snprintf(detail, sizeof(detail), "%ux%u  %s", deck.clip.width, deck.clip.height,
                      deck.clip.is_equirect() ? "equirect 360" : "plan 2D");
        const ImVec2 detail_size = ImGui::CalcTextSize(detail);
        draw->AddText(ImVec2(origin.x + (width - detail_size.x) * 0.5f,
                             origin.y + height * 0.5f + 4.0f),
                      kHair, detail);
    }
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

    // What is in video memory: the part a scratch reaches with no load. The
    // old label called this the VRAM window; the strip now shows it and the
    // tooltip explains it, because the question it answers -- how far can the
    // platter be thrown -- is a question about a place on the clip.
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

    draw->AddRect(origin, ImVec2(origin.x + width, origin.y + height), kHair);

    // The handle. An InvisibleButton over the strip we have just drawn, so the
    // drawing stays declarative and only the interaction is stateful.
    ImGui::SetCursorScreenPos(origin);
    ImGui::PushID(&deck);
    ImGui::InvisibleButton("scrub", ImVec2(width, height));
    const bool hovered = ImGui::IsItemHovered();
    const bool loaded = frames > 0;
    if (hovered && loaded) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

    // The deck, not the clock: the deck remembers where the position came
    // from, so letting go returns there. And `frame.scrubbing` is asserted
    // every frame the hand is on -- the front end hands back any Hand deck
    // nobody claimed, so forgetting this line froze every scrub on release.
    if (loaded && ImGui::IsItemActivated()) {
        deck.grab(position_under(deck, origin.x, width), frame.elapsed_s);
        frame.scrubbing = &deck;
    } else if (loaded && ImGui::IsItemActive()) {
        deck.scrub(position_under(deck, origin.x, width), frame.elapsed_s);
        frame.scrubbing = &deck;
    } else if (loaded && ImGui::IsItemDeactivated()) {
        deck.release(frame.elapsed_s);
    }
    ImGui::PopID();

    // The playhead, with a handle you can see is a handle.
    const float head = x_of(deck.clip.frame_at(deck.played.position_s));
    draw->AddLine(ImVec2(head, origin.y), ImVec2(head, origin.y + height), kInk, 2.0f);
    draw->AddCircleFilled(ImVec2(head, origin.y + height * 0.5f), 5.0f, kInk);

    if (deck.clock.source() == DeckSource::Hand) {
        draw->AddRect(origin, ImVec2(origin.x + width, origin.y + height), kAccent, 0.0f,
                      0, 2.0f);
    } else if (hovered && loaded) {
        // Where a click would land, in time, so a cue can be aimed.
        const float mx = std::clamp(ImGui::GetIO().MousePos.x, origin.x, origin.x + width);
        draw->AddLine(ImVec2(mx, origin.y), ImVec2(mx, origin.y + height), kMuted);
        const std::string at = clock_of(position_under(deck, origin.x, width));
        if (g_fonts.mono != nullptr && g_fonts.small != nullptr) {
            ImGui::PushFont(g_fonts.mono, g_fonts.small->LegacySize);
        }
        const ImVec2 size = ImGui::CalcTextSize(at.c_str());
        const float tx = std::clamp(mx - size.x * 0.5f, origin.x, origin.x + width - size.x);
        draw->AddRectFilled(ImVec2(tx - 4.0f, origin.y - size.y - 6.0f),
                            ImVec2(tx + size.x + 4.0f, origin.y - 2.0f), kPanel);
        draw->AddText(ImVec2(tx, origin.y - size.y - 4.0f), kInk, at.c_str());
        if (g_fonts.mono != nullptr && g_fonts.small != nullptr) ImGui::PopFont();
        draw->AddRect(origin, ImVec2(origin.x + width, origin.y + height), kMuted);
    }

    // What is in memory, said in a corner of the strip.
    if (loaded && !resident.empty()) {
        push_small();
        char memo[48];
        const double span_s = deck.window.window_seconds(deck.clip.frame_duration_s());
        if (resident.count >= frames) {
            std::snprintf(memo, sizeof(memo), "en m\xC3\xA9moire : tout le clip");
        } else {
            std::snprintf(memo, sizeof(memo), "en m\xC3\xA9moire : %.1f s", span_s);
        }
        const ImVec2 size = ImGui::CalcTextSize(memo);
        draw->AddText(ImVec2(origin.x + width - size.x - 6.0f, origin.y + 3.0f), kSlate, memo);
        pop_font();
    }
    if (hovered && loaded && !ImGui::IsItemActive()) {
        ImGui::SetTooltip("glisser pour tenir le clip \xC3\xA0 la main\n"
                          "bande bleut\xC3\xA9""e : ce qui est en m\xC3\xA9moire vid\xC3\xA9o, "
                          "donc scratchable sans chargement");
    }
}

// ---------------------------------------------------------------------------

void draw_status(Engine& engine, Frame& frame) {
    ImGui::BeginChild("status", ImVec2(0.0f, 40.0f), ImGuiChildFlags_None,
                      ImGuiWindowFlags_NoScrollbar);
    ImGui::Dummy(ImVec2(0.0f, 2.0f));
    text_c(kAccent, "scratchvj");

    // The platter link, as a light. Sage: locked and confident on every deck
    // that follows a platter. Amber: a platter is read but not locked, or its
    // confidence is low. Alert: a link that WAS there dropped. Hair: nothing is
    // plugged in -- said as such, never as a figure the demo would invent.
    ImGui::SameLine(0.0f, 22.0f);
    {
        ImU32 colour = kHair;
        const char* word = "aucune platine";
        if (frame.platter_connected || frame.script_running) {
            const TimecodeState& a = engine.deck_a().timecode.state();
            const TimecodeState& b = engine.deck_b().timecode.state();
            const bool any_lost = a.link == LinkState::Lost || b.link == LinkState::Lost;
            const bool weak = a.confidence < 0.6f && engine.deck_a().clock.source() == DeckSource::Timecode;
            colour = any_lost ? kAlert : (weak || !frame.platter_locked) && !frame.script_running ? kAmber : kSage;
            word = frame.script_running ? "Phase \xC2\xB7 script" : "Phase";
        }
        light(colour, word);
        if (ImGui::IsItemHovered() && (frame.platter_connected || frame.script_running)) {
            const TimecodeState& a = engine.deck_a().timecode.state();
            const TimecodeState& b = engine.deck_b().timecode.state();
            ImGui::SetTooltip("A %s \xC2\xB7 confiance %.0f %%\nB %s \xC2\xB7 confiance %.0f %%%s%s",
                              link_text(a.link), static_cast<double>(a.confidence) * 100.0,
                              link_text(b.link), static_cast<double>(b.confidence) * 100.0,
                              frame.platter_endpoint.empty() ? "" : "\n",
                              frame.platter_endpoint.c_str());
        }
    }

    // The desk: which devices answer.
    ImGui::SameLine(0.0f, 22.0f);
    {
        std::string names;
        std::size_t connected = 0;
        for (const Frame::RigDeviceView& d : frame.rig) {
            if (!d.connected) continue;
            ++connected;
            if (!names.empty()) names += " \xC2\xB7 ";
            names += d.profile != nullptr ? d.profile->display_name : d.profile_name;
            if (d.profile != nullptr && d.profile->name == "rp8000") {
                names += d.deck == 'a' ? " A" : " B";
            }
        }
        const std::string word = connected == 0 ? std::string("Table \xC2\xB7 aucune")
                                                : "Table \xC2\xB7 " + names;
        light(connected == 0 ? kHair : connected < frame.rig.size() ? kAmber : kSage,
              word.c_str());
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%zu appareil%s sur %zu configur\xC3\xA9%s \xC2\xB7 %llu messages MIDI",
                              connected, connected == 1 ? "" : "s", frame.rig.size(),
                              frame.rig.size() == 1 ? "" : "s",
                              static_cast<unsigned long long>(frame.midi_messages));
        }
    }

    ImGui::SameLine(0.0f, 22.0f);
    push_mono();
    text_c(kMuted, "%.1f BPM", engine.bpm());
    pop_font();

    // JOUER's two switches, which F and B also drive.
    ImGui::SameLine(0.0f, 22.0f);
    push_small();
    if (toggle("image seule  F", frame.full_frame)) frame.full_frame = !frame.full_frame;
    ImGui::SameLine(0.0f, 10.0f);
    if (toggle("rail  B", frame.rail_open)) frame.rail_open = !frame.rail_open;
    pop_font();

    // The scripted performance, and a way out of it. Scaffolding, not the
    // product: it only exists when the session was started with --demo.
    if (frame.demo_content) {
        ImGui::SameLine(0.0f, 22.0f);
        if (toggle(frame.script_running ? "D\xC3\x89MO" : "d\xC3\xA9mo fig\xC3\xA9""e",
                   frame.script_running)) {
            frame.script_running = !frame.script_running;
        }
    }

    // What the background is doing to the library, and the last thing that
    // went wrong. A WIN32 application has no console, so a failure that is not
    // on screen is a failure nobody sees.
    push_small();
    if (frame.analysis_busy && frame.analysis_pending > 0) {
        ImGui::SameLine(0.0f, 22.0f);
        text_c(kAmber, "analyse en cours \xC2\xB7 %zu en attente", frame.analysis_pending);
    } else if (frame.analysis_busy) {
        ImGui::SameLine(0.0f, 22.0f);
        text_c(kAmber, "analyse en cours");
    } else if (frame.analysis_pending > 0) {
        ImGui::SameLine(0.0f, 22.0f);
        text_c(kAmber, "%zu \xC3\xA0 analyser", frame.analysis_pending);
    }
    if (!frame.last_error.empty()) {
        ImGui::SameLine(0.0f, 22.0f);
        text_c(kAlert, "%s", frame.last_error.c_str());
    }
    if (!frame.notice.empty()) {
        ImGui::SameLine(0.0f, 22.0f);
        text_c(kSage, "%s", frame.notice.c_str());
    }

    // A take. Red while it runs, with what it has written so far.
    {
        const std::string label = frame.take_recording
                                      ? "REC " + std::to_string(frame.take_records)
                                      : "REC";
        const float w = ImGui::CalcTextSize(label.c_str()).x + 32.0f;
        ImGui::SameLine(0.0f, 22.0f);
        const ImVec2 at = ImGui::GetCursorScreenPos();
        ImGui::PushID("rec");
        const bool pressed = ImGui::InvisibleButton("rec", ImVec2(w, 24.0f));
        const bool hovered = ImGui::IsItemHovered();
        ImGui::PopID();
        ImDrawList* draw = ImGui::GetWindowDrawList();
        draw->AddRectFilled(at, ImVec2(at.x + w, at.y + 24.0f),
                            frame.take_recording ? kAlert : hovered ? kHair : kWell, 1.0f);
        draw->AddRect(at, ImVec2(at.x + w, at.y + 24.0f), frame.take_recording ? kAlert : kAlert, 1.0f);
        // A drawn dot rather than the glyph: the interface font has no U+25CF.
        draw->AddCircleFilled(ImVec2(at.x + 12.0f, at.y + 12.0f), 4.0f,
                              frame.take_recording ? kGround : kAlert);
        draw->AddText(ImVec2(at.x + 22.0f, at.y + (24.0f - ImGui::GetTextLineHeight()) * 0.5f),
                      frame.take_recording ? kGround : kAlert, label.c_str());
        if (pressed) frame.take_toggle_request = true;
        if (hovered) {
            ImGui::SetTooltip(frame.take_recording
                                  ? "%s\nclic : arr\xC3\xAAter"
                                  : "enregistrer une prise : tout ce que la table et les platines "
                                    "font, horodat\xC3\xA9, dans takes/%s",
                              frame.take_recording ? frame.take_name.c_str() : "");
        }
    }

    // Replaying a take: a picker, or the take running with its progress.
    ImGui::SameLine(0.0f, 8.0f);
    push_small();
    if (frame.take_replaying) {
        if (button("\xE2\x96\xA0 relecture", Icon::None, true)) frame.take_replay_stop = true;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s\nclic : arr\xC3\xAAter", frame.take_replay_name.c_str());
        ImGui::SameLine(0.0f, 6.0f);
        meter(frame.take_replay_progress, 60.0f, kAccent, false);
    } else {
        if (button("Relire\xE2\x80\xA6")) {
            frame.take_list_request = true;
            ImGui::OpenPopup("takes");
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("rejouer une prise : la table et les platines telles qu'elles ont \xC3\xA9t\xC3\xA9 enregistr\xC3\xA9""es");
        if (ImGui::BeginPopup("takes")) {
            eyebrow("PRISES");
            if (frame.takes.empty()) dim("aucune prise dans takes/");
            for (const std::string& path : frame.takes) {
                const std::size_t slash = path.find_last_of("/\\");
                const std::string name = slash == std::string::npos ? path : path.substr(slash + 1);
                if (ImGui::Selectable(name.c_str())) frame.take_replay_request = path;
            }
            ImGui::EndPopup();
        }
    }
    pop_font();

    // The outputs, right-aligned by measurement: Spout is open from startup
    // (ui/share), NDI is not built, and the screen the room sees.
    std::string outputs = frame.share_open ? "Spout scratchvj" : "Spout \xE2\x80\x94";
    if (frame.output_open) {
        for (const Frame::DisplayView& display : frame.displays) {
            if (!display.is_output) continue;
            outputs += "   \xE2\x86\x92 " + display.name;
            break;
        }
    }
    const float right_x = ImGui::GetContentRegionMax().x - ImGui::CalcTextSize(outputs.c_str()).x;
    if (right_x > ImGui::GetCursorPosX() + 40.0f) {
        ImGui::SameLine(right_x);
        text_c(frame.share_open ? kSage : kFaint, "%s", outputs.c_str());
    }
    pop_font();

    ImGui::EndChild();
}

// A chip: a small button with the mockup's underline when it is the current
// choice. The one control this interface repeats instead of stock widgets.
bool chip(const char* label, bool selected, ImU32 accent = kAccent) {
    const bool pressed = ImGui::SmallButton(label);
    if (selected) {
        ImDrawList* draw = ImGui::GetWindowDrawList();
        const ImVec2 lo = ImGui::GetItemRectMin();
        const ImVec2 hi = ImGui::GetItemRectMax();
        draw->AddLine(ImVec2(lo.x, hi.y), ImVec2(hi.x, hi.y), accent, 2.0f);
    }
    return pressed;
}

const char* state_text(const ClipEntry& clip) {
    switch (clip.state) {
        case AnalysisState::Ready: return nullptr;  // said by the buttons, not by a word
        case AnalysisState::Queued: return "en attente";
        case AnalysisState::Analysing: return "analyse";
        case AnalysisState::Failed: return "\xC3\xA9" "chec";
        case AnalysisState::Unanalysed:
        default: return "non analys\xC3\xA9";
    }
}


// --- library pieces shared by the rail, the screen and the pads ---------------------

// The clip's picture in a well of the given size, letterboxed. A clip with no
// picture yet (analysing, or an older cache) gets the well and a word.
void thumbnail_well(const Frame& frame, ClipId id, float width, float height,
                    const char* empty_word = nullptr) {
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const ImVec2 corner(origin.x + width, origin.y + height);
    draw->AddRectFilled(origin, corner, kWell);
    void* texture = id != kNoClip && static_cast<std::size_t>(id) < frame.thumbnails.size()
                        ? frame.thumbnails[static_cast<std::size_t>(id)]
                        : nullptr;
    if (texture != nullptr) {
        draw->AddImage(ImTextureRef(reinterpret_cast<ImTextureID>(texture)), origin, corner);
    } else if (empty_word != nullptr) {
        push_small();
        const ImVec2 size = ImGui::CalcTextSize(empty_word);
        draw->AddText(ImVec2(origin.x + (width - size.x) * 0.5f, origin.y + (height - size.y) * 0.5f),
                      kFaint, empty_word);
        pop_font();
    }
    draw->AddRect(origin, corner, kHair);
    ImGui::Dummy(ImVec2(width, height));
}

// The payload a clip travels as, from a library row to a pad bank cell.
constexpr const char* kClipPayload = "SVJ_CLIP";

// Makes the last item a drag source carrying `id`. The tooltip while
// dragging is the clip's name, so the hand knows what it holds.
void clip_drag_source(const Library& library, ClipId id) {
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
        ImGui::SetDragDropPayload(kClipPayload, &id, sizeof(id));
        push_small();
        ImGui::TextUnformatted(library.at(id).name.c_str());
        pop_font();
        ImGui::EndDragDropSource();
    }
}

// Makes the last item a drop target; returns the dropped clip, or kNoClip.
ClipId clip_drop_target() {
    ClipId dropped = kNoClip;
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kClipPayload)) {
            dropped = *static_cast<const ClipId*>(payload->Data);
        }
        ImGui::EndDragDropTarget();
    }
    return dropped;
}

// Where a clip goes: three buttons in the layers' colours, and the queue.
// A DJ loading a clip has already decided which layer, and a "which one?"
// step between the decision and the load is one beat too many.
void send_to_buttons(Engine& engine, Frame& frame, ClipId id, bool compact) {
    const float h = compact ? 24.0f : kControlHeight;
    const auto small_button = [&](const char* label, ImU32 tint) {
        const ImVec2 text = ImGui::CalcTextSize(label);
        const float w = text.x + (compact ? 14.0f : 20.0f);
        const ImVec2 origin = ImGui::GetCursorScreenPos();
        ImGui::PushID(label);
        const bool pressed = ImGui::InvisibleButton("b", ImVec2(w, h));
        const bool hovered = ImGui::IsItemHovered();
        ImGui::PopID();
        ImDrawList* draw = ImGui::GetWindowDrawList();
        draw->AddRectFilled(origin, ImVec2(origin.x + w, origin.y + h), hovered ? kHair : kWell, 1.0f);
        draw->AddRect(origin, ImVec2(origin.x + w, origin.y + h), kHair, 1.0f);
        draw->AddText(ImVec2(origin.x + (w - text.x) * 0.5f, origin.y + (h - text.y) * 0.5f), tint, label);
        return pressed;
    };
    if (compact) push_small();
    if (small_button("Deck A", kAmber)) {
        frame.load_clip = id;
        frame.load_target = DeckTarget::A;
    }
    ImGui::SameLine(0.0f, 4.0f);
    if (small_button("Deck B", kSlate)) {
        frame.load_clip = id;
        frame.load_target = DeckTarget::B;
    }
    ImGui::SameLine(0.0f, 4.0f);
    if (small_button(compact ? "Incr." : "Incrustation", kSage)) {
        frame.load_clip = id;
        frame.load_target = DeckTarget::Overlay;
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("sur la couche d'incrustation, hors crossfader");
    ImGui::SameLine(0.0f, 4.0f);
    // The queue is the performer's running order, not I/O, so the view may
    // write it -- the same rule the overlay layer follows.
    if (small_button(compact ? "+" : "File", kInk)) engine.queue().push(id);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("dans la file d'attente");
    if (compact) pop_font();
}

const char* projection_word(const ClipEntry& clip) {
    return clip.shown_equirect() ? "360\xC2\xB0" : "2D";
}

// Reloads every layer showing `id`, keeping its position: the projection of
// a clip changed under it.
void reload_where_shown(Frame& frame, ClipId id) {
    if (frame.clip_on_a == id) {
        frame.load_clip = id;
        frame.load_target = DeckTarget::A;
        frame.load_keep_position = true;
    } else if (frame.clip_on_b == id) {
        frame.load_clip = id;
        frame.load_target = DeckTarget::B;
        frame.load_keep_position = true;
    } else if (frame.clip_on_overlay == id) {
        frame.load_clip = id;
        frame.load_target = DeckTarget::Overlay;
        frame.load_keep_position = true;
    }
}

// A short name for a pad: the stem, cut to what fits a 44 px cell.
std::string pad_word(const std::string& name) {
    std::string stem = name;
    const std::size_t dot = stem.rfind('.');
    if (dot != std::string::npos && dot > 0) stem.resize(dot);
    if (stem.size() > 7) stem.resize(7);
    return stem;
}

void draw_library(Engine& engine, Frame& frame, float width, float height) {
    ImGui::BeginChild("library", ImVec2(width, height), ImGuiChildFlags_Borders);
    const float inner = ImGui::GetContentRegionAvail().x;
    eyebrow("BIBLIOTH\xC3\x88QUE");
    ImGui::SameLine(0.0f, 8.0f);
    push_small();
    dim("B replie");
    pop_font();
    ImGui::Dummy(ImVec2(0.0f, 2.0f));
    push_small();
    if (button("Importer\xE2\x80\xA6")) frame.import_files_request = true;
    ImGui::SameLine(0.0f, 4.0f);
    if (button("\xC3\x89""cran")) {
        frame.screen_request = Screen::Library;
        frame.screen_requested = true;
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("l'\xC3\xA9""cran BIBLIOTH\xC3\x88QUE : dossiers, caisses, banques de pads");
    ImGui::SameLine(0.0f, 8.0f);
    dim("ou glisser ici");
    pop_font();

    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputTextWithHint("##search", "chercher\xE2\x80\xA6", frame.library_search,
                             sizeof(frame.library_search));

    Library& library = engine.library();

    // Crates as small buttons; "Tous" is not a crate but the absence of a
    // filter, so it is drawn rather than stored.
    ImGui::Dummy(ImVec2(0.0f, 2.0f));
    push_small();
    char label[64];
    std::snprintf(label, sizeof(label), "Tous %d", static_cast<int>(library.size()));
    if (chip(label, frame.library_crate < 0)) frame.library_crate = -1;
    for (int crate = 0; crate < library.crate_count(); ++crate) {
        ImGui::SameLine(0.0f, 6.0f);
        ImGui::PushID(crate);
        std::snprintf(label, sizeof(label), "%s %d", library.crate_name(crate).c_str(),
                      static_cast<int>(library.crate_clips(crate).size()));
        if (chip(label, frame.library_crate == crate)) frame.library_crate = crate;
        ImGui::PopID();
    }
    pop_font();

    ImGui::Dummy(ImVec2(0.0f, 6.0f));
    ImGui::Separator();

    // The list: what matches the search, inside the chosen crate.
    const std::vector<ClipId> matches = library.search(frame.library_search);
    const float queue_h = 150.0f;
    ImGui::BeginChild("clips", ImVec2(0.0f, std::max(80.0f, ImGui::GetContentRegionAvail().y -
                                                                 queue_h)),
                      ImGuiChildFlags_None);
    int shown = 0;
    for (const ClipId id : matches) {
        if (frame.library_crate >= 0) {
            const auto& members = library.crate_clips(frame.library_crate);
            if (std::find(members.begin(), members.end(), id) == members.end()) continue;
        }
        ++shown;
        ClipEntry* clip = library.mutable_at(id);
        if (clip == nullptr) continue;
        ImGui::PushID(id);
        if (frame.library_cursor == id) {
            // The row a browse encoder or a mapped "next" is standing on.
            ImDrawList* draw = ImGui::GetWindowDrawList();
            const ImVec2 at = ImGui::GetCursorScreenPos();
            draw->AddRectFilled(ImVec2(at.x - 4.0f, at.y - 1.0f),
                                ImVec2(at.x - 2.0f, at.y + 30.0f), kAccent);
        }

        // The picture, then two lines: the name, and what it is. An unanalysed
        // clip is drawn as not loadable -- loading one would mean real-time
        // decoding, the thing this whole design refuses to do.
        ImGui::BeginGroup();
        thumbnail_well(frame, id, 48.0f, 27.0f);
        ImGui::EndGroup();
        if (clip->playable()) clip_drag_source(library, id);
        ImGui::SameLine(0.0f, 8.0f);
        ImGui::BeginGroup();
        push_mono();
        text_c(clip->playable() ? kInk : clip->state == AnalysisState::Failed ? kAlert : kFaint,
               "%s", clip->name.c_str());
        pop_font();
        push_small();
        ImGui::PushStyleColor(ImGuiCol_Text, rgba(kFaint));
        if (clip->state == AnalysisState::Analysing) {
            ImGui::Text("analyse %.0f %%", static_cast<double>(clip->progress) * 100.0);
            ImGui::SameLine(0.0f, 6.0f);
            meter(clip->progress, 50.0f, kAmber, false);
        } else if (clip->playable()) {
            ImGui::Text("%s \xC2\xB7 %ux%u \xC2\xB7 ", short_clock(clip->duration_s).c_str(),
                        clip->width, clip->height);
            ImGui::SameLine(0.0f, 0.0f);
            text_c(clip->shown_equirect() ? kSlate : kFaint, "%s%s", projection_word(*clip),
                   clip->has_alpha ? " \xC2\xB7 alpha" : "");
        } else {
            ImGui::Text("%s", state_text(*clip));
        }
        ImGui::PopStyleColor();
        pop_font();
        ImGui::EndGroup();

        if (clip->playable()) {
            send_to_buttons(engine, frame, id, true);
        } else if ((clip->state == AnalysisState::Unanalysed ||
                    clip->state == AnalysisState::Failed) &&
                   !clip->source_path.empty()) {
            push_small();
            if (button(clip->state == AnalysisState::Failed ? "r\xC3\xA9""essayer" : "analyser")) {
                frame.analyse_clip = id;
            }
            pop_font();
        }
        ImGui::Dummy(ImVec2(0.0f, 4.0f));
        ImGui::PopID();
    }
    if (shown == 0) {
        push_small();
        dim(library.size() == 0 ? "vide \xE2\x80\x94 glisser des vid\xC3\xA9os ici, ou Importer\xE2\x80\xA6"
                                : "rien ne correspond");
        pop_font();
    }
    ImGui::EndChild();

    ImGui::Separator();
    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    eyebrow("FILE D'ATTENTE");
    if (!engine.queue().empty()) {
        ImGui::SameLine(inner - 100.0f);
        push_small();
        if (button("Suivant \xE2\x86\x92", Icon::None, true)) frame.load_next_request = true;
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("charge le premier de la file (sur A si rien n'est dit)");
        }
        pop_font();
    }
    ImGui::Dummy(ImVec2(0.0f, 2.0f));

    ImGui::BeginChild("queue", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None);
    for (std::size_t i = 0; i < engine.queue().size(); ++i) {
        const QueueItem& item = engine.queue().at(i);
        const ClipEntry& clip = library.at(item.clip);
        ImGui::PushID(1000 + static_cast<int>(i));
        push_mono();
        text_c(kFaint, "%d", static_cast<int>(i + 1));
        pop_font();
        ImGui::SameLine(0.0f, 8.0f);
        text_c(kInk, "%s", clip.name.c_str());
        push_small();
        ImGui::SameLine(0.0f, 8.0f);
        // The target as a cycling chip: none, A, B, O.
        const char* target = item.target == DeckTarget::A       ? "\xE2\x86\x92 A"
                             : item.target == DeckTarget::B     ? "\xE2\x86\x92 B"
                             : item.target == DeckTarget::Overlay ? "\xE2\x86\x92 Incr."
                                                                  : "\xE2\x86\x92 ?";
        ImGui::PushStyleColor(ImGuiCol_Text, rgba(item.target == DeckTarget::None ? kFaint : kAccent));
        if (ImGui::SmallButton(target)) {
            const DeckTarget next = item.target == DeckTarget::None ? DeckTarget::A
                                    : item.target == DeckTarget::A  ? DeckTarget::B
                                    : item.target == DeckTarget::B  ? DeckTarget::Overlay
                                                                    : DeckTarget::None;
            engine.queue().set_target(i, next);
        }
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("o\xC3\xB9 il ira \xC2\xB7 clic pour changer");
        ImGui::SameLine(0.0f, 8.0f);
        if (ImGui::SmallButton("\xC3\x97")) engine.queue().remove(i);
        pop_font();
        ImGui::PopID();
    }
    ImGui::EndChild();

    ImGui::EndChild();
}

// --- BIBLIOTHÈQUE: the preparation screen -----------------------------------------------
//
// Three columns -- where the clips come from, the clips, one clip -- and the
// pad banks underneath. Everything the rail does, with room: crates are
// made here, a clip's projection is decided here, and a clip is put on a pad
// here, by dragging it.

void draw_bank_cell(Engine& engine, Frame& frame, int bank, DeckTarget target, int pad,
                    ImU32 accent) {
    Library& library = engine.library();
    const ClipId id = engine.matrix().at(bank, target, pad);
    const float size = 60.0f;
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::PushID(pad);
    ImGui::InvisibleButton("cell", ImVec2(size, size));
    const bool hovered = ImGui::IsItemHovered();
    const bool clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
    const bool cleared = ImGui::IsItemClicked(ImGuiMouseButton_Right);
    if (id != kNoClip) clip_drag_source(library, id);
    const ClipId dropped = clip_drop_target();
    ImGui::PopID();

    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 corner(origin.x + size, origin.y + size);
    const bool holding = ImGui::GetDragDropPayload() != nullptr;
    draw->AddRectFilled(origin, corner, holding && hovered ? IM_COL32(0xC9, 0x76, 0x2F, 0x30) : kWell, 2.0f);
    draw->AddRect(origin, corner, holding ? kAccent : id != kNoClip ? accent : kHair, 2.0f,
                  0, holding && hovered ? 2.0f : 1.0f);
    if (id != kNoClip) {
        void* texture = static_cast<std::size_t>(id) < frame.thumbnails.size()
                            ? frame.thumbnails[static_cast<std::size_t>(id)]
                            : nullptr;
        if (texture != nullptr) {
            draw->AddImage(ImTextureRef(reinterpret_cast<ImTextureID>(texture)),
                           ImVec2(origin.x + 4.0f, origin.y + 4.0f),
                           ImVec2(corner.x - 4.0f, origin.y + 26.0f));
        }
    }
    push_small();
    char number[4];
    std::snprintf(number, sizeof(number), "%d", pad + 1);
    draw->AddText(ImVec2(origin.x + (size - ImGui::CalcTextSize(number).x) * 0.5f, origin.y + 28.0f),
                  id != kNoClip ? accent : kFaint, number);
    if (id != kNoClip) {
        const std::string word = pad_word(library.at(id).name);
        draw->AddText(ImVec2(origin.x + (size - ImGui::CalcTextSize(word.c_str()).x) * 0.5f,
                             origin.y + 42.0f),
                      kMuted, word.c_str());
    }
    pop_font();

    if (dropped != kNoClip) {
        engine.matrix().set(bank, target, pad, dropped);
        frame.library_dirty = true;
    } else if (cleared && id != kNoClip) {
        engine.matrix().set(bank, target, pad, kNoClip);
        frame.library_dirty = true;
    } else if (clicked && id != kNoClip && !holding) {
        frame.load_clip = id;
        frame.load_target = target;
    }
    if (hovered && !holding) {
        ImGui::SetTooltip(id != kNoClip ? "%s\nclic : charger \xC2\xB7 clic droit : vider"
                                        : "%sd\xC3\xA9poser un clip ici",
                          id != kNoClip ? library.at(id).name.c_str() : "");
    }
}

void draw_pad_banks(Engine& engine, Frame& frame, float height) {
    ImGui::BeginChild("banks", ImVec2(0.0f, height), ImGuiChildFlags_Borders);
    Matrix& matrix = engine.matrix();

    eyebrow("BANQUES DE PADS");
    ImGui::SameLine(0.0f, 14.0f);
    {
        std::vector<std::string> names;
        std::vector<const char*> labels;
        for (int b = 0; b < matrix.bank_count(); ++b) names.push_back(matrix.bank(b).name);
        for (const std::string& n : names) labels.push_back(n.c_str());
        int current = matrix.current();
        if (segmented("bank", labels.data(), static_cast<int>(labels.size()), current)) {
            matrix.select(current);
            frame.library_dirty = true;
        }
    }
    ImGui::SameLine(0.0f, 6.0f);
    if (button("+")) {
        matrix.select(matrix.add_bank(""));
        frame.library_dirty = true;
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("une banque de plus");
    ImGui::SameLine(0.0f, 14.0f);
    row_label("ce que le mode Clips des pads d\xC3\xA9""clenche \xC2\xB7 glisser un clip dans une case");
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 200.0f);
    if (button("Vider la banque")) {
        matrix.clear_bank(matrix.current());
        frame.library_dirty = true;
    }
    ImGui::SameLine(0.0f, 6.0f);
    if (matrix.bank_count() > 1 && button("Supprimer")) {
        matrix.remove_bank(matrix.current());
        frame.library_dirty = true;
    }

    ImGui::Dummy(ImVec2(0.0f, 6.0f));
    const int bank = matrix.current();
    const struct { DeckTarget target; const char* title; ImU32 accent; } rows[] = {
        {DeckTarget::A, "Deck A", kAmber},
        {DeckTarget::B, "Deck B", kSlate},
        {DeckTarget::Overlay, "Incrustation", kSage},
    };
    bool first = true;
    for (const auto& row : rows) {
        if (!first) ImGui::SameLine(0.0f, 30.0f);
        first = false;
        ImGui::BeginGroup();
        push_small();
        text_c(row.accent, "%s", row.title);
        pop_font();
        ImGui::Dummy(ImVec2(0.0f, 2.0f));
        ImGui::PushID(static_cast<int>(row.target));
        for (int pad = 0; pad < kPadCount; ++pad) {
            if (pad > 0) ImGui::SameLine(0.0f, 6.0f);
            draw_bank_cell(engine, frame, bank, row.target, pad, row.accent);
        }
        ImGui::PopID();
        ImGui::EndGroup();
    }
    ImGui::EndChild();
}

void draw_library_screen(Engine& engine, Frame& frame) {
    Library& library = engine.library();

    // --- the toolbar --------------------------------------------------------------------
    ImGui::SetNextItemWidth(260.0f);
    ImGui::InputTextWithHint("##search", "chercher un clip\xE2\x80\xA6", frame.library_search,
                             sizeof(frame.library_search));
    ImGui::SameLine(0.0f, 12.0f);
    static const char* const kFilters[] = {"Tous", "2D", "360\xC2\xB0", "Alpha"};
    segmented("filter", kFilters, 4, frame.library_filter);
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 420.0f);
    if (button("Importer\xE2\x80\xA6")) frame.import_files_request = true;
    ImGui::SameLine(0.0f, 6.0f);
    if (button("Dossier\xE2\x80\xA6")) frame.import_folder_request = true;
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("importe ce qu'un dossier contient, une fois");
    ImGui::SameLine(0.0f, 6.0f);
    if (button("Tout analyser")) frame.analyse_all_request = true;
    ImGui::SameLine(0.0f, 6.0f);
    if (button("Rescanner")) frame.rescan_request = true;
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("relit les dossiers surveill\xC3\xA9s (R\xC3\x89GLAGES)");
    ImGui::Dummy(ImVec2(0.0f, 6.0f));

    const float banks_h = 200.0f;
    const float gap = ImGui::GetStyle().ItemSpacing.x;
    const float body_h = std::max(240.0f, ImGui::GetContentRegionAvail().y - banks_h - gap);
    const float left_w = 220.0f;
    const float right_w = 320.0f;
    const float list_w = std::max(300.0f, ImGui::GetContentRegionAvail().x - left_w - right_w - 2.0f * gap);

    // --- folders and crates --------------------------------------------------------------
    ImGui::BeginChild("sources", ImVec2(left_w, body_h), ImGuiChildFlags_Borders);
    eyebrow("DOSSIERS SURVEILL\xC3\x89S");
    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    if (frame.settings != nullptr) {
        for (const std::string& folder : frame.settings->library_folders) {
            int count = 0;
            for (std::size_t i = 0; i < library.size(); ++i) {
                if (library.at(static_cast<ClipId>(i)).source_path.rfind(folder, 0) == 0) ++count;
            }
            push_small();
            text_c(kMuted, "%s", folder.c_str());
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20.0f);
            text_c(kFaint, "%d", count);
            pop_font();
        }
    }
    push_small();
    if (button("+ dossier")) frame.add_folder_request = true;
    pop_font();

    ImGui::Dummy(ImVec2(0.0f, 12.0f));
    eyebrow("CAISSES");
    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    {
        const auto crate_row = [&](const char* name, int count, bool on, int crate) {
            ImGui::PushID(crate + 1);
            const ImVec2 origin = ImGui::GetCursorScreenPos();
            const float w = ImGui::GetContentRegionAvail().x;
            const bool pressed = ImGui::InvisibleButton("crate", ImVec2(w, 26.0f));
            ImDrawList* draw = ImGui::GetWindowDrawList();
            if (on) draw->AddRectFilled(origin, ImVec2(origin.x + w, origin.y + 26.0f), kPanel);
            draw->AddText(ImVec2(origin.x + 8.0f, origin.y + 5.0f), on ? kInk : kMuted, name);
            char n[16];
            std::snprintf(n, sizeof(n), "%d", count);
            push_small();
            draw->AddText(ImVec2(origin.x + w - ImGui::CalcTextSize(n).x - 8.0f, origin.y + 7.0f), kFaint, n);
            pop_font();
            // A clip dropped on a crate joins it.
            const ClipId dropped = clip_drop_target();
            if (dropped != kNoClip && crate >= 0 && library.add_to_crate(crate, dropped)) {
                frame.library_dirty = true;
            }
            ImGui::PopID();
            return pressed;
        };
        if (crate_row("Tous", static_cast<int>(library.size()), frame.library_crate < 0, -1)) {
            frame.library_crate = -1;
        }
        for (int crate = 0; crate < library.crate_count(); ++crate) {
            if (crate_row(library.crate_name(crate).c_str(),
                          static_cast<int>(library.crate_clips(crate).size()),
                          frame.library_crate == crate, crate)) {
                frame.library_crate = crate;
            }
        }
        static char new_crate[48] = "";
        push_small();
        if (button("+ nouvelle caisse")) ImGui::OpenPopup("newcrate");
        pop_font();
        if (ImGui::BeginPopup("newcrate")) {
            ImGui::SetNextItemWidth(200.0f);
            const bool entered = ImGui::InputTextWithHint("##name", "nom de la caisse", new_crate,
                                                          sizeof(new_crate),
                                                          ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::SameLine();
            if ((button("Cr\xC3\xA9""er", Icon::None, true) || entered) && new_crate[0] != '\0') {
                frame.library_crate = library.create_crate(new_crate);
                new_crate[0] = '\0';
                frame.library_dirty = true;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    if (frame.settings != nullptr) {
        ImGui::Dummy(ImVec2(0.0f, 12.0f));
        eyebrow("CACHES");
        ImGui::Dummy(ImVec2(0.0f, 4.0f));
        int ready = 0;
        for (std::size_t i = 0; i < library.size(); ++i) {
            if (library.at(static_cast<ClipId>(i)).playable()) ++ready;
        }
        push_small();
        text_c(kMuted, "%s", frame.settings->cache_dir.empty() ? "\xC3\xA0 c\xC3\xB4t\xC3\xA9 des sources"
                                                                : frame.settings->cache_dir.c_str());
        text_c(kFaint, "%d analys\xC3\xA9%s", ready, ready == 1 ? "" : "s");
        pop_font();
    }
    ImGui::EndChild();

    // --- the list ---------------------------------------------------------------------------
    ImGui::SameLine();
    ImGui::BeginChild("list", ImVec2(list_w, body_h), ImGuiChildFlags_Borders);
    const std::vector<ClipId> matches = library.search(frame.library_search);
    const float row_h = 48.0f;
    const float inner = ImGui::GetContentRegionAvail().x;
    int shown = 0;
    for (const ClipId id : matches) {
        ClipEntry* clip = library.mutable_at(id);
        if (clip == nullptr) continue;
        if (frame.library_crate >= 0) {
            const auto& members = library.crate_clips(frame.library_crate);
            if (std::find(members.begin(), members.end(), id) == members.end()) continue;
        }
        if (frame.library_filter == 1 && clip->shown_equirect()) continue;
        if (frame.library_filter == 2 && !clip->shown_equirect()) continue;
        if (frame.library_filter == 3 && !clip->has_alpha) continue;
        ++shown;

        ImGui::PushID(id);
        const ImVec2 origin = ImGui::GetCursorScreenPos();
        const bool selected = frame.library_selected == id;
        ImGui::SetNextItemAllowOverlap();
        if (ImGui::InvisibleButton("row", ImVec2(inner, row_h))) frame.library_selected = id;
        if (clip->playable()) clip_drag_source(library, id);
        ImDrawList* draw = ImGui::GetWindowDrawList();
        if (selected) {
            draw->AddRectFilled(origin, ImVec2(origin.x + inner, origin.y + row_h), kPanel);
            draw->AddRectFilled(origin, ImVec2(origin.x + 2.0f, origin.y + row_h), kAccent);
        } else if (ImGui::IsItemHovered()) {
            draw->AddRectFilled(origin, ImVec2(origin.x + inner, origin.y + row_h),
                                IM_COL32(0x1A, 0x19, 0x17, 0x80));
        }
        draw->AddLine(ImVec2(origin.x, origin.y + row_h), ImVec2(origin.x + inner, origin.y + row_h),
                      IM_COL32(0x23, 0x22, 0x20, 0xFF));
        if (frame.library_cursor == id) {
            draw->AddRectFilled(ImVec2(origin.x - 4.0f, origin.y), ImVec2(origin.x - 2.0f, origin.y + row_h),
                                kAccent);
        }

        ImGui::SetCursorScreenPos(ImVec2(origin.x + 8.0f, origin.y + 6.0f));
        thumbnail_well(frame, id, 64.0f, 36.0f);
        ImGui::SetCursorScreenPos(ImVec2(origin.x + 84.0f, origin.y + 6.0f));
        ImGui::BeginGroup();
        push_mono();
        text_c(clip->playable() ? kInk : clip->state == AnalysisState::Failed ? kAlert : kFaint,
               "%s", clip->name.c_str());
        pop_font();
        push_small();
        if (clip->state == AnalysisState::Analysing) {
            text_c(kAmber, "analyse %.0f %%", static_cast<double>(clip->progress) * 100.0);
            ImGui::SameLine(0.0f, 8.0f);
            meter(clip->progress, 120.0f, kAmber, false);
        } else if (clip->playable()) {
            ImGui::PushStyleColor(ImGuiCol_Text, rgba(kFaint));
            ImGui::Text("%s \xC2\xB7 %ux%u \xC2\xB7 ", short_clock(clip->duration_s).c_str(),
                        clip->width, clip->height);
            ImGui::PopStyleColor();
            ImGui::SameLine(0.0f, 0.0f);
            text_c(clip->shown_equirect() ? kSlate : kFaint, "%s", projection_word(*clip));
            if (clip->has_alpha) {
                ImGui::SameLine(0.0f, 0.0f);
                text_c(kSage, " \xC2\xB7 alpha");
            }
            if (clip->is_sequence || clip->is_still) {
                ImGui::SameLine(0.0f, 0.0f);
                text_c(kFaint, " \xC2\xB7 %s", clip->is_still ? "image fixe" : "s\xC3\xA9quence");
            }
        } else {
            text_c(kFaint, "%s", state_text(*clip));
        }
        pop_font();
        ImGui::EndGroup();

        // The buttons, at the right end of the row.
        ImGui::SetCursorScreenPos(ImVec2(origin.x + inner - 236.0f, origin.y + 12.0f));
        if (clip->playable()) {
            send_to_buttons(engine, frame, id, true);
        } else if ((clip->state == AnalysisState::Unanalysed ||
                    clip->state == AnalysisState::Failed) &&
                   !clip->source_path.empty()) {
            push_small();
            if (button(clip->state == AnalysisState::Failed ? "r\xC3\xA9""essayer" : "analyser")) {
                frame.analyse_clip = id;
            }
            pop_font();
        }
        ImGui::SetCursorScreenPos(ImVec2(origin.x, origin.y + row_h));
        ImGui::Dummy(ImVec2(0.0f, 1.0f));
        ImGui::PopID();
    }
    if (shown == 0) {
        push_small();
        dim(library.size() == 0 ? "vide \xE2\x80\x94 glisser des vid\xC3\xA9os, des images ou un dossier ici"
                                : "rien ne correspond");
        pop_font();
    }
    // The drop zone, said in words: the window takes files anywhere, but
    // a list that says so is a list nobody has to guess about.
    if (ImGui::GetContentRegionAvail().y > 60.0f) {
        const ImVec2 at = ImGui::GetCursorScreenPos();
        const float h = ImGui::GetContentRegionAvail().y - 8.0f;
        ImDrawList* draw = ImGui::GetWindowDrawList();
        push_small();
        const char* word = "d\xC3\xA9poser des vid\xC3\xA9os, des images ou un dossier ici";
        const ImVec2 size = ImGui::CalcTextSize(word);
        draw->AddText(ImVec2(at.x + (inner - size.x) * 0.5f, at.y + h * 0.5f - size.y * 0.5f), kFaint, word);
        pop_font();
        for (float x = at.x + 8.0f; x < at.x + inner - 8.0f; x += 8.0f) {
            draw->AddLine(ImVec2(x, at.y + 8.0f), ImVec2(std::min(x + 4.0f, at.x + inner - 8.0f), at.y + 8.0f), kHair);
            draw->AddLine(ImVec2(x, at.y + h), ImVec2(std::min(x + 4.0f, at.x + inner - 8.0f), at.y + h), kHair);
        }
        ImGui::Dummy(ImVec2(inner, h));
    }
    ImGui::EndChild();

    // --- the inspector -------------------------------------------------------------------------
    ImGui::SameLine();
    ImGui::BeginChild("inspector", ImVec2(right_w, body_h), ImGuiChildFlags_Borders);
    eyebrow("CLIP S\xC3\x89LECTIONN\xC3\x89");
    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    // Until a row is clicked, the first listed clip: an empty inspector
    // beside a full list teaches nothing.
    ClipId inspected = frame.library_selected;
    if (inspected == kNoClip && !matches.empty()) inspected = matches.front();
    ClipEntry* clip = inspected != kNoClip ? library.mutable_at(inspected) : nullptr;
    if (clip == nullptr) {
        push_small();
        dim("cliquer un clip dans la liste");
        pop_font();
    } else {
        const ClipId id = inspected;
        const float w = ImGui::GetContentRegionAvail().x;
        thumbnail_well(frame, id, w, w * 9.0f / 16.0f, "pas encore d'image");
        ImGui::Dummy(ImVec2(0.0f, 4.0f));
        text_c(kInk, "%s", clip->name.c_str());
        push_small();
        ImGui::PushStyleColor(ImGuiCol_Text, rgba(kFaint));
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + w);
        ImGui::Text("source : %s", clip->source_path.empty() ? "\xE2\x80\x94 (cache orphelin)" : clip->source_path.c_str());
        ImGui::Text("cache : %s", clip->path.c_str());
        ImGui::PopTextWrapPos();
        if (clip->playable()) {
            ImGui::Text("%s \xC2\xB7 %ux%u \xC2\xB7 %.3g i/s%s", short_clock(clip->duration_s).c_str(),
                        clip->width, clip->height, clip->fps, clip->has_alpha ? " \xC2\xB7 alpha" : "");
        } else {
            ImGui::Text("%s", state_text(*clip));
        }
        ImGui::PopStyleColor();
        pop_font();

        // The projection: the one place the word is decided, and the header
        // and the rail read it. Forcing reloads any deck showing the clip.
        ImGui::Dummy(ImVec2(0.0f, 8.0f));
        eyebrow("PROJECTION");
        ImGui::Dummy(ImVec2(0.0f, 2.0f));
        {
            char auto_label[24];
            std::snprintf(auto_label, sizeof(auto_label), "Auto (%s)", clip->equirect ? "360\xC2\xB0" : "2D");
            const char* const labels[] = {auto_label, "2D", "360\xC2\xB0"};
            int which = clip->projection == ProjectionOverride::Auto ? 0
                        : clip->projection == ProjectionOverride::Flat ? 1 : 2;
            if (segmented("projection", labels, 3, which)) {
                clip->projection = which == 0 ? ProjectionOverride::Auto
                                   : which == 1 ? ProjectionOverride::Flat : ProjectionOverride::Equirect;
                frame.library_dirty = true;
                reload_where_shown(frame, id);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("auto : un ratio 2:1 est pris pour une sph\xC3\xA8re.\nForcer recharge les decks qui le jouent.");
            }
        }

        ImGui::Dummy(ImVec2(0.0f, 8.0f));
        eyebrow("CAISSES");
        ImGui::Dummy(ImVec2(0.0f, 2.0f));
        if (library.crate_count() == 0) {
            push_small();
            dim("aucune caisse \xE2\x80\x94 en cr\xC3\xA9""er une \xC3\xA0 gauche");
            pop_font();
        }
        for (int crate = 0; crate < library.crate_count(); ++crate) {
            const auto& members = library.crate_clips(crate);
            const bool in = std::find(members.begin(), members.end(), id) != members.end();
            ImGui::PushID(crate);
            push_small();
            std::string label = in ? library.crate_name(crate) + "  \xC3\x97" : "+ " + library.crate_name(crate);
            if (button(label.c_str(), Icon::None, in)) {
                if (in) library.remove_from_crate(crate, id); else library.add_to_crate(crate, id);
                frame.library_dirty = true;
            }
            pop_font();
            ImGui::PopID();
            if (crate + 1 < library.crate_count()) ImGui::SameLine(0.0f, 4.0f);
        }

        if (clip->playable()) {
            ImGui::Dummy(ImVec2(0.0f, 8.0f));
            eyebrow("ENVOYER VERS");
            ImGui::Dummy(ImVec2(0.0f, 2.0f));
            send_to_buttons(engine, frame, id, false);

            ImGui::Dummy(ImVec2(0.0f, 8.0f));
            eyebrow("SUR UN PAD");
            ImGui::Dummy(ImVec2(0.0f, 2.0f));
            push_small();
            dim("glisser vers une case de banque, ou :");
            pop_font();
            const struct { DeckTarget target; const char* label; ImU32 tint; } cells[] = {
                {DeckTarget::A, "case libre A", kAmber},
                {DeckTarget::B, "B", kSlate},
                {DeckTarget::Overlay, "Incr.", kSage},
            };
            for (const auto& cell : cells) {
                if (cell.target != DeckTarget::A) ImGui::SameLine(0.0f, 4.0f);
                const int free = engine.matrix().first_free(cell.target);
                push_small();
                if (button(cell.label, Icon::None, false, 0.0f, cell.tint, free >= 0)) {
                    engine.matrix().set(engine.matrix().current(), cell.target, free, id);
                    frame.library_dirty = true;
                }
                pop_font();
                if (ImGui::IsItemHovered() && free < 0) ImGui::SetTooltip("la banque est pleine pour cette couche");
            }
        }

        if (!clip->source_path.empty()) {
            ImGui::Dummy(ImVec2(0.0f, 12.0f));
            push_small();
            if (button("R\xC3\xA9-analyser")) frame.analyse_clip = id;
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("refait le cache depuis la source (et sa vignette)");
            pop_font();
        }
    }
    ImGui::EndChild();

    draw_pad_banks(engine, frame, banks_h);
}

// --- RÉGLAGES: settings.json, on screen ----------------------------------------
//
// Every field here used to exist only in the file. A desk that changes --
// another interface, a cable moved, a folder of rushes on a new drive -- is
// configuration, and configuration a performer cannot reach without a text
// editor is configuration that gets left wrong.

// ImGui edits a char buffer; the setting is a std::string. Copy in, edit,
// copy out on change -- a few bytes a frame, and no static buffer that could
// hold a stale value when the settings are reloaded under it.
bool input_string(const char* label, std::string& value, float width = 320.0f) {
    char buffer[512];
    std::snprintf(buffer, sizeof(buffer), "%s", value.c_str());
    ImGui::SetNextItemWidth(width);
    if (ImGui::InputText(label, buffer, sizeof(buffer))) {
        value = buffer;
        return true;
    }
    return false;
}

void draw_settings_screen(Engine& engine, Frame& frame) {
    (void)engine;
    if (frame.settings == nullptr) {
        dim("aucun r\xC3\xA9glage charg\xC3\xA9");
        return;
    }
    DeskSettings& desk = *frame.settings;

    eyebrow("BIBLIOTH\xC3\x88QUE \xE2\x80\x94 les dossiers de rushs");
    push_small();
    ImGui::PushStyleColor(ImGuiCol_Text, rgba(kFaint));
    ImGui::TextUnformatted(
        "Parcourus r\xC3\xA9" "cursivement au lancement. Ce qu'ils contiennent appara\xC3\xAEt dans la "
        "biblioth\xC3\xA8que et s'analyse \xC3\xA0 la demande \xE2\x80\x94 pas tout seul : un dossier "
        "de 4K analys\xC3\xA9 sans qu'on l'ait demand\xC3\xA9, en plein set, saturerait la machine.");
    ImGui::PopStyleColor();
    pop_font();
    ImGui::Dummy(ImVec2(0.0f, 6.0f));
    for (std::size_t i = 0; i < desk.library_folders.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));
        push_mono();
        text_c(kInk, "%s", desk.library_folders[i].c_str());
        pop_font();
        ImGui::SameLine(0.0f, 12.0f);
        push_small();
        if (ImGui::SmallButton("retirer")) {
            desk.library_folders.erase(desk.library_folders.begin() +
                                       static_cast<std::ptrdiff_t>(i));
            frame.settings_dirty = true;
            frame.rescan_request = true;
            pop_font();
            ImGui::PopID();
            break;
        }
        pop_font();
        ImGui::PopID();
    }
    push_small();
    if (ImGui::SmallButton("Ajouter un dossier\xE2\x80\xA6")) frame.add_folder_request = true;
    ImGui::SameLine(0.0f, 8.0f);
    if (ImGui::SmallButton("Rescanner")) frame.rescan_request = true;
    ImGui::SameLine(0.0f, 8.0f);
    if (ImGui::SmallButton("Tout analyser")) frame.analyse_all_request = true;
    pop_font();

    ImGui::Dummy(ImVec2(0.0f, 8.0f));
    if (input_string("dossier des caches (vide = \xC3\xA0 c\xC3\xB4t\xC3\xA9 de la source)",
                     desk.cache_dir)) {
        frame.settings_dirty = true;
    }
    double fps = desk.sequence_fps;
    ImGui::SetNextItemWidth(120.0f);
    if (ImGui::InputDouble("cadence des s\xC3\xA9quences d'images", &fps, 1.0, 5.0, "%.2f")) {
        if (fps > 0.0) {
            desk.sequence_fps = fps;
            frame.settings_dirty = true;
        }
    }
    int budget = static_cast<int>(desk.vram_budget_mb);
    ImGui::SetNextItemWidth(120.0f);
    if (ImGui::InputInt("m\xC3\xA9moire vid\xC3\xA9o par deck (Mio)", &budget, 64, 256)) {
        if (budget >= 16) {
            desk.vram_budget_mb = static_cast<unsigned>(budget);
            frame.settings_dirty = true;
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("ce qu'un deck garde en m\xC3\xA9moire vid\xC3\xA9o : la bande bleut\xC3\xA9""e "
                          "de la barre de position.\nPlus grand, plus loin scratchable sans chargement.");
    }
    if (input_string("machine Unreal (h\xC3\xB4te du flux UDP)", desk.control_host, 200.0f)) {
        frame.settings_dirty = true;
    }
    int udp_port = static_cast<int>(desk.control_port);
    ImGui::SetNextItemWidth(120.0f);
    if (ImGui::InputInt("port du flux UDP", &udp_port, 1, 100)) {
        if (udp_port > 0 && udp_port < 65536) {
            desk.control_port = static_cast<unsigned>(udp_port);
            frame.settings_dirty = true;
        }
    }
    int max_width = static_cast<int>(desk.analysis_max_width);
    ImGui::SetNextItemWidth(120.0f);
    if (ImGui::InputInt("largeur max \xC3\xA0 l'analyse", &max_width, 64, 256)) {
        if (max_width >= 64) {
            desk.analysis_max_width = static_cast<unsigned>(max_width);
            frame.settings_dirty = true;
        }
    }

    ImGui::Dummy(ImVec2(0.0f, 14.0f));
    eyebrow("PLATEAU \xE2\x80\x94 o\xC3\xB9 arrive la porteuse du Phase");
    push_small();
    ImGui::PushStyleColor(ImGuiCol_Text, rgba(kFaint));
    ImGui::TextUnformatted("Mesur\xC3\xA9 sur ce bureau (docs/cablage.md). Pris en compte \xC3\xA0 la "
                           "prochaine ouverture de l'entr\xC3\xA9" "e.");
    ImGui::PopStyleColor();
    pop_font();
    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    if (input_string("entr\xC3\xA9" "e audio (fragment du nom)", desk.platter_endpoint, 220.0f)) {
        frame.settings_dirty = true;
    }
    int first = static_cast<int>(desk.platter_first_channel) + 1;
    ImGui::SetNextItemWidth(100.0f);
    if (ImGui::InputInt("premier canal de la paire", &first)) {
        if (first >= 1) {
            desk.platter_first_channel = static_cast<unsigned>(first - 1);
            frame.settings_dirty = true;
        }
    }
    double carrier = desk.carrier_hz;
    ImGui::SetNextItemWidth(120.0f);
    if (ImGui::InputDouble("porteuse (Hz)", &carrier, 100.0, 500.0, "%.0f")) {
        if (carrier > 0.0) {
            desk.carrier_hz = carrier;
            frame.settings_dirty = true;
        }
    }

    ImGui::Dummy(ImVec2(0.0f, 14.0f));
    eyebrow("MIDI \xE2\x80\x94 le rig");
    push_small();
    ImGui::PushStyleColor(ImGuiCol_Text, rgba(kFaint));
    ImGui::TextUnformatted(
        "Un appareil = un profil (ce qu'il a, comment on le dessine) sur un port. Le num\xC3\xA9ro "
        "d\xC3\xA9partage deux ports au m\xC3\xAAme nom (les deux platines) ; la lettre nomme les "
        "pads d'une platine. Rebranch\xC3\xA9 tout seul quand le port r\xC3\xA9" "appara\xC3\xAEt.");
    ImGui::PopStyleColor();
    pop_font();
    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    for (std::size_t i = 0; i < desk.devices.size(); ++i) {
        DeviceSetting& d = desk.devices[i];
        ImGui::PushID(static_cast<int>(i));
        // Profile, as a combo over what is built in and what profiles/ holds.
        ImGui::SetNextItemWidth(160.0f);
        if (ImGui::BeginCombo("##profile", d.profile.c_str())) {
            for (const std::string& name : frame.profile_names) {
                if (ImGui::Selectable(name.c_str(), name == d.profile)) {
                    d.profile = name;
                    frame.settings_dirty = true;
                    frame.rig_reconfigure_request = true;
                }
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine(0.0f, 8.0f);
        if (input_string("##port", d.port, 150.0f)) {
            frame.settings_dirty = true;
            frame.rig_reconfigure_request = true;
        }
        ImGui::SameLine(0.0f, 8.0f);
        ImGui::SetNextItemWidth(70.0f);
        if (ImGui::InputInt("n\xC2\xB0##ordinal", &d.ordinal)) {
            d.ordinal = std::max(0, d.ordinal);
            frame.settings_dirty = true;
            frame.rig_reconfigure_request = true;
        }
        ImGui::SameLine(0.0f, 8.0f);
        push_small();
        if (chip(d.deck == 'b' ? "deck B" : "deck A", true, d.deck == 'b' ? kSlate : kAmber)) {
            d.deck = d.deck == 'b' ? 'a' : 'b';
            frame.settings_dirty = true;
            frame.rig_reconfigure_request = true;
        }
        ImGui::SameLine(0.0f, 8.0f);
        const bool connected = i < frame.rig.size() && frame.rig[i].connected;
        text_c(connected ? kSage : kFaint, connected ? "connect\xC3\xA9" : "absent");
        ImGui::SameLine(0.0f, 8.0f);
        if (ImGui::SmallButton("retirer")) {
            desk.devices.erase(desk.devices.begin() + static_cast<std::ptrdiff_t>(i));
            frame.settings_dirty = true;
            frame.rig_reconfigure_request = true;
            pop_font();
            ImGui::PopID();
            break;
        }
        pop_font();
        ImGui::PopID();
    }
    push_small();
    if (ImGui::SmallButton("Ajouter un appareil")) {
        desk.devices.push_back(DeviceSetting{"apc40_mk2", "APC40", 0, 'a'});
        frame.settings_dirty = true;
        frame.rig_reconfigure_request = true;
    }
    pop_font();
    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    push_small();
    text_c(kFaint, "ports MIDI vus par la machine :");
    for (const std::string& port : frame.midi_ports) {
        ImGui::SameLine(0.0f, 8.0f);
        text_c(kMuted, "%s", port.c_str());
    }
    if (frame.midi_ports.empty()) {
        ImGui::SameLine(0.0f, 8.0f);
        text_c(kMuted, "aucun");
    }
    pop_font();

    ImGui::Dummy(ImVec2(0.0f, 14.0f));
    push_small();
    dim(frame.settings_dirty ? "settings.json : \xC3\xA9" "criture en cours\xE2\x80\xA6"
                             : "settings.json \xC3\xA0 jour");
    pop_font();
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

// The platter's diagnostics, folded away. Position, velocity, confidence,
// scratch rate and link are what a DVS engineer wants and what a performer
// with no turntable plugged in has no use for: they live in a drawer, opened
// by hand, or by itself when a real platter is driving the deck.
void draw_deck_diagnostics(Deck& deck, Engine& engine, Frame& frame, bool is_a) {
    const TimecodeState& state = deck.timecode.state();
    const bool live = deck.clock.source() == DeckSource::Timecode &&
                      (frame.platter_connected || frame.script_running);
    ImGui::SetNextItemOpen(live, ImGuiCond_Appearing);
    push_small();
    const bool open = ImGui::TreeNodeEx("diagnostic platine", ImGuiTreeNodeFlags_SpanAvailWidth |
                                                                  ImGuiTreeNodeFlags_NoTreePushOnOpen);
    pop_font();
    if (!open) return;

    readout("POSITION", clock_of(deck.played.position_s), nullptr, kInk);
    ImGui::SameLine(0.0f, 22.0f);
    {
        char v[16];
        std::snprintf(v, sizeof(v), "%.2f", deck.played.velocity);
        readout("VITESSE", v, "\xC3\x97",
                std::fabs(deck.played.velocity) > 2.5 ? kAmber : kInk);
    }
    ImGui::SameLine(0.0f, 22.0f);
    {
        char v[16];
        std::snprintf(v, sizeof(v), "%.0f", static_cast<double>(state.confidence) * 100.0);
        readout("CONFIANCE", v, "%", link_colour(state.link));
    }
    ImGui::SameLine(0.0f, 22.0f);
    {
        char v[16];
        std::snprintf(v, sizeof(v), "%.1f", static_cast<double>(deck.gestures.scratch_rate()));
        readout("SCRATCH", v, "/s", kInk);
    }
    ImGui::SameLine(0.0f, 22.0f);
    {
        ImGui::BeginGroup();
        eyebrow("LIAISON");
        text_c(link_colour(state.link), "%s", link_text(state.link));
        ImGui::EndGroup();
    }

    if (is_a) {
        // The audio input the platter is read from, in three states that look
        // alike from the outside: no device, a device with no carrier, and a
        // carrier being tracked.
        push_small();
        if (!frame.deck_a_live) {
            dim("plateau r\xC3\xA9""el : non lu \xE2\x80\x94 choisir « Platine » pour l'\xC3\xA9""couter");
        } else if (!frame.platter_connected) {
            text_c(kAmber, "plateau r\xC3\xA9""el : aucune entr\xC3\xA9""e audio");
        } else if (!frame.platter_locked) {
            text_c(kFaint, "%s \xC2\xB7 silence", frame.platter_endpoint.c_str());
        } else {
            text_c(frame.platter_slews > 0 ? kAmber : kSage, "%s \xC2\xB7 niveau %.2f%s",
                   frame.platter_endpoint.c_str(), static_cast<double>(frame.platter_level),
                   frame.platter_slews > 0 ? " !" : "");
        }
        pop_font();

        // The anchor against Serato: "this position on the record is this
        // position in the clip, now". Freshness, never drift: what is
        // observable is how long the anchor has been down and how many
        // discontinuities went past since; a number of seconds of error
        // would be invented.
        push_small();
        if (button("Ancrer", Icon::None, false, 0.0f, kInk,
                   deck.clock.source() == DeckSource::Timecode)) {
            frame.anchor_request = true;
        }
        pop_font();
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("caler le clip sur Serato : ici sur le disque = ici dans le clip.\n"
                              "\xC3\x80 refaire apr\xC3\xA8s un saut de timecode.");
        }
        ImGui::SameLine(0.0f, 10.0f);
        const int jumps = deck.timecode.jump_count();
        const float stale = engine.anchor().armed()
                                ? engine.anchor().staleness(frame.elapsed_s, jumps)
                                : 1.0f;
        meter(1.0f - stale, 130.0f, stale > 0.6f ? kAmber : kSage, false);
        ImGui::SameLine(0.0f, 10.0f);
        push_small();
        ImGui::PushStyleColor(ImGuiCol_Text, rgba(kFaint));
        ImGui::Text("ancrage : fra\xC3\xAE"
                    "cheur \xC2\xB7 %d saut%s",
                    jumps, jumps == 1 ? "" : "s");
        ImGui::PopStyleColor();
        pop_font();
    }
}

// A deck is a video player. Transport, a position bar you can drag, the
// source the position comes from, what the clip does at its ends -- and the
// turntable is ONE of the sources, offered only when one is there to follow.
void draw_deck(Deck& deck, Engine& engine, Frame& frame, void* texture,
               ImU32 accent, bool is_a, float width, float height) {
    ImGui::PushID(is_a ? "deck.a" : "deck.b");
    ImGui::BeginChild(is_a ? "deckA" : "deckB", ImVec2(width, height), ImGuiChildFlags_Borders);

    const float inner = ImGui::GetContentRegionAvail().x;
    const bool loaded = deck.clip.frame_count > 0;
    const ClipId on_deck = is_a ? frame.clip_on_a : frame.clip_on_b;
    const double now_s = frame.elapsed_s;

    // --- header: letter, name, definition, 2D | 360 ---------------------------
    {
        const float y = ImGui::GetCursorPosY();
        const float dy = (kControlHeight - ImGui::GetTextLineHeight()) * 0.5f;
        ImGui::SetCursorPosY(y + dy);
        push_mono();
        text_c(accent, "%s", is_a ? "A" : "B");
        pop_font();
        ImGui::SameLine(0.0f, 12.0f);
        text_c(loaded ? kInk : kFaint, "%s", loaded ? deck.name.c_str() : "aucun clip");
        if (loaded) {
            ImGui::SameLine(0.0f, 12.0f);
            push_small();
            ImGui::PushStyleColor(ImGuiCol_Text, rgba(kFaint));
            ImGui::Text("%ux%u", deck.clip.width, deck.clip.height);
            ImGui::PopStyleColor();
            pop_font();
        }
        // The projection selector, on the header where the eye is. It writes
        // the library entry and asks for the reload; a clip the library does
        // not know (the demo's) shows its shape as a word instead.
        static const char* const kShapes[] = {"2D", "360\xC2\xB0"};
        const float sel_w = ImGui::CalcTextSize(kShapes[0]).x + ImGui::CalcTextSize(kShapes[1]).x + 40.0f;
        ImGui::SameLine(inner - sel_w);
        ImGui::SetCursorPosY(y);
        if (loaded && on_deck != kNoClip) {
            int shape = deck.clip.is_equirect() ? 1 : 0;
            if (segmented("shape", kShapes, 2, shape)) {
                if (ClipEntry* entry = engine.library().mutable_at(on_deck)) {
                    entry->projection = shape == 1 ? ProjectionOverride::Equirect
                                                   : ProjectionOverride::Flat;
                    frame.library_dirty = true;
                    frame.load_clip = on_deck;
                    frame.load_target = is_a ? DeckTarget::A : DeckTarget::B;
                    frame.load_keep_position = true;
                }
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("comment lire ce clip : image plane, ou sph\xC3\xA8re "
                                  "\xC3\xA9quirectangulaire projet\xC3\xA9""e.\n"
                                  "Retenu dans la biblioth\xC3\xA8que pour ce clip.");
            }
        } else if (loaded) {
            ImGui::SetCursorPosY(y + dy);
            push_small();
            ImGui::PushStyleColor(ImGuiCol_Text, rgba(kFaint));
            ImGui::TextUnformatted(deck.clip.is_equirect() ? kShapes[1] : kShapes[0]);
            ImGui::PopStyleColor();
            pop_font();
        } else {
            ImGui::Dummy(ImVec2(1.0f, kControlHeight));
        }
    }

    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    const bool projected_360 = is_a && deck.clip.is_equirect();
    picture_well(deck, texture,
                 projected_360 ? static_cast<float>(engine.view_a().aspect) : 0.0f, inner,
                 std::max(120.0f, height * 0.26f));

    // --- transport ---------------------------------------------------------------
    ImGui::Dummy(ImVec2(0.0f, 6.0f));
    {
        const DeckSource source = deck.clock.source();
        const bool on_platter = source == DeckSource::Timecode;
        if (button("", Icon::SkipStart, false, kControlHeight, kInk, loaded)) deck.stop(now_s);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("retour au d\xC3\xA9""but");
        ImGui::SameLine(0.0f, 6.0f);
        // One button, two faces: it shows what pressing it DOES. On a platter
        // deck the record decides the motion, so the button offers a pause --
        // the free clock takes over, frozen, and the platter is dead to the
        // deck until « Platine » is chosen again.
        const bool running = loaded && (deck.playing() || on_platter);
        // Space works on the deck under the mouse: in a set nobody aims at a
        // 28 px button with the other hand on a fader.
        const bool space = loaded && ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) &&
                           !ImGui::GetIO().WantTextInput &&
                           ImGui::IsKeyPressed(ImGuiKey_Space, false);
        if (button("", running ? Icon::Pause : Icon::Play, running, 44.0f, kInk, loaded) ||
            space) {
            if (running) deck.pause(now_s); else deck.play(now_s);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(running ? "pause  (espace)" : "lecture  (espace)");
        }
        ImGui::SameLine(0.0f, 6.0f);
        if (button("", Icon::Stop, false, kControlHeight, kInk, loaded)) deck.stop(now_s);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("stop : pause et retour au d\xC3\xA9""but");

        ImGui::SameLine(0.0f, 12.0f);
        {
            const float dy = (kControlHeight - ImGui::GetTextLineHeight()) * 0.5f;
            const float y = ImGui::GetCursorPosY();
            ImGui::SetCursorPosY(y + dy);
            push_mono();
            text_c(loaded ? kInk : kFaint, "%s", clock_of(deck.played.position_s).c_str());
            pop_font();
            ImGui::SameLine(0.0f, 6.0f);
            ImGui::SetCursorPosY(y + dy);
            if (g_fonts.mono != nullptr && g_fonts.small != nullptr) {
                ImGui::PushFont(g_fonts.mono, g_fonts.small->LegacySize);
            }
            text_c(kFaint, "/ %s", short_clock(deck.clip.duration_s()).c_str());
            if (g_fonts.mono != nullptr && g_fonts.small != nullptr) ImGui::PopFont();
            ImGui::SetCursorPosY(y);
        }

        // The rate. A slider on a free deck; a reading on a platter deck,
        // because there the record sets it and a slider would be a lie.
        // Placed from what the row has actually used, so it never lands on
        // the time.
        const float rate_w = 90.0f;
        const float label_w = ImGui::CalcTextSize("VITESSE").x * 0.8f + 8.0f;
        const float used = ImGui::GetItemRectMax().x - ImGui::GetWindowPos().x -
                           ImGui::GetWindowContentRegionMin().x;
        const float rate_x = inner - rate_w;
        if (rate_x - label_w > used + 16.0f) {
            ImGui::SameLine(rate_x - label_w);
            row_label("VITESSE");
            ImGui::SameLine(rate_x);
            if (on_platter || source == DeckSource::Hand) {
                const float dy = (kControlHeight - ImGui::GetTextLineHeight()) * 0.5f;
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + dy);
                push_mono();
                text_c(std::fabs(deck.played.velocity) > 2.5 ? kAmber : kInk, "%+.2f\xC3\x97",
                       deck.played.velocity);
                pop_font();
            } else {
                float rate = static_cast<float>(deck.clock.rate());
                ImGui::SetNextItemWidth(rate_w);
                ImGui::PushID("rate");
                if (ImGui::SliderFloat("", &rate, -2.0f, 2.0f, "%.2f\xC3\x97")) {
                    deck.clock.set_rate(static_cast<double>(rate), now_s);
                    if (rate != 0.0f) deck.resume_rate = static_cast<double>(rate);
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("vitesse de lecture, n\xC3\xA9gative \xC3\xA0 l'envers\n"
                                      "double-clic : 1.00");
                }
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    deck.clock.set_rate(1.0, now_s);
                    deck.resume_rate = 1.0;
                }
                ImGui::PopID();
            }
        }
    }

    // --- the position bar ---------------------------------------------------------
    ImGui::Dummy(ImVec2(0.0f, 12.0f));  // room for the hover time above the strip
    filmstrip(deck, frame, inner);

    // --- source, play mode ---------------------------------------------------------
    ImGui::Dummy(ImVec2(0.0f, 8.0f));
    {
        // « Platine » is offered when there is a platter to follow: deck A's
        // real one on this desk, or the scripted ones of the demo. Choosing it
        // on deck A also starts reading the audio input -- that is what the
        // word means -- through the same request the old chip made.
        static const char* const kSources[] = {"Platine", "Lecture", "Tempo"};
        const bool platter_here = frame.demo_content || is_a;
        const bool enabled[3] = {platter_here, true, true};
        const DeckSource source = deck.clock.source();
        int which = source == DeckSource::Timecode ? 0 : source == DeckSource::TempoLocked ? 2 : 1;
        row_label("SOURCE");
        ImGui::SameLine(0.0f, 8.0f);
        if (segmented("source", kSources, 3, which, enabled)) {
            if (which == 0) {
                deck.clock.hand_over_to_timecode(deck.timecode.state().position_s, now_s);
                if (is_a && !frame.demo_content) frame.deck_a_live = true;
            } else if (which == 2) {
                deck.clock.set_source(DeckSource::TempoLocked, now_s);
            } else {
                deck.clock.set_source(DeckSource::FreeRun, now_s);
                if (is_a && !frame.demo_content) frame.deck_a_live = false;
            }
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("d'o\xC3\xB9 vient la position :\n"
                              "Platine \xE2\x80\x94 la platine, par le timecode\n"
                              "Lecture \xE2\x80\x94 l'horloge du deck, \xC3\xA0 la vitesse choisie\n"
                              "Tempo \xE2\x80\x94 l'horloge du deck, le clip \xC3\xA9tir\xC3\xA9 sur un nombre de temps");
        }

        static const char* const kModes[] = {"Boucle", "Aller-retour", "Une fois"};
        const ClipPlayMode mode = deck.clock.mode();
        int which_mode = mode == ClipPlayMode::Loop ? 0 : mode == ClipPlayMode::PingPong ? 1 : 2;
        float modes_w = ImGui::CalcTextSize("LECTURE").x + 8.0f;
        for (const char* label : kModes) modes_w += ImGui::CalcTextSize(label).x + 20.0f;
        // After an item the cursor sits at the start of the next line, so the
        // item's right edge minus the cursor is exactly what the row has used.
        const float used = ImGui::GetItemRectMax().x - ImGui::GetCursorScreenPos().x;
        if (inner - used - 18.0f >= modes_w) {
            ImGui::SameLine(0.0f, 18.0f);
        } else {
            ImGui::Dummy(ImVec2(0.0f, 4.0f));
        }
        row_label("LECTURE");
        ImGui::SameLine(0.0f, 8.0f);
        if (segmented("mode", kModes, 3, which_mode)) {
            deck.clock.set_mode(which_mode == 0   ? ClipPlayMode::Loop
                                : which_mode == 1 ? ClipPlayMode::PingPong
                                                  : ClipPlayMode::Once);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("ce que fait le clip \xC3\xA0 ses bouts : il reboucle, il repart "
                              "\xC3\xA0 l'envers, ou il s'arr\xC3\xAAte");
        }
        if (deck.played.reversed) {
            ImGui::SameLine(0.0f, 12.0f);
            row_label("\xE2\x86\x90 retour");
        }
    }

    // --- 360, only when the clip is a sphere ------------------------------------------
    if (is_a && deck.clip.is_equirect()) {
        ImGui::Dummy(ImVec2(0.0f, 8.0f));
        SphereView& gaze = engine.view_a();
        static const char* const kProjections[] = {"Perspective", "Little planet", "Fisheye"};
        int which = gaze.projection == Projection::Perspective     ? 0
                    : gaze.projection == Projection::LittlePlanet ? 1
                                                                  : 2;
        row_label("VUE 360");
        ImGui::SameLine(0.0f, 8.0f);
        if (segmented("projection", kProjections, 3, which, nullptr, kSlate)) {
            gaze.projection = which == 0   ? Projection::Perspective
                              : which == 1 ? Projection::LittlePlanet
                                           : Projection::Fisheye;
        }
        ImGui::SameLine(0.0f, 14.0f);
        {
            const float dy = (kControlHeight - ImGui::GetTextLineHeight()) * 0.5f;
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + dy);
            push_small();
            ImGui::PushStyleColor(ImGuiCol_Text, rgba(kMuted));
            ImGui::Text("lacet %+.0f\xC2\xB0 \xC2\xB7 tangage %+.0f\xC2\xB0 \xC2\xB7 %s %.0f",
                        gaze.yaw_deg, gaze.pitch_deg,
                        gaze.projection == Projection::Perspective ? "champ" : "zoom",
                        gaze.projection == Projection::Perspective ? gaze.fov_deg
                                                                   : gaze.planet_zoom);
            ImGui::PopStyleColor();
            pop_font();
        }
        ImGui::SameLine(0.0f, 10.0f);
        if (button("r\xC3\xA9gler", Icon::None, false, 0.0f, kSlate)) ImGui::OpenPopup("gaze");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("le regard suit aussi les potards EQ de la voie 1 (mapping) ;\n"
                              "un curseur boug\xC3\xA9 ici tient jusqu'au prochain pas du mapping");
        }
        if (ImGui::BeginPopup("gaze")) {
            // Doubles, because that is what core/sphere speaks; ImGui edits
            // floats, so each one is bounced rather than the geometry weakened.
            const auto slider = [](const char* label, double& value, float lo, float hi,
                                   const char* format) {
                float editable = static_cast<float>(value);
                ImGui::SetNextItemWidth(200.0f);
                if (ImGui::SliderFloat(label, &editable, lo, hi, format)) {
                    value = static_cast<double>(editable);
                }
            };
            eyebrow("REGARD");
            slider("lacet", gaze.yaw_deg, -180.0f, 180.0f, "%.0f\xC2\xB0");
            slider("tangage", gaze.pitch_deg, -90.0f, 90.0f, "%.0f\xC2\xB0");
            slider("roulis", gaze.roll_deg, -180.0f, 180.0f, "%.0f\xC2\xB0");
            if (gaze.projection == Projection::Perspective) {
                slider("champ", gaze.fov_deg, 20.0f, 170.0f, "%.0f\xC2\xB0");
            } else {
                slider("zoom", gaze.planet_zoom, 0.2f, 3.0f, "%.2f");
            }
            ImGui::Dummy(ImVec2(0.0f, 8.0f));
            eyebrow("SOURCE \xC3\x89QUIRECTANGULAIRE \xE2\x80\x94 cadre de vis\xC3\xA9""e");
            {
                const float w = 320.0f;
                const float h = w * 0.5f;  // an equirect is always 2:1
                ImDrawList* draw = ImGui::GetWindowDrawList();
                const ImVec2 origin = ImGui::GetCursorScreenPos();
                draw->AddRectFilled(origin, ImVec2(origin.x + w, origin.y + h), kWell);
                if (frame.tex_equirect != nullptr) {
                    draw->AddImage(ImTextureRef(reinterpret_cast<ImTextureID>(frame.tex_equirect)),
                                   origin, ImVec2(origin.x + w, origin.y + h));
                }
                draw_sight_frame(gaze, origin, w, h);
                draw->AddRect(origin, ImVec2(origin.x + w, origin.y + h), kHair);
                ImGui::Dummy(ImVec2(w, h));
            }
            ImGui::EndPopup();
        }
    }

    // --- the loop row --------------------------------------------------------------------
    // Beats, not seconds: the grid is the clip's (bpm from the analysis, or
    // the desk's), and a loop that is a number of beats is one a hand can
    // count. The Elite's loop encoder and these buttons write the same
    // commands.
    ImGui::Dummy(ImVec2(0.0f, 8.0f));
    {
        DeckCommands& commands = is_a ? frame.commands_a : frame.commands_b;
        const Loop& loop = deck.transport.loop();
        row_label("BOUCLE");
        ImGui::SameLine(0.0f, 8.0f);
        static const char* const kBeats[] = {"1", "2", "4", "8"};
        const double beats_of[] = {1.0, 2.0, 4.0, 8.0};
        int which = -1;
        if (loop.active && deck.transport.quantise()) {
            for (int i = 0; i < 4; ++i) {
                const double beat_s = 60.0 / engine.bpm();
                if (std::fabs(loop.length_s() - beats_of[i] * beat_s) < 1e-3) which = i;
            }
        }
        if (segmented("autoloop", kBeats, 4, which, nullptr, kAccent)) {
            commands.auto_loop_beats = beats_of[which];
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("une boucle de tant de temps, \xC3\xA0 partir d'ici");
        ImGui::SameLine(0.0f, 8.0f);
        if (button("Entr\xC3\xA9""e", Icon::None, false, 0.0f, kInk, loaded)) commands.loop_in = true;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("le d\xC3\xA9""but de la boucle, ici");
        ImGui::SameLine(0.0f, 4.0f);
        if (button("Sortie", Icon::None, false, 0.0f, kInk, loaded)) commands.loop_out = true;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("la fin de la boucle, ici");
        ImGui::SameLine(0.0f, 4.0f);
        if (button("\xC3\x97", Icon::None, loop.active, 0.0f, kInk, loop.active)) commands.loop_exit = true;
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(loop.active ? "quitter la boucle (%.2f s)" : "aucune boucle", loop.length_s());
        }
        // The second half -- the beat jump and the quantise switch -- on the
        // same line when it fits, under otherwise. A control off the edge is
        // a control that does not exist.
        {
            const float used = ImGui::GetItemRectMax().x - ImGui::GetWindowPos().x -
                               ImGui::GetWindowContentRegionMin().x;
            const float need = ImGui::CalcTextSize("SAUT").x + 6.0f + 2.0f * kControlHeight + 4.0f +
                               14.0f + 18.0f + ImGui::CalcTextSize("Quantis\xC3\xA9").x + 24.0f;
            if (inner - used - 14.0f >= need) {
                ImGui::SameLine(0.0f, 14.0f);
            } else {
                ImGui::Dummy(ImVec2(0.0f, 4.0f));
            }
        }
        row_label("SAUT");
        ImGui::SameLine(0.0f, 6.0f);
        if (button("\xE2\x88\x92", Icon::None, false, kControlHeight, kInk, loaded)) commands.beat_jump_beats = -1.0;
        ImGui::SameLine(0.0f, 4.0f);
        if (button("+", Icon::None, false, kControlHeight, kInk, loaded)) commands.beat_jump_beats = 1.0;
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("un temps en avant ; \xE2\x88\x92 un temps en arri\xC3\xA8re");
        ImGui::SameLine(0.0f, 14.0f);
        if (toggle("Quantis\xC3\xA9", deck.transport.quantise())) {
            deck.transport.set_quantise(!deck.transport.quantise());
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("cues et boucles cal\xC3\xA9s sur le temps le plus proche");
        if (deck.transport.slip()) {
            ImGui::SameLine(0.0f, 10.0f);
            row_label("SLIP");
        }
    }

    // --- the pads ----------------------------------------------------------------------------
    // Eight, as on the Elite, lit as the hardware's are. The mode decides
    // what a pad does; a pad here and a pad on the desk go through the same
    // commands and the same matrix.
    ImGui::Dummy(ImVec2(0.0f, 8.0f));
    {
        DeckCommands& commands = is_a ? frame.commands_a : frame.commands_b;
        int& mode = is_a ? frame.pad_mode_a : frame.pad_mode_b;
        const DeckTarget target = is_a ? DeckTarget::A : DeckTarget::B;
        const float pad = 44.0f;
        const ImVec2 row_origin = ImGui::GetCursorScreenPos();
        ImDrawList* draw = ImGui::GetWindowDrawList();
        const bool shift = ImGui::GetIO().KeyShift;
        static const double kLoopBeats[kPadCount] = {0.125, 0.25, 0.5, 1.0, 2.0, 4.0, 8.0, 16.0};
        static const char* const kLoopWords[kPadCount] = {"1/8", "1/4", "1/2", "1", "2", "4", "8", "16"};

        for (int i = 0; i < kPadCount; ++i) {
            if (i > 0) ImGui::SameLine(0.0f, 6.0f);
            const ImVec2 origin = ImGui::GetCursorScreenPos();
            ImGui::PushID(i);
            ImGui::InvisibleButton("pad", ImVec2(pad, pad));
            const bool hovered = ImGui::IsItemHovered();
            const bool pressed = ImGui::IsItemClicked(ImGuiMouseButton_Left);
            const bool right = ImGui::IsItemClicked(ImGuiMouseButton_Right);
            ClipId dropped = kNoClip;
            if (mode == 1) dropped = clip_drop_target();
            ImGui::PopID();

            bool lit = false;
            std::string word;
            std::string tip;
            if (mode == 0) {
                const HotCue& cue = deck.transport.cue(i);
                lit = cue.set;
                if (cue.set) word = short_clock(cue.position_s);
                tip = cue.set ? "sauter au cue \xC2\xB7 Maj + clic : reposer ici \xC2\xB7 clic droit : effacer"
                              : "Maj + clic : poser un cue ici";
            } else if (mode == 1) {
                const ClipId clip = engine.matrix().clip_at(target, i);
                lit = clip != kNoClip;
                if (lit) word = pad_word(engine.library().at(clip).name);
                tip = lit ? "charger ce clip \xC2\xB7 clic droit : vider la case"
                          : "d\xC3\xA9poser un clip ici (ou dans BIBLIOTH\xC3\x88QUE)";
            } else {
                const double beat_s = 60.0 / engine.bpm();
                lit = deck.transport.loop().active &&
                      std::fabs(deck.transport.loop().length_s() - kLoopBeats[i] * beat_s) < 1e-3;
                word = kLoopWords[i];
                tip = "une boucle de " + std::string(kLoopWords[i]) + " temps";
            }

            const ImVec2 corner(origin.x + pad, origin.y + pad);
            draw->AddRectFilled(origin, corner, hovered ? kHair : kWell, 2.0f);
            draw->AddRect(origin, corner, lit ? accent : kHair, 2.0f, 0, lit ? 2.0f : 1.0f);
            push_small();
            char number[4];
            std::snprintf(number, sizeof(number), "%d", i + 1);
            draw->AddText(ImVec2(origin.x + (pad - ImGui::CalcTextSize(number).x) * 0.5f,
                                 origin.y + (word.empty() ? 15.0f : 7.0f)),
                          lit ? accent : kFaint, number);
            if (!word.empty()) {
                const float ww = ImGui::CalcTextSize(word.c_str()).x;
                draw->AddText(ImVec2(origin.x + (pad - ww) * 0.5f, origin.y + 24.0f), kMuted, word.c_str());
            }
            pop_font();
            if (hovered && ImGui::GetDragDropPayload() == nullptr) ImGui::SetTooltip("%s", tip.c_str());

            if (mode == 0) {
                if (pressed && shift) {
                    commands.cue_set = true;
                    commands.cue_index = i;
                } else if (pressed && lit) {
                    commands.cue_jump = true;
                    commands.cue_index = i;
                } else if (right && lit) {
                    commands.cue_clear = true;
                    commands.cue_index = i;
                }
            } else if (mode == 1) {
                if (dropped != kNoClip) {
                    engine.matrix().set(engine.matrix().current(), target, i, dropped);
                    frame.library_dirty = true;
                } else if (pressed && lit) {
                    frame.load_clip = engine.matrix().clip_at(target, i);
                    frame.load_target = target;
                } else if (right && lit) {
                    engine.matrix().set(engine.matrix().current(), target, i, kNoClip);
                    frame.library_dirty = true;
                }
            } else if (pressed) {
                if (lit) commands.loop_exit = true; else commands.auto_loop_beats = kLoopBeats[i];
            }
        }

        // The mode, at the right end of the pad row.
        static const char* const kModes[] = {"Cues", "Clips", "Boucles"};
        float modes_w = 0.0f;
        for (const char* label : kModes) modes_w += ImGui::CalcTextSize(label).x + 20.0f;
        const float pads_w = kPadCount * pad + (kPadCount - 1) * 6.0f;
        if (inner - pads_w - 12.0f >= modes_w) {
            ImGui::SameLine(inner - modes_w);
            ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x, row_origin.y));
        } else {
            ImGui::Dummy(ImVec2(0.0f, 4.0f));
        }
        segmented("padmode", kModes, 3, mode, nullptr, accent);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("ce que fait un pad :\nCues \xE2\x80\x94 sauter \xC3\xA0 un rep\xC3\xA8re (Maj + clic le pose)\n"
                              "Clips \xE2\x80\x94 charger un clip de la banque %d\nBoucles \xE2\x80\x94 une boucle de tant de temps",
                              engine.matrix().current() + 1);
        }
    }

    ImGui::Dummy(ImVec2(0.0f, 6.0f));
    draw_deck_diagnostics(deck, engine, frame, is_a);

    ImGui::EndChild();
    ImGui::PopID();
}

// --- the reactive spectrum ---------------------------------------------------
//
// Eight log-spaced bands, drawn as bars with their edges in hertz underneath.
// The frequency labels are not decoration: an AudioBand mapping names a band by
// INDEX, and a band whose range is invisible is a number nobody can map on
// purpose. Seeing that band 0 stops at 90 Hz is what makes "graves -> bloom" a
// decision rather than a guess.
void draw_spectrum(const Engine& engine) {
    const SpectrumAnalyser& analyser = engine.spectrum();
    const std::size_t count = analyser.band_count();
    if (count == 0) return;

    eyebrow("SPECTRE \xE2\x80\x94 sources audio-r\xC3\xA9""actives");
    ImGui::Dummy(ImVec2(0.0f, 4.0f));

    const float height = 34.0f;
    const float gap = 3.0f;
    const float full = ImGui::GetContentRegionAvail().x;
    const float bar = (full - gap * static_cast<float>(count - 1)) /
                      static_cast<float>(count);
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImDrawList* draw = ImGui::GetWindowDrawList();

    for (std::size_t i = 0; i < count; ++i) {
        const float x = origin.x + static_cast<float>(i) * (bar + gap);
        const float level = analyser.bands()[i];

        draw->AddRectFilled(ImVec2(x, origin.y), ImVec2(x + bar, origin.y + height),
                            kWell);
        const float filled = height * level;
        if (filled > 0.5f) {
            // Amber low, sage high: the bass end is what a VJ reaches for first,
            // and giving it the accent colour makes the kick findable at a
            // glance across a dark booth.
            const float mix = static_cast<float>(i) / static_cast<float>(count - 1 ? count - 1 : 1);
            const ImU32 colour = mix < 0.5f ? kAccent : kSage;
            draw->AddRectFilled(ImVec2(x, origin.y + height - filled),
                                ImVec2(x + bar, origin.y + height), colour);
        }
    }
    ImGui::Dummy(ImVec2(full, height + 2.0f));

    push_small();
    ImGui::PushStyleColor(ImGuiCol_Text, rgba(kFaint));
    for (std::size_t i = 0; i < count; ++i) {
        if (i > 0) ImGui::SameLine(0.0f, gap);
        const double hz = analyser.band_high_hz(i);
        char label[16];
        if (hz >= 1000.0) {
            std::snprintf(label, sizeof(label), "%.0fk", hz / 1000.0);
        } else {
            std::snprintf(label, sizeof(label), "%.0f", hz);
        }
        // Fixed-width cells, so the labels stay under their own bars instead of
        // drifting as the numbers change width.
        ImGui::BeginGroup();
        ImGui::Dummy(ImVec2(bar, 0.0f));
        ImGui::TextUnformatted(label);
        ImGui::EndGroup();
    }
    ImGui::PopStyleColor();
    pop_font();
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
            Frame& frame, ControlIndex index, ImU32 accent, float scale = 1.0f) {
    const float diameter = 46.0f * scale;
    const float column = 64.0f * scale;
    // Below about two thirds the caption and the number collide with the arc.
    // A shrunk knob keeps its shape and loses its writing; the panel it sits
    // on says what it is by where it is, and hovering still gives the number.
    const bool captioned = scale > 0.66f;

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

    if (!captioned) {
        if (hovered) {
            ImGui::SetTooltip("%s  %s", label,
                              known ? std::to_string(value).substr(0, 4).c_str() : "?");
        }
        ImGui::EndGroup();
        return changed;
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
                    Frame& frame, ControlIndex index, ImU32 accent, float scale = 1.0f) {
    const float height = 96.0f * scale;
    const float column = 46.0f * scale;
    const bool captioned = scale > 0.66f;

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

    if (captioned) {
        push_small();
        const float label_w = ImGui::CalcTextSize(label).x;
        ImGui::SetCursorScreenPos(ImVec2(cx - label_w * 0.5f, origin.y + height + 8.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, rgba(known ? kMuted : kFaint));
        ImGui::TextUnformatted(label);
        ImGui::PopStyleColor();
        pop_font();
    } else if (hovered) {
        ImGui::SetTooltip("%s", label);
    }
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

    // The value under the cap, and nothing else: the curve is chosen right
    // beside this widget now, and a caption that named one was a lie whenever
    // the setting was another.
    (void)engine;
    push_small();
    push_mono();
    const char* shown = known ? nullptr : "?";
    char value_text[16];
    if (known) {
        std::snprintf(value_text, sizeof(value_text), "%.2f", static_cast<double>(value));
        shown = value_text;
    }
    const float tw = ImGui::CalcTextSize(shown).x;
    ImGui::SetCursorScreenPos(ImVec2(origin.x + (width - tw) * 0.5f, origin.y + height + 4.0f));
    text_c(known ? kMuted : kFaint, "%s", shown);
    pop_font();
    pop_font();

    ImGui::EndGroup();
    return changed;
}

ImU32 accent_colour(const std::string& name) {
    if (name == "amber") return kAmber;
    if (name == "slate") return kSlate;
    if (name == "sage") return kSage;
    if (name == "faint") return kFaint;
    return kInk;
}

// A button or a pad: a small square, lit while the control reads high, with
// the last segment of its id under it. Clicking it presses it for the frame,
// which is what a mouse can honestly do to a momentary control.
void small_square(const char* id, const char* label, const Control* control, Frame& frame,
                  ControlIndex index, ImU32 accent, float side, bool optional) {
    ImGui::BeginGroup();
    push_small();
    const float label_w = label != nullptr ? ImGui::CalcTextSize(label).x : 0.0f;
    pop_font();
    const float column = std::max(side, label_w + 2.0f);
    const ImVec2 cell = ImGui::GetCursorScreenPos();
    const ImVec2 origin(cell.x + (column - side) * 0.5f, cell.y);
    ImGui::InvisibleButton(id, ImVec2(column, side));
    const bool known = control != nullptr && control->known;
    const bool high = known && control->value > 0.5f;
    if (ImGui::IsItemActive() && control != nullptr) claim(frame, index, 1.0f);
    if (ImGui::IsItemDeactivated() && control != nullptr) claim(frame, index, 0.0f);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 corner(origin.x + side, origin.y + side);
    if (high) {
        draw->AddRectFilled(origin, corner, accent, 2.0f);
    } else if (known) {
        draw->AddRectFilled(origin, corner, kWell, 2.0f);
        draw->AddRect(origin, corner, accent, 2.0f);
    } else {
        // Unknown: dashed, the same honesty the rotaries have.
        draw->AddRectFilled(origin, corner, kWell, 2.0f);
        for (float x = origin.x; x < corner.x; x += 5.0f) {
            draw->AddLine(ImVec2(x, origin.y), ImVec2(std::min(x + 2.5f, corner.x), origin.y),
                          optional ? kHair : kFaint);
            draw->AddLine(ImVec2(x, corner.y - 1.0f),
                          ImVec2(std::min(x + 2.5f, corner.x), corner.y - 1.0f),
                          optional ? kHair : kFaint);
        }
    }
    if (label != nullptr && label[0] != '\0') {
        push_small();
        const ImVec2 size = ImGui::CalcTextSize(label);
        draw->AddText(ImVec2(cell.x + (column - size.x) * 0.5f, corner.y + 1.0f),
                      optional ? kHair : kFaint, label);
        ImGui::Dummy(ImVec2(column, size.y + 2.0f));
        pop_font();
    }
    ImGui::EndGroup();
}

// The last segment of an id, which is what a label under a control needs:
// "ch1.eq.hi" -> "hi", "pad.elite.a.3" -> "3", "fx.a.beats.push" -> "push".
const char* short_label(const std::string& id) {
    const std::size_t dot = id.rfind('.');
    return dot == std::string::npos ? id.c_str() : id.c_str() + dot + 1;
}

// One control of a profile, at the cursor, at a scale. The kind decides the
// widget; the id's last segment is the label.
void draw_profile_control(const DeviceProfile& profile, const std::string& id, Engine& engine,
                          Frame& frame, std::size_t device_index, ImU32 accent, float scale,
                          float xfader_width) {
    const ProfileControl* spec = profile_control(profile, id);
    if (spec == nullptr) return;
    ControlIndex index = kNoControl;
    const Control* control = surface_control(engine.surface(), id.c_str(), &index);
    char widget_id[160];
    std::snprintf(widget_id, sizeof(widget_id), "##%zu.%s", device_index, id.c_str());
    const char* label = short_label(id);
    switch (spec->kind) {
        case ControlKind::Knob:
            rotary(widget_id, label, control, frame, index, accent, scale);
            break;
        case ControlKind::Encoder:
            // An endless encoder drawn as a rotary that keeps turning; its
            // value is a walk, its detents go to whoever scrolls a list.
            rotary(widget_id, label, control, frame, index, kSage, scale);
            break;
        case ControlKind::Fader:
            if (id == "xfader") {
                crossfader(control, frame, index, engine, xfader_width);
            } else {
                vertical_fader(widget_id, label, control, frame, index, accent, scale);
            }
            break;
        case ControlKind::Pad:
            small_square(widget_id, label, control, frame, index, accent, 22.0f * scale,
                         spec->optional);
            break;
        case ControlKind::Button:
        default:
            small_square(widget_id, label, control, frame, index, accent, 18.0f * scale,
                         spec->optional);
            break;
    }
}

// A group placed on a panel: its own rectangle, its title in the corner, and
// its controls flowed inside at whatever scale makes them fit.
//
// This is the difference between a list of knobs and a picture of the mixer.
// A hand finds the filter knob by its PLACE; an interface that shows the same
// controls in a different arrangement is a second panel to learn rather than
// a mirror of the one under your fingers.
void draw_group_at(const DeviceProfile& profile, const ProfileGroup& group, Engine& engine,
                   Frame& frame, std::size_t device_index, ImVec2 at, ImVec2 size) {
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImU32 accent = accent_colour(group.accent);
    const ImVec2 corner(at.x + size.x, at.y + size.y);
    draw->AddRectFilled(at, corner, kPanel, 3.0f);
    draw->AddRect(at, corner, kHair, 3.0f);

    push_small();
    const float title_h = ImGui::GetTextLineHeight() + 2.0f;
    draw->AddText(ImVec2(at.x + 5.0f, at.y + 2.0f), accent, group.title.c_str());
    pop_font();

    const int count = static_cast<int>(group.controls.size());
    if (count == 0) return;
    const int columns = group.columns > 0 ? std::min(group.columns, count) : count;
    const int rows = (count + columns - 1) / columns;
    const float pad = 4.0f;
    const float cell_w = (size.x - 2.0f * pad) / static_cast<float>(columns);
    const float cell_h = (size.y - title_h - 2.0f * pad) / static_cast<float>(rows);
    // 64 x 78 is a rotary's natural cell; the scale is whatever fits, floored
    // so a dense bank stays clickable rather than vanishing.
    const float scale = std::clamp(std::min(cell_w / 64.0f, cell_h / 78.0f), 0.34f, 1.0f);

    for (int i = 0; i < count; ++i) {
        const int row = i / columns;
        const int column = i % columns;
        ImGui::SetCursorScreenPos(ImVec2(at.x + pad + cell_w * static_cast<float>(column),
                                         at.y + title_h + pad + cell_h * static_cast<float>(row)));
        draw_profile_control(profile, group.controls[static_cast<std::size_t>(i)], engine, frame,
                             device_index, accent, scale, std::max(60.0f, size.x - 2.0f * pad));
    }
}

// A whole device, drawn as its panel: every group where the hardware has it.
void draw_device_panel(const DeviceProfile& profile, Engine& engine, Frame& frame,
                       std::size_t device_index, ImVec2 size) {
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const float scale = std::min(size.x / profile.panel_w, size.y / profile.panel_h);
    const float panel_w = profile.panel_w * scale;
    const float panel_h = profile.panel_h * scale;
    // Centred in what it was given, so a panel narrower than the window does
    // not sit against one edge.
    const ImVec2 at(origin.x + (size.x - panel_w) * 0.5f, origin.y);

    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(at, ImVec2(at.x + panel_w, at.y + panel_h), kWell, 5.0f);
    draw->AddRect(at, ImVec2(at.x + panel_w, at.y + panel_h), kHair, 5.0f);

    for (const ProfileGroup& group : profile.layout) {
        if (group.front_panel || !group.placed()) continue;
        ImGui::PushID(group.title.c_str());
        draw_group_at(profile, group, engine, frame, device_index,
                      ImVec2(at.x + group.x * scale, at.y + group.y * scale),
                      ImVec2(group.w * scale, group.h * scale));
        ImGui::PopID();
    }
    ImGui::SetCursorScreenPos(ImVec2(origin.x, at.y + panel_h + 4.0f));

    // The front edge, in a strip below: those controls exist, and a top-down
    // picture is exactly where they are not.
    for (const ProfileGroup& group : profile.layout) {
        if (!group.front_panel) continue;
        ImGui::PushID(group.title.c_str());
        push_small();
        text_c(kFaint, "%s", group.title.c_str());
        pop_font();
        ImGui::SameLine(0.0f, 10.0f);
        for (const std::string& id : group.controls) {
            draw_profile_control(profile, id, engine, frame, device_index,
                                 accent_colour(group.accent), 0.55f, 120.0f);
            ImGui::SameLine(0.0f, 4.0f);
        }
        ImGui::NewLine();
        ImGui::PopID();
    }
}

// One group of a profile's layout: a title, then its controls, drawn by kind.
// Grids (pads, button banks) wrap at `columns`; rows run on.
void draw_group(const DeviceProfile& profile, const ProfileGroup& group, Engine& engine,
                Frame& frame, std::size_t device_index) {
    const Surface& surface = engine.surface();
    const ImU32 accent = accent_colour(group.accent);
    ImGui::BeginGroup();
    push_small();
    text_c(accent, "%s", group.title.c_str());
    pop_font();
    ImGui::Dummy(ImVec2(0.0f, 2.0f));

    int column = 0;
    bool first_in_row = true;
    for (const std::string& id : group.controls) {
        const ProfileControl* spec = profile_control(profile, id);
        if (spec == nullptr) continue;
        ControlIndex index = kNoControl;
        const Control* control = surface_control(surface, id.c_str(), &index);
        const bool grid = group.columns > 0;
        if (!first_in_row) ImGui::SameLine(0.0f, grid ? 3.0f : 2.0f);
        first_in_row = false;

        char widget_id[128];
        std::snprintf(widget_id, sizeof(widget_id), "##%zu.%s", device_index, id.c_str());
        // Labels: a pad shows its number, a knob its last segment, a channel
        // control its last segment too ("hi", "trim", "fader").
        const char* label = short_label(id);
        switch (spec->kind) {
            case ControlKind::Knob:
                rotary(widget_id, label, control, frame, index, accent);
                break;
            case ControlKind::Encoder:
                // An endless encoder drawn as a rotary that keeps turning;
                // its value is a walk, its detents go to whoever scrolls.
                rotary(widget_id, label, control, frame, index, kSage);
                break;
            case ControlKind::Fader:
                if (id == "xfader") {
                    crossfader(control, frame, index, engine,
                               std::max(180.0f, std::min(320.0f, ImGui::GetContentRegionAvail().x)));
                } else {
                    vertical_fader(widget_id, label, control, frame, index, accent);
                }
                break;
            case ControlKind::Pad:
                small_square(widget_id, label, control, frame, index, accent, 22.0f, spec->optional);
                break;
            case ControlKind::Button:
            default:
                small_square(widget_id, label, control, frame, index, accent, 18.0f, spec->optional);
                break;
        }
        if (grid) {
            if (++column >= group.columns) {
                column = 0;
                first_in_row = true;
            }
        }
    }
    ImGui::EndGroup();
}

// The mixer's switches, next to the crossfader: the curves and the reverse for
// the crossfader and for the channel faders, as chips. The Elite has these on
// its front panel; until their MIDI is measured this is where they are set.
// Who is connected, how much of them is bound, and the learn buttons. Drawn
// above both the glance strip and the full table.
void draw_surface_header(Frame& frame) {
    // The heading tells the truth about where the values come from, which
    // changes the moment a device is plugged in -- and WHICH devices, by
    // their profiles' names, not a literal.
    std::string names;
    std::size_t connected = 0;
    for (const Frame::RigDeviceView& d : frame.rig) {
        if (!d.connected) continue;
        if (!names.empty()) names += " \xC2\xB7 ";
        names += d.profile != nullptr ? d.profile->display_name : d.profile_name;
        if (d.profile != nullptr && d.profile->name == "rp8000") names += d.deck == 'b' ? " B" : " A";
        ++connected;
    }
    if (connected > 0) {
        eyebrow(("SURFACE \xE2\x80\x94 " + names).c_str());
        ImGui::SameLine(0.0f, 14.0f);
        push_small();
        text_c(frame.midi_bound > 0 ? kSage : kAmber,
               "%zu port%s \xC2\xB7 %llu messages \xC2\xB7 %zu/%zu li\xC3\xA9s", connected,
               connected > 1 ? "s" : "", static_cast<unsigned long long>(frame.midi_messages),
               frame.midi_bound, frame.midi_total);
        pop_font();
    } else {
        std::string waiting = "SURFACE \xE2\x80\x94 ";
        for (std::size_t i = 0; i < frame.rig.size(); ++i) {
            if (i > 0) waiting += " \xC2\xB7 ";
            waiting += frame.rig[i].profile != nullptr ? frame.rig[i].profile->display_name
                                                       : frame.rig[i].profile_name;
        }
        waiting += " \xE2\x80\x94 la souris joue en attendant le MIDI";
        eyebrow(waiting.c_str());
    }

    // MIDI learn, per device. The Elite's map is not published and nothing
    // here is hard-coded, so this is how the surface ever gets bound -- and
    // the mixer was measured NOT to announce its state on connect, which is
    // why a deliberate sweep is the only way. A device whose profile ships
    // bindings (APC40, Push) can still be re-learned, which is how a profile
    // written from a document gets corrected on the desk.
    ImGui::SameLine(0.0f, 16.0f);
    if (frame.learning) {
        push_small();
        const char* who = frame.learn_device >= 0 &&
                                  static_cast<std::size_t>(frame.learn_device) < frame.rig.size() &&
                                  frame.rig[static_cast<std::size_t>(frame.learn_device)].profile != nullptr
                              ? frame.rig[static_cast<std::size_t>(frame.learn_device)]
                                    .profile->display_name.c_str()
                              : "?";
        text_c(kAmber, "%s \xE2\x80\x94 BOUGE : %s", who, frame.learn_prompt.c_str());
        pop_font();
        ImGui::SameLine(0.0f, 10.0f);
        if (ImGui::SmallButton("passer")) frame.learn_skip = true;
        ImGui::SameLine(0.0f, 6.0f);
        if (ImGui::SmallButton("arr\xC3\xAAter")) frame.learn_cancel = true;
        ImGui::SameLine(0.0f, 10.0f);
        push_small();
        text_c(kFaint, "%zu restants", frame.learn_remaining);
        pop_font();
    } else {
        for (std::size_t i = 0; i < frame.rig.size(); ++i) {
            const Frame::RigDeviceView& d = frame.rig[i];
            if (!d.connected) continue;
            ImGui::PushID(static_cast<int>(i));
            push_small();
            char label[96];
            std::snprintf(label, sizeof(label), "apprendre %s",
                          d.profile != nullptr ? d.profile->display_name.c_str()
                                               : d.profile_name.c_str());
            if (ImGui::SmallButton(label)) {
                frame.learn_device = static_cast<int>(i);
                frame.learn_start = true;
            }
            if (d.profile != nullptr && !d.profile->verified) {
                ImGui::SameLine(0.0f, 4.0f);
                text_c(kAmber, "non v\xC3\xA9rifi\xC3\xA9");
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("profil \xC3\xA9" "crit depuis un document, jamais balay\xC3\xA9 sur "
                                      "le mat\xC3\xA9riel \xE2\x80\x94 voir profiles/README.md");
                }
            }
            pop_font();
            ImGui::PopID();
            ImGui::SameLine(0.0f, 8.0f);
        }
        ImGui::NewLine();
    }

}

// The whole rig, drawn as its panels: the TABLE screen. A portrait mixer
// panel needs real height, which a strip along the bottom of the performance
// screen does not have -- so the measured drawing lives here, where it can be
// read, and the strip below keeps only what a hand glances at mid-set.
void draw_mapping_list(Engine& engine, Frame& frame);  // below, with the rest of the mapping

void draw_table_screen(Engine& engine, Frame& frame) {
    draw_surface_header(frame);
    ImGui::Dummy(ImVec2(0.0f, 6.0f));
    const float list_w = 540.0f;
    const float gap = ImGui::GetStyle().ItemSpacing.x;
    const float table_w = std::max(400.0f, ImGui::GetContentRegionAvail().x - list_w - gap);
    ImGui::BeginChild("table", ImVec2(table_w, 0.0f), ImGuiChildFlags_None,
                      ImGuiWindowFlags_HorizontalScrollbar);

    // The available height, shared by the devices that have a panel drawing.
    const float body_h = std::max(120.0f, ImGui::GetContentRegionAvail().y - 6.0f);
    bool first = true;
    for (std::size_t i = 0; i < frame.rig.size(); ++i) {
        const Frame::RigDeviceView& d = frame.rig[i];
        if (d.profile == nullptr) continue;
        // The RP-8000's pads are named after their deck; the built-in profile
        // is deck A's, so deck B's comes from the same table with its letter.
        const DeviceProfile deck_profile =
            d.profile->name == "rp8000" ? rp8000_profile(d.deck) : DeviceProfile{};
        const DeviceProfile& profile = d.profile->name == "rp8000" ? deck_profile : *d.profile;

        if (!first) ImGui::SameLine(0.0f, 20.0f);

        if (profile_has_geometry(profile)) {
            // A panel: every section where the hardware has it.
            const float panel_width = body_h * (profile.panel_w / profile.panel_h);
            ImGui::PushID(static_cast<int>(i));
            ImGui::BeginGroup();
            draw_device_panel(profile, engine, frame, i, ImVec2(panel_width, body_h * 0.86f));
            ImGui::EndGroup();
            ImGui::PopID();
            first = false;
            continue;
        }

        // No measured geometry: an honest flowing row rather than an invented
        // panel. A wrong picture of a controller is worse than a list.
        for (const ProfileGroup& group : profile.layout) {
            if (!first) ImGui::SameLine(0.0f, 26.0f);
            first = false;
            ImGui::PushID(static_cast<int>(i));
            ImGui::PushID(group.title.c_str());
            draw_group(profile, group, engine, frame, i);
            ImGui::PopID();
            ImGui::PopID();
        }
    }
    if (first) {
        push_small();
        dim("aucun appareil configur\xC3\xA9 \xE2\x80\x94 voir R\xC3\x89GLAGES");
        pop_font();
    }
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("mappings", ImVec2(list_w, 0.0f), ImGuiChildFlags_Borders);
    // The front edge's three switch pairs, in their left-to-right order:
    // channel 1, the crossfader, channel 2. The Elite's own switches write
    // the same settings; here they can be clicked.
    {
        MixSettings& mix = engine.mix_settings();
        static const char* const kCurves[] = {"Douce", "Lin.", "Sharp", "Cut"};
        const auto curve_index = [](FaderCurve curve) {
            switch (curve) {
                case FaderCurve::Smooth: return 0;
                case FaderCurve::Linear: return 1;
                case FaderCurve::Sharp: return 2;
                case FaderCurve::Cut: return 3;
            }
            return 1;
        };
        const auto curve_of = [](int index) {
            return index == 0 ? FaderCurve::Smooth : index == 1 ? FaderCurve::Linear
                              : index == 2 ? FaderCurve::Sharp : FaderCurve::Cut;
        };
        eyebrow("FACE AVANT \xE2\x80\x94 courbes");
        ImGui::Dummy(ImVec2(0.0f, 4.0f));
        const struct { const char* title; FaderCurve* curve; bool* reverse; ImU32 accent; } pairs[] = {
            {"VOIE 1", &mix.channel, &mix.channel_reverse, kAmber},
            {"CROSSFADER", &mix.xfader, &mix.xfader_reverse, kAccent},
            {"VOIE 2", &mix.channel_b, &mix.channel_b_reverse, kSlate},
        };
        for (int i = 0; i < 3; ++i) {
            push_small();
            ImGui::PushID(i);
            {
                const float dy = (kControlHeight - ImGui::GetTextLineHeight()) * 0.5f;
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + dy);
                text_c(pairs[i].accent, "%s", pairs[i].title);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() - dy);
            }
            ImGui::SameLine(110.0f);
            int index = curve_index(*pairs[i].curve);
            if (segmented("curve", kCurves, 4, index, nullptr, pairs[i].accent)) {
                *pairs[i].curve = curve_of(index);
                frame.settings_dirty = true;
            }
            ImGui::SameLine(0.0f, 10.0f);
            if (toggle("Invers\xC3\xA9", *pairs[i].reverse)) {
                *pairs[i].reverse = !*pairs[i].reverse;
                frame.settings_dirty = true;
            }
            ImGui::PopID();
            pop_font();
        }
        ImGui::Dummy(ImVec2(0.0f, 10.0f));
    }
    draw_mapping_list(engine, frame);
    ImGui::EndChild();
}

// --- EFFETS: the whole battery, with its correspondences stated honestly ------

const char* source_text(SourceKind kind);  // below, with the mapping

// A control's name, short, for "assigned to".
std::string assigned_source_text(const Mapping& row) {
    if (row.source.kind == SourceKind::Control) return row.source.control_id;
    if (row.source.kind == SourceKind::Modulator) return "modulateur " + std::to_string(row.source.index + 1);
    if (row.source.kind == SourceKind::AudioBand) return "bande " + std::to_string(row.source.index + 1);
    return source_text(row.source.kind);
}

// One rack slot as a card: what it is, whether it runs, whether audio and
// video are linked, its six parameters -- each with what drives it.
void draw_effect_card(Engine& engine, Frame& frame, std::size_t slot) {
    EffectUnit& unit = engine.rack().at(slot);
    const EffectDescriptor* info = describe(unit.type);
    MappingEngine& mapping = engine.mapping();
    ImGui::PushID(static_cast<int>(slot));
    ImGui::BeginChild("card", ImVec2(0.0f, 226.0f), ImGuiChildFlags_Borders);
    const float inner = ImGui::GetContentRegionAvail().x;

    // --- the head ----------------------------------------------------------------
    push_mono();
    text_c(kFaint, "%zu", slot + 1);
    pop_font();
    ImGui::SameLine(0.0f, 10.0f);
    ImGui::SetNextItemWidth(190.0f);
    if (ImGui::BeginCombo("##type", info != nullptr ? info->id : "?")) {
        for (const EffectDescriptor& fx : effect_catalogue()) {
            const bool on = fx.type == unit.type;
            if (ImGui::Selectable(fx.id, on)) {
                engine.rack().load(slot, fx.type);
                engine.rack().at(slot).enabled = true;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s\naudio : %s\nvid\xC3\xA9o : %s", fx.id,
                                  fx.audio != nullptr ? fx.audio : "\xE2\x80\x94", fx.video);
            }
        }
        ImGui::EndCombo();
    }
    // Right end: link, on, clear.
    const float right_w = 330.0f;
    ImGui::SameLine(inner - right_w);
    {
        // Only the UNLINKED state is red: it is the one state where the two
        // domains say different things.
        const bool linked = unit.link;
        const ImVec2 origin = ImGui::GetCursorScreenPos();
        ImGui::PushID("link");
        const char* label = linked ? "Audio et vid\xC3\xA9o li\xC3\xA9s" : "D\xC3\xA9li\xC3\xA9s";
        const float w = ImGui::CalcTextSize(label).x + 34.0f;
        const bool pressed = ImGui::InvisibleButton("l", ImVec2(w, kControlHeight));
        ImGui::PopID();
        ImDrawList* draw = ImGui::GetWindowDrawList();
        draw->AddCircleFilled(ImVec2(origin.x + 8.0f, origin.y + kControlHeight * 0.5f), 5.0f,
                              linked ? kSage : kAlert);
        draw->AddText(ImVec2(origin.x + 22.0f, origin.y + (kControlHeight - ImGui::GetTextLineHeight()) * 0.5f),
                      linked ? kInk : kAlert, label);
        if (pressed) {
            if (linked) unit.unlink(); else unit.relink();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(linked ? "un potard bouge les deux domaines. Clic : les d\xC3\xA9lier."
                                     : "d\xC3\xA9li\xC3\xA9s : les potards ci-dessous r\xC3\xA8glent la vid\xC3\xA9o seule, "
                                       "l'audio garde ses valeurs. Clic : relier.");
        }
    }
    ImGui::SameLine(0.0f, 12.0f);
    if (toggle("Actif", unit.enabled)) unit.enabled = !unit.enabled;
    ImGui::SameLine(0.0f, 12.0f);
    push_small();
    if (button("Vider")) engine.rack().clear(slot);
    pop_font();

    {
        push_small();
        if (info != nullptr) {
            switch (info->relation) {
                case Correspondence::Identical:
                    text_c(kMuted, "audio : %s \xC2\xB7 vid\xC3\xA9o : %s \xC2\xB7 correspondance exacte", info->audio, info->video);
                    break;
                case Correspondence::Analogue:
                    text_c(kMuted, "audio : %s \xC2\xB7 vid\xC3\xA9o : %s \xC2\xB7 ", info->audio, info->video);
                    ImGui::SameLine(0.0f, 0.0f);
                    text_c(kAmber, "analogue");
                    break;
                case Correspondence::VideoOnly:
                    text_c(kMuted, "vid\xC3\xA9o seule : %s", info->video);
                    break;
            }
        }
        pop_font();
    }
    // --- the six parameters ----------------------------------------------------------
    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    EffectParams& params = unit.link ? unit.shared : unit.video_override;
    struct Param { const char* name; const char* id; float* value; };
    const Param table[] = {
        {"Mix", "mix", &params.mix},           {"Amount", "amount", &params.amount},
        {"Time", "time", &params.time},        {"Feedback", "feedback", &params.feedback},
        {"Depth", "depth", &params.depth},     {"Tone", "tone", &params.tone},
    };
    const float col_w = 118.0f;
    for (int p = 0; p < 6; ++p) {
        if (p > 0) ImGui::SameLine(0.0f, 8.0f);
        ImGui::BeginGroup();
        ImGui::PushID(p);
        push_small();
        text_c(kFaint, "%s", table[p].name);
        pop_font();
        ImGui::SetNextItemWidth(col_w - 8.0f);
        float v = *table[p].value;
        if (ImGui::SliderFloat("##v", &v, 0.0f, 1.0f, "%.2f")) *table[p].value = v;
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            *table[p].value = p == 0 || p == 3 ? 0.0f : 0.5f;
        }
        // What drives it. A mapping to this destination shows its source and
        // can be dropped; none shows "assigner", which waits for the next
        // control touched on the desk -- turn the knob, it is done.
        const std::string dest = "fx." + std::to_string(slot + 1) + "." + table[p].id;
        int bound = -1;
        for (std::size_t i = 0; i < mapping.size(); ++i) {
            if (mapping.at(i).destination.target == dest) bound = static_cast<int>(i);
        }
        push_small();
        if (bound >= 0) {
            const Mapping& row = mapping.at(static_cast<std::size_t>(bound));
            const bool listening = frame.mapping_listen && frame.mapping_selected == bound;
            const std::string text = listening ? "\xE2\x80\xA6 bouger un contr\xC3\xB4le"
                                     : row.source.control_id.empty() && row.source.kind == SourceKind::Control
                                         ? "sans contr\xC3\xB4le"
                                         : "\xE2\x86\x90 " + assigned_source_text(row);
            if (button(text.c_str(), Icon::None, listening, col_w - 8.0f,
                       listening ? kInk : kAccent)) {
                // Clicking a bound one drops the binding; the row goes.
                if (!listening) {
                    mapping.remove(static_cast<std::size_t>(bound));
                    if (frame.mapping_selected == bound) frame.mapping_selected = -1;
                    frame.mappings_dirty = true;
                } else {
                    frame.mapping_listen = false;
                }
            }
            if (ImGui::IsItemHovered() && !listening) ImGui::SetTooltip("clic : retirer cette liaison");
        } else {
            if (button("assigner", Icon::None, false, col_w - 8.0f, kMuted)) {
                Mapping row;
                row.source.kind = SourceKind::Control;
                const ControlIndex touched = engine.surface().last_touched();
                row.source.control_id = touched != kNoControl ? engine.surface().at(touched).id : "";
                row.destination.target = dest;
                row.name = (row.source.control_id.empty() ? "?" : row.source.control_id) + " -> " + dest;
                frame.mapping_selected = static_cast<int>(mapping.add(row));
                frame.mapping_listen = true;  // the NEXT move names it, even if one was touched
                frame.mappings_dirty = true;
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("puis bouger un potard de la table : il pilote %s", table[p].name);
        }
        pop_font();
        ImGui::PopID();
        ImGui::EndGroup();
    }

    // --- sync --------------------------------------------------------------------------
    ImGui::SameLine(0.0f, 16.0f);
    ImGui::BeginGroup();
    if (toggle("Sync tempo", unit.sync.tempo)) unit.sync.tempo = !unit.sync.tempo;
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Time devient une division du temps, depuis le BPM");
    {
        static const char* const kDivs[] = {"1/8", "1/4", "1/2", "1", "2"};
        const double divs[] = {0.125, 0.25, 0.5, 1.0, 2.0};
        int which = -1;
        for (int d = 0; d < 5; ++d) if (std::fabs(unit.sync.beats - divs[d]) < 1e-6) which = d;
        push_small();
        if (segmented("div", kDivs, 5, which)) {
            unit.sync.beats = divs[which];
            unit.sync.tempo = true;
        }
        pop_font();
    }
    if (is_multi_tap_effect(unit.type)) {
        const int moments = frame.tap_moments[slot < 3 ? slot : 2];
        push_small();
        text_c(moments > 1 ? kSage : kFaint, "%d moment%s lus", moments, moments == 1 ? "" : "s");
        pop_font();
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("les tra\xC3\xAEn\xC3\xA9""es lisent le clip \xC3\xA0 plusieurs moments ;\n"
                              "un seul = le disque ne bouge pas, rien \xC3\xA0 tra\xC3\xAEner");
        }
    } else if (!is_single_frame_effect(unit.type)) {
        push_small();
        text_c(kAmber, "pas encore \xC3\xA0 l'image");
        pop_font();
    }
    ImGui::EndGroup();

    ImGui::EndChild();
    ImGui::PopID();
}

void draw_effects_screen(Engine& engine, Frame& frame) {
    const float right_w = 360.0f;
    const float gap = ImGui::GetStyle().ItemSpacing.x;
    const float left_w = std::max(560.0f, ImGui::GetContentRegionAvail().x - right_w - gap);

    ImGui::BeginChild("rack", ImVec2(left_w, 0.0f));
    eyebrow("RACK");
    ImGui::SameLine(0.0f, 10.0f);
    push_small();
    dim("trois emplacements, dans l'ordre du signal \xC2\xB7 un emplacement inactif laisse passer l'image");
    pop_font();
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 110.0f);
    if (button("Tout couper")) {
        for (std::size_t i = 0; i < engine.rack().size(); ++i) engine.rack().at(i).enabled = false;
    }
    ImGui::Dummy(ImVec2(0.0f, 6.0f));
    for (std::size_t slot = 0; slot < engine.rack().size(); ++slot) {
        draw_effect_card(engine, frame, slot);
        ImGui::Dummy(ImVec2(0.0f, 4.0f));
    }

    ImGui::Dummy(ImVec2(0.0f, 8.0f));
    eyebrow("CATALOGUE \xE2\x80\x94 chaque effet audio et son pendant visuel");
    push_small();
    ImGui::PushStyleColor(ImGuiCol_Text, rgba(kFaint));
    ImGui::TextUnformatted("\xE2\x89\x88 : la correspondance est un analogue perceptif choisi, jamais pr\xC3\xA9sent\xC3\xA9 comme identique (core/effect.cpp le dit aussi). "
                           "Choisir un effet dans une carte le charge.");
    ImGui::PopStyleColor();
    pop_font();
    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    if (ImGui::BeginTable("catalogue", 4, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("effet", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableSetupColumn("audio", ImGuiTableColumnFlags_WidthFixed, 220.0f);
        ImGui::TableSetupColumn("video", ImGuiTableColumnFlags_WidthFixed, 260.0f);
        ImGui::TableSetupColumn("lien", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        for (const EffectDescriptor& fx : effect_catalogue()) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            text_c(kInk, "%s", fx.id);
            ImGui::TableSetColumnIndex(1);
            push_small();
            text_c(fx.audio != nullptr ? kMuted : kFaint, "%s", fx.audio != nullptr ? fx.audio : "\xE2\x80\x94");
            pop_font();
            ImGui::TableSetColumnIndex(2);
            push_small();
            text_c(kMuted, "%s", fx.video);
            pop_font();
            ImGui::TableSetColumnIndex(3);
            push_small();
            switch (fx.relation) {
                case Correspondence::Identical: text_c(kSage, "identique"); break;
                case Correspondence::Analogue: text_c(kAmber, "\xE2\x89\x88 analogue"); break;
                case Correspondence::VideoOnly: text_c(kFaint, "vid\xC3\xA9o seule"); break;
            }
            pop_font();
        }
        ImGui::EndTable();
    }
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("side", ImVec2(right_w, 0.0f));
    // The audio side of the correspondence, live: the temporal reading made
    // visible, and where a performer picks which band to point at what.
    draw_spectrum(engine);
    ImGui::Dummy(ImVec2(0.0f, 14.0f));
    eyebrow("R\xC3\x89""ACTIF AU SON");
    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    {
        int shown = 0;
        for (std::size_t i = 0; i < engine.mapping().size(); ++i) {
            const Mapping& row = engine.mapping().at(i);
            if (row.source.kind != SourceKind::AudioBand && row.source.kind != SourceKind::Modulator) continue;
            ++shown;
            const DestinationSpec* spec = describe_destination(resolve_destination(row.destination.target));
            push_small();
            text_c(row.enabled ? kInk : kFaint, "%s \xE2\x86\x92 %s", assigned_source_text(row).c_str(),
                   spec != nullptr ? spec->about : row.destination.target.c_str());
            pop_font();
        }
        push_small();
        if (shown == 0) dim("aucune : une liaison depuis une bande audio ou un LFO, dans TABLE");
        else dim("\xC3\xA9""dition dans TABLE, « ce que fait la table »");
        pop_font();
    }
    ImGui::EndChild();
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

// What the desk does: every mapping, source to destination, with its live
// value. It sits on the TABLE screen because "what does this knob do" and
// "where is this knob" are one question. Editing comes with the pads lot.
void draw_mapping_list(Engine& engine, Frame& frame) {
    MappingEngine& mapping = engine.mapping();
    eyebrow("CE QUE FAIT LA TABLE");
    ImGui::SameLine(0.0f, 10.0f);
    push_small();
    ImGui::PushStyleColor(ImGuiCol_Text, rgba(kFaint));
    ImGui::Text("mapping.json \xC2\xB7 %zu liaisons", mapping.size());
    ImGui::PopStyleColor();
    pop_font();
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 90.0f);
    if (button("+ Liaison", Icon::None, true)) {
        // A new row starts from the last control touched, so "turn the knob,
        // press +" is the whole gesture.
        Mapping row;
        const ControlIndex touched = engine.surface().last_touched();
        row.source.kind = SourceKind::Control;
        row.source.control_id = touched != kNoControl ? engine.surface().at(touched).id : "";
        row.destination.target = "fx.1.mix";
        row.name = (row.source.control_id.empty() ? std::string("?") : row.source.control_id) +
                   " -> " + row.destination.target;
        frame.mapping_selected = static_cast<int>(mapping.add(row));
        frame.mapping_listen = row.source.control_id.empty();
        frame.mappings_dirty = true;
    }
    ImGui::Dummy(ImVec2(0.0f, 6.0f));

    const float editor_h = 250.0f;
    ImGui::BeginChild("rows", ImVec2(0.0f, std::max(120.0f, ImGui::GetContentRegionAvail().y - editor_h)));
    if (ImGui::BeginTable("mappings", 5,
                          ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg |
                              ImGuiTableFlags_ScrollY)) {
        ImGui::TableSetupColumn("on", ImGuiTableColumnFlags_WidthFixed, 24.0f);
        ImGui::TableSetupColumn("source", ImGuiTableColumnFlags_WidthFixed, 170.0f);
        ImGui::TableSetupColumn("destination", ImGuiTableColumnFlags_WidthFixed, 190.0f);
        ImGui::TableSetupColumn("valeur", ImGuiTableColumnFlags_WidthFixed, 64.0f);
        ImGui::TableSetupColumn("x", ImGuiTableColumnFlags_WidthFixed, 24.0f);

        int to_remove = -1;
        for (std::size_t i = 0; i < mapping.size(); ++i) {
            Mapping& row = mapping.mutable_at(i);
            const bool selected = frame.mapping_selected == static_cast<int>(i);
            ImGui::TableNextRow();
            if (selected) {
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, kPanel);
            }
            ImGui::PushID(static_cast<int>(i));
            ImGui::TableSetColumnIndex(0);
            {
                // The enable dot: sage on, hair off. A row that is off is
                // kept, so a wrong knob can be silenced without losing it.
                ImDrawList* draw = ImGui::GetWindowDrawList();
                const ImVec2 at = ImGui::GetCursorScreenPos();
                if (ImGui::InvisibleButton("on", ImVec2(18.0f, ImGui::GetTextLineHeight()))) {
                    row.enabled = !row.enabled;
                    frame.mappings_dirty = true;
                }
                draw->AddCircleFilled(ImVec2(at.x + 8.0f, at.y + ImGui::GetTextLineHeight() * 0.5f), 4.0f,
                                      row.enabled ? (mapping.active(i) ? kSage : kAmber) : kHair);
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip(row.enabled ? (mapping.active(i) ? "active \xC2\xB7 clic : couper"
                                                                        : "activ\xC3\xA9""e mais sans valeur (contr\xC3\xB4le jamais touch\xC3\xA9 ?) \xC2\xB7 clic : couper")
                                                  : "coup\xC3\xA9""e \xC2\xB7 clic : activer");
                }
            }
            ImGui::TableSetColumnIndex(1);
            {
                std::string source;
                switch (row.source.kind) {
                    case SourceKind::Control: source = row.source.control_id.empty() ? "?" : row.source.control_id; break;
                    case SourceKind::Modulator: source = "modulateur " + std::to_string(row.source.index + 1); break;
                    case SourceKind::AudioBand: source = "bande audio " + std::to_string(row.source.index + 1); break;
                    case SourceKind::Gesture: source = "geste"; break;
                    default: source = std::string(source_text(row.source.kind)) + (row.source.deck == 0 ? " A" : " B"); break;
                }
                push_mono();
                if (ImGui::Selectable(source.c_str(), selected, ImGuiSelectableFlags_SpanAllColumns)) {
                    frame.mapping_selected = static_cast<int>(i);
                    frame.mapping_listen = false;
                }
                pop_font();
            }
            ImGui::TableSetColumnIndex(2);
            {
                const DestinationSpec* spec = describe_destination(resolve_destination(row.destination.target));
                text_c(row.enabled ? kInk : kFaint, "%s", spec != nullptr ? spec->about : row.destination.target.c_str());
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", row.destination.target.c_str());
            }
            ImGui::TableSetColumnIndex(3);
            push_mono();
            text_c(row.enabled ? kMuted : kFaint, "%7.2f", static_cast<double>(mapping.value(i)));
            pop_font();
            ImGui::TableSetColumnIndex(4);
            push_small();
            if (ImGui::SmallButton("\xC3\x97")) to_remove = static_cast<int>(i);
            pop_font();
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("supprimer cette liaison");
            ImGui::PopID();
        }
        ImGui::EndTable();
        if (to_remove >= 0) {
            mapping.remove(static_cast<std::size_t>(to_remove));
            if (frame.mapping_selected == to_remove) frame.mapping_selected = -1;
            else if (frame.mapping_selected > to_remove) --frame.mapping_selected;
            frame.mappings_dirty = true;
        }
    }
    ImGui::EndChild();

    // --- the selected row -------------------------------------------------------------
    ImGui::BeginChild("editor", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders);
    eyebrow("LIAISON S\xC3\x89LECTIONN\xC3\x89""E");
    if (frame.mapping_selected < 0 || static_cast<std::size_t>(frame.mapping_selected) >= mapping.size()) {
        push_small();
        dim("cliquer une ligne, ou + Liaison");
        pop_font();
        ImGui::EndChild();
        return;
    }
    Mapping& row = mapping.mutable_at(static_cast<std::size_t>(frame.mapping_selected));
    bool changed = false;
    ImGui::Dummy(ImVec2(0.0f, 4.0f));

    row_label("CONTR\xC3\x94LE");
    ImGui::SameLine(0.0f, 8.0f);
    {
        const float dy = (kControlHeight - ImGui::GetTextLineHeight()) * 0.5f;
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + dy);
        push_mono();
        if (row.source.kind == SourceKind::Control) {
            text_c(row.source.control_id.empty() ? kAmber : kInk, "%s",
                   row.source.control_id.empty() ? "aucun" : row.source.control_id.c_str());
        } else {
            text_c(kInk, "%s", source_text(row.source.kind));
        }
        pop_font();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - dy);
    }
    ImGui::SameLine(0.0f, 10.0f);
    push_small();
    if (button(frame.mapping_listen ? "\xE2\x80\xA6 bouger un contr\xC3\xB4le" : "Bouger un contr\xC3\xB4le",
               Icon::None, frame.mapping_listen)) {
        frame.mapping_listen = !frame.mapping_listen;
    }
    pop_font();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("le prochain contr\xC3\xB4le qui bouge sur la table devient la source");
    ImGui::SameLine(0.0f, 10.0f);
    {
        static const char* const kKinds[] = {"Contr\xC3\xB4le", "Position", "Vitesse", "Scratch/s", "LFO", "Audio"};
        const SourceKind kinds[] = {SourceKind::Control, SourceKind::DeckPosition, SourceKind::DeckVelocity,
                                    SourceKind::DeckScratchRate, SourceKind::Modulator, SourceKind::AudioBand};
        int which = 0;
        for (int k = 0; k < 6; ++k) if (kinds[k] == row.source.kind) which = k;
        push_small();
        if (segmented("kind", kKinds, 6, which)) {
            row.source.kind = kinds[which];
            changed = true;
        }
        pop_font();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("d'o\xC3\xB9 vient la valeur : un contr\xC3\xB4le de la table, ou une grandeur d\xC3\xA9riv\xC3\xA9""e du plateau");
    }

    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    row_label("DESTINATION");
    ImGui::SameLine(0.0f, 8.0f);
    {
        const DestinationSpec* current = describe_destination(resolve_destination(row.destination.target));
        ImGui::SetNextItemWidth(300.0f);
        if (ImGui::BeginCombo("##dest", current != nullptr ? current->about : row.destination.target.c_str())) {
            for (const DestinationSpec& spec : destination_catalogue()) {
                if (spec.dest == Dest::None) continue;
                const bool on = current != nullptr && current->dest == spec.dest;
                if (ImGui::Selectable(spec.about, on)) {
                    row.destination.kind = DestinationKind::Local;
                    row.destination.target = spec.id;
                    changed = true;
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s%s", spec.id, spec.trigger ? " \xC2\xB7 d\xC3\xA9""clencheur (front)" : "");
            }
            ImGui::EndCombo();
        }
    }

    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    row_label("COURBE");
    ImGui::SameLine(0.0f, 8.0f);
    {
        static const char* const kCurves[] = {"Lin\xC3\xA9""aire", "Expo", "Log", "Courbe S"};
        int which = static_cast<int>(row.transform.curve);
        push_small();
        if (segmented("curve", kCurves, 4, which)) {
            row.transform.curve = static_cast<CurveKind>(which);
            changed = true;
        }
        pop_font();
    }
    ImGui::SameLine(0.0f, 10.0f);
    push_small();
    if (toggle("Invers\xC3\xA9""e", row.transform.invert)) {
        row.transform.invert = !row.transform.invert;
        changed = true;
    }
    pop_font();

    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    row_label("ZONE MORTE");
    ImGui::SameLine(0.0f, 8.0f);
    ImGui::SetNextItemWidth(120.0f);
    {
        float deadzone = row.transform.deadzone * 100.0f;
        if (ImGui::SliderFloat("##dz", &deadzone, 0.0f, 30.0f, "%.0f %%")) {
            row.transform.deadzone = deadzone / 100.0f;
            changed = true;
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("la part de la course, au centre, qui vaut le milieu : un potard crant\xC3\xA9 tient au neutre");
    }
    ImGui::SameLine(0.0f, 12.0f);
    row_label("LISSAGE");
    ImGui::SameLine(0.0f, 8.0f);
    ImGui::SetNextItemWidth(120.0f);
    if (ImGui::SliderFloat("##sm", &row.transform.smoothing_ms, 0.0f, 500.0f, "%.0f ms")) changed = true;
    ImGui::SameLine(0.0f, 12.0f);
    row_label("SORTIE");
    ImGui::SameLine(0.0f, 8.0f);
    ImGui::SetNextItemWidth(70.0f);
    if (ImGui::InputFloat("##lo", &row.transform.out_lo, 0.0f, 0.0f, "%.2f")) changed = true;
    ImGui::SameLine(0.0f, 4.0f);
    ImGui::SetNextItemWidth(70.0f);
    if (ImGui::InputFloat("##hi", &row.transform.out_hi, 0.0f, 0.0f, "%.2f")) changed = true;
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("la plage de sortie : \xC2\xB1""180 pour un lacet, 0..1 pour un mix");

    if (changed) {
        row.name = (row.source.kind == SourceKind::Control ? row.source.control_id : source_text(row.source.kind)) +
                   std::string(" -> ") + row.destination.target;
        frame.mappings_dirty = true;
    }
    ImGui::EndChild();
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

// Which screen the room sees. The list is what SDL reports now, refreshed by
// the front end about once a second, so a projector switched on mid-set shows
// up without a restart.
void draw_output_displays(Frame& frame) {
    eyebrow("\xC3\x89""CRAN DE SORTIE");
    push_small();
    ImGui::PushStyleColor(ImGuiCol_Text, rgba(kFaint));
    ImGui::TextUnformatted(
        "Le programme part sur l'\xC3\xA9""cran choisi, sans bordure et en plein \xC3\xA9""cran. "
        "C'est la m\xC3\xAAme image que l'aper\xC3\xA7u et que Spout \xE2\x80\x94 un \xC3\xA9""cran "
        "d'une autre forme re\xC3\xA7oit des bandes noires, jamais une image \xC3\xA9tir\xC3\xA9""e.");
    ImGui::PopStyleColor();
    pop_font();
    ImGui::Dummy(ImVec2(0.0f, 6.0f));

    if (frame.displays.empty()) {
        push_small();
        dim("aucun \xC3\xA9""cran d\xC3\xA9tect\xC3\xA9");
        pop_font();
        return;
    }

    for (const Frame::DisplayView& display : frame.displays) {
        ImGui::PushID(static_cast<int>(display.id));
        const bool on = display.is_output;
        push_mono();
        text_c(on ? kSage : kInk, "%s", display.name.c_str());
        pop_font();
        ImGui::SameLine(0.0f, 10.0f);
        push_small();
        ImGui::PushStyleColor(ImGuiCol_Text, rgba(kFaint));
        ImGui::Text("%dx%d  %.0f Hz%s", display.width, display.height,
                    static_cast<double>(display.refresh_hz),
                    display.primary ? "  \xC2\xB7 principal" : "");
        ImGui::PopStyleColor();
        ImGui::SameLine(0.0f, 12.0f);
        if (on) {
            if (chip("SORTIE ICI", true, kSage)) frame.close_output_request = true;
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("clic pour fermer la sortie");
        } else {
            // The screen the interface is on can be chosen too: a fullscreen
            // window over the instrument is a mistake, but it is the
            // performer's to make -- a single-screen rehearsal is a real case.
            if (chip(display.primary ? "envoyer ici (recouvre l'interface)" : "envoyer ici",
                     false)) {
                frame.open_output_display = display.id;
            }
        }
        pop_font();
        ImGui::PopID();
    }

    if (!frame.output_error.empty()) {
        push_small();
        text_c(kAlert, "%s", frame.output_error.c_str());
        pop_font();
    }
    push_small();
    dim("\xC3\x89""chap ferme la sortie avant de quitter l'instrument.");
    pop_font();
    ImGui::Dummy(ImVec2(0.0f, 14.0f));
}

void draw_output_screen(Engine& engine, Frame& frame) {
    if (!g_presets.listed) refresh_presets();

    draw_output_displays(frame);

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
    Mask& mask = engine.mask();
    // The tool follows the engine's mesh switch, so a loaded preset lands on
    // the right one; the mask tool is the panel's own.
    if (frame.output_tool != 2) frame.output_tool = engine.mesh_enabled() ? 1 : 0;
    const bool mesh_on = engine.mesh_enabled();
    const bool mask_tool = frame.output_tool == 2;

    {
        static const char* const kTools[] = {"Coins", "Grille", "Masque"};
        int tool = frame.output_tool;
        row_label("SURFACE");
        ImGui::SameLine(0.0f, 8.0f);
        if (segmented("tool", kTools, 3, tool)) {
            frame.output_tool = tool;
            if (tool != 2) engine.set_mesh_enabled(tool == 1);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Coins : quatre coins, une homographie â un mur plat de travers\n"
                              "Grille : une surface qui n'est pas plane\n"
                              "Masque : ce que le projecteur ne doit pas é""clairer");
        }
        ImGui::SameLine(0.0f, 14.0f);
        row_label(engine.pin().is_identity() && !mesh_on && mask.empty()
                      ? "rien n'est appliqué : l'é""cran de sortie reçoit l'image telle quelle"
                      : "appliqué sur l'é""cran de sortie · Spout reçoit l'image avant");
    }
    ImGui::Dummy(ImVec2(0.0f, 6.0f));

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

    // The mask, over whichever geometry is on: the shape the projector keeps
    // dark. Drawn dim when another tool is up, with handles when it is the
    // tool. Points live in the output's unit square, like the pin's corners.
    {
        const std::vector<Point>& points = mask.points();
        if (points.size() >= 2) {
            for (std::size_t i = 0; i < points.size(); ++i) {
                const Point& a = points[i];
                const Point& b = points[(i + 1) % points.size()];
                draw->AddLine(to_screen(a), to_screen(b), mask_tool ? kSlate : IM_COL32(0x6E, 0x86, 0x96, 0x60),
                              mask_tool ? 2.0f : 1.0f);
            }
        }
        if (mask_tool) {
            std::vector<Point> edited = points;
            bool moved = false;
            for (std::size_t i = 0; i < edited.size(); ++i) {
                char id[16];
                std::snprintf(id, sizeof(id), "mask%zu", i);
                const bool on = frame.mask_selected == static_cast<int>(i);
                if (handle(id, to_screen(edited[i]), on ? kAccent : kSlate, on ? 6.0f : 4.5f)) {
                    edited[i] = from_screen(ImGui::GetIO().MousePos);
                    frame.mask_selected = static_cast<int>(i);
                    moved = true;
                }
            }
            if (moved) mask.set_polygon(std::move(edited));
            if (points.empty()) {
                push_small();
                const char* word = "aucun masque â « + point » en ajoute un ; trois font une forme";
                const ImVec2 size = ImGui::CalcTextSize(word);
                draw->AddText(ImVec2(origin.x + (width - size.x) * 0.5f, origin.y + height - size.y - 8.0f), kMuted, word);
                pop_font();
            }
        }
    }

    ImGui::SetCursorScreenPos(ImVec2(origin.x + width + 24.0f, origin.y));
    ImGui::BeginGroup();

    if (mask_tool) {
        eyebrow("MASQUE");
        ImGui::Dummy(ImVec2(0.0f, 4.0f));
        push_small();
        if (button("+ point")) {
            // A new point after the selected one (or at the end), midway to
            // the next, so the shape grows where the hand is working.
            std::vector<Point> edited = mask.points();
            if (edited.size() < 3) {
                const Point seeds[] = {Point{0.1, 0.1}, Point{0.9, 0.1}, Point{0.9, 0.9}, Point{0.1, 0.9}};
                edited.push_back(seeds[edited.size() % 4]);
            } else {
                const std::size_t at = frame.mask_selected >= 0 && static_cast<std::size_t>(frame.mask_selected) < edited.size()
                                           ? static_cast<std::size_t>(frame.mask_selected)
                                           : edited.size() - 1;
                const Point& a = edited[at];
                const Point& b = edited[(at + 1) % edited.size()];
                edited.insert(edited.begin() + static_cast<std::ptrdiff_t>(at + 1),
                              Point{(a.x + b.x) * 0.5, (a.y + b.y) * 0.5});
                frame.mask_selected = static_cast<int>(at + 1);
            }
            mask.set_polygon(std::move(edited));
        }
        ImGui::SameLine(0.0f, 4.0f);
        if (button("â point", Icon::None, false, 0.0f, kInk, !mask.points().empty())) {
            std::vector<Point> edited = mask.points();
            const std::size_t at = frame.mask_selected >= 0 && static_cast<std::size_t>(frame.mask_selected) < edited.size()
                                       ? static_cast<std::size_t>(frame.mask_selected)
                                       : edited.size() - 1;
            edited.erase(edited.begin() + static_cast<std::ptrdiff_t>(at));
            frame.mask_selected = -1;
            mask.set_polygon(std::move(edited));
        }
        ImGui::SameLine(0.0f, 4.0f);
        if (button("Effacer", Icon::None, false, 0.0f, kInk, !mask.points().empty())) {
            mask.set_polygon({});
            frame.mask_selected = -1;
        }
        pop_font();
        ImGui::Dummy(ImVec2(0.0f, 8.0f));
        row_label("ADOUCIR");
        ImGui::SameLine(0.0f, 8.0f);
        ImGui::SetNextItemWidth(160.0f);
        {
            float feather = static_cast<float>(mask.feather() * 100.0);
            if (ImGui::SliderFloat("##feather", &feather, 0.0f, 20.0f, "%.1f %%")) {
                mask.set_feather(static_cast<double>(feather) / 100.0);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("le bord du masque, en fraction de l'image.\n"
                                  "Rendu sur une grille 32x32 : un bord très fin est un peu plus grossier "
                                  "sur l'é""cran que sur cet aperçu.");
            }
        }
        push_small();
        ImGui::PushStyleColor(ImGuiCol_Text, rgba(kFaint));
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 290.0f);
        ImGui::TextUnformatted("Dedans, l'image ; dehors, du noir. Glisser un point ; le point "
                               "sélectionné est en orange.");
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();
        pop_font();
    } else if (mesh_on) {
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
        preset.mask = mask.points();
        preset.mask_feather = mask.feather();
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
                mask.set_polygon(preset.mask);
                mask.set_feather(preset.mask_feather);
                frame.mask_selected = -1;
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


// --- the mixer column ----------------------------------------------------------
//
// Between the two decks, as the Elite is between the two turntables: the two
// channel faders, the crossfader, and the curves that decide whether a flick
// is a cut or a fade. The mouse writes into the surface through the same door
// MIDI uses. The whole measured table is under TABLE.
void draw_mixer_column(Engine& engine, Frame& frame, float width, float height) {
    ImGui::BeginChild("mixer", ImVec2(width, height), ImGuiChildFlags_Borders);
    const float inner = ImGui::GetContentRegionAvail().x;
    eyebrow("MIXER");
    ImGui::Dummy(ImVec2(0.0f, 6.0f));

    ControlIndex a_index = kNoControl;
    ControlIndex b_index = kNoControl;
    const Control* fader_a = surface_control(engine.surface(), "ch1.fader", &a_index);
    const Control* fader_b = surface_control(engine.surface(), "ch2.fader", &b_index);
    const float fader_scale = 1.4f;
    const float pair_w = 2.0f * 46.0f * fader_scale + 14.0f;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(0.0f, (inner - pair_w) * 0.5f));
    vertical_fader("ch1.fader", "A", fader_a, frame, a_index, kAmber, fader_scale);
    ImGui::SameLine(0.0f, 14.0f);
    vertical_fader("ch2.fader", "B", fader_b, frame, b_index, kSlate, fader_scale);

    ImGui::Dummy(ImVec2(0.0f, 8.0f));
    ControlIndex xf_index = kNoControl;
    const Control* xf = surface_control(engine.surface(), "xfader", &xf_index);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20.0f);
    crossfader(xf, frame, xf_index, engine, inner - 42.0f);

    // The curves. One block per fader family; the Elite's own switches write
    // the same settings, so a change here and a flick there agree.
    MixSettings& mix = engine.mix_settings();
    static const char* const kCurves[] = {"Douce", "Lin\xC3\xA9""aire", "Sharp", "Cut"};
    const auto curve_index = [](FaderCurve curve) {
        switch (curve) {
            case FaderCurve::Smooth: return 0;
            case FaderCurve::Linear: return 1;
            case FaderCurve::Sharp: return 2;
            case FaderCurve::Cut: return 3;
        }
        return 1;
    };
    const auto curve_of = [](int index) {
        return index == 0 ? FaderCurve::Smooth : index == 1 ? FaderCurve::Linear
                          : index == 2 ? FaderCurve::Sharp : FaderCurve::Cut;
    };
    {
        ImGui::Dummy(ImVec2(0.0f, 10.0f));
        eyebrow("COURBE XF");
        ImGui::Dummy(ImVec2(0.0f, 2.0f));
        int index = curve_index(mix.xfader);
        if (segmented("curve", kCurves, 4, index, nullptr, kAccent, true, inner)) {
            mix.xfader = curve_of(index);
            frame.settings_dirty = true;
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("la loi du crossfader : douce (fondu long), lin\xC3\xA9""aire, "
                              "sharp (scratch), cut (tout ou rien).\n"
                              "Les courbes des faders de voie sont dans TABLE, une par voie.");
        }
        if (toggle("Invers\xC3\xA9", mix.xfader_reverse)) {
            mix.xfader_reverse = !mix.xfader_reverse;
            frame.settings_dirty = true;
        }
    }

    // How A becomes B: the transition. A real VJ decision, so it sits with
    // the crossfader rather than in a settings screen.
    ImGui::Dummy(ImVec2(0.0f, 10.0f));
    eyebrow("TRANSITION");
    ImGui::Dummy(ImVec2(0.0f, 2.0f));
    {
        static const char* const kNames[] = {"Cut", "Fondu", "Additif", "Multipli\xC3\xA9", "Screen",
                                             "Wipe luma", "Wipe", "RVB d\xC3\xA9""cal\xC3\xA9", "Zoom"};
        static const char* const kAbout[] = {
            "tout ou rien au milieu de la course",
            "B par-dessus A, \xC3\xA0 son poids",
            "B ajout\xC3\xA9 \xC3\xA0 A : jamais de noir au milieu, le scratch",
            "A, puis les deux multipli\xC3\xA9s, puis B",
            "A, puis les deux en screen, puis B",
            "les pixels sombres de A c\xC3\xA8""dent d'abord",
            "de gauche \xC3\xA0 droite, bord doux",
            "rouge, vert puis bleu passent l'un apr\xC3\xA8s l'autre",
            "A grossit en partant, B monte dessous",
        };
        const int current = static_cast<int>(mix.transition);
        ImGui::SetNextItemWidth(inner);
        if (ImGui::BeginCombo("##transition", kNames[current])) {
            for (int t = 0; t < 9; ++t) {
                if (ImGui::Selectable(kNames[t], t == current)) {
                    mix.transition = static_cast<Transition>(t);
                    frame.settings_dirty = true;
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", kAbout[t]);
            }
            ImGui::EndCombo();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("comment A devient B le long du crossfader\n%s\n"
                              "Fondu et Additif suivent la courbe ; les autres suivent la position",
                              kAbout[current]);
        }
    }

    ImGui::Dummy(ImVec2(0.0f, 8.0f));
    push_small();
    ImGui::PushStyleColor(ImGuiCol_Text, rgba(engine.cuts().transforming() ? kAmber : kFaint));
    ImGui::Text("coupes %.1f/s%s", static_cast<double>(engine.cuts().cuts_per_second()),
                engine.cuts().transforming() ? "  TRANSFORM" : "");
    ImGui::PopStyleColor();
    pop_font();

    ImGui::EndChild();
}

// --- the program strip ---------------------------------------------------------
//
// What leaves the machine, and the two things layered over the decks' mix: the
// overlay, and the rack. The preview IS the texture Spout and the output
// screen carry, so what the interface shows is what a receiver gets.
void draw_program_strip(Engine& engine, Frame& frame, float height) {
    ImGui::BeginChild("program", ImVec2(0.0f, height), ImGuiChildFlags_Borders);
    const float inner_h = ImGui::GetContentRegionAvail().y;
    const float preview_h = inner_h - ImGui::GetTextLineHeight() - 8.0f;
    const float aspect = frame.program_height > 0
                             ? static_cast<float>(frame.program_width) /
                                   static_cast<float>(frame.program_height)
                             : 16.0f / 9.0f;
    const float preview_w = preview_h * aspect;

    ImGui::BeginGroup();
    eyebrow(engine.deck_a().clip.is_equirect() ? "PROGRAMME \xC2\xB7 360 PROJET\xC3\x89"
                                               : "PROGRAMME");
    ImGui::Dummy(ImVec2(0.0f, 2.0f));
    draw_program_view(frame, preview_w, preview_h,
                      frame.output_open ? "ce que voit la salle" : "Spout scratchvj");
    ImGui::EndGroup();

    ImGui::SameLine(0.0f, 18.0f);
    ImGui::BeginGroup();
    // --- the overlay ---------------------------------------------------------
    Layer& overlay = engine.overlay_layer();
    if (toggle("Incrustation", overlay.enabled)) overlay.enabled = !overlay.enabled;
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("la couche par-dessus les deux decks \xE2\x80\x94 un logo, un masque, "
                          "une texture \xE2\x80\x94 hors crossfader");
    }
    ImGui::SameLine(0.0f, 14.0f);
    row_label("OPACIT\xC3\x89");
    ImGui::SameLine(0.0f, 8.0f);
    ImGui::SetNextItemWidth(120.0f);
    ImGui::SliderFloat("##overlay.opacity", &overlay.opacity, 0.0f, 1.0f, "%.2f");
    ImGui::SameLine(0.0f, 14.0f);
    row_label("SOURCE");
    ImGui::SameLine(0.0f, 8.0f);
    {
        // The overlay is the layer that can take a live source: it has no
        // platter, so the question that decides a live DECK -- hold the frame
        // you grabbed, or hold your distance behind the present -- does not
        // arise. A scratchable live deck needs DeckSource::Live, not written.
        static const char* const kOverlaySources[] = {"Clip", "Live Spout"};
        int which = frame.overlay_live ? 1 : 0;
        if (segmented("overlay.source", kOverlaySources, 2, which, nullptr, kSage)) {
            frame.overlay_live = which == 1;
        }
    }
    ImGui::SameLine(0.0f, 14.0f);
    row_label("FUSION");
    ImGui::SameLine(0.0f, 8.0f);
    {
        // How the overlay sits on the mix. Alpha for a logo or a mask with a
        // real alpha channel; screen for a texture that should lift the
        // picture; the rest for taste.
        static const char* const kBlends[] = {"Normal", "Ajout", "Multipli\xC3\xA9", "Screen", "Alpha"};
        int which = static_cast<int>(overlay.blend);
        if (segmented("overlay.blend", kBlends, 5, which, nullptr, kSage)) {
            overlay.blend = static_cast<BlendMode>(which);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("comment l'incrustation se pose sur le mix\n"
                              "Alpha : par sa transparence (choisi tout seul pour un clip avec alpha)\n"
                              "Screen : \xC3\xA9""claircit sans jamais assombrir");
        }
    }
    {
        push_small();
        if (frame.overlay_live) {
            // Said plainly: a receiver waiting for a sender looks exactly
            // like a receiver that is broken.
            if (frame.live_connected && frame.live_is_self) {
                text_c(kAmber, "retour \xE2\x80\x94 sa propre sortie \xC2\xB7 %.1f s",
                       static_cast<double>(frame.live_span_s));
            } else if (frame.live_connected) {
                text_c(kFaint, "%s \xC2\xB7 %.1f s d'historique", frame.live_sender.c_str(),
                       static_cast<double>(frame.live_span_s));
            } else {
                text_c(kAmber, "aucun sender Spout");
            }
        } else if (engine.overlay().clip.frame_count > 0) {
            text_c(kFaint, "%s \xC2\xB7 %s", engine.overlay().name.c_str(),
                   clock_of(engine.overlay().played.position_s).c_str());
        } else {
            text_c(kFaint, "incrustation : aucun clip \xE2\x80\x94 bouton Incr. dans la biblioth\xC3\xA8que");
        }
        pop_font();
    }

    // --- the rack, in one line ----------------------------------------------
    ImGui::Dummy(ImVec2(0.0f, 8.0f));
    row_label("RACK");
    for (std::size_t i = 0; i < engine.rack().size(); ++i) {
        const EffectUnit& unit = engine.rack().at(i);
        const EffectDescriptor* info = describe(unit.type);
        if (info == nullptr) continue;
        ImGui::SameLine(0.0f, 8.0f);
        char label[64];
        std::snprintf(label, sizeof(label), "%zu \xC2\xB7 %s  %.0f %%   ", i + 1, info->id,
                      static_cast<double>(unit.video_params().mix) * 100.0);
        ImGui::PushID(static_cast<int>(i));
        // Only the UNLINKED state is signalled. Linked is the norm and the
        // whole idea of the rack; unlinked is the one state where the two
        // domains say different things.
        if (button(label, Icon::None, false, 0.0f, unit.enabled ? kInk : kFaint)) {
            frame.screen_request = Screen::Effects;
            frame.screen_requested = true;
        }
        {
            ImDrawList* draw = ImGui::GetWindowDrawList();
            const ImVec2 hi = ImGui::GetItemRectMax();
            const ImVec2 lo = ImGui::GetItemRectMin();
            draw->AddCircleFilled(ImVec2(hi.x - 9.0f, (lo.y + hi.y) * 0.5f), 3.5f,
                                  unit.link ? kSage : kAlert);
        }
        if (ImGui::IsItemHovered()) {
            const bool tapped = is_multi_tap_effect(unit.type);
            const int moments = frame.tap_moments[i < 3 ? i : 2];
            ImGui::SetTooltip("%s\n%s%s%s\nclic : ouvrir EFFETS", info->video != nullptr ? info->video : info->id,
                              unit.link ? "audio et vid\xC3\xA9o li\xC3\xA9s" : "D\xC3\x89LI\xC3\x89S : la vid\xC3\xA9o suit ce potard, l'audio non",
                              tapped ? "\n" : "", tapped ? (moments > 1 ? "trainées actives" : "une seule image lue : le disque ne bouge pas") : "");
        }
        ImGui::PopID();
    }
    ImGui::SameLine(0.0f, 12.0f);
    row_label("vert : audio et vid\xC3\xA9o li\xC3\xA9s \xC2\xB7 rouge : d\xC3\xA9li\xC3\xA9s");

    // --- the mix, read --------------------------------------------------------
    ImGui::Dummy(ImVec2(0.0f, 10.0f));
    const StackWeights stack = engine.stack();
    push_mono();
    text_c(kAmber, "A");
    pop_font();
    ImGui::SameLine(0.0f, 8.0f);
    meter(stack.a, 110.0f, kAmber, false, 7.0f);
    ImGui::SameLine(0.0f, 14.0f);
    push_mono();
    text_c(kSlate, "B");
    pop_font();
    ImGui::SameLine(0.0f, 8.0f);
    meter(stack.b, 110.0f, kSlate, false, 7.0f);
    ImGui::SameLine(0.0f, 18.0f);
    push_small();
    {
        const double vram_gb =
            static_cast<double>(engine.deck_a().window.capacity()) *
            static_cast<double>(block_bytes_per_frame(engine.deck_a().clip.width,
                                                      engine.deck_a().clip.height,
                                                      engine.deck_a().clip.format)) /
            (1024.0 * 1024.0 * 1024.0);
        text_c(kFaint, "m\xC3\xA9moire vid\xC3\xA9o %.2f Go \xC2\xB7 %d passe%s d'effets", vram_gb,
               frame.effect_passes, frame.effect_passes == 1 ? "" : "s");
    }
    pop_font();
    ImGui::EndGroup();

    ImGui::EndChild();
}

// --- JOUER ------------------------------------------------------------------------
//
// The mirror of the desk: deck A, the mixer, deck B, as the Elite sits between
// the two turntables. The library rail folds away (B); F leaves nothing but
// the program, for the screen the audience sees or for judging the picture
// with no interface in the way.
void draw_play_screen(Engine& engine, Frame& frame) {
    if (!ImGui::GetIO().WantTextInput) {
        if (ImGui::IsKeyPressed(ImGuiKey_F, false)) frame.full_frame = !frame.full_frame;
        if (ImGui::IsKeyPressed(ImGuiKey_B, false)) frame.rail_open = !frame.rail_open;
    }
    if (frame.full_frame) {
        draw_program_view(frame, ImGui::GetContentRegionAvail().x,
                          ImGui::GetContentRegionAvail().y, nullptr);
        return;
    }

    const float gap = ImGui::GetStyle().ItemSpacing.x;
    const float program_h = 232.0f;
    const float body_h = std::max(360.0f, ImGui::GetContentRegionAvail().y - program_h - gap);
    const float rail = frame.rail_open ? 250.0f : 0.0f;
    const float mixer_w = 176.0f;
    const float decks_w =
        std::max(520.0f, ImGui::GetContentRegionAvail().x - rail - (rail > 0.0f ? gap : 0.0f) -
                             mixer_w - 2.0f * gap);
    const float deck_w = decks_w * 0.5f;

    if (frame.rail_open) {
        draw_library(engine, frame, rail, body_h);
        ImGui::SameLine();
    }
    draw_deck(engine.deck_a(), engine, frame, frame.tex_a, kAmber, true, deck_w, body_h);
    ImGui::SameLine();
    draw_mixer_column(engine, frame, mixer_w, body_h);
    ImGui::SameLine();
    draw_deck(engine.deck_b(), engine, frame, frame.tex_b, kSlate, false, deck_w, body_h);

    draw_program_strip(engine, frame, program_h);
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

    // Six screens, one row. JOUER is the one a set lives in; the others are
    // preparation and configuration, which is why they can afford to be
    // screens at all instead of fighting for the same pixels. A panel that
    // wants another screen (the rack summary opening EFFETS) asks through
    // the frame and this selects the tab once.
    const auto flags_for = [&](Screen screen) {
        return frame.screen_requested && frame.screen_request == screen
                   ? ImGuiTabItemFlags_SetSelected
                   : ImGuiTabItemFlags_None;
    };
    if (ImGui::BeginTabBar("screens")) {
        if (ImGui::BeginTabItem("JOUER", nullptr, flags_for(Screen::Play))) {
            draw_play_screen(engine, frame);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("BIBLIOTH\xC3\x88QUE", nullptr, flags_for(Screen::Library))) {
            draw_library_screen(engine, frame);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("EFFETS", nullptr, flags_for(Screen::Effects))) {
            draw_effects_screen(engine, frame);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("TABLE", nullptr, flags_for(Screen::Table))) {
            draw_table_screen(engine, frame);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("SORTIE", nullptr, flags_for(Screen::Output))) {
            draw_output_screen(engine, frame);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("R\xC3\x89GLAGES", nullptr, flags_for(Screen::Settings))) {
            draw_settings_screen(engine, frame);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    frame.screen_requested = false;

    ImGui::End();
}

}  // namespace svj::ui
