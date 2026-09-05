#include "media.h"

#include <SDL3/SDL.h>

#include "core/bc1.h"

namespace svj::ui {

DeckMedia::~DeckMedia() { close(); }

void DeckMedia::close() {
    if (texture_ != nullptr) {
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
    }
    reader_.close();
    open_ = false;
    last_frame_ = 0xFFFFFFFFu;
}

bool DeckMedia::open(const std::string& path, std::string& error) {
    close();
    if (!reader_.open(path, error)) return false;
    if (reader_.header().format != BlockFormat::BC1) {
        // Only BC1 has a CPU decoder here. Refusing is better than showing
        // garbage that looks like a corrupted clip rather than a missing codec.
        error = "seul BC1 est décodable par l'interface pour l'instant";
        reader_.close();
        return false;
    }
    open_ = true;
    return true;
}

SDL_Texture* DeckMedia::frame_at(SDL_Renderer* renderer, double position_s) {
    if (!open_) return nullptr;

    const CacheHeader& header = reader_.header();
    const std::uint32_t frame = header.frame_at(position_s);
    if (frame == last_frame_ && texture_ != nullptr) return texture_;

    std::string error;
    if (!reader_.read_frame(frame, packed_, error)) return texture_;
    decode_bc1(packed_.data(), header.width, header.height, rgba_);

    if (texture_ == nullptr) {
        texture_ = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                                     SDL_TEXTUREACCESS_STREAMING,
                                     static_cast<int>(header.width),
                                     static_cast<int>(header.height));
        if (texture_ == nullptr) return nullptr;
        SDL_SetTextureScaleMode(texture_, SDL_SCALEMODE_LINEAR);
    }

    SDL_UpdateTexture(texture_, nullptr, rgba_.data(),
                      static_cast<int>(header.width) * 4);
    last_frame_ = frame;
    return texture_;
}

}  // namespace svj::ui
