**Further development of OMM as a distributed multi-media solution takes place in the omm-9p repository.**

# Contents

- [Introduction](https://github.com/captaingroove/omm-upnp#introduction)
- [Building from Source](https://github.com/captaingroove/omm-upnp#building-from-source)
- [Dependencies](https://github.com/captaingroove/omm-upnp#dependencies)
- [Running the Applications](https://github.com/captaingroove/omm-upnp#running-the-applications)

## Introduction

Omm is a set of applications and libraries for easily making your multimedia content available
to your local network. It aims to be compatible to the UPnP V1.0 and UPnP-AV V1.0 specification.

Omm currently includes the following components:

### omm

The main application, providing a sensible mix of Omm components.

### ommcontroller 

a UPnP-AV Media Controller
ommcontroller is the "remote control" of your multimedia system.
It lets you select your media source (UPnP-AV Server), browse the content of the server, select a media object
(song, video, tv-channel, ...), select a media output (UPnP-AV Renderer) and control the transport of the media
from Server to Renderer (start, stop, seek, ...).

### ommrenderer

a UPnP-AV Media Renderer
ommrenderer is the "set-top box" for you TV. It renders the media streams
coming from the UPnP-AV Server to a screen and a sound system.

### ommserver

a set of UPnP-AV Media Servers
Currently only a plugin for VDR (Video Disc Recorder - http://www.cadsoft.de/vdr/) is available.
It gives you access to digital television provided by VDR (DVB-S, DVB-C or DVB-T). The plugin
exposes the TV channels, you can watch live-TV and the recordings. Seeking in recordings is
supported. For browsing the EPG or setting timers you need a web based interface like vdr-admin.
For delivering other content, such as music and video files (mp3, avi, ...) you can use one
of the UPnP-AV servers out there (see compatibility list below).

### libommupnp

UPnP 1.0 implementation

### libommupnpav

UPnP AV 1.0 implementation

### ommgen

A stub generator using UPnP device descriptions to generate C++ interfaces for libommupnp

## Building from Source

**outdated**

To build Omm from source you need CMake as of version 2.6 or later. In the top-level directory
just run:

$ ./make.sh config
$ ./make.sh

and you should be done.

Install the binaries directly from the build tree
$ sudo ./make.sh install

or create packages:
$ ./make.sh package

## Dependencies

**outdated**

All Omm components depend on the Poco libraries (http://pocoproject.org/).
Further dependencies are:

ommcontroller: Qt 4.4.3 or later
ommrenderer: X with support for xvideo is currently recommended
ommrenderer VLC engine: libvlc (as of vlc 0.9.4 or later)
ommrenderer mplayer engine: mplayer at least 1.0 pre-something installed as a runtime dependency
ommrenderer xine engine: libxine 1.1.15 or later
ommserver-vdr: VDR 1.6.0 with streamdev-server plugin

The mplayer engine has an incredible good seek performance on http streams, but has some flaws when
it comes to controlling it via IPC.

VLC has a clean library design and an overall good performance, especially on mpeg streams via http.
That's nice for TV streaming. However, seeking and AV-Transport related control is not as fast as mplayer's.

The xine engine is imcomplete and probably not working. Seeking on http streams is not implemented in
libxine, so this should be done first do reactivate it for Omm.

So, currently I recommend the VLC engine.

Update of dependencies (2019-10-11):
libommnet on Linux: libudev, libdbus, libdbus-c++, network-manager-dev
libommgui on Linux: Qt4 including webkit

Packaging dependencies (2014-04-22):
target mingw-linux: NSIS

## Running the Applications

**outdated**

This is pretty straight forward: just run ommcontroller or ommrenderer on a single or on several different machines.
No configuration is needed, in the first place. All UPnP-AV Servers and UPnP-AV Renderers announce
themselves on the local network and then pop up in ommcontroller. You can quit or start a second ommcontroller while
some media is rendered, so different instances of ommcontroller can concurrently control one playing media.
On the other hand, one instance of ommcontroller can control different instances of ommrenderer, for example one
in the living-room and one in the kitchen. Your mileage may vary.

ommserver-vdr must be installed as a vdr-plugin, usually in /usr/lib/vdr/plugins. The streamdev-plugin must be
configured and working on the standard ports. Don't forget to add:

streamdev-server.AllowSuspend = 1
streamdev-server.SuspendMode = 1

to vdr's setup.conf, to enable changing of transponders on the stream's request.
