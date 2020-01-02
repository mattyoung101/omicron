"""
Python script which contains functions for defining and calculating the shortest distance to a given function.

Written by Ethan Lo, so don't expect this to work too well :)
"""

from math import sqrt

def euclideanDist(startX, startY, endX, endY): # just pythag
    return sqrt(pow(startX - endX, 2) + pow(startY - endY, 2))

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
    Now before anyone tells me that a circle is not a function, you're right, it's not. However, there is nothing more
    than a quarter circle, therefore we will treat it as a function. Also it just fits the naming convention better.
    """
    