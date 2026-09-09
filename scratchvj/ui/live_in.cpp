#include "live_in.h"

#include <algorithm>
#include <cstring>

#include <bgfx/bgfx.h>

#if defined(_WIN32)
#include "SpoutDX.h"
#endif

namespace svj::ui {

#if defined(_WIN32)

struct LiveInput::Impl {
    spoutDX receiver;
    std::vector<unsigned char> scratch;  // one frame, as SpoutDX fills it
};

LiveInput::~LiveInput() { close(); }

bool LiveInput::open(const std::string& sender, std::uint64_t budget_bytes) {
    close();
    impl_ = new Impl();
    if (!impl_->receiver.OpenDirectX11()) {
        delete impl_;
        impl_ = nullptr;
        return false;
    }
    if (!sender.empty()) impl_->receiver.SetReceiverName(sender.c_str());
    budget_bytes_ = budget_bytes;
    sender_ = sender;
    ready_ = true;
    return true;
}

void LiveInput::close() {
    if (impl_ != nullptr) {
        impl_->receiver.ReleaseReceiver();
        impl_->receiver.CloseDirectX11();
        delete impl_;
        impl_ = nullptr;
    }
    if (texture_ != 0xFFFF) {
        bgfx::TextureHandle handle;
        handle.idx = texture_;
        bgfx::destroy(handle);
        texture_ = 0xFFFF;
    }
    frames_.clear();
    frames_.shrink_to_fit();
    ring_.reset();
    width_ = height_ = 0;
    last_slot_ = static_cast<std::size_t>(-1);
    ready_ = false;
    connected_ = false;
    clamped_ = false;
}

void LiveInput::resize(std::uint32_t width, std::uint32_t height) {
    if (width == 0 || height == 0) return;

    width_ = width;
    height_ = height;

    // The ring's depth follows the budget, so the same setting serves 720p and
    // 4K. At least two frames, or "pick the nearest of the straddling pair" has
    // nothing to straddle.
    const std::uint64_t frame_bytes =
        static_cast<std::uint64_t>(width) * height * 4ull;
    auto capacity = static_cast<std::size_t>(budget_bytes_ / std::max<std::uint64_t>(frame_bytes, 1));
    capacity = std::max<std::size_t>(capacity, 2);

    ring_.configure(capacity);
    frames_.assign(static_cast<std::size_t>(frame_bytes) * capacity, 0);
    last_slot_ = static_cast<std::size_t>(-1);

    if (texture_ != 0xFFFF) {
        bgfx::TextureHandle old;
        old.idx = texture_;
        bgfx::destroy(old);
        texture_ = 0xFFFF;
    }
    // Created empty and filled by updateTexture2D. Supplying pixels at creation
    // makes a bgfx texture IMMUTABLE -- the trap that once silently dropped
    // every later upload here and showed a black picture with no error.
    const bgfx::TextureHandle texture = bgfx::createTexture2D(
        static_cast<std::uint16_t>(width), static_cast<std::uint16_t>(height), false, 1,
        bgfx::TextureFormat::RGBA8, BGFX_SAMPLER_UVW_CLAMP);
    if (bgfx::isValid(texture)) texture_ = texture.idx;

    impl_->scratch.assign(static_cast<std::size_t>(frame_bytes), 0);
}

bool LiveInput::poll(double now_s) {
    if (!ready_ || impl_ == nullptr) return false;

    unsigned int width = width_;
    unsigned int height = height_;
    const bool got = impl_->receiver.ReceiveImage(
        impl_->scratch.empty() ? nullptr : impl_->scratch.data(), width, height);
    connected_ = impl_->receiver.IsConnected();
    if (!got) return false;

    if (impl_->receiver.IsUpdated()) {
        // The sender changed size, or appeared. Everything sized from it has to
        // follow, and the history cannot survive it: frames of two different
        // shapes in one ring have no meaning.
        resize(impl_->receiver.GetSenderWidth(), impl_->receiver.GetSenderHeight());
        sender_ = impl_->receiver.GetSenderName();
        return false;  // the next call fills the freshly sized buffer
    }
    if (impl_->scratch.empty() || width_ == 0) return false;

    const std::size_t slot = ring_.push(now_s);
    const std::size_t frame_bytes = static_cast<std::size_t>(width_) * height_ * 4;
    std::memcpy(frames_.data() + slot * frame_bytes, impl_->scratch.data(), frame_bytes);
    return true;
}

void* LiveInput::frame_at(double capture_s) {
    if (texture_ == 0xFFFF || ring_.empty()) return nullptr;

    const LivePick pick = ring_.pick(capture_s);
    clamped_ = pick.clamped;
    if (!pick.valid) return nullptr;

    // Uploaded only when the chosen frame changed, the way DeckMedia does it. A
    // record held still costs nothing; a scratch costs one upload per frame it
    // actually crosses.
    if (pick.slot != last_slot_) {
        const std::size_t frame_bytes = static_cast<std::size_t>(width_) * height_ * 4;
        bgfx::TextureHandle handle;
        handle.idx = texture_;
        bgfx::updateTexture2D(
            handle, 0, 0, 0, 0, static_cast<std::uint16_t>(width_),
            static_cast<std::uint16_t>(height_),
            bgfx::copy(frames_.data() + pick.slot * frame_bytes,
                       static_cast<std::uint32_t>(frame_bytes)));
        last_slot_ = pick.slot;
    }
    return reinterpret_cast<void*>(static_cast<std::uint64_t>(texture_) + 1);
}

const std::uint8_t* LiveInput::pixels(std::size_t slot) const {
    if (frames_.empty() || slot >= ring_.capacity() || width_ == 0) return nullptr;
    const std::size_t frame_bytes = static_cast<std::size_t>(width_) * height_ * 4;
    return frames_.data() + slot * frame_bytes;
}

#else

// No texture sharing outside Windows yet: Syphon is the macOS counterpart and is
// not written. A live input simply reports itself unavailable, which callers
// treat as "no live source", not as an error.
struct LiveInput::Impl {};

LiveInput::~LiveInput() { close(); }
bool LiveInput::open(const std::string&, std::uint64_t) { return false; }
void LiveInput::close() {}
void LiveInput::resize(std::uint32_t, std::uint32_t) {}
bool LiveInput::poll(double) { return false; }
void* LiveInput::frame_at(double) { return nullptr; }
const std::uint8_t* LiveInput::pixels(std::size_t) const { return nullptr; }

#endif

}  // namespace svj::ui
