# Hardware Design

This year we focused on designing a simple and robust robot which was quick to assemble and disassemble when needed. 
We also aimed on improving hardware from previous years including the custom omni-directional wheels introduced last 
year.

The following are the major hardware improvements that have occurred this year:

* Custom omni-directional wheels
* Custom kicker unit
* Custom microcontroller circuits
* Custom camera hardware
* Mouse sensor implemenation
* Various debugging options for software to use

## Design Overview

**TODO: PUT PICTURE OF DESIGN HERE, POSSIBLE EXPLODED VIEW OR A360 LINK**

## Omni-directional Wheels

Omni-directional wheels (also referred to as *omniwheels*) are specially designed to give robots full freedom of 
motion, enabling them to travel in any direction without turning. The downside to these wheels is the relative lack 
of tracktion and robustness when compared to conventional tires due to the individual rollers. As such, the primary 
goal of new iterations is to increase the grip and distance of the wheels.

The following image shows the development of the omniwheel used throughout the years of competition with the rightmost 
being the latest design:

**TODO: PUT PICTURE HERE**

### Past Iterations

The two leftmost designs, the *Kornylak* and *GTF* omniwheel are stock designs bought pre-made from the manufacturer. 
Through using both these wheels for a year, we discovered that they were lacking in grip and robustness respectively, 
leading us to develop our own designs.

The third design was our first attempt at a custom omniwheel, featuring double layered rollers and individual roller 
axles for faster repair. Though it allowed for much smoother movement and better traction, the wheels were large and 
unwieldly and the rollers still broke due to the use of softer silicon rollers. Furthermore, the use of 3D printed 
components caused rollers to fall out of the wheel during games.

**TODO: PUT PICTURES OF NATOINALS WHEEL, DOUBLE WHEEL WITH SMALL ROLLERS AND BIG WHEEL WITH SMALL ROLLERS**


### Development of current wheel

The leftmost design was a revised version of your first custom omniwheel. Various different materials were lasercut in
order to replace the 3D printed components. After extensive testing, it was decided that PVC was to be used as it was 
durable, cheap and easy to laser cut. Alternatives materials included acrylic, which was not strong enough, and 
aluminium, which was too expensive to manufacture. 

The middle design is our second attempt at a custom omniwheel, once again featuring double layered rollers. However, 
this time smaller diameter rollers were used to allow the wheel to be much more compact. Consequently, this permitted 
the use of wheel protection guards, as seen in the image below. This prevented the wheels from getting damaged, often a 
result of the wheels grinding up against other robots or objects. In addition, the individual roller axels were replaced
with metal wire to prevent the rollers from falling out.

The rightmost design is a modified version of the middle design. Instead of having lots of smaller rollers, fewer longer 
rollers are used instead. This makes the wheel much easier to take apart and the rollers less likely to break.

**TODO: modify paragraph above once final design has been chosen to demonstrate the "development flow"**

**TODO: Explain new design when it decides to exist**

## Kicker Unit

**TODO: Idek if this works**

## Chip-on-Board Microcontrollers

## Custom Vision System

The primary goal for this year was to improve the vision system of the robot by developing a custom vision pipeline.
This was achieved with the 

## Debugging

Further improvements upon the debugging options were made this year including:
* 4 piezzoelectric buzzers for audio feedback
* 8 LEDs and a text LCD screen for visual feedback
* Wireless degbugging UI for easy visualisation