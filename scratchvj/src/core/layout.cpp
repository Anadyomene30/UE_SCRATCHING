#include "core/layout.h"

namespace svj {
namespace {

void push(std::vector<LearnTarget>& out, std::string id, ControlKind kind) {
    out.push_back(LearnTarget{std::move(id), kind});
}

// One channel strip. Ids are hierarchical so the UI can group them without a
// separate table of which control belongs where.
void push_channel(std::vector<LearnTarget>& out, int channel) {
    const std::string prefix = "ch" + std::to_string(channel) + ".";
    push(out, prefix + "trim", ControlKind::Knob);
    push(out, prefix + "eq.hi", ControlKind::Knob);
    push(out, prefix + "eq.mid", ControlKind::Knob);
    push(out, prefix + "eq.low", ControlKind::Knob);
    push(out, prefix + "filter", ControlKind::Knob);
    push(out, prefix + "fader", ControlKind::Fader);
    push(out, prefix + "cue", ControlKind::Button);
    push(out, prefix + "fx.on", ControlKind::Button);
}

void push_pads(std::vector<LearnTarget>& out, const std::string& prefix, int count) {
    for (int i = 1; i <= count; ++i) {
        push(out, prefix + std::to_string(i), ControlKind::Pad);
    }
}

}  // namespace

std::vector<LearnTarget> elite_layout() {
    std::vector<LearnTarget> targets;

    push_channel(targets, 1);
    push_channel(targets, 2);

    push(targets, "xfader", ControlKind::Fader);

    // Effect section. These are the controls that will drive the paired
    // audio/video effect units, so they are worth learning even on day one.
    push(targets, "fx.time", ControlKind::Knob);
    push(targets, "fx.mix", ControlKind::Knob);
    push(targets, "fx.beats", ControlKind::Encoder);
    push(targets, "fx.paddle.a", ControlKind::Button);
    push(targets, "fx.paddle.b", ControlKind::Button);

    // Eight performance pads per deck, plus the mode button above each bank.
    push_pads(targets, "pad.elite.a.", 8);
    push_pads(targets, "pad.elite.b.", 8);
    push(targets, "pad.elite.a.mode", ControlKind::Button);
    push(targets, "pad.elite.b.mode", ControlKind::Button);

    push(targets, "browse.encoder", ControlKind::Encoder);
    push(targets, "browse.load.a", ControlKind::Button);
    push(targets, "browse.load.b", ControlKind::Button);

    push(targets, "master.level", ControlKind::Knob);
    push(targets, "booth.level", ControlKind::Knob);
    push(targets, "cue.mix", ControlKind::Knob);
    push(targets, "cue.level", ControlKind::Knob);

    return targets;
}

std::vector<LearnTarget> rp8000_layout(char deck, int layer) {
    std::vector<LearnTarget> targets;
    const std::string prefix =
        std::string("pad.rp8000.") + deck + ".l" + std::to_string(layer) + ".";
    push_pads(targets, prefix, 8);
    return targets;
}

std::vector<LearnTarget> default_rig_layout() {
    std::vector<LearnTarget> targets = elite_layout();
    for (const char deck : {'a', 'b'}) {
        const std::vector<LearnTarget> pads = rp8000_layout(deck, 1);
        targets.insert(targets.end(), pads.begin(), pads.end());
    }
    return targets;
}

void declare_layout(const std::vector<LearnTarget>& targets, Surface& surface) {
    for (const LearnTarget& target : targets) {
        surface.declare(target.id, target.kind);
    }
}

}  // namespace svj
