# Omicontrol
Omicontrol is Team Omicron's custom remote debugging and management application that we use to communicate with Omicam running
on our single board computer. It enables team members to control the robot's movement, view helpful debug information as well
as tune and view camera output.

The app is written in Kotlin using JavaFX (via TornadoFX) for the UI. It receives data from Omicam in the format of
Protocol Buffers over a TCP socket. The code quality in this project is kind of bad, because I'm not experienced with 
UI development, and spend most of my time working on Omicam. My apologies in advance!

Omicontrol is built and maintained by Matt Young.

## Features list
- Relatively easy to use UI, views should be mostly self-explanatory
- Decode Omicam JPEG/data stream while overlaying threshold information
- Edit and upload camera thresholds on the fly
- Many helpful diagnostic displays in the Field View for debugging the localiser
- Automatic mirror model calculation through non-linear least squares regression (exponential and polynomial models supported)
- Control the robots through a multitude of useful commands including automatically resetting to starting positions
- Mirror dewapring. Turns 360 mirror view into an orthogonal perspective (separate tool, CameraDewarper.kt)
- Lots of useful shortcuts and other quality of life improvements built-in
- Large buttons designed for tablet usage
- Dark theme!!!

### Stalled/incomplete features
Due to the COVID-19 pandemic, development on Omicontrol stalled around July 2020. As of the end of 2020 open source release,
the following features remain incomplete:

- Robot control in field view. Transmission of robot move commands in Protobuf works correctly, but currently the ESP32
doesn't respond to them, and doesn't forward them over Bluetooth to the other robot.
- Replay view that can and play *.omirec files. Halted due to lack of interest.

## Building and running
Firstly, you'll need to install OpenJDK 11 and JavaFX.

### Linux
It's as simple as:

`sudo apt install openjdk-11-jdk openjdk-11-source`

`sudo apt install openjfx openjfx-source`

You may need to add the OpenJFX directory to IntelliJ if it doesn't pick it up, it should be under /usr/share/openjfx/lib.

### Mac and Windows
I recommend using [Amazon Corretto 11](https://aws.amazon.com/corretto/) or [AdotOpenJDK 11](https://adoptopenjdk.net/).
The Oracle JDKs should also work too, but due to Oracle and their associated licencing garbage I suggest you avoid them.

Next, follow [these instructions](https://developer.tizen.org/development/articles/openjdk-and-openjfx-installation-guide#install_openjfx)
to install OpenJFX for your specific platform.

### Running Omicontrol
IntelliJ IDEA is the only officially supported IDE for developing and running Omicontrol in. 

Firstly, import the "omicontrol" directory as a Gradle project. Then, add the new OpenJDK 11 installation as a Project SDK in the 
Project Structure dialogue and make it the selected one. After this, the Gradle build should work automatically.

With the project imported and built, go to Main.kt and click on the play icon next to the `main()` method. On your first run
it will look weird and complain about missing resources, to fix this, edit the run configuration to have its working directory
in the src/main/resources directory.

**Very important:** If you're on Windows, please add an environment variable called "HOME" which is your home directory, 
or the output of `echo %userprofile%`. In IntelliJ, you can also add this to the environment variables section in the run
configuration. In the future there'll be a workaround for this but it's not been added yet.

To run a release build, use the following command:

`java -XX:+UseStringDeduplication --illegal-access=deny -Dglass.win.uiScale=100% -jar omicontrol-<VERSION>.jar`

## Compatibility
Omicontrol has been tested on Windows, Mac OS X and Linux. The application _functions_ on all platforms,
however there are various quirks specific to each one. This is especially the case when using high DPI displays which
cause all manner of platform specific issues with JavaFX.

### Linux
- Distro: KDE neon and Linux Mint
- Tested on a normal DPI monitor, scaling is fine
- Frequent bug where dialogue boxes will show up incredibly small (with no content). Workaround is to press ALT+F4 and
try again, it will eventually pop up.
- Linux is fully supported because it's my development machine. Both KDE and GTK environments appear to work correctly.

### Windows 10
- Tested on a high DPI display, scaling is bugged if you use Windows' fantastically named "make everything bigger" setting.
Workaround is to add either `Dprism.allowhidpi=false` or `-Dglass.win.uiScale=100%` to the VM options to disable DPI scaling.
- This will make fonts smaller, but fixes everything else.
- Windows is well supported because it's installed on our laptops that we use at the venue.

### Mac OS X
- Tested on a high DPI (Retina?) display, not only is scaling bugged but the Windows workaround doesn't work. Instead, I
added a manual workaround which scales down the preview window to 90% its normal size if Mac OS is detected. The Windows
workaround is not required, and actually makes things worse (blurry).
- This workaround may be added to Windows if it looks better than completely disabling DPI scaling.
- Mac OS X is partially supported because only one team member uses a Mac, but should still mostly work.

## Licence
Omicontrol is licenced under the Mozilla Public License 2.0, the same as the rest of Team Omicron's code.

## Open source libraries used
- [Protocol Buffers](https://github.com/protocolbuffers/protobuf): BSD 3-clause licence
- [TornadoFX](https://github.com/edvin/tornadofx): Apache 2 licence
- [Apache Commons Math, Lang, IO](https://commons.apache.org/): Apache 2 licence
- [GreenRobot EventBus](https://github.com/greenrobot/EventBus): Apache 2 licence
- [exp4j](https://www.objecthunter.net/exp4j/): Apache 2 licence
- [colormath](https://github.com/ajalt/colormath): Apache 2 licence