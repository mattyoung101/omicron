# ESP32 firmware
This folder contains the ESP32 firmware, written in C using the ESP-IDF. Currently, we use VSCode to edit
but are planning to switch to Eclipse once we can upgrade to IDF v4.

The "main" directory contains the actual ESP32 firmware, "components" contains some libraries that are
linked in through the IDF's component system (which is just a wrapper around CMake), and "scripts" contains 
various Python scripts for data generation and development assistance. 

The ESP32 firmware is maintained by two groups of people: Lachlan Ellis and Ethan Lo for the control
software (e.g. game strategies, tuning), and Matt Young for the low level software (e.g. UART comms, 
FSM implementation).

## Feature list
- T.B.A.

## Building and running
As mentioned before, VSCode is currently the only supported IDE for editing. We've had no like using
CLion, but will try Eclipse soon.

Firstly, you'll need to the ESP32 toolchain for your platform. Please 
[visit Espressif's docs](https://docs.espressif.com/projects/esp-idf/en/v3.3/get-started-cmake/index.html) for information
on how to install this. **Please observe the important workaround for Windows:** you need to make the default program to open
.py files the Python 2 interpreter in order for "idf.py" to work properly.

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

## Libraries and licenses
**TODO ADD THESE**