# OpenCV CUDA Workarounds
## Lack of inRange()
We have two options for this. Either we use a combination of existing CUDA functions, notably `threshold`, `bitwise_and`,
`merge` and others to replicate the functionality of `inRange`, assisted by the fact that we're only using RGB not
something complicated like HSV. Or alternatively, we write a custom CUDA kernel that does `inRange` on the GPU.

The good thing is for all of these there are some resources online, notably:

- https://answers.opencv.org/question/91579/opencv-cuda-method-that-works-like-inrange/ (source for option 1)
- https://github.com/opencv/opencv/issues/6295 (source for option 2) 

So for my own implementation, of option 1, here's what we could do:
1. Split up the RGB image into the 3 channels
2. Call `threshold` on each channel to get the binary masks
3. `bitwise_and` R&G channels, then `bitwise_and` that with the `B` channel to do a 3-way bitwise and

**We ended up going with option 2 and it seems to work pretty well.**

## Performance issues
The problem: since `connectedComponentsWithStats()` doesn't run on the GPU, we have a lot of copying to do back and forth
between the CPU and GPU.

The solution: As suggested by TFras, parallelise the shit out of our processing step using CUDA streams. The first step
is, we can queue the resize for whenever because while it's resizing, we can be processing ball and lines as they are
not resized. 

In addition, we can actually just parallelise it so we process each object independently and queue them all
at the same time, then just wait for it to complete before dispatching it to the remote debugger/UART. 

Also, as soon as lines are finished processing, we should immediately dispatch it to the localiser. This will mean checking _inside_
`process_object` rather than inside the frame thread. This should give the localiser a bit of an advantage so it can try
more solutions.

The final step is also we may be able to read and process multiple entire frames at the same time, but I'm not sure if
this will end up being useful because it will screw the FPS counter and we only really want the most recent frame.