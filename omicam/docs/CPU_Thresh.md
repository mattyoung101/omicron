# CPU Thresholding Performance

## Scalar (no NEON)
Scalar optimised (release): **20 fps**

Scalar unoptimised (debug): **3 fps** (and memory leak? believe because it's too slow to catch up on backlog)

## Vector (NEON)
Forgot the exact readings, but it didn't help much

## Multi-threaded (no NEON)
Unoptimised (debug mode): 20 fps

Optimised (release mode): ~60 fps, negligible (if any) loss