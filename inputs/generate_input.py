#!/usr/bin/env python3
"""Generate a triangle input file; coordinates are uniformly sampled in a configurable range."""

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


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("count", type=int, help="number of triangles")
    parser.add_argument("output", type=Path, help="output file")
    parser.add_argument("--seed", type=int, help="seed for reproducible inputs")
    parser.add_argument("--min", dest="minimum", type=float, default=-10,
                        help="minimum coordinate (default: -10)")
    parser.add_argument("--max", dest="maximum", type=float, default=10,
                        help="maximum coordinate (default: 10)")
    args = parser.parse_args()
    if args.count <= 0:
        parser.error("count must be positive")

    if not (math.isfinite(args.minimum) and math.isfinite(args.maximum)
            and args.minimum < args.maximum):
        parser.error("coordinate bounds must be finite and --min must be less than --max")

    rng = random.Random(args.seed)
    with args.output.open("w", encoding="utf-8") as output:
        for _ in range(args.count):
            output.write("3\n")
            try:
                points = triangle(rng, args.minimum, args.maximum)
            except ValueError as error:
                parser.error(str(error))
            for point in points:
                output.write(" ".join(f"{coordinate:.6f}" for coordinate in point) + "\n")
            output.write("\n")


if __name__ == "__main__":
    main()
