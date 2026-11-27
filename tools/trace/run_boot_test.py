"""Test: a disc boots with the real BIOS and reaches the game (smoke test of an edition that has no disc map yet).

Runs pce_trace --frame with the BIOS and a button script for N frames and checks:
  1. it ran to the end (exit code 0) and printed no FAILED line (a BIOS routine or a part of the CD missing);
  2. the CD music played;
  3. each --ram16 ADDRESS=VALUE: the 16-bit little-endian value at that CPU address of the work RAM ($2000-$3FFF)
     in the final state (--dump-state). The address and the value come from the caller, read from that disc.
The image is written (to look at when the test fails) but not checked: no value taken from the disc's pictures or
text is kept here.

Usage: python tools/trace/run_boot_test.py <pce_trace> <image.cue> <bios.pce> <buttons.txt> <frames> <output dir>
           [--ram16 ADDRESS=VALUE ...]
Exit code: 0 = ok, 1 = failed, 77 = no disc or BIOS (test skipped).
"""
import argparse
import os
import re
import subprocess
import sys

# Offset of the work RAM in the --dump-state file (PCERecomp tools/trace/vdc_state.py): VRAM (32768 words), SAT (256),
# VCE colors (512), VDC registers (20), then the 8 KB of RAM
RAM_OFFSET = (32768 + 256 + 512 + 20) * 2
RAM_BASE, RAM_SIZE = 0x2000, 0x2000


def ram16(text):
    m = re.fullmatch(r"(?:0x|\$)?([0-9A-Fa-f]+)=(\d+)", text)
    if not m:
        raise argparse.ArgumentTypeError(f"'{text}' is not ADDRESS=VALUE (hex address, decimal value)")
    address, value = int(m.group(1), 16), int(m.group(2))
    if not RAM_BASE <= address < RAM_BASE + RAM_SIZE - 1 or value > 0xFFFF:
        raise argparse.ArgumentTypeError(f"'{text}': the address is outside $2000-$3FFE or the value over 65535")
    return address, value


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("pce_trace")
    ap.add_argument("cue")
    ap.add_argument("bios")
    ap.add_argument("buttons")
    ap.add_argument("frames", type=int)
    ap.add_argument("out")
    ap.add_argument("--ram16", type=ram16, action="append", default=[], metavar="ADDRESS=VALUE")
    a = ap.parse_args()

    missing = [p for p in (a.cue, a.bios) if not os.path.exists(p)]
    if missing:
        print(f"missing disc or BIOS ({' / '.join(missing)}): test skipped")
        return 77
    os.makedirs(a.out, exist_ok=True)
    ppm = os.path.join(a.out, "boot.ppm")
    state = os.path.join(a.out, "boot.state")
    if os.path.exists(state):
        os.remove(state)  # never judge a state left by an earlier run
    r = subprocess.run([a.pce_trace, "--frame", "--input", a.buttons, "--bios", a.bios, a.cue, str(a.frames), ppm,
                        "--dump-state", state], capture_output=True, text=True, encoding="utf-8", errors="replace")
    text = r.stdout + r.stderr
    print(text.strip())
    ok = True
    if r.returncode != 0:
        print(f"FAILED: pce_trace ended with code {r.returncode}")
        ok = False
    if re.search(r"^FAILED", text, re.M):
        print("FAILED: the run reported a failure (see message above)")
        ok = False
    m = re.search(r"disc music played ([0-9.]+) s", text)
    if not m or float(m.group(1)) <= 0:
        print("FAILED: the CD music did not play")
        ok = False
    if a.ram16:
        data = open(state, "rb").read() if os.path.exists(state) else b""
        if len(data) < RAM_OFFSET + RAM_SIZE:
            print(f"FAILED: no final state ({state})")
            ok = False
        else:
            for address, value in a.ram16:
                o = RAM_OFFSET + address - RAM_BASE
                got = data[o] | data[o + 1] << 8
                same = got == value
                print(f"RAM ${address:04X} (16 bits): {got}, expected {value}: {'ok' if same else 'DIFFERENT'}")
                if not same:
                    print("FAILED: the game did not reach the expected state")
                    ok = False
    print("OK: the disc booted with the BIOS and reached the game" if ok else "RESULT: failed")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
