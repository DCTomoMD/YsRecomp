"""Checks that moving values into the profile did not change them (review_jp/05_phase3a_plan.md, chapter 4, T1).

Compares the source as it was at BASE (a git commit) with the working tree:
  - numbers: the numeric literals that left the code (runtime/main.c, runtime/app/, tools/trace/) must be, by value,
    the ones that entered the profile (runtime/profile/), as many times each;
  - strings: each string literal that left the code must be one that entered the profile, or a new format string of
    the code with its %s/%d filled by values that entered the profile (or kept as they were);
  - every string that entered the profile must be used by one of the above.
Comments, spacing and #include paths are ignored; adjacent string literals are joined as the compiler does. Numbers
that left the code for another reason (for example "sizeof(x) - 1" replaced by strlen) are given with
--allow VALUE:COUNT, and the reason goes in the stage's notes. The count is over all the files together, so a number
written anew in the code (a loop that now starts at 0, "base + 1" for the 2nd byte of a value) hides as many moved ones:
those are given with --new VALUE:COUNT (at most as many as the code now has), also explained in the notes. Numbers and
strings that are new in the code are listed for review, not counted as errors.

Usage: python tools/profile_values.py BASE [--allow VALUE:COUNT ...] [--new VALUE:COUNT ...] [--show]
Exit: 0 = the values match, 1 = some difference (listed), 2 = usage error.
Reads no ROM: only the project's sources.
"""
import argparse
import itertools
import os
import re
import subprocess
import sys
from collections import Counter

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from code_same import strip  # noqa: E402  (tokens without comments; string literals kept whole)

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CODE = ("runtime/main.c", "runtime/app", "tools/trace")
PROFILE = ("runtime/profile",)
EXT = (".c", ".h")
NUMBER = re.compile(r"(0[xX][0-9A-Fa-f]+|\d+)[uUlL]*$")
PLACEHOLDER = re.compile(r"%[-+ #0]*\d*(?:\.\d+)?(?:hh|h|ll|l|z)?[sdux]")


def git(*args):
    return subprocess.run(["git", "-C", ROOT, *args], check=True, capture_output=True, text=True,
                          encoding="utf-8").stdout


def base_files(base, tops):
    names = git("ls-tree", "-r", "--name-only", base, "--", *tops).split("\n")
    return {n: git("show", f"{base}:{n}") for n in names if n.endswith(EXT)}


def tree_files(tops):
    out = {}
    for top in tops:
        path = os.path.join(ROOT, top)
        if os.path.isfile(path):
            out[top] = open(path, encoding="utf-8").read()
            continue
        for d, dirs, files in os.walk(path):
            dirs[:] = sorted(x for x in dirs if not x.startswith(".") and x != "__pycache__")
            for f in sorted(files):
                if f.endswith(EXT) and not f.startswith("."):
                    p = os.path.join(d, f)
                    out[os.path.relpath(p, ROOT).replace("\\", "/")] = open(p, encoding="utf-8").read()
    return out


def c_int(text):
    """Value of a C integer literal (0x hex, leading 0 octal, else decimal)."""
    if text[:2] in ("0x", "0X"):
        return int(text, 16)
    if len(text) > 1 and text[0] == "0":
        return int(text, 8)
    return int(text)


def literals(sources):
    """(numbers by value, strings) of every file, as multisets."""
    nums, strs = Counter(), Counter()
    for src in sources.values():
        pending = None
        prev = None
        for tok in strip(src):
            after_include, prev = prev == "include", tok
            if tok.startswith('"') and after_include:
                continue  # #include "file": a path, not a value
            if tok.startswith('"'):
                pending = (pending or "") + tok[1:-1]
                continue
            if pending is not None:
                strs[pending] += 1
                pending = None
            m = NUMBER.match(tok)
            if m:
                nums[c_int(m.group(1))] += 1
        if pending is not None:
            strs[pending] += 1
    return nums, strs


def fill(fmt, values, target):
    """Every text the format can give with its placeholders filled by the values (or left as they are). Only the values
    whose text is in target are tried: the others cannot give it."""
    parts = PLACEHOLDER.split(fmt)
    holes = PLACEHOLDER.findall(fmt)
    if not holes or len(holes) > 6:
        return
    choices = []
    for h in holes:
        if h.endswith("s"):
            choices.append([h] + [v for v in values if isinstance(v, str) and v in target])
        else:
            choices.append([h] + [str(v) for v in values if isinstance(v, int) and str(v) in target])
    for combo in itertools.product(*choices):
        yield "".join(p + (combo[i] if i < len(combo) else "") for i, p in enumerate(parts)), combo


def main():
    ap = argparse.ArgumentParser(description="Values moved into the profile are unchanged (T1).")
    ap.add_argument("base", help="git commit before the stage")
    ap.add_argument("--allow", action="append", default=[], metavar="VALUE:COUNT",
                    help="a number that left the code for another reason (say why in the stage's notes)")
    ap.add_argument("--new", action="append", default=[], metavar="VALUE:COUNT",
                    help="a number written anew in the code, hiding a moved one (say why in the stage's notes)")
    ap.add_argument("--show", action="store_true", help="also list every value that matched")
    a = ap.parse_args()
    try:
        git("rev-parse", "--verify", a.base + "^{commit}")
    except subprocess.CalledProcessError:
        print(f"profile_values: FAILED: {a.base} is not a commit")
        return 2

    code_old, code_new = literals(base_files(a.base, CODE)), literals(tree_files(CODE))
    prof_old, prof_new = literals(base_files(a.base, PROFILE)), literals(tree_files(PROFILE))
    left_n, entered_n = code_old[0] - code_new[0], prof_new[0] - prof_old[0]
    new_code_n = code_new[0] - code_old[0]
    left_s, entered_s = code_old[1] - code_new[1], prof_new[1] - prof_old[1]
    new_code_s = code_new[1] - code_old[1]

    def counts(items):
        out = Counter()
        for item in items:
            v, _, c = item.partition(":")
            out[c_int(v)] += int(c or 1)
        return out

    allowed, written = counts(a.allow), counts(a.new)
    problems = []

    # numbers written anew in the code hide as many moved ones: count those as having left
    extra_new = Counter({v: c - code_new[0][v] for v, c in written.items() if c > code_new[0][v]})
    if extra_new:
        problems.append(f"--new more than the code has: {dict(extra_new)}")
    left_n = left_n + written

    # numbers: what left the code (less the allowed ones) = what entered the profile
    extra_allowed = allowed - left_n
    if extra_allowed:
        problems.append(f"--allow more than left the code: {dict(extra_allowed)}")
    left_n = left_n - allowed
    for v in sorted(set(left_n) | set(entered_n)):
        if left_n[v] != entered_n[v]:
            problems.append(f"number {v} (0x{v:X}): left the code {left_n[v]} time(s), entered the profile "
                            f"{entered_n[v]} time(s)")

    # strings: each one that left is in the profile, or a new format of the code filled with profile values
    values = sorted(entered_s) + sorted(entered_n)
    remaining = Counter(entered_s)
    used = set()
    explained = []
    for s in sorted(left_s.elements()):
        if remaining[s] > 0:
            remaining[s] -= 1
            used.add(s)
            explained.append(f'"{s}" = the profile\'s "{s}"')
            continue
        found = None
        for fmt in sorted(new_code_s):
            found = next((combo for text, combo in fill(fmt, values, s) if text == s), None)
            if found:
                break
        if found:
            used.update(c for c in found if c in entered_s)
            explained.append(f'"{s}" = "{fmt}" with {list(found)}')
        else:
            problems.append(f'string "{s}" left the code and is not rebuilt from the profile')
    for v in sorted(entered_s):
        if v not in used:
            problems.append(f'string "{v}" entered the profile but no string that left the code uses it')

    print(f"profile_values: base {a.base}")
    print(f"  numbers: {sum(left_n.values())} left the code (+{sum(allowed.values())} allowed; "
          f"{sum(written.values())} counted with --new), "
          f"{sum(entered_n.values())} entered the profile")
    print(f"  strings: {sum(left_s.values())} left the code, {sum(entered_s.values())} entered the profile")
    if a.show:
        for v in sorted(entered_n.elements()):
            print(f"    number {v} (0x{v:X})")
        for e in explained:
            print(f"    {e}")
    if new_code_n or new_code_s:
        print("  new in the code (review):")
        for v in sorted(new_code_n.elements()):
            print(f"    number {v}")
        for s in sorted(new_code_s.elements()):
            print(f'    string "{s}"')
    for p in problems:
        print(f"  DIFFERENCE: {p}")
    print(f"profile_values: {'OK' if not problems else f'{len(problems)} difference(s)'}")
    return 0 if not problems else 1


if __name__ == "__main__":
    sys.exit(main())
