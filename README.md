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
- components: contains libraries, see below for links and licenses
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
- [ESP-IDF](https://github.com/espressif/esp-idf/): Apache 2 license
- [DG_dynarr](https://github.com/DanielGibson/Snippets/blob/master/DG_dynarr.h): Public domain
- [HandmadeMath](https://github.com/HandmadeMath/Handmade-Math): Public domain
- [Nanopb](https://github.com/nanopb/nanopb/): zlib license
- [BNO055_driver](https://github.com/BoschSensortec/BNO055_driver): BSD 3-Clause license
- [Wren VM](https://github.com/wren-lang/wren/): MIT license
- [esp32-button](https://github.com/craftmetrics/esp32-button): MIT license