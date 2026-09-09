// scratchvj — proves the Spout output end to end, with no third-party software.
//
// A sender that nothing has ever received is a claim, not a feature. This tool
// is the receiver side of the claim: it connects to the "scratchvj" sender,
// pulls frames for a few seconds, and reports what it saw -- resolution, frame
// count, and mean luminance (so an all-black stream, the classic silent failure
// of texture sharing, fails the check instead of passing it).
//
// Exit codes: 0 frames received and not black, 1 nothing arrived, 2 black.
//
// `spout_check send [name] [seconds]` runs the OTHER half: it publishes a
// gradient under that name through the very ProgramShare the application uses.
// That exists because "no frame arrived" has two causes that look identical
// from the receiving end -- the application is not publishing, or Spout does
// not work on this machine at all -- and only a sender that is known good can
// tell them apart. Run it in one console and the plain receiver in another.
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <vector>

#include "SpoutDX.h"
#include "share.h"

namespace {

// Every sender the machine currently registers. "No frame arrived" has two
// causes that look identical from the receiving end: nothing is registered
// under that name, or something is registered and not publishing. Naming what
// IS there also catches the third case, which is the one that actually bites
// -- a sender running under a name nobody is asking for.
int run_list() {
    spoutDX receiver;
    if (!receiver.OpenDirectX11()) {
        std::fprintf(stderr, "DX11 indisponible\n");
        return 1;
    }
    const int count = receiver.GetSenderCount();
    std::printf("%d sender%s Spout :\n", count, count == 1 ? "" : "s");
    for (int i = 0; i < count; ++i) {
        char name[256] = {0};
        if (!receiver.GetSender(i, name, sizeof(name))) continue;
        unsigned int width = 0, height = 0;
        HANDLE handle = nullptr;
        DWORD format = 0;
        if (receiver.GetSenderInfo(name, width, height, handle, format)) {
            std::printf("  %-32s %ux%u  format %lu\n", name, width, height,
                        static_cast<unsigned long>(format));
        } else {
            std::printf("  %-32s (pas d'info)\n", name);
        }
    }
    receiver.CloseDirectX11();
    return count > 0 ? 0 : 1;
}

int run_sender(const char* name, double seconds) {
    constexpr unsigned int kW = 640;
    constexpr unsigned int kH = 360;

    svj::ui::ProgramShare sender;
    if (!sender.open(name)) {
        std::fprintf(stderr, "ouverture du sender \"%s\" refusee\n", name);
        return 1;
    }

    // A gradient, never black: the receiver's black check has to be able to
    // pass, or this would prove only half of the path.
    std::vector<std::uint8_t> frame(static_cast<size_t>(kW) * kH * 4, 0);
    for (unsigned int y = 0; y < kH; ++y) {
        for (unsigned int x = 0; x < kW; ++x) {
            std::uint8_t* px = &frame[(static_cast<size_t>(y) * kW + x) * 4];
            px[0] = static_cast<std::uint8_t>(x * 255 / (kW - 1));
            px[1] = static_cast<std::uint8_t>(y * 255 / (kH - 1));
            px[2] = 128;
            px[3] = 255;
        }
    }

    std::printf("sender \"%s\" ouvert, %ux%u, %.0f s\n", name, kW, kH, seconds);
    const auto until = std::chrono::steady_clock::now() +
                       std::chrono::milliseconds(static_cast<long long>(seconds * 1000.0));
    unsigned long long sent = 0;
    while (std::chrono::steady_clock::now() < until) {
        sender.send(frame.data(), kW, kH);
        ++sent;
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    sender.close();
    std::printf("%llu frames publiees\n", sent);
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc > 1 && std::strcmp(argv[1], "list") == 0) return run_list();
    if (argc > 1 && std::strcmp(argv[1], "send") == 0) {
        const char* name = argc > 2 ? argv[2] : "scratchvj";
        const double seconds = argc > 3 ? std::atof(argv[3]) : 10.0;
        return run_sender(name, seconds);
    }

    const char* name = argc > 1 ? argv[1] : "scratchvj";

    spoutDX receiver;
    if (!receiver.OpenDirectX11()) {
        std::fprintf(stderr, "DX11 indisponible\n");
        return 1;
    }
    receiver.SetReceiverName(name);

    std::vector<unsigned char> pixels;
    unsigned int width = 0, height = 0;
    unsigned long long received = 0;
    double luma_sum = 0.0;

    // ~6 seconds of polling at roughly 60 Hz.
    for (int tick = 0; tick < 360; ++tick) {
        if (receiver.ReceiveImage(pixels.empty() ? nullptr : pixels.data(), width,
                                  height)) {
            if (receiver.IsUpdated()) {
                width = receiver.GetSenderWidth();
                height = receiver.GetSenderHeight();
                pixels.assign(static_cast<size_t>(width) * height * 4, 0);
                continue;  // next call fills the freshly sized buffer
            }
            if (!pixels.empty()) {
                ++received;
                if (received % 30 == 1) {  // sample, not every frame
                    unsigned long long sum = 0;
                    for (size_t i = 0; i < pixels.size(); i += 4) {
                        sum += pixels[i] + pixels[i + 1] + pixels[i + 2];
                    }
                    luma_sum += static_cast<double>(sum) /
                                (static_cast<double>(width) * height * 3.0);
                }
            }
        }
        Sleep(16);
    }
    receiver.ReleaseReceiver();
    receiver.CloseDirectX11();

    if (received == 0) {
        std::fprintf(stderr, "aucune frame du sender \"%s\"\n", name);
        return 1;
    }
    const double mean = luma_sum / static_cast<double>((received + 29) / 30);
    std::printf("sender \"%s\"  %ux%u  %llu frames  luminance moyenne %.1f\n", name, width,
                height, received, mean);
    if (mean < 1.0) {
        std::fprintf(stderr, "flux entierement noir : partage suspect\n");
        return 2;
    }
    return 0;
}
