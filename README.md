Team Omicron (2020)
====================

Welcome to the official repository for Team Omicron, a robotics team competing in RoboCup Jr Open Soccer from Brisbane
Boys' College in Brisbane, Queensland, Australia.

This repo contains all the hardware (including full PCBs and robot designs), software (including firmware for all
microcontrollers, our custom robot control software and vision pipeline) and associated documentation of our process.

If you have any specific questions, please check out the Team Omicron website and feel free to contact any team member 
directly (our info is below, or just use our GitHub profile). Thanks, and have fun.

## About Team Omicron
Team Omicron was formed at the end of 2019 as a merger between two BBC teams, J-TEC (previously Team APEX) and Team 
Omicron (previously Deus Vult). Our team members are:

| Name               | Primary responsibilities                                        | Original team | Contact |
|--------------------|-----------------------------------------------------------------|---------------|---------|
| Lachlan Ellis      | Movement code                                                   | J-TEC         | TBA     |
| Tynan Jones        | Electrical design                                               | J-TEC         | TBA     | 
| Ethan Lo           | Mechanical & electrical design, movement code, docs             | Omicron       | ethanlo2010@gmail.com |
| James Talkington   | Mechanical design                                               | J-TEC         | TBA     |
| Matt Young         | Vision systems developer, docs                                  | Omicron       | matt.young.1@outlook.com |

Important: Team roles are a lot more nuanced than written here. Please see individual projects in the repo for
a more thorough description of the project itself, as well as who developed it and how to contact them. Thanks!

## List of projects
Our team members work on seven main projects:

| Name       | Path                | Description                                                                   |
|------------|---------------------|-------------------------------------------------------------------------------|
| Electrical | /designs/electrical | All the PCBs used in our robots.                                              |
| Mechanical | /designs/mechanical | All the CAD designs for the structure of the robot.                           |
| ESP32      | /esp32              | The ESP32 firmware that powers our movement and strategy code.                |
| ATMega     | /atmega             | The firmware that powers our ATmega328P motor and mouse sensor slave device.  |
| Teensy     | /teensy             | The firmware that powers our Teensy4.0 light sensor and LRF slave device.     |
| Omicam     | /omicam             | Our advanced vision and localisation system running on a Lattepanda Delta.    |
| Omicontrol | /omicontrol         | Our custom wireless visualisation and debugging application.                  |

## License
These code and design files are currently property of Team Omicron.
No redistribution or use outside of our team is permitted. 

**TODO update to MPL 2.0**

## Other links
For more information on our robot, please checkout our [website](https://teamomicron.github.io/). Game footage, testing 
videos and other miscellaneous things will be posted on [Youtube](https://www.youtube.com/channel/UCAu5r4Zpu0z3kGZSfxlnySw?) 
and probably Twitter (no link yet). 
