Team Omicron (2019)
====================

This repository contains the code powering Team Omicron's 2019 robot, competing in RoboCup Jr Soccer. It is written almost entirely in C, without some Python scripts for code generation and debugging.

For more information on our robot, please see our team's PowerPoint and poster.

Contact Matt Young (25070@bbc.qld.edu.au) for any questions, queries, qualms or concerns.

### Important notice about IDF version
Due to the fact that we sync the file `sdkconfig`, it's important that you use the exact same IDF version that we do, to avoid merge conflicts.
This project currently uses the **[v3.3 release branch](https://github.com/espressif/esp-idf/tree/release/v3.3) version** (i.e. the latest commit on origin/release/v3.3).

Please visit [the docs](https://docs.espressif.com/projects/esp-idf/en/latest/versions.html) for more information about IDF versions.

## Directory structure
- .vscode: VSCode settings, mainly spellchecking. In order to create `c_cpp_properties.json` for the C/C++ extension (which is not synced on Git), please see `docs/c_properties_template.txt`.
- components/
    - vl53l0x: ST's VL53L0X LRF driver and docs. We use Kyle Hendrick's [ESP32 port](https://github.com/kylehendricks/esp32-vl53l0x).
    - nanopb: A C library for encoding/decoding Protocol Buffer data: [repo](https://github.com/nanopb/nanopb).
- docs: contains various bits of documentation, templates, etc
- main: contains the main code that runs on the master and slave ESP32
- openmv: contains the code that runs on our camera, the OpenMV H7
- scripts: contains various Python scripts for generating code and simple debugging
    - midis: contains MIDI files to be converted and played on the robot
- Teensy: contains PlatformIO project for Teensy slave
- b.bat, b.sh: shortcut for "idf.py build"
- f.bat, f.sh: shortcut for "idf.py flash"
- fm.bat, fm.sh: shortcut to run "idf.py flash" then "idf.py monitor" if compilation was successful

## License
This code is currently proprietary and confidential to Brisbane Boys' College and Team Omicron. No redistribution or use outside of our team is permitted. 

_At some point, the code may be re-licensed under the BSD 3-Clause/4-Clause or the MPL 2.0._

## Libraries and licenses
- ESP-IDF: Apache 2
- DG_dynarr: Public Domain
- HandmadeMath: Public Domain
- nanopb: zlib license
- BNO050 driver from Bosch Sensortech: BSD 3-Clause