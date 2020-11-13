#  This file is part of the Omicam project.
#  Copyright (c) 2019-2020 Team Omicron. All rights reserved.
#
#  Team Omicron members: Lachlan Ellis, Tynan Jones, Ethan Lo,
#  James Talkington, Matt Young.
#
#  This Source Code Form is subject to the terms of the Mozilla Public
#  License, v. 2.0. If a copy of the MPL was not distributed with this
#  file, You can obtain one at http://mozilla.org/MPL/2.0/.

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