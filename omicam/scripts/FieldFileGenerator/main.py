from lineFunctions import linearFunction, circularFunction
from math import sqrt, pow, radians
import itertools
from PIL import Image

# Rules and field specifications:
# https://junior.robocup.org/wp-content/uploads/2020Rules/2020_Soccer_Rules_draft01.pdf

fieldLength = 243 # cm
fieldWidth = 182
output = [[None for x in range(fieldWidth)] for y in range(fieldLength)] # Note: notation is output[y][x]
whiteLines = [
    linearFunction(-66, 96.5, -66, -96.5)
]

for rownum, row in enumerate(output):
    for colnum, cell in enumerate(row):
        smallestDist = sqrt(pow(fieldLength, 2) + pow(fieldWidth, 2))
        for line in whiteLines:
            smallestDist = min(line.smallestDist(colnum - fieldWidth / 2, rownum - fieldLength / 2), smallestDist)
        output[rownum][colnum] = smallestDist

maxDist = max([sublist[-1] for sublist in output])

for rownum, row in enumerate(output):
    for colnum, cell in enumerate(row):
        output[rownum][colnum] = int(cell / maxDist * 255) * -1 + 255

output = list(itertools.chain(*output))

# for row in output:
#     print(*row)

img = Image.new('L', (fieldWidth, fieldLength))
img.putdata(output)
img.show()