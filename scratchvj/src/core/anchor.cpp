#include "core/anchor.h"

#include <algorithm>

namespace svj {

void Anchor::set(double timecode_position_s, double clip_position_s, double now_s,
                 int jump_count) {
    offset_s_ = timecode_position_s - clip_position_s;
    placed_at_s_ = now_s;
    placed_at_jump_count_ = jump_count;
    armed_ = true;
}

void Anchor::clear() {
    armed_ = false;
    offset_s_ = 0.0;
    placed_at_s_ = 0.0;
    placed_at_jump_count_ = 0;
}

double Anchor::clip_position(double timecode_position_s) const {
    return timecode_position_s - offset_s_;
}

double Anchor::age_s(double now_s) const {
    if (!armed_) return 0.0;
    return std::max(0.0, now_s - placed_at_s_);
}

int Anchor::jumps_since(int jump_count) const {
    if (!armed_) return 0;
    return std::max(0, jump_count - placed_at_jump_count_);
}

float Anchor::staleness(double now_s, int jump_count) const {
    if (!armed_) return 1.0f;

    const double by_age = config_.stale_after_s > 0.0
                              ? age_s(now_s) / config_.stale_after_s
                              : 0.0;

    const double by_jumps = config_.stale_after_jumps > 0
                                ? static_cast<double>(jumps_since(jump_count)) /
                                      static_cast<double>(config_.stale_after_jumps)
                                : 0.0;

    // The worse of the two, not their sum: either cause alone is enough to have
    // broken the correspondence.
    return static_cast<float>(std::clamp(std::max(by_age, by_jumps), 0.0, 1.0));
}

bool Anchor::needs_reanchor(double now_s, int jump_count) const {
    if (!armed_) return true;
    return staleness(now_s, jump_count) >= config_.reanchor_threshold;
}

}  // namespace svj
