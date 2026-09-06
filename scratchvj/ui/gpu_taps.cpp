#include "gpu_taps.h"

#include <bgfx/bgfx.h>

// See gpu_compose.cpp: our shaders carry no WebGPU build.
#define BGFX_PLATFORM_SUPPORTS_WGSL 0
#include <bgfx/embedded_shader.h>

#include "essl/fs_taps.sc.bin.h"
#include "essl/vs_program.sc.bin.h"
#include "glsl/fs_taps.sc.bin.h"
#include "glsl/vs_program.sc.bin.h"
#include "spirv/fs_taps.sc.bin.h"
#include "spirv/vs_program.sc.bin.h"
#if defined(_WIN32)
#include "dxbc/fs_taps.sc.bin.h"
#include "dxbc/vs_program.sc.bin.h"
#include "dxil/fs_taps.sc.bin.h"
#include "dxil/vs_program.sc.bin.h"
#endif

namespace svj::ui {
namespace {

const bgfx::EmbeddedShader kShaders[] = {
    BGFX_EMBEDDED_SHADER(vs_program),
    BGFX_EMBEDDED_SHADER(fs_taps),
    BGFX_EMBEDDED_SHADER_END(),
};

struct Vertex {
    float x, y, z;
    float u, v;
};

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

bool TapsGpu::init(std::uint32_t width, std::uint32_t height, std::uint16_t view_id) {
    destroy();
    width_ = width;
    height_ = height;
    view_id_ = view_id;

    const bgfx::TextureHandle target = bgfx::createTexture2D(
        static_cast<std::uint16_t>(width), static_cast<std::uint16_t>(height), false, 1,
        bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_RT | BGFX_SAMPLER_UVW_CLAMP);
    if (!bgfx::isValid(target)) return false;
    target_ = target.idx;

    const bgfx::FrameBufferHandle framebuffer = bgfx::createFrameBuffer(1, &target, false);
    if (!bgfx::isValid(framebuffer)) return false;
    framebuffer_ = framebuffer.idx;

    const bgfx::RendererType::Enum type = bgfx::getRendererType();
    const bgfx::ProgramHandle program = bgfx::createProgram(
        bgfx::createEmbeddedShader(kShaders, type, "vs_program"),
        bgfx::createEmbeddedShader(kShaders, type, "fs_taps"), true);
    if (!bgfx::isValid(program)) return false;
    program_ = program.idx;

    sampler_ = bgfx::createUniform("s_taps", bgfx::UniformType::Sampler).idx;
    taps_uniform_ = bgfx::createUniform("u_taps", bgfx::UniformType::Vec4).idx;
    weights_uniform_ = bgfx::createUniform("u_weights", bgfx::UniformType::Vec4, 2).idx;

    bgfx::VertexLayout layout;
    layout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .end();
    vertices_ =
        bgfx::createVertexBuffer(bgfx::makeRef(kFullscreen, sizeof(kFullscreen)), layout)
            .idx;

    bgfx::FrameBufferHandle fb;
    fb.idx = framebuffer_;
    bgfx::setViewFrameBuffer(view_id_, fb);
    bgfx::setViewRect(view_id_, 0, 0, static_cast<std::uint16_t>(width),
                      static_cast<std::uint16_t>(height));
    bgfx::setViewClear(view_id_, BGFX_CLEAR_COLOR, 0x000000ff, 1.0f, 0);

    ready_ = true;
    return true;
}

void TapsGpu::destroy() {
    if (!ready_) return;
    if (framebuffer_ != 0xFFFF) {  // owns target_
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
    kill_uniform(sampler_);
    kill_uniform(taps_uniform_);
    kill_uniform(weights_uniform_);
    if (vertices_ != 0xFFFF) {
        bgfx::VertexBufferHandle v;
        v.idx = vertices_;
        bgfx::destroy(v);
        vertices_ = 0xFFFF;
    }
    ready_ = false;
}

std::uint16_t TapsGpu::render(std::uint16_t taps, std::uint16_t fallback,
                              const EffectUnit& unit, const TapPlan& plan) {
    if (!ready_ || taps == 0xFFFF) return fallback;
    if (!unit.video_active() || !is_multi_tap_effect(unit.type)) return fallback;
    // A collapsed plan is every tap on one frame: the record is not moving, so
    // there is nothing to trail. Drawing anyway would spend a pass to reproduce
    // the dry frame and, worse, would hide that the effect had gone quiet.
    if (plan.collapsed) return fallback;

    const float parameters[4] = {unit.type == EffectType::SlitScan ? 1.0f : 0.0f,
                                 unit.video_params().mix,
                                 static_cast<float>(plan.count), 0.0f};
    float weights[8] = {};
    for (int k = 0; k < kTapCount && k < 8; ++k) weights[k] = plan.weight[k];

    bgfx::UniformHandle u_taps, u_weights, u_sampler;
    u_taps.idx = taps_uniform_;
    u_weights.idx = weights_uniform_;
    u_sampler.idx = sampler_;
    bgfx::setUniform(u_taps, parameters);
    bgfx::setUniform(u_weights, weights, 2);
    bgfx::setTexture(0, u_sampler, tex_of(taps));

    bgfx::VertexBufferHandle vb;
    vb.idx = vertices_;
    bgfx::setVertexBuffer(0, vb);
    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
    bgfx::ProgramHandle p;
    p.idx = program_;
    bgfx::submit(view_id_, p);
    return target_;
}

}  // namespace svj::ui
