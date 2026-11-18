"""Where two rendered gallery pages differ: bounding boxes of the differences.

    python tools/diff_regions.py a.png b.png [min-count]

Groups differing pixels into rows of 8 and merges neighbouring rows, so the
report names the bands of the picture that are not the same, with their first
difference and their colors. Used while the gallery of examples/gallery.xml is
matched against examples/gallery_ui.c (see tests/xml_gallery_shots.c).
"""
import sys

from PIL import Image
import numpy as np


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 2
    a = np.asarray(Image.open(argv[1]).convert("RGBA")).astype(np.int16)
    b = np.asarray(Image.open(argv[2]).convert("RGBA")).astype(np.int16)
    minimum = int(argv[3]) if len(argv) > 3 else 1
    if a.shape != b.shape:
        print("different sizes: %s against %s" % (a.shape, b.shape))
        return 1
    diff = np.any(a != b, axis=2)
    total = int(diff.sum())
    print("%d pixels differ of %d (%.3f%%)" % (total, diff.size, 100.0 * total / diff.size))
    if not total:
        return 0
    rows = np.where(diff.any(axis=1))[0]
    bands = []
    start = prev = rows[0]
    for y in rows[1:]:
        if y - prev > 12:
            bands.append((start, prev))
            start = y
        prev = y
    bands.append((start, prev))
    for y0, y1 in bands:
        band = diff[y0:y1 + 1]
        count = int(band.sum())
        if count < minimum:
            continue
        xs = np.where(band.any(axis=0))[0]
        y, x = np.argwhere(band)[0]
        first = (y0 + int(y), int(xs[0]))
        ca = a[first[0], first[1]]
        cb = b[first[0], first[1]]
        print("y %4d..%-4d  x %4d..%-4d  %7d pixels   first at %d,%d  #%02x%02x%02x against #%02x%02x%02x"
              % (y0, y1, int(xs[0]), int(xs[-1]), count, first[1], first[0],
                 ca[0], ca[1], ca[2], cb[0], cb[1], cb[2]))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
