#include "core/transport.h"

#include <cmath>

namespace svj {
namespace {

// Folds a position into [start, end) however far outside it lands, in either
// direction. A scratch inside a short loop can cross it several times in one
// block, so a single wrap would not be enough.
double wrap_into(double value, double start, double length) {
    if (length <= 0.0) return value;
    const double offset = std::fmod(value - start, length);
    return start + (offset < 0.0 ? offset + length : offset);
}

}  // namespace

double BeatGrid::snap(double seconds) const {
    if (!valid()) return seconds;
    const double beat = beat_duration_s();
    const double beats = (seconds - first_beat_s) / beat;
    return first_beat_s + std::round(beats) * beat;
}

void Transport::configure(double clip_duration_s, BeatGrid grid) {
    clip_duration_s_ = clip_duration_s;
    grid_ = grid;
    reset();
}

void Transport::reset() {
    have_platter_ = false;
    platter_s_ = 0.0;
    offset_s_ = 0.0;
    shadow_offset_s_ = 0.0;
    position_s_ = 0.0;
    loop_ = Loop{};
    cues_.fill(HotCue{});
    slip_ = false;
}

double Transport::quantised(double seconds) const {
    return quantise_ && grid_.valid() ? grid_.snap(seconds) : seconds;
}

void Transport::shift(double delta_s) {
    offset_s_ += delta_s;
    // Slip protects the shadow from the jump; without slip the two stay together.
    if (!slip_) shadow_offset_s_ += delta_s;
}

double Transport::map(double platter_position_s) {
    platter_s_ = platter_position_s;
    have_platter_ = true;

    double position = platter_position_s + offset_s_;

    if (loop_.active && loop_.length_s() > 0.0) {
        const double wrapped = wrap_into(position, loop_.start_s, loop_.length_s());
        // Record the wrap as an offset change so the loop stays a pure function
        // of platter position rather than accumulated state.
        const double correction = wrapped - position;
        if (correction != 0.0) {
            offset_s_ += correction;
            if (!slip_) shadow_offset_s_ += correction;
        }
        position = wrapped;
    }

    position_s_ = position;
    return position_s_;
}

double Transport::shadow_position_s() const {
    return platter_s_ + shadow_offset_s_;
}

// ---- loops ------------------------------------------------------------------

void Transport::loop_in(double clip_position_s) {
    loop_.start_s = quantised(clip_position_s);
    loop_.active = false;  // not a loop until an out point closes it
}

void Transport::loop_out(double clip_position_s) {
    const double end = quantised(clip_position_s);
    if (end <= loop_.start_s) return;  // a backwards loop is not a loop
    loop_.end_s = end;
    loop_.active = true;
}

void Transport::auto_loop(double beats) {
    if (!grid_.valid() || beats <= 0.0) return;
    const double start = quantised(position_s_);
    loop_.start_s = start;
    loop_.end_s = start + beats * grid_.beat_duration_s();
    loop_.active = true;
}

void Transport::exit_loop() {
    if (!loop_.active) return;
    loop_.active = false;
    // Slip's whole purpose: leaving the loop lands where the clip would have got
    // to, not where the loop left off.
    if (slip_) offset_s_ = shadow_offset_s_;
}

// ---- hot cues ---------------------------------------------------------------

void Transport::set_cue(int index, double clip_position_s, std::uint8_t colour) {
    if (index < 0 || index >= kHotCueCount) return;
    cues_[static_cast<std::size_t>(index)] = HotCue{quantised(clip_position_s), true, colour};
}

void Transport::clear_cue(int index) {
    if (index < 0 || index >= kHotCueCount) return;
    cues_[static_cast<std::size_t>(index)] = HotCue{};
}

const HotCue& Transport::cue(int index) const {
    static const HotCue empty{};
    if (index < 0 || index >= kHotCueCount) return empty;
    return cues_[static_cast<std::size_t>(index)];
}

bool Transport::jump_to_cue(int index) {
    if (index < 0 || index >= kHotCueCount) return false;
    const HotCue& c = cues_[static_cast<std::size_t>(index)];
    if (!c.set) return false;
    shift(c.position_s - position_s_);
    position_s_ = c.position_s;
    return true;
}

// ---- beat jump --------------------------------------------------------------

void Transport::beat_jump(double beats) {
    if (!grid_.valid()) return;
    const double delta = beats * grid_.beat_duration_s();
    shift(delta);
    position_s_ += delta;
}

// ---- slip -------------------------------------------------------------------

void Transport::set_slip(bool on) {
    if (on == slip_) return;
    slip_ = on;
    if (on) {
        // Engaging slip starts the shadow from where playback actually is.
        shadow_offset_s_ = offset_s_;
    } else {
        // Releasing it lands on the shadow, which is the point of the feature.
        offset_s_ = shadow_offset_s_;
        if (have_platter_) position_s_ = platter_s_ + offset_s_;
    }
}

}  // namespace svj
