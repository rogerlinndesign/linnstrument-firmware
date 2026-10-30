#!/usr/bin/env python3
"""Draws the name the front panel prints at the right end of the instrument, as a white on
transparent image the app tints and turns: Resources/Nameplate/Nameplate.png, the name, a rule
under it and the tagline, all as wide as the name, in the proportions measured off Roger Linn
Design's photo of the panel; and Name.png, the name alone, for an app's toolbar.

The panel prints them in Eurostile Bold Extended. Of the Eurostile cuts at hand, Eurostile Next LT
Pro Bold, stretched to the panel's proportions with its slightly tight spacing, fits the panel's
letters best. The font only draws these images: it is not in this repository and the app does not
ship it.

    tools/virtual/make_nameplate.py --font EUROSTILE.otf
"""
import argparse
import os

import numpy as np
from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "Resources", "Nameplate")
TAGLINE = "Music Performance Keyboard with 3D Note Expression"
# the name's height in the images, from the top of its L to its baseline, and everything else in
# fractions of it, as on the panel: "LinnStrument" is 12.67 times as wide as high, its letters
# 0.01 of the type's size closer than the font sets them; the rule 0.22 below the baseline and
# 0.074 thick; the tagline 0.185 below the rule with capitals 0.22 high
HEIGHT = 160
RATIO = 12.67
TRACKING = -0.01
RULE_GAP = 0.22
RULE = 0.074
TAGLINE_GAP = 0.185
TAGLINE_CAPS = 0.22


def ink(font, text, tracking):
    """The text set a letter at a time with the given tracking, cropped to its ink, with the
    height its capitals' top to its baseline take within the crop."""
    size = font.size
    canvas = Image.new("L", (int(sum(font.getlength(c) for c in text) + 2 * size), size * 2), 0)
    draw = ImageDraw.Draw(canvas)
    x = size / 2
    for letter in text:
        draw.text((x, size / 2), letter, font=font, fill=255)
        x += font.getlength(letter) + tracking * size
    a = np.asarray(canvas)
    ys = np.where(a.max(1) > 0)[0]
    xs = np.where(a.max(0) > 0)[0]
    # the baseline: the bottom of a letter without a descender, the first one
    _, _, _, baseline = draw.textbbox((size / 2, size / 2), text[0], font=font)
    return canvas.crop((xs.min(), ys.min(), xs.max() + 1, ys.max() + 1)), baseline - ys.min()


def white(mask):
    image = Image.new("RGBA", mask.size, (255, 255, 255, 0))
    image.putalpha(mask)
    return image


def name(font_path):
    text, text_height = ink(ImageFont.truetype(font_path, 400), "LinnStrument", TRACKING)
    scale = HEIGHT / text_height
    return text.resize((int(round(RATIO * HEIGHT)), int(round(text.height * scale))), Image.LANCZOS)


def nameplate(font_path):
    text = name(font_path)
    width = text.width

    tag, tag_height = ink(ImageFont.truetype(font_path, 400), TAGLINE, 0)
    tag_scale = TAGLINE_CAPS * HEIGHT / tag_height
    tag = tag.resize((width, int(round(tag.height * tag_scale))), Image.LANCZOS)

    rule_top = HEIGHT + RULE_GAP * HEIGHT
    rule_bottom = rule_top + RULE * HEIGHT
    tag_top = rule_bottom + TAGLINE_GAP * HEIGHT
    total = int(np.ceil(tag_top + tag.height)) + 2
    plate = Image.new("L", (width, total), 0)
    plate.paste(text, (0, 0))
    ImageDraw.Draw(plate).rectangle((0, int(round(rule_top)), width - 1, int(round(rule_bottom)) - 1), fill=255)
    plate.paste(tag, (0, int(round(tag_top))))
    return white(plate)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--font", required=True, help="Eurostile Next LT Pro Bold")
    args = parser.parse_args()
    os.makedirs(OUT, exist_ok=True)
    for file, image in (("Nameplate.png", nameplate(args.font)), ("Name.png", white(name(args.font)))):
        path = os.path.join(OUT, file)
        image.save(path, optimize=True)
        print(f"wrote {path}")


if __name__ == "__main__":
    main()
