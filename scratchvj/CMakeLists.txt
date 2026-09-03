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
  src/core/curve.cpp
  src/core/framewindow.cpp
  src/core/gestures.cpp
  src/core/layout.cpp
  src/core/learn.cpp
  src/core/mapping.cpp
  src/core/midi.cpp
  src/core/protocol.cpp
  src/core/surface.cpp
  src/core/take.cpp
  src/core/timecode.cpp
  src/core/transport.cpp
  src/core/videocache.cpp
)
target_include_directories(scratchvj_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# --- scratchvj_config ---------------------------------------------------------
#
# Split out so the JSON library, which is a large header, is compiled exactly
# once instead of by every translation unit that happens to touch a mapping.
add_library(scratchvj_config STATIC src/config/mapping_io.cpp)
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

# --- scratchvj (application) --------------------------------------------------
#
# The demo runs the whole engine off a scripted performance, so the behaviour can
# be watched and recorded before any hardware is connected.
# The simulation and the dashboard live in a library rather than in the
# executable, so the demo's behaviour is covered by tests instead of only ever
# being eyeballed.
add_library(scratchvj_app STATIC
  src/app/dashboard.cpp
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

# --- tests --------------------------------------------------------------------
option(SCRATCHVJ_BUILD_TESTS "Build the scratchvj test suite" ON)

if(SCRATCHVJ_BUILD_TESTS)
  enable_testing()
  add_executable(scratchvj_tests
    tests/harness.cpp
    tests/test_anchor.cpp
    tests/test_app.cpp
    tests/test_config.cpp
    tests/test_curve.cpp
    tests/test_framewindow.cpp
    tests/test_gestures.cpp
    tests/test_layout.cpp
    tests/test_learn.cpp
    tests/test_mapping.cpp
    tests/test_midi.cpp
    tests/test_protocol.cpp
    tests/test_surface.cpp
    tests/test_take.cpp
    tests/test_timecode.cpp
    tests/test_transport.cpp
    tests/test_videocache.cpp
  )
  target_include_directories(scratchvj_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
  target_link_libraries(scratchvj_tests PRIVATE scratchvj_app scratchvj_config)
  add_test(NAME scratchvj_core_tests COMMAND scratchvj_tests)
endif()
