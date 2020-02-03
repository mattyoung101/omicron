## Matt's objective function idea

### Initial steps
1. In the field file generator, render a bitmap where a black pixel indicates no line and a white pixel indicates
a line.

### Raycasting
1. Move to the localiser guessed position on the field
2. Use Bresenham's algorithm to cast out 64 rays just like the original camera image, but cast them on the above
generated field instead. No dewarping is required.
    - This acts as our guide for what the robot _should_ theoretically see if it were to be in the guessed position.
    - This generates a certain number of line points, could be more or less than the original depending on field
    position and line thresholding.

### Similarity calculation
Consider the following:

- Set A has N points
- Set B has N + Q points
- both N and Q are arbitrary real numbers

Note that either set A or set B could be the one raycasted from the camera or the field, in this case we just consider
that set B is the one with more points than set A.

Usually this would be trivial if we could guarantee both set A and B had N points, but unfortunately we cannot, so we
must devise a way to get an accurate sample of the points no matter how big or small each set is compared to each
other. For example, if we were to just iterate around and compare each point in set B to each point in set A, this may
work but the value of the objective function when the robot positions are exactly equal would not be zero, because we
would be summing some values twice or three times.

Hence, we can consider this algorithm:

1. Select the top N points from set B that are closest to any point in set A, creating set C
    - so what we do for this is, for each point in set B, loop through all points in set A to find the minimum
    distance to _any_ point in set A. then, we `qsort` these points and their minimum distances to find the top 64
    "best" points, or the ones with the smallest distance to any point in set A
    - perhaps we should instead be pessimistic and consider worst case?
2. For each point in set C, sum the euclidean distance between that point and the closest point in set A

## Notes
- this is very slow, but may still be feasible if Subplex doesn't do too many evaluations. the recursive square
subdivide algorithm I brainstormed is probably not feasible for this, too many evaluations.
- room for optimisation in the similarity calculation section
    - it seems we do the "similarity to point in list" calculation twice, perhaps we could cache this in a hashmap