# scratchvj — the native front end.
#
# Kept behind an option, OFF by default, and in its own directory. Two reasons,
# both deliberate:
#
#   1. `scratchvj_core` has no external dependencies, which is what lets its
#      tests run on three operating systems from a bare checkout. Pulling SDL3
#      into the default build would quietly end that. The CI job keeps building
#      the default configuration, so the guarantee is actually enforced rather
#      than merely stated.
#   2. This layer is a VIEW. It reads the engine and draws it; it holds no
#      behaviour of its own, so nothing here needs to be testable and nothing
#      that is testable belongs here.
#
# The roadmap picked SDL3 for the window and Dear ImGui for the interface. bgfx
# is NOT here yet on purpose: it is for drawing video frames, and there are no
# decoded frames to draw until the FFmpeg analysis pass exists. ImGui's own
# SDL_Renderer backend carries the interface until then, and the seam it leaves
# is exactly where bgfx will attach.
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)

FetchContent_Declare(SDL3
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG release-3.4.16
  GIT_SHALLOW TRUE
)
FetchContent_Declare(imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.92.9
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(SDL3 imgui)

# Dear ImGui ships no CMake of its own; it is meant to be compiled into the app.
add_library(scratchvj_imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp
)
target_include_directories(scratchvj_imgui PUBLIC
  ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(scratchvj_imgui PUBLIC SDL3::SDL3)

if(WIN32)
  # Spout, pinned like the other dependencies. Only SpoutDX is built: the GL
  # half of the SDK serves applications that render with OpenGL, which this one
  # never will. /MT is switched off so Spout links the same runtime as SDL.
  set(SPOUT_BUILD_SPOUTDX ON CACHE BOOL "" FORCE)
  set(SPOUT_BUILD_LIBRARY OFF CACHE BOOL "" FORCE)
  set(SPOUT_BUILD_CMT OFF CACHE BOOL "" FORCE)
  set(SKIP_INSTALL_ALL ON CACHE BOOL "" FORCE)
  FetchContent_Declare(spout2
    GIT_REPOSITORY https://github.com/leadedge/Spout2.git
    GIT_TAG c2bcc12147711d12ace7d5f08e869d774d840f8a  # 2.007.017
  )
  FetchContent_MakeAvailable(spout2)
endif()

# bgfx, the renderer the roadmap chose: mature, permissive, native D3D/Vulkan/
# Metal backends, and first-class support for the BCn formats and texture
# arrays the video engine is built on. Tools and examples are off -- shaderc
# joins now: the compositor is the first scratchvj shader, and every shader is
# compiled at build time and validated against its CPU reference in core/.
set(BGFX_BUILD_TOOLS ON CACHE BOOL "" FORCE)
set(BGFX_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BGFX_INSTALL OFF CACHE BOOL "" FORCE)
set(BGFX_AMALGAMATED ON CACHE BOOL "" FORCE)
FetchContent_Declare(bgfx
  GIT_REPOSITORY https://github.com/bkaradzic/bgfx.cmake.git
  GIT_TAG 297c84a6ca5a43c0840059b2169ed05ff288c42c
  GIT_SHALLOW ON
)
FetchContent_MakeAvailable(bgfx)

# The scratchvj shaders, compiled by shaderc at build time into per-backend
# headers. Their fragment logic is a transcription of core/compose; gpu_check
# holds them to it.
include(${bgfx_SOURCE_DIR}/cmake/bgfxToolUtils.cmake)
# bgfxToolUtils expects BGFX_SHADER_INCLUDE_PATH, which bgfx.cmake only sets in
# its own directory scope; FetchContent leaves it unset here.
set(BGFX_SHADER_INCLUDE_PATH ${bgfx_SOURCE_DIR}/bgfx/src)
bgfx_compile_shaders(
  TYPE VERTEX
  SHADERS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/vs_program.sc
  VARYING_DEF ${CMAKE_CURRENT_SOURCE_DIR}/shaders/varying.def.sc
  OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated_shaders
  AS_HEADERS
  OUT_FILES_VAR SVJ_VS_HEADERS
)
bgfx_compile_shaders(
  TYPE FRAGMENT
  SHADERS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/fs_program.sc
          ${CMAKE_CURRENT_SOURCE_DIR}/shaders/fs_view360.sc
  VARYING_DEF ${CMAKE_CURRENT_SOURCE_DIR}/shaders/varying.def.sc
  OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated_shaders
  AS_HEADERS
  OUT_FILES_VAR SVJ_FS_HEADERS
)
# ImGui's own shaders, from bgfx's example sources but compiled BY US into the
# same generated tree. Including the examples directory instead would put its
# imgui.h ahead of the real one on the include path -- the collision that once
# broke this build while a too-broad log filter hid the errors.
bgfx_compile_shaders(
  TYPE VERTEX
  SHADERS ${bgfx_SOURCE_DIR}/bgfx/examples/common/imgui/vs_ocornut_imgui.sc
  VARYING_DEF ${bgfx_SOURCE_DIR}/bgfx/examples/common/imgui/varying.def.sc
  OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated_shaders
  AS_HEADERS
  OUT_FILES_VAR SVJ_IMGUI_VS_HEADERS
)
bgfx_compile_shaders(
  TYPE FRAGMENT
  SHADERS ${bgfx_SOURCE_DIR}/bgfx/examples/common/imgui/fs_ocornut_imgui.sc
  VARYING_DEF ${bgfx_SOURCE_DIR}/bgfx/examples/common/imgui/varying.def.sc
  OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated_shaders
  AS_HEADERS
  OUT_FILES_VAR SVJ_IMGUI_FS_HEADERS
)

# In its own library so the headless gpu_check tool uses the exact object code
# the application does -- validating a copy would validate nothing.
add_library(scratchvj_gpu STATIC gpu_compose.cpp gpu_view360.cpp ${SVJ_VS_HEADERS} ${SVJ_FS_HEADERS})
target_include_directories(scratchvj_gpu PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}/generated_shaders)
target_link_libraries(scratchvj_gpu PUBLIC bgfx bx bimg scratchvj_core)

add_executable(scratchvj_ui WIN32 imgui_impl_bgfx.cpp main_ui.cpp media.cpp netout.cpp
                                  panels.cpp share.cpp
                                  ${SVJ_IMGUI_VS_HEADERS} ${SVJ_IMGUI_FS_HEADERS})
target_link_libraries(scratchvj_ui PRIVATE scratchvj_app scratchvj_imgui SDL3::SDL3 bgfx
                                           scratchvj_gpu)

# The proof the shader matches its reference: same layers through core/compose
# and through fs_program on a hidden window, compared channel for channel.
add_executable(gpu_check tools/gpu_check.cpp)
target_link_libraries(gpu_check PRIVATE scratchvj_core scratchvj_gpu SDL3::SDL3 bgfx bx bimg)

# And the 360 view pass to core/sphere, same bar.
add_executable(sphere_check tools/sphere_check.cpp)
target_link_libraries(sphere_check PRIVATE scratchvj_core scratchvj_gpu SDL3::SDL3 bgfx bx bimg)
if(WIN32)
  target_link_libraries(scratchvj_ui PRIVATE SpoutDX_static d3d11 dxgi)

  # The proof of the output: a receiver built from the same SDK, so the Spout
  # path is testable end to end with no third-party software installed.
  add_executable(spout_check tools/spout_check.cpp)
  target_link_libraries(spout_check PRIVATE SpoutDX_static d3d11 dxgi)

  # Same idea for the UDP control stream: a listener decoding with the very
  # core/protocol the packets were encoded with.
  add_executable(net_check tools/net_check.cpp)
  target_link_libraries(net_check PRIVATE scratchvj_core ws2_32)
endif()

# The vendored sources are not ours to keep warning-clean, so the strict flags
# apply to our own translation units only.
if(MSVC)
  target_compile_options(scratchvj_ui PRIVATE /W4 /permissive-)
else()
  target_compile_options(scratchvj_ui PRIVATE -Wall -Wextra -Wpedantic)
endif()

# SDL3 is built shared, so the DLL has to sit next to the executable, and the
# fonts are loaded at runtime relative to it (SDL_GetBasePath), so they travel
# with it too.
add_custom_command(TARGET scratchvj_ui POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          $<TARGET_FILE:SDL3::SDL3> $<TARGET_FILE_DIR:scratchvj_ui>
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_CURRENT_SOURCE_DIR}/fonts $<TARGET_FILE_DIR:scratchvj_ui>/fonts)
