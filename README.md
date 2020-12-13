Team Omicron (2020)
====================

Welcome to the official repository for Team Omicron, a robotics team competing in RoboCup Jr Open Soccer from Brisbane
Boys' College in Brisbane, Queensland, Australia.

This repo contains all the hardware (including full PCBs and robot designs), software (including firmware for all
microcontrollers, our custom robot control software and vision pipeline) and associated documentation of our processes.

We've very excited to release this, because we believe it's one of the most complete releases of a RoboCup Jr team
to date. We hope that this release will benefit RoboCup teams for many years to come.

If you have any specific questions, please check out the Team Omicron website and feel free to contact any team member 
directly (our info is below, or just use our GitHub profile). Thanks, and have fun.

Website: [teamomicron.github.io/](https://teamomicron.github.io/)

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


## List of projects
Our team members work on seven main projects:

| Name       | Path                | Description                                                                   |
|------------|---------------------|-------------------------------------------------------------------------------|
| Electrical | /designs/electrical | All the PCBs used in our robots.                                              |
| Mechanical | /designs/mechanical | All the CAD designs for the structure of the robot.                           |
| ESP32      | /esp32              | The ESP32 firmware that powers our movement and strategy code.                |
| ATMega     | /atmega             | The firmware that powers our ATmega328P motor and mouse sensor slave device.  |
| Teensy     | /teensy             | The firmware that powers our Teensy4.0 light sensor and LRF slave device.     |
| Omicam     | /omicam             | Our advanced vision and localisation system running on a LattePanda Delta.    |
| Omicontrol | /omicontrol         | Our custom wireless visualisation and debugging application.                  |

## License
### Code
All code written by Team Omicron is released under the **Mozilla Public License 2.0** (see LICENSE.txt in each directory).
You will be able to tell which code is ours due to the presence of Omicron copyright notices at the top of the file.

For information on this licence, please [read this FAQ](https://www.mozilla.org/en-US/MPL/2.0/FAQ/), and [this
question](https://opensource.stackexchange.com/a/8832). Simply put, if you are building a robot based on Team Omicron's
code, the MPL requires that you disclose that your robot uses open-source software from Team Omicron, and where anyone
can obtain it (this repo). _A great place to do so would be in your poster and presentation._ If you modify any files with
the MPL licence header, the licence requires that you release your improvements under the MPL 2.0 as well. New files
that you create can be under any licence.

We have decided to use the MPL because we believe it balances freedom of usage, while making sure that any improvements are
released back to the RoboCup community for future teams to benefit from.

### Designs
TODO figure out designs licence. CC-BY-SA most likely.

## Other links
For more information on our robot, please checkout our [website](https://teamomicron.github.io/). Game footage, testing 
videos and other miscellaneous things will be posted on [Youtube](https://www.youtube.com/channel/UCAu5r4Zpu0z3kGZSfxlnySw?) 
and probably Twitter (no link yet). 
