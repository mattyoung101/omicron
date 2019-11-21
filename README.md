Team Omicron (2020)
====================

This repository contains the code powering Team Omicron's 2020 robot, competing in RoboCup Jr Open Soccer at the Bordeaux
Internationals. The Omicam and the ESP32 firmware are written in C, Teensy firmware in C++ (Arduino), and Omicontrol in Kotlin.

For more information on our robot that isn't covered here (especially hardware), please see our team's PowerPoint and poster.

Contact repo maintainer Matt Young (25070@bbc.qld.edu.au) for any questions, queries, qualms or concerns - I will forward
you to the appropriate team member.

## General overview
**TODO update for new robot**

Our robot consists of two microcontrollers: the ESP32 which acts as a master for running logic, and a Teensy 3.5
slave which reads light sensors and controls motors. The ESP project is in main/ and components/, while the Teensy
project is in Teensy/. The ESP project uses the ESP-IDF toolchain, and as of writing (October 2019) it is believed that
we are the only team in RoboCup Jr worldwide to use the ESP32 with IDF (most other teams use Arduino). The ESP32 project
is written in C11 and the Teensy 3.5 project is written in C++98.

The robot's behaviour is implemented using a finite state machine. The implementation of this is in fsm.c and fsm.h,
while the implementation of the FSM states are in the states* files (this is all in the "main" directory). To increase
our robot's teamwork ability, we use Bluetooth Classic stack built in to the ESP32 to perform inter-robot communication,
meaning our defender robot can switch to an attacker if it gets the ball, and the attacker can become a defender if the
current attacker goes off for damage.

To perform intra-robot communication between the ESP32 and Teensy, we use Protocol Buffers via the nanopb library. The
bytes are sent over UART clocked at 115200 baud.

For more information about our gameplay features and hardware, you should consult Omicron's team poster and/or website.

### Team history and current members
**TODO and list members**

## Directory structure
- .vscode: VSCode settings, mainly spellchecking. In order to create `c_cpp_properties.json` for the C/C++ extension (which is not synced on Git), please see `docs/c_properties_template.txt`.
- components: contains ESP32 libraries, see below for links and licenses
- docs: contains various bits of documentation, templates, etc for ESP32 project
- main: contains the main code that runs on the master and slave ESP32
- omicam: contains our advanced custom camera running on a Raspberry Pi Compute Module 3+, please see the README inside
- omicontrol: contains our custom remote debug/management app for the robot and camera, see the README inside for more info
- openmv: contains the legacy camera code that used to run on our old OpenMV H7, now replaced by the custom Omicam. Scheduled for removal soon.
- scripts: contains various Python scripts for generating code and simple debugging
    - midis: contains MIDI files to be converted and played on the robot
- Teensy: contains PlatformIO project for Teensy slave
- b.bat, b.sh: shortcut for "idf.py build"
- f.bat, f.sh: shortcut for "idf.py flash"
- fm.bat, fm.sh: shortcut to run "idf.py flash" then "idf.py monitor" if compilation was successful

## License
This code is currently proprietary and confidential to Brisbane Boys' College and Team Omicron. No redistribution or use outside of our team is permitted. 

_If we are allowed to, the code will be made available under a permissive open source license once our competition is done._

## Developer notes
### IDF version
Due to the fact that we sync the file `sdkconfig`, it's important that you use the exact same IDF version that we do, to avoid merge conflicts.
This project currently uses the **[v3.3 release branch](https://github.com/espressif/esp-idf/tree/release/v3.3) version** (i.e. the latest commit on origin/release/v3.3).

Please visit [the docs](https://docs.espressif.com/projects/esp-idf/en/latest/versions.html) for more information about IDF versions.

**TODO this is only for ESP, also we should move ESP project to a subfolder**
## Libraries and licenses
- [ESP-IDF](https://github.com/espressif/esp-idf/): Apache 2 license
- [DG_dynarr](https://github.com/DanielGibson/Snippets/blob/master/DG_dynarr.h): Public domain
- [HandmadeMath](https://github.com/HandmadeMath/Handmade-Math): Public domain
- [Nanopb](https://github.com/nanopb/nanopb/): zlib license
- [BNO055_driver](https://github.com/BoschSensortec/BNO055_driver): BSD 3-Clause license
- [Wren VM](https://github.com/wren-lang/wren/): MIT license
- [esp32-button](https://github.com/craftmetrics/esp32-button): MIT license