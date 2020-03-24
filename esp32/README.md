# ESP32 firmware
This folder contains the ESP32 firmware, written in C using Espressif's IoT Development Framework (ESP-IDF). 
Currently, we use VSCode and CLion to edit.

The "main" directory contains the actual ESP32 firmware, "components" contains some libraries that are
linked in through the IDF's component system (which is just a wrapper around CMake), and "scripts" contains 
various Python scripts for data generation and development assistance. 

**Credits:**
- Ethan Lo: behaviour and movement code (FSM frontend), tuning, comms debugging
- Lachlan Ellis: behaviour and movement code (FSM frontend, path following algorithm, strategies)
- Matt Young: low level software (UART/Bluetooth comms, FSM backend, Protobuf, FreeRTOS stuff)

## Feature list
- Advanced novel orbit approach using ellipse maths
- Advanced strategies including ball hiding, flick-shots, line-running and more
- Hierarchical Finite State Machine (HFSM) for robot behaviour management
- Robust Bluetooth communication between robots using the ESP32's built in Bluetooth Classic APIs
- PID motor control with velocity feedback from mouse sensor
- Custom UART comms protocol (JimBus) that sends Protobuf messages between devices with a CRC8 checksum
- I2C protocol (JimBusLE) that sends manually encoded data (bitshifting) to less powerful devices, also with CRC8
- Uses both cores of the ESP32, heavy use of FreeRTOS threads, mutexes, timers, etc
- Written in C11 using the ESP-IDF (no Arduino here!)

## Building and running
Firstly, you'll need to the ESP32 IDF v3.3 toolchain for your platform. Please 
[visit Espressif's docs](https://docs.espressif.com/projects/esp-idf/en/v3.3/get-started-cmake/index.html) for information
on how to install this. **Important workarounds for Windows:** you need to make the default program to open
.py files the Python 2 interpreter if you want to type "idf.py" in your shell as they do in the docs.

Once you've installed the toolchain, you need to open a terminal in the "esp32" directory and run `idf.py build` to 
generate the initial build.

### VSCode Instructions
Once that's done, install the C/C++ extension for VSCode and open this folder ("esp32"). When (or if) it prompts you if you'd 
like to configure C/C++ automatically using compile_commands.json, select "yes" and choose the one that's **NOT**
the bootloader project. 

If this process fails (as it often does), try restarting VSCode. If it still fails, then check docs/c_cpp_properties_template.txt 
and copy it into .vscode/c_cpp_properties.json, entering the correct path to your ESP toolchain directory and restart VSCode
again. If it still fails I'm afraid you're out of luck, just try redoing the whole process again.

Assuming you've got this setup successfully, you will now have syntax highlighting and IntelliSense in VSCode.
So just use the shell scripts "b" for "build", "fm" for "flash & monitor", "f" for "flash" and "m" for "monitor" to
compile and monitor. Remember the golden rule: if VSCode complains, try restarting it and rebuilding the IntelliSense
cache. At worst, ignore it because it's almost always wrong in these cases.

### CLion Instructions
Open main.c from the "esp32/main" directory. CLion should ask for you to choose a CMakeLists.txt file (if it doesn't, 
wait a couple seconds or restart CLion). Select the CMakeLists.txt file in the esp32 and you should be good to go.

## Developer notes
### IDF version
Currently, only IDF v3.3 is supported, specifically, the 
[v3.3 release branch version](https://github.com/espressif/esp-idf/tree/release/v3.3), which is supported till 2022. 
Please make sure you're referring to v3.3 docs on Espressif's website when developing, and have an installed the v3.3 toolchain.

It _is_ possible to port to IDF v4, and we plan to do so soon; it just requires a little bit of refactoring, for example
to code using `ets_delay_us` (as that's been moved to a different header).

## Libraries and licenses
- [ESP-IDF](https://github.com/espressif/esp-idf/): Apache 2 license
- [DG_dynarr](https://github.com/DanielGibson/Snippets/blob/master/DG_dynarr.h): Public domain
- [HandmadeMath](https://github.com/HandmadeMath/Handmade-Math): Public domain
- [Nanopb](https://github.com/nanopb/nanopb/): zlib license
- [BNO055_driver](https://github.com/BoschSensortec/BNO055_driver): BSD 3-Clause license
- [esp32-button](https://github.com/craftmetrics/esp32-button): MIT license
