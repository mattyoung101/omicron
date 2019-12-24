# Omicam

This is the custom vision system for Team Omicron, to be deployed in the 2020 Internationals. 
The production setup uses a NVIDIA Jetson Nano with a Pi Camera v2.

**For the full technical writeup on our custom vision pipeline, please see docs/DESCRIPTION.md.**

Omicam is built and maintained by Matt Young, so if you have any questions, please contact: 25070@bbc.qld.edu.au

## Features list
- Efficient camera decoding using V4L2 via gstreamer
- State of the art, GPU accelerated image processing using OpenCV and OpenCL
- Highly advanced, millimetre accurate, custom localisation algorithm using non-linear optimisation methods
- Fast, custom designed omnidirectional camera dewarping with nearest neighbour interpolation
- Asynchronous, wireless frame streaming using SIMD accelerated libjpeg-turbo, to custom Kotlin remote management app
    - Network protocol uses TCP socket and Protocol Buffers with zlib compression (low bandwidth requirements)
    - Also transmits misc. data like temperature of the Jetson
- Written almost entirely in C11 (just a little C++ for interfacing with OpenCV)
- Well-documented code and design document

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

We exclusively use CLion to develop Omicam. Import the project into CLion on your host computer and follow the 
[instructions provided by JetBrains](https://www.jetbrains.com/help/clion/remote-projects-support.html) to configure a 
remote toolchain, deployment and run configuration. The IP should be the IP of your Jetson, check your router or use nmap
if you're unsure what this is.

To run, just use SHIFT+F10 or SHIFT+F9 to debug, like you would normally. CLion will handle the rest for you. 

### Known issues
- **Clang is the only supported compiler** as it appears that gcc's implementation 
of Google's Sanitizers doesn't work. You can do this by changing the compiler path from the default gcc 
to /usr/bin/clang in CLion's toolchain settings.
- If lldb is broken, try using gdb instead (it doesn't matter that you're compiling with Clang, both will work fine).
- You will need to disable the visual Address Sanitizer output in CLion as that is also broken.
- If you install a new library on the Jetson, you will need run Tools->"Resync with remote hosts" to get the new headers.
- CLion's remote upload occasionally (a few times per full day of work) fails temporarily, just ignore it and try again.

## License
Omicam is available under the license of the whole Team Omicron repo, see LICENSE.txt in the root directory. 

## Libraries and licenses
- [log.c](https://github.com/rxi/log.c): MIT license
- [map](https://github.com/rxi/map): MIT license
- [iniparser](https://github.com/ndevilla/iniparser): MIT license
- [nanopb](https://github.com/nanopb/nanopb): Zlib license
- [DG_dynarr](https://github.com/DanielGibson/Snippets/blob/master/DG_dynarr.h): Public Domain
- [rpa_queue](https://github.com/chrismerck/rpa_queue): Apache 2
- [libjpeg-turbo](https://github.com/libjpeg-turbo/libjpeg-turbo): Various BSD and Zlib
- [NLopt](https://github.com/stevengj/nlopt): MIT (as no LGPL code is used)
- [OpenCV](https://opencv.org/): BSD 3-Clause
- [kdtree](https://github.com/jtsiomb/kdtree): BSD 3-Clause