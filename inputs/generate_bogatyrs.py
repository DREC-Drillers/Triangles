
#!/usr/bin/env python3
"""Convert Vasnetsov's Bogatyrs into a triangular relief for Triangles."""

import argparse
import math
from pathlib import Path

from PIL import Image, ImageOps

# Public-domain reproduction from Google Arts & Culture via Wikimedia Commons.
# https://commons.wikimedia.org/wiki/File:Viktor_Vasnetsov_-_Богатыри_-_Google_Art_Project.jpg
PAINTING = Path(__file__).with_name("vasnetsov_bogatyrs.jpg")
SCENE_WIDTH = 24.0


def load_image(source):
    with Image.open(source) as image:
        return ImageOps.exif_transpose(image).convert("RGB")


def relief_triangles(image, columns, rows, depth, extra_splits):
    """Sample the image onto a shared grid and split every cell into two faces."""
    tones = ImageOps.autocontrast(
        image.convert("L").resize((columns, rows), Image.Resampling.LANCZOS), cutoff=1
    )
    pixels = tones.load()
    step = SCENE_WIDTH / (columns - 1)
    scene_height = SCENE_WIDTH * image.height / image.width
    x_coordinates = [(column - (columns - 1) / 2) * step
                     for column in range(columns)]

    def vertex_row(row):
        y = (0.5 - row / (rows - 1)) * scene_height
        return [(x, y, (pixels[column, row] / 255 - 0.5) * depth)
                for column, x in enumerate(x_coordinates)]

    cells = (columns - 1) * (rows - 1)
    split_progress = 0
    top = vertex_row(0)
    for row in range(1, rows):
        bottom = vertex_row(row)
        for column in range(columns - 1):
            top_left, top_right = top[column:column + 2]
            bottom_left, bottom_right = bottom[column:column + 2]

            # Use the diagonal with the smaller height jump to follow edges.
            if abs(top_left[2] - bottom_right[2]) <= abs(top_right[2] - bottom_left[2]):
                first = (top_left, bottom_left, bottom_right)
                second = (top_left, bottom_right, top_right)
            else:
                first = (top_left, bottom_left, top_right)
                second = (top_right, bottom_left, bottom_right)

            # Split selected faces internally so neighboring edges stay intact.
            split_progress += extra_splits
            if split_progress >= cells:
                split_progress -= cells
                a, b, c = first
                center = tuple(sum(axis) / 3 for axis in zip(a, b, c))
                yield a, b, center
                yield b, c, center
                yield c, a, center
            else:
                yield first
            yield second
        top = bottom


def write_scene(path, image, columns, rows, depth, extra_splits):
    # Sample the quantized image at each triangle's centroid. The sidecar stores
    # 256 RGB entries followed by one byte per face in geometry-file order.
    samples = image.resize((2 * columns, 2 * rows), Image.Resampling.LANCZOS)
    indexed = samples.quantize(colors=256, dither=Image.Dither.NONE)
    palette = bytes(indexed.getpalette()).ljust(256 * 3, b"\0")
    pixels = indexed.load()
    width, height = indexed.size
    scene_height = SCENE_WIDTH * image.height / image.width
    indices = bytearray()
    triangles = relief_triangles(image, columns, rows, depth, extra_splits)

    with path.open("w", encoding="utf-8", newline="\n") as output:
        for triangle in triangles:
            output.write("3\n")
            for x, y, z in triangle:
                output.write(f"{x:.12f} {y:.12f} {z:.12f}\n")
            output.write("\n")
            x, y, _ = (sum(axis) / 3 for axis in zip(*triangle))
            column = int((0.5 + x / SCENE_WIDTH) * width)
            row = int((0.5 - y / scene_height) * height)
            indices.append(pixels[column, row])

    color_path = path.with_name(path.name + ".colors")
    color_path.write_bytes(palette + indices)
    return len(indices)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path, help="output file in Triangles format")
    parser.add_argument("--image", type=Path, default=PAINTING,
                        help="use a different image as the relief source")
    resolution = parser.add_mutually_exclusive_group()
    resolution.add_argument("--columns", type=int,
                            help="mesh vertices across the painting (default: 640)")
    resolution.add_argument("--triangles", type=int, help="exact even triangle count")
    parser.add_argument("--depth", type=float, default=0.9,
                        help="maximum relief height difference (default: 0.9)")
    args = parser.parse_args()

    if args.columns is not None and args.columns < 2:
        parser.error("--columns must be at least 2")
    if args.triangles is not None and (args.triangles < 2 or args.triangles % 2):
        parser.error("--triangles must be a positive even number")
    if not math.isfinite(args.depth) or args.depth <= 0:
        parser.error("--depth must be positive and finite")

    try:
        image = load_image(args.image)
        ratio = image.height / image.width
        extra_splits = 0
        if args.triangles is None:
            columns = args.columns or 640
            rows = max(2, round((columns - 1) * ratio) + 1)
        else:
            cells = args.triangles // 2
            across = min(cells, max(1, round(math.sqrt(cells / ratio))))
            down, extra_splits = divmod(cells, across)
            columns, rows = across + 1, down + 1
        count = write_scene(args.output, image, columns, rows, args.depth, extra_splits)
    except (OSError, ValueError) as error:
        parser.exit(1, f"{parser.prog}: {error}\n")

    print(f"Triangles: {count}; output: {args.output}")


if __name__ == "__main__":
    main()
