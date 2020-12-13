# Protocol Buffers

_Author(s): Matt Young, vision systems & low-level developer_

Team Omicron inherets our use of Protocol Buffers, a Google-developed technology, from our team last year (Team Deus Vult, RoboCup
Internationals 2019). However, we have greatly expanded on our usage of this technology in our project, as it's now used in almost
all communications we do everywhere.

**Note:** some of this documentation is borrowed from the RoboCup 2019 Sydney submission by our previous team (Deus Vult).

## Introduction to Protobuf
Protocol Buffers (Protobuf) is a Google developed technology that allows easy and fast encoding of data (known as serialisation) 
which can then be transmitted to another device, over wires using a protocol such as Inter-Integrated Circuit (I2C), and finally 
decoded (or deserialised) into usable information.

## Usage in Omicron
