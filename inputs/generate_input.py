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
        if sum(x * x for x in cross(u, v)) > 1e-12:
            return points
    raise ValueError("Range is too small to generate a valid triangle at 6 decimal places")


def cross(u, v):
    return (u[1] * v[2] - u[2] * v[1],
            u[2] * v[0] - u[0] * v[2],
            u[0] * v[1] - u[1] * v[0])


def random_frame(rng):
    # Uniformly distributed axis on the sphere and two perpendicular unit vectors.
    height = rng.uniform(-1, 1)
    azimuth = rng.uniform(0, math.tau)
    radial = math.sqrt(1 - height * height)
    cosine, sine = math.cos(azimuth), math.sin(azimuth)
    axis = (radial * cosine, radial * sine, height)
    tangent = (-sine, cosine, 0)
    return axis, tangent, cross(axis, tangent)


def triangle_around(rng, center, axis, tangent, radius):
    phase = rng.uniform(0, math.tau)
    points = []
    for index in range(3):
        angle = phase + index * math.tau / 3 + rng.uniform(-0.3, 0.3)
        distance = radius * rng.uniform(0.35, 1)
        along = distance * math.cos(angle)
        across = distance * math.sin(angle)
        points.append(tuple(c + along * u + across * v
                            for c, u, v in zip(center, axis, tangent)))
    # Consecutive rays are less than pi apart, so center is strictly inside.
    rng.shuffle(points)
    return points


def crossing_group(rng, origin, step, size):
    center = tuple(value + step * rng.uniform(0.42, 0.58) for value in origin)
    axis, tangent, bitangent = random_frame(rng)
    phase = rng.uniform(0, math.pi)
    spacing = math.pi / size
    for index in range(size):
        # Each plane contains the same line, with well-separated angles modulo pi.
        tilt = phase + spacing * (index + rng.uniform(-0.15, 0.15))
        cosine, sine = math.cos(tilt), math.sin(tilt)
        direction = tuple(v * cosine + w * sine for v, w in zip(tangent, bitangent))
        yield triangle_around(rng, center, axis, direction,
                              step * rng.uniform(0.14, 0.35))


def separated_group(rng, origin, step, size):
    # The two bounding boxes have a gap on x regardless of rotation.
    for slot in (0.25, 0.75)[:size]:
        center = (origin[0] + step * (slot + rng.uniform(-0.04, 0.04)),
                  origin[1] + step * rng.uniform(0.24, 0.76),
                  origin[2] + step * rng.uniform(0.24, 0.76))
        axis, tangent, _ = random_frame(rng)
        yield triangle_around(rng, center, axis, tangent,
                              step * rng.uniform(0.08, 0.19))


def intersecting_count(count, fraction):
    return min(2 * round(count * fraction / 2), 2 * (count // 2))


def mixed_triangles(rng, count, minimum, maximum, intersecting_fraction):
    cells = (count + 1) // 2
    side = math.ceil(cells ** (1 / 3))
    while side ** 3 < cells:
        side += 1
    step = (maximum - minimum) / side
    # Keep a generous margin so floating-point rounding cannot collapse a group.
    if not math.isfinite(step) or step < 1024 * math.ulp(max(abs(minimum), abs(maximum))):
        raise ValueError("Coordinate range is too small for the requested count")

    active = intersecting_count(count, intersecting_fraction)
    inactive = count - active
    order = list(range(cells))
    rng.shuffle(order)

    for cell in order:
        if active + inactive == 0:
            break

        origin = (minimum + (cell % side) * step,
                  minimum + ((cell // side) % side) * step,
                  minimum + (cell // (side * side)) * step)
        if rng.randrange(active + inactive) < active:
            size = rng.randint(2, min(5, active))
            if active - size == 1:
                # Leave either nothing or enough triangles for another crossing.
                size = active if size == 2 else size - 1
            yield from crossing_group(rng, origin, step, size)
            active -= size
        else:
            size = min(2, inactive)
            yield from separated_group(rng, origin, step, size)
            inactive -= size


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("count", type=int, help="number of triangles")
    parser.add_argument("output", type=Path, help="output file")
    parser.add_argument("--seed", type=int, help="seed for reproducible inputs")
    parser.add_argument("--mode", choices=("mixed", "random"), default="mixed",
                        help="mixed: crossing 3D groups with a controlled intersection ratio (default)")
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
        intersecting = intersecting_count(args.count, args.intersecting_fraction)
        print(f"Triangles: {args.count}; intersecting: {intersecting}; "
              f"non-intersecting: {args.count - intersecting}")


if __name__ == "__main__":
    main()
