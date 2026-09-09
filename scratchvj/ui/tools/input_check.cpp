// scratchvj — can this desk's carrier input actually be opened?
//
// WHY THIS EXISTS. `scratchvj_ui` is a WIN32 subsystem application: it has no
// console, and its stderr goes nowhere even when a launcher redirects it. So
// when `--live` opened nothing, the whole report was "nothing happened" -- the
// deck quietly stayed on its own clock and no file said why. That cost an
// afternoon once already, and the interface now carries the reason to a place
// where it can be read; this is the same answer from a console, before the
// window is even opened.
//
// It asks exactly what the application asks -- the same settings.json, the same
// AudioInput, the same channel pair -- so an answer here IS the answer there.
// Anything that reimplemented the lookup would be testing itself.
#include <chrono>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

#include "audio_in.h"
#include "config/settings_io.h"

int main(int argc, char** argv) {
    svj::DeskSettings desk;
    std::string error;
    const std::string path = argc > 1 ? argv[1] : "settings.json";
    if (svj::settings_load(path, desk, error)) {
        std::printf("settings   : %s\n", path.c_str());
    } else {
        std::printf("settings   : %s introuvable ou illisible, valeurs par defaut\n", path.c_str());
    }
    std::printf("demande    : \"%s\", voies %u/%u, porteuse %.0f Hz\n",
                desk.platter_endpoint.c_str(), desk.platter_first_channel + 1,
                desk.platter_first_channel + 2, desk.carrier_hz);

    svj::ui::AudioInput input;
    if (!input.open(desk.platter_endpoint, desk.platter_first_channel)) {
        std::printf("resultat   : ECHEC -- %s\n", input.error().c_str());
        return 1;
    }

    std::printf("resultat   : ouvert -- %s\n", input.endpoint_name().c_str());
    std::printf("format     : %u canaux, %.0f Hz\n", input.channel_count(), input.sample_rate());

    // A device that opens and delivers nothing is a different fault from one
    // that refuses to open, so the count is worth waiting a moment for.
    std::vector<float> pcm;
    for (int i = 0; i < 20; ++i) {
        input.drain(pcm);
        if (input.frames_captured() > 0) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    std::printf("capture    : %llu trames\n",
                static_cast<unsigned long long>(input.frames_captured()));
    input.close();
    return 0;
}
