# Omicam
Omicam is Team Omicron's custom, high-performance vision and localisation application that runs on a single board computer.

For the full technical writeup on our vision pipeline, please see [the relevant page on our website](https://teamomicron.github.io/omicam/).

**Credits:**
- Matt Young: main C/C++ programmer, Markdown docs, project maintainer
- Ethan Lo: field file generator, initial localisation research, maths assistance

## Features list
- High performance field object detection (ball, lines, goals) using OpenCV 4
    - Colour thresholding followed by connected component labelling (CCL)
    - Uses parallel programming to make most use of multi-core CPUs
    - Works with localiser to accurately determine absolute positions of field objects
- Novel localisation algorithm using a sensor-fusion/non-linear optimisation method, accurate to ~1.5cm
    - Constructs an initial estimate using fast but inaccurate methods, then refines it by solving a 2D non-linear minimisation problem.
    - Initial estimate is used to constrain and seed optimisation stage for faster and more stable results
    - Support for arbitrary number of field geometries by defining field file (Australian fields, SuperTeam, etc)
    - Dynamic moving average smoothing based on robot velocity with configurable parameters
- Feature rich remote debugging and visualisation module. Viewable in custom Kotlin/JavaFX desktop app (Omicontrol).
    - Works locally using WiFi or Ethernet, and even remotely over the Internet
    - Camera frame streaming over TCP/IP using SIMD accelerated libjpeg-turbo
    - Custom network protocol uses Protocol Buffers with zlib compression
    - Transmits detailed information about robot position, debug info, field object bounding boxes, etc.
    - Receives and executes commands from remote (e.g. switch objects, save config, etc)
- Efficient replay file format using Protocol Buffers
    - Encodes robot data (localised position, orientation, FSM state, etc) at 30 Hz into a Protobuf file
    - Fault tolerant: file is periodically re-written every 5 seconds
    - Should be able to encode around ~45 minutes of data in around ~16 MB
- UART communication to ESP32 also using Protocol Buffers format
- INI config files can be reloaded from disk
- Written mostly in C11 (just a little C++ for interfacing with OpenCV)
- Well-documented code (see our website for a full writeup on the process as well)

## Building, running and configuration
### Initial setup
Omicam will probably only run under Linux. At Team Omicron, we use a LattePanda Delta 432 SBC running Xubuntu 18.04, so
that's the only setup we can confirm it works reliably under.
In saying that, there's no reason it shouldn't work with any other SBCs (except maybe for UART stuff).

This guide is written assuming you have booted and installed a Debian-based distro with apt, but the general principles applies to
any package manager or distro.

- CMake: To work around various issues (see below), a newer version of CMake than the one provided by the Ubuntu repos is provided.
  The easiest way to install the latest CMake, in my opinion, is to [add the PPA](https://apt.kitware.com/) and then just 
  `sudo apt install cmake`.
- Clang & LLVM: `sudo apt install clang lldb llvm libasan5 libasan5-dbg` **TODO consider removing, use gcc instead**
- NLopt: `sudo apt install libnlopt-dev libnlopt0`
- libjpeg-turbo: `sudo apt install libturbojpeg libturbojpeg0-dev`
- gstreamer: [see here](https://gstreamer.freedesktop.org/documentation/installing/on-linux.html) and also 
  `sudo apt install libgstreamer-opencv1.0-0 libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev`
- ffmpeg: `sudo apt install ffmpeg libavformat-dev libavcodec-dev libswscale-dev libavresample-dev`
- GTK devlopment files: `sudo apt install libgtk2.0-dev`
- create_ap: `sudo apt install hostapd`, then [see here](https://github.com/oblique/create_ap) for information.
- NTP tools: `sudo apt install ntpd sntp ntpstat`
- OpenCV: You have to build from source, [see here](https://docs.opencv.org/master/d7/d9f/tutorial_linux_install.html).
  Pro tip: if your host computer is Linux-based, use `ssh -Y <username>@<domain>` so you can X-forward cmake-gui to your host.

**Note:** It may be advisable to remove a bunch of the useless packages Ubuntu installs by default such as LibreOffice.

### LattePanda specific instructions
Because the UART port is owned by the ATMega32U4, you need to download and install Arduino to upload the `PandaUARTHack`
sketch which forwards UART I/O. Follow the Linux Arduino install instructions [here](https://www.arduino.cc/en/guide/linux)
and **make sure you definitely run the Linux setup shell script** or it will not be possible to upload to the device.

### Network setup
We have tested Ethernet and WiFi to connect to the SBC. Due to the latency requirements of Omicam and Omicontrol, we 
suggest Ethernet as the best approach. While WiFi can be convenient to use, we received significant stuttering and disconnects 
using the LattePanda Delta's built-in WiFi chip. It's possible that there are also some more obscure approaches like Bluetooth 
or USB that may or may not work.

**To configure Ethernet**, we used [this guide](https://superuser.com/a/306703/599535). Essentially, find an Ethernet crossover
cable and plug it into your computer and the SBC. Then, set your host's Ethernet connection to have a static IP of `10.0.0.1` and
subnet mask of `255.255.255.0`. On your SBC, set the Ethernet static IP to `10.0.0.2` and the subnet mask to `255.255.255.0`.
Make sure you disable DHCP on both ends. Once this is done, you should be good to enter `10.0.0.2` as the IP in Omicontrol/SSH
and connect.

**To configure WiFi**, use [this information](https://github.com/oblique/create_ap/issues/238#issuecomment-292175855) to 
create a permanent WiFi service that starts on boot with create_ap. Generally you'll want to call the hotspot "Omicam01" and "Omicam02" 
for each robot. To connect to the robot, simply connect to the hotspot, run `ifconfig`/`ipconfig` to get the default gateway
and enter that as the IP in Omicontrol/SSH.

### Syncing the time
Because of the fact that `make` decides which files to rebuild based on the modification time, it's very important that
the SBC has an accurate time source. This is made difficult due to the fact that the SBC is offline almost all of
the time. See [this link](https://stackoverflow.com/a/3824532/5007892) for more information.

There are two ways to solve this issue: you can either put a coin cell battery in the SBC and sync the time once,
or for a consistently accurate time source with no Internet required, set up a local NTP server on your host computer.

#### Windows host
Download and install [NetTime](http://www.timesynctool.com/) and make sure it's installed as a system service. Click
"Update Now" to sync the time, then click "Settings" and check both "Allow other computers to sync to this computer"
and "Always provide time" (ignore the warning).

Make sure you unblock UDP port 123 in Windows Firewall as both incoming and outgoing connections.

#### Linux host
Currently untested, you will want to install an NTP daemon like ntpd and then do some configuration to host a local 
NTP server. Google is your friend.

#### Mac host
There's no way to do this on a Mac as far as I'm aware. Please sync with a Linux or Windows host beforehand.

### Synchronisation
Once you have an NTP server running on your host machine, run `sudo sntp -S 10.0.0.1` over SSH on the SBC to sync
the time. It may also be possible to use ntpd to do this automatically but it hasn't been researched yet.

### CLion setup
At Team Omicron, we use the CLion IDE to work with the project, in part due to its remote project support. However,
it should work with any other IDE that supports CMake projects.

Import the project into CLion on your host computer and follow the 
[instructions provided by JetBrains](https://www.jetbrains.com/help/clion/remote-projects-support.html) to configure a 
remote toolchain, deployment and run configuration. The IP should be the IP of your SBC, check your router or use nmap
if you're unsure what this is. You'll probably also want to enable auto upload to remote.

If you want, you can use the Ninja build tool, just install it on the SBC and add `-G Ninja`, though we didn't find any
performance benefits doing this.

Also in Build, Exectuion & Deployment, select the remote toolchain and make the C compiler `/usr/bin/clang` and the C++
compiler `/usr/bin/clang++` if you would like to use Clang.

To run, just use SHIFT+F10 or SHIFT+F9 to debug, like you would normally. CLion will handle syncing to the remote
by itself.

### Build modes
TODO compare release and debug.

### Running in production
Once development is finished, you should install Python 3 and add `run.py` to your system's auto-start scripts.
On Xfce, this can be done in the GUI. This will ensure that in a competition, Omicam is automatically started on
boot without you having to use ssh or CLion.

### Known issues and workarounds
Important ones are in bold. Most of these are covered above.

- **Robot detection is bugged.** This feature was mainly added as an experiment and has not yet been finished.
- **Some comms packets remain unimplemented.** I have not yet added or tested most robot movement packets such as `CMD_MOVE_TO_XY` 
or `CMD_ORIENT`, this will most likely not work yet.
- **Replays may not work.** They or may not work, I haven't tested this yet because the Omicontrol replay UI isn't
finished.
- **INI config reloading is sketchy.** Some features like mirror crop will work, others won't. This is mainly obscure implementation
details, that we haven't fixed since we don't use INI reloading too much.
- **If Arduino can't upload, don't panic!** It's just being dumb. You need to run the setup script which disables the modem
monitor thingy which runs by default and interferes with UART. Then try about 5 times and it should work eventually.
- If gcc's sanitizers don't work, try using clang intsead. This was an old problem using ARM boards, so it should be fine on x86.
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

## Special thanks to...
- Riley Bowyer at CSIRO, for his great advice and theory in regards to the localiser, specifically with stability and
performance. Cheers.
- Angus Scroggie for lots of help with the maths related to robot detection.
- The contributors to the NLopt library, especially Steven G Johnson the initial author, and Tom Rowan for his PhD
thesis describing the Subplex algorithm.
- Huimin Lu, Xun Li, Hui Zhang, Mei Hu and Zhiqiang Zheng, authors of the paper we initially read for ideas on how to localise
properly.
- Everyone on Team Omicron! You guys are all the best teammates, and wouldn't be remotely possible without you all <3
