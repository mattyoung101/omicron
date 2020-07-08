#!/usr/bin/fish
# if you don't have fish just make the above /usr/bin/sh or something
protoc --nanopb_out=. --java_out=../../../omicontrol/src/main/java RemoteDebug.proto
protoc --nanopb_out=. UART.proto
# recompile UART comms again and copy it into the esp32 folder - TODO why don't we just literally use "cp" instead of being dumb?
protoc --nanopb_out=../../../esp32/components/nanopb UART.proto
protoc --nanopb_out=. --python_out=../../scripts/FieldFileGenerator FieldFile.proto
protoc --nanopb_out=. --java_out=../../../omicontrol/src/main/java Replay.proto