// scratchvj — reading the clip at several positions at once.
//
// The rack's remaining video effects -- a delay's trails, a slit scan -- cannot
// be made from the frame on screen. They need the clip at several moments, and
// WHICH moments is the whole design problem, so it lives here in core with
// tests rather than inside a shader.
//
// The obvious implementation of trails is a feedback buffer: blend each frame
// into the last. It is also wrong for this instrument, and not by a little. A
// feedback buffer is an INTEGRATOR -- it accumulates whatever it is fed, in the
// order it was fed. Scratch backwards and it keeps piling up forwards; let go
// and it decays on wall-clock time regardless of where the record is. The first
// design principle forbids exactly that.
//
// So the taps are computed instead, from position and velocity:
//
//     tap k = played_position - k * delay_seconds * velocity
//
// which is not an approximation of a delay line but the exact statement of one.
// `delay_seconds * velocity` is how far the clip travels in `delay_seconds` of
// WALL time, so tap k is precisely the frame that was on screen k delays ago.
// At nominal speed it reduces to the audio delay it is paired with, position
// minus k times the delay. At double speed the trail spreads twice as far,
// because the picture really did move twice as far. Run the record backwards
// and the offsets change sign on their own, so the trail swings to the other
// side of the motion the moment the hand reverses. Stop, and the taps collapse
// onto one frame: nothing is moving, so nothing trails.
//
// That last consequence is worth stating rather than patching. An audio delay
// keeps ringing when the record stops, because it remembers. These trails do
// not, because they refuse to. Remembering is what an integrator does.
#pragma once

#include <cstdint>

#include "core/effect.h"
#include "core/playback.h"

namespace svj {

// How many moments a deck can hold at once. Eight is the number that makes a
// slit scan read as one, and eight BC1 frames of a program-sized clip is under
// a megabyte -- the layer count is a video memory decision, not a taste one.
inline constexpr int kTapCount = 8;

struct TapPlan {
    int count = 0;                          // layers actually worth uploading
    double position_s[kTapCount] = {};      // where each layer reads
    float weight[kTapCount] = {};           // the delay's sum; normalised
    bool collapsed = false;                 // every tap on the same frame
};

// The moments `unit` needs, for a deck at `played_s` moving at `velocity`
// (signed, 1.0 nominal). `beat_duration_s` serves a tempo-synced delay.
//
// Positions are folded into the clip with its own play mode, so a trail running
// off the head of a loop comes back from the tail rather than sticking at zero.
TapPlan plan_taps(const EffectUnit& unit, double played_s, double velocity,
                  double beat_duration_s, double clip_duration_s, ClipPlayMode mode);

// True for the effects this machinery serves. The single-frame effects have
// their own path in core/videofx; asking rather than assuming keeps a deck from
// paying for eight uploads to draw one frame.
bool is_multi_tap_effect(EffectType type);

}  // namespace svj
