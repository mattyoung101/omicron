# Omicam
Omicam is Team Omicron's custom, high-performance vision and localisation application that runs on a single board computer.

For the full technical writeup on our vision pipeline, please see our website or "Omicam Design Doc.md" in the docs folder.

**Credits:**
- Matt Young: main C/C++ programmer, Markdown docs, project maintainer
- Ethan Lo: field file generator, localisation research, maths assistance

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

## Building, running and configuration
### Initial setup
While Omicam should in theory work on any Linux-based single board computer with a bit of effort, we use a LattePanda Delta 432
running Xubuntu 18.04. It will only work under Linux.

This guide is written assuming you have booted and installed a Debian-based distro with apt, but the general principle applies to
any package manager or distro.

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
- create_ap: `sudo apt install hostapd`, then [see here](https://github.com/oblique/create_ap) for information.
- OpenCV: You have to build from source, [see here](https://docs.opencv.org/master/d7/d9f/tutorial_linux_install.html).
  Pro tip: if your host computer is Linux-based, use `ssh -Y <username>@<domain>` so you can X-forward cmake-gui to your host.

**Note:** It may be advisable to remove a bunch of the useless packages Ubuntu installs by default such as LibreOffice.

### Network setup
We have tested Ethernet and WiFi to connect to the SBC. Due to the latency requirements of Omicam and Omicontrol, we 
suggest Ethernet as the best approach. While WiFi can be convenient to use, we received significant stuttering and disconnects 
using the LattePanda Delta's built-in WiFi chip. It's possible that there are also some more obscure approaches like Bluetooth 
or USB that may or may not work.

**To configure Ethernet**, we used [this guide](https://superuser.com/a/306703/599535). Essentially, buy an Ethernet crossover
cable and plug it into your computer and the SBC. Then, set your host's Ethernet connection to have a static IP of `10.0.0.1` and
subnet mask of `255.255.255.0`. On your SBC, set the Ethernet static IP to `10.0.0.2` and the subnet mask to `255.255.255.0`.
Make sure you disable DHCP on both ends. Once this is done, you should be good to enter `10.0.0.2` as the IP in Omicontrol/SSH
and connect.

**To configure WiFi**, use [this information](https://github.com/oblique/create_ap/issues/238#issuecomment-292175855) to 
create a permanent WiFi service that starts on boot with create_ap. Generally you'll want to call the hotspot "Omicam01" and "Omicam02" 
for each robot. To connect to the robot, simply connect to the hotspot, run `ifconfig`/`ipconfig` to get the default gateway
and enter that as the IP in Omicontrol/SSH.

### Syncing the time
I'm working on a solution to this problem. Please see Known issues as to why this is important.

### CLion setup
If you don't have CLion, you'll need to download and install it. It's free for students if you have an *.edu email.

Import the project into CLion on your host computer and follow the 
[instructions provided by JetBrains](https://www.jetbrains.com/help/clion/remote-projects-support.html) to configure a 
remote toolchain, deployment and run configuration. The IP should be the IP of your SBC, check your router or use nmap
if you're unsure what this is. You'll probably also want to enable auto upload to remote.

Under Build, Execution & Deployment in CMake, for your Debug and Release configurations add "-G Ninja" to
the CMake options section, to use Ninja as your build runner. If Ninja breaks, simply remove this and delete the caches
to go back to using Unix Makefiles.

Also in Build, Exectuion & Deployment, select the remote toolchain and make the C compiler `/usr/bin/clang` and the C++
compiler `/usr/bin/clang++` if you would like to use Clang (this is probably a good idea, it's what we do).

To run, just use SHIFT+F10 or SHIFT+F9 to debug, like you would normally. CLion will (mostly) handle syncing to the remote
by itself.

### Known issues and workarounds
Important ones are in bold.

- **WiFi is incredibly bad on the LattePanda.** For this reason, if you are using this SBC, we thoroughly recommend Ethernet.
- **Builds will become difficult on the SBC due to clock drift.** [This link](https://stackoverflow.com/a/3824532/5007892)
  has an explanation of what is going on. If you are experiencing weird behaviour, please reimport the CMake project and 
  recompile everything from scratch and it may fix it. We are working on a solution to this problem.
- Last we checked, Clang may be the only supported compiler as it appears that gcc's implementation of Google's Sanitizers 
  doesn't work. This was on ARM a while ago though, and it probably works fine on x86.
- If lldb is broken, try using gdb instead (it doesn't matter that you're compiling with Clang, both will work fine).
- You will need to disable the visual Address Sanitizer output in CLion as that is also broken.
- CLion's remote upload occasionally (a few times per full day of work) fails temporarily, just ignore it and try again.

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

## Special thanks
- Steven G Johnson, for writing the amazing NLopt library
- Tom Rowan, author of the Subplex optimisation algorithm
- Huimin Lu, Xun Li, Hui Zhang, Mei Hu and Zhiqiang Zheng, authors of the paper our localisation algorithm was heavily
  inspired by