# About us
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

**(TODO: will need to be fixed for stupid robocup rules to hide one contributor ugh)**

Our members are all very experienced, having competed in previous internationals competitions. Our team collectively
also has many victories and podium placements in Australian Nationals, States and Regionals competitions.

We have been preparing for this competition for about **(TODO: how many?)** months and we estimate to have a combined
**(TODO: put hours here)** hours developing and improving our current iteration of robots.

## Our current robots
This year, our team brings many new and exciting innovations to the table, as well as reliably building on and improving 
previously developed technologies.

Some new innovations we have developed this year include:

- **Omicam**: our custom vision pipeline developed to replace the OpenMV H7. Programmed in C, running on a Jetson Nano, this
device is capable of 720p@60fps **(TODO: please confirm)** field object tracking, and centimetre accurate field localisation
 using only camera data (no LRFs, etc) via a novel approach that uses non-linear optimisation algorithms.
- **Omicontrol**: our custom, wireless, all-in-one robot control software. Written in Kotlin, this program is used to
tune our cameras, move robots around and visualise sensor data.
- **Mouse sensor and velocity control**: our robots have fully working PWM-3360 mouse sensors, which we use to accurately
estimate and control our robot's velocity on the field, in addition to interpolating our localisation data.
- **Advanced strategies**: using the above projects, our movement software developers have combined highly accurate localisation
data with precise velocity control to enable the execution of advanced ball-manipulation strategies on the field.
- Wheels(?)
- Double dribbler and kicker(?)

Some technologies we build upon this year include:

- **FSM**: last year, we introduced the concept of a Hierarchical Finite State Machine (HFSM) as a novel method of organising
robot behaviour through a graph of inter-connected state machines, each which contains a series of states that can be
"switched" through. This year, we continue to build upon this technology, introducing more state machines and more states.
- PCBs (modular?)
- Wheels