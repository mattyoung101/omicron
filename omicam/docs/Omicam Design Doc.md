# Omicam

**ATTENTION: this is outdated, especially the localisation section. You'd be better of checking our website instead.**

This document describes in detail how all the systems (pipelines) in Omicam function.

## Pipelines
The camera system is made up of several interconnected pipelines. The vision pipeline is used to get the frame from the 
camera and detect different field objects, such as the ball, the goals and the lines. Some of this information is then 
passed onto the localisation pipeline, whose job it is to estimate the 2D position of the robot on the field. 
The final pipeline is the remote debug pipeline which sends information to Omicontrol.

## Vision pipeline
The vision pipeline relies heavily on OpenCV to do the heavy lifting.

1. Read in the camera frame using gstreamer using OpenCV video capture
2. Pre-process image (downscale, convert to RGB, etc)
2. Apply all the thresholds to generate 1-bit greyscale masks of each field object
3. Detect the largest ball "blob", find the centroid and send this to the ESP32.
4. Send the mask of the lines to the localiser.
5. Send threshold mask, largest blob bounding box and centroid to remote debug pipeline.

The reason why we send the ball data separately to the localisation data is that the localiser is much slower, and we
can work without a localisation position temporarily but we cannot work without a ball position.

## Localisation pipeline
**THIS IS OUT OF DATE**

1. Using the MSL approach found by Ethan, rays are cast from the centre of the image detecting all points it hits the line. 
We will use Bresenham's line drawing algorithm to check each pixel.
2. Record and linearize all line intersection points.
3. Using NLopt, apply the Nelder-Mead simplex optimisation algorithm to optimise an (x, y) coordinate which makes the error
of the difference between virtual and real lines the smallest (i.e. find similar lines on the virtual field, and by doing so,
we will find us).

With this, we may wish to use the mouse sensor to determine a rough start location for our new optimisation after the
initial one. We could also just use the last point that we used.

Optimisation is terminated after the simplex doesn't move far enough or a certain estimated accuracy (how?) is achieved
or after a certain number of max steps.

## Remote debug pipeline
The remote debug pipeline is a highly asynchronous pipeline which handles encoding information from the camera into
Protocol Buffers which can be read and displayed by Omicontrol. The connection is handled over TCP. The debug pipeline
consists of the following threads:
- Frame encode thread: the main remote debug thread, listens for incoming information as well as encoding and sending
the data into Protobufs.
- Thermal thread: reads the CPU's temperature periodically.
- TCP thread: awaits the Omicontrol connection (in its own thread so it doesn't block the main)

This is how the pipeline looks:
1. On `remote_debug_post()`, package and send data to work queue to later be read by frame encode thread.
2. The frame thread periodically checks for data in its work queue, and if there is some processes it. Otherwise,
it checks for incoming messages from Omicontrol and if there are any, decodes them, processes them and sends a response.
3. If there is data in the work queue: compress the RGB frame image using libjpeg-turbo, compress the threshold mask
using zlib, add extra information to Protobuf and send.

The thermal thread is a trivial thread which periodically polls the Linux special file `/sys/class/thermal/thermal_zone0/temp`
to determine the CPU temperature. 

The TCP thread is a trivial `bind()`, `listen()`, `accept()` loop very common in networking.

Some information about zlib encoding for the threshold frame: I observed that this data is a great target for compression
with something like zlib, rather than an image encoding library, due to the huge amount of repeated data which is a
great target for run length encoding. Zlib has more advanced compression than RLE, and we can compress a 921KB image
to only about 1-2KB.

## Other information
### Transmission
Protocol Buffers are extensively used to transmit information to and from the ESP32 and Omicontrol. Nanopb is the
library used for this. There is a Fish (shell) script to compile it automatically.

### Performance notes
- We make make many call to `malloc` and `free` which in theory could cause memory fragmentation, but I haven't
observed that yet. I assume the glibc allocator is pretty smart about this.