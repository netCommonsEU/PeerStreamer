# PeerStreamer for Community Networks

This is the official Repository for the developments made on the
PeerStreamer platform for the [netCommons](http://netcommons.eu)
research project.

## Requirements

Tested on Ubuntu 16.04 LTS (it should work on any Linux distribution with
proper developing tools installed).

Streamers requires the following libraries for in order to compile:

* [GRAPES] (https://github.com/netCommonsEU/PeerStreamer-grapes)
* [NAPA] (https://github.com/netCommonsEU/PeerStreamer-napa-baselibs)
* [libevent2] (http://monkey.org/~provos/libevent/)

## Build

We recommend using the [PeerStreamer build system]
(https://github.com/netCommonsEU/PeerStreamer-build).

### Manual build

Assuming $GRAPES is the full path to the root directory of the compiled
GRAPES libraries, $NAPA is the full path to the root directory of the compiled
NAPA libraries and $LIBEV is the full path to the root directory of the compiled
libevent library, then you can execute the following two commands:

` ./configure --with-grapes=$GRAPES --with-napa=$NAPA --with-libevent=$LIBEV
--with-net-helper=ml --with-static=2 `
`make`

Use --with-static=0 if you want to compile a dynamically linked version of the
streamer.

## Installation

The executable can be found in the PeerStreamer root directory at the end of the
build process. Its name is streamer-ml-grapes-static in case of statically
linked executable of streamer-ml-grapes, otherwise.
For installing the streamer, move the executable in one of the directoris in
your $PATH.

## Usage

Work in progress.

## Testing
See [testing instructions](testing/Testing.md).



