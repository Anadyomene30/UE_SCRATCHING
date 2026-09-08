#include "app/analyze.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "core/bc1.h"
#include "core/bc3.h"
#include "core/cachemeta.h"
#include "core/videocache.h"

#ifdef _WIN32
#define SVJ_POPEN _popen
#define SVJ_PCLOSE _pclose
#define SVJ_POPEN_READ "rb"  // binary, or Windows swallows 0x1A and mangles 0x0A
#else
#define SVJ_POPEN popen
#define SVJ_PCLOSE pclose
#define SVJ_POPEN_READ "r"
#endif

namespace svj {
namespace {

// Runs a command and returns its stdout as text. Used for ffprobe only, whose
// whole answer fits in a few lines.
bool run_text(const std::string& command, std::string& out, std::string& error) {
    FILE* pipe = SVJ_POPEN(command.c_str(), SVJ_POPEN_READ);
    if (pipe == nullptr) {
        error = "impossible de lancer: " + command;
        return false;
    }
    char buffer[512];
    out.clear();
    while (std::fgets(buffer, sizeof(buffer), pipe) != nullptr) out += buffer;
    const int status = SVJ_PCLOSE(pipe);
    if (status != 0) {
        error = "ffprobe a échoué — ffmpeg est-il installé et dans le PATH ?";
        return false;
    }
    return true;
}

bool parse_u32(std::string_view text, std::uint32_t& out) {
    if (text.empty()) return false;
    std::uint64_t value = 0;
    for (const char c : text) {
        if (c < '0' || c > '9') return false;
        value = value * 10 + static_cast<std::uint64_t>(c - '0');
        if (value > 0xFFFFFFFFull) return false;
    }
    out = static_cast<std::uint32_t>(value);
    return true;
}

std::string_view trimmed(std::string_view text) {
    while (!text.empty() && (text.back() == '\n' || text.back() == '\r' || text.back() == ' ')) {
        text.remove_suffix(1);
    }
    while (!text.empty() && text.front() == ' ') text.remove_prefix(1);
    return text;
}

// The input half of the ffmpeg/ffprobe command line: a file, or a sequence
// pattern with the options the image2 demuxer needs to read it as one clip.
std::string input_args(const AnalyzeOptions& options) {
    std::string args;
    if (options.is_sequence) {
        args += "-framerate " + std::to_string(options.sequence_fps) +
                " -start_number " + std::to_string(options.sequence_start) + " ";
    }
    if (options.is_still) {
        // One frame of the picture. Without -frames:v ffmpeg would emit it
        // once anyway, but saying so is what makes the intent readable.
        args += "-i \"" + options.input + "\" -frames:v 1";
    } else {
        args += "-i \"" + options.input + "\"";
    }
    return args;
}

}  // namespace

bool parse_rate(std::string_view text, std::uint32_t& num, std::uint32_t& den) {
    text = trimmed(text);
    const std::size_t slash = text.find('/');
    std::uint32_t n = 0, d = 1;
    if (slash == std::string_view::npos) {
        if (!parse_u32(text, n)) return false;
    } else {
        if (!parse_u32(text.substr(0, slash), n)) return false;
        if (!parse_u32(text.substr(slash + 1), d)) return false;
    }
    if (n == 0 || d == 0) return false;
    num = n;
    den = d;
    return true;
}

bool parse_probe_kv(std::string_view text, ProbeInfo& out, std::string& error) {
    ProbeInfo info;
    bool have_rate = false;
    std::string rate_text;
    std::size_t start = 0;
    while (start < text.size()) {
        std::size_t end = text.find('\n', start);
        if (end == std::string_view::npos) end = text.size();
        const std::string_view line = trimmed(text.substr(start, end - start));
        start = end + 1;
        const std::size_t eq = line.find('=');
        if (eq == std::string_view::npos) continue;
        const std::string_view key = line.substr(0, eq);
        const std::string_view value = trimmed(line.substr(eq + 1));
        if (value.empty() || value == "N/A") continue;

        if (key == "width") {
            parse_u32(value, info.width);
        } else if (key == "height") {
            parse_u32(value, info.height);
        } else if (key == "pix_fmt") {
            info.pix_fmt = std::string(value);
        } else if (key == "r_frame_rate") {
            // The stream's rate wins over avg_frame_rate, which is a mean over
            // a variable-rate file and lands cues between frames.
            if (parse_rate(value, info.fps_num, info.fps_den)) have_rate = true;
            rate_text = std::string(value);
        } else if (key == "avg_frame_rate") {
            if (!have_rate && parse_rate(value, info.fps_num, info.fps_den)) have_rate = true;
        } else if (key == "duration") {
            // Both the stream and the format section carry one; the first
            // usable figure is kept, and a stream without one falls through to
            // the container's.
            if (info.duration_s <= 0.0) {
                char* end_ptr = nullptr;
                const std::string copy(value);
                const double d = std::strtod(copy.c_str(), &end_ptr);
                if (end_ptr != copy.c_str() && d > 0.0) info.duration_s = d;
            }
        } else if (key == "nb_frames") {
            parse_u32(value, info.nb_frames);
        }
    }

    if (info.width == 0 || info.height == 0) {
        error = "ffprobe n'a pas reconnu de flux vidéo";
        return false;
    }
    if (!have_rate) {
        error = "cadence illisible: " + (rate_text.empty() ? std::string("absente") : rate_text);
        return false;
    }
    out = info;
    return true;
}

bool pix_fmt_has_alpha(std::string_view pix_fmt) {
    if (pix_fmt.empty()) return false;
    // ffmpeg's naming is regular enough to read: an 'a' in the plane list
    // means an alpha plane. yuva420p, yuva444p10le, rgba, bgra, argb, abgr,
    // gbrap, gbrap12le, ya8, ya16be, pal8 (palettes carry alpha).
    if (pix_fmt == "pal8") return true;
    if (pix_fmt.rfind("yuva", 0) == 0) return true;
    if (pix_fmt.rfind("gbrap", 0) == 0) return true;
    if (pix_fmt.rfind("ya", 0) == 0 && pix_fmt.size() > 2 &&
        pix_fmt[2] >= '0' && pix_fmt[2] <= '9') {
        return true;
    }
    // Packed RGB orderings: any of the four letters may lead.
    for (const char* packed : {"rgba", "bgra", "argb", "abgr"}) {
        if (pix_fmt.rfind(packed, 0) == 0) return true;
    }
    return false;
}

std::uint32_t estimated_frames(const ProbeInfo& info) {
    if (info.nb_frames > 0) return info.nb_frames;
    if (info.duration_s > 0.0 && info.fps_num > 0 && info.fps_den > 0) {
        const double frames =
            info.duration_s * static_cast<double>(info.fps_num) / static_cast<double>(info.fps_den);
        if (frames > 0.0 && frames < 4.0e9) return static_cast<std::uint32_t>(std::lround(frames));
    }
    return 0;
}

bool analyze_clip(const AnalyzeOptions& options, AnalyzeResult& result,
                  const AnalyzeProgress& progress, std::string& error) {
    // --- probe ----------------------------------------------------------------
    // Named key=value lines rather than csv: ffprobe writes csv columns in the
    // order of ITS struct, not of the request, and a request that grew from
    // three fields to six is exactly where positional parsing goes wrong.
    std::string probe;
    if (!run_text("ffprobe -v error " +
                      (options.is_sequence
                           ? "-framerate " + std::to_string(options.sequence_fps) +
                                 " -start_number " + std::to_string(options.sequence_start) + " "
                           : std::string()) +
                      "-select_streams v:0 "
                      "-show_entries stream=width,height,r_frame_rate,avg_frame_rate,pix_fmt,"
                      "duration,nb_frames:format=duration "
                      "-of default=noprint_wrappers=1 \"" +
                      options.input + "\"",
                  probe, error)) {
        return false;
    }

    ProbeInfo info;
    if (!parse_probe_kv(probe, info, error)) {
        error += " (" + options.input + ")";
        return false;
    }
    std::uint32_t fps_num = info.fps_num, fps_den = info.fps_den;
    std::uint32_t expected = estimated_frames(info);
    if (options.is_still) {
        // One frame that lasts one second: any position lands on it, and the
        // duration is a number a deck can show without lying about motion.
        fps_num = 1;
        fps_den = 1;
        expected = 1;
    } else if (options.is_sequence) {
        // The files carry no cadence; the given one is the truth.
        const double fps = options.sequence_fps > 0.0 ? options.sequence_fps : 30.0;
        fps_num = static_cast<std::uint32_t>(std::lround(fps * 1000.0));
        fps_den = 1000;
    }
    const std::uint32_t src_w = info.width;
    const std::uint32_t src_h = info.height;

    // Downscale preserving aspect. The height keeps the exact ratio rather than
    // being rounded to a block multiple: the block encoders pad their edge
    // blocks themselves, and stretching the picture to flatter the compressor
    // would be backwards.
    std::uint32_t width = src_w;
    std::uint32_t height = src_h;
    if (width > options.max_width) {
        height = static_cast<std::uint32_t>(
            (static_cast<std::uint64_t>(src_h) * options.max_width + src_w / 2) / src_w);
        width = options.max_width;
        if (height == 0) height = 1;
    }

    // --- open the cache -------------------------------------------------------
    const bool alpha = pix_fmt_has_alpha(info.pix_fmt);
    CacheHeader header;
    header.width = width;
    header.height = height;
    header.fps_num = fps_num;
    header.fps_den = fps_den;
    // BC3 only when the source actually has an alpha plane: it doubles every
    // frame's bytes, and a clip that is opaque anyway would pay that for a
    // channel of solid 255.
    header.format = alpha ? BlockFormat::BC3 : BlockFormat::BC1;
    if (alpha) header.flags |= kCacheAlpha;
    // The 2:1 convention is how equirectangular footage is recognised without
    // metadata; a false positive costs a wrong default the library's per-clip
    // projection override corrects.
    if (src_w == src_h * 2) header.flags |= kCacheEquirect;

    const std::string out_path =
        options.output.empty() ? options.input + ".svcache" : options.output;
    // The blob is sized now and filled later: the thumbnail comes from a
    // frame this pass has not decoded yet, and the blob's length is fixed
    // the moment the first frame goes behind it.
    CacheMeta meta;
    meta.source = options.input;
    meta.thumb_w = kThumbnailWidth;
    meta.thumb_h = thumbnail_height_for(width, height);
    meta.thumb_bc1.assign(block_bytes_per_frame(meta.thumb_w, meta.thumb_h, BlockFormat::BC1),
                          std::uint8_t{0});
    // A third of the way in: past the fade-in and the slate most rushes open
    // on, well before the end. Frame zero when the length is unknown, and it
    // is always taken first so a clip shorter than expected still has one.
    const std::uint32_t thumb_frame = expected / 3;
    bool thumb_taken = false;

    CacheWriter writer;
    if (!writer.open(out_path, header, encode_meta(meta), error)) return false;

    // --- decode over the pipe -------------------------------------------------
    const std::string decode =
        "ffmpeg -v error " + input_args(options) + " -vf scale=" + std::to_string(width) +
        ":" + std::to_string(height) + ":flags=lanczos -f rawvideo -pix_fmt rgba -";
    FILE* pipe = SVJ_POPEN(decode.c_str(), SVJ_POPEN_READ);
    if (pipe == nullptr) {
        error = "impossible de lancer ffmpeg";
        return false;
    }

    const std::size_t frame_rgba = static_cast<std::size_t>(width) * height * 4;
    std::vector<std::uint8_t> rgba(frame_rgba);
    std::vector<std::uint8_t> packed;
    std::uint32_t frames = 0;

    for (;;) {
        std::size_t got = 0;
        while (got < frame_rgba) {
            const std::size_t n = std::fread(rgba.data() + got, 1, frame_rgba - got, pipe);
            if (n == 0) break;
            got += n;
        }
        if (got == 0) break;  // clean end of stream
        if (got < frame_rgba) {
            // A torn last frame means the decode died mid-frame; half a picture
            // in the cache would be a permanent glitch at one exact position.
            SVJ_PCLOSE(pipe);
            error = "flux tronqué en cours de décodage (frame incomplète)";
            return false;
        }

        if (frames == 0 || (frames == thumb_frame && !thumb_taken)) {
            make_thumbnail(rgba.data(), width, height, meta);
            thumb_taken = frames == thumb_frame;
        }
        if (alpha) {
            encode_bc3(rgba.data(), width, height, packed);
        } else {
            encode_bc1(rgba.data(), width, height, packed);
        }
        if (!writer.write_frame(packed.data(), packed.size(), error)) {
            SVJ_PCLOSE(pipe);
            return false;
        }
        ++frames;
        if (progress && (frames % 30 == 0)) progress(frames, expected);
    }
    SVJ_PCLOSE(pipe);

    if (frames == 0) {
        error = "aucune frame décodée — ffmpeg a-t-il pu ouvrir le fichier ?";
        return false;
    }
    if (!writer.rewrite_metadata(encode_meta(meta), error)) return false;
    if (!writer.close(error)) return false;
    if (progress) progress(frames, frames);

    result.width = width;
    result.height = height;
    result.frames = frames;
    result.fps = static_cast<double>(fps_num) / static_cast<double>(fps_den);
    result.equirect = header.is_equirect();
    result.has_alpha = alpha;
    return true;
}

}  // namespace svj
