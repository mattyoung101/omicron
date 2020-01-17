# Omicontrol
Omicontrol is Team Omicron's custom wireless debugging and management app that we use to communicate with Omicam running
on our single board computer. It enables team members to control the robot's movement, view helpful debug information as well
as tune and view camera output.

The app is written in Kotlin using JavaFX (via TornadoFX) for the UI. It receives data from Omicam in the format of
Protocol Buffers over a TCP socket.

Omicontrol is built and maintained by Matt Young.

**Note:** The code quality in this project is kind of bad, because I'm not experienced with UI development (so you
won't be seeing any proper design patterns like MVC). My apologies in advance!

## Features list
- Easy to use UI with built-in tutorials
- Decode Omicam JPEG/data stream while overlaying threshold information
- Edit and upload camera thresholds on the fly
- Control the camera single board computer (reboot, shutdown, halt) and ESP32 (reboot)
- Visualise both robots' localised position on the field
- Control the robots through a multitude of useful commands including automatically resetting to starting positions
- Large buttons designed for tablet usage
- Dark theme!!!

## Building and running
Firstly, you'll need to install OpenJDK 11 and JavaFX.

### Linux
It's as simple as:

`sudo apt install openjdk-11-jdk openjdk-11-source`

`sudo apt install openjfx openjfx-source`

You may need to add the OpenJFX directory to IntelliJ if it doesn't pick it up, it should be under /usr/share/openjfx/lib.

### Mac and Windows
We recommend using [Amazon Corretto 11](https://aws.amazon.com/corretto/) or [AdotOpenJDK 11](https://adoptopenjdk.net/).
The Oracle JDKs should also work too, but due to Oracle and its associated licensing garbage I suggest you avoid them.

Next, follow [these instructions](https://developer.tizen.org/development/articles/openjdk-and-openjfx-installation-guide#install_openjfx)
to install OpenJFX for your specific platform.

### Running Omicontrol
IntelliJ IDEA is the only officially supported IDE for developing and running Omicontrol in. 

Firstly, import the "omicontrol" directory as a Gradle project. Then, add the new OpenJDK 11 installation as a Project SDK in the 
Project Structure dialogue and make it the selected one. After this, the Gradle build should work automatically (enable auto-import
if it asks you somewhere in this process).

With the project imported and built, go to Main.kt and click on the play icon next to the `main()` method. On your first run
it will look weird and complain about missing resources, to fix this, edit the run configuration to have its working directory
in the src/main/resources directory.

## Compatibility
Omicontrol has been tested on Windows, Mac OS X and Linux (KDE neon). The application _functions_ on all platforms,
however there are various quirks specific to each one. This is especially the case when using high DPI displays which
cause all manner of platform specific issues with JavaFX.

### Linux (KDE)
- Tested on a normal DPI monitor, scaling is fine
- Frequent bug where dialogue boxes will show up incredibly small (with no content). Workaround is to press ALT+F4 and
try again, it will eventually pop up.
- Linux is fully supported because it's my development machine. I only use KDE, but GTK-based desktops may be more reliable.

### Windows 10
- Tested on a high DPI display, scaling is bugged if you use Windows' fantastically named "make everything bigger" setting.
Workaround is to add either `Dprism.allowhidpi=false` or `-Dglass.win.uiScale=100%` to the VM options to disable DPI scaling.
- This will make fonts smaller, but fixes everything else.
- Windows is well supported because it's installed on our laptops.

### Mac OS X
- Tested on a high DPI (Retina?) display, not only is scaling bugged but the Windows workaround doesn't work. Instead, I
added a manual workaround which scales down the preview window to 90% it's normal size if Mac OS is detected. The Windows
workaround is not required, and actually makes things worse (blurry).
- This workaround may be added to Windows if it looks better than completely disabling DPI scaling.
- Mac OS X is partially supported because only one team member uses a Mac, but should still mostly work.

## License
Omicontrol is available under the license of the whole Team Omicron repo, see LICENSE.txt in the root directory. 

## Libraries and licenses
- [Protocol Buffers](https://github.com/protocolbuffers/protobuf): BSD 3-clause
- [TornadoFX](https://github.com/edvin/tornadofx): Apache 2
- [Apache Commons](https://commons.apache.org/): Apache 2