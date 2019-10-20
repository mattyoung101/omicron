# Omicam

_This design document is currently confidential property of Team Omicron and should not be redistributed outside our team._

## Vision pipeline
This section covers the entire process of how the vision pipeline will work.

### Video decoding and pre-processing
1. Decode frames from raspi camera using Omxcam. When a frame comes in, it will be decoded as RGB and stored in a buffer.
2. Generate an OpenGL texture from the Omxcam RGB buffer and send it off to the GPU to run a fragment shader.
3. The fragment shader will threshold the image and spit out greyscale masks for white (lines) and orange (ball). We could
try using YUV colour space (convert in shader) as well for thresholds.
4. The framebuffer is copied back from the GPU into a byte array.

### Blob detection
Originally, since we were tracking orange, blue, yellow and white, I was going to implement the algorithm described
by Acevedo-Avila, Gonzalez-Mendoza and Garcia-Garcia (2016) appearing in the Sensors journal, but because we're only
tracking orange (the localisation doesn't use blobs), I am going to implement a custom algorithm instead.

1. Use a stack-based scanline flood fill to segment blobs in an image (this might be able to be parallelised)
2. As we scan through the image, record the lowest and highest diagonal point in the blob (lowest x+y, highest x+y)
3. Optionally merge rectangles together (only if they're small).
4. Select the biggest blob and return it's centre, this is the ball's relative position to the robot.

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

We will send localisation as a separate Protobuf message to ball detection, because both run in parallel. Otherwise,
we'd have to wait for both to finish before sending them meaning the fastest is only as fast as the slowest.

## Live tuning & analysis
We might develop a Kotlin desktop app or webapp that delivers a low quality, low framerate (eg 5 fps) image stream so that
the camera be analysed. You will be able to edit camera settings and threshold values in real time and see their result.

To implement this, we'll save every few frame of RGB video output and encode it using libjpeg-turbo and send it off.
In the Kotlin app we'd use TCP sockets and maybe Protobuf and on the webapp just use JS to refresh an image.

## Performance notes
- Obviously, we use OpenMAX IL (via omxcam) to use the GPU to accelerate camera frame acquisition and decoding. Past experience
has shown that the default V4L2 driver is obnoxiously slow (~1 FPS).
- Thresholding is done on GPU as its pixel processing rate is faster than the CPU.
- The fragment shader should record the location of the first thresholded pixel, so that the flood filler can just jump to that 
position instead of scanning over the whole image.
- We might be able to parallelise scanline flood fill, or run it on the GPU (this would be extremely difficult and not
really worth the cost imo)
- We have 4 cores to work with, since we run localisation and blob detection at the same time (they are independent processes),
we can safely use two CPU cores per task. We could simply halve the image and run both bits in parallel then.
- It's safe to create lots of pthreads, we don't need a thread pool. The overhead should only be about 10us according
to StackOverflow **(FIXME: ADD LINK WHEN INTERNET RETURNS)**

Biggest bottlenecks are going to likely be:
1. CPU flood filler, especially if many blobs in image
2. Copying data to and from GPU (in theory we have shared RAM so it shouldn't matter too much?)