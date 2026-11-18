# macOS (Apple Clang, Apple Silicon): compiler flags and host layer sources.
# Included by CMakeLists.txt right after project(), before PCERecomp and SDL3 (the flags apply to them too).
# M1: skeleton. The full macOS build (generated C, .app, Info.plist, icns, warnings of the generated code) is M3.

# arm64 only (decision D9)
if(NOT CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES arm64 CACHE STRING "macOS architectures" FORCE)
endif()

# No FMA contraction, so the floating-point results match the Windows build (decision D4)
add_compile_options(-ffp-contract=off)

# Host layer (runtime/host): no macOS implementation yet (null: no refresh time, no GPU time, no priority change)
set(PLATFORM_HOST_SOURCES
    runtime/host/null/display_timing_null.c
    runtime/host/null/process_null.c
    runtime/host/null/gpu_timing_null.c)
set(PLATFORM_HOST_LIBRARIES)

# Generated C: flags come in M3
function(platform_generated_code target)
endfunction()

# The player's executable: .app bundle, Info.plist and icon come in M3
function(platform_app_executable target)
endfunction()
