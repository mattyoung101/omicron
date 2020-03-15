# ATmega Project
This is the PlatformIO project that runs on our ATmega328P slave device, which handles the the mouse sensor and motor. 
The IC itself is soldered directly to the Base PCB at the bottom of the robot, and can be programmed from the shared 
USB port on the Main PCB. It communicates directly with the ESP32 over I2C at 100KHz and exchanges motor speeds and 
mouse sensor data. 

**Credits:**
- Lachlan Ellis: mouse sensor testing and debugging
- Ethan Lo: communication and motor code, project maintenance

## Features list
- PMW3360 lol

## Building and running
Install the PlatformIO extension in VSCode, then import this folder. Alternatively install the platformio CLI (command 
line interface) with pip (ensure you are using Python 3.x as Python 2 is deprecated). See platformio documentation for 
usage: https://docs.platformio.org/en/latest/core/index.html. If you use CLion, you can install platformio for CLion. 
See: https://docs.platformio.org/en/latest/integration/ide/clion.html.

## Libraries and licenses
- [Timer](https://github.com/TomFraser/FG-B-2018/tree/master/Software/lib/Timer): GPLv3 license
- [Arduino](https://github.com/arduino/Arduino): GPLv2 license