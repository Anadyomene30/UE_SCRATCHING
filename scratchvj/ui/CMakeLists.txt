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

add_executable(scratchvj_ui WIN32 main_ui.cpp panels.cpp)
target_link_libraries(scratchvj_ui PRIVATE scratchvj_app scratchvj_imgui SDL3::SDL3)

# The vendored sources are not ours to keep warning-clean, so the strict flags
# apply to our own translation units only.
if(MSVC)
  target_compile_options(scratchvj_ui PRIVATE /W4 /permissive-)
else()
  target_compile_options(scratchvj_ui PRIVATE -Wall -Wextra -Wpedantic)
endif()

# SDL3 is built shared, so the DLL has to sit next to the executable.
add_custom_command(TARGET scratchvj_ui POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          $<TARGET_FILE:SDL3::SDL3> $<TARGET_FILE_DIR:scratchvj_ui>)
