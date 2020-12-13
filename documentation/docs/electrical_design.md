# Electrical Design

_Author(s): Ethan Lo, mechanical & electrical design_

PCBs were used for our electronics to mitigate wiring, as well us enable us to use components that would otherwise be unusable. We use 4 custom-made PCBs, which we label as the “Main”, “Base”, “Kicker” and “Debug” boards.

**TODO: WE NEED IMAGES**

## Main Board

The main board is the central board of our electrical design. It contains the power supply, master microcontroller, test points and connections to all the other external electronics. Of these, the most noteworthy development is the use of test points. These are regions of exposed copper designed for convenient probing during the debug stage.

The Main Board contains:

* Power supply and safety circuitry
* Chip-on-Board ESP32 microcontroller
* Teensy 4.0 microcontroller
* Micro-USB port for uploading
* 2 position slide switching for choosing upload target
* BNO055 9-Axis IMU
* Laser Rangefinders
* Buzzers for audio debug
* LEDs for visual debug
* Various test points for easy probing

## Base Board

At the bottom of our robot is the base board. It holds our light sensors, light gate outputs, mouse sensor, and motor controllers. To drive these a slave microcontroller (namely the ATMEGA328p) was soldered directly onto the board. This is to save money and vertical clearance, and to decrease the chance of a short occurring due to the board’s proximity to the motors.

## Kicker Board

On the kicker board is the necessary electronics to drive two solenoids. This is not new to RoboCup, however we have made major modifications to the tried and tested design. Instead of directly driving the power MOSFET, we use an optocoupler to isolate the sensitive signal lines from the high voltages running through the circuit in an effort to protect the microcontroller in the event of a failure. Furthermore, we have moved away from relays and instead use MOSFETS to switch the load. We found that relays wore out after many frequent uses, and therefore we decided to use a non-mechanical solution.

## Debug Board

A problem we have previously faced is the lack of debug tools for our robot. Whilst debug messages are adequate for software design, when the robot is running it is often difficult to connect a computer to it. Hence, we developed a dedicated debug board that allows us to manually interface with our robot for debug purposes.
