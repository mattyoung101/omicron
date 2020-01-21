# Hardware Design

This year we focused on designing a simple and robust robot which was quick to assemble and disassemble when needed. 
We also aimed on improving hardware from previous years including the custom omni-directional wheels introduced last 
year.

The following are the major hardware improvements that have occurred this year:
* Custom omni-directional wheels
* Custom wound solenoid and kicking circuit
* Custom microcontroller circuits
* Custom camera hardware
* Mouse sensor implemenation
* Various debugging options for software to use

## Omni-directional Wheels

Omni-directional wheels (also referred to as *omniwheels*) are specially designed to give robots full freedom of 
motion, enabling them to travel in any direction without turning. The downside to these wheels is the relative lack 
of tracktion and robustness when compared to conventional tires due to the individual rollers. As such, the primary 
goal of new iterations is to increase the grip and strength of the wheels.

The following image shows the development of the omniwheel used throughout the years of competition with the rightmost 
being the latest design:

**TODO: PUT PICTURE HERE**

The two leftmost designs, the *Kornylak* and *GTF* omniwheel are stock designs bought pre-made from the manufacturer. 
Through using both these wheels for a year, we discovered that they were lacking in grip and robustness respectively, 
leading us to develop our own designs.

The third design was our first attempt at a custom omniwheel, featuring double layered rollers and individual roller 
axles for faster repair. Though it allowed for much smoother movement and better traction, the wheels were large and 
unwieldly and the rollers still broke due to the use of softer silicon rollers. Furthermore, the use of 3D printed 
components caused rollers to fall out of the wheel during games.

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