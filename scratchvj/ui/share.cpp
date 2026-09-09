#include "share.h"

#ifdef _WIN32
#include "SpoutDX.h"
#endif

namespace svj::ui {

#ifdef _WIN32

struct ProgramShare::Impl {
    spoutDX sender;
};

ProgramShare::ProgramShare() : impl_(new Impl) {}

ProgramShare::~ProgramShare() {
    close();
    delete impl_;
}

bool ProgramShare::open(const std::string& name) {
    // spoutDX owns its own DX11 device; sharing the renderer's would save a
    // copy but weld this file to SDL's backend choice. Not worth it for the
    // CPU-frame era.
    if (!impl_->sender.OpenDirectX11()) return false;
    impl_->sender.SetSenderName(name.c_str());
    active_ = true;
    return true;
}

void ProgramShare::close() {
    if (!active_) return;
    impl_->sender.ReleaseSender();
    impl_->sender.CloseDirectX11();
    active_ = false;
}

void ProgramShare::send(const std::uint8_t* rgba, std::uint32_t width,
                        std::uint32_t height) {
    if (!active_ || rgba == nullptr || width == 0 || height == 0) return;
    impl_->sender.SendImage(rgba, width, height);
}

#else  // no transport on this platform yet: Syphon is the roadmap's answer

struct ProgramShare::Impl {};
ProgramShare::ProgramShare() = default;
ProgramShare::~ProgramShare() = default;
bool ProgramShare::open(const std::string&) { return false; }
void ProgramShare::close() {}
void ProgramShare::send(const std::uint8_t*, std::uint32_t, std::uint32_t) {}

#endif

}  // namespace svj::ui
