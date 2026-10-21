#!/usr/bin/env python3
"""mips.py WAD OUT: the flats made smaller, for the floors and ceilings far away (src/pd_render.cpp), packed next to
the WHX (src/nw/data.S): each flat of the WAD (every lump from F_START to F_END, as the game numbers them) at 32 x 32,
16 x 16 and 8 x 8, each pixel the average of the flat's pixels under it, in the palette's colours, then the
palette's nearest colour. Point-sampled from 64 x 64, a flat seen far away jumps from one texel to another as the
view moves (the red lights of E1M1 from its first door); its smaller copy has what all those texels average to.

OUT: "NWMP", the number of flats (16 bits), 0 (16 bits), then 1344 bytes for each flat: 32 x 32, 16 x 16, 8 x 8,
stored as the game stores flats, a column at a time (pixel [x][y] at x * size + y, x across the flat as the WAD
has it). A lump that is not a 64 x 64 flat (a marker) has zeros."""
import struct
import sys

SIZES = (32, 16, 8)
STRIDE = sum(s * s for s in SIZES)


def read_wad(path):
    d = open(path, "rb").read()
    if d[:4] not in (b"IWAD", b"PWAD"):
        sys.exit(f"mips.py: {path} is not a WAD")
    n, off = struct.unpack("<II", d[4:12])
    lumps = []
    for i in range(n):
        o, s, name = struct.unpack("<II8s", d[off + 16 * i:off + 16 * i + 16])
        lumps.append((name.split(b"\0")[0].decode("latin1").upper(), d[o:o + s]))
    return lumps


def last(lumps, name):
    found = [i for i, (n, _) in enumerate(lumps) if n == name]
    if not found:
        sys.exit(f"mips.py: no {name}")
    return found[-1]


def main():
    lumps = read_wad(sys.argv[1])
    pal = lumps[last(lumps, "PLAYPAL")][1][:768]
    rgb = [tuple(pal[3 * i:3 * i + 3]) for i in range(256)]
    nearest = {}

    def colour(r, g, b):
        k = (r, g, b)
        if k not in nearest:
            nearest[k] = min(range(256), key=lambda i: (rgb[i][0] - r) ** 2 + (rgb[i][1] - g) ** 2 + (rgb[i][2] - b) ** 2)
        return nearest[k]

    first, end = last(lumps, "F_START") + 1, last(lumps, "F_END")
    out = bytearray(b"NWMP" + struct.pack("<HH", end - first, 0))
    for name, flat in lumps[first:end]:
        if len(flat) != 4096:
            out += bytes(STRIDE)
            continue
        for size in SIZES:
            k = 64 // size
            n = k * k
            for x in range(size):
                for y in range(size):
                    r = g = b = 0
                    for v in range(y * k, y * k + k):
                        row = v * 64
                        for u in range(x * k, x * k + k):
                            c = rgb[flat[row + u]]
                            r += c[0]
                            g += c[1]
                            b += c[2]
                    out.append(colour((r + n // 2) // n, (g + n // 2) // n, (b + n // 2) // n))
    open(sys.argv[2], "wb").write(bytes(out))


if __name__ == "__main__":
    main()
