# Omicam

This is the custom vision system for Team Omicron, to be deployed in the 2020 Internationals. 
The production setup uses a NVIDIA Jetson Nano with a Pi Camera v2.
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
### Instructions
Hardware wise, you'll need a Jetson Nano and a Raspberry Pi Camera v2 (or v1 if that's unavailable).

Flash your Jetson's SD card as per NVIDIA's instructions, boot and update it, then install the following packages:
- CMake: `sudo apt install cmake`
- Clang and lldb: `sudo apt install clang lldb`
- NLopt: follow the instructions linked [here](https://NLopt.readthedocs.io/en/latest/)
- libjpeg-turbo: `sudo apt install libturbojpeg0 libturbojpeg0-dev`
- SDL2: `sudo apt install libsdl2-2.0.0 libsdl2-2.0-0-dbgsym libsdl2-dev libsdl2-doc`
- OpenCV: follow the instructions linked [here](https://docs.opencv.org/master/d7/d9f/tutorial_linux_install.html)

**Note:** A lot of these packages can also be compiled from source if you're experiencing issues.

**FIXME: talk about how to add optimisations to the NLopt build (adding O3 and hard FPU, etc)**

**FIXME: cover running on PC (test target) instead of Jetson. Also include specific Jetson setup instructions.**

Import the project into CLion on your host computer and follow the 
[instructions provided by JetBrains](https://www.jetbrains.com/help/clion/remote-projects-support.html) to configure a 
remote toolchain, deployment and run configuration. The IP should be the IP of your Jetson, check your router or use nmap
if you're unsure what this is.

**Note:** CLion is the only supported and tested IDE for working with the project. Other IDEs may work, but this is
your own responsibility.

To run, just use SHIFT+F10 or SHIFT+F9 to debug, like you would normally. CLion will handle the rest for you. 

### Known issues
- **It is extremely important that you compile with Clang, NOT gcc** as it appears that gcc's implementation 
of Google's Sanitizers doesn't work. You can do this by changing the compiler path from the default gcc 
to /usr/bin/clang in CLion's toolchain settings.
- Currently you should leave the debugger as the default gdb because it seems lldb doesn't work.
- You will need to disable the visual Address Sanitizer output in CLion as that is also broken.
- If you install a new library on the Jetson, you will need run Tools->"Resync with remote hosts" to get the new headers.
- CLion's remote upload occasionally (few times per full day of work) fails temporarily, just ignore it and try again.

## License
Omicam is available under the license of the whole Team Omicron repo, see LICENSE.txt in the root directory. 

## Libraries and licenses
- [log.c](https://github.com/rxi/log.c): MIT license
- [iniparser](https://github.com/ndevilla/iniparser): MIT license
- [nanopb](https://github.com/nanopb/nanopb): Zlib license
- [DG_dynarr](https://github.com/DanielGibson/Snippets/blob/master/DG_dynarr.h): Public Domain
- [rpa_queue](https://github.com/chrismerck/rpa_queue): Apache 2
- [libjpeg-turbo](https://github.com/libjpeg-turbo/libjpeg-turbo): Various BSD and Zlib
- [NLopt](https://github.com/stevengj/nlopt): MIT (as no LGPL code is used)
- [SDL](https://www.libsdl.org/): Zlib license
- [OpenCV](https://opencv.org/): BSD 3-Clause