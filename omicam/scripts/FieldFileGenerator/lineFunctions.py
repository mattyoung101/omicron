"""
Python script which contains functions for defining and calculating the shortest distance to a given function.

Written by Ethan Lo, so don't expect this to work too well :)
"""

class linnearFunction:
    """
    Note: These docstrings are not here cos i know good code practice, it's cos i forget which thing is what

    grad: the gradient of the input function
    int: the y-intercept of the input function
    perp: the perpendicular gradient of the input function
    domainLower: lower bound of the X coordinate
    domainUpper: upper bound of the X coordinate
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

    def smallestDist(self, pointX, pointY):
        if self.grad == None: # Check if line is vertical
            return abs(pointX - self.domainLower)
        elif self.perp == None: # Check if line horizontal
            return abs(pointY - self.int)
