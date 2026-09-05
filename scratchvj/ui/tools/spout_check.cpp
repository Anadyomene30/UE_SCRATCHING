// scratchvj — proves the Spout output end to end, with no third-party software.
//
// A sender that nothing has ever received is a claim, not a feature. This tool
// is the receiver side of the claim: it connects to the "scratchvj" sender,
// pulls frames for a few seconds, and reports what it saw -- resolution, frame
// count, and mean luminance (so an all-black stream, the classic silent failure
// of texture sharing, fails the check instead of passing it).
//
// Exit codes: 0 frames received and not black, 1 nothing arrived, 2 black.
#include <cstdio>
#include <cstring>
#include <vector>

#include "SpoutDX.h"

int main(int argc, char** argv) {
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
