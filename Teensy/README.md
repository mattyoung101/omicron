# Teensy PlatformIO project
This directory contains the **legacy** code which ran on our Teensy 3.5 slave during 2018 and 2019.

It has since been superseded by our series of ATMega slaves.


## Old description
This directory contains all the code which runs on our Teensy slave for running light sensors and controlling motors.

As opposed to the ESP project in the root directory which uses CMake and ESP-IDF, this uses PlatformIO.

The main source code is in "src", while all the external libraries live in "lib".

This is all written by Ethan (except for the Protobuf decoder which was written by Matt).
