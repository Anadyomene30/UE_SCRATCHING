// scratchvj — the offline analysis pass: any video in, a .svcache out.
//
// This is the Serato move: analyse once, then never decode again. FFmpeg is the
// only way to open "any video", but linking libavcodec is a heavy dependency
// with its own build problems on three platforms — and the analysis pass is a
// BATCH job, not a realtime one. So this drives the ffmpeg EXECUTABLE over a
// pipe instead: it decodes and scales, we read raw RGBA frames from its stdout,
// compress each one to BC1 (core/bc1, tested) -- or BC3 (core/bc3) when the
// source carries alpha -- and append it to the cache. The process boundary
// costs one memcpy per frame and buys us zero link-time dependencies; the
// trade only fails when ffmpeg is not installed, and then it fails with a
// sentence saying exactly that.
//
// Lives in app/, not core/: core stays free of even process-level dependencies,
// and this file is glue — the logic worth testing (BC1/BC3, the cache format,
// the frame arithmetic, the probe parsing below) lives in core or is a pure
// function here with a test.
#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

namespace svj {

struct AnalyzeOptions {
    std::string input;
    std::string output;              // defaults to input with .svcache appended
    std::uint32_t max_width = 1024;  // frames are downscaled to at most this wide

    // A numbered image sequence: `input` is then an ffmpeg pattern such as
    // "frame_%04d.png", and the cadence is not in the files, so it is given.
    bool is_sequence = false;
    std::uint32_t sequence_start = 0;
    double sequence_fps = 30.0;

    // A single picture: one frame, held. A logo for the overlay, typically.
    bool is_still = false;
};

struct AnalyzeResult {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t frames = 0;
    double fps = 0.0;
    bool equirect = false;
    bool has_alpha = false;
};

// What ffprobe said about the first video stream, as this pass reads it.
struct ProbeInfo {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t fps_num = 0;
    std::uint32_t fps_den = 1;
    std::string pix_fmt;
    double duration_s = 0.0;       // 0 when the container does not say
    std::uint32_t nb_frames = 0;   // 0 when the container does not say
};

// Parses ffprobe's `key=value` lines (its `default` writer without wrappers).
// Lines are read by name, so the order ffprobe happens to emit them in -- which
// is the order of its internal struct, not of the request -- cannot break it.
// "N/A" values are treated as absent. Returns false when width, height or a
// usable frame rate is missing.
bool parse_probe_kv(std::string_view text, ProbeInfo& out, std::string& error);

// "30000/1001" -> 30000, 1001; also plain "30". False on zero or garbage.
bool parse_rate(std::string_view text, std::uint32_t& num, std::uint32_t& den);

// Whether an ffmpeg pixel format carries an alpha plane: yuva420p, rgba, argb,
// gbrap, ya8 and their relatives. Decides BC3 over BC1.
bool pix_fmt_has_alpha(std::string_view pix_fmt);

// The frame count the probe lets us expect: nb_frames when the container
// states it, otherwise duration x rate, otherwise zero (unknown). Only ever a
// denominator for a progress bar, never trusted for the cache -- ffprobe's
// counts are estimates for many containers.
std::uint32_t estimated_frames(const ProbeInfo& info);

// Progress: frames done so far, and the estimate of the total (0 = unknown).
using AnalyzeProgress = std::function<void(std::uint32_t done, std::uint32_t estimated)>;

// Runs the pass. `progress` may be null.
bool analyze_clip(const AnalyzeOptions& options, AnalyzeResult& result,
                  const AnalyzeProgress& progress, std::string& error);

}  // namespace svj
