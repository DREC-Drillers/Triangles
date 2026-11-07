#!/usr/bin/env python3
"""Generate mixed or uniformly random triangles in a configurable coordinate range."""

import argparse
import math
import random
from pathlib import Path


def triangle(rng, minimum, maximum):
    for _ in range(1000):
        points = [tuple(round(rng.uniform(minimum, maximum), 6) for _ in range(3))
                  for _ in range(3)]
        a, b, c = points
        u = [b[i] - a[i] for i in range(3)]
        v = [c[i] - a[i] for i in range(3)]
        cross = (u[1] * v[2] - u[2] * v[1],
                 u[2] * v[0] - u[0] * v[2],
                 u[0] * v[1] - u[1] * v[0])
        if sum(x * x for x in cross) > 1e-12:
            return points
    raise ValueError("Range is too small to generate a valid triangle at 6 decimal places")


def mixed_triangles(rng, count, minimum, maximum, intersecting_fraction):
    cells = (count + 1) // 2
    side = math.ceil(cells ** (1 / 3))
    while side ** 3 < cells:
        side += 1
    step = (maximum - minimum) / side
    if not math.isfinite(step) or step <= 0:
        raise ValueError("Coordinate range is not representable")

    intersecting = min(2 * round(count * intersecting_fraction / 2), 2 * (count // 2))
    order = list(range(cells))
    rng.shuffle(order)

    for position, cell in enumerate(order):
        origin = (minimum + (cell % side) * step,
                  minimum + ((cell // side) % side) * step,
                  minimum + (cell // (side * side)) * step)
        axes = [0, 1, 2]
        rng.shuffle(axes)
        active = position < intersecting // 2
        offsets = (0, 0.05 if active else 0.5)
        pair_size = min(2, count - position * 2)
        for offset in offsets[:pair_size]:
            points = []
            for u, v in ((0.1, 0.1), (0.4, 0.1), (0.1, 0.4)):
                local = (u + offset, v + offset, 0.5)
                point = tuple(origin[axis] + step * local[axes[axis]]
                              for axis in range(3))
                points.append(point)
            if len(set(points)) != 3:
                raise ValueError("Coordinate range is too small for the requested count")
            yield points


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("count", type=int, help="number of triangles")
    parser.add_argument("output", type=Path, help="output file")
    parser.add_argument("--seed", type=int, help="seed for reproducible inputs")
    parser.add_argument("--mode", choices=("mixed", "random"), default="mixed",
                        help="mixed: isolated pairs with a controlled intersection ratio (default)")
    parser.add_argument("--intersecting-fraction", type=float, default=0.5,
                        help="fraction of triangles participating in intersections in mixed mode (default: 0.5)")
    parser.add_argument("--min", dest="minimum", type=float, default=-10,
                        help="minimum coordinate (default: -10)")
    parser.add_argument("--max", dest="maximum", type=float, default=10,
                        help="maximum coordinate (default: 10)")
    args = parser.parse_args()
    if args.count <= 0:
        parser.error("count must be positive")
    if not 0 <= args.intersecting_fraction <= 1:
        parser.error("--intersecting-fraction must be between 0 and 1")

    if not (math.isfinite(args.minimum) and math.isfinite(args.maximum)
            and args.minimum < args.maximum):
        parser.error("coordinate bounds must be finite and --min must be less than --max")

    rng = random.Random(args.seed)
    if args.mode == "mixed":
        triangles = mixed_triangles(rng, args.count, args.minimum, args.maximum,
                                    args.intersecting_fraction)
    else:
        triangles = (triangle(rng, args.minimum, args.maximum)
                     for _ in range(args.count))

    with args.output.open("w", encoding="utf-8") as output:
        try:
            for points in triangles:
                output.write("3\n")
                for point in points:
                    output.write(" ".join(f"{coordinate:.17f}" for coordinate in point) + "\n")
                output.write("\n")
        except ValueError as error:
            parser.error(str(error))

    if args.mode == "mixed":
        intersecting = min(2 * round(args.count * args.intersecting_fraction / 2),
                           2 * (args.count // 2))
        print(f"Triangles: {args.count}; intersecting: {intersecting}; "
              f"non-intersecting: {args.count - intersecting}")


if __name__ == "__main__":
    main()
