cmake_minimum_required(VERSION 3.20)
project(ue_scratching LANGUAGES CXX)

# Must be called from the top-level list file, otherwise ctest registers nothing
# in the root build directory and `ctest --test-dir build` finds no tests.
enable_testing()

add_subdirectory(scratchvj)
