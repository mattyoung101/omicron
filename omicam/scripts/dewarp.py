from PIL import Image
from math import pow, cos, sin, degrees, radians, fmod, sqrt, atan2
import numpy
import cmath
from scipy.spatial import distance
from scipy.interpolate import NearestNDInterpolator

SCALAR = 4
TEST_RADIUS = 4

def linearise(measurement):
    return 0.0003 * pow(measurement, 2.8206)

if __name__ == "__main__":
    in_img = Image.open("frame.png")
    width, height = in_img.size
    out_img = Image.new("RGB", (width * SCALAR, height * SCALAR))
    out_width, out_height = out_img.size

    points = []
    values = []

    print("Dewarping...")
    for y in range(height):
        for x in range(width):
            col_r, col_g, col_b = in_img.getpixel((x, y))

            vec = complex(x - (width / 2), y - (height / 2)) # make a vector (complex number in this case), from the centre
            r, phi = cmath.polar(vec) # convert to polar so we can scale the magnitude
            r = linearise(r) # linearise the magnitude
            cart = cmath.rect(r, phi) # convert back to cartesian to be drawn
            cart = complex(cart.real + (out_width / 2), cart.imag + (out_height / 2)) # add back half the image width to get back to the centre
            
            points.append([cart.real, cart.imag])
            values.append([col_r, col_g, col_b])

            out_img.putpixel((round(cart.real), round(cart.imag)), (col_r, col_g, col_b))

    visited = 0
    last_progress = 0
    interpolator = NearestNDInterpolator(numpy.array(points), numpy.array(values))

    print("Interpolating...")
    for y in range(out_height):
        for x in range(out_width):
            r, g, b = out_img.getpixel((x, y))
            coord = (x, y)
            
            # get interpolated colour for this coordinate
            i_r, i_g, i_b = interpolator(x, y)
            out_img.putpixel((x, y), (i_r, i_g, i_b))

            visited += 1
            complete = round((visited / (out_width * out_height) * 100))
            if complete != last_progress:
                print(f"{complete}% complete")
                last_progress = complete

    out_img.save("out.png", "PNG")
    out_img.show()