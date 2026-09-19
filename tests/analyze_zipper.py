#!/usr/bin/env python3
"""
Zipper-noise metric for a .f32 render.

Block-rate parameter updates create discontinuities exactly at block
boundaries. This measures the mean sample-to-sample jump AT those boundaries
against the mean jump everywhere else.

    ratio ~= 1   : no block-synchronous discontinuity (smooth)
    ratio >> 1   : zipper noise — the signal steps once per block

Usage:  analyze_zipper.py <file.f32> [blockSize] [numChannels]
"""
import struct, sys, math

def analyze(path, block=512, ch=2):
    raw = open(path, "rb").read()
    v = struct.unpack("<%df" % (len(raw) // 4), raw)
    frames = len(v) // ch
    out = []
    for c in range(ch):
        x = v[c::ch]
        bsum = bn = isum = ino = 0
        bmax = 0.0
        for n in range(1, frames):
            d = abs(x[n] - x[n - 1])
            if n % block == 0:
                bsum += d; bn += 1
                if d > bmax: bmax = d
            else:
                isum += d; ino += 1
        bmean = bsum / bn if bn else 0.0
        imean = isum / ino if ino else 0.0
        ratio = (bmean / imean) if imean > 0 else float("inf")
        out.append((bmean, imean, ratio, bmax))
    return out

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(__doc__); sys.exit(2)
    path = sys.argv[1]
    block = int(sys.argv[2]) if len(sys.argv) > 2 else 512
    ch = int(sys.argv[3]) if len(sys.argv) > 3 else 2
    res = analyze(path, block, ch)
    name = path.split("/")[-1]
    print(f"{name}  (block={block})")
    for i, (bmean, imean, ratio, bmax) in enumerate(res):
        db = (20 * math.log10(bmax)) if bmax > 0 else float("-inf")
        print(f"  ch{i}: jump at boundary {bmean:.3e} | elsewhere {imean:.3e} "
              f"| ratio {ratio:6.2f}x | worst step {db:7.2f} dBFS")
    worst = max(r[2] for r in res)
    print(f"  => zipper ratio {worst:.2f}x  "
          f"({'CLEAN' if worst < 1.5 else 'ZIPPER PRESENT'})")
