# Omicam

_This design document is currently confidential property of Team Omicron and should not be redistributed outside our team._

## Description
This section covers the entire process of how the vision pipeline will work.

### Video decoding and pre-processing
1. Decode frames from raspi camera using Omxcam. When a frame comes in, it will be decoded as RGB and stored in a buffer.
2. Generate an OpenGL texture from the Omxcam RGB buffer and send it off to the GPU to run a fragment shader.
3. The fragment shader will threshold the image and spit out greyscale masks for yellow, blue, orange and white based on thresholds. 
It may be a good idea to convert to another colour space such as YUV (https://stackoverflow.com/a/7902019/5007892) because 
it's easier to detect colours in rather than RGB.
4. The threshold images are then sent back to the CPU and the linked list blob tracker is run to detect the biggest blob for each image.

### Localisation
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

### Transmission
Using nanopb, the biggest orange blob (ball) and localisation data will be encoded into a Protocol Buffer and sent
over UART to the ESP32.

## Live debugging & analysis
We might develop a Kotlin desktop app or webapp that delivers a low quality, low framerate (eg 5 fps) image stream so that
the camera be analysed. You will be able to edit camera settings and threshold values in real time and see their result.

To implement this, we'll save every few frame of RGB video output and encode it using libjpeg-turbo and send it off.
In the Kotlin app we'd use TCP sockets and maybe Protobuf and on the webapp just use JS to refresh an image.