"""Checks every recorded session not yet checked (acceptance).

For each session_*.txt in the folders (default: games/ys/sessions and dist/YsRecomp/sessions, where the player's
package records with record_sessions=1):
  1. Lockstep (ys_trace --lockstep): the recompiled code runs the whole session together with the interpreter and
     everything is compared at every instruction and frame. Also notes how many instructions fell into the
     interpreter (code the coverage has not seen yet or that the game rewrote).
  2. If the session was recorded with the real BIOS (header "# bios:" other than "-"): check_session.py, the
     interpreter against Geargrafx in image and sound of every frame. Without the BIOS (the player's case)
     Geargrafx is no reference: it needs the BIOS, and the C routines have slightly different timing
     (docs/ROADMAP.md); the lockstep holds, plus the C routines checked by run_bios_check.py.

Each session's result goes to <session>.conferencia (next to it) and an equal session is not checked again while
its number of frames does not change (--force checks everything; different ones are always checked again). With
YsRecomp open, sessions modified less than 3 minutes ago are skipped: the game may still be recording (the
recording is rewritten every minute).

Usage: python tools/trace/check_all_sessions.py [FOLDER ...] [--force] [--jobs N] (N sessions at the same time)
                                               [--edition usa] [--cue <disc>] [--bios <system card>]
       (--edition picks the trace tool, the default folders and the game's executable name; the headers' disc and
       BIOS are resolved on this machine by tools/editions.py, --cue/--bios replace them)
Exit: 0 = all equal, 1 = some different or with an error.
"""
import argparse
import concurrent.futures
import glob
import os
import re
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.dirname(HERE))
import build_paths  # noqa: E402
import editions  # noqa: E402
WORK = os.path.join(ROOT, "build", "sessions")
RECENT_S = 180


def folders(ed):
    """The edition's sessions and the ones the player's package records (record_sessions=1)."""
    return (ed.sessions_dir, os.path.join(ROOT, "dist", ed.exe, "sessions"))


def game_running(ed):
    """Is the game open (Windows: tasklist; elsewhere: pgrep)? When it cannot tell, says yes."""
    name = build_paths.exe_name(ed.exe)
    try:
        if sys.platform == "win32":
            out = subprocess.run(["tasklist", "/FI", f"IMAGENAME eq {name}", "/NH"], capture_output=True,
                                 text=True, timeout=10).stdout
            return name in out
        r = subprocess.run(["pgrep", "-x", name], capture_output=True, text=True, timeout=10)
    except (OSError, subprocess.SubprocessError):
        return True
    if r.returncode > 1:  # 0 = found, 1 = none, more = pgrep error
        return True
    return r.returncode == 0


sys.path.insert(0, HERE)
from check_session import read_header  # noqa: E402


def lockstep(session, info, frames, ed, a):
    """Returns (ok, result text)."""
    name = os.path.splitext(os.path.basename(session))[0]
    work = os.path.join(WORK, name)
    os.makedirs(work, exist_ok=True)
    # Header paths relative to the project root (sessions recorded with --record games\...), resolved on this
    # machine (tools/editions.py)
    cmd = [build_paths.exe(ed.trace), "--frame", "--lockstep", "--input", os.path.abspath(session)]
    if info.get("bios", "-") != "-":
        cmd += ["--bios", editions.header_path(info["bios"], ed, a.bios)]
    if info.get("save"):
        save = editions.normalize(info["save"])
        if not os.path.exists(save):  # copy whose header has only the save name: it sits next to the session
            save = os.path.join(os.path.dirname(os.path.abspath(session)), info["save"])
        cmd += ["--bram", save]
    cmd += [editions.header_path(info["game"], ed, a.cue), str(frames), os.path.join(work, "lockstep.ppm")]
    t = time.time()
    r = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8", errors="replace", cwd=work)
    out = r.stdout + r.stderr
    secs = time.time() - t
    ok_line = re.search(r"LOCKSTEP OK: (\d+) instructions and (\d+) frames", out)
    interp = re.search(r"interpreter: (\d+) instructions", out)
    if r.returncode != 0 or not ok_line:
        tail = "\n".join(out.strip().splitlines()[-15:])
        return False, f"lockstep DIFFERENT or error (code {r.returncode}, {secs:.0f} s):\n{tail}"
    total = int(ok_line.group(1))
    n_interp = int(interp.group(1)) if interp else 0
    pct = 100.0 * n_interp / total if total else 0.0
    # With the real BIOS, its code (no generated C for it) counts as interpreted
    note = " (includes the real BIOS)" if info.get("bios", "-") != "-" else ""
    return True, (f"lockstep OK: {total} instructions and {ok_line.group(2)} frames identical to the interpreter "
                  f"({secs:.0f} s); in the interpreter: {n_interp} instructions ({pct:.2f}%){note}")


def geargrafx(session, ed, a):
    cmd = [sys.executable, os.path.join(HERE, "check_session.py"), os.path.abspath(session), "--edition", ed.id]
    if a.cue:
        cmd += ["--cue", a.cue]
    if a.bios:
        cmd += ["--bios", a.bios]
    r = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8", errors="replace", cwd=ROOT)
    lines = (r.stdout + r.stderr).strip().splitlines()
    if r.returncode == 0:
        lines = [l for l in lines if l.startswith("EQUAL")] or lines[-1:]
    return r.returncode == 0, "Geargrafx: " + "\n".join(lines[-6:])


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("folders", nargs="*")
    ap.add_argument("--force", action="store_true")
    ap.add_argument("--jobs", type=int, default=1, help="sessions checked at the same time")
    editions.add_argument(ap)
    ap.add_argument("--cue", help="the disc to use instead of the headers'")
    ap.add_argument("--bios", help="the System Card to use instead of the headers' (sessions with one)")
    a = ap.parse_args()
    ed = editions.get(a.edition)
    trace = build_paths.exe(ed.trace)
    if not os.path.exists(trace):
        print(f"{trace} not found: build first (build.ps1)")
        return 1
    sessions, names = [], set()
    for folder in a.folders or folders(ed):
        for s in sorted(glob.glob(os.path.join(folder, "session_*.txt"))):
            if os.path.basename(s) not in names:  # the same session copied to games/ys/sessions
                names.add(os.path.basename(s))
                sessions.append(s)
    if not sessions:
        print("No session found.")
        return 0
    running = game_running(ed)
    todo = []
    for s in sessions:
        info = read_header(s)
        frames = int(info.get("frames", "0"))
        result_path = os.path.splitext(s)[0] + ".conferencia"
        label = os.path.basename(s)
        if not info.get("game") or frames <= 0:
            print(f"{label}: header without 'game' or 'frames', skipped")
            continue
        if info.get("edition", ed.id) != ed.id:
            print(f"{label}: edition {info['edition']}, not {ed.id}: skipped (--edition {info['edition']})")
            continue
        if running and time.time() - os.path.getmtime(s) < RECENT_S:
            print(f"{label}: recorded less than 3 min ago with YsRecomp open (may still be recording), skipped")
            continue
        if not a.force and os.path.exists(result_path):
            with open(result_path, encoding="utf-8") as f:
                old = f.read()
            if old.startswith(f"frames: {frames}\nEQUAL\n"):  # different: check again (it may have been fixed)
                print(f"{label}: already checked - EQUAL")
                continue
        todo.append((s, info, frames, result_path))

    def check(item):
        s, info, frames, result_path = item
        ok, text = lockstep(s, info, frames, ed, a)
        lines = [text]
        if ok and info.get("bios", "-") != "-":
            ok, text = geargrafx(s, ed, a)
            lines.append(text)
        verdict = "EQUAL" if ok else "DIFFERENT"
        with open(result_path, "w", encoding="utf-8") as f:
            f.write(f"frames: {frames}\n{verdict}\n" + "\n".join(lines) + "\n")
        return ok, (f"{os.path.basename(s)}: {frames} frames ({frames / 59.82 / 60:.1f} min of play)\n  {verdict}\n  " +
                    "\n  ".join("\n".join(lines).splitlines()))

    failed = checked = 0
    with concurrent.futures.ThreadPoolExecutor(max(1, a.jobs)) as ex:
        for ok, text in ex.map(check, todo):
            print(text, flush=True)
            failed += not ok
            checked += 1
    print(f"{checked} checked now; {failed} with a difference or error." if failed
          else f"{checked} checked now; all equal.")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
