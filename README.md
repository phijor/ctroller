# ctroller

*ctroller* lets you use your 3DS as an input device for your Linux system via
the uinput kernel module. It consists of a client that runs on your 3DS,
continously streaming the 3DS input data to a server on your PC. The server
exposes a virtual device to your system, interpretes the data it receives and
writes it to a event node under `/dev/input/event*` or similar.

## Prerequisites

You will need
[DevkitARM](https://sourceforge.net/projects/devkitpro/files/devkitARM/) and the
[ctrulib](https://github.com/smealum/ctrulib) to build the 3DS component.

To run the server, the `uinput` kernel module needs to be loaded:

```bash
$ modprobe uinput
```

## Building

Run `make` to build both components, or enter either of the `3DS`/`linux`
subdirectories to build them individually by running `make` there. You can use
the `debug` target for development.

## Installation

If you are using Arch Linux,
[here's](https://aur.archlinux.org/packages/ctroller-git/) the AUR package.

1. Install the server to your system by running `make install`. This is
   equivalent to:
```bash
$ cd linux
$ make install DESTDIR="/" BINDIR="usr/bin"
```

2. build the 3DS application:
```bash
$ cd 3DS
$ make release
```

3. Copy `3DS/ctroller.{3dsx,smdh}` to `/3ds/ctroller/` on your SD card. You can
   also directly upload the application to your 3DS using
   [upload.sh](./3DS/upload.sh) (do not blindly execute unknown scrips, I'm not
   responsible if this accidentally deletes your SD card or unfreezes your
   fridge).  To do so, start a FTP server (such as
   [ftpd](https://github.com/mtheall/ftpd)) on your 3DS on port 5000, then run:
   ```bash
   $ cd 3DS
   $ make upload DSIP=<IP of your 3DS here>
   ```
   This requires `ftp` to be installed on your system.

4. Create a directory `ctroller` in the root of your SD card

5. **Place [3DS/ctroller.cfg](./3DS/ctroller.cfg) in there and replace the IP
   with the one of your PC.** (The config file should now be at
   `sdmc:/ctroller/ctroller.cfg`)

## Running

Start the server by running:
```bash
$ ./linux/ctroller
```

Usage:
```
  -d  --daemonize              execute in background
  -h  --help                   print this help text
  -u  --uinput-device=<path>   uinput character device (defaults to /dev/uinput)
```

Then launch the *ctroller.3dsx* application on your 3DS using a homebrew
launcher of your choice.

For development purposes, the 3DS-Makefile includes a `run` target that uses
`3dslink` to upload and run the application using the Homebrew Menu NetLoader.

## Notes

This program is intended to be used in a private network. For simplicity, the
server right now accepts any connection on it's port, which might pose a
security risk if others can send data to it. This will be changed in future
releases. For now, you probably shouldn't be using this in a public network.
