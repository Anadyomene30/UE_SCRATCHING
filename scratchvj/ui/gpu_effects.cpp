#include "gpu_effects.h"

#include <bgfx/bgfx.h>

// See gpu_compose.cpp: our shaders carry no WebGPU build.
#define BGFX_PLATFORM_SUPPORTS_WGSL 0
#include <bgfx/embedded_shader.h>

#include "essl/fs_effects.sc.bin.h"
#include "essl/vs_program.sc.bin.h"
#include "glsl/fs_effects.sc.bin.h"
#include "glsl/vs_program.sc.bin.h"
#include "spirv/fs_effects.sc.bin.h"
#include "spirv/vs_program.sc.bin.h"
#if defined(_WIN32)
#include "dxbc/fs_effects.sc.bin.h"
#include "dxbc/vs_program.sc.bin.h"
#include "dxil/fs_effects.sc.bin.h"
#include "dxil/vs_program.sc.bin.h"
#endif

namespace svj::ui {
namespace {

const bgfx::EmbeddedShader kShaders[] = {
    BGFX_EMBEDDED_SHADER(vs_program),
    BGFX_EMBEDDED_SHADER(fs_effects),
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

// The branch the shader takes, and the integer parameter it needs. Kept beside
// each other so the two halves of the mapping cannot be edited apart.
struct ShaderCall {
    float id = 0.0f;
    float parameter = 1.0f;
};

ShaderCall shader_call(const VideoFx& fx) {
    switch (fx.type) {
        case EffectType::LowPass:
            return {0.0f, static_cast<float>(blur_step_texels(fx.amount))};
        case EffectType::HighPass:
            return {1.0f, static_cast<float>(blur_step_texels(fx.amount))};
        case EffectType::Bitcrusher:
            return {2.0f, static_cast<float>(posterise_levels(fx.amount))};
        case EffectType::Invert:
            return {3.0f, 1.0f};
        case EffectType::Mirror:
            return {4.0f, 1.0f};
        case EffectType::Kaleidoscope:
            return {5.0f, static_cast<float>(kaleidoscope_segments(fx.amount))};
        default:
            return {3.0f, 1.0f};  // unreachable: callers filter on is_single_frame_effect
    }
}

}  // namespace

bool EffectsGpu::init(std::uint32_t width, std::uint32_t height,
                      std::uint16_t first_view_id) {
    destroy();
    width_ = width;
    height_ = height;
    view_id_ = first_view_id;

    for (int i = 0; i < 2; ++i) {
        const bgfx::TextureHandle target = bgfx::createTexture2D(
            static_cast<std::uint16_t>(width), static_cast<std::uint16_t>(height), false, 1,
            bgfx::TextureFormat::RGBA8,
            BGFX_TEXTURE_RT | BGFX_SAMPLER_UVW_CLAMP | BGFX_SAMPLER_POINT);
        if (!bgfx::isValid(target)) return false;
        targets_[i] = target.idx;

        const bgfx::FrameBufferHandle framebuffer =
            bgfx::createFrameBuffer(1, &target, false);
        if (!bgfx::isValid(framebuffer)) return false;
        framebuffers_[i] = framebuffer.idx;

    }

    // One view per pass, each bound to the target that pass writes. Views are
    // ordered by id, so the chain runs in the order it was submitted whatever
    // the backend decides about sorting within a view.
    for (int pass = 0; pass < kMaxEffectPasses; ++pass) {
        const std::uint16_t view = static_cast<std::uint16_t>(view_id_ + pass);
        bgfx::FrameBufferHandle fb;
        fb.idx = framebuffers_[pass % 2];
        bgfx::setViewFrameBuffer(view, fb);
        bgfx::setViewRect(view, 0, 0, static_cast<std::uint16_t>(width),
                          static_cast<std::uint16_t>(height));
        bgfx::setViewClear(view, BGFX_CLEAR_COLOR, 0x000000ff, 1.0f, 0);
    }

    const bgfx::RendererType::Enum type = bgfx::getRendererType();
    const bgfx::ProgramHandle program = bgfx::createProgram(
        bgfx::createEmbeddedShader(kShaders, type, "vs_program"),
        bgfx::createEmbeddedShader(kShaders, type, "fs_effects"), true);
    if (!bgfx::isValid(program)) return false;
    program_ = program.idx;

    sampler_ = bgfx::createUniform("s_source", bgfx::UniformType::Sampler).idx;
    fx_uniform_ = bgfx::createUniform("u_fx", bgfx::UniformType::Vec4).idx;
    texel_uniform_ = bgfx::createUniform("u_texel", bgfx::UniformType::Vec4).idx;

    bgfx::VertexLayout layout;
    layout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .end();
    vertices_ =
        bgfx::createVertexBuffer(bgfx::makeRef(kFullscreen, sizeof(kFullscreen)), layout)
            .idx;

    ready_ = true;
    return true;
}

void EffectsGpu::destroy() {
    if (!ready_) return;
    for (int i = 0; i < 2; ++i) {
        if (framebuffers_[i] != 0xFFFF) {  // owns its target
            bgfx::FrameBufferHandle fb;
            fb.idx = framebuffers_[i];
            bgfx::destroy(fb);
            framebuffers_[i] = 0xFFFF;
            targets_[i] = 0xFFFF;
        }
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
    kill_uniform(fx_uniform_);
    kill_uniform(texel_uniform_);
    if (vertices_ != 0xFFFF) {
        bgfx::VertexBufferHandle v;
        v.idx = vertices_;
        bgfx::destroy(v);
        vertices_ = 0xFFFF;
    }
    ready_ = false;
}

void EffectsGpu::run_pass(std::uint16_t source, int pass, const VideoFx& fx) {
    const ShaderCall call = shader_call(fx);
    const float parameters[4] = {call.id, fx.mix, call.parameter, 0.0f};
    const float texel[4] = {1.0f / static_cast<float>(width_),
                            1.0f / static_cast<float>(height_),
                            static_cast<float>(width_), static_cast<float>(height_)};

    bgfx::UniformHandle u_fx, u_texel, u_sampler;
    u_fx.idx = fx_uniform_;
    u_texel.idx = texel_uniform_;
    u_sampler.idx = sampler_;
    bgfx::setUniform(u_fx, parameters);
    bgfx::setUniform(u_texel, texel);
    bgfx::setTexture(0, u_sampler, tex_of(source));

    bgfx::VertexBufferHandle vb;
    vb.idx = vertices_;
    bgfx::setVertexBuffer(0, vb);
    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
    bgfx::ProgramHandle p;
    p.idx = program_;
    bgfx::submit(static_cast<std::uint16_t>(view_id_ + pass), p);
}

std::uint16_t EffectsGpu::render(std::uint16_t source, const EffectRack& rack) {
    passes_ = 0;
    if (!ready_ || source == 0xFFFF) return source;

    std::uint16_t current = source;
    for (std::size_t i = 0; i < rack.size() && passes_ < kMaxEffectPasses; ++i) {
        const EffectUnit& unit = rack.at(i);
        if (!unit.video_active()) continue;
        // An effect that needs the clip at several positions is SKIPPED, not
        // approximated with the one frame at hand: a dry picture presented as
        // an effect is worse than a knob that visibly does nothing yet.
        if (!is_single_frame_effect(unit.type)) continue;

        const EffectParams params = unit.video_params();
        run_pass(current, passes_, VideoFx{unit.type, params.mix, params.amount});
        current = targets_[passes_ % 2];
        ++passes_;
    }
    return current;
}

}  // namespace svj::ui
