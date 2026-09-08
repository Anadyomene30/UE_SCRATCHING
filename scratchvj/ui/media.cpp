#include "media.h"

#include <bgfx/bgfx.h>

#include "imgui_impl_bgfx.h"

namespace svj::ui {
namespace {

// The sampler format matching the cache: the bytes go up exactly as stored,
// which is the whole point, so the texture has to be told what they are.
bgfx::TextureFormat::Enum format_of(const CacheHeader& header) {
    return header.format == BlockFormat::BC3 ? bgfx::TextureFormat::BC3
                                             : bgfx::TextureFormat::BC1;
}

}  // namespace

DeckMedia::~DeckMedia() { close(); }

void DeckMedia::close() {
    if (taps_ != 0xFFFF) {
        bgfx::TextureHandle handle;
        handle.idx = taps_;
        bgfx::destroy(handle);
        taps_ = 0xFFFF;
    }
    if (texture_ != 0xFFFF) {
        bgfx::TextureHandle handle;
        handle.idx = texture_;
        bgfx::destroy(handle);
        texture_ = 0xFFFF;
    }
    reader_.close();
    open_ = false;
    last_frame_ = 0xFFFFFFFFu;
    for (std::uint32_t& frame : tap_frames_) frame = 0xFFFFFFFFu;
}

bool DeckMedia::open(const std::string& path, std::string& error) {
    close();
    if (!reader_.open(path, error)) return false;
    if (reader_.header().format != BlockFormat::BC1 &&
        reader_.header().format != BlockFormat::BC3) {
        // The textures below are created in the cache's own format; a format
        // with no encoder yet (BC7) would display convincing garbage. Refusing
        // names the real problem instead.
        error = "seuls BC1 et BC3 sont gérés par l'affichage pour l'instant";
        reader_.close();
        return false;
    }
    open_ = true;
    return true;
}

void* DeckMedia::frame_at(double position_s) {
    if (!open_) return nullptr;

    const CacheHeader& header = reader_.header();
    const std::uint32_t frame = header.frame_at(position_s);
    if (frame == last_frame_ && texture_ != 0xFFFF) {
        return reinterpret_cast<void*>(ImGuiBgfx_TextureId(texture_));
    }

    std::string error;
    if (!reader_.read_frame(frame, packed_, error)) {
        return texture_ != 0xFFFF
                   ? reinterpret_cast<void*>(ImGuiBgfx_TextureId(texture_))
                   : nullptr;
    }

    // The display path: the BC1 block data, byte for byte as the cache stores
    // it, straight to the sampler. The decode below is only for the CPU
    // compositor and dies with it.
    if (texture_ == 0xFFFF) {
        const bgfx::TextureHandle handle = bgfx::createTexture2D(
            static_cast<std::uint16_t>(header.width),
            static_cast<std::uint16_t>(header.height), false, 1,
            format_of(header), BGFX_SAMPLER_NONE);
        if (!bgfx::isValid(handle)) return nullptr;
        texture_ = handle.idx;
    }
    bgfx::TextureHandle handle;
    handle.idx = texture_;
    bgfx::updateTexture2D(handle, 0, 0, 0, 0,
                          static_cast<std::uint16_t>(header.width),
                          static_cast<std::uint16_t>(header.height),
                          bgfx::copy(packed_.data(),
                                     static_cast<std::uint32_t>(packed_.size())));

    last_frame_ = frame;
    return reinterpret_cast<void*>(ImGuiBgfx_TextureId(texture_));
}

std::uint16_t DeckMedia::taps_texture(const TapPlan& plan) {
    if (!open_ || plan.count <= 0) return 0xFFFF;
    const CacheHeader& header = reader_.header();

    if (taps_ == 0xFFFF) {
        const bgfx::TextureHandle handle = bgfx::createTexture2D(
            static_cast<std::uint16_t>(header.width),
            static_cast<std::uint16_t>(header.height), false,
            static_cast<std::uint16_t>(kTapCount), format_of(header),
            BGFX_SAMPLER_UVW_CLAMP);
        if (!bgfx::isValid(handle)) return 0xFFFF;
        taps_ = handle.idx;
        for (std::uint32_t& frame : tap_frames_) frame = 0xFFFFFFFFu;
    }

    bgfx::TextureHandle handle;
    handle.idx = taps_;
    std::string error;
    for (int k = 0; k < plan.count && k < kTapCount; ++k) {
        const std::uint32_t frame = header.frame_at(plan.position_s[k]);
        if (frame == tap_frames_[k]) continue;  // this moment has not moved
        if (!reader_.read_frame(frame, packed_, error)) continue;
        bgfx::updateTexture2D(handle, static_cast<std::uint16_t>(k), 0, 0, 0,
                              static_cast<std::uint16_t>(header.width),
                              static_cast<std::uint16_t>(header.height),
                              bgfx::copy(packed_.data(),
                                         static_cast<std::uint32_t>(packed_.size())));
        tap_frames_[k] = frame;
    }
    return taps_;
}

}  // namespace svj::ui
