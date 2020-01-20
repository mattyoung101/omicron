import numpy

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

        # TODO: Transform the linePoints (rotation)

class linePoint:
    def __init__(self, relX, relY):
        self.x = relX
        self.y = relY