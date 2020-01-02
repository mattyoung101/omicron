# Teensy project
This is the PlatformIO project that runs on our Teensy 4.0 slave device, which handles the light sensors and LRFs.

The Teensy project is built and maintained by Ethan Lo.

## Features list
- Efficient and robust cluster-based light senor processing algorithm to detect the line
- Process LRF data
- Process and send messages using Protocol Buffers to ESP32

## Building and running
Install the PlatformIO extension in VSCode, then import this folder.