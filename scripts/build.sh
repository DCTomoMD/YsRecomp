#!/usr/bin/env bash
# Builds the project (Release) in build/ on macOS: the counterpart of build.ps1 (Windows), same steps and options.
# Usage: scripts/build.sh              -> build
#        scripts/build.sh --test       -> build and run the quick tests ("quick" group)
#        scripts/build.sh --test-all   -> build and run every test except the "history" group, several at a time
#        scripts/build.sh --test-hist  -> build and run the "history" group
#        scripts/build.sh --package    -> build, test the package ("package" group) and assemble the player package
#                                         in dist/ (YsRecomp/ with YsRecomp.app + README.txt, and YsRecomp.zip)
#        --verbose                     -> show everything (full build and test output)
#        --target NAME                 -> build only that target (repeatable; e.g. --target app_tests)
#        --edition usa|jp|all          -> edition of the game (default all: every edition whose disc and BIOS were
#                                         found, today only usa); the other editions' tests are left out (common
#                                         tests always run). jp: Phase 3B. As build.ps1 -Edition
# Without --verbose, only build warnings and errors and the test summary are shown. To see a failed test's output:
#   ctest --test-dir build -C Release -R <name> --output-on-failure
# Generator: Ninja Multi-Config, so the executables go to build/Release/ as on Windows (tools/build_paths.py).
# Python: .venv/bin/python when it exists (3.11 or later: CMakeLists.txt checks it), otherwise the one in PATH.
# Disc and BIOS: -DYS_USA_CUE=... -DYS_USA_BIOS=... at the first configuration (or cmake -S . -B build -D...), otherwise
# roms/<upstream name>, then roms/usa/ and roms/bios/ (cmake/ys_edition.cmake).
set -euo pipefail

TEST=""; PACKAGE=0; VERBOSE=0; TARGETS=(); EDITION=all
while [ $# -gt 0 ]; do
    case "$1" in
        --test) TEST=quick ;;
        --test-all) TEST=all ;;
        --test-hist) TEST=history ;;
        --package) PACKAGE=1 ;;
        --verbose) VERBOSE=1 ;;
        --target) shift; TARGETS+=("${1:?--target needs a name}") ;;
        --edition) shift; EDITION="${1:?--edition needs usa, jp or all}" ;;
        -h|--help) sed -n '2,19p' "$0"; exit 0 ;;
        *) echo "Unknown option: $1 (--help)" >&2; exit 2 ;;
    esac
    shift
done

# Editions (tools/editions.py; the ctest label of each one's tests). jp comes in Phase 3B.
AVAILABLE="usa"; PLANNED="jp"
case "$EDITION" in
    all) EDITIONS="$AVAILABLE" ;;
    usa) EDITIONS="$EDITION" ;;
    jp) echo "Edition 'jp' is not available yet (Phase 3B)" >&2; exit 2 ;;
    *) echo "Unknown edition: $EDITION (usa, jp or all)" >&2; exit 2 ;;
esac
EXCLUDED=""
for e in $AVAILABLE $PLANNED; do
    case " $EDITIONS " in *" $e "*) ;; *) EXCLUDED="${EXCLUDED:+$EXCLUDED|}$e" ;; esac
done

cd "$(dirname "$0")/.."
command -v cmake >/dev/null || { echo "cmake not found in PATH" >&2; exit 1; }
command -v ninja >/dev/null || { echo "ninja not found in PATH (Ninja Multi-Config generator)" >&2; exit 1; }

# Runs a program; without --verbose, shows only the lines matching the filter (first argument). Returns its exit code.
run_filtered() {
    local filter="$1"; shift
    if [ "$VERBOSE" = 1 ]; then
        "$@"
        return
    fi
    local log rc=0
    log="$(mktemp -t ysrecomp_build)"
    "$@" >"$log" 2>&1 || rc=$?
    grep -E -i "$filter" "$log" || true
    rm -f "$log"
    return $rc
}

if [ ! -f build/CMakeCache.txt ]; then
    args=(-S . -B build -G "Ninja Multi-Config")
    if [ -x .venv/bin/python ]; then
        args+=(-DPYTHON_EXE="$PWD/.venv/bin/python")
    fi
    run_filtered 'error|warning' cmake "${args[@]}" || { echo "CMake configuration failed" >&2; exit 1; }
fi
bargs=(--build build --config Release --parallel)
for t in "${TARGETS[@]+"${TARGETS[@]}"}"; do
    bargs+=(--target "$t")
done
run_filtered 'warning:|error|fatal' cmake "${bargs[@]}" || { echo "Build failed" >&2; exit 1; }
echo "Built."

# Tests in parallel: half the cores (each test runs up to two programs at the same time).
cores="$(sysctl -n hw.ncpu 2>/dev/null || echo 2)"
jobs=$(( cores / 2 )); [ "$jobs" -ge 1 ] || jobs=1
summary='tests passed|\*\*\*Failed|\*\*\*Exception|\*\*\*Timeout|Not Run|^ +[0-9]+ - '
if [ -n "$TEST" ]; then
    # Whatever fails runs once more (real-time tests can lose pacing when the computer is busy; a real defect
    # fails both times)
    targs=(--test-dir build -C Release -j "$jobs" --repeat until-pass:2)
    [ "$VERBOSE" = 1 ] && targs+=(--output-on-failure)
    # (-L twice would mean "both labels" and drop the common tests: the other editions are excluded instead)
    left_out="$EXCLUDED"
    case "$TEST" in
        history) targs+=(-L history) ;;
        all) left_out="history${left_out:+|$left_out}" ;;
        *) targs+=(-L quick) ;;
    esac
    [ -n "$left_out" ] && targs+=(-LE "^($left_out)\$")
    run_filtered "$summary" ctest "${targs[@]}" || { echo "Tests failed" >&2; exit 1; }
fi
if [ "$PACKAGE" = 1 ]; then
    # The package test ("package" group), unless --test-all already ran it; then one package per edition
    if [ "$TEST" != all ]; then
        pargs=(--test-dir build -C Release -L package --repeat until-pass:2)
        [ -n "$EXCLUDED" ] && pargs+=(-LE "^($EXCLUDED)\$")
        [ "$VERBOSE" = 1 ] && pargs+=(--output-on-failure)
        run_filtered "$summary" ctest "${pargs[@]}" || { echo "Package test failed" >&2; exit 1; }
    fi
    py=python3; [ -x .venv/bin/python ] && py=.venv/bin/python
    for ed in $EDITIONS; do
        "$py" tools/package/make_package.py --edition "$ed" build/Release/YsRecomp.app/Contents/MacOS/YsRecomp dist ||
            { echo "Package assembly failed" >&2; exit 1; }
    done
fi
