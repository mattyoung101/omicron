#!/usr/bin/fish
# FIXME: move this script to the scripts folder (will need to update paths)
# if you don't have fish just make this /usr/bin/sh or something
protoc --nanopb_out=. --java_out=../../../omicontrol/src/main/java RemoteDebug.proto
protoc --nanopb_out=. UART.proto
protoc --nanopb_out=. --python_out=../../scripts/FieldFileGenerator FieldFile.proto