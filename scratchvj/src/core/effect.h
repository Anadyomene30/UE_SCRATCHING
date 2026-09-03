// scratchvj — the paired effect rack.
//
// An effect unit is not a video effect sitting next to an audio effect. It is ONE
// unit with two implementations driven by the SAME parameters, so a single knob
// echoes the sound and the picture together.
//
// The pairing follows one rule, which is what makes the rack a system rather than
// a pile of effects:
//
//   Audio acts on the temporal frequencies of a 1D signal.
//   Video acts on the spatial frequencies of a 2D one.
//
// Where the correspondence is exact it is implemented as such -- a low-pass filter
// really is a blur, a bitcrusher really is posterisation, a delay really is
// trails, down to the same equation. Where it is not, the nearest perceptual
// analogue is chosen and MARKED as such, so nobody later mistakes a metaphor for
// an identity.
//
// The catalogue below is that table as data. Keeping it in code rather than only
// in prose means the documentation and the implementation cannot drift apart.
#pragma once

#include <cstdint>
#include <vector>

namespace svj {

enum class EffectType : std::uint8_t {
    // Paired: an audio effect and its visual counterpart.
    LowPass, HighPass, DjFilter, Bitcrusher, Decimate, Delay, Distortion,
    RingMod, Gate, Reverse, BeatRepeat, Noise, Reverb, Flanger, PitchShift, Pan,
    // Video only: no audio counterpart exists, and none is invented.
    SlitScan, Datamosh, Feedback, Kaleidoscope, Mirror, Displace, ChromaKey,
    LumaKey, PixelSort, Tile, Polar, Halftone, Vhs, Bloom, HueIsolate, Invert,
};

// Whether the two sides are literally the same operation in another domain, or a
// perceptual analogue that was chosen. Stating this is the honest part.
enum class Correspondence : std::uint8_t {
    Identical,
    Analogue,
    VideoOnly,
};

struct EffectDescriptor {
    EffectType type;
    const char* id;
    const char* audio;  // nullptr for video-only effects
    const char* video;
    Correspondence relation;
};

// The whole catalogue, in one place.
const std::vector<EffectDescriptor>& effect_catalogue();
const EffectDescriptor* describe(EffectType type);

// Parameters an effect unit exposes. Deliberately few and shared across every
// effect: it is what lets one knob mean the same thing everywhere, and what lets
// the audio and video halves be driven from one set of numbers.
struct EffectParams {
    float mix = 0.0f;       // dry to wet
    float amount = 0.5f;    // the effect's main character
    float time = 0.5f;      // delay time, rate, period
    float feedback = 0.0f;
    float depth = 0.5f;
    float tone = 0.5f;
};

// Tempo division for a synced effect. Free means `time` is used directly.
struct Sync {
    bool tempo = false;
    double beats = 1.0;

    // The effect's time in seconds, from the beat duration when synced.
    double seconds(double beat_duration_s, float time_param,
                   double free_max_s = 2.0) const;
};

struct EffectUnit {
    EffectType type = EffectType::Delay;
    bool enabled = false;

    // Linked by default. Unlinking is the ONLY state in which the two domains say
    // different things, which is why it is the only one the interface signals.
    bool link = true;

    EffectParams shared;
    EffectParams audio_override;
    EffectParams video_override;
    Sync sync;

    EffectParams audio_params() const { return link ? shared : audio_override; }
    EffectParams video_params() const { return link ? shared : video_override; }

    // Splits the shared values into the two overrides before unlinking, so the
    // sound does not lurch the instant the chain is broken.
    void unlink();
    void relink();

    bool audio_active() const;
    bool video_active() const;
};

class EffectRack {
public:
    explicit EffectRack(std::size_t slots = 3);

    std::size_t size() const { return units_.size(); }
    EffectUnit& at(std::size_t index) { return units_[index]; }
    const EffectUnit& at(std::size_t index) const { return units_[index]; }

    // Loads an effect into a slot, replacing whatever was there.
    void load(std::size_t index, EffectType type);
    void clear(std::size_t index);

    void set_all_links(bool linked);
    std::size_t active_count() const;

private:
    std::vector<EffectUnit> units_;
};

}  // namespace svj
