// scratchvj — the offline analysis pass: any video in, a .svcache out.
//
// This is the Serato move: analyse once, then never decode again. FFmpeg is the
// only way to open "any video", but linking libavcodec is a heavy dependency
// with its own build problems on three platforms — and the analysis pass is a
// BATCH job, not a realtime one. So this drives the ffmpeg EXECUTABLE over a
// pipe instead: it decodes and scales, we read raw RGBA frames from its stdout,
// compress each one to BC1 (core/bc1, tested), and append it to the cache.
// The process boundary costs one memcpy per frame and buys us zero link-time
// dependencies; the trade only fails when ffmpeg is not installed, and then it
// fails with a sentence saying exactly that.
//
// Lives in app/, not core/: core stays free of even process-level dependencies,
// and this file is glue — the logic worth testing (BC1, the cache format, the
// frame arithmetic) already lives in core with tests.
#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace svj {

struct AnalyzeOptions {
    std::string input;
    std::string output;          // defaults to input with .svcache appended
    std::uint32_t max_width = 1024;  // frames are downscaled to at most this wide
};

struct AnalyzeResult {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t frames = 0;
    double fps = 0.0;
    bool equirect = false;
};

// Runs the pass. `progress` (may be null) is called with the frame count so far;
// the total is unknown until the end, which is honest — ffprobe's frame counts
// are estimates for many containers, so none is promised.
bool analyze_clip(const AnalyzeOptions& options, AnalyzeResult& result,
                  const std::function<void(std::uint32_t)>& progress, std::string& error);

}  // namespace svj
