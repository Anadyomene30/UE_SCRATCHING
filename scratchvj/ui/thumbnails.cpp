#include "thumbnails.h"

#include <bgfx/bgfx.h>

#include "imgui_impl_bgfx.h"

namespace svj::ui {

ThumbnailStore::~ThumbnailStore() { clear(); }

void ThumbnailStore::clear() {
    for (Slot& slot : slots_) {
        if (slot.texture == 0xFFFF) continue;
        bgfx::TextureHandle handle;
        handle.idx = slot.texture;
        bgfx::destroy(handle);
        slot.texture = 0xFFFF;
    }
    slots_.clear();
    textures_.clear();
}

void ThumbnailStore::sync(const Library& library) {
    if (slots_.size() < library.size()) slots_.resize(library.size());
    textures_.assign(library.size(), nullptr);

    for (std::size_t i = 0; i < library.size(); ++i) {
        const ClipEntry& entry = library.at(static_cast<ClipId>(i));
        Slot& slot = slots_[i];
        const bool stale = slot.texture != 0xFFFF && slot.path != entry.path;
        if (stale) {
            bgfx::TextureHandle handle;
            handle.idx = slot.texture;
            bgfx::destroy(handle);
            slot.texture = 0xFFFF;
        }
        if (slot.texture == 0xFFFF && entry.thumb_w > 0 && entry.thumb_h > 0 &&
            !entry.thumbnail.empty()) {
            // The BC1 bytes go to the sampler exactly as the cache holds them,
            // the same way a deck's frames do (ui/media).
            const bgfx::TextureHandle handle = bgfx::createTexture2D(
                static_cast<std::uint16_t>(entry.thumb_w),
                static_cast<std::uint16_t>(entry.thumb_h), false, 1,
                bgfx::TextureFormat::BC1, BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP,
                bgfx::copy(entry.thumbnail.data(),
                           static_cast<std::uint32_t>(entry.thumbnail.size())));
            if (bgfx::isValid(handle)) {
                slot.texture = handle.idx;
                slot.path = entry.path;
            }
        }
        if (slot.texture != 0xFFFF) {
            textures_[i] = reinterpret_cast<void*>(ImGuiBgfx_TextureId(slot.texture));
        }
    }
}

}  // namespace svj::ui
