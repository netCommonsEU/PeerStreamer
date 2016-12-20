# Testing procedure for PeerStreamer

PeerStreamer for CNs is split in two applications, the streaming
engine that runs on a CN node, equipped with OpenWRT and a dedicated
device which runs PeerViewer, the web-based visualizer of the video
(alternatively, other tools like ffplay or VLC can be used for live video
playback).

In a typical configuration, the streaming engine written in C runs on
the router node, it does only the "chunk trading" function and requires
few computation resources. Another node runs PeerViewer, it can be a
PC, an embedded device such as a Raspberry. PeerViewer opens a web
interface where the video is streamed, anybody accessing the web interface
can watch the video.

To perform tests one needs to set-up the following testing environment,
in increasing complexity.

>### *Requirements*
The following tests assume you were able to build and install both PeerStreamer
and PeerViewer using the [PeerStreamer Build System]
(https://github.com/netCommonsEU/PeerStreamer-build).
For downloading the scripts and videos required by the tests execute the
following command from the root directory of the [PeerStreamer Build System]
(https://github.com/netCommonsEU/PeerStreamer-build):
`make download_tests`

### High level software architecture

The figure reports a high level architecture of the software modules required
for running initial tests. PeerStreamer and PeerViewer are provided by the
University of Trento.
for the other modules, standard open source tools are used.

![alt text](figures/high_level_architecture_v3.png "PeerStreamer software architecture")

* Video/Audio source: for initial testing a file containing audio+video will be
  used. Currently supported format are: VP8 for video, Opus or MP3 for audio).
The University of Trento can provide the video testing files.

* RTP flows generator: any software capable of generating Video/Audio RTP/RTCP
  sessions (e.g., VLC, ffmpeg, GStreamer). The University of Trento will provide test examples with
GStreamer.

* PeerStreamer entry/exit points: peerstreamer software. The University of
  Trento will provide examples for building and executing it.

* RTP flows parser: when the PeerStreamer exit point is configured as a
  dechunkizer it will output the original RTP/RTCP sessions. In this case a
proper RTP/RTCP session parser is required (e.g., VLC, ffmpeg, GStreamer). The
University of Trento
will provide examples with GStreamer.

* Video/Audio Player: if the RTP flows parser is used, a proper player is
  required for reproducing the video in real time (e.g., ffplay, VLC).

* PeerViewer: this software is provided by the University of Trento and its purpose is to
  dechunkise
the chunks produced by the PeerStreamer exit point and converting them in an
HTTP flow which enables a web browser to reproduce the video/audio in real-time.
Any modern web browser can be used for this purpose. Most of the initial tests
will relies on PeerViewer for reproducing the multimedia content.
PeerViewer is written in Go.

## Simple testing, one host

### Video Streaming without PeerViewer (Streamed video saved on a file)

This test is executed on a single host running Ubuntu 16.04 LTS. This test does
not run a live video playback, but instead it uses GStreamer for saving the
streamed video on a file that can be reproduced afterwards by any video player
which supports H264 video + AAC audio wrapped in a Matroska container.

![alt text](figures/single_host_test_no_peerviewer_output_file.png "Single host testing, no peerviewer, no live playback")

In this case the Video/Audio source is a file located in
testing/videos/sintel_trailer_h264_aac.mkv (relative path with respect to the
root directory of the PeerStreamer Build System). The file is given as input to
GStreamer which plays the role of the "RTP flows generator" and generates the video
and audio RTP/RTCP flows respectively on UDP ports 5000/5001 and 5002/5003. The
RTC/RTCP flows are received by the PeerStreamer entry point that embeds them
into PeerStreamer chunks that are then forwarded to the PeerStreamer exit point
on UDP port 6000. The PeerStreamer exit point extract the original RTP/RTCP
video and audio flows from the chunks and forward them to the RTP flows parser
respectively on UDP ports 7000/7001 and 7002/7003. This test uses again
GStreamer as RTP flows parser which saves the streamed video in /tmp/test.mkv.

This test can be completely automatized. From the root directory of the
[PeerStreamer Build System]
(https://github.com/netCommonsEU/PeerStreamer-build) execute the following
command:

`make test_file_ouptut`

After the test complete it is possible to play the streamed video with any video
player which supports H264 video + AAC audio wrapped in a Matroska container.
For example, using ffplay:

`ffplay /tmp/test.mkv`


### Video Streaming without PeerViewer, live video playback using ffplay

This test is very similar to the previous one. The difference is that in this
test GStreamer is not used anymore as the "RTP flow parser" for saving the
streamed video on a file. Instead, the test use ffplay as the "RTP flow parser"
for playing the streamed video in real-time.

Also this test can be completely automatized. From the root directory of the
[PeerStreamer Build System]
(https://github.com/netCommonsEU/PeerStreamer-build) execute the following
command:

`make test_ffplay_live_playback`

After a few seconds you shuld start seeing the streamed video on screen.


### Basic PeerViewer Test

This test is used for checking if PeerViewer is installed correctly on the
system. This test does not perform any video streaming. Instead, PeerViewer is
configured to generate test video/audio flows that are streamed through HTTP to
a browser.

For starting the test, execute the following command the root directory of the
[PeerStreamer Build System]
(https://github.com/netCommonsEU/PeerStreamer-build):

`make test_peerviewer_basic`

at this point you should be able to reach the PeerViewer web interface pointing
your browser to http://localhost:8080/.

### Video Streaming and live video playback with PeerViewer

As reported in the figure below, in this test we are going to use PeerViewer for live
video/audio playback. For performing this test follow the following three
steps:

1. execute `make start_peerviewer_webserver`
2. Point your browser to <http://localhost:8080/watch/0> (currently tested only with an up-to-date Chrome browser)
3. execute `make test_peerviewer_webm`

After a few seconds you should be able to visualize the video in the web browser
window. Note that because of buffering management the video playback could
require up to 50 seconds before starting.

![alt text](figures/single_host_test.png "Single host testing")

## Three-nodes testing

>### *NOTE*
This test requires a Raspberry Pi 2/3 device with PeerViewer installed. Please,
look at the [instructions for installing PeerViewer on Raspberry Pi]
(https://github.com/netCommonsEU/PeerStreamer-peerviewer/blob/D3.2-testing/docs/raspberry_build.md).

This test uses three nodes (called Node A, B and C) connected to each other by a
switch. The three nodes are:

* Node A: this node is used as the GStreamer source and the PeerStreamer
  Chunkiser/Dechunkiser. This node is an x86_64 device running Ubuntu 16.04 LTS
  and configured with the following IP address: 192.168.0.2/24. For installing
  PeerStreamer and GStreamer it's possible to use the [PeerStreamer Build System]
  (https://github.com/netCommonsEU/PeerStreamer-build):

* Node B: this node is used for running PeerViewer. This is a Raspberry Pi 2/3
  running Raspbian Jessie Lite and configured with the following IP address:
  192.168.0.100/24. The procedure for installing PeerViewer on the Raspberry Pi
  is described [here]
  (https://github.com/netCommonsEU/PeerStreamer-peerviewer/blob/D3.2-testing/docs/raspberry_build.md).

* Node C: this node is the PeerViewer client and the only requirements is that
  it is able to run an up-to-date version of the Chrome browser. We assume the
  node is configured with the following IP address: 192.168.0.1/24.

Below are reported the step-by-step instructions for executing the test:

### On Node B

Create the PeerViewer configuration file with the following command:

```bash
echo '{"http": { "listen": ":8080" }, "streams": [ { "description": "Sintel trailer",  "listen": ":60006", "kind": "video-webm" } ]}' > /tmp/config.json
```

Run PeerViewer with the following command:

```bash
peerviewer -c /tmp/config.json
```


### On Node C

Point a web browser (Chrome suggested) to http://192.168.0.100:8080/watch/0


### On Node A

Download the test video with the following command:

```bash
wget --no-check-certificate 'https://docs.google.com/uc?export=download&id=0B5RVMOFu09QCWVAtdEJvNnc5bkk' -O /tmp/test_video.webm
```

Start the PeerStreamer chunkiser with the following command:

```bash
peerstreamer -I lo -n stun_server=localhost,verbosity=3 -f null,chunkiser=rtp,chunk_size=2000,max_delay_ms=50,verbosity=2,video=5000,audio=5002 -P 6000 > /tmp/chunkiser.log 2>&1 &
```

Start the PeerStreamer Dechunkiser with the following command:

```bash
peerstreamer -I lo -i 127.0.0.1 -n stun_server=localhost,verbosity=3 -p 6000 -F null,dechunkiser=rtp,verbosity=2,video=7000,audio=7002 -x 60006 -y 192.168.0.100 > /tmp/dechunkiser.log 2>&1 &
```

Start the streaming with the following command:

```bash
gst-launch-1.0 -v rtpbin name=rtpbin filesrc location=/tmp/test_video.webm ! queue ! matroskademux name=demux demux. ! queue ! rtpvp8pay ! rtpbin.send_rtp_sink_0 rtpbin.send_rtp_src_0 ! udpsink port=5000 rtpbin.send_rtcp_src_0 ! udpsink port=5001 sync=false async=false demux. ! queue ! rtpopuspay ! rtpbin.send_rtp_sink_1 rtpbin.send_rtp_src_1 ! udpsink port=5002 rtpbin.send_rtcp_src_1 ! udpsink port=5003 sync=false async=false
```

After a few seconds you should be able to visualize the video in the web browser
window on Node C. Note that because of buffering management the video playback
could require up to 50 seconds before starting.

At the end of the test, execute the following command on Node A for stopping
PeerStreamer:

`killall peerstreamer`


## Real network testing

>### *NOTE 1*
This test requires a Raspberry Pi 2/3 device with PeerViewer installed. Please,
look at the [instructions for installing PeerViewer on Raspberry Pi]
(https://github.com/netCommonsEU/PeerStreamer-peerviewer/blob/D3.2-testing/docs/raspberry_build.md).

>### *NOTE 2*
This test requires two devices running OpenWRT with PeerStreamer installed.
Please, look at the [instruction for building PeerStreamer for OpenWRT]
(https://github.com/netCommonsEU/PeerStreamer-build/blob/D3.2-testing/docs/openwrt_build.md).
In the following we assume the PeerStreamer executable name on OpenWRT is
peerstreamer.

This test uses five nodes (called Node A, B, C, D and E) connected to each other
by a switch. The five nodes are:

* Node A: this node is used as the GStreamer source. This node is an x86_64
  device running Ubuntu 16.04 LTS and configured with the following IP address:
  192.168.0.2/24.

* Node B: this node is used for running PeerViewer. This is a Raspberry Pi 2/3
  running Raspbian Jessie Lite and configured with the following IP address:
  192.168.0.100/24. The procedure for installing PeerViewer on the Raspberry Pi
  is described [here]
  (https://github.com/netCommonsEU/PeerStreamer-peerviewer/blob/D3.2-testing/docs/raspberry_build.md).

* Node C: this node is used as a PeerStreamer Chunkiser. This is a device
  running OpenWRT and with a network interface named eth0 configured with the
  following IP address: 192.168.0.50/24.

* Node D: this node is used as a PeerStreamer Dechunkiser. This is a device
  running OpenWRT and with a network interface named eth0 configured with the
  following IP address: 192.168.0.51/24. The procedure for building PeerStreamer
  for OpenWRT is described [here]
  (https://github.com/netCommonsEU/PeerStreamer-build/blob/D3.2-testing/docs/openwrt_build.md).

* Node E: this node is the PeerViewer client and the only requirements is that
  it is able to run an up-to-date version of the Chrome browser. We assume the
  node is configured with the following IP address: 192.168.0.1/24.

Below are reported the step-by-step instructions for executing the test:


### On Node B

Create the PeerViewer configuration file with the following command:

```bash
echo '{"http": { "listen": ":8080" }, "streams": [ { "description": "Sintel trailer",  "listen": ":60006", "kind": "video-webm" } ]}' > /tmp/config.json
```

Run PeerViewer with the following command:

```bash
peerviewer -c /tmp/config.json
```

### On Node E

Point a web browser (Chrome suggested) to http://192.168.0.100:8080/watch/0


### On Node C

Start the PeerStreamer chunkiser with the following command:

```bash
peerstreamer -I eth0 -n stun_server=localhost,verbosity=3 -f null,chunkiser=rtp,chunk_size=2000,max_delay_ms=50,verbosity=2,video=5000,audio=5002 -P 6000 > /tmp/chunkiser.log 2>&1 &
```

### On Node D

Start the PeerStreamer Dechunkiser with the following command:

```bash
peerstreamer -I eth0 -i 192.168.0.50 -n stun_server=localhost,verbosity=3 -p 6000 -F null,dechunkiser=rtp,verbosity=2,video=7000,audio=7002 -x 60006 -y 192.168.0.100 > /tmp/dechunkiser.log 2>&1 &
```

### On Node A

Download the test video with the following command:

```bash
wget --no-check-certificate 'https://docs.google.com/uc?export=download&id=0B5RVMOFu09QCWVAtdEJvNnc5bkk' -O /tmp/test_video.webm
```

Start the streaming with the following command:

```bash
gst-launch-1.0 -v rtpbin name=rtpbin filesrc location=/tmp/test_video.webm ! queue ! matroskademux name=demux demux. ! queue ! rtpvp8pay ! rtpbin.send_rtp_sink_0 rtpbin.send_rtp_src_0 ! udpsink host=192.168.0.50 port=5000 rtpbin.send_rtcp_src_0 ! udpsink host=192.168.0.50 port=5001 sync=false async=false demux. ! queue ! rtpopuspay ! rtpbin.send_rtp_sink_1 rtpbin.send_rtp_src_1 ! udpsink host=192.168.0.50 port=5002 rtpbin.send_rtcp_src_1 ! udpsink host=192.168.0.50 port=5003 sync=false async=false
```

After a few seconds you should be able to visualize the video in the web browser
window on Node E. Note that because of buffering management the video playback
could require up to 50 seconds before starting.

At the end of the test, execute the following command on Nodes C and D for stopping
PeerStreamer:

`killall peerstreamer`


## Expected feedbacks

### Before the tests

* Does the partner responsible for testing the PeerStreamer framework satisfy
  the hardware requirements described above?

* Does the partner responsible for testing the PeerStreamer framework agree with
  the software requirements described above (Linux distribution and version,
support software like RTP flows generator and video/audio source)?

* Any other question and doubt about the high level tests description provided
  above (More detailed guides for building and running the required software will
be provided).

### After the tests

* Qualitative considerations about the quality of real-time video playback.

* Opinions and feedbacks about the process for building and running the
  software, with a focus on PeerStreamer and PeerViewer.

