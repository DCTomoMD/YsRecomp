cmake_minimum_required(VERSION 3.24)
project(YsRecomp C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Single-configuration generators (Ninja, Makefiles): Release unless chosen otherwise. The multi-configuration ones
# (Visual Studio, Ninja Multi-Config) pick it at build time (--config Release).
get_property(YS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT YS_MULTI_CONFIG AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Operating system: compiler flags, host layer sources (runtime/host), generated-code flags and the executable's
# resources. Each file defines PLATFORM_HOST_SOURCES, PLATFORM_HOST_LIBRARIES, platform_generated_code(target) and
# platform_app_executable(target). Edition (USA / JP) differences never go there.
if(WIN32)
    include(cmake/platform_windows.cmake)
elseif(APPLE)
    include(cmake/platform_macos.cmake)
else()
    message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME} (Windows and macOS only: cmake/platform_*.cmake)")
endif()

# All .exe files in build\Release (PCERecomp's too), where the scripts look for them.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

# User's disc and BIOS (roms/, outside Git), per edition: -DYS_USA_CUE=... -DYS_USA_BIOS=... pick the files; left
# empty, roms/<upstream name> and then roms/usa/ (disc) or roms/bios/ (BIOS) (cmake/ys_edition.cmake, ys_rom_file).
include(cmake/ys_edition.cmake)
ys_rom_file(YS_USA_CUE "Ys Book I & II (USA).cue" usa "USA disc (.cue)")
ys_rom_file(YS_USA_BIOS "[BIOS] TurboGrafx CD System Card (USA) (v2.0).pce" bios "USA System Card 2.0 (.pce)")

# PCERecomp (the emulated console and the tools): the working copy next to this folder (..\PCERecomp) when it
# exists (development of both projects side by side), otherwise the submodule in external/PCERecomp (a clone made
# with --recursive). PCERECOMP_DIR picks another one.
set(PCERECOMP_DIR "" CACHE PATH "PCERecomp folder")
if(NOT PCERECOMP_DIR)
    if(EXISTS "${CMAKE_SOURCE_DIR}/../PCERecomp/CMakeLists.txt")
        get_filename_component(PCERECOMP_DIR "${CMAKE_SOURCE_DIR}/../PCERecomp" ABSOLUTE)
    else()
        set(PCERECOMP_DIR "${CMAKE_SOURCE_DIR}/external/PCERecomp")
    endif()
endif()
if(NOT EXISTS "${PCERECOMP_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "PCERecomp not found at ${PCERECOMP_DIR} (git submodule update --init, or -DPCERECOMP_DIR=...)")
endif()
set(PCE_TEST_CUE "${YS_USA_CUE}")  # PCERecomp's cd_image test also checks the Ys disc
set(PCE_GAME_TRACE ON)          # pce_trace is built here (with the Ys extension)
add_subdirectory("${PCERECOMP_DIR}" pcerecomp)
set(GG_TRACE "${PCERECOMP_DIR}/tools/reference_trace/build/Release/gg_trace${CMAKE_EXECUTABLE_SUFFIX}")

# SDL3, downloaded and built as a static library (no DLL needed next to the .exe).
include(FetchContent)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
# The official release file, with its SHA256 as published on the release page (checked on download).
FetchContent_Declare(SDL3
    URL https://github.com/libsdl-org/SDL/releases/download/release-3.2.30/SDL3-3.2.30.tar.gz
    URL_HASH SHA256=4c3b09330d866dc52eb65b66259a6684ad387252ca8c57901b3a2b534eb42e3d
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(SDL3)

# Cross-file optimization (LTCG) in Release, for our targets (SDL, created above, is left out):
# the hardware clock calls small functions from several files on every instruction.
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)

# runtime/app (this project) next to runtime/{cpu,pce,cd,bios} (PCERecomp)
include_directories(runtime)

# PC version: portable configuration and identification of the disc chosen by the player.
add_library(pce_app STATIC
    runtime/app/config.c
    runtime/app/disc_id.c
    runtime/app/volume.c
    runtime/app/menu.c
    runtime/app/menu_font.c
    runtime/app/menu_chip.c
    runtime/app/input.c
    runtime/app/intro_skip.c
    runtime/app/video.c
    runtime/app/pacing.c
    runtime/app/present.c
    runtime/app/redraw.c
    runtime/app/equip.c
    runtime/app/battle.c
    runtime/app/game_fixes.c
    runtime/app/motion.c runtime/app/ys_masks.c
    runtime/app/gpu_draw.c
    runtime/app/ys_engine.c
    runtime/app/ys_windows.c runtime/app/ys_scroll.c runtime/app/ys_screen.c runtime/app/ys_music.c runtime/app/ys_dialog.c runtime/app/ys_intro.c runtime/app/ys_fade.c runtime/app/ys_panel.c runtime/app/ys_reward.c
    runtime/app/crop.c
    runtime/app/fade.c
    runtime/app/launcher.c
    runtime/app/launcher_sound.c
    runtime/app/launcher_image.c
    runtime/host/common/host_paths_sdl.c
    ${PLATFORM_HOST_SOURCES})
target_include_directories(pce_app PUBLIC runtime)
target_link_libraries(pce_app PUBLIC pce_core SDL3::SDL3-static ${PLATFORM_HOST_LIBRARIES})
# Launcher: the user's JPG images, decoded with the stb_image bundled in the SDL sources
set_source_files_properties(runtime/app/launcher_image.c PROPERTIES INCLUDE_DIRECTORIES "${sdl3_SOURCE_DIR}/src/video")

# Main program (window), with the interpreter.
add_executable(pce_runtime runtime/main.c)
target_link_libraries(pce_runtime PRIVATE pce_core pce_app pce_input trace_profiler SDL3::SDL3-static)

# PCERecomp's trace generator (its tools/trace/pce_trace.c; comparison with the reference emulator) with the
# Ys extension (tools/trace/ys_trace_ext.c: the PC version's options; pce_trace --help).
pce_trace_executable(pce_trace tools/trace/ys_trace_ext.c tools/trace/ys_actors.c tools/trace/hud_old.c runtime/app/ys_panel.c runtime/app/ys_fade.c runtime/app/menu_chip.c runtime/app/redraw.c runtime/app/game_fixes.c runtime/app/motion.c runtime/app/ys_masks.c
    runtime/app/equip.c runtime/app/equip_none.c
    runtime/app/gpu_draw.c
    runtime/app/ys_engine.c runtime/app/ys_windows.c runtime/app/ys_scroll.c runtime/app/ys_screen.c runtime/app/ys_music.c runtime/app/ys_dialog.c runtime/app/crop.c runtime/app/fade.c runtime/app/menu_font.c runtime/app/battle.c runtime/app/ys_reward.c)
target_link_libraries(pce_trace PRIVATE SDL3::SDL3-static)

# ---- Tests ----
enable_testing()


# Configuration (YsRecomp.ini) and identification of the disc in the chosen folder (error cases with
# altered copies of the user's disc in build/app_output; without the disc, the disc part is skipped).
add_executable(app_tests tests/app/app_tests.c)
target_link_libraries(app_tests PRIVATE pce_app)
add_test(NAME app_config COMMAND app_tests config "${CMAKE_BINARY_DIR}/app_output/config")
# Separate volumes (Mednafen balance) and the Geargrafx mix unchanged.
add_test(NAME app_volume COMMAND app_tests volume "${CMAKE_BINARY_DIR}/app_output/volume")
# Options menu (keys, volumes, exit confirmation, images by CRC).
add_test(NAME app_menu COMMAND app_tests menu "${CMAKE_BINARY_DIR}/app_output/menu")
add_test(NAME app_video COMMAND app_tests video "${CMAKE_BINARY_DIR}/app_output/video")
# Image options drawn with a software renderer (no window), checked pixel by pixel.
add_test(NAME app_image COMMAND app_tests image "${CMAKE_BINARY_DIR}/app_output/image")
# Border crop (recognition by shape, crops pixel by pixel, on screen by CRC).
add_test(NAME app_crop COMMAND app_tests crop "${CMAKE_BINARY_DIR}/app_output/crop")
# Smooth motion on a fake frame (redraw, sub-pixel positions, constant speed).
add_test(NAME app_redraw COMMAND app_tests redraw)
add_test(NAME app_fade COMMAND app_tests fade)
# Start menu (background, covers, selection, texts, sounds and the sound effects file).
add_test(NAME app_launcher COMMAND app_tests launcher "${CMAKE_BINARY_DIR}/app_output/launcher")
# Controls (keyboard and SDL virtual gamepad, rebinding through the menu, configuration).
add_test(NAME app_input COMMAND app_tests input "${CMAKE_BINARY_DIR}/app_output/input")
add_test(NAME app_disc_id COMMAND app_tests disc "${YS_USA_CUE}"
    "${CMAKE_BINARY_DIR}/app_output/disco")
set_tests_properties(app_disc_id PROPERTIES SKIP_RETURN_CODE 77)
# CD music with the options menu open (console paused) and then resuming where it stopped.
add_test(NAME app_cd_menu_music COMMAND app_tests cdmenu "${YS_USA_CUE}")
set_tests_properties(app_cd_menu_music PROPERTIES SKIP_RETURN_CODE 77)
# Where the program reads its resources and writes the user's data (host layer; the OS policy comes from
# cmake/platform_*.cmake: next to the executable on Windows, Application Support on macOS).
add_test(NAME host_paths COMMAND app_tests paths ${PLATFORM_USER_DIR})


find_program(PYTHON_EXE NAMES python python3)
# The recompiler and the tools read TOML with tomllib (Python 3.11 or later)
if(PYTHON_EXE)
    execute_process(COMMAND "${PYTHON_EXE}" -c "import sys; print('%d.%d' % sys.version_info[:2])"
        OUTPUT_VARIABLE YS_PYTHON_VERSION RESULT_VARIABLE YS_PYTHON_RC OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT YS_PYTHON_RC EQUAL 0 OR YS_PYTHON_VERSION VERSION_LESS 3.11)
        message(FATAL_ERROR "Python 3.11 or later needed (tomllib): ${PYTHON_EXE} gave '${YS_PYTHON_VERSION}'. "
            "Choose another one with -DPYTHON_EXE=...")
    endif()
    # Dependency rules between the layers: common code without OS #ifdefs, host without the game, PCERecomp
    # without the game (review_portability/05_host_layer_design.md, chapter 7)
    add_test(NAME host_layers COMMAND ${PYTHON_EXE} "${CMAKE_SOURCE_DIR}/tools/check_layers.py"
        --pcerecomp "${PCERECOMP_DIR}")
    set_tests_properties(host_layers PROPERTIES PASS_REGULAR_EXPRESSION "check_layers: OK")
endif()



# Everything that belongs to one edition of the game (disc map, generated C, YsRecomp, ys_trace and the tests on
# its disc): cmake/ys_edition.cmake. Only the USA edition until Phase 3B (review_jp/02_build_layout.md, chapter 2).
ys_edition(ID usa CUE "${YS_USA_CUE}" BIOS "${YS_USA_BIOS}" GAME_DIR "${CMAKE_SOURCE_DIR}/games/ys")

# ---- Test groups (LABELS) ----
# quick:    ~20 s; run on every change (.\build.ps1 -Test)
# console:  simulated console (CPU, video, sound, CD, BIOS in C) against Geargrafx/the real BIOS
# recomp:   recompiled code against the interpreter (lockstep), performance and window options in the game
# package:  the player package (.\build.ps1 -Package)
# history:  old bugs checked in long sessions (smooth motion, crop, sound); outside
#           -TestAll, run with .\build.ps1 -TestHist when touching those parts and before large milestones
# The full suite (.\build.ps1 -TestAll, everything except history) runs in parallel before committing milestones that
# touch the console or the recompiler. Tests of the same session run together in a single run.
set(TESTS_QUICK app_config app_volume app_menu app_video app_image app_crop
    app_redraw app_fade app_input app_disc_id app_cd_menu_music app_start_ys nobios_ys app_launcher equip_c_ys
    app_mark_ys host_layers host_paths)
set(TESTS_CONSOLE trace_ys_boot trace_ys_run session_ys_boot session_ys_game session_ys_save fade_ys
    coverage_ys bios_c_ys hle_timing_ys nobios_cabin_ys equip_preview_ys equip_ice_preview_ys game_windows_ys windows_ys1 windows_crop_ys
    panel_ys)
set(TESTS_RECOMP recomp_disc_ys recomp_lockstep_ys recomp_lockstep_ys_nobios dialog_ys app_menu_ys app_intro_ys app_menu_intro_ys perf_ys torn_window_ys save_window_ring_ys hud_save_ys hud_save_ys2
    app_crop_ys app_torn_window_ys app_load_no_border_ys layout_switches_ys app_rhythm_sewer_ys rhythm_cave_ys equipment_ys actors_ys actors_ys1 motion_fast_ys
    motion_cave_fast_ys gpu_roof_ys motion_edge_ys gpu_behind_roof_ys masks_mine_ys masks_window_ys light_window_ys zoom_save_ys zoom_gameover_ys app_ring_list_ys
    equip_ys1 equip_ys2 battle_ys2 equip_blue_monster_ys equip_yellow_monster_ys equip_monster_back_ys equip_list_ys
    equip_fade_bios_ys equip_ice_ys
    app_menu_image_ys app_menu_chip_ys app_hiccup_ys app_stall_ys app_stall_device_ys scroll_ys scroll_back_ys
    app_prep_ys)
set(TESTS_HISTORY cd_pop_ys motion_save_ys crop_flash_ys item_window_ys motion_intro2_ys
    motion_cave_ys motion_light_ys motion_ys motion_temple_ys gpu_window_ys)
set(TESTS_PACKAGE app_package_ys app_launcher_ys)
# No test shows on screen or plays sound: the window opens hidden (PCE_HIDDEN_WINDOW, runtime/main.c: no
# fullscreen, no focus, no V-Sync) and the sound goes to SDL's dummy audio device (it consumes at the real pace:
# the sound queues of the real-time tests still count). Tests that measure real time or count what the window
# drew run alone (RUN_SERIAL): other heavy tests alongside would change the pacing.
set(TESTS_TEMPO_REAL app_stall_ys app_stall_device_ys app_hiccup_ys app_torn_window_ys app_load_no_border_ys app_rhythm_sewer_ys app_ring_list_ys
    app_menu_chip_ys app_mark_ys app_intro_ys app_menu_intro_ys app_crop_ys app_prep_ys)
foreach(group QUICK CONSOLE RECOMP HISTORY PACKAGE)
    string(TOLOWER ${group} label)
    foreach(t ${TESTS_${group}})
        if(TEST ${t})
            set_property(TEST ${t} APPEND PROPERTY LABELS ${label})  # next to the edition label (ys_edition)
        endif()
    endforeach()
endforeach()
get_property(ALL_TESTS DIRECTORY PROPERTY TESTS)
foreach(t ${ALL_TESTS})
    set_tests_properties(${t} PROPERTIES ENVIRONMENT "PCE_HIDDEN_WINDOW=1;SDL_AUDIO_DRIVER=dummy")
endforeach()
if(TEST battle_ys2)
    set_property(TEST battle_ys2 APPEND PROPERTY ENVIRONMENT "PCE_SMOOTH_STEPS=1")
endif()
foreach(t ${TESTS_TEMPO_REAL})
    if(TEST ${t})
        set_tests_properties(${t} PROPERTIES RUN_SERIAL TRUE)
    endif()
endforeach()
