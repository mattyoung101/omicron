# OpenCV CUDA Workarounds
_We could avoid all of this if NVIDIA would bloody implement OpenCL on their Tegras. Wouldn't that be nice!_

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