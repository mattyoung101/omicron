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

Our members are experienced, having competed in 2-3 previous internationals competitions. Our team collectively
also has many victories and podium placements in Australian Nationals, States and Regionals competitions.

We have been preparing for this competition for about **(TODO: how many?)** months and we estimate to have a combined
**(TODO: put hours here)** hours developing and improving our current iteration of robots.

## COVID-19
**TODO cover COVID problems**

## Our current robots
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
