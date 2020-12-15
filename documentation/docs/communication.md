# Communication
_Page author: Matt Young, vision systems & low-level developer_

Inter-robot communication is an extremely important aspect of RoboCup Jr. Using wireless protocols such as Zigbee or
Bluetooth allows robots to exchange game strategy information with each other, switch FSM states and much more. This year,
building on our codebase from last year, we have Bluetooth Classic communication between the two robots using the ESP32's
built in wireless module and it's implementation in the IDF API.

Likewise, intra-robot communication is an essential aspect of a multi-micro-controller setup like the one we have.
This year, we extend our previous Protocol Buffer, UART-backed communication protocol to include a CRC8 checksum and better
error handling to improve reliability.

**Note:** Some of this documentation is borrowed from the RoboCup 2019 Sydney submission by our previous team (Deus Vult).

## Bluetooth communication
Our Bluetooth inter-robot comms remain largely unchanged from last year, minus a few bug fixes and more testing.

### Background
Bluetooth is a low-power wireless communications protocol used in many devices. Our ESP32s support both Bluetooth
Classic v4 Enhanced Data Rate (EDR) and Bluetooth Low Energy (LE). We opted to use the BT Classic as its simpler and has
faster speeds. On the ESP32, Bluetooth, like most other things, is highly parallel and runs in separate threads on its
own core (core 0). This means that our code, which runs on core 1, can run totally asynchronously to Bluetooth.

### Implementation in Omicron
Bluetooth is an extremely complicated protocol with many layers and quirks, but in essence, we use a Simple Port Profile
(SPP) to connect to the other robot using Simple Secure Pairing (SSP).

We have two robots, Robot 0 and Robot 1. Robot 0 is the “host”, who establishes an SPP server. Robot 1 is the “client”,
who discovers Robot 0 and connects to it. 

Our Bluetooth aims to be very reliable, and on the event of a disconnect, the client will continuously attempt to
reconnect to the host. Similarly, the host will always be listening for a new client on disconnect. If too many Protobuf
decode errors occur, the server will disconnect the client. We also use whitelisting to make sure no foreign devices can
interrupt our Bluetooth connectivity by connecting during a game.

Our Bluetooth communication uses Protocol Buffers. Each robot runs a receive and send task. The receive
tasks reads Protobuf byte streams and decodes them, while send task encodes Protobuf byte streams and sends them.

The default Bluetooth timeout is about 3 seconds, which is too slow for our purposes. Instead, we programmed a custom
timer for 800ms where if a packet is not received within this time, the other robot is considered off for damage. When
this is detected, the other robot destroys the Bluetooth connection and enters into defence.

We also have some complex conflict resolution code. Both robots send each other’s FSM states, and if both are in attack
or both are in defence, an algorithm is run to resolve the conflict, which works by having the robot closest to the goal
become the defender, as well as some code to deal with special circumstances such as if a robot can’t see the goal.

One of the most important parts of Bluetooth is switching. Inside the FSM, each robot will set a boolean true/false if
they themselves are willing to switch. The attacker is willing to switch almost all of the time, except when its doing
something important like shooting or dribbling to goal. The defender will only switch in much more specific
circumstances like if it’s surging (the ball is in front of it and its close to the goal). One robot, robot 0, is the
designated “switch observer”. It will observe if it’s willing to switch and the other robot is willing to switch, and if
so, broadcast a special message over Bluetooth that causes each robot to invert its state. To prevent fast-paced
switching that’s detrimental to gameplay, a switch timer is also used which means the robot is only permitted to switch
every 1.5 seconds.

## UART communication
Last year, we improved our communication infrastructure significantly by switching from I2C to UART and using Protocol
Buffers instead of an arbitrary, bit-shifted binary format. For our purposes, we found I2C to be too error-prone and 
complex, with small payload sizes. Instead, through testing we observed that UART on 115200 baud rate was simpler
to setup and use, more reliable and fast enough for our situation.

This year, we continue our tradition of using Protocol Buffers for everything imaginable in our comms protocol, but also
extend our communication protocol (nicknamed _JimBus_ after a member on our team) to improve reliability.

A standard JimBus message sent over UART has the following structure:

- Byte `0x0B` to indicate message start
- Message ID
- Message length
- Protobuf data (message contents)
- CRC8 checksum of protobuf data

Essentially, our messages have a 3 byte header and a 1 byte checksum. JimBus, our custom UART comms protocol, runs on
the ESP32, the Teensy 3.5 and even the LattePanda using POSIX termios, so it's highly cross-platform and easy to decode.
We introduced a CRC8 checksum this year to ensure that, when a message is corrupted due to electrical noise, the
Protobuf decoding doesn't spectacuarly fail. Instead, if the received CRC8 checksum doesn't match the received checksum
(calculated before sending by the other device), we reject the message, log an error and attempt to resync the UART
stream.

In future, we would like to add an extra byte to the message size field so we can transmit messages longer than 255
bytes, and also use a CRC32 checksum instead of CRC8 for better accuracy.