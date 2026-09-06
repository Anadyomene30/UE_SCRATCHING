#include "imgui_impl_bgfx.h"

#include <cstddef>
#include <cstring>

#include <bgfx/bgfx.h>

// See gpu_compose.cpp: our shaders carry no WebGPU build.
#define BGFX_PLATFORM_SUPPORTS_WGSL 0
#include <bgfx/embedded_shader.h>

#include "imgui.h"

// ImGui's shaders, compiled by our own shaderc pass from bgfx's example
// sources -- see the CMake note about why the example directory itself is
// never on the include path.
#include "essl/fs_ocornut_imgui.sc.bin.h"
#include "essl/vs_ocornut_imgui.sc.bin.h"
#include "glsl/fs_ocornut_imgui.sc.bin.h"
#include "glsl/vs_ocornut_imgui.sc.bin.h"
#include "spirv/fs_ocornut_imgui.sc.bin.h"
#include "spirv/vs_ocornut_imgui.sc.bin.h"
#if defined(_WIN32)
#include "dxbc/fs_ocornut_imgui.sc.bin.h"
#include "dxbc/vs_ocornut_imgui.sc.bin.h"
#include "dxil/fs_ocornut_imgui.sc.bin.h"
#include "dxil/vs_ocornut_imgui.sc.bin.h"
#endif

namespace svj::ui {
namespace {

const bgfx::EmbeddedShader kShaders[] = {
    BGFX_EMBEDDED_SHADER(vs_ocornut_imgui),
    BGFX_EMBEDDED_SHADER(fs_ocornut_imgui),
    BGFX_EMBEDDED_SHADER_END(),
};

struct Backend {
    std::uint16_t view_id = 0;
    bgfx::ProgramHandle program = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle sampler = BGFX_INVALID_HANDLE;
    bgfx::VertexLayout layout;
};

Backend* g_backend = nullptr;

bgfx::TextureHandle handle_of(ImTextureID id) {
    bgfx::TextureHandle handle;
    handle.idx = static_cast<std::uint16_t>(static_cast<std::uint64_t>(id) - 1);
    return handle;
}

// The 1.92 contract: the draw data carries texture requests, and the renderer
// is the one place they are honoured. The font atlas is simply the first
// texture to ask.
void process_texture(ImTextureData* texture) {
    if (texture->Status == ImTextureStatus_WantCreate) {
        IM_ASSERT(texture->Format == ImTextureFormat_RGBA32);
        const bgfx::Memory* memory =
            bgfx::copy(texture->GetPixels(), static_cast<std::uint32_t>(
                                                 texture->Width * texture->Height * 4));
        const bgfx::TextureHandle handle = bgfx::createTexture2D(
            static_cast<std::uint16_t>(texture->Width),
            static_cast<std::uint16_t>(texture->Height), false, 1,
            bgfx::TextureFormat::RGBA8, BGFX_SAMPLER_NONE, memory);
        texture->SetTexID(ImGuiBgfx_TextureId(handle.idx));
        texture->SetStatus(ImTextureStatus_OK);
        return;
    }

    if (texture->Status == ImTextureStatus_WantUpdates) {
        const bgfx::TextureHandle handle = handle_of(texture->TexID);
        for (const ImTextureRect& rect : texture->Updates) {
            // bgfx wants a tight rectangle; the atlas rows are strided, so the
            // region is repacked line by line.
            const std::uint32_t bytes =
                static_cast<std::uint32_t>(rect.w) * rect.h * 4;
            const bgfx::Memory* memory = bgfx::alloc(bytes);
            for (int row = 0; row < rect.h; ++row) {
                memcpy(memory->data + static_cast<std::size_t>(row) * rect.w * 4,
                       texture->GetPixelsAt(rect.x, rect.y + row),
                       static_cast<std::size_t>(rect.w) * 4);
            }
            bgfx::updateTexture2D(handle, 0, 0, rect.x, rect.y, rect.w, rect.h, memory);
        }
        texture->SetStatus(ImTextureStatus_OK);
        return;
    }

    if (texture->Status == ImTextureStatus_WantDestroy && texture->UnusedFrames > 0) {
        bgfx::destroy(handle_of(texture->TexID));
        texture->SetTexID(ImTextureID_Invalid);
        texture->SetStatus(ImTextureStatus_Destroyed);
    }
}

}  // namespace

std::uint64_t ImGuiBgfx_TextureId(std::uint16_t handle_index) {
    return static_cast<std::uint64_t>(handle_index) + 1;
}

bool ImGuiBgfx_Init(std::uint16_t view_id) {
    IM_ASSERT(g_backend == nullptr);
    g_backend = new Backend();
    g_backend->view_id = view_id;

    const bgfx::RendererType::Enum type = bgfx::getRendererType();
    g_backend->program = bgfx::createProgram(
        bgfx::createEmbeddedShader(kShaders, type, "vs_ocornut_imgui"),
        bgfx::createEmbeddedShader(kShaders, type, "fs_ocornut_imgui"), true);
    if (!bgfx::isValid(g_backend->program)) return false;

    g_backend->sampler = bgfx::createUniform("s_tex", bgfx::UniformType::Sampler);

    g_backend->layout.begin()
        .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
        .end();

    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = "imgui_impl_bgfx (scratchvj)";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
    return true;
}

void ImGuiBgfx_Shutdown() {
    if (g_backend == nullptr) return;
    // Textures the contract says we own.
    for (ImTextureData* texture : ImGui::GetPlatformIO().Textures) {
        if (texture->RefCount == 1 && texture->TexID != ImTextureID_Invalid) {
            bgfx::destroy(handle_of(texture->TexID));
            texture->SetTexID(ImTextureID_Invalid);
            texture->SetStatus(ImTextureStatus_Destroyed);
        }
    }
    bgfx::destroy(g_backend->sampler);
    bgfx::destroy(g_backend->program);
    ImGui::GetIO().BackendRendererName = nullptr;
    delete g_backend;
    g_backend = nullptr;
}

void ImGuiBgfx_Render(ImDrawData* draw_data) {
    Backend& backend = *g_backend;

    if (draw_data->Textures != nullptr) {
        for (ImTextureData* texture : *draw_data->Textures) {
            if (texture->Status != ImTextureStatus_OK) process_texture(texture);
        }
    }

    const float width = draw_data->DisplaySize.x * draw_data->FramebufferScale.x;
    const float height = draw_data->DisplaySize.y * draw_data->FramebufferScale.y;
    if (width <= 0.0f || height <= 0.0f) return;

    // One orthographic view over the framebuffer; ImGui's coordinates are the
    // window's.
    {
        float ortho[16];
        const float L = draw_data->DisplayPos.x;
        const float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
        const float T = draw_data->DisplayPos.y;
        const float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
        memset(ortho, 0, sizeof(ortho));
        ortho[0] = 2.0f / (R - L);
        ortho[5] = 2.0f / (T - B);
        ortho[10] = -1.0f;
        ortho[12] = (R + L) / (L - R);
        ortho[13] = (T + B) / (B - T);
        ortho[15] = 1.0f;
        bgfx::setViewTransform(backend.view_id, nullptr, ortho);
        bgfx::setViewRect(backend.view_id, 0, 0, static_cast<std::uint16_t>(width),
                          static_cast<std::uint16_t>(height));
    }

    const ImVec2 clip_off = draw_data->DisplayPos;
    const ImVec2 clip_scale = draw_data->FramebufferScale;

    // CmdLists is the truth; the legacy CmdListsCount int is no longer kept in
    // step by every ImGui version, and iterating it renders nothing at all --
    // the bug that once left this window empty while metrics counted 5508
    // vertices that were really there.
    for (const ImDrawList* commands : draw_data->CmdLists) {

        const std::uint32_t vertex_count =
            static_cast<std::uint32_t>(commands->VtxBuffer.Size);
        const std::uint32_t index_count =
            static_cast<std::uint32_t>(commands->IdxBuffer.Size);
        if (bgfx::getAvailTransientVertexBuffer(vertex_count, backend.layout) <
                vertex_count ||
            bgfx::getAvailTransientIndexBuffer(index_count) < index_count) {
            break;  // out of transient space this frame; drop the rest, not the app
        }

        bgfx::TransientVertexBuffer vertices;
        bgfx::TransientIndexBuffer indices;
        bgfx::allocTransientVertexBuffer(&vertices, vertex_count, backend.layout);
        bgfx::allocTransientIndexBuffer(&indices, index_count);
        memcpy(vertices.data, commands->VtxBuffer.Data,
               vertex_count * sizeof(ImDrawVert));
        memcpy(indices.data, commands->IdxBuffer.Data, index_count * sizeof(ImDrawIdx));

        for (const ImDrawCmd& command : commands->CmdBuffer) {
            if (command.UserCallback != nullptr) {
                command.UserCallback(commands, &command);
                continue;
            }
            if (command.ElemCount == 0) continue;

            ImVec2 clip_min((command.ClipRect.x - clip_off.x) * clip_scale.x,
                            (command.ClipRect.y - clip_off.y) * clip_scale.y);
            ImVec2 clip_max((command.ClipRect.z - clip_off.x) * clip_scale.x,
                            (command.ClipRect.w - clip_off.y) * clip_scale.y);
            if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y) continue;
            clip_min.x = clip_min.x < 0.0f ? 0.0f : clip_min.x;
            clip_min.y = clip_min.y < 0.0f ? 0.0f : clip_min.y;

            bgfx::setScissor(
                static_cast<std::uint16_t>(clip_min.x),
                static_cast<std::uint16_t>(clip_min.y),
                static_cast<std::uint16_t>(clip_max.x - clip_min.x),
                static_cast<std::uint16_t>(clip_max.y - clip_min.y));

            bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                           BGFX_STATE_MSAA |
                           BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA,
                                                 BGFX_STATE_BLEND_INV_SRC_ALPHA));

            bgfx::setTexture(0, backend.sampler, handle_of(command.GetTexID()));
            bgfx::setVertexBuffer(0, &vertices, command.VtxOffset, vertex_count);
            bgfx::setIndexBuffer(&indices, command.IdxOffset, command.ElemCount);
            bgfx::submit(backend.view_id, backend.program);
        }
    }
}

}  // namespace svj::ui
