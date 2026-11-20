# macOS (Apple Clang, Apple Silicon): compiler flags and host layer sources.
# Included by CMakeLists.txt right after project(), before PCERecomp and SDL3 (the flags apply to them too).
# M3: the generated C (M3-1), the .app with Info.plist, icon and signature (M3-5).

# arm64 only (decision D9)
if(NOT CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES arm64 CACHE STRING "macOS architectures" FORCE)
endif()
# Oldest macOS the programs run on (H5): 13 Ventura. Without it Apple Clang targets the macOS of the build machine
# (the .app would not open on an older one). SDL3 3.2.30 itself goes back to 10.13 (its docs/README-macos.md).
if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET 13.0 CACHE STRING "Oldest macOS the programs run on" FORCE)
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

# SDL3 (built from its sources): its Cocoa and controller code still calls APIs Apple marked deprecated for macOS 13
# (22 -Wdeprecated-declarations warnings, M3-5). They are SDL's, not ours, and cannot be fixed here: silenced for the
# SDL3 target only (Q12). Our own code keeps every warning.
function(platform_sdl target)
    target_compile_options(${target} PRIVATE -Wno-deprecated-declarations)
endfunction()

# The player's executable as a .app (YsRecomp.app/Contents/MacOS/YsRecomp; $<TARGET_FILE> is that file, so the tests
# run it as before):
# - Info.plist from runtime/host/mac/Info.plist.in, with the name and version of the Windows .rc;
# - the icon: tools/package/make_icon.py --iconset (the same art as the .ico, nothing from the disc) and the system's
#   iconutil, in the build folder (not versioned);
# - the launcher images in Contents/Resources/launcher/: SDL_GetBasePath() is Contents/Resources/ inside a .app, so
#   the .app finds them through host_resource_dir() and does not depend on PCE_DOCS_DIR (the source folder, R23);
# - an ad-hoc signature over the whole bundle (decision D12: no Developer ID, no notarization).
set(YS_MACOS_BUNDLE_ID "local.ysrecomp.YsRecomp" CACHE STRING "CFBundleIdentifier of YsRecomp.app")
function(platform_app_executable target)
    set(work "${CMAKE_BINARY_DIR}/macos")
    set(icns "${work}/${target}.icns")
    add_custom_command(OUTPUT "${icns}"
        COMMAND "${CMAKE_COMMAND}" -E rm -rf "${work}/${target}.iconset"
        COMMAND "${PYTHON_EXE}" "${CMAKE_SOURCE_DIR}/tools/package/make_icon.py" --iconset "${work}/${target}.iconset"
        COMMAND iconutil -c icns -o "${icns}" "${work}/${target}.iconset"
        DEPENDS "${CMAKE_SOURCE_DIR}/tools/package/make_icon.py"
        COMMENT "Icon ${target}.icns"
        VERBATIM)
    target_sources(${target} PRIVATE "${icns}")
    set_source_files_properties("${icns}" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    # The names main.c looks for (k_launcher_images)
    foreach(image "Ys Logo.jpg" "YS I II.jpg" "Ys III.jpg" "Ys IV.jpg")
        target_sources(${target} PRIVATE "${CMAKE_SOURCE_DIR}/docs/${image}")
        set_source_files_properties("${CMAKE_SOURCE_DIR}/docs/${image}" PROPERTIES MACOSX_PACKAGE_LOCATION Resources/launcher)
    endforeach()
    set_target_properties(${target} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/runtime/host/mac/Info.plist.in"
        MACOSX_BUNDLE_BUNDLE_NAME "${target}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "${YS_MACOS_BUNDLE_ID}"
        MACOSX_BUNDLE_ICON_FILE "${target}.icns"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0.0"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0")
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND codesign --force --sign - --timestamp=none "$<TARGET_BUNDLE_DIR:${target}>"
        COMMENT "Ad-hoc signature of ${target}.app"
        VERBATIM)
endfunction()
