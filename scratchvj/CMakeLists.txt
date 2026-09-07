cmake_minimum_required(VERSION 3.20)
project(scratchvj LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE)
endif()

# --- scratchvj_core -----------------------------------------------------------
#
# Zero external dependencies on purpose. Everything here is pure logic -- the
# surface model, the mapping engine, gesture detection and the wire format -- so
# it compiles and its tests run on Windows, macOS and Linux from day one. That is
# what makes the macOS CI job meaningful before any audio or GPU code exists.
add_library(scratchvj_core STATIC
  src/core/anchor.cpp
  src/core/bc1.cpp
  src/core/compose.cpp
  src/core/curve.cpp
  src/core/effect.cpp
  src/core/framewindow.cpp
  src/core/gestures.cpp
  src/core/headset.cpp
  src/core/layout.cpp
  src/core/learn.cpp
  src/core/library.cpp
  src/core/livering.cpp
  src/core/mapping.cpp
  src/core/mesh.cpp
  src/core/midi.cpp
  src/core/mixer.cpp
  src/core/modulator.cpp
  src/core/playback.cpp
  src/core/protocol.cpp
  src/core/quadrature.cpp
  src/core/spectrum.cpp
  src/core/sphere.cpp
  src/core/surface.cpp
  src/core/take.cpp
  src/core/timecode.cpp
  src/core/transport.cpp
  src/core/videocache.cpp
  src/core/videofx.cpp
  src/core/videotaps.cpp
  src/core/warp.cpp
)
target_include_directories(scratchvj_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# --- scratchvj_config ---------------------------------------------------------
#
# Split out so the JSON library, which is a large header, is compiled exactly
# once instead of by every translation unit that happens to touch a mapping.
add_library(scratchvj_config STATIC src/config/mapping_io.cpp src/config/warp_io.cpp)
target_include_directories(scratchvj_config
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/vendor
)
target_link_libraries(scratchvj_config PUBLIC scratchvj_core)

if(MSVC)
  target_compile_options(scratchvj_core PRIVATE /W4 /permissive-)
  target_compile_options(scratchvj_config PRIVATE /W4 /permissive-)
else()
  target_compile_options(scratchvj_core PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_options(scratchvj_config PRIVATE -Wall -Wextra -Wpedantic)
endif()

# --- scratchvj_dvs ------------------------------------------------------------
#
# The timecode decoder: xwax's timecoder.c, vendored, behind a thin C++ bridge.
# Its own library and OFF by default, for two reasons that both matter.
#
#   1. `scratchvj_core` has no external dependency, which is what lets it be
#      tested from a bare checkout on three operating systems. Vendored C in
#      core would end that.
#   2. xwax is GPL-3. A separate target keeps the licence boundary visible
#      rather than implicit.
#
# The vendored sources are not ours to keep warning-clean, so the strict flags
# apply to our bridge only.
option(SCRATCHVJ_BUILD_DVS "Build the xwax timecode decoder" OFF)

if(SCRATCHVJ_BUILD_DVS)
  add_library(scratchvj_dvs STATIC
    dvs/decoder.cpp
    dvs/generate.cpp
    vendor/xwax/timecoder.c
    vendor/xwax/lut.c
  )
  target_include_directories(scratchvj_dvs
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
           ${CMAKE_CURRENT_SOURCE_DIR}/src
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/vendor/xwax
  )
  target_link_libraries(scratchvj_dvs PUBLIC scratchvj_core)
  set_source_files_properties(
    vendor/xwax/timecoder.c vendor/xwax/lut.c PROPERTIES LANGUAGE C)
  if(MSVC)
    target_compile_options(scratchvj_dvs PRIVATE
      $<$<COMPILE_LANGUAGE:CXX>:/W4> $<$<COMPILE_LANGUAGE:CXX>:/permissive->)
  else()
    target_compile_options(scratchvj_dvs PRIVATE
      $<$<COMPILE_LANGUAGE:CXX>:-Wall> $<$<COMPILE_LANGUAGE:CXX>:-Wextra>)
  endif()

  # Which definition, on which channel pair, actually locks against the real
  # signal -- and which sign of the pitch means forwards. WASAPI only, so no SDL
  # or bgfx: this is an audio question.
  if(WIN32)
    add_executable(dvs_check dvs/tools/dvs_check.cpp)
    target_link_libraries(dvs_check PRIVATE scratchvj_dvs ole32)
  endif()
endif()

# --- scratchvj (application) --------------------------------------------------
#
# The demo runs the whole engine off a scripted performance, so the behaviour can
# be watched and recorded before any hardware is connected.
# The simulation and the dashboard live in a library rather than in the
# executable, so the demo's behaviour is covered by tests instead of only ever
# being eyeballed.
add_library(scratchvj_app STATIC
  src/app/analyze.cpp
  src/app/dashboard.cpp
  src/app/engine.cpp
  src/app/simulation.cpp
)
target_include_directories(scratchvj_app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(scratchvj_app PUBLIC scratchvj_core)

add_executable(scratchvj src/app/main.cpp)
target_link_libraries(scratchvj PRIVATE scratchvj_app)

if(MSVC)
  target_compile_options(scratchvj_app PRIVATE /W4 /permissive-)
  target_compile_options(scratchvj PRIVATE /W4 /permissive-)
else()
  target_compile_options(scratchvj_app PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_options(scratchvj PRIVATE -Wall -Wextra -Wpedantic)
endif()

# --- the native front end -----------------------------------------------------
#
# OFF by default and in its own directory. The default build stays free of every
# external dependency, which is what keeps the core testable from a bare checkout
# on three operating systems -- and what makes the CI job that builds the default
# configuration a real guarantee rather than a habit.
option(SCRATCHVJ_BUILD_UI "Build the SDL3 + Dear ImGui front end" OFF)

if(SCRATCHVJ_BUILD_UI)
  add_subdirectory(ui)
endif()

# --- tests --------------------------------------------------------------------
option(SCRATCHVJ_BUILD_TESTS "Build the scratchvj test suite" ON)

if(SCRATCHVJ_BUILD_TESTS)
  enable_testing()
  add_executable(scratchvj_tests
    tests/harness.cpp
    tests/test_anchor.cpp
    tests/test_bc1.cpp
    tests/test_compose.cpp
    tests/test_app.cpp
    tests/test_config.cpp
    tests/test_curve.cpp
    tests/test_effect.cpp
    tests/test_engine.cpp
    tests/test_framewindow.cpp
    tests/test_gestures.cpp
    tests/test_headset.cpp
    tests/test_layout.cpp
    tests/test_learn.cpp
    tests/test_library.cpp
    tests/test_livering.cpp
    tests/test_mapping.cpp
    tests/test_mesh.cpp
    tests/test_midi.cpp
    tests/test_mixer.cpp
    tests/test_modulator.cpp
    tests/test_playback.cpp
    tests/test_protocol.cpp
    tests/test_quadrature.cpp
    tests/test_spectrum.cpp
    tests/test_sphere.cpp
    tests/test_surface.cpp
    tests/test_take.cpp
    tests/test_timecode.cpp
    tests/test_transport.cpp
    tests/test_videocache.cpp
    tests/test_videofx.cpp
    tests/test_videotaps.cpp
    tests/test_warp.cpp
    tests/test_warp_io.cpp
  )
  target_include_directories(scratchvj_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
  target_link_libraries(scratchvj_tests PRIVATE scratchvj_app scratchvj_config)

  # The DVS tests join the same binary when the decoder is built, so they run
  # under the same `ctest` rather than needing to be remembered separately.
  if(SCRATCHVJ_BUILD_DVS)
    target_sources(scratchvj_tests PRIVATE tests/test_dvs.cpp)
    target_link_libraries(scratchvj_tests PRIVATE scratchvj_dvs)
  endif()

  add_test(NAME scratchvj_core_tests COMMAND scratchvj_tests)
endif()
