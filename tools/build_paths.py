"""Where the build puts this project's executables, on every OS.

The executables go to build/Release/ (CMakeLists.txt: CMAKE_RUNTIME_OUTPUT_DIRECTORY with a multi-configuration
generator: Visual Studio on Windows, Ninja Multi-Config on macOS through scripts/build.sh). Only the name differs:
NAME.exe on Windows, NAME elsewhere.

    import build_paths
    build_paths.exe("ys_trace")      -> <root>/build/Release/ys_trace(.exe)
"""
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUILD_DIR = os.path.join(ROOT, "build", "Release")
EXE_SUFFIX = ".exe" if sys.platform == "win32" else ""


def exe_name(name):
    """File name of the executable NAME on this OS."""
    return name + EXE_SUFFIX


def exe(name):
    """Path of the executable NAME built by this project."""
    return os.path.join(BUILD_DIR, exe_name(name))
