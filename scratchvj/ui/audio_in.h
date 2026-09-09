// scratchvj — a live audio input, for the timecode carrier.
//
// The first real audio device in the project, and deliberately the smallest
// thing that can be: one WASAPI capture endpoint, SHARED mode, one channel pair
// out of however many the interface has. It exists to feed core/quadrature the
// carrier the Phase puts on the MOTU's channels 5/6, and nothing else yet.
//
// SHARED MODE IS NOT A PREFERENCE. The roadmap's follower mode has one rule
// about audio: never open the input exclusively, because Serato may be on the
// same interface and exclusive would stop it dead. That was measured to hold --
// the MOTU and the Elite both open shared while Serato is live -- and this
// class cannot ask for anything else.
//
// The capture runs on its own thread because WASAPI delivers in small packets
// on its own clock, and a 60 Hz render loop polling it would miss some. The
// thread does nothing but copy: no decoding, no tracking, so there is nothing
// on it that can stall, and the render loop drains at its own pace.
#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace svj::ui {

class AudioInput {
public:
    ~AudioInput();

    // `endpoint` is a fragment of the device name ("MOTU"). `first_channel` is
    // the 0-based index of the pair's left leg: 4 for the MOTU's 5/6. Returns
    // false when nothing matched or the device refused a shared open.
    //
    // A fragment can match SEVERAL endpoints -- a MOTU exposes its first pair
    // and its whole front separately, both called "MOTU" -- so which one is
    // opened is decided by core/endpoint on the channels each one has, not by
    // whichever the machine enumerates first.
    bool open(const std::string& endpoint, unsigned first_channel);
    void close();
    bool ready() const { return running_; }

    // Why the last open() failed, in words meant for the interface. This
    // exists because the alternative does not work: on Windows this runs in a
    // WIN32 subsystem application with no console, so a reason written to
    // stderr is a reason nobody can read. Empty after a successful open.
    const std::string& error() const { return error_; }

    const std::string& endpoint_name() const { return endpoint_; }
    double sample_rate() const { return sample_rate_; }
    unsigned channel_count() const { return channels_; }
    unsigned first_channel() const { return first_channel_; }

    // Hands over everything captured since the last call, as INTERLEAVED
    // STEREO -- the shape core/quadrature and dvs/decoder both take.
    // `out` is replaced, not appended to.
    void drain(std::vector<float>& out);

    // How many frames arrived in total, so the interface can show the device
    // is alive even when the platter is still and the carrier is silent.
    std::uint64_t frames_captured() const;

private:
    void run();

    struct Impl;
    Impl* impl_ = nullptr;

    std::thread thread_;
    std::mutex lock_;
    std::vector<float> pending_;
    std::uint64_t captured_ = 0;

    std::string endpoint_;
    std::string error_;
    double sample_rate_ = 0.0;
    unsigned channels_ = 0;
    unsigned first_channel_ = 0;
    bool running_ = false;
    bool stop_ = false;
};

}  // namespace svj::ui
