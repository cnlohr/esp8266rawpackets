#Demonstration of raw packet sending on the ESP8266 for the purpose of triangulation.

NOW 2.0.0 COMPATIBLE.  see https://github.com/cnlohr/esp8266rawpackets/issues/7

## Introduction
This project is geared for the end attempt at triangulation (Which it can't do currently) but, I have run into some useful things along the way.  Right now, this allows us to connect to an AP at the same time as send/receive arbitrary packets as well as get timestamps on both the sending and receiving.

I use the tactic outlined here: https://github.com/ernacktob/esp8266_wifi_raw to override aaEnqueueRxq.  I am compiling against 1.5.4.  Any other version will require libpp.a to be patched.  This mechanism allows us to intercet message packet receives by overriding that function.  Once overridden, all broadcast and packets targeted for our host will roll on in!  We can process them any way we want.  see esp_rawsend.c.  Keep in mind this isn't true promiscuous mode, as it does not receive packets that are unicasted to a specific node but not this node.

Sending raw packets is done by directly calling the esp ```wifi_send_pkt_freedom``` function.  I had originaly written ```RawSendBuffer```, but it seems less reliable.  For this test, I send data packets to broadcast, but use a special tag to identify them as my packets "82668266" type. 

By using the custom build script, custom.ld, either the NMI or the User interrupt can be overridden.  The custom interrupt handler in this case looks for packets with the packet type 0x82668266.   If so, it writes in the CCOUNT that happened when the interrupt fired.  This isn't very good since if there are two packets that come in one after another, or the interrupt is in use for a different cause, it will make sure the packet is skipped, or has inaccurate time.  If a packet is missing entirelly, the time code will be 0.  It is important to always remember the timecode may be very inaccurate and can be thrown out.  

As it stands, the ESPs expect to be connected to a wifi network and will send all 82668266 packets back to a host at a fixed ip via ```pUdpServer``` - this means the host that is expecting the packets should be on the connected wifi network at 192.168.1.113, port 9999.  The packets returned contain the standard Espressif wifi stack header.

## Collecting the data

We can now run a host at 192.168.1.113.  This host can bind to UDP port 9999 and collect all data from all ESPs on the network.  An example of this app can be found in toprecorder.  Data can be redirected to a data file.  Once recorded the data can be processed.

An enterprising young soul, should they choose could probably process the data in real time, if they so chose.

## 2D/3D Localization

This is where things get tricky.  Unlike GPS where we know the time of sending a packet, we can only really trust the time of packet reception.  Additionally, we don't know anything about the times on the sending nodes.  The only thing we can do is know the time a node sent a packet on a receiver's local oscillator time - we can determine the send time on the initial constellation by subtracting out the distance from one node to another.

We can synchronize nodes's clocks to one another only when two nodes receive the same packet.  We can then know they're at the same time (barring outliers due to the interrupt happening at the wrong time).  Once we do this for a while, we can get a pretty good idea what the time on one node would mean for another node.  Over a little longer while, we can start to find the clock skew between the nodes and calibrate it back out.

If we wanted to find the location of a node within the constelaltion that we didn't know the locaiton of, we could transmit, then, look at the differential receive times on that node.  This can tell us the relative distances of the transmitter to each node.  There is no way to know the absolute distance unless we can be completely confident we can synchronize it's clock and that we can get transmit time.

There is a jitter of +/-4 clock ticks, so it would take many, many readings to get a good solid reading on what the true differential value would be.

It's important to note that in our example, all of the nodes are co-planar, so only 2D localization would be possible anyway.  I recommend trying to resolve the location of node #5 to the constellation of 1-4 if you want to try to find something.

## Why it's hard.

So, this is a hard question to answer.  I think the biggest problem I have with my data is that I'm not very good at analyzing these sorts of things.  Additionally, I think I'm fighting clock jitter.  Though all crystals have natural speed differences which can be observed, easily in the +/- 5-10ppm range, the crystals also seem to have a random walk which makes it very, very hard to nail down times.  Also, figuring out time synchronization from the differential systems is... hard :( - mathematically.

I've tried doing this a number of times, and I've checked my math by changing the "distance" between nodes to make sure the distances of the true constellation should match that of the data --- and often, they don't :(.

This data is outside and ideal.  Multipath could mess things up!  Def need too look at all the encoding schemes.  Extra packets might be messing with the timing.  Could consider sending data back via serial or Ethernet?

Also, maybe you could use rssi to help resolve?
