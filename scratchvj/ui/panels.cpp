#include "panels.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "imgui.h"

namespace svj::ui {
namespace {

// The mockup's palette, as the interface actually uses it. Named by role rather
// than by hue so a later retune changes one table instead of every call site.
const ImU32 kGround = IM_COL32(0x14, 0x14, 0x12, 0xFF);
const ImU32 kPanel = IM_COL32(0x1B, 0x1A, 0x18, 0xFF);
const ImU32 kHair = IM_COL32(0x2E, 0x2D, 0x28, 0xFF);
const ImU32 kInk = IM_COL32(0xE9, 0xE6, 0xDF, 0xFF);
const ImU32 kFaint = IM_COL32(0x60, 0x5D, 0x56, 0xFF);
const ImU32 kAccent = IM_COL32(0xC9, 0x76, 0x2F, 0xFF);
const ImU32 kSage = IM_COL32(0x7E, 0x94, 0x6B, 0xFF);
const ImU32 kAmber = IM_COL32(0xC9, 0x9A, 0x2F, 0xFF);
const ImU32 kAlert = IM_COL32(0xB5, 0x4B, 0x3A, 0xFF);
const ImU32 kSlate = IM_COL32(0x6E, 0x86, 0x96, 0xFF);

ImVec4 rgba(ImU32 c) { return ImGui::ColorConvertU32ToFloat4(c); }

void text_c(ImU32 colour, const char* fmt, ...) IM_FMTARGS(2);
void text_c(ImU32 colour, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    ImGui::PushStyleColor(ImGuiCol_Text, rgba(colour));
    ImGui::TextV(fmt, args);
    ImGui::PopStyleColor();
    va_end(args);
}

// A small caps label, the mockup's one recurring typographic device.
void label(const char* text) {
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

// A horizontal meter. `ghost` draws the track without a fill, which is how an
// absolute knob whose real position is still unknown has to be shown: inventing
// a value would be worse than admitting there is none.
void meter(float value01, float width, ImU32 colour, bool ghost) {
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const float height = ImGui::GetTextLineHeight();
    const float mid = origin.y + height * 0.5f;

    draw->AddRectFilled(ImVec2(origin.x, mid - 3.0f), ImVec2(origin.x + width, mid + 3.0f),
                        kHair);
    if (ghost) {
        // Dashes rather than a bar: visibly not a measurement.
        for (float x = origin.x; x < origin.x + width; x += 7.0f) {
            draw->AddRectFilled(ImVec2(x, mid - 1.0f), ImVec2(x + 3.0f, mid + 1.0f), kFaint);
        }
    } else {
        const float filled = width * std::clamp(value01, 0.0f, 1.0f);
        draw->AddRectFilled(ImVec2(origin.x, mid - 3.0f), ImVec2(origin.x + filled, mid + 3.0f),
                            colour);
    }
    ImGui::Dummy(ImVec2(width, height));
}

// The filmstrip: the clip end to end, with the loop, the hot cues, the portion
// resident in video memory, and the playhead.
//
// The VRAM bracket is the reason this is drawn rather than written out. It is
// the part of the clip that is instantly scratchable, so it tells the performer
// how far they can throw the platter before they hit a load -- a question no
// number answers as fast as a mark in the right place does.
void filmstrip(const Deck& deck, float width) {
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const float height = 26.0f;
    const std::uint32_t frames = deck.clip.frame_count;

    const auto x_of = [&](std::uint32_t frame) {
        const float ratio = frames <= 1 ? 0.0f
                                        : static_cast<float>(frame) /
                                              static_cast<float>(frames - 1);
        return origin.x + width * std::clamp(ratio, 0.0f, 1.0f);
    };

    draw->AddRectFilled(origin, ImVec2(origin.x + width, origin.y + height), kPanel);
    draw->AddRect(origin, ImVec2(origin.x + width, origin.y + height), kHair);

    const FrameRange& resident = deck.window.resident();
    if (!resident.empty()) {
        const float x0 = x_of(resident.first);
        const float x1 = x_of(resident.last());
        draw->AddRectFilled(ImVec2(x0, origin.y + 1.0f), ImVec2(std::max(x1, x0 + 1.5f),
                                                                origin.y + height - 1.0f),
                            IM_COL32(0x2E, 0x2D, 0x28, 0xFF));
        draw->AddLine(ImVec2(x0, origin.y), ImVec2(x0, origin.y + height), kSlate, 1.5f);
        draw->AddLine(ImVec2(x1, origin.y), ImVec2(x1, origin.y + height), kSlate, 1.5f);
    }

    if (deck.transport.loop().active) {
        const float x0 = x_of(deck.clip.frame_at(deck.transport.loop().start_s));
        const float x1 = x_of(deck.clip.frame_at(deck.transport.loop().end_s));
        draw->AddRectFilled(ImVec2(x0, origin.y + 1.0f), ImVec2(x1, origin.y + height - 1.0f),
                            IM_COL32(0xC9, 0x76, 0x2F, 0x30));
    }

    for (int i = 0; i < kHotCueCount; ++i) {
        const HotCue& cue = deck.transport.cue(i);
        if (!cue.set) continue;
        const float x = x_of(deck.clip.frame_at(cue.position_s));
        draw->AddRectFilled(ImVec2(x - 1.0f, origin.y + height - 7.0f),
                            ImVec2(x + 1.0f, origin.y + height), kAccent);
    }

    const float head = x_of(deck.clip.frame_at(deck.played.position_s));
    draw->AddLine(ImVec2(head, origin.y), ImVec2(head, origin.y + height), kInk, 2.0f);

    ImGui::Dummy(ImVec2(width, height));
}

void draw_deck(const Deck& deck, ImU32 accent, float width) {
    ImGui::PushID(deck.name.c_str());
    ImGui::BeginGroup();

    text_c(accent, "%s", deck.name.c_str());

    ImGui::AlignTextToFramePadding();
    label("pos");
    ImGui::SameLine();
    text_c(kInk, "%s", clock_of(deck.played.position_s).c_str());

    ImGui::SameLine();
    label("vit");
    ImGui::SameLine();
    const double velocity = deck.played.velocity;
    text_c(std::fabs(velocity) > 2.5 ? kAmber : kInk, "%6.2fx", velocity);

    ImGui::SameLine();
    label("liaison");
    ImGui::SameLine();
    const TimecodeState& state = deck.timecode.state();
    text_c(link_colour(state.link), "%s", link_text(state.link));
    if (state.platter_stopped) {
        ImGui::SameLine();
        label("[plateau arrete]");
    }
    if (state.link == LinkState::Lost) {
        ImGui::SameLine();
        text_c(kAlert, "<< GELE");
    }

    filmstrip(deck, width);

    label(deck.transport.loop().active ? "boucle active" : "boucle  -");
    ImGui::SameLine();
    label(deck.transport.slip() ? " slip ON" : " slip off");
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, rgba(kFaint));
    ImGui::Text("  vram %.1fs", deck.window.window_seconds(deck.clip.frame_duration_s()));
    ImGui::PopStyleColor();

    ImGui::EndGroup();
    ImGui::PopID();
}

void draw_status(const Engine& engine, const Frame& frame) {
    text_c(kAccent, "scratchvj");
    ImGui::SameLine();
    label(frame.phase.empty() ? "demo" : frame.phase.c_str());
    ImGui::SameLine();
    text_c(kInk, "%s", clock_of(frame.elapsed_s).c_str());

    ImGui::SameLine(0.0f, 28.0f);
    label("mode");
    ImGui::SameLine();
    text_c(kInk, "%s", frame.follower_mode ? "SERATO" : "AUTONOME");

    ImGui::SameLine(0.0f, 28.0f);
    label("bpm");
    ImGui::SameLine();
    text_c(kInk, "%.1f", engine.bpm());

    // Freshness, never drift: what is observable is how long the anchor has been
    // down and how many timecode discontinuities have gone past since. A number
    // of seconds of error would be invented.
    ImGui::SameLine(0.0f, 28.0f);
    label("fraicheur");
    ImGui::SameLine();
    const int jumps = engine.deck_a().timecode.jump_count();
    const float stale = engine.anchor().armed()
                            ? engine.anchor().staleness(frame.elapsed_s, jumps)
                            : 1.0f;
    meter(1.0f - stale, 90.0f, stale > 0.6f ? kAmber : kSage, false);
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, rgba(kFaint));
    ImGui::Text("sauts %d", jumps);
    ImGui::PopStyleColor();
}

void draw_surface(const Surface& surface) {
    label("SURFACE");
    if (ImGui::BeginTable("surface", 3,
                          ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("id", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupColumn("meter", ImGuiTableColumnFlags_WidthFixed, 180.0f);
        ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthFixed, 60.0f);

        for (std::size_t i = 0; i < surface.size(); ++i) {
            const Control& control = surface.at(static_cast<ControlIndex>(i));
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            text_c(control.known ? kInk : kFaint, "%s", control.id.c_str());

            ImGui::TableSetColumnIndex(1);
            meter(control.value, 175.0f, kAccent, !control.known);

            ImGui::TableSetColumnIndex(2);
            // A ghost shows no number at all. This is the whole point: the Elite's
            // pots are absolute, so their real position is unknown until touched,
            // and printing 0.00 would be a fabrication the performer would trust.
            if (control.known) {
                text_c(kInk, "%5.2f", control.value);
            } else {
                text_c(kFaint, "    ?");
            }
        }
        ImGui::EndTable();
    }
}

void draw_mix(Engine& engine) {
    label("MIX");

    const StackWeights stack = engine.stack();
    ImGui::AlignTextToFramePadding();
    label("A");
    ImGui::SameLine();
    meter(stack.a, 120.0f, kAmber, false);
    ImGui::SameLine();
    label("B");
    ImGui::SameLine();
    meter(stack.b, 120.0f, kSlate, false);
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, rgba(engine.cuts().transforming() ? kAmber : kFaint));
    ImGui::Text("  coupes %.1f/s%s", static_cast<double>(engine.cuts().cuts_per_second()),
                engine.cuts().transforming() ? "  TRANSFORM" : "");
    ImGui::PopStyleColor();

    // On its own row: the overlay does not answer to the crossfader, and sharing
    // the row above would say that it did.
    Layer& overlay = engine.overlay_layer();
    ImGui::AlignTextToFramePadding();
    ImGui::Checkbox("incrust", &overlay.enabled);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(140.0f);
    ImGui::SliderFloat("##incrust.opacite", &overlay.opacity, 0.0f, 1.0f, "%.2f");
    ImGui::SameLine();
    meter(stack.overlay, 90.0f, kSage, !overlay.enabled);
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, rgba(kFaint));
    ImGui::Text("%s %s  %s", clock_of(engine.overlay().played.position_s).c_str(),
                engine.overlay().played.reversed ? "<<" : ">>",
                engine.overlay().name.c_str());
    ImGui::PopStyleColor();

    ImGui::Dummy(ImVec2(0.0f, 6.0f));

    if (ImGui::BeginTable("rack", 4,
                          ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("fx", ImGuiTableColumnFlags_WidthFixed, 110.0f);
        ImGui::TableSetupColumn("link", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("audio", ImGuiTableColumnFlags_WidthFixed, 110.0f);
        ImGui::TableSetupColumn("video", ImGuiTableColumnFlags_WidthFixed, 110.0f);

        for (std::size_t i = 0; i < engine.rack().size(); ++i) {
            const EffectUnit& unit = engine.rack().at(i);
            const EffectDescriptor* info = describe(unit.type);
            if (info == nullptr) continue;

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            text_c(kInk, "%s", info->id);

            // Only the UNLINKED state is signalled. Linked is the norm and the
            // whole idea of the rack; marking it everywhere would be noise, and
            // unlinked is the one state where the two domains disagree.
            ImGui::TableSetColumnIndex(1);
            if (unit.link) {
                label("lie");
            } else {
                text_c(kAlert, "delie");
            }

            ImGui::TableSetColumnIndex(2);
            text_c(kFaint, "audio %.2f", static_cast<double>(unit.audio_params().mix));
            ImGui::TableSetColumnIndex(3);
            text_c(kFaint, "video %.2f", static_cast<double>(unit.video_params().mix));
        }
        ImGui::EndTable();
    }
}

void draw_mapping(const Engine& engine) {
    label("MAPPING");
    if (ImGui::BeginTable("mapping", 2,
                          ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthFixed, 260.0f);
        ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthFixed, 90.0f);
        for (std::size_t i = 0; i < engine.mapping().size(); ++i) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            text_c(engine.mapping().active(i) ? kInk : kFaint, "%s",
                   engine.mapping().at(i).name.c_str());
            ImGui::TableSetColumnIndex(1);
            text_c(kInk, "%9.2f", static_cast<double>(engine.mapping().value(i)));
        }
        ImGui::EndTable();
    }
}

}  // namespace

void apply_style() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.FrameRounding = 0.0f;
    style.TabRounding = 0.0f;
    style.ScrollbarRounding = 0.0f;
    style.WindowBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f;
    style.CellPadding = ImVec2(8.0f, 4.0f);
    style.ItemSpacing = ImVec2(10.0f, 7.0f);
    style.WindowPadding = ImVec2(20.0f, 18.0f);

    ImVec4* colours = style.Colors;
    colours[ImGuiCol_WindowBg] = rgba(kGround);
    colours[ImGuiCol_ChildBg] = rgba(kGround);
    colours[ImGuiCol_PopupBg] = rgba(kPanel);
    colours[ImGuiCol_Text] = rgba(kInk);
    colours[ImGuiCol_TextDisabled] = rgba(kFaint);
    colours[ImGuiCol_Border] = rgba(kHair);
    colours[ImGuiCol_Separator] = rgba(kHair);
    colours[ImGuiCol_TableBorderLight] = rgba(kHair);
    colours[ImGuiCol_TableBorderStrong] = rgba(kHair);
    colours[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colours[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.02f);
    colours[ImGuiCol_FrameBg] = rgba(kPanel);
    colours[ImGuiCol_Header] = rgba(kPanel);
}

void draw(Engine& engine, const Frame& frame) {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::Begin("scratchvj", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus);

    draw_status(engine, frame);
    ImGui::SameLine(0.0f, 28.0f);
    label("echap pour quitter");
    ImGui::Separator();

    const float column = std::max(320.0f, (ImGui::GetContentRegionAvail().x - 40.0f) * 0.5f);
    draw_deck(engine.deck_a(), kAmber, column - 20.0f);
    ImGui::SameLine(0.0f, 40.0f);
    draw_deck(engine.deck_b(), kSlate, column - 20.0f);

    ImGui::Dummy(ImVec2(0.0f, 10.0f));
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0.0f, 6.0f));

    ImGui::BeginGroup();
    draw_surface(engine.surface());
    ImGui::EndGroup();

    ImGui::SameLine(0.0f, 40.0f);
    ImGui::BeginGroup();
    draw_mix(engine);
    ImGui::Dummy(ImVec2(0.0f, 10.0f));
    draw_mapping(engine);
    ImGui::EndGroup();

    ImGui::End();
}

}  // namespace svj::ui
