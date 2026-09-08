#include "output_window.h"

#include <SDL3/SDL.h>
#include <bgfx/bgfx.h>

#define BGFX_PLATFORM_SUPPORTS_WGSL 0
#include <bgfx/embedded_shader.h>

#include <algorithm>
#include <cmath>
#include <vector>

#include "essl/fs_blit.sc.bin.h"
#include "essl/fs_warp.sc.bin.h"
#include "essl/vs_program.sc.bin.h"
#include "essl/vs_warp.sc.bin.h"
#include "glsl/fs_blit.sc.bin.h"
#include "glsl/fs_warp.sc.bin.h"
#include "glsl/vs_program.sc.bin.h"
#include "glsl/vs_warp.sc.bin.h"
#include "spirv/fs_blit.sc.bin.h"
#include "spirv/fs_warp.sc.bin.h"
#include "spirv/vs_program.sc.bin.h"
#include "spirv/vs_warp.sc.bin.h"
#if defined(_WIN32)
#include "dxbc/fs_blit.sc.bin.h"
#include "dxbc/fs_warp.sc.bin.h"
#include "dxbc/vs_program.sc.bin.h"
#include "dxbc/vs_warp.sc.bin.h"
#include "dxil/fs_blit.sc.bin.h"
#include "dxil/fs_warp.sc.bin.h"
#include "dxil/vs_program.sc.bin.h"
#include "dxil/vs_warp.sc.bin.h"
#endif

namespace svj::ui {
namespace {

const bgfx::EmbeddedShader kShaders[] = {
    BGFX_EMBEDDED_SHADER(vs_program),
    BGFX_EMBEDDED_SHADER(fs_blit),
    BGFX_EMBEDDED_SHADER(vs_warp),
    BGFX_EMBEDDED_SHADER(fs_warp),
    BGFX_EMBEDDED_SHADER_END(),
};

struct Vertex {
    float x, y, z;
    float u, v;
};

// The warp grid's vertex: the same, plus the mask's coverage in the colour.
struct WarpVertex {
    float x, y, z;
    float u, v;
    std::uint32_t abgr;
};

// One triangle over the whole clip space; uv 0 at the top so the picture
// reads the right way up. Same shape the compositor uses, same reason: a
// quad's diagonal would run the fragment shader twice along it.
const Vertex kFullscreen[3] = {
    {-1.0f, 1.0f, 0.0f, 0.0f, 0.0f},
    {3.0f, 1.0f, 0.0f, 2.0f, 0.0f},
    {-1.0f, -3.0f, 0.0f, 0.0f, 2.0f},
};

void* native_handle(SDL_Window* window) {
#if defined(_WIN32)
    return SDL_GetPointerProperty(SDL_GetWindowProperties(window),
                                  SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
#elif defined(__APPLE__)
    return SDL_GetPointerProperty(SDL_GetWindowProperties(window),
                                  SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
#else
    return SDL_GetPointerProperty(SDL_GetWindowProperties(window),
                                  SDL_PROP_WINDOW_X11_WINDOW_NUMBER, nullptr);
#endif
}

bgfx::VertexLayout warp_layout() {
    bgfx::VertexLayout layout;
    layout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
        .end();
    return layout;
}

}  // namespace

bool OutputGeometry::is_identity() const {
    const bool pinned = pin != nullptr && !pin->is_identity();
    const bool meshed = mesh_enabled && mesh != nullptr;
    const bool masked = mask != nullptr && !mask->empty();
    return !pinned && !meshed && !masked;
}

std::vector<DisplayInfo> displays() {
    std::vector<DisplayInfo> out;
    int count = 0;
    SDL_DisplayID* ids = SDL_GetDisplays(&count);
    if (ids == nullptr) return out;
    const SDL_DisplayID primary = SDL_GetPrimaryDisplay();
    for (int i = 0; i < count; ++i) {
        DisplayInfo info;
        info.id = ids[i];
        const char* name = SDL_GetDisplayName(ids[i]);
        info.name = name != nullptr ? name : "écran";
        if (const SDL_DisplayMode* mode = SDL_GetDesktopDisplayMode(ids[i])) {
            info.width = mode->w;
            info.height = mode->h;
            info.refresh_hz = mode->refresh_rate;
        }
        info.primary = ids[i] == primary;
        out.push_back(std::move(info));
    }
    SDL_free(ids);
    return out;
}

OutputWindow::~OutputWindow() { close(); }

bool OutputWindow::open(std::uint32_t display_id, std::uint16_t clear_view,
                        std::uint16_t draw_view) {
    close();
    error_.clear();
    clear_view_ = clear_view;
    draw_view_ = draw_view;

    SDL_Rect bounds{};
    if (!SDL_GetDisplayBounds(display_id, &bounds)) {
        error_ = std::string("écran introuvable : ") + SDL_GetError();
        return false;
    }

    // Created at the display's own size and moved onto it BEFORE going
    // fullscreen: asking for fullscreen on a window the compositor still
    // thinks belongs to the primary screen puts the picture on the wrong one.
    window_ = SDL_CreateWindow("scratchvj — sortie", bounds.w, bounds.h,
                               SDL_WINDOW_BORDERLESS | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (window_ == nullptr) {
        error_ = std::string("fenêtre de sortie : ") + SDL_GetError();
        return false;
    }
    SDL_SetWindowPosition(window_, SDL_WINDOWPOS_CENTERED_DISPLAY(display_id),
                          SDL_WINDOWPOS_CENTERED_DISPLAY(display_id));
    SDL_SetWindowFullscreen(window_, true);
    SDL_SyncWindow(window_);
    SDL_GetWindowSizeInPixels(window_, &width_, &height_);
    display_id_ = display_id;

    void* nwh = native_handle(window_);
    if (nwh == nullptr) {
        error_ = "cette fenêtre n'a pas de handle natif";
        close();
        return false;
    }

    // A second swap chain on the SAME device: the program's texture is drawn
    // straight into it, with no readback and no copy through main memory.
    if ((bgfx::getCaps()->supported & BGFX_CAPS_SWAP_CHAIN) == 0) {
        error_ = "ce moteur de rendu ne gère pas une seconde fenêtre";
        close();
        return false;
    }
    bgfx::SwapChain chain;  // its constructor carries the sane defaults
    chain.nwh = nwh;
    chain.width = static_cast<std::uint32_t>(width_);
    chain.height = static_cast<std::uint32_t>(height_);
    const bgfx::FrameBufferHandle framebuffer = bgfx::createFrameBuffer(chain);
    if (!bgfx::isValid(framebuffer)) {
        error_ = "bgfx a refusé une seconde chaîne d'échange";
        close();
        return false;
    }
    framebuffer_ = framebuffer.idx;

    const bgfx::RendererType::Enum type = bgfx::getRendererType();
    const bgfx::ProgramHandle program = bgfx::createProgram(
        bgfx::createEmbeddedShader(kShaders, type, "vs_program"),
        bgfx::createEmbeddedShader(kShaders, type, "fs_blit"), true);
    if (!bgfx::isValid(program)) {
        error_ = "le shader de sortie n'a pas pu être créé";
        close();
        return false;
    }
    program_ = program.idx;
    const bgfx::ProgramHandle warp = bgfx::createProgram(
        bgfx::createEmbeddedShader(kShaders, type, "vs_warp"),
        bgfx::createEmbeddedShader(kShaders, type, "fs_warp"), true);
    if (!bgfx::isValid(warp)) {
        error_ = "le shader de déformation n'a pas pu être créé";
        close();
        return false;
    }
    warp_program_ = warp.idx;
    sampler_ = bgfx::createUniform("s_source", bgfx::UniformType::Sampler).idx;

    bgfx::VertexLayout layout;
    layout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .end();
    vertices_ = bgfx::createVertexBuffer(bgfx::makeRef(kFullscreen, sizeof(kFullscreen)), layout)
                    .idx;
    return true;
}

void OutputWindow::close() {
    const auto destroy = [](std::uint16_t& index, auto make) {
        if (index == 0xFFFF) return;
        auto handle = make();
        handle.idx = index;
        bgfx::destroy(handle);
        index = 0xFFFF;
    };
    destroy(framebuffer_, [] { return bgfx::FrameBufferHandle{}; });
    destroy(program_, [] { return bgfx::ProgramHandle{}; });
    destroy(warp_program_, [] { return bgfx::ProgramHandle{}; });
    destroy(sampler_, [] { return bgfx::UniformHandle{}; });
    destroy(vertices_, [] { return bgfx::VertexBufferHandle{}; });
    if (window_ != nullptr) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    display_id_ = 0;
    width_ = 0;
    height_ = 0;
}

bool OutputWindow::owns(std::uint32_t window_id) const {
    return window_ != nullptr && SDL_GetWindowID(window_) == window_id;
}

void OutputWindow::present(std::uint16_t source, std::uint32_t src_w, std::uint32_t src_h,
                           const OutputGeometry& geometry) {
    if (window_ == nullptr || framebuffer_ == 0xFFFF) return;

    bgfx::FrameBufferHandle framebuffer;
    framebuffer.idx = framebuffer_;

    // The whole screen, painted black. Everything the picture does not cover
    // has to be black rather than whatever the last frame left there.
    bgfx::setViewFrameBuffer(clear_view_, framebuffer);
    bgfx::setViewRect(clear_view_, 0, 0, static_cast<std::uint16_t>(width_),
                      static_cast<std::uint16_t>(height_));
    bgfx::setViewClear(clear_view_, BGFX_CLEAR_COLOR, 0x000000ff, 1.0f, 0);
    bgfx::touch(clear_view_);

    if (source == 0xFFFF || src_w == 0 || src_h == 0 || width_ <= 0 || height_ <= 0) return;

    // Letterbox: the largest rectangle of the program's shape that fits the
    // screen, centred. Never a stretch -- a mask or a corner pin drawn against
    // a stretched picture would be wrong by exactly that distortion.
    const float screen_aspect = static_cast<float>(width_) / static_cast<float>(height_);
    const float source_aspect = static_cast<float>(src_w) / static_cast<float>(src_h);
    int fit_w = width_;
    int fit_h = height_;
    if (source_aspect > screen_aspect) {
        fit_h = static_cast<int>(static_cast<float>(width_) / source_aspect + 0.5f);
    } else {
        fit_w = static_cast<int>(static_cast<float>(height_) * source_aspect + 0.5f);
    }
    const int x = (width_ - fit_w) / 2;
    const int y = (height_ - fit_h) / 2;

    bgfx::setViewFrameBuffer(draw_view_, framebuffer);
    bgfx::setViewRect(draw_view_, static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y),
                      static_cast<std::uint16_t>(fit_w), static_cast<std::uint16_t>(fit_h));
    bgfx::setViewClear(draw_view_, BGFX_CLEAR_NONE, 0, 1.0f, 0);

    bgfx::TextureHandle texture;
    texture.idx = source;
    bgfx::UniformHandle sampler;
    sampler.idx = sampler_;
    bgfx::setTexture(0, sampler, texture);

    if (!geometry.is_identity()) {
        draw_warped(source, geometry);
        return;
    }

    bgfx::VertexBufferHandle vertices;
    vertices.idx = vertices_;
    bgfx::setVertexBuffer(0, vertices);
    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);

    bgfx::ProgramHandle program;
    program.idx = program_;
    bgfx::submit(draw_view_, program);
}

void OutputWindow::draw_warped(std::uint16_t source, const OutputGeometry& geometry) {
    (void)source;
    // The source's unit square, tessellated; every vertex placed by the SAME
    // functions the SORTIE screen previews with (core/warp, core/mesh), so
    // the projector and the preview cannot disagree. Positions are in the
    // output's unit square (0,0 top-left), like the pin's corners.
    constexpr int kSide = kWarpCells + 1;
    const std::uint32_t vertex_count = kSide * kSide;
    const std::uint32_t index_count = kWarpCells * kWarpCells * 6;
    const bgfx::VertexLayout layout = warp_layout();
    if (bgfx::getAvailTransientVertexBuffer(vertex_count, layout) < vertex_count ||
        bgfx::getAvailTransientIndexBuffer(index_count) < index_count) {
        return;  // this frame is skipped rather than drawn half; the next one draws
    }
    bgfx::TransientVertexBuffer vertices;
    bgfx::TransientIndexBuffer indices;
    bgfx::allocTransientVertexBuffer(&vertices, vertex_count, layout);
    bgfx::allocTransientIndexBuffer(&indices, index_count);

    const bool meshed = geometry.mesh_enabled && geometry.mesh != nullptr;
    const Homography h = geometry.pin != nullptr ? homography_from(*geometry.pin) : Homography{};
    const Mask* mask = geometry.mask != nullptr && !geometry.mask->empty() ? geometry.mask : nullptr;

    WarpVertex* out = reinterpret_cast<WarpVertex*>(vertices.data);
    for (int row = 0; row < kSide; ++row) {
        const double v = static_cast<double>(row) / kWarpCells;
        for (int col = 0; col < kSide; ++col) {
            const double u = static_cast<double>(col) / kWarpCells;
            const Point placed = meshed ? geometry.mesh->map(u, v) : apply(h, Point{u, v});
            // The mask is a shape on the OUTPUT (where the wall is), so it is
            // read at the placed point, not at the source point.
            const double cover = mask != nullptr ? mask->coverage(placed) : 1.0;
            const std::uint8_t alpha =
                static_cast<std::uint8_t>(std::lround(std::clamp(cover, 0.0, 1.0) * 255.0));
            WarpVertex& vertex = out[row * kSide + col];
            vertex.x = static_cast<float>(placed.x * 2.0 - 1.0);
            vertex.y = static_cast<float>(1.0 - placed.y * 2.0);
            vertex.z = 0.0f;
            vertex.u = static_cast<float>(u);
            vertex.v = static_cast<float>(v);
            vertex.abgr = (static_cast<std::uint32_t>(alpha) << 24) | 0x00FFFFFFu;
        }
    }
    std::uint16_t* index = reinterpret_cast<std::uint16_t*>(indices.data);
    for (int row = 0; row < kWarpCells; ++row) {
        for (int col = 0; col < kWarpCells; ++col) {
            const std::uint16_t a = static_cast<std::uint16_t>(row * kSide + col);
            const std::uint16_t b = static_cast<std::uint16_t>(a + 1);
            const std::uint16_t c = static_cast<std::uint16_t>(a + kSide);
            const std::uint16_t d = static_cast<std::uint16_t>(c + 1);
            *index++ = a; *index++ = c; *index++ = b;
            *index++ = b; *index++ = c; *index++ = d;
        }
    }

    bgfx::setVertexBuffer(0, &vertices);
    bgfx::setIndexBuffer(&indices);
    // No culling: a pin that crosses itself or a mesh folded over flips a
    // triangle's winding, and a flipped triangle must still be drawn.
    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
    bgfx::ProgramHandle program;
    program.idx = warp_program_;
    bgfx::submit(draw_view_, program);
}

}  // namespace svj::ui
