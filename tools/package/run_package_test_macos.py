"""Test app_package_ys on macOS: the player package runs from a clean folder (the counterpart of run_package_test.py).

Builds the package (make_package.py) and checks:
  - the folder holds only YsRecomp.app and README.txt, and the .zip only YsRecomp/YsRecomp.app/... and
    YsRecomp/README.txt (no resource forks, no extended attributes, nothing from reference/ or the disc);
  - the app's signature is valid, its Info.plist names the executable, and the executable can be run;
  - building again makes the folder anew (nothing left over from before).
Then extracts the .zip (ditto, as Finder does) into a temporary folder outside the project and starts the app as the
player does, with no game arguments. The player's settings and saves normally go to ~/Library/Application Support/
YsRecomp/; the test points them to a temporary folder instead (PCE_USER_DIR, runtime/host/host_paths.h), so the real
ones are never touched. In it, a YsRecomp.ini with only the disc folder (the folder picker window would block the test):
  - the disc is recognized and the recompiled code runs 600 frames;
  - the save goes to saves/ in that folder (2 KB); with record_sessions=1, the session goes to sessions/;
  - nothing is written next to the app or inside it (its signature is still valid afterwards).

Usage: run_package_test_macos.py YsRecomp.app/Contents/MacOS/YsRecomp DISC_FOLDER WORK_FOLDER
Without the disc in the folder: checks only the package and exits with 77 (skipped).
"""
import glob
import os
import plistlib
import shutil
import subprocess
import sys
import tempfile
import zipfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import make_package  # noqa: E402

EXE = make_package.EXE  # YsRecomp
FILES = sorted([f"{EXE}.app", "README.txt"])
FORBIDDEN = ("reference", "Geargrafx", "huc6280-tests", "mednafen", ".cue", ".bin", ".pce", ".brm", ".ini",
             "menu_font.bin", "launcher_sfx.bin")


def signature_ok(app):
    return subprocess.run(["codesign", "--verify", "--strict", app], capture_output=True).returncode == 0


def check_package(folder, zip_path):
    """Problems with the built package (empty = ok)."""
    problems = []
    if sorted(os.listdir(folder)) != FILES:
        problems.append(f"the package folder has {sorted(os.listdir(folder))}, expected {FILES}")
    app = os.path.join(folder, f"{EXE}.app")
    if not signature_ok(app):
        problems.append("the app's signature is not valid")
    with open(os.path.join(app, "Contents", "Info.plist"), "rb") as f:
        info = plistlib.load(f)
    exe = os.path.join(app, "Contents", "MacOS", info.get("CFBundleExecutable", ""))
    if info.get("CFBundleExecutable") != EXE or not os.access(exe, os.X_OK):
        problems.append(f"Info.plist names {info.get('CFBundleExecutable')!r}, not the executable {EXE}")
    with zipfile.ZipFile(zip_path) as z:
        names = z.namelist()
    tops = sorted({"/".join(n.split("/")[:2]) for n in names if n.count("/") >= 1 and n.split("/")[1]})
    if tops != sorted(f"{EXE}/{n}" for n in FILES):
        problems.append(f"the zip holds {tops}")
    bad = [n for n in names if "__MACOSX" in n or "/._" in n or any(w.lower() in n.lower() for w in FORBIDDEN)]
    if bad:
        problems.append(f"the zip holds files it must not: {bad[:5]}")
    return problems


def main():
    exe, game_dir, work = sys.argv[1:4]
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    r = make_package.make(exe, work)
    if not r:
        print("FAILED: the package was not built")
        return 1
    folder, zip_path = r
    problems = check_package(folder, zip_path)
    if problems:
        for p in problems:
            print(f"FAILED: {p}")
        return 1
    print(f"OK: package with {FILES} ({os.path.getsize(zip_path) // 1024} KB zipped), signature valid")
    # Build again on top of a folder with something else in it: made anew (the player's data is never there)
    open(os.path.join(folder, "stale.txt"), "w").close()
    make_package.make(exe, work)
    if sorted(os.listdir(folder)) != FILES:
        print(f"FAILED: after building again, the folder has {sorted(os.listdir(folder))}")
        return 1
    print("OK: building again makes the folder anew")

    if not glob.glob(os.path.join(game_dir, "*.cue")):
        print(f"No .cue in {game_dir}: running-package test skipped.")
        return 77
    clean = tempfile.mkdtemp(prefix="ysrecomp_package_")
    try:
        subprocess.run(["ditto", "-x", "-k", zip_path, clean], check=True)
        game = os.path.join(clean, EXE)
        app = os.path.join(game, f"{EXE}.app")
        before = sorted(os.listdir(game))
        user = os.path.join(clean, "user")  # stands for ~/Library/Application Support/YsRecomp/
        os.makedirs(user)
        with open(os.path.join(user, f"{EXE}.ini"), "w", encoding="utf-8") as f:
            f.write(f"fullscreen=0\nrecord_sessions=1\ngame_folder={os.path.abspath(game_dir)}\n")
        env = {**os.environ, "PCE_USER_DIR": user}
        r = subprocess.run([os.path.join(app, "Contents", "MacOS", EXE), "--frames", "600", "--turbo"], cwd=clean,
                           env=env, capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=300)
        out = r.stdout + r.stderr
        ok = True
        if r.returncode != 0:
            print(out[-3000:])
            print(f"FAILED: exited with code {r.returncode}")
            ok = False
        for line in ("Running the recompiled code", "Window closed after 600 frames"):
            if line not in out:
                print(f"FAILED: '{line}' missing from the output")
                ok = False
        saves = glob.glob(os.path.join(user, "saves", "*.brm"))
        if len(saves) != 1 or os.path.getsize(saves[0]) != 2048:
            print(f"FAILED: save expected in saves/ of the user folder (2048 bytes); found: {saves}")
            ok = False
        rec = sorted(os.listdir(os.path.join(user, "sessions"))) if os.path.isdir(os.path.join(user, "sessions")) else []
        txt = [n for n in rec if n.endswith(".txt")]
        if len(txt) != 1 or len(rec) != 1:
            print(f"FAILED: expected 1 session in sessions/ of the user folder; found: {rec}")
            ok = False
        else:
            with open(os.path.join(user, "sessions", txt[0]), encoding="utf-8") as f:
                if "# frames: 600\n" not in f.read():
                    print("FAILED: the recorded session does not have '# frames: 600'")
                    ok = False
        if sorted(os.listdir(game)) != before:
            print(f"FAILED: files appeared next to the app: {sorted(set(os.listdir(game)) - set(before))}")
            ok = False
        if not signature_ok(app):
            print("FAILED: the app's signature is no longer valid after running (something was written inside it)")
            ok = False
        print("OK: the package ran from a clean folder and reached the game; the user data went to PCE_USER_DIR."
              if ok else "FAILED")
        return 0 if ok else 1
    finally:
        shutil.rmtree(clean, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
