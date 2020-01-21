from lineFunctions import linearFunction, circularFunction
from objectiveFunction import virtualRobot, linePoint
from math import sqrt, pow, radians
import itertools
from PIL import Image
import FieldFile_pb2 as pb

def constrain(val, smol, big):
    return min(max(val, smol), big)

# Rules and field specifications:
# https://junior.robocup.org/wp-content/uploads/2020Rules/2020_Soccer_Rules_draft01.pdf

fieldLength = 243 # cm
fieldWidth = 182
output = [[None for x in range(fieldWidth)] for y in range(fieldLength)] # Note: notation is output[y][x]
whiteLines = [
    linearFunction(-66, 96.5, -66, -96.5),
    linearFunction(-66, 96.5, 66, 96.5),
    linearFunction(66, 96.5, 66, -96.5),
    linearFunction(66, -96.5, -66, -96.5),
    linearFunction(35, 96.5, 35, 86.5),
    linearFunction(-35, 96.5, -35, 86.5),
    linearFunction(35, -96.5, 35, -86.5),
    linearFunction(-35, -96.5, -35, -86.5),
    linearFunction(20, 71.5, -20, 71.5),
    linearFunction(20, -71.5, -20, -71.5),
    circularFunction(0, radians(90), 15, 20, -86.5),
    circularFunction(radians(90), radians(180), 15, -20, -86.5),
    circularFunction(radians(180), radians(270), 15, -20, 86.5),
    circularFunction(radians(270), radians(360), 15, 20, 86.5)
]

robot = virtualRobot(500, 1000, 0)

# --- GENERATES FIELD FILE ARRAY --- #

for rownum, row in enumerate(output):
    for colnum, cell in enumerate(row):
        smallestDist = sqrt(pow(fieldLength, 2) + pow(fieldWidth, 2))
        for line in whiteLines:
            smallestDist = min(line.smallestDist(colnum - fieldWidth / 2, rownum - fieldLength / 2), smallestDist)
        output[rownum][colnum] = smallestDist

# --- FAKE OBJECTIVE FUNCTION --- #

objectiveMap = [[None for x in range(fieldWidth)] for y in range(fieldLength)] # Note: notation is output[y][x]

for X in range(0, fieldWidth):
    x = X - fieldWidth / 2
    for Y in range(0, fieldLength):
        y = Y - fieldLength / 2
        runningTotal = 0
        for point in robot.points:
            runningTotal += output[constrain(int(round(point.y + fieldLength / 2 + y)), 0, fieldLength - 1)][constrain(int(round(point.x + fieldWidth / 2 + x)), 0, fieldWidth - 1)]
        objectiveMap[Y][X] = runningTotal

# print(objectiveMap)

maxDist = max([sublist[-1] for sublist in objectiveMap])

for rownum, row in enumerate(output):
    for colnum, cell in enumerate(row):
        objectiveMap[rownum][colnum] = int(cell / maxDist * 255) * -1 + 255

output = list(itertools.chain(*objectiveMap))

for row in output:
    print(row)

img = Image.new('L', (fieldWidth, fieldLength))
img.putdata(output)
# img.save("Standard Field.bmp")
img.show()

# message = pb.FieldFile()
# message.unitDistance = 1
# message.cellCount = int(fieldLength * fieldWidth)
# message.width = int(fieldWidth)
# message.length = int(fieldLength)
# message.data.extend(output)
#
# with open("../../fields/Ints_StandardField.ff", "wb") as file:
#     serialised = message.SerializeToString()
#     file.write(serialised)
