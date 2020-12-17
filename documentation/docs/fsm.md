# Finite State Machine

We inherit our Hierarchical Finite State Machine (HFSM), implemented by us in C on the ESP32, from last year.

**Note:** Due to COVID and development delays, our FSM implementation remains unchanged from last year (with
a few minor bug fixes). Hence, most of this documentation is borrowed from the RoboCup 2019 Sydney submission
by our previous team (Deus Vult).

## Introduction
In the simplest possible terms, a Finite State Machine is a model of computation whereby a system (in this case, the 
robot's software), can be in exactly one of finite states at any given time. FSMs are commonly used to manage AI in 
video games and to manage complex system software (for example, our ESP32 uses an FSM internally in its I2C stack). 

Each state is designed to handle a very specific set of criteria. Should a state not meet its very specific criteria, it 
will revert to a previous state or change states to a new state. In this way, each state function has only the minimal 
amount of code required to perform one specific action, leading to an efficient and clean codebase.

## Implementation
Our FSM implementation is entirely our own custom implementation written in pure C11 on the ESP32. It stores its state 
history to a stack data structure, meaning that it’s possible to revert back through states, just like you can undo and 
redo in certain programs.

Each state is represented as a set of functions: state_enter, state_update and state_exit. Enter and exit are called
respectively when the given state is entered into or exited from. Update is called each time the main task’s loop is
executed for the state the robot is in.

Our FSM implementation can be considered to be “hierarchical”, meaning that we have nested state machines. Our global 
state machine has three states: attack, defence and general. Each sub-state-machine has another series of states, listed below.

## Benefits
The main benefit of the FSM is the improved debugging as a result of the various states. Since the robot must be in one
state at all times, issues can be rapidly identified and pinpointed from the terminal output. This has greatly improved
our efficiency with regards to solving problems and debugging.

It also allows for greater flexibility when programming the robot. New actions can simply be added by adding a new state
to the finite state machine. Furthermore, various additions such as timers can be added quickly and efficiently.

Another benefit is the code cleanliness. Using the FSM we have found, avoids much of the dreaded "spaghetti code" a lot
of teams have.

## Drawbacks
The main drawback of an FSM is the possibility of rapid alternating between two states caused by logic errors or noise.
At its worse, this can cause the robot get stuck swapping between two states and not make any progress. However, these
instances are not only rare, but solvable, with tuning. Furthermore, the FSM itself makes catching and rectifying these
errors easier.

## States
### Attacker states
- Idle: entered into when the robot cannot see the ball for 5 seconds. Manoeuvres into the centre of the field.
- Pursue: entered into when the robot needs to move quickly towards the ball when it is far away.
- Orbit: our exponential orbit, entered into when the robot is behind the ball and needs to be in front of it.
- Dribble: once the robot has the ball in its capture zone, this state is entered into. Accelerates the robot linearly towards the goal.

### Defender states
- Reverse: entered into when the goal is not visible. Reverses and aligns itself to the back wall with the LRF
- Idle: entered into when the ball is not visible. Manoeuvres in front of the goal.
- Defend: entered into when the robot is visible. Positions itself between the ball and the goal.
- Surge: entered into when the robot is in possession of the ball. Rushes forward to put distance between the ball and the goal. If the other robot is active, it will switch to attacker state.

### General states (shared)
Shoot: entered into when the robot is has a clear shot of the goal. Activates solenoid kicker.

## Timers
Timers are an essential part of our robots behaviour, and are particularly relevant for our finite state machine. We use FreeRTOS’ highly efficient software timers, which use zero CPU time and add zero overhead to the tick unless they’re activated. We extended FreeRTOS’ timer system using a custom add-on layer that we developed, to assist in creating timers, stopping them and starting them if they’re not already running.

### List of timers
- Bluetooth packet timer: if a BT packet is not received in this time, the other robot must be off for damage
- Cooldown timer: after a switch, this timer is started, and another switch is not permitted until it expires
- Idle timer: if the ball is not visible for this time, switch into idle state
- Dribble timer: if the ball is in the capture zone and close enough for this time, switch into dribble state
- Surge timer: same as the above for defender, but switch into surge state
- Shoot timer: if the ball is in the capture zone for long enough, kick