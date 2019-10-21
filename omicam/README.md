# Omicam

This is the custom vision system for our team. It runs on a Raspberry Pi Compute Module 3+ with a Pi Camera. For details
on how the vision system works, please see docs/DESCRIPTION.md.

## Building and running
JetBrains CLion is the only supported IDE for working with Omicam. 
You have two options to work with the code: remote mode and cross compilation. Remote mode is currently the easiest way
to work with the project. If you don't have a Pi on you then you could try cross compilation, but this isn't supported.

### Remote mode
Firstly, you'll need a board compatible with the Raspi Compute Module 3+ and Pi Cam v2.1. I use a Raspi 3 Model B and
Pi Cam v1.3.

Flash your Pi's SD card with Raspbian Lite, boot and update it, then install the following packages:
- CMake: `sudo apt install cmake`
- [NLopt](https://NLopt.readthedocs.io/en/latest/): follow the instructions linked
- libjpeg-turbo: `sudo apt install libturbojpeg0 libturbojpeg0-dev`

Import the project into CLion on your host computer and follow the instructions 
[provided by JetBrains](https://www.jetbrains.com/help/clion/remote-projects-support.html) to setup a remote toolchain
and run configuration. The remote settings are synced in Git, but you may need to modify the IP address to that of your
local Pi. For some reason it doesn't seem that `raspberrypi.local` works.

To run, just use SHIFT+F10 like you would normally. CLion will handle the rest for you. If you install a new library on
the host, you will need run Tools->Resync with remote hosts.

### Cross-compilation
**Warning: this is mostly untested and no longer supported.**

You can get a Raspi cross compiler
[here](https://github.com/abhiTronix/raspberry-pi-cross-compilers/). We use `cross-gcc-9.1.0-pi_3+`.

You'll also need to acquire the `opt/vc` VideoCore SDK that comes with the Pi and put it in your system at the same path.
To get this folder, download and mount the September 2019 Raspbian image (may work with other versions, untested)
by following [this guide](https://github.com/mozilla-iot/wiki/wiki/Loop-mounting-a-Raspberry-Pi-image-file-under-Linux).

Install NLopt and libjpeg-turbo on your host as you'd do on the Pi under remote mode.

Import the project into CLion and then create a new Raspberry Pi toolchainusing `gcc` and `g++` that come with the cross 
compiler toolchain.

To run, you'll need to upload the generated binary from `cmake-build-[debug/release]-raspberrypi` to your Pi and execute.

## License
Omicam is available under the main project license, see LICENSE.txt in this directory or the root directory.

## Libraries used
- [omxcam](https://github.com/gagle/raspberrypi-omxcam): MIT license
- [log.c](https://github.com/rxi/log.c): MIT license