# Omicam
Omicam is Team Omicron's custom, high-performance vision and localisation application that runs on a single board computer.

For the full technical writeup on our vision pipeline, please see docs/OMICAM_DESIGN_DOC.md or our website.

**Credits:**
- Matt Young: main C/C++ programmer, Markdown docs, project maintainer
- Ethan Lo: field file generator, localisation research & assistance

**Special thanks:**
- Tom Fraser: assistance with performance debugging, help with selecting the best SBC and camera, as well as overall enthusiasm and support!

## Features list
- Efficient camera decoding using V4L2 via the popular GStreamer library
- State of the art, high-performance image processing using OpenCV 4
- Advanced custom localisation algorithm using a novel three step non-linear optimisation method, accurate to ~1cm
- Wireless frame streaming over TCP/IP using SIMD accelerated libjpeg-turbo, to custom Kotlin remote management app (Omicontrol)
    - Custom network protocol uses Protocol Buffers with zlib compression (low bandwidth requirements)
    - Also transmits misc. data like temperature of the SBC
- UART comms using termios for communicating to MCU (also using Protocol Buffers)
- Written mostly in C11 (just a little C++ for interfacing with OpenCV)
- Well-documented code and design document

## Building and running
### Instructions
While Omicam should in theory work on any single board computer with a bit of effort, we use a LattePanda Delta 432
running Xubuntu 18.04. It will only work under Linux.

Boot and install any Debian-based Linux distro to your SBC, then install the following extra packages:

- CMake: To work around various issues (see below), a newer version of CMake than the one provided by the Ubuntu repos is provided.
  The easiest way to install the latest CMake, in my opinion, is to [add the PPA](https://apt.kitware.com/) and then just 
  `sudo apt install cmake`.
- Ninja: `sudo apt install ninja-build`
- Clang & LLVM: `sudo apt install clang lldb llvm libasan5 libasan5-dbg`
- NLopt: `sudo apt install libnlopt-dev libnlopt0`
- libjpeg-turbo: `sudo apt install libturbojpeg libturbojpeg0-dev`
- gstreamer: [see here](https://gstreamer.freedesktop.org/documentation/installing/on-linux.html) and also 
  `sudo apt install libgstreamer-opencv1.0-0 libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev`
- ffmpeg: `sudo apt install ffmpeg libavformat-dev libavcodec-dev libswscale-dev libavresample-dev`
- GTK devlopment files: `sudo apt install libgtk2.0-dev`
- create_ap: follow [these instructions](https://github.com/oblique/create_ap). Then, use
  [this information](https://github.com/oblique/create_ap/issues/238#issuecomment-292175855) to create a permanent WiFi
  service that starts on boot. Generally you'll want to call the hotspot "Omicam01" and "Omicam02" for each robot.
- OpenCV: You have to build from source, [see here](https://docs.opencv.org/master/d7/d9f/tutorial_linux_install.html).
  Pro tip: if your host computer is Linux-based, use `ssh -Y <username>@<domain>` so you can run cmake-gui remotely.

**Note:** It may be advisable to remove a bunch of the useless packages Ubuntu installs by default such as LibreOffice.

We exclusively use CLion to develop Omicam. Import the project into CLion on your host computer and follow the 
[instructions provided by JetBrains](https://www.jetbrains.com/help/clion/remote-projects-support.html) to configure a 
remote toolchain, deployment and run configuration. The IP should be the IP of your SBC, check your router or use nmap
if you're unsure what this is. You'll probably also want to enable auto upload to remote.

Under Build, Execution & Deployment in CMake, for your Debug and Release configurations add "-G Ninja" to
the CMake options section to use Ninja as your build runner.

To run, just use SHIFT+F10 or SHIFT+F9 to debug, like you would normally. CLion will (mostly) handle syncing to the remote
by itself.

### Known issues
- **Clang is the only supported compiler** as it appears that gcc's implementation 
of Google's Sanitizers doesn't work. You can do this by changing the compiler path from the default gcc 
to /usr/bin/clang in CLion's toolchain settings.
- If lldb is broken, try using gdb instead (it doesn't matter that you're compiling with Clang, both will work fine).
- You will need to disable the visual Address Sanitizer output in CLion as that is also broken.
- If you install a new library on the SBC, you will need run Tools->"Resync with remote hosts" to get the new headers.
- CLion's remote upload occasionally (a few times per full day of work) fails temporarily, just ignore it and try again.

## Licence
Omicam is available under the same licence of the whole Team Omicron repo, see LICENSE.txt in the root directory. 

## Open source libraries used
- [log.c](https://github.com/rxi/log.c): MIT license
- [iniparser](https://github.com/ndevilla/iniparser): MIT license
- [nanopb](https://github.com/nanopb/nanopb): Zlib license
- [DG_dynarr](https://github.com/DanielGibson/Snippets/blob/master/DG_dynarr.h): Public Domain
- [rpa_queue](https://github.com/chrismerck/rpa_queue): Apache 2 license
- [libjpeg-turbo](https://github.com/libjpeg-turbo/libjpeg-turbo): BSD 3-Clause, IJG and Zlib licenses
- [NLopt](https://github.com/stevengj/nlopt): MIT license
- [OpenCV](https://opencv.org/): BSD 3-Clause license
- [mathc](https://github.com/felselva/mathc): Zlib license
- [qdbmp](https://github.com/cbraudo/qdbmp): MIT license