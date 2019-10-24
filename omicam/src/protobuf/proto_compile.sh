#!/usr/bin/fish
# if you don't have fish just make this /usr/bin/sh or something
protoc --nanopb_out=. RemoteDebug.proto
