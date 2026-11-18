"""Checks a compiled Windows resource file (.res, from rc.exe) of the player's executable: it holds the icon group,
every image of the .ico byte for byte (rc found the icon the .rc names) and the version information.
With --compare, the resources must also be the same as in another .res (e.g. the one compiled before moving the
.rc and the .ico: same icon, same version block).

Usage: python tools/ci/check_rc.py --res FILE.res --ico FILE.ico [--compare OTHER.res]
Prints "check_rc: OK" when everything matches.
"""
import argparse
import hashlib
import struct
import sys

RT_ICON, RT_GROUP_ICON, RT_VERSION = 3, 14, 16


def _id(data, pos):
    """A resource type or name: an ordinal (0xFFFF + WORD) or a zero-terminated UTF-16 string."""
    if struct.unpack_from("<H", data, pos)[0] == 0xFFFF:
        return struct.unpack_from("<H", data, pos + 2)[0], pos + 4
    end = pos
    while struct.unpack_from("<H", data, end)[0] != 0:
        end += 2
    return data[pos:end].decode("utf-16-le"), end + 2


def read_res(path):
    """[(type, name, language, data)] of a 32-bit .res file (the first, empty entry left out)."""
    with open(path, "rb") as f:
        data = f.read()
    out = []
    pos = 0
    while pos + 8 <= len(data):
        size, header = struct.unpack_from("<II", data, pos)
        rtype, p = _id(data, pos + 8)
        name, p = _id(data, p)
        p = (p + 3) & ~3
        _version, _flags, lang = struct.unpack_from("<IHH", data, p)
        body = data[pos + header:pos + header + size]
        if len(body) != size:
            raise ValueError(f"{path}: truncated resource at offset {pos}")
        if not (rtype == 0 and size == 0):
            out.append((rtype, name, lang, body))
        pos = (pos + header + size + 3) & ~3
    return out


def read_ico(path):
    """The images (bytes) of an .ico file, in directory order."""
    with open(path, "rb") as f:
        data = f.read()
    reserved, kind, count = struct.unpack_from("<HHH", data, 0)
    if reserved != 0 or kind != 1:
        raise ValueError(f"{path}: not an .ico file")
    images = []
    for i in range(count):
        size, offset = struct.unpack_from("<II", data, 6 + 16 * i + 8)
        images.append(data[offset:offset + size])
    return images


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--res", required=True)
    ap.add_argument("--ico", required=True)
    ap.add_argument("--compare", help="another .res that must hold the same resources")
    args = ap.parse_args()

    errors = []
    res = read_res(args.res)
    ico = read_ico(args.ico)
    for rtype, name, lang, body in res:
        print(f"  type {rtype} name {name} lang 0x{lang:04x}: {len(body)} bytes, "
              f"sha256 {hashlib.sha256(body).hexdigest()[:16]}")
    groups = [r for r in res if r[0] == RT_GROUP_ICON]
    icons = {r[1]: r[3] for r in res if r[0] == RT_ICON}
    if len(groups) != 1:
        errors.append(f"{len(groups)} icon groups (1 expected)")
    else:
        body = groups[0][3]
        count = struct.unpack_from("<H", body, 4)[0]
        if count != len(ico):
            errors.append(f"icon group has {count} images, the .ico {len(ico)}")
        for i in range(min(count, len(ico))):
            icon_id = struct.unpack_from("<H", body, 6 + 14 * i + 12)[0]
            if icons.get(icon_id) != ico[i]:
                errors.append(f"image {i} (resource {icon_id}) differs from the .ico")
    if not any(r[0] == RT_VERSION for r in res):
        errors.append("no version information")
    elif "YsRecomp".encode("utf-16-le") not in next(r[3] for r in res if r[0] == RT_VERSION):
        errors.append("version information without the product name")
    if args.compare:
        other = read_res(args.compare)
        if [(t, n, l, hashlib.sha256(b).digest()) for t, n, l, b in res] != \
           [(t, n, l, hashlib.sha256(b).digest()) for t, n, l, b in other]:
            errors.append(f"resources differ from {args.compare}")
    for e in errors:
        print("check_rc: " + e)
    print("check_rc: " + ("FAILED" if errors else "OK"))
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
