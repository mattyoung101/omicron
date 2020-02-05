## Cool field art
All of these were created mostly by accident while trying to test the implementation of the objective function
for the localisation algorithm.

## What they are
- gradients.bmp: each time you finish raycasting for a grid cell on the field, do this:
```c
image[(int32_t) (point.x + field.length * point.y)] = (uint8_t) (x + y);
```
- lights.bmp: raycast for all points, and set the value to be the number of line points there are 