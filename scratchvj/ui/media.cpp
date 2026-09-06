#include "media.h"

#include <bgfx/bgfx.h>

#include "imgui_impl_bgfx.h"

namespace svj::ui {

DeckMedia::~DeckMedia() { close(); }

void DeckMedia::close() {
    if (texture_ != 0xFFFF) {
        bgfx::TextureHandle handle;
        handle.idx = texture_;
        bgfx::destroy(handle);
        texture_ = 0xFFFF;
    }
    reader_.close();
    open_ = false;
    last_frame_ = 0xFFFFFFFFu;
}

bool DeckMedia::open(const std::string& path, std::string& error) {
    close();
    if (!reader_.open(path, error)) return false;
    if (reader_.header().format != BlockFormat::BC1) {
        // The texture below is created as BC1; feeding it BC3/BC7 blocks would
        // display convincing garbage. Refusing names the real problem instead.
        error = "seul BC1 est géré par l'affichage pour l'instant";
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
            bgfx::TextureFormat::BC1, BGFX_SAMPLER_NONE);
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

}  // namespace svj::ui
