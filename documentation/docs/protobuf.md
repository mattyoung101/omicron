# Protocol Buffers

_Page author: Matt Young, vision systems & low-level developer_

Team Omicron inherits our use of Protocol Buffers, a Google-developed technology, from our team last year (Team Deus
Vult, RoboCup Internationals 2019). However, we have greatly expanded on our usage of this technology in our project, as
it's now used in almost all communications we do everywhere, as well as non-communication files such as replays
and field files.

**Note:** Some of this documentation is borrowed from the RoboCup 2019 Sydney submission by our previous team (Deus Vult).

## Introduction
Low-level communication protocols such as UART and I2C operate on the byte level. This is a problem, because much of the
data we work with is larger than one byte (max range of 0-255). Most teams resolve this issue by inventing a custom
binary protocol, for example by turning a 16-bit integer into two 8-bit integers for transmisson which are then pieced
back together on the receiving end. However, this is a complicated, difficult to program approach which is difficult to
add new fields to in the future and only supports an extremely limited amount of data types that can be easily added by
the programmers (usually 16-bit ints, 8-bit ints and booleans). Without documentation about the custom protocol used,
future engineers on the project will struggle to identify what each byte in a byte stream means.

Protocol Buffers (Protobuf) is a Google developed technology that allows easy and fast encoding of data (known as
serialisation) to an arbitrary byte stream (in our case, a UART bus), and finally decoded (or deserialised) into the same
useable information as before.

As a cross-platform, cross-language serialisation framework, Protocol Buffers allows us to transmit many types of data
such as floating points, up to 64 bit integers, strings and more. This can all be done simply by specifying the packet
structure in a protocol definition file, importing the appropriate encoding/decoding libraries for the language in use
and generating some boilerplate code using the Protobuf compiler.

Thus, our intra-robot (i.e. between microcontroller) communications are reliable, efficient and easier to manage than
ever before.

## Implementation in Omicron
On the ESP32, LattePanda (via Omicam) and in the Teensy 3.5, we use Nanopb, which is an implementation of Protocol
Buffers v3 for C. We selected this library compared to the alternatives because it’s easy to use, space efficient and
fast (about 1ms to encode a message, and a little bit more to decode it).

This library, licensed under the Zlib license, is used to encode data to a byte stream (file, UART bus, etc) and decode
it a later date. Data is stored in memory using a standard C struct. This struct is then fed to the Nanopb encoder which
turns it into a byte stream. Usually, this byte stream is either written to disk (replays, field files) or transmitted
over UART. At a later date, and possibly on a different device, the decoder then reads these bytes in, and uses them to
reconstruct the same C struct that was sent.

Compared to last year, after ditching the OpenMV we added Protocol Buffer support to Omicam, which it uses both in
transmitting to the ESP32 over UART, and to Omicontrol over Wi-Fi. Likewise, Omicontrol uses Google's official Java
Protobuf library to talk back to Omicam. Despite being two different libraries, due to the Protobuf spec being
standardised, they can communicate successfully.

## Drawbacks
As the encoding/decoding processes are long and complex, our weaker ATMega328P slave device doesn't use Protcol Buffers
since it would be too slow and resource intensive.