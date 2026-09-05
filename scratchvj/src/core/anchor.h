// scratchvj — follower mode: lining a video clip up with what Serato is playing.
//
// In follower mode Serato plays the audio and this application plays only the
// video. Both read the SAME timecode, so once the two are lined up they stay
// lined up: the anchor is just the offset between the position on the control
// record and the position in the clip.
//
// WHAT THIS DELIBERATELY DOES NOT DO IS MEASURE DRIFT.
//
// Serato has no public API, so its real playhead is unreadable. If the DJ loops,
// censors or needle-drops inside Serato, its audio moves and the timecode does
// not -- the correspondence breaks with nothing observable on our side. A number
// of seconds labelled "drift" would therefore be invented, and an invented number
// on stage is worse than no number.
//
// What IS observable is how much has happened since the anchor was placed: how
// long ago, and how many timecode discontinuities have been seen since. That is
// reported as STALENESS -- a measure of risk, not of error. It answers the only
// question that matters in a set: is it time to tap re-anchor?
#pragma once

namespace svj {

struct AnchorConfig {
    // Age alone at which an anchor is considered fully stale.
    double stale_after_s = 300.0;

    // Discontinuities after which it is considered fully stale. One lifted
    // remote is usually enough to have broken the correspondence.
    int stale_after_jumps = 1;

    // Staleness at or above which the UI should call for a re-anchor.
    float reanchor_threshold = 0.5f;
};

class Anchor {
public:
    explicit Anchor(AnchorConfig config = {}) : config_(config) {}

    // Places the anchor: from now on this position on the control record means
    // this position in the clip.
    void set(double timecode_position_s, double clip_position_s, double now_s,
             int jump_count);

    void clear();
    bool armed() const { return armed_; }

    // The clip position implied by a timecode position. Meaningless when unarmed,
    // so callers check armed() first.
    double clip_position(double timecode_position_s) const;

    double offset_s() const { return offset_s_; }
    double placed_at_s() const { return placed_at_s_; }

    double age_s(double now_s) const;
    int jumps_since(int jump_count) const;

    // 0 = just placed and nothing has happened; 1 = assume it no longer holds.
    // Explicitly a risk estimate, never a measured error.
    float staleness(double now_s, int jump_count) const;

    bool needs_reanchor(double now_s, int jump_count) const;

private:
    AnchorConfig config_;
    bool armed_ = false;
    double offset_s_ = 0.0;
    double placed_at_s_ = 0.0;
    int placed_at_jump_count_ = 0;
};

}  // namespace svj
