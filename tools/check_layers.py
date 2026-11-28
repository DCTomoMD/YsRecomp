"""Checks the dependency rules between the layers (review_portability/05_host_layer_design.md, chapter 7).

  1. common code (runtime/main.c, runtime/app/, runtime/profile/): no OS headers (windows, dwmapi, d3d11, dxgi,
     Cocoa, Metal) and no _WIN32 / __APPLE__: the OS is chosen by CMake (cmake/platform_*.cmake), not by #ifdef;
  2. host layer (runtime/host/): knows nothing of the game or the edition (no app/, profile/, ys_, g_ys_profile);
  3. each OS implementation (runtime/host/<os>/) includes nothing from another OS's folder;
  4. host layer: does not include the emulated console's headers (pce/, cpu/, cd/, bios/);
  5. PCERecomp (--pcerecomp FOLDER): nothing about this game nor about the host and profile layers
     (its README may name the game, as the project that uses it);
  6. profile layer (runtime/profile/): plain data, includes nothing from app/ or host/ and no SDL
     (review_jp/01_profile_design.md, chapter 9);
  7. common code and the trace extension (runtime/main.c, runtime/app/, tools/trace/): no branch on the edition
     (no edition_id comparison, no PCE_EDITION): the edition is chosen by linking one profile, not by code;
  8. the profile itself (g_ys_profile) is defined only in runtime/profile/profile_<edition>.c, exactly once in each
     (review_jp/04_phase3b_plan.md, 3B-2): runtime/, tools/ and tests/ define it nowhere else.

Usage: python tools/check_layers.py [--pcerecomp FOLDER]
Exit: 0 = all rules hold, 1 = some rule broken (each case listed), 2 = usage error.
"""
import argparse
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SOURCE_EXT = (".c", ".h", ".m", ".mm")
HOST_OS_DIRS = ("win", "mac", "null")

COMMON_OS = re.compile(r"#\s*include\s*<(windows|dwmapi|d3d11|dxgi)\.h>|#\s*include\s*<(Cocoa|Metal)/|\b_WIN32\b|\b__APPLE__\b")
HOST_GAME = re.compile(r"\bapp/|\bprofile/|\bys_|\bg_ys_profile\b")
HOST_CONSOLE = re.compile(r"#\s*include\s*\"(pce|cpu|cd|bios)/")
PCERECOMP_GAME = re.compile(r"\bYs\b|\bys_|\bapp/|\bAdol\b|\bhost/|\bprofile/")
PCERECOMP_EXT = SOURCE_EXT + (".py", ".txt", ".md", ".cmake", ".toml")
PCERECOMP_SKIP_DIRS = {"reference", "build", ".git", "__pycache__"}
PROFILE_DEPS = re.compile(r"#\s*include\s*[<\"](SDL|app/|host/)")
EDITION_BRANCH = re.compile(r"\bedition_id\s*[=!]=|strcmp\s*\([^)]*edition|\bPCE_EDITION")
PROFILE_DEFINITION = re.compile(r"^\s*(const\s+)?YsProfile\s+(const\s+)?g_ys_profile\s*(=|;|\[)")
PROFILE_FILE = re.compile(r"profile_[a-z0-9]+\.c")


def source_files(top, exts=SOURCE_EXT, skip_dirs=()):
    if os.path.isfile(top):
        yield top
        return
    for d, dirs, files in os.walk(top):
        dirs[:] = sorted(x for x in dirs if x not in skip_dirs and not x.startswith("."))
        for f in sorted(files):
            if f.endswith(exts) and not f.startswith("."):  # no macOS ._* files
                yield os.path.join(d, f)


def scan(path, pattern, problems, rule):
    with open(path, encoding="utf-8", errors="replace") as f:
        for n, line in enumerate(f, 1):
            if pattern.search(line):
                problems.append(f"{rule}: {os.path.relpath(path, ROOT)}:{n}: {line.strip()}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--pcerecomp", help="PCERecomp folder (rule 5); without it, rule 5 is not checked")
    a = ap.parse_args()
    problems = []
    runtime = os.path.join(ROOT, "runtime")
    host = os.path.join(runtime, "host")
    if not os.path.isdir(host):
        print(f"check_layers: FAILED: {host} does not exist")
        return 1

    for top in (os.path.join(runtime, "main.c"), os.path.join(runtime, "app"), os.path.join(runtime, "profile")):
        if os.path.exists(top):
            for p in source_files(top):
                scan(p, COMMON_OS, problems, "1 common->OS")
    profile = os.path.join(runtime, "profile")
    if os.path.isdir(profile):
        for p in source_files(profile):
            scan(p, PROFILE_DEPS, problems, "6 profile->app/host/SDL")
    for top in (os.path.join(runtime, "main.c"), os.path.join(runtime, "app"), os.path.join(ROOT, "tools", "trace")):
        for p in source_files(top):
            scan(p, EDITION_BRANCH, problems, "7 edition branch")
    profile_files = set()
    for top in (runtime, os.path.join(ROOT, "tools"), os.path.join(ROOT, "tests")):
        if not os.path.isdir(top):
            continue
        for p in source_files(top):
            with open(p, encoding="utf-8", errors="replace") as f:
                lines = [n for n, line in enumerate(f, 1) if PROFILE_DEFINITION.search(line)]
            rel = os.path.relpath(p, ROOT)
            own = os.path.dirname(os.path.abspath(p)) == os.path.abspath(profile) and PROFILE_FILE.fullmatch(
                os.path.basename(p))
            if own:
                profile_files.add(rel)
                if len(lines) != 1:
                    problems.append(f"8 profile definition: {rel}: g_ys_profile defined {len(lines)} time(s), "
                                    "expected 1")
            else:
                for n in lines:
                    problems.append(f"8 profile definition: {rel}:{n}: g_ys_profile defined outside "
                                    "runtime/profile/profile_<edition>.c")
    if not profile_files:
        problems.append("8 profile definition: no runtime/profile/profile_<edition>.c found")
    for p in source_files(host):
        scan(p, HOST_GAME, problems, "2 host->game")
        scan(p, HOST_CONSOLE, problems, "4 host->console")
    for os_dir in HOST_OS_DIRS:
        others = "|".join(x for x in HOST_OS_DIRS if x != os_dir)
        pattern = re.compile(r"#\s*include\s*\"(host/)?(" + others + r")/")
        top = os.path.join(host, os_dir)
        if os.path.isdir(top):
            for p in source_files(top):
                scan(p, pattern, problems, f"3 host/{os_dir}->other OS")
    if a.pcerecomp:
        if not os.path.isfile(os.path.join(a.pcerecomp, "CMakeLists.txt")):
            print(f"check_layers: FAILED: PCERecomp not found at {a.pcerecomp}")
            return 1
        for p in source_files(a.pcerecomp, PCERECOMP_EXT, PCERECOMP_SKIP_DIRS):
            if os.path.basename(p) == "README.md" and os.path.dirname(os.path.abspath(p)) == os.path.abspath(a.pcerecomp):
                continue
            scan(p, PCERECOMP_GAME, problems, "5 PCERecomp->game/host/profile")

    for line in problems:
        print(line)
    print(f"check_layers: {'OK' if not problems else f'{len(problems)} problem(s)'}")
    return 0 if not problems else 1


if __name__ == "__main__":
    sys.exit(main())
