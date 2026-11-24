#!/usr/bin/env python3
"""Generate a faceted rubber duck in the Triangles input format."""

import argparse
import math
from pathlib import Path


def ellipsoid(center, radii, segments, tilt=0):
    """Yield outward-facing triangles, with one vertex at each pole."""
    rings = segments // 2
    cosine, sine = math.cos(tilt), math.sin(tilt)

    def point(x, y, z):
        x *= radii[0]
        y *= radii[1]
        return (center[0] + x * cosine - y * sine,
                center[1] + x * sine + y * cosine,
                center[2] + z * radii[2])

    azimuths = [index * math.tau / segments for index in range(segments)]
    directions = [(math.cos(angle), math.sin(angle)) for angle in azimuths]
    start, end = point(1, 0, 0), point(-1, 0, 0)
    previous = None

    for index in range(1, rings):
        angle = math.pi * index / rings
        x, radius = math.cos(angle), math.sin(angle)
        ring = [point(x, radius * y, radius * z) for y, z in directions]
        for j in range(segments):
            next_j = (j + 1) % segments
            if previous is None:
                yield start, ring[j], ring[next_j]
            else:
                yield previous[j], ring[j], ring[next_j]
                yield previous[j], ring[next_j], previous[next_j]
        previous = ring

    for j in range(segments):
        yield previous[j], end, previous[(j + 1) % segments]


def duck(segments):
    """Build the silhouette in the XY plane, facing right."""
    small = max(8, segments // 2)
    # Rounded body, raised tail, and head.
    yield from ellipsoid((-0.45, -1.65, 0), (5.05, 2.9, 3.25), segments)
    yield from ellipsoid((-4.55, -0.02, 0), (2.05, 0.9, 1.33), small, -0.52)
    yield from ellipsoid((1.85, 2.15, 0), (2.7, 2.65, 2.4), segments)

    # Flattened upper and lower bill give the profile a lip.
    yield from ellipsoid((4.25, 1.32, 0), (2.1, 0.64, 1.49), segments, -0.15)
    yield from ellipsoid((4.35, 0.83, 0), (1.84, 0.32, 1.30), small, -0.12)

    for side in (-1, 1):
        yield from ellipsoid((-1.15, -1.27, side * 2.66),
                             (2.95, 1.41, 0.8), small, 0.22)
        yield from ellipsoid((2.79, 2.96, side * 2.16),
                             (0.44, 0.52, 0.37), small)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path, help="output file")
    parser.add_argument("--segments", type=int, default=24,
                        help="resolution of the larger parts (default: 24)")
    args = parser.parse_args()
    if args.segments < 8:
        parser.error("--segments must be at least 8")

    count = 0
    with args.output.open("w", encoding="utf-8", newline="\n") as output:
        for triangle in duck(args.segments):
            output.write("3\n")
            for point in triangle:
                output.write(" ".join(f"{coordinate:.12f}" for coordinate in point) + "\n")
            output.write("\n")
            count += 1
    print(f"Triangles: {count}; output: {args.output}")


if __name__ == "__main__":
    main()
