// scratchvj — Dear ImGui rendered by bgfx.
//
// The SDL_Renderer backend served while every pixel on screen was a widget.
// It could not survive the video engine: SDL's renderer cannot upload
// block-compressed textures, so every displayed frame took a CPU decode that
// the whole .svcache design exists to avoid. bgfx takes BC1 as-is.
//
// This is a from-scratch backend rather than a copy of the one in bgfx's
// examples, because that one is welded to bgfx's own entry/input framework and
// predates ImGui 1.92's texture contract. This one implements 1.92 properly:
// the renderer owns texture creation, updates and destruction, driven by the
// ImTextureData requests that arrive with the draw data (the font atlas is
// just the first customer). ImTextureID carries a bgfx handle as idx+1, so a
// null id can never collide with bgfx's perfectly valid handle 0.
//
// The shaders are the ones bgfx EMBEDS for its ImGui example -- prebuilt for
// every backend (D3D, Vulkan, Metal, GL) -- which is what lets this compile
// without shaderc in the toolchain. shaderc joins when the first scratchvj
// shader does.
#pragma once

#include <cstdint>

struct ImDrawData;

namespace svj::ui {

// Call after ImGui::CreateContext(), with the bgfx view id ImGui will draw
// into. bgfx must already be initialised.
bool ImGuiBgfx_Init(std::uint16_t view_id);
void ImGuiBgfx_Shutdown();

// Renders one frame's draw data. Handles the texture requests attached to it.
void ImGuiBgfx_Render(ImDrawData* draw_data);

// Wraps a live bgfx texture handle index as an ImTextureID (idx + 1).
std::uint64_t ImGuiBgfx_TextureId(std::uint16_t handle_index);

}  // namespace svj::ui
