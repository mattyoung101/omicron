import sensor, image, time, utime, pyb
from pyb import UART
import ucollections

def constrain(val, min_val, max_val):
    return min(max_val, max(min_val, val))

# OpenMV object tracking, by Matt Young
# Serial out format:
# [0xB, bfound, bx, by, yfound, yx, yy, 0xE] (6 bytes not including 0xB and 0xE)

thresholds = [(70, 83, 1, 33, -5, 43), # yellow
             (47, 64, -3, 31, -79, -54), # blue
             (52, 75, 59, 83, -11, 46)] # orange

# Practice: (53, 67, -128, 12, -128, -28)
# Actual: (19, 55, -19, 27, -69, -41)

# this comes from the output of blob.code()
# you're meant to compare them using binary (see docs) but... yeah nah
YELLOW = 1
BLUE = 2
ORANGE = 3

pyb.LED(1).on()

debug = True
light = False
out = []
clock = time.clock()
uart = UART(3)
uart.init(115200, bits=8, parity=None, stop=1, timeout_char=1000)

# sensor setup
sensor.reset()
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QVGA) #Resolution, QVGA = 42FPS,QQVGA = 85FPS

sensor.skip_frames(time=500)

sensor.set_auto_exposure(False)
sensor.set_auto_whitebal(False)
# Need to let the above 2 settings get in...
sensor.skip_frames(time=100)
#sensor.set_windowing((30, 5, 230, 230)) # Robot A
sensor.set_windowing((32, 0, 230, 230)) # Robot B

# === GAIN ===
curr_gain = sensor.get_gain_db()
sensor.set_auto_gain(False, gain_db=curr_gain)

# === EXPOSURE ===
curr_exposure = sensor.get_exposure_us()
sensor.set_auto_exposure(False, exposure_us = 20000)

# === WHITE BAL ===
sensor.set_auto_whitebal(False,
rgb_gain_db=(-5.368132, -6.02073, 2.766099))

# Standard
sensor.set_brightness(0)
sensor.set_contrast(3)
sensor.set_saturation(3)

sensor.skip_frames(time=500)

# Blink LEDs
pyb.LED(1).off()
for i in range(3):
    pyb.LED(2).on()
    pyb.delay(50)
    pyb.LED(2).off()
    pyb.delay(50)

if light:
    pyb.LED(1).on()
    pyb.LED(2).on()
    pyb.LED(3).on()

# Find biggest blob of specific colour
def scanBlobs(blobs, colour):
    biggestBlob = None
    bbArea = 0

    for blob in blobs:
        if blob.code() != colour:
            continue

        if biggestBlob == None:
            biggestBlob = blob
            bbArea = blob.area()
        else:
            blobArea = blob.area()

            if blobArea > bbArea:
                biggestBlob = blob
                bbArea = blobArea

    return biggestBlob

# Main loop
while True:
    begin = utime.time()
    clock.tick()
    img = sensor.snapshot()
    blobs = img.find_blobs(thresholds, x_stride=5, y_stride=5, pixels_threshold=40,
            area_threshold=15, merge=True, margin=2)
    biggestYellow = scanBlobs(blobs, YELLOW)
    biggestBlue = scanBlobs(blobs, BLUE)

    print(thresholds[-1])
    orangeBlobs = img.find_blobs([thresholds[-1]], x_stride=2, y_stride=2, pixels_threshold=1,
                    area_threshold=4, merge=False, margin=2)
    try:
        biggestOrange = sorted(orangeBlobs, key=lambda l: l.area(), reverse=True)[0]
    except Exception:
        biggestOrange = None

    # Debug drawing
    if biggestYellow != None and debug:
        img.draw_rectangle(biggestYellow.rect(), color=(255, 255, 0))
        img.draw_cross(biggestYellow.cx(), biggestYellow.cy())
        img.draw_string(biggestYellow.cx(), biggestYellow.cy(), "Goal_Y",
                        color=(255, 0, 0))

    if biggestBlue != None and debug:
        img.draw_rectangle(biggestBlue.rect(), color=(0, 0, 255))
        img.draw_cross(biggestBlue.cx(), biggestBlue.cy())
        img.draw_string(biggestBlue.cx(), biggestBlue.cy(), "Goal_B",
                        color=(255, 0, 0))

    if biggestOrange != None and debug:
        img.draw_rectangle(biggestOrange.rect(), color=(254, 95, 27))
        img.draw_cross(biggestOrange.cx(), biggestOrange.cy())
        img.draw_string(biggestOrange.cx(), biggestOrange.cy(), "Ball",
                        color=(255, 0, 0))

    # Serial out preparation
    out.clear()
    out += [0xB]
    #out += [0xB]

    if biggestBlue == None:
        out += [False, 0, 0]
    else:
        out += [True, constrain(int(biggestBlue.cx()), 0, 255), constrain(int(biggestBlue.cy()), 0, 255)]

    if biggestYellow == None:
        out += [False, 0, 0]
    else:
        out += [True, constrain(int(biggestYellow.cx()), 0, 255), constrain(int(biggestYellow.cy()), 0, 255)]

    if biggestOrange == None:
        out += [False, 0, 0]
    else:
        out += [True, constrain(int(biggestOrange.cx()), 0, 255), constrain(int(biggestOrange.cy()), 0, 255)]

    out += [0xE]

    #pyb.LED(2).on()
    for byte in out:
        uart.writechar(byte)
    #pyb.LED(2).off()

    if debug:
        img.draw_string(5, 5, "" + "\n".join(str(x) for x in out))

    print(clock.fps())
