# Hybrid localisation plan
_Authors: Matt Young, Ethan Lo_

## Background and justification
Currently, our localisation solution using just the lines works well when tested with Fusion renders, but struggles
in real life due to noise and difficulty thresholding lines. The problem is that the perspective of the camera and
the size of the tiny ints field lines doesn't allow us to see four lines on the field to make the localiser work.
We can't see the line on the opposite side of the field, so therefore the localiser really struggles to find a solution.

We're going to fix this by developing a hybrid localisation algorithm that does sensor fusion between the low-accuracy
goal localisation, the medium accuracy mouse sensor (integrated displacement values), and finally the high accuracy
but error prone optimisation solution with field lines.

By doing this, we essentially get increased accuracy by combining the strengths of all three sensors. It mainly acts
so that the localiser has less area to search which makes it faster and more stable.

## Sensors
### Goals
Basically we determine a localisation (for each goal?) using trigonometry, just like many other teams do. Except,
by building a model beforehand, we calculate the uncertainty vs distance for a goal and use that to create an 
uncertainty for each localisation.

### Mouse sensor
Sum mouse sensor relative displacements to get absolute field position, basically. We feed this data back into the camera
by sending it from the ATMega to the ESP32 first, then to Omicam via a Protobuf message over UART.

### Lines
Same approach as before, nothing different. Subplex optimiser and raycasting.

## Algorithm
1. Determine an initial approximation based on goals. The uncertainty of this initial approximation is our search radius 
   or domain to run the localiser in.
2. Seed the initial position of the localiser using the mouse sensor data, fed back in from the ATMega via the ESP32. 
   On the ATMega, we'll keep integrating the mouse sensor data over time with respect to our last localised position (or
   a rough origin point if we have no position yet) to determine our absolute position in field coords.
3. Run the Subplex optimiser _only in this domain_ and from the mouse sensor determined last position, converge on
   a final position.

## Notes for implementation
- Clamping Subplex optimiser domain to circle: we can just check if it's outside and return LOCALISER_LARGE_ERROR (8900)
     - Alternatively use a box and do it the way you're supposed to with `nlopt_set_lower_bounds`, `nlopt_set_upper_bounds`
- Should probably merge goal components to reduce jitter, and perhaps even smooth them (lerp)?
     - algorithm for this is, for the goal image, if two components distances are <= THRESH and their size is >= THRESH,
     then merge the rectangles
- If goals are not visible, default to mouse sensor and add a static radius around it. We don't use mouse sensor constantly
  due to compounding error from integration (look into this though).
- Always be conservative in estimates of uncertainty, we never want to under-estimate it
- If the goal uncertainty is good enough, sometimes we could just not bother with the Subplex optimiser (will this ever happen?)