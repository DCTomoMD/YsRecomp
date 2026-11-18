"""Compares two ctest runs (JUnit XML from `ctest --output-junit`) of the same runner: a baseline commit and the
commit under test. Each test is classified as passed (it ran and passed), skipped (exit code 77, SKIP_RETURN_CODE:
the test needs files that are not there, e.g. the user's disc) or failed (it ran and failed, or could not run).
A skip is not a pass: skips are listed apart, with the test's own message.

The run under test is rejected (exit code 1) when:
  - a test fails now and did not fail in the baseline (new failure), or a test new in this commit fails;
  - a test that passed in the baseline is skipped now;
  - a test of the baseline no longer exists;
  - a test fails in both runs and is not listed with --known-fail (every accepted failure needs a stated reason);
  - a test given with --require-pass did not run and pass;
  - either JUnit file is missing or unreadable (the build or the tests did not get that far).

Usage:
  python tools/ci/ctest_report.py --base base.xml --head head.xml
      [--known-fail NAME=REASON ...] [--require-pass NAME ...] [--summary FILE]
--summary appends a Markdown report (GitHub's $GITHUB_STEP_SUMMARY); the same report goes to stdout.
"""
import argparse
import sys
import xml.etree.ElementTree as ET

SKIP_77 = "SKIP_RETURN_CODE=77"


def read_junit(path):
    """{test name: (state, detail)}; state is 'pass', 'skip' or 'fail'. None when the file cannot be read."""
    try:
        root = ET.parse(path).getroot()
    except (OSError, ET.ParseError) as e:
        print(f"ctest_report: cannot read {path}: {e}", file=sys.stderr)
        return None
    tests = {}
    for case in root.iter("testcase"):
        name = case.get("name")
        status = case.get("status", "")
        skipped = case.find("skipped")
        failure = case.find("failure")
        if failure is not None or status == "fail":
            detail = failure.get("message", "") if failure is not None else ""
            tests[name] = ("fail", detail or "failed")
        elif skipped is not None:
            message = skipped.get("message", "")
            if message == SKIP_77:
                out = case.findtext("system-out") or ""
                line = next((l.strip() for l in out.splitlines() if "SKIP" in l.upper()), "")
                tests[name] = ("skip", line or message)
            else:
                # "Unable to find executable", "Disabled", ...: the test did not run, which is not a skip
                tests[name] = ("fail", f"not run: {message}")
        elif status == "run":
            tests[name] = ("pass", "")
        else:
            tests[name] = ("fail", f"unknown status '{status}'")
    return tests


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--base", required=True, help="JUnit XML of the baseline commit")
    ap.add_argument("--head", required=True, help="JUnit XML of the commit under test")
    ap.add_argument("--base-name", default="base", help="label of the baseline in the report")
    ap.add_argument("--head-name", default="head", help="label of the commit under test in the report")
    ap.add_argument("--known-fail", action="append", default=[], metavar="NAME=REASON",
                    help="a test accepted as failing in both runs, with the reason")
    ap.add_argument("--require-pass", action="append", default=[], metavar="NAME",
                    help="a test that must run and pass in the commit under test")
    ap.add_argument("--summary", help="also append the Markdown report to this file")
    args = ap.parse_args()

    known = {}
    for item in args.known_fail:
        name, _, reason = item.partition("=")
        known[name] = reason or "(no reason given)"

    base = read_junit(args.base)
    head = read_junit(args.head)
    lines = [f"## ctest: {args.head_name} compared with {args.base_name}", ""]
    problems = []
    if base is None:
        problems.append(f"no test results for {args.base_name} ({args.base})")
    if head is None:
        problems.append(f"no test results for {args.head_name} ({args.head})")

    if base is not None and head is not None:
        lines += [f"| test | {args.base_name} | {args.head_name} | note |", "|---|---|---|---|"]
        counts = {"pass": 0, "skip": 0, "fail": 0}
        skips = []
        for name in sorted(set(base) | set(head)):
            b = base.get(name)
            h = head.get(name)
            bs = b[0] if b else "-"
            hs = h[0] if h else "-"
            note = ""
            if h is None:
                note = "REMOVED"
                problems.append(f"{name}: exists in {args.base_name}, missing in {args.head_name}")
            else:
                counts[hs] += 1
                if hs == "fail":
                    if b is None:
                        note = f"NEW TEST FAILS: {h[1]}"
                        problems.append(f"{name}: new test, fails ({h[1]})")
                    elif bs != "fail":
                        note = f"NEW FAILURE: {h[1]}"
                        problems.append(f"{name}: {bs} in {args.base_name}, fails now ({h[1]})")
                    elif name in known:
                        note = f"known failure (both): {known[name]}"
                    else:
                        note = f"FAILS IN BOTH, no stated reason: {h[1]}"
                        problems.append(f"{name}: fails in both runs and is not a --known-fail ({h[1]})")
                elif hs == "skip":
                    skips.append((name, h[1]))
                    note = f"skipped (77): {h[1]}"
                    if bs == "pass":
                        note = "PASSED BEFORE, SKIPPED NOW: " + h[1]
                        problems.append(f"{name}: passed in {args.base_name}, skipped now")
                elif b is None:
                    note = "new test"
            if name in args.require_pass and hs != "pass":
                problems.append(f"{name}: required to pass, got {hs}")
                note = (note + "; " if note else "") + "REQUIRED TO PASS"
            lines.append(f"| {name} | {bs} | {hs} | {note} |")
        for name in args.require_pass:
            if name not in head and name not in base:
                problems.append(f"{name}: required to pass, not defined")
        lines += ["", f"**{args.head_name}: {counts['pass']} passed (ran), {counts['skip']} skipped (exit 77, "
                  f"not a pass), {counts['fail']} failed** — {len(head)} tests; {args.base_name}: {len(base)} tests.",
                  ""]
        if skips:
            lines.append("Skipped (they did not check anything here):")
            lines += [f"- `{n}`: {m}" for n, m in skips]
            lines.append("")
        unused = sorted(set(known) - {n for n, v in head.items() if v[0] == "fail"})
        if unused:
            lines.append("Listed as known failures but did not fail: " + ", ".join(f"`{n}`" for n in unused))
            lines.append("")

    if problems:
        lines.append("### REJECTED")
        lines += [f"- {p}" for p in problems]
    else:
        lines.append("### OK: no new failures, no lost tests, no pass turned into a skip")
    report = "\n".join(lines) + "\n"
    sys.stdout.write(report)
    if args.summary:
        with open(args.summary, "a", encoding="utf-8") as f:
            f.write(report)
    print("ctest_report: " + ("REJECTED" if problems else "OK"))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
