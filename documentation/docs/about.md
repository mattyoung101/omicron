# About us
## About Team Omicron
Team Omicron was formed in 2019 as a merger between two BBC teams, J-TEC (previously Team APEX) and Team Omicron (previously
Deus Vult). Our team members are:

| Name               | Primary responsibilities                                        | Original team | Contact |
|--------------------|-----------------------------------------------------------------|---------------|---------|
| Lachlan Ellis      | Strategy code                                                   | J-TEC         | TBA     |
| Tynan Jones        | Electrical design                                               | J-TEC         | TBA     | 
| Ethan Lo           | Mechanical & electrical deign, strategy code, docs              | Omicron       | TBA     |
| James Talkington   | Mechanical design                                               | J-TEC         | TBA     |
| Matt Young         | Vision systems developer, docs                                  | Omicron       | TBA     |

**(TODO: will need to be fixed for stupid robocup rules)**

Our members are all very experienced, having competed in 2-3 previous internationals competitions. Our team collectively
also has many victories and podium placements in Australian Nationals, States and Regionals competitions.

We have been preparing for this competition for about **(TODO: how many?)** months and we estimate to have a combined
**(TODO: put hours here)** hours developing and improving our current iteration of robots.

## Our current robots
This year, our team brings many new and exciting innovations to the table, as well as building on reliable technologies
we have previously developed.

Some new innovations we have developed this year include:

- **Omicam**: our advanced, custom-developed vision application designed to replace the OpenMV H7. Written mainly in C, Omicam 
is capable of 70 FPS field object tracking at 720p (1280x720 pixels), and 1cm accurate robot position estimation 
(localisation) by solving a multi-variate optimisation problem in real time.
- **Omicontrol**: our custom, wireless/wired, all-in-one robot control software. Written in Kotlin, this program is used to
tune our cameras, move robots around and visualise sensor data.
- **Mouse sensor and velocity control**: our robots have fully working PWM-3360 mouse sensors, which we use to accurately
estimate and control our robot's velocity on the field, in addition to interpolating our localisation data.
- **Advanced game strategies**: using the above projects, our movement software developers have combined highly accurate localisation
data with precise velocity control to enable the execution of advanced ball-manipulation strategies on the field, designed to
get the upper edge on our opponents.
- Wheels(?)
- Double dribbler and kicker(?)

Some technologies we build upon this year include:

- **FSM**: last year, we introduced the concept of a Hierarchical Finite State Machine (HFSM) as a novel method of organising
robot behaviour through a graph of inter-connected state machines, each which contains a series of states that can be
"switched" through. This year, we continue to build upon this technology, introducing more states to our FSM.
- **Protocol Buffers:** last year, we unveiled Protocol Buffers as a an easy and intuitive way of passing complex data
between devices. This year, we continue to use the Google-developed technology, expanding upon its usage scope significantly.
- PCBs (modular?)
- Wheels