# Omicam
Omicam Team Omicron's custom, high-performance, all-in-one vision and localisation application that runs on a single board computer.

For the full technical writeup on our vision pipeline, please see docs/DESCRIPTION.md or our website.

**Credits:**
- Matt Young: main C/C++ programmer, Markdown docs
- Ethan Lo: field file generator, localisation research

**Special thanks:**
- Tom Fraser: assistance with OpenCV CUDA performance debugging (when it was still being used)

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
Hardware wise, you'll need a Jetson Nano and a supported CSI camera, probably the Raspberry Pi Camera v2 (or v1 if that's unavailable).

Flash your Jetson's SD card as per NVIDIA's instructions, boot and update it, then install the following additional packages:

- CMake: To work around various issues (see below), you need to [build the latest CMake from source](https://cmake.org/install/) rather
than getting it from the repos.
You will need to `sudo apt install libssl-dev` beforehand, otherwise it will complain. To bootstrap the fastest, I'd recommend using this: 
`./bootstrap --parallel=4 -- -DCMAKE_BUILD_TYPE:STRING=Release`
- Ninja: `sudo apt install ninja-build`
- Clang & LLVM: `sudo apt install clang lldb llvm libasan5 libasan5-dbg`
- NLopt: `sudo apt install libnlopt-dev libnlopt0`
- libjpeg-turbo: `sudo apt install libturbojpeg libturbojpeg0-dev`
- SDL2: `sudo apt install libsdl2-2.0.0 libsdl2-dev libsdl2-doc`
- OpenCV: You will also have to build from source, [see here](https://docs.opencv.org/master/d7/d9f/tutorial_linux_install.html)

**Note:** A lot of these packages can also be compiled from source if you're experiencing issues.

It may be advisable to remove a bunch of the useless packages Ubuntu installs by default such as LibreOffice. You can do
so with `sudo apt remove <PACKAGE_NAME>`.

**FIXME: talk about how to add optimisations to the NLopt build (adding O3 and hard FPU, etc) if we still build by source.**

**FIXME: cover running on PC (test target) instead of Jetson. Also include specific Jetson setup instructions.**

**FIXME: cover setting up the toolchain as Ninja**

We exclusively use CLion to develop Omicam. Import the project into CLion on your host computer and follow the 
[instructions provided by JetBrains](https://www.jetbrains.com/help/clion/remote-projects-support.html) to configure a 
remote toolchain, deployment and run configuration. The IP should be the IP of your Jetson, check your router or use nmap
if you're unsure what this is.

To run, just use SHIFT+F10 or SHIFT+F9 to debug, like you would normally. CLion will handle the rest for you. 

### Known issues
- **Clang is the only supported compiler** as it appears that gcc's implementation 
of Google's Sanitizers doesn't work. You can do this by changing the compiler path from the default gcc 
to /usr/bin/clang in CLion's toolchain settings.
- CLion runs the wrong executable due to a long-standing bug with CUDA. Due to this, and also the fact we have elected
to use the Ninja build tool, you have to build CMake from source, because the latest version in the repos is 3.10 which
is quite outdated. Specifiying Ninja as the build generator fixes the wrong executable issue.
- If lldb is broken, try using gdb instead (it doesn't matter that you're compiling with Clang, both will work fine).
- You will need to disable the visual Address Sanitizer output in CLion as that is also broken.
- If you install a new library on the Jetson, you will need run Tools->"Resync with remote hosts" to get the new headers.
- CLion's remote upload occasionally (a few times per full day of work) fails temporarily, just ignore it and try again.

### Final notes
I sincerely apologise for how complex this build process is, but a lot of it is out of my control as I have to do certain
steps to work around unfixed bugs and other issues. If you have any questions, don't hesitate to contact me (see above
for details).

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