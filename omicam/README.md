# Omicam

This is the custom vision system for Team Omicron, to be deployed in the 2020 Internationals. 
The production setup uses a Raspberry Pi Compute Module 3+ with a Pi Camera v2.
For the full technical writeup on our custom vision pipeline, please see, docs/DESCRIPTION.md.

Omicam is built and maintained by Matt Young, so if you have any questions, please contact: 25070@bbc.qld.edu.au

## Features list
- GPU accelerated camera decoding using Broadcom's MMAL libraries, capable of 720p 60fps
- Custom designed multi-threaded colour segmentation with linear threads vs. framerate relationship
- **(WIP)** Highly optimal CPU connected-component labelling (blob detection) based on recent state-of-the-art papers
- **(WIP)** Advanced localisation by way of non-linear optimisation (Nelder-Mead simplex method provided by NLopt)
- Wireless, multi-threaded frame streaming using SIMD accelerated libjpeg-turbo, to custom Kotlin remote management app
    - Network protocol uses TCP socket and Protocol Buffers
- Uses less than 5% of the Pi's RAM in release mode (sanitizers disabled)
- **(WIP)** Transmits data to main ESP32 microcontroller via Protocol Buffers over 115200 baud UART
- Well-documented C and associated design documents (see docs folder and inline comments).

## Building and running
JetBrains CLion is the only supported IDE for working with Omicam. We use CLion's full remote mode to work with the Pi.

### Instructions
Firstly, you'll need a board compatible with the Raspi Compute Module 3+ and Pi Cam v2.1. I use a Raspi 3 Model B and
Pi Cam v1.3.

Flash your Pi's SD card with Raspbian Lite, boot and update it, then install the following packages:
- CMake: `sudo apt install cmake`
- Clang and lldb: `sudo apt install clang lldb`
- [NLopt](https://NLopt.readthedocs.io/en/latest/): follow the instructions linked
- libjpeg-turbo: `sudo apt install libturbojpeg0 libturbojpeg0-dev`

**FIXME: talk about how to add optimisations to the NLopt build (adding O3 and hard FPU, etc)**

Import the project into CLion on your host computer and follow the 
[instructions provided by JetBrains](https://www.jetbrains.com/help/clion/remote-projects-support.html) to setup a remote toolchain
and run configuration. The remote settings should be synced in Git, but you may need to modify the IP address to that of your
local Pi. For some reason it doesn't seem that `raspberrypi.local` works.

To run, just use SHIFT+F10 or SHIFT+F9 to debug, like you would normally. CLion will handle the rest for you. 

#### Known issues
- **It is extremely important that you compile with Clang, NOT gcc** as it appears that gcc's implementation 
of Address Sanitizer doesn't work (only Clang's does). You can do this by changing the compiler path from the default gcc 
to /usr/bin/clang in CLion's toolchain settings. You should be able to compile with gcc for release mode as ASan isn't
used but it's not recommended. While ASan is a vital tool for debugging, if you are only building for release you
can use GCC if you want.

- Currently you should leave the debugger as the default gdb because it seems lldb doesn't work, and you won't notice a
difference in CLion. 

- You will need to disable the visual Address Sanitizer output as that is also broken.

- If you install a new library on the Pi, you will need run Tools->Resync with remote hosts to update the new headers.

- CLion's remote upload occasionally (few times per full day of work) fails temporarily, just ignore it and try again.

## License
Omicam is available under the main project license, see LICENSE.txt in this directory or the root directory.

## Libraries and licenses
- [log.c](https://github.com/rxi/log.c): MIT license
- [iniparser](https://github.com/ndevilla/iniparser): MIT license
- [nanopb](https://github.com/nanopb/nanopb): Zlib license
- [raspicam (MMAL)](https://github.com/raspberrypi/userland/tree/master/host_applications/linux/apps/raspicam): BSD 3-Clause
- [DG_dynarr](https://github.com/DanielGibson/Snippets/blob/master/DG_dynarr.h): Public Domain
- [rpa_queue](https://github.com/chrismerck/rpa_queue): Apache 2
- [libjpeg-turbo](https://github.com/libjpeg-turbo/libjpeg-turbo): Various BSD and Zlib
- [NLopt](https://github.com/stevengj/nlopt): MIT, as no LGPL code is used