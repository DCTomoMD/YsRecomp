"""Where PCERecomp is (the emulated console and the generic tools), for the YsRecomp scripts.

Order (same as CMakeLists.txt): the PCERECOMP_DIR variable; the working copy next to this folder (../PCERecomp);
the submodule external/PCERecomp.
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import build_paths  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def find():
    env = os.environ.get("PCERECOMP_DIR")
    if env:
        return os.path.abspath(env)
    side = os.path.abspath(os.path.join(ROOT, "..", "PCERecomp"))
    if os.path.isfile(os.path.join(side, "CMakeLists.txt")):
        return side
    return os.path.join(ROOT, "external", "PCERecomp")


PCERECOMP = find()
TRACE_TOOLS = os.path.join(PCERECOMP, "tools", "trace")
# Geargrafx reference (built by PCERecomp's tools/reference_trace): gg_trace.exe on Windows, gg_trace elsewhere
GG_TRACE = os.path.join(PCERECOMP, "tools", "reference_trace", "build", "Release", build_paths.exe_name("gg_trace"))
