// scratchvj — handing the program to other software.
//
// Spout on Windows, Syphon on macOS one day, NDI on both eventually: the
// roadmap's rule is one interface with backends behind it, so the engine and
// the front end never know which transport is running. This is that interface,
// with the Spout backend as its first occupant.
//
// The frames go out from the CPU compositor for now (spoutDX uploads them to a
// shared DX11 texture internally). When bgfx exists the program will already be
// a GPU texture and this call becomes a texture handoff instead of a memcpy --
// same interface, one line of difference to the caller.
#pragma once

#include <cstdint>
#include <string>

namespace svj::ui {

class ProgramShare {
public:
    ProgramShare();
    ~ProgramShare();

    ProgramShare(const ProgramShare&) = delete;
    ProgramShare& operator=(const ProgramShare&) = delete;

    // Registers the sender under `name`. Returns false where no transport
    // exists (non-Windows builds), which callers treat as "output disabled",
    // not as an error.
    bool open(const std::string& name);
    void close();
    bool active() const { return active_; }

    // Publishes one opaque RGBA8 frame. Cheap to call every frame; receivers
    // that are not connected cost nothing.
    void send(const std::uint8_t* rgba, std::uint32_t width, std::uint32_t height);

private:
    struct Impl;
    Impl* impl_ = nullptr;
    bool active_ = false;
};

}  // namespace svj::ui
