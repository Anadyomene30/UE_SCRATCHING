// scratchvj — the video effect rack, on the GPU.
//
// Runs the rack's slots over the program, one pass each, ping-ponging between
// two targets so a slot never reads the target it is writing. Slots whose
// effect this pass cannot do alone (delay trails, slit scan -- they read the
// clip at several positions) are SKIPPED rather than approximated: the honest
// behaviour is that nothing happens, not that something almost right happens.
//
// The shader is a transcription of core/videofx, and tools/fx_check holds it
// there. That is the same arrangement as the compositor against core/compose
// and the 360 pass against core/sphere: the arithmetic lives in core with
// tests, the GPU runs it, and a tool proves they still agree.
#pragma once

#include <cstdint>

#include "core/effect.h"
#include "core/videofx.h"

namespace svj {
class EffectRack;
}

namespace svj::ui {

// As many passes as the rack has slots. Two targets still suffice -- a pass
// alternates between them and so never reads the one it writes.
inline constexpr int kMaxEffectPasses = 4;

class EffectsGpu {
public:
    // Takes kMaxPasses consecutive view ids from `first_view_id`, all of which
    // must order after the compositor and before the interface. One view PER
    // PASS rather than two reused: bgfx does not promise the order of two draws
    // submitted to the same view, and an effect chain whose passes can swap is
    // an effect chain that produces a different picture on a different driver.
    bool init(std::uint32_t width, std::uint32_t height, std::uint16_t first_view_id);
    void destroy();
    bool ready() const { return ready_; }

    // Runs every active slot of `rack` over `source` (a bgfx texture handle
    // index). Returns the handle index holding the result -- `source` itself
    // when no slot did anything, so an idle rack costs one comparison and no
    // passes at all.
    std::uint16_t render(std::uint16_t source, const EffectRack& rack);

    // How many passes the last render actually ran, for the interface to show.
    int passes() const { return passes_; }

private:
    void run_pass(std::uint16_t source, int pass, const VideoFx& fx);

    std::uint32_t width_ = 0;
    std::uint32_t height_ = 0;
    std::uint16_t view_id_ = 0;
    bool ready_ = false;
    int passes_ = 0;

    std::uint16_t targets_[2] = {0xFFFF, 0xFFFF};
    std::uint16_t framebuffers_[2] = {0xFFFF, 0xFFFF};
    std::uint16_t program_ = 0xFFFF;
    std::uint16_t sampler_ = 0xFFFF;
    std::uint16_t fx_uniform_ = 0xFFFF;
    std::uint16_t texel_uniform_ = 0xFFFF;
    std::uint16_t vertices_ = 0xFFFF;
};

}  // namespace svj::ui
