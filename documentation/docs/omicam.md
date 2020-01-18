# Omicam 
One of the biggest innovation Team Omicron brings this year is our advanced, all-in-one, custom developed vision and
localisation application called _Omicam_. The application is developed mostly in C, with some C++ code to interface with
OpenCV. It runs on the powerful LattePanda Delta 432 single board computer and uses Linux (Xubuntu 18.04) as its OS.

We are proud to report that Omicam is a significant step up compared to previous vision applications in use at BBC
Robotics, as you will see below.

Omicam consists of **X** lines of code, and took about **X** hours to develop.

## Background and previous methods
Intelligent, accurate and fast computer vision continues to become increasingly important in RoboCup Jr Open Soccer.
With the introduction of the orange ball and advanced teams' application of "ball-hiding" strategies, accurate and fast
computer vision is now one of the most important elements of a successful RoboCup Jr Open robot.

Previously, our team used an OpenMV H7 to provide vision. This is a module which uses an STM32 MCU combined combined with
an OmniVision camera module to provide low-resolution vision in an easy-to-use MicroPython environment. However, although
this approach is functional, its resolution and framerate are extremely limiting for our use case. Hence, we decided the
best solution was to do what the most advanced teams were doing, and develop a custom vision application running on a 
single board computer (SBC).

**TODO cover Omicam development with other SBCs**

## Performance and results
Omicam is capable of detecting 4 field objects at **over 60fps at 720p (1280x720) resolution**. Compared to the previous
OpenMV H7, this is **12x higher resolution at 3x the framerate.<sup>1</sup>**

In addition, using the novel vision-based localisation algorithm we developed this year, we can now determine the
robot's position to **~1cm accuracy** at realtime speeds. This is over **5x/25x more accurate<sup>2</sup>** than any previous methods 
used at BBC Robotics, and has been shown to be much more reliable and stable.

To achieve this performance, we made heavy use of parallel programming techniques, OpenCV's x86 SIMD CPU optimisations,
Clang optimiser flags as well as intelligent image downscaling for the goal threshold images (as they are mostly unused).
In addition, the localisation, vision and remote debug pipelines all run in parallel to each other so the entire application
is asynchronous.

_<sup>1 results based on mediocre lighting conditions running well optimised OpenMV H7 code at QVGA resolution.</sup>_ 

_<sup>2 depending on whether LRF based/goal based localisation was used.</sup>_

## Hardware
Omicam runs on a Linux-based LattePanda Delta 432.

**More here, camera and stuff.**

## Field object detection
The primary responsibility of Omicam is to detect field objects: the ball, goals and also lines. To do this, we use the 
popular computer vision library OpenCV, v4. We use Linux's UVC driver to acquire an MJPEG stream from the USB camera. 
Then, we apply any pre-processing steps such as downscaling the goal frames and gamma boosting.

Next, we threshold all objects in parallel to make use of our quad-core CPU, using OpenCV's thresholding function but a custom
parallel framework (as it doesn't run in parallel by default). 
Then, we use OpenCV's parallel connected component labeller, specifically the algorithm by Grana et al[1] to detect regions
of the same colour in the image. The largest connected region will be the field object we are looking for.

We then dispatch the largest detected blob's centroid via UART to the ESP32, encoded using Protocol Buffers.

**TODO describe in more detail**

## Localisation
Localisation is the problem of detecting where the robot is on the field. This information is essential to know in order
to develop advanced strategies and precise movement control, instead of just driving directly towards the ball.

### Previous methods
Currently, teams use three main ways of localisation. Firstly, the simplest approach uses the detected goals
in the camera to estimate the robot's position. This approach is very inaccurate because of the low resolution of
most cameras (such as the OpenMV), the fact that there are only two goals to work with as well as the fact that sometimes
the goals are not visible (especially in super team). Expected accuracy is 15-25cm.

The second approach in use is based on distance sensors such laser range finders (LRFs) or ultrasonic sensors. By using
several of these sensors on a robot, the position of the robot can be inferred with trigonometry, by measuring the distance
to the walls. This approach has a major drawback: it is impossible to reliably distinguish between anomalous objects,
such as a hand or another robot, compared to a wall. This means that although this approach is somewhat accurate on an empty
field, it is very difficult to use reliably in actual games. Thus, localisation data was almost never trusted by teams
who used this approach and so is not very helpful. In addition, distances sensors, especially ultrasonics, are slow and suffer 
from inaccuracy at long distances. Expected accuracy is 5-10cm, but data is invalidated every time a robot moves in the way of a sensor,
which is impossible to know.

The third approach in use by some teams is one based on 360 degree LiDARS. These are expensive, heavy, difficult to use, slow and are
still vulnerable to all the problems the second approach suffers from as well. We are not aware of expected accuracy, but regard
this as not an ideal approach.

### Our solution
This year, Team Omicron presents a novel approach to robot localisation based on a middle-size league paper by Lu, Li, Zhang,
Hu & Zheng (2013). We localise using purely RGB camera data by solving a non-linear optimisation problem using the lines on the
playing field.

In practice, this works by the vision processor passing its thresholded line data to the localiser. Using its worker threads,
a certain number of rays are casted on the line image using Bresenham's line algorithm[3] to find every time a ray intersects
a line (these are called "line points"). Using a manually determined equation, these points are then dewarped to counter
the distortion of the 360 degree mirror.

To localise, the Subplex[4] non-linear optimiser (part of the NLopt package[5]) is used to minimise an objective function that 
**TODO describe what it does in simple terms.**

In simple terms, we optimise by moving a virtual robot around on a field and comparing the distances between what it might
see and what we _really_ see to "reverse-engineer" the (x,y) position of the real robot.

## Interfacing with Omicontrol
To interface with our remote management application Omicontrol, Omicam starts a TCP server on port 42708. This server sends
Protocol Buffer packets containing JPEG encoded frames, zlib compressed threshold data as well as other information such as
the temperature of the SBC.

We use the SIMD optimised libjpeg-turbo to efficiently encode JPEG frames, so as to not waste performance to the remote debugger
(which is disabled during competition). Instead of compressing threshold frames with JPEG, because they are 1-bit images,
it was determined that zlib could compress them more efficiently (around about 460,800x reduction in size). 

With all these optimisations, even at high framerates (60+ packets per second), the remote debug system only uses 300KB/s to 1 MB/s 
of network bandwidth.

## Debugging
Low-level compiled languages such as C and C++ are notoriously unstable. In order to improve the stability of Omicam
and fix bugs, we used Google's Address Sanitizer to easily find and trace a variety of bugs such as buffer overflows,
memory leaks and more. In addition, we used the LLVM toolchain's debugger lldb to analyse the application frequently.

To assist in performance evaluation, we used the Linux tool OProfile to determine the slowest method calls in the application.

## References
**TODO USE MENDELEY**

[1] C. Grana, D. Borghesani, and R. Cucchiara, “Optimized Block-based Connected Components Labeling with Decision Trees,” IEEE Transac-tions on Image Processing, vol. 19, no. 6, pp. 1596–1609, 2010.

[2] **TODO CITE PROPERLY** Robust and Real-time Self-Localization Based on Omnidirectional Vision for Soccer Robots

[3] Bresenham's paper

[4] Subplex paper

[5] NLopt paper
