# Omicontrol

This is the custom wireless debugging and management app for Team Omicron, to be deployed in the 2020 Internationals.

The app is written in Kotlin using JavaFX (via TornadoFX) for the UI. It enables developers to control the robot, view
debug information, view the camera output and edit camera calibration values.

Omicontrol is built and maintained by Matt Young, so if you have any questions, please contact: 25070@bbc.qld.edu.au

**Note:** The code quality in this project is kind of bad, because I'm not experienced with UI development (so you
won't be seeing any proper design patterns like MVC). My apologies in advance!

## Features list
- Easy to use UI with built-in tutorials
- Decode Omicam JPEG/data stream while overlaying threshold information
- Edit and upload camera thresholds on the fly
- Control the Jetson system (reboot, shutdown, halt) and ESP32 (reboot)
- Visualise both robots' localised position on the field
- Control the robots through a multitude of useful commands including automatically resetting to starting positions
- Large buttons designed for tablet usage
- Dark theme!!!

## Building and running
Import the Gradle project into IntelliJ, and run Main.kt.

## License
Omicam is available under the license of the whole Team Omicron repo, see LICENSE.txt in the root directory. 

## Libraries and licenses
- [Protocol Buffers](https://github.com/protocolbuffers/protobuf): BSD 3-clause
- [TornadoFX](https://github.com/edvin/tornadofx): Apache 2