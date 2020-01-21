import numpy as np
from math import cos, sin, radians

class virtualRobot:
    def __init__(self, xPos, yPos, heading):
        self.x = xPos
        self.y = yPos
        self.heading = heading
        self.points = [
            linePoint(66 - self.x, 96.5 - self.y),
            linePoint(66 - self.x, -96.5 - self.y),
            linePoint(-66 - self.x, -96.5 - self.y),
            linePoint(-66 - self.x, 96.5 - self.y),
            linePoint(0 - self.x, 96.5 - self.y),
            linePoint(0 - self.x, -96.5 - self.y),
            linePoint(66 - self.x, 0 - self.y),
            linePoint(-66 - self.x, 0 - self.y)
        ]

        for point in self.points:
            point.transform(self.heading)

class linePoint:
    def __init__(self, relX, relY):
        self.x = relX
        self.y = relY

    def transform(self, heading):
        rotation = np.array([[cos(radians(-heading)), sin(radians(-heading))],
                             [-sin(radians(-heading)), cos(radians(-heading))]])
        mat = np.array([[self.x],
                        [self.y]])
        new = np.matmul(rotation, mat)
        self.x = new[0, 0]
        self.y = new[1, 0]