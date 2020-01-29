Team Omicron (2020)
====================

Welcome to the official repository for Team Omicron, a robotics team competing in RoboCup Jr Open Soccer from Brisbane
Boys' College in Brisbane, Queensland, Australia.

This repo contains all the hardware (including full PCBs and robot designs), software (including firmware for all
microcontrollers, our custom robot control software and vision pipeline) and associated documentation that we used to
compete in the 2020 Bordeaux Internationals in France.

This is an unprecedented amount of information all available under a relatively permissive license (see below for details). 
We hope this will make an excellent resource for advanced members of the league to study and learn from in the years to
come. So please, clone the repo, read the docs and get tinkering - and make sure you tell your friends!

If you have any specific questions, please check out the Team Omicron Team Description Paper, our website and feel
free to contact any team member directly (our info is below, or just use our GitHub profile). Thanks, and have fun!

**we should include how many lines of code we we wrote and perhaps some metric for designs as well!**

## About Team Omicron
Team Omicron was formed in 2019 as a merger between two BBC teams, J-TEC (previously Team APEX) and Team Omicron (previously
Deus Vult). Our team members are:

| Name               | Primary responsibilities                                        | Original team | Contact |
|--------------------|-----------------------------------------------------------------|---------------|---------|
| Lachlan Ellis      | Movement code                                                   | J-TEC         | TBA     |
| Tynan Jones        | Electrical design                                               | J-TEC         | TBA     | 
| Ethan Lo           | Mechanical & electrical deign, movement code, docs              | Omicron       | TBA     |
| James Talkington   | Mechanical design                                               | J-TEC         | TBA     |
| Matt Young         | Vision systems developer, docs                                  | Omicron       | TBA     |

**Important: Team roles are a lot more nuanced than written here.** Please see individual projects in the repo for
a more thorough description of the project itself, as well as who developed it and how to contact them. Thanks!

## List of projects
Our team members work on seven main projects:

| Name       | Path                | Description                                                                   |
|------------|---------------------|-------------------------------------------------------------------------------|
| Electrical | /designs/electrical | All the PCBs used in our robots.                                              |
| Mechanical | /designs/mechanical | All the CAD designs used to construct our robotss.                            |
| ESP32      | /esp32              | The ESP32 firmware that powers our movement and strategy code.                |
| ATMega     | /atmega             | The firmware that powers our ATMega328P motor and mouse sensor slave device.  |
| Teensy     | /teensy             | The firmware that powers our Teensy 4 light sensor and LRF slave device.      |
| Omicam     | /omicam             | Our advanced vision and localisation system running on a NVIDIA Jetson Nano.  |
| Omicontrol | /omicontrol         | Our custom wireless visualisation and debugging application.                  |

## License
This code is currently proprietary and confidential to Brisbane Boys' College and Team Omicron. No redistribution or use outside of our team is permitted. 

_Once our competition is done, this code will be released under the MPL 2.0._