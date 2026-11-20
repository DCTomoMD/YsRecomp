# macOS (Apple Clang, Apple Silicon): compiler flags and host layer sources.
# Included by CMakeLists.txt right after project(), before PCERecomp and SDL3 (the flags apply to them too).
# M1: skeleton. The full macOS build (generated C, .app, Info.plist, icns, warnings of the generated code) is M3.

# arm64 only (decision D9)
if(NOT CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES arm64 CACHE STRING "macOS architectures" FORCE)
endif()

# No FMA contraction, so the floating-point results match the Windows build (decision D4)
add_compile_options(-ffp-contract=off)

# Apple's linker warns when a library appears twice on a link line (pce_core comes directly and again through pce_app);
# it links it once either way, so the warning says nothing about the result (M3-3)
add_link_options(-Wl,-no_warn_duplicate_libraries)

# Host layer (runtime/host): the user-data folder (mac/); the rest has no macOS implementation yet
# (null: no refresh time, no GPU time, no priority change)
set(PLATFORM_HOST_SOURCES
    runtime/host/null/display_timing_null.c
    runtime/host/null/process_null.c
    runtime/host/null/gpu_timing_null.c
    runtime/host/mac/paths_mac.c)
set(PLATFORM_HOST_LIBRARIES)
# Where the user's data goes (test host_paths): ~/Library/Application Support/<app id>/ (decision D3)
set(PLATFORM_USER_DIR pref)

# Generated C: at most 4 files compiled at the same time (M3-1: the largest part takes ~2.6 GB and ~2 min with
# Apple Clang -O3; 4 at a time fits in 24 GB with room for the rest of the build). No warning flags: none came out.
set_property(GLOBAL APPEND PROPERTY JOB_POOLS generated_c=4)
function(platform_generated_code target)
    set_property(TARGET ${target} PROPERTY JOB_POOL_COMPILE generated_c)
endfunction()

# The player's executable: .app bundle, Info.plist and icon come in M3
function(platform_app_executable target)
endfunction()
