# Windows (MSVC): compiler flags, host layer sources and the executable's resources.
# Included by CMakeLists.txt right after project(), before PCERecomp and SDL3 (the flags apply to them too).
# Moved as is from CMakeLists.txt (M1): the flags are the same as before.

if(MSVC)
    add_compile_options(/W4 /utf-8)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    # .pdb in Release too, so the profiler (pce_trace --profile) can name the functions.
    # Does not change the generated code; /OPT:REF,ICF keeps the .exe the same size as before.
    add_compile_options($<$<CONFIG:Release>:/Zi>)
    add_link_options($<$<CONFIG:Release>:/DEBUG> $<$<CONFIG:Release>:/OPT:REF> $<$<CONFIG:Release>:/OPT:ICF>)
    # C library inside the .exe (/MT), everywhere (SDL included): YsRecomp.exe runs on a Windows without the
    # "Visual C++ Redistributable" package installed.
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    enable_language(RC)
endif()

# Host layer (runtime/host): the Windows implementations.
set(PLATFORM_HOST_SOURCES
    runtime/host/win/display_timing_dwm.c
    runtime/host/win/process_win.c
    runtime/host/win/gpu_timing_d3d11.c
    runtime/host/win/paths_win.c)
set(PLATFORM_HOST_LIBRARIES dwmapi)
# Where the user's data goes (test host_paths): next to the executable
set(PLATFORM_USER_DIR exe)
# The player package's test (app_package_ys): the .exe, its icon and the folder next to it
set(PLATFORM_PACKAGE_TEST tools/package/run_package_test.py)

# Generated C (~40 MB in 24 parts)
function(platform_generated_code target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W1 /bigobj /MP)  # /MP: the 24 parts in parallel
    endif()
endfunction()

# SDL3 (built from its sources): nothing to add on Windows
function(platform_sdl target)
endfunction()

# The player's executable: icon and version information
function(platform_app_executable target)
    target_sources(${target} PRIVATE runtime/host/win/ysrecomp.rc)
endfunction()
