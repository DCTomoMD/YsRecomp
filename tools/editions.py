"""Editions of the game (USA; JP from Phase 3B) for the YsRecomp scripts: names, folders, disc and BIOS.

The disc and the BIOS follow the same rule as CMake (cmake/ys_edition.cmake, ys_rom_file): the cache variable
(YS_USA_CUE, YS_USA_BIOS in build/CMakeCache.txt) when it was given, otherwise roms/<upstream name> and then
roms/usa/ (disc) or roms/bios/ (BIOS).

Recorded sessions write the disc and the BIOS as the recording machine saw them (`# game: roms\\...cue`,
`# game: D:\\Roms\\...\\...cue`). header_path() turns a header value into a path on this machine without changing the
session file: backslashes become slashes; a path that exists is used as it is; otherwise, when its file name is the
file name of the edition's disc or BIOS, that file is used.

    import editions
    ed = editions.get("usa")                 # exits with a message for an unknown or not yet available edition
    ed.cue(), ed.bios(), ed.sessions_dir, build_paths.exe(ed.trace)
    editions.header_path(info["game"], ed)
"""
import os
import re
import sys
from dataclasses import dataclass

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CMAKE_CACHE = os.path.join(ROOT, "build", "CMakeCache.txt")
DEFAULT = "usa"
PLANNED = ("jp",)  # Phase 3B


@dataclass(frozen=True)
class Edition:
    id: str
    exe: str            # player executable (without .exe)
    trace: str          # recompiled trace tool
    ptrace: str         # interpreter trace tool (with the Ys extension)
    game_dir: str       # disc.toml, sessions/, coverage/, generated/
    cue_var: str
    cue_name: str
    cue_subdir: str
    bios_var: str
    bios_name: str
    bios_subdir: str

    @property
    def sessions_dir(self):
        return os.path.join(self.game_dir, "sessions")

    def cue(self):
        return rom_file(self.cue_var, self.cue_name, self.cue_subdir)

    def bios(self):
        return rom_file(self.bios_var, self.bios_name, self.bios_subdir)


EDITIONS = {
    "usa": Edition(id="usa", exe="YsRecomp", trace="ys_trace", ptrace="pce_trace",
                   game_dir=os.path.join(ROOT, "games", "ys"),
                   cue_var="YS_USA_CUE", cue_name="Ys Book I & II (USA).cue", cue_subdir="usa",
                   bios_var="YS_USA_BIOS", bios_name="[BIOS] TurboGrafx CD System Card (USA) (v2.0).pce",
                   bios_subdir="bios"),
}


def get(edition_id):
    """The edition with this id; exits (code 2) for one that does not exist or is not available yet."""
    ed = EDITIONS.get(edition_id)
    if ed:
        return ed
    if edition_id in PLANNED:
        print(f"edition '{edition_id}' is not available yet (Phase 3B); available: {', '.join(EDITIONS)}",
              file=sys.stderr)
    else:
        print(f"unknown edition '{edition_id}'; available: {', '.join(EDITIONS)}", file=sys.stderr)
    sys.exit(2)


def add_argument(parser):
    parser.add_argument("--edition", default=DEFAULT,
                        help=f"edition of the game: {', '.join(EDITIONS)} (default {DEFAULT})")


def cmake_cache_value(name, cache=CMAKE_CACHE):
    """Value of NAME in build/CMakeCache.txt, or '' (not there, empty, or no build yet)."""
    try:
        with open(cache, encoding="utf-8", errors="replace") as f:
            for line in f:
                m = re.match(r"([A-Za-z0-9_]+):[A-Z]+=(.*)$", line.rstrip("\r\n"))
                if m and m.group(1) == name:
                    return m.group(2)
    except OSError:
        pass
    return ""


def rom_file(var, file_name, subdir):
    """The disc or BIOS file chosen like CMake does (it may not exist)."""
    given = cmake_cache_value(var)
    if given:
        return given
    upstream = os.path.join(ROOT, "roms", file_name)
    sub = os.path.join(ROOT, "roms", subdir, file_name)
    if not os.path.exists(upstream) and os.path.exists(sub):
        return sub
    return upstream


def normalize(value):
    """A path written in a session header, with slashes, relative to the project root unless absolute."""
    p = value.replace("\\", "/")
    if os.path.isabs(p):
        return p
    return os.path.join(ROOT, p)


def header_path(value, edition, override=None):
    """The disc or BIOS of a session header ('game' or 'bios' value) on this machine.

    override: a path given on the command line (--cue/--bios): the user's explicit choice, used instead of the
    header."""
    if override:
        return os.path.abspath(override)
    p = normalize(value)
    if os.path.exists(p):
        return p
    name = os.path.basename(value.replace("\\", "/"))
    for candidate in (edition.cue(), edition.bios()):
        if os.path.basename(candidate) == name and os.path.exists(candidate):
            return candidate
    return p


def session_edition(info, given=None):
    """The edition of a session: its '# edition:' header line (sessions before it have none: usa), checked against
    the one given on the command line. Exits (code 2) when they differ."""
    header = info.get("edition", "").strip()
    if header and given and header != given:
        print(f"the session is of edition '{header}', not '{given}'", file=sys.stderr)
        sys.exit(2)
    return get(header or given or DEFAULT)
