# Low-level code

_Page author: Matt Young, vision systems & low-level developer_

Team Omicron's code is written at a much lower level (we believe) than many other teams in the RoboCup Jr competition.
For example, most of our gameplay code runs on the ESP32 and is written in plain C11 using Espressif's IoT Development
Framework (as opposed to Arduino). At last year's competition, only one other team was using the ESP32 and they were
using the Arduino runtime instead of the IDF. We believe that working at a lower level allows more flexibility,
performance and knowledge to be gained.

**Note:** Due to COVID and development delays, a lot of our low-level code is the same as last year (with
a few minor bug fixes). Hence, a lot of this documentation is borrowed from the RoboCup 2019 Sydney submission
by our previous team (Deus Vult).

he Teensy 3.5 microcontroller. This year we have decided to use the ESP32-WROOM module from Espressif Systems. 

## ESP32-IDF
Last year, under our previous team Deus Vult, we migrated from using a Teensy 3.5 as the main device to using an ESP32.
This year we continue to use the ESP32 as our main device, working with and improving on last year's codebase. 

Compared to the Teensy 3.5, the ESP32-WROOM-32 has twice the clock rate (at 240 MHz), 700% more flash storage (4MB) and
50% more RAM (384 KB). In addition, it has many more advanced features we utilise, including two physical CPU cores (for
multi-tasking), built-in Wi-Fi and Bluetooth among other features. For all these extra benefits, the ESP32 is 54%
cheaper than the Teensy, making it a clear winner.

Espressif Systems (ESP32 manufacturers) provides two options for software development: the Arduino core, which provides
a familiar C++ environment with ported Arduino functions, and secondly the IoT Development Framework (IDF), which is a
completely ESP32 specific platform created by Espressif. The IDF is the recommended option to program advanced devices
in, and we consider our robot to be an advanced device. Espressif recommends programming projects in the IDF using C
(rather than C++). Ethan and I (Matt) already knew C from last year's competition, and all of the ESP32 code was already
mostly written after being ported from Arduino. That only left Lachlan to learn a little bit of C to write his orbit
strategy. Like last year, we selected the IDF over the Arduino core due to its higher quality codebase, greater
performance, the fact that Espressif provides more official support to the IDF, dissatisfaction with Arduino functions
and most importantly: access to advanced features.

We document some important ESP32-IDF features below.

### Core dumps
When an ESP32 encounters an irrecoverable error (a crash), it can be configured to produce what’s known as a core dump,
and save it to flash memory. This core dump contains highly technical information that our software engineers can use to
debug the root cause of the problem. Such information includes the name of the error, the memory locations where it
happened, a register dump, as well as a backtrace containing all function calls to the problematic code.

Due to the fact that this code is saved to flash memory, if the robot crashes on the field, we can perform a virtual
autopsy of the ESP32, by retrieving and analysing its core dump. This has been invaluable in debugging many errors the
robot has encountered.

When an error happens on the robot, unlike most other microcontrollers, the ESP32 is actually capable of rebooting
itself. This way, when our robot crashes, it can safely reboot and continue on with normal play, without us having to
take it off for damage.

### Task watchdog timer
A frequent problem many teams encounter is the robot stopping responding, also known as hanging. This can be caused by a
multitude of reasons, but is often very challenging to debug effectively. As is covered more in the FreeRTOS section,
our robot’s code runs in parallel tasks, meaning hanging is less of an issue: however, these tasks themselves can still
stop responding without bringing down the whole robot, which is still an issue! Using the IDF, we can make these issues
much easier to resolve using a feature known as the Task Watchdog Timer (TWDT). When a task stops responding for about 5
seconds, the TWDT will print error messages containing the name of the task which crashed and can optionally reboot the
robot.

### Logging library
One of the most critical part of debugging is understanding what your robot is doing, and why it’s doing it.
Traditionally, this has been a difficult challenge due the lack of an official logging library for Teensies (and
Arduino). However, the IDF includes a built-in high performance logging library with support for coloured output and
tags. This makes determining what the robot was doing (in our case, what state it’s in and why it’s switching states) a
trivial task. The ESP’s logging library has solved countless bugs in our robot and we’re thankful that Espressif created
it!

An issue sometimes happens on Windows based computers, where the computer can’t handle continuous outputs of logging,
causing us to end in a back-buffer of old data. To work around this, one of our programmers created a log once
function, which calculates the unique Jenkins hash of a string and stores it into a database, so that the same message
won’t be logged twice, leading to cleaner outputs.

## FreeRTOS
Building on last year's work, we once again use FreeRTOS as the real-time operating system of the ESP32.

FreeRTOS is an industry standard real time operating system kernel (RTOS). It’s open source, released by the developers
under the permissive MIT license. It has been created over a 15 year period and is widely renowned by millions of users
as being stable, safe and secure. Whilst teams using an Arduino or a Teensy will be stuck with a cyclic-executive
runtime in which only one action can be run at once, we can run multiple tasks at the same time, giving us performance
and design benefits.

An RTOS is an operating system that is optimised for use in embedded/real time applications, such as our robot. The main
component of FreeRTOS is the task scheduler, which has the responsibility of deciding which task at any given point in
time should run on the CPU. It decides this by choosing the task with the highest priority, and when two tasks have the
same priority, it uses an algorithm known as round-robin scheduling where each task is given equal CPU time. 

Changing which task runs on the CPU happens through a process known as context switching. Essentially, it involves
saving and later restoring the entire execution state of the microcontroller, such as the location of the instruction
pointer and values of all registers. FreeRTOS’s scheduler is preemptive, meaning that tasks are not told when a switch
will occur and the kernel is entitled to context switch at any state of execution.

The ESP32-IDF heavily revolves around FreeRTOS, and most drivers are written using highly efficient, parallel
thread-aware code. This is further aided by the fact that the ESP32 is dual core. As an example of this, we run our
Bluetooth stack on core 0, which can be running at the same time as core 1 processes Protocol Buffer data and moves the
motors.

FreeRTOS has a tick rate, meaning not how fast the processor itself runs, but instead how often the RTOS updates or
ticks. We have FreeRTOS running at 128 Hz (overclocked from the default 100 Hz). Our ESP32 itself runs at 240 MHz, the
fastest speed possible. This is an ideal clock rate for FreeRTOS as clocking it too high can actually lead to
performance degradation due to most CPU time being spent in the RTOS rather than user code.

### How it's used
As mentioned previously, FreeRTOS works by running multiple tasks in parallel on both the ESP32’s cores. Most of the
IDF’s drivers, such as the entire Bluetooth stack, run on core 0 (the “protocol core”). All of our tasks, by default,
run on core 1 (the “application core”), meaning Bluetooth can run with absolutely zero overhead to the rest of our
robot. In ESP32 (technically the Xtensa) port of FreeRTOS, a task can float between cores, meaning it will run on
whichever core is least busy, or it can be pinned to core meaning it is locked down to that one. However, upon accessing
the floating point unit (FPU) embedded into the ESP32 to perform fast decimal calculations, due to some complicated
hardware specifics that are out of the scope of our documentation, a task must be pinned to the core it started on. This
means in practice most of our tasks run on core 1, and most of the IDF’s tasks run on core 0.

**List of RTOS tasks:**

- MasterTask: main task which runs when the ESP32 is in master mode. Initialises and receives data from other tasks, updates FSM and motors.
- CameraReceiveTask: receives data and calculates goal distance and angles from the camera over UART.
- I2CReceiveTask: receives data from the slave over I2C and decodes Protocol Buffers
- BTLogicTask: Bluetooth logic task, created when a Bluetooth connection is established. Manages sending and receiving Bluetooth data, switching and other logic associated with that ar

### Benefits
- Increased performance and efficiency due to asynchronous tasks and utilising dual-core architecture
- Increased debugging resources as FreeRTOS is widely used in the real world
- Tighter integration with ESP32-IDF as it also uses FreeRTOS multi-tasking

### Drawbacks
- Multi-tasking brings with it lots of complexities including notoriously hard to debug race conditions, deadlocks, etc.
- Less lightweight than cyclic-executive runtime, meaning some elements (e.g. IMU reading) may be slower leading to IMU drift
