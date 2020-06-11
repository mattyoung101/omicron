![Omicam logo](images/omicontrol_logo_dark.png)    
Omicontrol is Team Omicron's wireless robot/camera debugging and management application. With the creation of Omicam,
we needed a way of visualising camera output, managing the SBC and editing thresholds. In the past, we used the OpenMV IDE,
but given that Omicam is entirely custom developed, this meant we also had to custom-develop our own camera application.

In addition, with our new advanced strategies and localisation, as well as the potential auto referee future rule addition,
we felt like this year was a good time to add a remote robot management feature with the ability to reposition, reorient
and automatically reset robots on the field wirelessly, as well as visualise their positions. 

We added both of these features into Omicontrol, to make it essentially our all-in-one robot management application.
Because the application will be used frequently in debugging as well as in the high-pressure mid-game interval, Omicontrol's
primary design goals are to be easy to use and reliable.

Omicontrol is a cross-platform application that works on Windows, Mac and Linux. It's developed in Kotlin
and uses JavaFX for the GUI, via the TornadoFX framework.

![Omicontrol](images/omicontrol.png)    
_Figure 1: Omicontrol main window running on KDE neon_

**TODO: maybe make the above a video demonstration fo Omicontrol**

## Views
Omicontrol functions through the concepts of "views", which provide different information about different aspects of our
robot(s).

### Camera view
The Camera View can to view camera output and tune threshold parameters.

### Field view
Similar to teams in the major league, we implement a Field View to display the robots' localised positions and enable
control of them.

### Calibration view
This view is used to calibrate the mirror dewarp model, which translates pixel distances from the centre of the mirror
into centimetre distances on the real field. In the past, this was complex and could take upwards of half an hour.
We decided this process could be greatly streamlined, so we added the calibration view to Omicontrol.

The general flow of generating a mirror model is this process:

1. Lay out a ruler, with clear tape every 5cm
2. Start Omicam and Omicontrol, and open the Calibration View
3. Click on each of the 5cm strips in order to generate data points
4. Select the regression type (exponential or polynomial)
5. Click "Calculate model"
6. Copy the results into the `mirrorModel` field in Omicam and refresh the config file

This approach streamlines mirror model calculation from being a painstaking 30 minute task, into one which can be
done in a few button presses in under 5 minutes.

### Model calculation process
Once the data points are determined by the user, we need to do a non-linear least squares regression to calculate the
unknown coefficients in the selected mirror model. For example, in the exponential `f(x) = ae^(bx)`, the unknowns are
`a` and `b`. Traditionally, we used to export the data to Excel and have it calculated in there, but we decided instead
to implement the regression ourselves.

To do this, we use the Apache Commons Math library and its `AbstractCurveFitter`. This internally uses the
Levenberg-Marquardt algorithm, an improvement over the Gauss-Newton algorithm, to converge on the most optimal values
for the unknown coefficients.

Although Commons Math has a built in `PolynomialCurveFitter` which can fit a `PolynomialFunction`, there is no 
exponential curve fitter or exponential function provided, so we built our own `ExponentialCurveFitter` and `ExpFunction`.

## Wireless connection
Being able to connect to any robot wirelessly and manage it is a great quality of life improvement, which saves looking
for cables, troubleshooting connection problems and more. There are a variety of wireless protocols in existence such as
Bluetooth Classic, Bluetooth LE, Zigbee and others. However, we decided to use TCP/IP over WiFi because it's easy to
setup and reliable, compared to Bluetooth which has issues on many platforms.

To accomplish this, the LattePanda SBC creates a WiFi access point (with no internet connection) that the client computer
then connects to, establishing a direct link between the two computers. As shown in Figure 1, the user simply types
the local IP to connect and everything else is handled for them.

Due to the nature of TCP/IP, we interestingly also have Omicontrol across the Internet. In this setup,
an Internet-connected router hosts the LattePanda device on its network via Ethernet or WiFi. The router must port forward
port 42708 which is used for Omicam to Omicontrol communications. Then, the client computer in an entirely different location,
even a different country, connects to the Omicam router's public IP address and can interact with the camera and robots
as normal. While this is interesting, nonetheless it's not used in practice due to lack of security, lack of need and
bandwidth issues.

## Wired connection
Although wireless connection is very flexible, on the LattePanda it comes at the cost of a very poor connection in some
situations and on some SBCs, unfortunately such as the LattePanda. Thus, we decided to use gigabit Ethernet as an alternative 
connection type. This also acts as a backup in case the venue bans WiFi hotspots (as has happened before), or there's too much signal 
noise to connect properly.

This works by using a special Ethernet crossover cable and creating a simple network through assigning static IPs. The  user
assigns their computer's Ethernet connection the static IP of 10.0.0.1 and the SBC an IP of 10.0.0.2. Then, the user
simply connects to 10.0.0.1 as the Remote IP in Omicam. Just like the WiFi setup, this creates an offline connection 
between the two devices at a very high bandwidth and stability. We experience no connection issues at all over Ethernet 
(although there are some occasional issues with SSH).

## Communication protocol
The backbone of bidrectional Omicam to Omicontrol communication is a simple TCP socket on port 42708, with Omicam as the host
and Omicontrol as the client.

Communication between the two devices makes heavy use of Protocol Buffers, on the Omicam side via nanopb, and on the 
Omicontrol end in Kotlin via the official Java API from Google (which also works with Kotlin).

**Should put more info here, stuff like how we basically have budget RPC with protobuf**

## Design considerations
The Omicontrol UI was designed to look professional, be simple to use, friendly to use as a tablet and work cross-platform.

**Cover how we achieved these design goals. It's just like digisol (yay.....)**