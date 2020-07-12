# SBC
**TODO: this section should be refactored a lot with intro sections and everything**

## Hardware
Omicam supports any single board computer (SBC) that can run Linux. In our case, we use a LattePanda Delta 432 with a 
2.4 GHz quad-core Intel Celeron N4100, 4GB RAM, 32GB of storage, WiFi, Bluetooth, gigabit Ethernet and a UART bus.

The current camera we use is an e-con Systems Hyperyon, based on the Sony Starvis IMX290 ultra low-light sensor capable
of seeing in almost pitch black at high framerates. This is a USB 2.0 camera module, since the LattePanda has no 
MIPI port.

### SBC iterations
The current iteration of Omicam's SBC setup is the cumulation of around 2 years of prototyping iterations in both
hardware and software approaches.

**Prototype 1 (December 2018-January 2019):** This consisted of a Raspberry Pi Zero W, with a 1 GHz single-core CPU.
It was our initial prototype for a single-board computer, however, we quickly found it was far too weak to do any vision,
and our inexperience at the time didn't help. Thus, we canned the SBC project for around a year.

**Prototype 2 (December 2019):** After resurrecting the SBC R&D project for our 2020 Internationals, we started development
with the Raspberry Pi 4. This has a 1.5 GHz quad-core ARM Cortex-A72 CPU. We began developing a custom computer vision
library tailored specifically to our task, using the Pi's MMAL API for GPU-accelerated camera decoding. Initial results
showed we could threshold images successfully, but we believed it would be too slow to localise and run a
connected-component labeller in real time.

**Prototype 3 (January 2020):** Next, we moved onto the NVIDIA Jetson Nano, containing a 1.43 GHz quad-core Cortex-A57,
but far more importantly a 128-core Maxwell GPU. At this time we also switched to using OpenCV 4 for our computer vision.
In theory, using the GPU for processing would lead to a huge performance boost due to the parallelism, however, in practice
we observed the GPU was significantly slower than even the weaker Cortex-A43 CPU, (presumably) due to copying times. We
were unable to optimise this to standard after weeks of development, thus we decided to move on from this prototype.

**Prototype 4 (January-February 2020):** After much research, we decided to use the LattePanda Delta 432. The OpenCV
pipeline is now entirely CPU bound, and despite not using the GPU at all, we are able to achieve good performance.

### Camera module iterations
In addition to the SBC, the camera module has undergone many iterations, as it's also an important element in the 
vision pipeline.

**Pi Camera:** The initial camera we used in hardware prototypes 1-3, was the Raspberry Pi Camera. 
In prototype 1, we used the Pi Cam v1.3 which is an OV5647 connected via MIPI, and in prototypes 2 and 3 we used the 
Pi Cam v2 which is an IMX219 again connected via MIPI. We had to drop this camera in later iterations because
the LattePanda doesn't have a MIPI port. Both of these cameras were capable of around 720p at 60 fps.

**OV4689-based module:** We experimented with a generic OV4689 USB 2.0 camera module from Amazon, which is capable of
streaming JPEGs (aka an MJPEG stream) at 720p at 120 fps (we could get around 90 fps in practice with no processing).
While this framerate was useful, the camera module suffered from noise and flickering in relatively good
lighting conditions, so it was dropped.

**e-con Hyperyon:** After these two failures, we began looking into industrial-grade cameras to use on
our robot. While most of these, from companies like FLIR, are out of our price range, we found e-con Systems as a
relatively affordable vendor of high quality cameras. We narrowed down our selection to two devices: the
See3CAM_CU30 USB 2/USB 3 2MP camera module, which is fairly standard and affordable, as well as the more expensive
Hyperyon described above. After testing both, we decided the Hyperon fit our needs better, despite its higher latency,
due to its extremely good low light performance.