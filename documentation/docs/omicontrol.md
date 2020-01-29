# Omicontrol
Omicontrol is Team Omicron's wireless robot/camera debugging and management application. With the creation of Omicam,
we needed a way of visualising camera output, managing the SBC and editing thresholds. In the past, we used the OpenMV IDE,
but given that Omicam is entirely custom developed, this meant we also had to custom-develop our own camera application.

In addition, with our new advanced strategies and localisation, as well as the potential auto referee future rule addition,
we felt like this year was a good time to add a remote robot management feature with the ability to reposition, reorient
and automatically reset robots on the field wirelessly, as well as visualise their positions. 

We added both of these features into Omicontrol, to make it essentially our all-in-one robot management application.
Because the application will be used frequently in debugging as well as in the high-pressure mid-game interval, Omicontrol's
primary design goals are to be easy to use and reliable.

Omicontrol is a cross-platform application that is confirmed to work on Windows, Mac and Linux. It's developed in Kotlin
and uses JavaFX for the GUI.

![Omicontrol](images/omicontrol.png)    
_Figure 1: Omicontrol main window running on KDE neon_

**TODO: maybe make the above a video demonstration fo Omicontrol**

## Wireless connection
Being able to connect to any robot wirelessly and manage it is a great quality of life improvement, which saves looking
for cables, troubleshooting connection problems and more. There are a variety of wireless protocols in existence such as
Bluetooth Classic, Bluetooth LE, Zigbee and others. However, we decided to use TCP/IP over WiFi because it's easy to
setup and reliable, compared to Bluetooth which has issues on many platforms.

To accomplish this, the LattePanda SBC creates a WiFi access point (AP) with no internet connection that the client computer
then connects to, establishing a direct link between the two computers. As shown in Figure 1, to connect you simply have
to enter the LattePanda's IP on your local network.

Due to the nature of TCP/IP, we also observed the ability to connect to Omicontrol across the Internet. In this setup,
an Internet-connected router hosts the LattePanda device on its network via Ethernet or WiFi. The router must port forward
port 42708 which is used for Omicam to Omicontrol communications. Then, the client computer in an entirely different location,
even a different country, connects to the Omicam router's public IP address and can interact with the camera and robots
as normal. While this is interesting, nonetheless it's not used in practice due to lack of a security-focused design or 
a need to connect across large distances.

## Communication protocol
Communication between the two devices makes heavy use of Protocol Buffers, on the Omicam via nanopb, and on the 
Omicontrol end in Kotlin via the official Java API from Google. Although there's no Kotlin API for Protocol Buffers, Kotlin
is fully backwards compatible with Java so there's no issue using the generated Java files.