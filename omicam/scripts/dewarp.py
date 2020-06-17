from PIL import Image
from math import exp, floor
import numpy
import cmath
from scipy.spatial import distance
from scipy.interpolate import NearestNDInterpolator
import sys

SCALAR = 4
MIRROR_RADIUS = 384

def linearise(x):
    """Calculated mirror model, from omicam.ini"""
    return 2.5874667517468426 * exp(0.012010445463214335 * x)

def lerp(fromValue, toValue, progress):
    return fromValue + (toValue - fromValue) * progress

if __name__ == "__main__":
    in_img = Image.open("frame2.jpg")
    width, height = in_img.size
    out_img = Image.new("RGB", (MIRROR_RADIUS * 2 + 32, MIRROR_RADIUS * 2 + 32))
    out_width, out_height = out_img.size
    out_mag_range_px = distance.euclidean((0, 0), (out_width, out_height))
    print(f"Output size: {out_width}x{out_height}, mag range pixels {out_mag_range_px}")

    points = []
    values = []
    largest_mag = -99999
    smallest_mag = 99999
    largest_pos = [0, 0]
    smallest_pos = [0, 0]

    # First, go through and find largest and smallest dewarp model magnitudes
    # FIXME won't smallest always be at (0,0) and largest at mirror bounds?
    print("Finding magnitude range...")
    for y in range(height):
        for x in range(width):
            # convert pixel to polar
            vec = complex(x - (width / 2), y - (height / 2))
            r, phi = cmath.polar(vec)
            # if we're outside the mirror (before dewarp, as it's measured in pixels), skip
            if r > MIRROR_RADIUS:
                continue
            r = linearise(r)

            # store largest and smallest vals
            if r > largest_mag:
                largest_mag = r
                largest_pos = [x, y]
            elif r < smallest_mag:
                smallest_mag = r
                smallest_pos = [x, y]

    print(f"Largest mag: {largest_mag} at coordinate {largest_pos[0]},{largest_pos[1]}")
    print(f"Smallest mag: {smallest_mag} at coordinate {smallest_pos[0]},{smallest_pos[1]}")  

    print("Dewarping...")
    for y in range(height):
        for x in range(width):
            col_r, col_g, col_b = in_img.getpixel((x, y))

            # make a vector (complex number in this case), from the centre
            vec = complex(x - (width / 2), y - (height / 2))
            # convert to polar so we can scale the magnitude
            r, phi = cmath.polar(vec)
            # if we're outside the mirror, skip
            if r > MIRROR_RADIUS:
                continue
            # linearise the magnitude
            r = linearise(r)
            # scale magnitude to be in range of output image
            r_scaled = ((r - smallest_mag) / (largest_mag - smallest_mag)) * out_mag_range_px

            # convert back to cartesian to be drawn
            cart = cmath.rect(r_scaled, phi) 
            # add back half the image width to get back to the centre
            cart = complex(cart.real + (out_width / 2), cart.imag + (out_height / 2)) 
            
            points.append([cart.real, cart.imag])
            values.append([col_r, col_g, col_b])

            try:
                out_img.putpixel((floor(cart.real), floor(cart.imag)), (col_r, col_g, col_b))
            except IndexError:
                # TODO WHY THE FUCK DOES THIS SHIT KEEP GOING NEGATIVE, WRITE IT TO ANOTHER FILE?????
                #print(f"Fucked up, trying to write to: {cart.real}, {cart.imag}: r is {r}, scaled r: {r_scaled}, angle: {phi}")
                #sys.exit(0)
                continue

    visited = 0
    last_progress = 0
    interpolator = NearestNDInterpolator(numpy.array(points), numpy.array(values))
    # FIXME only interpolate if empty or black pixel in original!!!

    # print("Interpolating...")
    # for y in range(out_height):
    #     for x in range(out_width):
    #         r, g, b = out_img.getpixel((x, y))
    #         coord = (x, y)
            
    #         # get interpolated colour for this coordinate
    #         i_r, i_g, i_b = interpolator(x, y)
    #         out_img.putpixel((x, y), (i_r, i_g, i_b))

    #         visited += 1
    #         complete = round((visited / (out_width * out_height) * 100))
    #         if complete != last_progress:
    #             print(f"{complete}% complete")
    #             last_progress = complete

    out_img.save("out2.png", "PNG")
    out_img.show()