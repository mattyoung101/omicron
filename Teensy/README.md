# Teensy project
This is the PlatformIO project that runs on our Teensy 4.0 slave device, which handles the light sensors and LRFs. We 
chose to run LRFs and light sensors on a separate devices to maximise the speed at which we could track the line. 
Furthermore the Teensy was used due to the large number of hardware serial ports that could be used for LRFs.

Credits:
- Ethan Lo: I2C code, LRF code and main programmer
- Lachlan Ellis: light sensor code
- Matt Young: UART comms/Protobuf

## Features list
- Efficient and robust cluster-based light sensor processing algorithm to detect the line
- Process LRF data
- Process and send messages using Protocol Buffers to ESP32 over UART

## Building and running
Install the PlatformIO extension in VSCode, then import this folder. Can also use CLion if you wish.

## Licence
The Teensy firmware is licenced under the Mozilla Public License 2.0, the same as the rest of Team Omicron's code.

## Libraries and licenses
- [Nanopb](https://github.com/nanopb/nanopb/): zlib license
- [FG&B Timer](https://github.com/TomFraser/FG-B-2018/tree/master/Software/lib/Timer): GPLv3 license
- [Arduino](https://github.com/arduino/Arduino): GPLv2 license
