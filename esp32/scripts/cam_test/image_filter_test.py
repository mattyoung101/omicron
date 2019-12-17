from PIL import Image, ImageDraw
import math
from numba import njit

palette = {
    "yellow": [195, 153, 41],
    "green": [0, 255, 0],
    "white": [255, 255, 255],
    "orange": [255, 153, 0],
    "black": [0, 0, 0],
    "blue": [0, 170, 255]
}

# https://en.wikipedia.org/wiki/Color_difference#Euclidean
@njit(fastmath=True)
def dist(colour1, colour2):
    r1, g1, b1 = colour1
    r2, g2, b2 = colour2
    return math.sqrt((r2 - r1) ** 2 + (g2 - g1) ** 2 + (b2 - b1) ** 2)

# TODO parallelise this with numba
if __name__ == "__main__":
    im = Image.open("field.png")
    draw = ImageDraw.Draw(im)
    width, height = im.size

    for y in range(height):
        for x in range(width):
            colour = im.getpixel((x, y))
            sorted_palette = sorted(palette.items(), key=lambda x: dist(list(colour)[:-1], x[1])) # may need [:-1] if RGBA
            r, g, b = sorted_palette[0][1]
            draw.point((x, y), (r, g, b))

    del draw
    im.save("out.png", "PNG")