# Components directory
This directory contains all the external libraries that we use. Miscellaneous/single file libraries are in the 
"libraries" directory, while bigger libraries get their own directory.

The IDF toolchain links each folder in with the main project as a "component", which is mostly just a wrapper
around some obnoxious CMake stuff. Read Epressif's docs for more info on this.
