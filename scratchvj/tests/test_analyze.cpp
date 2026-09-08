#include <string>

#include "app/analyze.h"
#include "harness.h"

using namespace svj;

SVJ_TEST("analyze: the probe is read by key, in whatever order ffprobe emits it") {
    // ffprobe writes fields in the order of its own struct, not of the
    // request. The csv reading this replaced assumed the request's order and
    // would have read pix_fmt as a frame rate.
    const char* text =
        "width=1920\n"
        "height=1080\n"
        "pix_fmt=yuv420p\n"
        "r_frame_rate=30000/1001\n"
        "avg_frame_rate=30000/1001\n"
        "duration=12.512500\n"
        "nb_frames=375\n"
        "duration=12.540000\n";  // the format section's, second
    ProbeInfo info;
    std::string error;
    CHECK(parse_probe_kv(text, info, error));
    CHECK_EQ(info.width, 1920u);
    CHECK_EQ(info.height, 1080u);
    CHECK_EQ(info.pix_fmt, std::string("yuv420p"));
    CHECK_EQ(info.fps_num, 30000u);
    CHECK_EQ(info.fps_den, 1001u);
    CHECK_NEAR(info.duration_s, 12.5125, 1e-9);  // the stream's, first
    CHECK_EQ(info.nb_frames, 375u);
}

SVJ_TEST("analyze: N/A values are absent, not zero, and the stream duration falls through") {
    // A Matroska stream reports duration=N/A; the format section has it.
    const char* text =
        "width=640\r\n"
        "height=360\r\n"
        "r_frame_rate=25/1\r\n"
        "duration=N/A\r\n"
        "nb_frames=N/A\r\n"
        "duration=4.000000\r\n";
    ProbeInfo info;
    std::string error;
    CHECK(parse_probe_kv(text, info, error));
    CHECK_NEAR(info.duration_s, 4.0, 1e-9);
    CHECK_EQ(info.nb_frames, 0u);
    CHECK_EQ(estimated_frames(info), 100u);
}

SVJ_TEST("analyze: a probe without a picture or a rate is refused with a reason") {
    ProbeInfo info;
    std::string error;
    CHECK(!parse_probe_kv("codec_name=aac\n", info, error));
    CHECK(error.find("vid") != std::string::npos);
    CHECK(!parse_probe_kv("width=10\nheight=10\nr_frame_rate=0/0\n", info, error));
    CHECK(error.find("0/0") != std::string::npos);
}

SVJ_TEST("analyze: 30000/1001 is read as an exact rational, never as 29.97 rounded") {
    std::uint32_t num = 0, den = 0;
    CHECK(parse_rate("30000/1001", num, den));
    CHECK_EQ(num, 30000u);
    CHECK_EQ(den, 1001u);
    CHECK(parse_rate("30\n", num, den));
    CHECK_EQ(num, 30u);
    CHECK_EQ(den, 1u);
    CHECK(!parse_rate("0/0", num, den));
    CHECK(!parse_rate("abc", num, den));
}

SVJ_TEST("analyze: pixel formats with an alpha plane are told from those without") {
    CHECK(pix_fmt_has_alpha("yuva420p"));
    CHECK(pix_fmt_has_alpha("yuva444p10le"));  // ProRes 4444
    CHECK(pix_fmt_has_alpha("rgba"));
    CHECK(pix_fmt_has_alpha("bgra"));
    CHECK(pix_fmt_has_alpha("argb"));
    CHECK(pix_fmt_has_alpha("gbrap"));
    CHECK(pix_fmt_has_alpha("gbrap12le"));
    CHECK(pix_fmt_has_alpha("ya8"));
    CHECK(pix_fmt_has_alpha("pal8"));
    CHECK(!pix_fmt_has_alpha("yuv420p"));
    CHECK(!pix_fmt_has_alpha("yuv422p10le"));
    CHECK(!pix_fmt_has_alpha("rgb24"));
    CHECK(!pix_fmt_has_alpha("gbrp"));
    CHECK(!pix_fmt_has_alpha("gray"));
    CHECK(!pix_fmt_has_alpha(""));
}

SVJ_TEST("analyze: the frame estimate prefers a stated count, then duration times rate") {
    ProbeInfo info;
    info.fps_num = 24;
    info.fps_den = 1;
    CHECK_EQ(estimated_frames(info), 0u);  // nothing to go on
    info.duration_s = 10.0;
    CHECK_EQ(estimated_frames(info), 240u);
    info.nb_frames = 237;
    CHECK_EQ(estimated_frames(info), 237u);
}
