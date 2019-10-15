# Omicam

This is the custom vision system for our team. It runs on a Raspberry Pi Compute Module 3+ with a Pi Camera. For details
on how the vision system works, please see docs/DESCRIPTION.md.

## Building and running
### Installing dependencies
We ship a pre-compiled version of omxcam as `libomxcam.so` in the lib directory because compiling it is kind of annoying
and pointless since it hasn't been updated in a very long time.
You can get omxcam's source here [here](https://github.com/gagle/raspberrypi-omxcam) if you need to modify it (just replace
libomxcam.so with your own version).

You'll also need to acquire the `opt/vc` VideoCore SDK that comes with the Pi and put it in your system at the same path.
To get this folder, download and mount the September 2019 Raspbian image (may work with other versions, untested)
by following [this guide](https://github.com/mozilla-iot/wiki/wiki/Loop-mounting-a-Raspberry-Pi-image-file-under-Linux).
We don't currently distribute this with the project because it's easy enough to install and would bloat the repo with a 
bunch of binaries and headers.

Currently, builds are cross compiled on a Linux computer and uploaded to the Pi. You can get a Raspi cross compiler
[here](https://github.com/abhiTronix/raspberry-pi-cross-compilers/). We use `cross-gcc-9.1.0-pi_3+`.

### Importing the project
Development is done in CLion. You'll need to import the project, and then create a new custom Raspberry Pi toolchain
using `gcc` and `g++` that come with the cross compiler toolchain.

### Running
To run, you'll need to upload the generated binary from `cmake-build-[debug/release]-raspberrypi` to your Pi and execute.

## License
Omicam is available under the main project license, see LICENSE.txt in this directory or the root directory.

## Libraries used
- [omxcam](https://github.com/gagle/raspberrypi-omxcam): MIT license
- [log.c](https://github.com/rxi/log.c): MIT license