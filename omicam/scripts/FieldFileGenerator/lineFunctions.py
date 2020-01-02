"""
Python script which contains functions for defining and calculating the shortest distance to a given function.

Written by Ethan Lo, so don't expect this to work too well :)

General Notes:
* ALL ANGLES ARE IN RADIANS, STARTING FROM THE POSITIVE X-AXIS
* Angles should go from 0 -> 360
* Units are cm
"""

from math import sqrt, pow, atan2, cos, sin

def euclideanDist(startX, startY, endX, endY): # just pythag
    return sqrt(pow(startX - endX, 2) + pow(startY - endY, 2))

# https://stackoverflow.com/questions/11406189/determine-if-angle-lies-between-2-other-angles/11412077#11412077
def isAngleBetween(target, angle1, angle2):
    rAngle = ((angle2 - angle1) % 360 + 360) % 360
    if rAngle >= 180:
        angle1, angle2 = angle2, angle1

    if angle1 <= angle2:
        return target >= angle1 and target <= angle2
    else:
        return target >= angle1 or target <= angle2

class linearFunction:
    """
    grad: the gradient of the input function
    int: the y-intercept of the input function
    perp: the perpendicular gradient of the input function
    domainLower: lower bound of the X coordinate
    domainUpper: upper bound of the X coordinate
    rangeLower: lower bound of the Y coordinate
    rangeUpper: upper bound of the Y coordinate
    """

    def __init__(self, startX, startY, endX, endY):
        if startX == endX: # Line is vertical
            self.grad = None # Set to None as these would most likely be undefined/non-real
            self.int = None
            self.perp = 0 # Perpendicular gradient
        else:
            self.grad = (startY - endY) / (startX - endX)
            self.int = startY - (self.grad * startX)
            if self.grad == 0: # Check if line is horizontal to avoid undefined perpendicular
                self.perp = None
            else:
                self.perp = -1 / self.grad

        self.domainLower = min(startX, endX)
        self.domainUpper = max(startX, endX)
        self.rangeLower = min(startY, endY)
        self.rangeUpper = max(startY, endY)

    def smallestDist(self, pointX, pointY):
        if self.grad == None: # Check if line is vertical
            if pointY > self.rangeUpper:
                return euclideanDist(self.domainLower, self.rangeUpper, pointX, pointY)
            elif pointY < self.rangeLower:
                return euclideanDist(self.domainLower, self.rangeLower, pointX, pointY)
            else:
                return abs(pointX - self.domainLower)
        elif self.perp == None: # Check if line horizontal
            if pointX > self.domainUpper:
                return euclideanDist(self.domainUpper, self.rangeLower, pointX, pointY)
            elif pointX < self.domainLower:
                return euclideanDist(self.domainLower, self.rangeLower, pointX, pointY)
            else:
                return abs(pointY - self.int)
        else: # https://www.desmos.com/calculator/lbp0ttkhg1
            closestX = -((self.grad * self.int - self.grad * pointY - pointX) / (1 + pow(self.grad, 2)))
            closestY = self.grad * closestX + self.int

            return euclideanDist(closestX, closestY, pointX, pointY)

class circularFunction:
    """
    Now before anyone tells me that a circle is not a function, you're right, it's not. However, there is nothing larger
    than a quarter circle, therefore we will treat it as a function. Also it just fits the naming convention better.

    startAngle: the angle at which the arc starts
    endAngle: the angle at which the arc ends
    radius: the radius of the arc
    centreX: the X coordinate of the centre
    centreY: the Y coordinate of the centre
    startX: the start X coordinate of the arc
    startY: the start Y coordinate of the arc
    endX: the end X coordinate of the arc
    endY: the end Y coordinate of the arc
    """

    def __init__(self, startAngle, endAngle, radius, centreX, centreY):
        self.startAngle = startAngle
        self.endAngle = endAngle
        self.radius = radius
        self.centreX = centreX
        self.centreY = centreY
        self.startX = cos(startAngle) + centreX
        self.startY = sin(startAngle) + centreY
        self.endX = cos(endAngle) + centreX
        self.endY = sin(endAngle) + centreY

    def smallestDist(self, pointX, pointY):
        distToCentre = euclideanDist(self.centreX, self.centreY, pointX, pointY)
        interceptAngle = (atan2(pointY - self.centreY, pointX - self.centreX) + 360) % 360

        if isAngleBetween(interceptAngle, self.startAngle, self.endAngle):
            return abs(distToCentre - self.radius)
        else:
            distToStart = euclideanDist(self.startX, self.startY, pointX, pointY)
            distToEnd = euclideanDist(self.endX, self.endY, pointX, pointY)

            return min(distToStart, distToEnd)