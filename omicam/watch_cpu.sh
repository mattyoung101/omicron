#!/usr/bin/sh
# Watches CPU temperature and performance at the same time
# Requires lm_sensors to be installed and setup correctly
# Invoke like this: watch -n 0.5 ./temp.sh
sensors && cat /proc/cpuinfo | grep MHz