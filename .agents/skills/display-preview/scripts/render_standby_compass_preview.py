#!/usr/bin/env python3
from pathlib import Path
import argparse
import re
import subprocess


ROOT = Path(__file__).resolve().parents[4]
MASK_PATH = ROOT / "src/standby_compass/tape_masks.c"
DEFAULT_OUTPUT = ROOT / "previews/standby_compass/current.png"

DISPLAY_WIDTH = 172
DISPLAY_HEIGHT = 320
DISPLAY_CENTER_Y = DISPLAY_HEIGHT // 2
TAPE_BAND_X = 56
TAPE_BAND_WIDTH = 84
TAPE_WIDTH = TAPE_BAND_WIDTH
COMPASS_CYCLE_HEIGHT = 1120
TAPE_HEIGHT = 1440
NORTH_Y = 561

BACKGROUND = (16, 16, 16)
MARK = (255, 255, 255)
LUBBER = (220, 220, 220)

TRAPEZOID_TOP_INSET_PX = 8
PERSPECTIVE_TOP_PULL_PX = 12

TICK_START_X = 13
TICK_FIVE_DEGREE_END_X = 22
TICK_END_X = 28
TICK_THIN_HALF_WIDTH = 1
TICK_THICK_HALF_WIDTH = 2


def load_label_mask():
    text = MASK_PATH.read_text()
    bytes_ = [int(value, 16) for value in re.findall(r"0x([0-9a-fA-F]{2})", text)]
    pixels = []
    for byte in bytes_:
        pixels.append(byte >> 4)
        pixels.append(byte & 0x0F)
    return pixels[: TAPE_WIDTH * TAPE_HEIGHT]


def label_alpha(mask, x, y):
    if x < 0 or x >= TAPE_WIDTH:
        return 0
    return mask[(y % COMPASS_CYCLE_HEIGHT) * TAPE_WIDTH + x]


def lightened_label_alpha(mask, x, y):
    alpha = label_alpha(mask, x, y)
    if alpha == 0:
        return 0

    edge_pixel = (
        label_alpha(mask, x - 1, y) == 0
        or label_alpha(mask, x + 1, y) == 0
        or label_alpha(mask, x, y - 1) == 0
        or label_alpha(mask, x, y + 1) == 0
    )
    return 9 if edge_pixel else alpha


def edge_factor(display_y):
    distance = abs(display_y - DISPLAY_CENTER_Y)
    return distance * distance * 1000 // (DISPLAY_CENTER_Y * DISPLAY_CENTER_Y)


def perspective_heading_offset(display_y, source_x):
    top_weight = (TAPE_WIDTH - 1 - source_x) * 1000 // (TAPE_WIDTH - 1)
    return (DISPLAY_CENTER_Y - display_y) * PERSPECTIVE_TOP_PULL_PX * top_weight // (DISPLAY_CENTER_Y * 1000)


def tick_alpha(source_x, source_y):
    position = (source_y - NORTH_Y) % COMPASS_CYCLE_HEIGHT
    tick_index = (position * 72 + (COMPASS_CYCLE_HEIGHT // 2)) // COMPASS_CYCLE_HEIGHT
    tick_index %= 72
    tick_y = tick_index * COMPASS_CYCLE_HEIGHT // 72
    distance = abs(position - tick_y)
    if distance > COMPASS_CYCLE_HEIGHT // 2:
        distance = COMPASS_CYCLE_HEIGHT - distance

    labeled_tick = (tick_index % 6) == 0
    ten_degree_tick = (tick_index % 2) == 0
    tick_end_x = TICK_END_X if ten_degree_tick else TICK_FIVE_DEGREE_END_X
    half_width = TICK_THICK_HALF_WIDTH if labeled_tick else TICK_THIN_HALF_WIDTH

    if source_x < TICK_START_X or source_x > tick_end_x or distance > half_width:
        return 0
    if distance == half_width and half_width > 1:
        return 10
    return 15


def blend(alpha):
    if alpha <= 0:
        return BACKGROUND
    if alpha >= 15:
        return MARK
    return tuple((BACKGROUND[i] * (15 - alpha) + MARK[i] * alpha) // 15 for i in range(3))


def center_y_for_heading_centi(heading_centi):
    normalized = heading_centi % 36000
    distance_from_north = ((36000 - normalized) % 36000) * COMPASS_CYCLE_HEIGHT // 36000
    return NORTH_Y + distance_from_north


def render_ppm(heading_centi, ppm_path):
    mask = load_label_mask()
    top_y = center_y_for_heading_centi(heading_centi) - DISPLAY_CENTER_Y
    image = [[BACKGROUND for _ in range(DISPLAY_WIDTH)] for _ in range(DISPLAY_HEIGHT)]

    for y in range(DISPLAY_HEIGHT):
        top_inset = TRAPEZOID_TOP_INSET_PX * edge_factor(y) // 1000
        projected_height = TAPE_BAND_WIDTH - top_inset
        for x in range(TAPE_BAND_WIDTH):
            if x < top_inset:
                pixel = BACKGROUND
            else:
                source_x = (x - top_inset) * TAPE_WIDTH // projected_height
                source_y = top_y + y + perspective_heading_offset(y, source_x)
                label_alpha_value = lightened_label_alpha(mask, source_x, source_y)
                pixel = blend(max(label_alpha_value, tick_alpha(source_x, source_y)))
            image[y][TAPE_BAND_X + x] = pixel

    for y in range(DISPLAY_CENTER_Y - 1, DISPLAY_CENTER_Y + 2):
        for x in range(DISPLAY_WIDTH):
            image[y][x] = LUBBER

    with ppm_path.open("wb") as file:
        file.write(f"P6\n{DISPLAY_WIDTH} {DISPLAY_HEIGHT}\n255\n".encode())
        for row in image:
            for pixel in row:
                file.write(bytes(pixel))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--heading-centi", type=int, default=0)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--rotate-ccw", action="store_true", help="Rotate preview counter-clockwise into the current standby compass gauge mounting orientation")
    args = parser.parse_args()

    args.output.parent.mkdir(parents=True, exist_ok=True)
    ppm_path = args.output.with_suffix(".ppm")
    png_unrotated = args.output.with_name(args.output.stem + "-raw.png")
    render_ppm(args.heading_centi, ppm_path)

    subprocess.run(["magick", str(ppm_path), str(png_unrotated)], check=True)
    if args.rotate_ccw:
        subprocess.run(["magick", str(png_unrotated), "-rotate", "-90", str(args.output)], check=True)
    else:
        png_unrotated.replace(args.output)

    ppm_path.unlink(missing_ok=True)
    print(args.output)


if __name__ == "__main__":
    main()
