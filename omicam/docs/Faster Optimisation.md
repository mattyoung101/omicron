# Faster optimisation
_Author: Matt Young_

(Note: this ties in with our Hybrid localisation plan, see Hybrid Localisation Plan.md)

**Note: kept for historical reasons, pretty much all of the ideas in this document don't work**

## The problem
The main problem is that I think we're using an algorithm too complex for our problem. So, certainly the Subplex
optimiser does work, but it's designed for high dimensional non-linear problems, right, and our problem is two dimensional,
and technically discrete.. Hence, there's lots of room for improvement. Currently we get about 12 Hz, but I think we could
get probably closer to like 20-30 Hz if we improve how many evaluations of the objective function are required and also 
the complexity of the stepping process, which I assume Subplex is pretty complex for.

## The solution
### Hybrid localisation
With our hybrid localisation idea, the optimiser will pick an initial guess that can be refined. Because we seed
the localiser with the mouse sensor and greatly constrain the search space, it should already be much faster.

### Ray count
In the objective function, it's likely that we can reduce the ray count from 128 to like 64 or so. We should do error
tests to determine the minimum number of rays required in most cases for a good

### Better optimisation algorithms
#### Linear programming
The function isn't linear, so we can't use linear programming. Silly mistake I made before, but as it turns out
with multivariate functions it isn't quite as simple as it seems.

#### Simple heuristic algorithm(s)
Consider the hill climbing algorithm. I think this is a great target for our problem because it's simple to implement,
and it works by refining an initial guess rather than creating a new guess like how (I think) Subplex and NM-Simplex
work. At the end of the day we're trying to minimise two things, the number of objective function evals and also
the work required to generate a new eval location, so if hill climbing can do both of those things we're good.

Particle optimisation? May also be helpful to find the global minima, which isn't a requirement, but would be nice.

**Discredit these, both of them use more objective func evals than Subplex**

#### Try more algorithms from NLopt?
Some of the Powell algorithms like BOBYQA might be worth looking into.

#### Gradients
Could we find a gradient of our 3D function?