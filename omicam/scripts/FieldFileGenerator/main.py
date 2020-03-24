from lineFunctions import linearFunction, circularFunction, euclideanDist
from objectiveFunction import virtualRobot, linePoint
from math import sqrt, pow, radians
import itertools
from PIL import Image
import FieldFile_pb2 as pb

# Rules and field specifications:
# https://junior.robocup.org/wp-content/uploads/2020Rules/2020_Soccer_Rules_draft01.pdf

fieldLength = 243 # cm
fieldWidth = 182
lineWidth = 1 # field lines are 2cm but it's 1cm each side of the line

output = [[None for x in range(fieldLength)] for y in range(fieldWidth)] # Note: notation is output[y][x]
whiteLines = [
    linearFunction(-61, 91.5, -61, -91.5),
    linearFunction(-61, 91.5, 61, 91.5),
    linearFunction(61, 91.5, 61, -91.5),
    linearFunction(61, -91.5, -61, -91.5),
    # linearFunction(35, 96.5, 35, 86.5),
    # linearFunction(-35, 96.5, -35, 86.5),
    # linearFunction(35, -96.5, 35, -86.5),
    # linearFunction(-35, -96.5, -35, -86.5),
    # linearFunction(20, 71.5, -20, 71.5),
    # linearFunction(20, -71.5, -20, -71.5),
    # circularFunction(0, radians(90), 15, 20, -86.5),
    # circularFunction(radians(90), radians(180), 15, -20, -86.5),
    # circularFunction(radians(180), radians(270), 15, -20, 86.5),
    # circularFunction(radians(270), radians(360), 15, 20, 86.5)
]

robot = virtualRobot(0, 0, 0)

# --- GENERATES FIELD FILE ARRAY --- #

for colnum, row in enumerate(output):
    for rownum, cell in enumerate(row):
        smallestDist = sqrt(pow(fieldLength, 2) + pow(fieldWidth, 2))
        for line in whiteLines:
            smallestDist = min(line.smallestDist(colnum - fieldWidth / 2, rownum - fieldLength / 2), smallestDist)
        output[colnum][rownum] = smallestDist

maxDist = max([sublist[-1] for sublist in output])

for colnum, row in enumerate(output):
    for rownum, cell in enumerate(row):
        # check if it's on the line before drawing
        if cell <= lineWidth:
            output[colnum][rownum] = 255
        else:
            output[colnum][rownum] = 0

output = list(itertools.chain(*output))

img = Image.new('L', (fieldLength, fieldWidth))
img.putdata(output)
img.save("field_img2.bmp")
img.show()

message = pb.FieldFile()
message.unitDistance = 1
message.cellCount = int(fieldLength * fieldWidth)
message.length = int(fieldLength)
message.width = int(fieldWidth)
message.data = bytes(output)

with open("ausfield.ff", "wb") as file:
    serialised = message.SerializeToString()
    file.write(serialised)
