# Omicontrol

This is the custom wireless debugging and management app for Team Omicron, to be deployed in the 2020 Internationals.

The app is written in Kotlin using JavaFX (via TornadoFX) for the UI. It enables developers to control the robot, view
debug information, view the camera output and edit camera calibration values.

## Features list
- Decode Omicam JPEG stream while overlaying threshold information
- Edit and upload camera thresholds on the fly
- Control the Jetson system (reboot, shutdown, halt, etc)
- Visualise both robots' localised position on the field
- Control the robots through commands like "reset to starting formation" and click to move to location
- Large buttons designed for tablet usage
- Dark theme!!!

## Building and running
Import the Gradle project into IntelliJ, and run Main.kt.

## License
Omicam is available under the license of the whole Team Omicron repo, see LICENSE.txt in the root directory. 

## Libraries and licenses
- [Protocol Buffers](https://github.com/protocolbuffers/protobuf): BSD 3-clause
- [TornadoFX](https://github.com/edvin/tornadofx): Apache 2