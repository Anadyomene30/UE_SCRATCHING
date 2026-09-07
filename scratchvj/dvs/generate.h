// scratchvj — synthesising a DVS control signal, to test the decoder without a
// turntable.
//
// A decoder that has only ever been tried against one record on one desk is a
// decoder nobody can change safely. This generates the signal xwax's own
// `mktimecode` writes -- same carrier, same amplitude modulation, same LFSR --
// so `tests/test_dvs` can state the property that matters: ENCODE A POSITION,
// DECODE IT, GET THE SAME POSITION BACK.
//
// It is a transcription of mktimecode.c, not an interpretation. Where it differs
// from that file the decoder is right and this is wrong, by construction.
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace svj::dvs {

struct GeneratorSpec {
    std::string definition = "serato_2a";
    unsigned sample_rate = 48000;
    // Where on the record to start, in seconds from timecode zero.
    double position_s = 0.0;
    // Speed ratio. 1.0 is nominal. Only forward speeds are generated: playing a
    // control signal BACKWARDS is not the mirror image of playing it forwards --
    // the bitstream runs through its LFSR in reverse -- and a generator that
    // pretended otherwise would test the decoder against a signal no record can
    // produce. Direction is exercised instead by swapping the channels, which is
    // what actually happens with a reversed RCA pair.
    double pitch = 1.0;
    // Peak amplitude, 0..1. mktimecode writes at 0.5 of full scale, which is
    // what a real record delivers into a phono stage; a weak cartridge is worth
    // testing too.
    double amplitude = 0.5;
    // Swaps left and right. The one knob that inverts every direction reading
    // downstream, and the thing tools/dvs_check calibrates against real gear.
    bool swap_channels = false;
};

// Fills `out` with `frames` interleaved stereo samples. Returns false when the
// definition is unknown.
bool generate_timecode(const GeneratorSpec& spec, std::size_t frames,
                       std::vector<std::int16_t>& out);

}  // namespace svj::dvs
