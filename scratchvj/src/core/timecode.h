// scratchvj — turning a raw timecode decoder's output into a usable position.
//
// The decoder (xwax's timecoder) reports a position, a pitch and a signal level.
// This layer turns that into something a deck can play from: extrapolated to the
// present, continuous across discontinuities, and honest about what it does not
// know.
//
// The interesting part is the SIGNAL PROFILE. A control record and an MWM Phase
// fail in opposite ways, and reading one as the other produces exactly the wrong
// behaviour on stage:
//
//   Vinyl    — the needle rides a groove. No signal means the platter is STOPPED.
//              That is a normal, constant state during a set.
//   Wireless — the Phase dock synthesises the signal and emits it continuously,
//              even with the platter at a standstill. No signal therefore means
//              the LINK IS LOST: a flat remote battery, or radio trouble.
//
// Same symptom, opposite meaning. Treat a wireless dropout as a stopped platter
// and the picture silently freezes with no explanation; treat a stopped vinyl as
// a dropout and every pause raises a false alarm.
#pragma once

#include <cstdint>

namespace svj {

enum class SignalProfile : std::uint8_t {
    Vinyl,
    Wireless,  // MWM Phase and anything else that synthesises its timecode
};

// How the platter drives playback position.
enum class TransportMode : std::uint8_t {
    Absolute,  // position on the record IS the position in the clip
    Relative,  // the platter contributes movement; discontinuities are absorbed
    Internal,  // the platter is ignored; the deck plays on its own
};

enum class LinkState : std::uint8_t {
    Ok,
    Degraded,  // carrier present but the position bits are unreadable
    Lost,      // no carrier at all
};

struct TimecodeConfig {
    SignalProfile profile = SignalProfile::Wireless;
    TransportMode mode = TransportMode::Relative;

    // Control-signal amplitude below which the carrier counts as absent.
    float silence_level = 0.05f;

    // |pitch| below which the platter counts as standing still.
    float stopped_pitch = 0.02f;

    // A discontinuity is a position step larger than the platter could physically
    // have produced in the time available. The budget therefore scales with the
    // block length: a fixed threshold false-positives whenever the speed changes
    // sharply -- which is to say, during every scratch.
    double max_speed_ratio = 24.0;   // far beyond any real backspin
    double jump_tolerance_s = 0.02;  // slack for decoder jitter

    // Absolute mode glides across a discontinuity over this long instead of
    // snapping, so a replaced remote does not make the picture crack.
    double relock_ramp_s = 0.08;
};

// One reading from the decoder, per audio block.
struct DecoderSample {
    double time_s = 0.0;       // monotonic clock
    double position_s = -1.0;  // position on the control record; negative = no lock
    float pitch = 0.0f;        // signed speed ratio, 1.0 nominal
    float signal_level = 0.0f; // carrier amplitude, 0..1
};

struct TimecodeState {
    double position_s = 0.0;
    float velocity = 0.0f;
    float confidence = 0.0f;
    LinkState link = LinkState::Lost;

    // Only ever true when the carrier is present. On a wireless profile a lost
    // link is NOT a stopped platter, and this stays false to say so.
    bool platter_stopped = false;

    // Set for the one update that observed a discontinuity.
    bool jumped = false;
};

class TimecodeTracker {
public:
    explicit TimecodeTracker(TimecodeConfig config = {}) : config_(config) {}

    void reset();

    // Feeds one decoder reading and returns the resulting state.
    const TimecodeState& submit(const DecoderSample& sample);

    const TimecodeState& state() const { return state_; }
    const TimecodeConfig& config() const { return config_; }

    void set_mode(TransportMode mode);
    void set_profile(SignalProfile profile) { config_.profile = profile; }

    // Number of discontinuities seen since the last reset. Follower mode uses it
    // to judge how stale an anchor has become.
    int jump_count() const { return jump_count_; }

private:
    void hold();
    void advance_output(double dt, float velocity);

    TimecodeConfig config_;
    TimecodeState state_;

    bool have_previous_ = false;
    double previous_time_ = 0.0;
    double previous_raw_position_ = 0.0;

    // Set while the link is down or the bits are unreadable. The first reading
    // after that cannot be compared with the last one from before: we have no
    // idea how far the platter travelled unobserved.
    bool relocking_ = false;

    // Absolute mode glide across a discontinuity.
    bool ramping_ = false;
    double ramp_remaining_ = 0.0;
    double ramp_offset_ = 0.0;

    int jump_count_ = 0;
};

}  // namespace svj
