#!/usr/bin/env python3
"""Draws the name the front panel prints at the right end of the instrument, as white on
transparent images the app tints and turns, for each model: Resources/Nameplate/Nameplate128.png
and Nameplate200.png, the name, the rule under it and the tagline; and Name128.png and Name200.png,
the name alone, for an app's toolbar.

They come from Roger Linn Design's outlines of the panel's logos, a PDF per model, with the logo
white on white and sideways at the page's right end, which poppler's pdftocairo renders. The PDFs
only draw these images and are not in this repository.

    tools/virtual/make_nameplate.py LOGO_128.pdf LOGO_200.pdf
"""
import argparse
import os
import subprocess
import tempfile

import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "Resources", "Nameplate")
WIDTH = 2048


def logo(pdf):
    """The logo upright, white with the outlines' coverage as alpha, cropped to its ink."""
    with tempfile.TemporaryDirectory() as folder:
        base = os.path.join(folder, "logo")
        subprocess.run(["pdftocairo", "-png", "-transp", "-r", "720", "-singlefile", pdf, base], check=True)
        alpha = Image.open(base + ".png").getchannel("A")
    alpha = alpha.crop(alpha.getbbox()).rotate(-90, expand=True)
    image = Image.new("RGBA", alpha.size, (255, 255, 255, 0))
    image.putalpha(alpha)
    return image


def name(plate):
    """The name, above the rule: the first rows that are ink all the way across."""
    alpha = np.asarray(plate.getchannel("A"))
    rule = int(np.where((alpha > 128).mean(1) > 0.97)[0][0])
    above = plate.crop((0, 0, plate.width, rule - 1))
    return above.crop(above.getbbox())


def scaled(image):
    return image.resize((WIDTH, round(image.height * WIDTH / image.width)), Image.LANCZOS)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("logo128", help="the LinnStrument 128's logo outlines")
    parser.add_argument("logo200", help="the LinnStrument 200's logo outlines")
    args = parser.parse_args()
    os.makedirs(OUT, exist_ok=True)
    for model, pdf in (("128", args.logo128), ("200", args.logo200)):
        plate = logo(pdf)
        for file, image in ((f"Nameplate{model}.png", plate), (f"Name{model}.png", name(plate))):
            path = os.path.join(OUT, file)
            scaled(image).save(path, optimize=True)
            print(f"wrote {path}")


if __name__ == "__main__":
    main()
