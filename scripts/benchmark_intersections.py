#!/usr/bin/env python3
"""Measure preprocessing time versus the total number of triangles."""

import argparse
import csv
import html
import math
import random
import statistics
import subprocess
from pathlib import Path


STAGES = ("Parsing_ms", "World_ms", "Intersections_ms", "Scene_ms", "Total_ms")
COLORS = ("#2563eb", "#dc2626", "#059669", "#9333ea", "#d97706")


def generate_input(path, count, intersecting, seed):
    # Each slot contains two coplanar triangles. Active pairs overlap; all
    # other triangles occupy disjoint bounding boxes, including across slots.
    order = list(range(count // 2))
    random.Random(seed).shuffle(order)
    with path.open("w", encoding="utf-8") as output:
        for slot in order:
            x = slot * 8
            offsets = (0, 0.25 if slot < intersecting // 2 else 3)
            for offset in offsets:
                left = x + offset
                output.write(f"3\n{left} 0 0\n{left + 1} 0 0\n{left} 1 0\n\n")


def measure(binary, path, count, intersecting):
    result = subprocess.run(
        [str(binary), str(path), "--no-visualization"],
        check=True, capture_output=True, text=True,
    )
    values = dict(line.split(": ", 1) for line in result.stdout.splitlines()
                  if ": " in line)
    expected = {"Triangles": count, "Intersecting": intersecting,
                "Non-intersecting": count - intersecting}
    for key, value in expected.items():
        if int(values[key]) != value:
            raise RuntimeError(f"{key}: expected {value}, got {values[key]}")
    return {stage: float(values[stage]) for stage in STAGES}


def axis_scale(maximum, integer=False):
    """Choose readable, equally spaced ticks starting at zero."""
    raw_step = (maximum or 1) / 5
    magnitude = 10 ** math.floor(math.log10(raw_step))
    step = next(value * magnitude for value in (1, 2, 2.5, 5, 10)
                if value * magnitude >= raw_step)
    if integer:
        step = max(1, math.ceil(step))
    ticks = [index * step for index in range(math.ceil(maximum / step) + 1)]
    if len(ticks) == 1:
        ticks.append(step)
    return ticks[-1], ticks


def tick_label(value):
    return f"{value:,.9f}".rstrip("0").rstrip(".").replace(",", " ")


def write_plot(path, rows):
    # Standalone SVG keeps the benchmark usable without plotting dependencies.
    width, height = 1200, 850
    fractions = sorted({row["fraction"] for row in rows})
    parts = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" '
             f'height="{height}" viewBox="0 0 {width} {height}">',
             '<rect width="100%" height="100%" fill="white"/>',
             '<g font-family="sans-serif" font-size="13" fill="#111827">',
             '<text x="100" y="28" font-size="19">Время обработки в зависимости от числа треугольников</text>',
             '<text x="100" y="51">Медианы замеров; обе оси линейные, начало отсчёта — ноль</text>']
    for panel, stage in enumerate(("Intersections_ms", "Total_ms")):
        top, left, plot_width, plot_height = 110 + panel * 370, 120, 750, 250
        max_x, x_ticks = axis_scale(max(row["count"] for row in rows), integer=True)
        max_y, y_ticks = axis_scale(max(row[stage] for row in rows) * 1.05)
        title = ("Поиск пересечений" if stage == "Intersections_ms"
                 else "Вся подготовка до открытия окна")
        parts.append(f'<text x="{left}" y="{top - 20}" font-size="16">{title}</text>')
        for value in y_ticks:
            y = top + plot_height * (1 - value / max_y)
            parts.extend([
                f'<path d="M {left} {y} h {plot_width}" stroke="#e5e7eb"/>',
                f'<text x="{left - 10}" y="{y + 4}" text-anchor="end">{tick_label(value)}</text>',
            ])
        for value in x_ticks:
            x = left + value / max_x * plot_width
            parts.extend([
                f'<path d="M {x} {top} v {plot_height}" stroke="#e5e7eb"/>',
                f'<text x="{x}" y="{top + plot_height + 22}" text-anchor="middle">{tick_label(value)}</text>',
            ])
        parts.extend([
            f'<path d="M {left} {top} v {plot_height} h {plot_width}" fill="none" stroke="#111827"/>',
            f'<text x="{left + plot_width / 2}" y="{top + plot_height + 48}" text-anchor="middle">Общее количество треугольников N, шт.</text>',
            f'<text transform="translate(28 {top + plot_height / 2}) rotate(-90)" text-anchor="middle">Время t, мс</text>',
            f'<text x="900" y="{top}">Доля пересекающихся</text>',
            f'<text x="900" y="{top + 19}">треугольников:</text>',
        ])
        for index, fraction in enumerate(fractions):
            series = sorted((row for row in rows if row["fraction"] == fraction),
                            key=lambda row: row["count"])
            color = COLORS[index % len(COLORS)]
            points = [(left + row["count"] / max_x * plot_width,
                       top + plot_height * (1 - row[stage] / max_y))
                      for row in series]
            coordinates = " ".join(f"{x},{y}" for x, y in points)
            parts.append(f'<polyline points="{coordinates}" fill="none" stroke="{color}" stroke-width="2"/>')
            for x, y in points:
                parts.append(f'<circle cx="{x}" cy="{y}" r="3" fill="{color}"/>')
            label = html.escape(f"{fraction * 100:g}%")
            parts.append(f'<text x="920" y="{top + 45 + index * 23}" fill="{color}">{label}</text>')
    parts.append('</g></svg>')
    path.write_text("\n".join(parts), encoding="utf-8")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, default=Path("build/profile/triangles"))
    parser.add_argument("--counts", type=int, nargs="+", default=[100, 500, 1000, 2500, 5000])
    parser.add_argument("--fractions", type=float, nargs="+", default=[0, 0.25, 0.5, 0.75, 1])
    parser.add_argument("--repeats", type=int, default=3)
    parser.add_argument("--warmups", type=int, default=1)
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--output", type=Path, default=Path("build/benchmark-intersections"))
    parser.add_argument("--plot-from", type=Path,
                        help="redraw a medians.csv file without running the benchmark")
    args = parser.parse_args()
    if any(count < 2 or count % 2 for count in args.counts):
        parser.error("counts must be even and at least 2")
    if any(not 0 <= fraction <= 1 for fraction in args.fractions):
        parser.error("fractions must be between 0 and 1")
    if args.repeats < 1 or args.warmups < 0:
        parser.error("repeats must be positive; warmups must be nonnegative")
    if args.plot_from:
        with args.plot_from.open(newline="", encoding="utf-8") as source:
            rows = []
            for row in csv.DictReader(source):
                count, intersecting = int(row["count"]), int(row["intersecting"])
                fraction = (float(row["fraction"]) if "fraction" in row else
                            min(args.fractions, key=lambda value:
                                abs(value - intersecting / count)))
                if 2 * round(count * fraction / 2) != intersecting:
                    parser.error("old CSV fractions differ; specify the original --fractions")
                rows.append({"count": count, "intersecting": intersecting,
                             "fraction": fraction,
                             **{stage: float(row[stage]) for stage in STAGES}})
        if not rows:
            parser.error("the CSV contains no measurements")
        args.output.mkdir(parents=True, exist_ok=True)
        write_plot(args.output / "timings.svg", rows)
        print(f"Plot: {(args.output / 'timings.svg').resolve()}")
        return
    binary = args.binary.resolve(strict=True)
    args.output.mkdir(parents=True, exist_ok=True)
    fields = ["count", "intersecting", "fraction", "repeat", *STAGES]
    medians = []
    with (args.output / "samples.csv").open("w", newline="", encoding="utf-8") as output:
        writer = csv.DictWriter(output, fieldnames=fields)
        writer.writeheader()
        output.flush()
        for count in sorted(set(args.counts)):
            for fraction in sorted(set(args.fractions)):
                intersecting = 2 * round(count * fraction / 2)
                print(f"N={count}, intersecting={intersecting}, "
                      f"pair checks/run={count * (count - 1) // 2:,}", flush=True)
                path = args.output / "input.txt"
                generate_input(path, count, intersecting, args.seed)
                samples = []
                for iteration in range(args.warmups + args.repeats):
                    timings = measure(binary, path, count, intersecting)
                    if iteration < args.warmups:
                        continue
                    samples.append(timings)
                    writer.writerow({"count": count, "intersecting": intersecting,
                                     "fraction": fraction,
                                     "repeat": iteration - args.warmups + 1, **timings})
                    output.flush()
                    print(f"  intersections={timings['Intersections_ms']:.3f} ms, "
                          f"total={timings['Total_ms']:.3f} ms", flush=True)
                medians.append({"count": count, "intersecting": intersecting,
                                "fraction": fraction,
                                **{stage: statistics.median(sample[stage] for sample in samples)
                                   for stage in STAGES}})
                # Persist completed cases even if a later, larger run is interrupted.
                with (args.output / "medians.csv").open("w", newline="", encoding="utf-8") as summary:
                    summary_writer = csv.DictWriter(summary, fieldnames=["count", "intersecting", "fraction", *STAGES])
                    summary_writer.writeheader()
                    summary_writer.writerows(medians)
                write_plot(args.output / "timings.svg", medians)
    print(f"Results: {args.output.resolve()}")


if __name__ == "__main__":
    main()
