import numpy as np
from numba import njit
import pygame
from PIL import Image, ImageDraw

# Given the position of the robot on the field, renders a contour plot of the most likely place the robot is
# based on virtual sensors. The approach is described more on a card on our Trello.

# in cm
FIELD_WIDTH = 243
FIELD_HEIGHT = 182
FIELD_CENTRE = (FIELD_WIDTH / 2, FIELD_HEIGHT / 2)

FIELD = [
    (0 ,0), (FIELD_WIDTH, 0), # bottom
    (0, FIELD_HEIGHT), (FIELD_WIDTH, FIELD_HEIGHT), # top
    (0, 0), (0, FIELD_HEIGHT), # left
    (FIELD_WIDTH, 0), (FIELD_WIDTH, FIELD_HEIGHT) # right
]

ROBOT_POS = (60, 60)

# LRFs are at 45, 135, 225 and 315 degrees (if the circle has 0 pointing directly up and 180 pointing directly down)

# Source: https://stackoverflow.com/a/42727584/5007892
@njit(fastmath=True)
def get_intersect(a1, a2, b1, b2):
    """ 
    Returns the point of intersection of the lines passing through a2,a1 and b2,b1.
    a1: [x, y] a point on the first line
    a2: [x, y] another point on the first line
    b1: [x, y] a point on the second line
    b2: [x, y] another point on the second line
    """
    s = np.vstack([a1,a2,b1,b2])        # s for stacked
    h = np.hstack((s, np.ones((4, 1)))) # h for homogeneous
    l1 = np.cross(h[0], h[1])           # get first line
    l2 = np.cross(h[2], h[3])           # get second line
    x, y, z = np.cross(l1, l2)          # point of intersection
    if z == 0:                          # lines are parallel
        return (float('inf'), float('inf'))
    return (x/z, y/z)

if __name__ == "__main__":
    img = Image.new("RGB", (FIELD_WIDTH, FIELD_HEIGHT))
    draw = ImageDraw.Draw(img)
    draw.polygon(FIELD)
    draw.point(FIELD_CENTRE, fill=(255, 0, 0))
    draw.point(ROBOT_POS, fill=(255, 0, 0))
    img.show()