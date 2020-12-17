# About us
## About Team Omicron
Team Omicron is a robotics team from Brisbane Boys' College, competing in RoboCup Jr Open Soccer. We are from Brisbane,
Queensland, Australia.

Team Omicron was formed in 2019 as a merger between two BBC teams, J-TEC (previously Team APEX) and Team Omicron (previously
Deus Vult). Our team members are:

| Name               | Primary responsibilities                                        | Original team | Contact |
|--------------------|-----------------------------------------------------------------|---------------|---------|
| Lachlan Ellis      | Strategy code                                                   | J-TEC         | TBA     |
| Tynan Jones        | Electrical design                                               | J-TEC         | TBA     | 
| Ethan Lo           | Mechanical & electrical deign, strategy code, docs              | Omicron       | ethanlo2010@gmail.com    |
| James Talkington   | Mechanical design                                               | J-TEC         | TBA     |
| Matt Young         | Vision systems developer, docs                                  | Omicron       | matt.young.1@outlook.com |

Our members are experienced, having competed in 2-3 previous internationals competitions. Our team collectively
also has many victories and podium placements in Australian Nationals, States and Regionals competitions.

## COVID-19
Our competition was scheduled for the 2020 RoboCup Internationals in Bordeaux, France. Unfortunately, due to the 
COVID-19 world pandemic, our competition has been cancelled. In addition, our current knowledge indicates that the 
2021 competition (the time that the 2020 one was postponed to) will either be cancelled, or we would not be able to
attend for other reasons.

Thus, we have decided to release absolutely everything we have produced: code, designs, PCBs, documentation, as open
source to give back to the community. See [this section](open_source_release.md) here for more information.

After we were made aware of COVID, and consequently the 2020 Internationals were "postponed", development on
the robot was significantly slowed down. This means that some announced features may not be fully complete
or bug-free. However, we believe our product is still of value to the larger RoboCup community.

## Robot components
This section contains the overview of the components on our robots.

Microcontrollers:
    - Main: ESP32 DEVKIT-C
    - Secondary: Teensy 4, ATMega328P

SBC:
    - LattePanda Delta 432

Camera module:
    - e-con Systems Hyperyon

Motors:
    - 4x Maxon DCX19
    

## New features & refinements
This year, our team brings many new and exciting innovations to the table, as well as building on reliable technologies
we have previously developed.

Some new innovations we have developed this year include:

- **Omicam**: Our advanced vision and localisation application, capable of 70 FPS ball, goal and line detection at 
1280x720 resolution. It's also capable of 1.5cm accurate localisation by using a using a novel hybrid sensor-fusion/non-linear optimisation algorithm.
- **Omicontrol**: Our custom, WiFi/Ethernet robot control software. Written in Kotlin, this program is used to
tune Omicam, move robots around and visualise sensor data.
- **Mouse sensor and velocity control**: Our robots use PixArt PWM3360 mouse sensor, which we use to accurately
estimate and control our robot's velocity on the field, in addition to interpolating our localisation data.
- **Advanced game strategies**: Using the above projects, our movement software developers have developed interesting
and complex ball-manipulation strategies that we use to a competitive advantage on the field such as flick-shots and
"line running".
- **Double dribbler and kicker:** (TODO hardware team write info)
- Wheels(?)

Some technologies we build upon this year include:

- **FSM**: In 2019, we introduced the concept of a Hierarchical Finite State Machine (HFSM) as a method of organising
robot behaviour through a graph of inter-connected state machines, each which contains a series of states that can be
"switched" through. This year, we continue to build upon this technology, introducing more states to our FSM.
- **Protocol Buffers:** In 2019, we unveiled Protocol Buffers as a an easy and intuitive way of passing complex data
between devices. This year, we continue to use the Google-developed technology, expanding upon its usage scope significantly.
- PCBs (modular?)
- Wheels