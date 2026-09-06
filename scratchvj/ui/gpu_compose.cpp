#include "gpu_compose.h"

#include <bgfx/bgfx.h>

// Our shaders are compiled for the profiles the platforms below actually run;
// WebGPU is not one of them, and without this the embedded-shader table would
// demand _wgsl arrays we do not generate.
#define BGFX_PLATFORM_SUPPORTS_WGSL 0
#include <bgfx/embedded_shader.h>


// The compositor shaders, compiled by shaderc at build time into per-backend
// headers -- the first scratchvj shaders, and the reason shaderc entered the
// toolchain.
#include "essl/fs_program.sc.bin.h"
#include "essl/vs_program.sc.bin.h"
#include "glsl/fs_program.sc.bin.h"
#include "glsl/vs_program.sc.bin.h"
#include "spirv/fs_program.sc.bin.h"
#include "spirv/vs_program.sc.bin.h"
#if defined(_WIN32)
#include "dxbc/fs_program.sc.bin.h"
#include "dxbc/vs_program.sc.bin.h"
#include "dxil/fs_program.sc.bin.h"
#include "dxil/vs_program.sc.bin.h"
#endif

namespace svj::ui {
namespace {

const bgfx::EmbeddedShader kShaders[] = {
    BGFX_EMBEDDED_SHADER(vs_program),
    BGFX_EMBEDDED_SHADER(fs_program),
    BGFX_EMBEDDED_SHADER_END(),
};

struct Vertex {
    float x, y, z;
    float u, v;
};

// One triangle over the whole clip space; uv 0 at the top so the target reads
// like an image.
const Vertex kFullscreen[3] = {
    {-1.0f, 1.0f, 0.0f, 0.0f, 0.0f},
    {3.0f, 1.0f, 0.0f, 2.0f, 0.0f},
    {-1.0f, -3.0f, 0.0f, 0.0f, 2.0f},
};

bgfx::TextureHandle tex_of(std::uint16_t index) {
    bgfx::TextureHandle handle;
    handle.idx = index;
    return handle;
}

}  // namespace

bool ProgramGpu::init(std::uint32_t width, std::uint32_t height, std::uint16_t view_id,
                      std::uint16_t blit_view_id) {
    destroy();
    width_ = width;
    height_ = height;
    view_id_ = view_id;
    blit_view_id_ = blit_view_id;

    const bgfx::TextureHandle target = bgfx::createTexture2D(
        static_cast<std::uint16_t>(width), static_cast<std::uint16_t>(height), false, 1,
        bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_RT | BGFX_SAMPLER_UVW_CLAMP);
    if (!bgfx::isValid(target)) return false;
    target_ = target.idx;

    const bgfx::FrameBufferHandle framebuffer = bgfx::createFrameBuffer(1, &target, false);
    if (!bgfx::isValid(framebuffer)) return false;
    framebuffer_ = framebuffer.idx;

    const bgfx::TextureHandle readback = bgfx::createTexture2D(
        static_cast<std::uint16_t>(width), static_cast<std::uint16_t>(height), false, 1,
        bgfx::TextureFormat::RGBA8,
        BGFX_TEXTURE_BLIT_DST | BGFX_TEXTURE_READ_BACK);
    if (!bgfx::isValid(readback)) return false;
    readback_ = readback.idx;

    const bgfx::RendererType::Enum type = bgfx::getRendererType();
    const bgfx::ProgramHandle program = bgfx::createProgram(
        bgfx::createEmbeddedShader(kShaders, type, "vs_program"),
        bgfx::createEmbeddedShader(kShaders, type, "fs_program"), true);
    if (!bgfx::isValid(program)) return false;
    program_ = program.idx;

    uniform_ = bgfx::createUniform("u_gains", bgfx::UniformType::Vec4).idx;
    samplers_[0] = bgfx::createUniform("s_deckA", bgfx::UniformType::Sampler).idx;
    samplers_[1] = bgfx::createUniform("s_deckB", bgfx::UniformType::Sampler).idx;
    samplers_[2] = bgfx::createUniform("s_overlay", bgfx::UniformType::Sampler).idx;

    bgfx::VertexLayout layout;
    layout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .end();
    vertices_ =
        bgfx::createVertexBuffer(bgfx::makeRef(kFullscreen, sizeof(kFullscreen)), layout)
            .idx;

    // A 1x1 black texel for an absent layer: composing with it is a no-op in
    // every blend mode this shader has, which is exactly what "no clip loaded"
    // must mean.
    const std::uint32_t black_texel = 0xFF000000u;
    black_ = bgfx::createTexture2D(1, 1, false, 1, bgfx::TextureFormat::RGBA8,
                                   BGFX_SAMPLER_NONE,
                                   bgfx::copy(&black_texel, sizeof(black_texel)))
                 .idx;

    for (Slot& slot : slots_) {
        slot.pixels.assign(static_cast<std::size_t>(width) * height * 4, 0);
        slot.pending = false;
    }

    bgfx::FrameBufferHandle fb;
    fb.idx = framebuffer_;
    bgfx::setViewFrameBuffer(view_id_, fb);
    bgfx::setViewRect(view_id_, 0, 0, static_cast<std::uint16_t>(width),
                      static_cast<std::uint16_t>(height));
    bgfx::setViewClear(view_id_, BGFX_CLEAR_COLOR, 0x000000ff, 1.0f, 0);

    ready_ = true;
    return true;
}

void ProgramGpu::destroy() {
    if (!ready_) return;
    const auto kill_texture = [](std::uint16_t& index) {
        if (index != 0xFFFF) {
            bgfx::destroy(tex_of(index));
            index = 0xFFFF;
        }
    };
    kill_texture(readback_);
    kill_texture(black_);
    if (framebuffer_ != 0xFFFF) {  // owns target_, destroyed with it
        bgfx::FrameBufferHandle fb;
        fb.idx = framebuffer_;
        bgfx::destroy(fb);
        framebuffer_ = 0xFFFF;
        target_ = 0xFFFF;
    }
    if (program_ != 0xFFFF) {
        bgfx::ProgramHandle p;
        p.idx = program_;
        bgfx::destroy(p);
        program_ = 0xFFFF;
    }
    const auto kill_uniform = [](std::uint16_t& index) {
        if (index != 0xFFFF) {
            bgfx::UniformHandle u;
            u.idx = index;
            bgfx::destroy(u);
            index = 0xFFFF;
        }
    };
    kill_uniform(uniform_);
    for (std::uint16_t& sampler : samplers_) kill_uniform(sampler);
    if (vertices_ != 0xFFFF) {
        bgfx::VertexBufferHandle v;
        v.idx = vertices_;
        bgfx::destroy(v);
        vertices_ = 0xFFFF;
    }
    ready_ = false;
}

void ProgramGpu::render(std::uint16_t deck_a, std::uint16_t deck_b, std::uint16_t overlay,
                        float gain_a, float gain_b, float gain_overlay,
                        int overlay_mode) {
    if (!ready_) return;

    const float gains[4] = {gain_a, gain_b, gain_overlay,
                            static_cast<float>(overlay_mode)};
    bgfx::UniformHandle u;
    u.idx = uniform_;
    bgfx::setUniform(u, gains);

    bgfx::UniformHandle s0, s1, s2;
    s0.idx = samplers_[0];
    s1.idx = samplers_[1];
    s2.idx = samplers_[2];
    bgfx::setTexture(0, s0, tex_of(deck_a != 0xFFFF ? deck_a : black_));
    bgfx::setTexture(1, s1, tex_of(deck_b != 0xFFFF ? deck_b : black_));
    bgfx::setTexture(2, s2, tex_of(overlay != 0xFFFF ? overlay : black_));

    bgfx::VertexBufferHandle vb;
    vb.idx = vertices_;
    bgfx::setVertexBuffer(0, vb);
    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
    bgfx::ProgramHandle p;
    p.idx = program_;
    bgfx::submit(view_id_, p);

}

void ProgramGpu::queue_readback(std::uint16_t source) {
    if (!ready_ || source == 0xFFFF) return;

    // Two slots cover the GPU's natural two-frame latency without ever stalling
    // on it: one is in flight while the other is being filled.
    Slot& slot = slots_[next_slot_];
    if (slot.pending) return;

    bgfx::TextureRegion destination;
    destination.init(tex_of(readback_), 0, 0);
    bgfx::TextureRegion origin;
    origin.init(tex_of(source), 0, 0);
    bgfx::blit(blit_view_id_, destination, origin);

    bgfx::TextureRegion whole;
    whole.init(tex_of(readback_), 0, 0);
    slot.ready_frame = bgfx::read(whole, slot.pixels.data());
    slot.pending = true;
    next_slot_ = 1 - next_slot_;
}

void* ProgramGpu::imgui_texture() const {
    if (!ready_) return nullptr;
    // idx+1, the imgui_impl_bgfx convention -- encoded here directly so this
    // module (and the headless gpu_check tool) never depends on ImGui.
    return reinterpret_cast<void*>(static_cast<std::uint64_t>(target_) + 1);
}

const std::uint8_t* ProgramGpu::completed_frame(std::uint32_t current_frame) {
    const std::uint8_t* newest = nullptr;
    for (Slot& slot : slots_) {
        if (slot.pending && current_frame >= slot.ready_frame) {
            slot.pending = false;
            newest = slot.pixels.data();
        }
    }
    return newest;
}

}  // namespace svj::ui
