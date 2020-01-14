# Omicam
## Project goals
Develop an advanced single board computer-based camera module, which can track the ball and two goals with almost zero overhead (thereby limiting us only to the camera's framerate), while also being highly accurate. In future, I would also like to dewarp the camera's conical image into a rectangular one but this is difficult and computationally expensive. Currently going to use Raspberry Pi.

## Implementation details
- linked list based blob tracking (there are some papers on this)
- base it on quickblob?
- divide and conquer - use multiple CPU cores - use OpenMP!!
- needs to support multiple blobs 
- image pyramid (progressive enhancement)
- fast blurring algorithm? blur to reduce noise - box blur??
- GPU acceleration if possible (platform specific, in this case, no OpenCL so we'll use MMAL/OpenMAX for frame decoding and OpenGL ES for other image operations like blurring)
- write it in pure C without OpenCV (maybe use CCV?)
- run it on Raspi 4
- investigate GPU accelerated/multi-threaded/in general fast frame decoding from camera (OpenMAX?) - this will vary per chip, so we'll have to look into ways to do it based on the Raspi's GPU, I saw some stuff on this before
- need to setup a git pull and build toolchain on the pi, or a cross compiling toolchain on windows
- use YUV colour space!

## Steps
1. Decode camera frames into a YUV buffer, probably using raspberrypi-omxcam (or raw MMAL, OpenMAX or GStreamer/ffmpeg as C libraries, or RaspiVid if worst comes to worst) - makes extensive use of GPU
2. Pre-process image: brighten, blur, generate image pyramid, posterize, etc - all of these should be OpenGL ES fragment shaders, see https://raspberrypi.stackexchange.com/a/52
3. Run linked list blob tracking algorithm - try multi threading this if possible
4. Post results to ESP32 over UART

## Colour space idea
One of the links below gave me an idea: given that we know that there's only so many colours that should be on the field (green, white, orange, yellow, blue, black [for robots]), why don't we for each pixel compute the closest colour from our colour palette and then, select only say the classified orange pixels and then run simple blob detection on that? Saves blob detection on RGB images.

This can be achieved through posterizing, and can be implemented in a GPU shader for speed! Posterizing works much better than the above approach, see scripts/cam_test

### Observations from experimenting
In Photoshop, convert RGB to LAB. Then, disable the lightness layer (leaving only A & B enabled). Suddenly, we have everything highlighted we needed: background objects are grey, the field is green, the ball is bright orange, and the goals are decently coloured too. _Unfortunately, the goal colours are probably too dark for this to work properly._

## Colour normalisation
Apparently can be used to remove lighting from objects? Some links in my bookmarks, main one is: http://aishack.in/tutorials/normalized-rgb/

## Links
- https://github.com/gagle/raspberrypi-omxcam
- https://github.com/tjormola/rpi-openmax-demos
- https://elinux.org/Raspberry_Pi_VideoCore_APIs
- http://home.nouwen.name/RaspberryPi/documentation/ilcomponents/camera.html (looks like OpenMAX/MMAL firmware docs or something)
- http://www.jvcref.com/files/PI/documentation/html/index.html (MMAL docs)
- http://robotex.ing.ee/2012/01/pixel-classification-and-blob-detection/ (blob tracking ideas)
- https://jan.newmarch.name/RPi/ (book on raspi GPU programming)
- https://github.com/Apress/raspberry-pi-gpu-audio-video-prog (may be mildly useful, GPU programming)
- https://elinux.org/Rpi_Camera_Module
- https://picamera.readthedocs.io/en/release-1.13/fov.html#sensor-modes (description of camera hardware - many other goods docs related to picam and MMAL on this site)
- http://aishack.in/tutorials/normalized-rgb/ (colour normalisation)