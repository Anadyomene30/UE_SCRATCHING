#include "app/analyze.h"

#include <cstdio>
#include <cstring>
#include <vector>

#include "core/bc1.h"
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
// whole answer fits in a line.
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

bool parse_u32(const std::string& text, std::uint32_t& out) {
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

// "30000/1001" -> 29.97; also plain "30".
bool parse_rate(const std::string& text, std::uint32_t& num, std::uint32_t& den) {
    const std::size_t slash = text.find('/');
    std::uint32_t n = 0, d = 1;
    if (slash == std::string::npos) {
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

// Splits "w,h,rate" as ffprobe's csv writer emits it, trailing newline included.
bool parse_probe(const std::string& text, std::uint32_t& w, std::uint32_t& h,
                 std::string& rate) {
    std::string line = text;
    while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) line.pop_back();
    const std::size_t first = line.find(',');
    const std::size_t second = first == std::string::npos ? first : line.find(',', first + 1);
    if (second == std::string::npos) return false;
    if (!parse_u32(line.substr(0, first), w)) return false;
    if (!parse_u32(line.substr(first + 1, second - first - 1), h)) return false;
    rate = line.substr(second + 1);
    return true;
}

}  // namespace

bool analyze_clip(const AnalyzeOptions& options, AnalyzeResult& result,
                  const std::function<void(std::uint32_t)>& progress, std::string& error) {
    // --- probe ----------------------------------------------------------------
    std::string probe;
    if (!run_text("ffprobe -v error -select_streams v:0 "
                  "-show_entries stream=width,height,r_frame_rate "
                  "-of csv=p=0 \"" + options.input + "\"",
                  probe, error)) {
        return false;
    }

    std::uint32_t src_w = 0, src_h = 0;
    std::string rate;
    if (!parse_probe(probe, src_w, src_h, rate) || src_w == 0 || src_h == 0) {
        error = "ffprobe n'a pas reconnu de flux vidéo dans " + options.input;
        return false;
    }
    std::uint32_t fps_num = 30, fps_den = 1;
    if (!parse_rate(rate, fps_num, fps_den)) {
        error = "cadence illisible: " + rate;
        return false;
    }

    // Downscale preserving aspect. The height keeps the exact ratio rather than
    // being rounded to a block multiple: BC1 pads its edge blocks itself, and
    // stretching the picture to flatter the compressor would be backwards.
    std::uint32_t width = src_w;
    std::uint32_t height = src_h;
    if (width > options.max_width) {
        height = static_cast<std::uint32_t>(
            (static_cast<std::uint64_t>(src_h) * options.max_width + src_w / 2) / src_w);
        width = options.max_width;
        if (height == 0) height = 1;
    }

    // --- open the cache -------------------------------------------------------
    CacheHeader header;
    header.width = width;
    header.height = height;
    header.fps_num = fps_num;
    header.fps_den = fps_den;
    header.format = BlockFormat::BC1;
    // The 2:1 convention is how equirectangular footage is recognised without
    // metadata; a false positive costs a wrong default the UI can override.
    if (src_w == src_h * 2) header.flags |= kCacheEquirect;

    const std::string out_path =
        options.output.empty() ? options.input + ".svcache" : options.output;
    std::vector<std::uint8_t> metadata(options.input.begin(), options.input.end());

    CacheWriter writer;
    if (!writer.open(out_path, header, metadata, error)) return false;

    // --- decode over the pipe -------------------------------------------------
    const std::string decode =
        "ffmpeg -v error -i \"" + options.input + "\" -vf scale=" + std::to_string(width) +
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

        encode_bc1(rgba.data(), width, height, packed);
        if (!writer.write_frame(packed.data(), packed.size(), error)) {
            SVJ_PCLOSE(pipe);
            return false;
        }
        ++frames;
        if (progress && (frames % 30 == 0)) progress(frames);
    }
    SVJ_PCLOSE(pipe);

    if (frames == 0) {
        error = "aucune frame décodée — ffmpeg a-t-il pu ouvrir le fichier ?";
        return false;
    }
    if (!writer.close(error)) return false;

    result.width = width;
    result.height = height;
    result.frames = frames;
    result.fps = static_cast<double>(fps_num) / static_cast<double>(fps_den);
    result.equirect = header.is_equirect();
    return true;
}

}  // namespace svj
