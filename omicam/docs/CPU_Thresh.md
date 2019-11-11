# CPU Thresholding Performance
Accurate as of 9 November 2019, on Raspberry Pi Model 3 B+ with no clock settings changed.

## Multi-threaded (no NEON)
This is the currently selected implementation.

|CMake mode | Colours             | Framerate | Thread count |
|-----------|---------------------|-----------|--------------|
| Debug     | Ball only           | 20        | 4            |
| Debug     | Ball, goals         | 15        | 4            |
| Release   | Ball only           | 60        | 4            |
| Release   | Ball, goals         | 43        | 4            |
| Release   | Ball, goals, line   | 35        | 4            |

### Additional optimisations added
- Remove the memcpy in blob_detector_post that copies the MMAL frame buffer to receivedFrame. Instead just set receivedFrame's
pointer to the buffer header's data pointer. Because receivedFrame is read only, there is no need to copy it. With ball and goals,
this gets 17 fps in debug mode and **56 fps in release mode.**

## Historical implementations
### Scalar (no NEON)
Scalar optimised (release): **20 fps**

Scalar unoptimised (debug): **3 fps** (and memory leak? believe because it's too slow to catch up on backlog)

_I may be wrong but I think scalar unoptimised got faster (to around 20 fps?) and scalar optimised capped out at around 40._

### Vector (NEON)
Forgot the exact readings, but it didn't help much.