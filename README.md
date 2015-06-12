# TI CC3200 Cloud Demo for Exosite Ready

This repository contains code that demonstrates using the Exosite cloud platform
with TI's CC3200 WiFi launchpad.
This is also a demonstration for the https part of libexosite, which can support encrypted read, write, subscribe, and content download.

# Environment

This repository is compiled under "CCS 6.1.0" IDE, and use TI "CC3200SDK_1.1.0" SDK package.

# Using It

The first step is to flash all the needed files to the board. There is a `.usf`
file in the "flash" folder, this is a config file for TI's uniflash tool. It
will flash the included mcu inage and all the supporting files to the sflash
on the launch pad.

If you haven't programmed an access point profile (your WiFi SSID and key) yet,
place a jumpper between the VCC and P58 pins of the launchpad header and reset
the board. It should now reboot into AP mode. Connect to the
'mysimplelink-XXXXXX' AP with a phone or laptop and go to
http://mysimplelink.net and configure your network settings profile just like
in the out of box demo.

Once configured, remove the jumpper and reset the board. It will now connect to
your network and attempt to activate itself on the Exosite platform. That means
that you can now add it to your account on https://ti.exosite.com and it should
then be able to activate itself and begin writing data to the platform.

# Known Issues

* Bloat; There is a lot of code and html/js files left over from the out of box
  demo that this is based on. This needs to be removed and replaced with a nice
  looking, mobile friendly, single page, the C code that makes the API will
  probably stay as-is.
* Make it go into AP mode if no profiles are saved or can't connect.
