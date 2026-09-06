#include "gpu_eye.h"

#include <algorithm>
#include <cmath>

#include <bgfx/bgfx.h>

// See gpu_compose.cpp: our shaders carry no WebGPU build.
#define BGFX_PLATFORM_SUPPORTS_WGSL 0
#include <bgfx/embedded_shader.h>

#include "essl/fs_view360_eye.sc.bin.h"
#include "essl/vs_program.sc.bin.h"
#include "glsl/fs_view360_eye.sc.bin.h"
#include "glsl/vs_program.sc.bin.h"
#include "spirv/fs_view360_eye.sc.bin.h"
#include "spirv/vs_program.sc.bin.h"
#if defined(_WIN32)
#include "dxbc/fs_view360_eye.sc.bin.h"
#include "dxbc/vs_program.sc.bin.h"
#include "dxil/fs_view360_eye.sc.bin.h"
#include "dxil/vs_program.sc.bin.h"
#endif

namespace svj::ui {
namespace {

const bgfx::EmbeddedShader kShaders[] = {
    BGFX_EMBEDDED_SHADER(vs_program),
    BGFX_EMBEDDED_SHADER(fs_view360_eye),
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

constexpr double kPi = 3.14159265358979323846;

}  // namespace

bool EyeGpu::init(std::uint32_t width, std::uint32_t height, std::uint16_t view_id) {
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
        bgfx::createEmbeddedShader(kShaders, type, "fs_view360_eye"), true);
    if (!bgfx::isValid(program)) return false;
    program_ = program.idx;

    fov_uniform_ = bgfx::createUniform("u_eye_fov", bgfx::UniformType::Vec4).idx;
    rot_uniform_ = bgfx::createUniform("u_eye_rot", bgfx::UniformType::Vec4).idx;
    gaze_uniform_ = bgfx::createUniform("u_gaze", bgfx::UniformType::Vec4).idx;
    sampler_ = bgfx::createUniform("s_equirect", bgfx::UniformType::Sampler).idx;

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

void EyeGpu::destroy() {
    if (!ready_) return;
    if (framebuffer_ != 0xFFFF) {
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
    kill_uniform(fov_uniform_);
    kill_uniform(rot_uniform_);
    kill_uniform(gaze_uniform_);
    kill_uniform(sampler_);
    if (vertices_ != 0xFFFF) {
        bgfx::VertexBufferHandle v;
        v.idx = vertices_;
        bgfx::destroy(v);
        vertices_ = 0xFFFF;
    }
    ready_ = false;
}

void EyeGpu::render(std::uint16_t equirect, const SphereView& world, const EyeView& eye) {
    if (!ready_ || equirect == 0xFFFF) return;

    const auto radians = [](double degrees) {
        return static_cast<float>(degrees * kPi / 180.0);
    };

    const float fov[4] = {
        static_cast<float>(eye.fov.left), static_cast<float>(eye.fov.right),
        static_cast<float>(eye.fov.up), static_cast<float>(eye.fov.down)};
    // Normalised here rather than per fragment: a runtime may hand back a pose
    // slightly off unit length, and rescaling the world once per eye is free
    // while doing it per pixel is not.
    const Quat q = normalise(eye.orientation);
    const float rot[4] = {static_cast<float>(q.x), static_cast<float>(q.y),
                          static_cast<float>(q.z), static_cast<float>(q.w)};
    const float gaze[4] = {radians(world.yaw_deg), radians(world.pitch_deg),
                           radians(world.roll_deg), 0.0f};

    bgfx::UniformHandle u_fov, u_rot, u_gaze, u_sampler;
    u_fov.idx = fov_uniform_;
    u_rot.idx = rot_uniform_;
    u_gaze.idx = gaze_uniform_;
    u_sampler.idx = sampler_;
    bgfx::setUniform(u_fov, fov);
    bgfx::setUniform(u_rot, rot);
    bgfx::setUniform(u_gaze, gaze);
    bgfx::setTexture(0, u_sampler, tex_of(equirect));

    bgfx::VertexBufferHandle vb;
    vb.idx = vertices_;
    bgfx::setVertexBuffer(0, vb);
    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
    bgfx::ProgramHandle p;
    p.idx = program_;
    bgfx::submit(view_id_, p);
}

void* EyeGpu::imgui_texture() const {
    if (!ready_) return nullptr;
    return reinterpret_cast<void*>(static_cast<std::uint64_t>(target_) + 1);
}

}  // namespace svj::ui
