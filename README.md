# ![Logo](https://github.com/catid/kvm/raw/master/art/logo_44.png "Logo") IP KVM / IoT Crash Cart Adapter using Raspberry Pi 4

Provides secure [out-of-band](https://en.wikipedia.org/wiki/Out-of-band_management) remote management of IT assets, for under $100.

By connecting your Raspberry Pi 4 to the HDMI and USB ports of a target device, you can use a web browser to access the target remotely, as if you were sitting at the target with a keyboard and mouse attached.  You will be able to access the BIOS and boot menus of the device remotely.

[Project Video](https://www.youtube.com/watch?v=X4b9830Ifjo)

Features:

* The browser displays video from the target device.
* Keystrokes and mouse movements are relayed to the target device.
* Supports multiple sessions with the device in different browser windows.
* All data sent over the network is encrypted using TLS.
* Robust to replugging HDMI capture card while in use.
* CBR H.264 WebRTC video and data channels are used to ensure consistently low latency.
* Motion (click/key) to photons (video update in browser) latency: 200 milliseconds.


## Overview

[Hardware Required](#hardware-required): Connect all the hardware.

[OS Installation](#os-installation): Set up the Raspberry Pi with SSH and WiFi access.

[Windows Setup](#windows-setup): Prepare the Windows computer to access the Raspberry Pi by hostname, and install a terminal app.

[Software Installation](#software-installation): Clone this repo, build the software, and run the install script as root.

[How To Use](#how-to-use): Connect the Pi to a target device.

Navigate to https://kvm.local/ to access the KVM web app.


## Hardware Required

Amazon List (Containing these items): https://www.amazon.com/hz/wishlist/ls/25IKRAT6M9NGW

Required hardware:

* Raspberry Pi 4 (The 1 GB RAM model should work, untested): https://www.amazon.com/dp/B07TC2BK1X/
* Protective Case: https://www.amazon.com/dp/B07X5Y81C6/
* SD Card: https://www.amazon.com/dp/B073JYC4XM/
* SD Card Reader: https://www.amazon.com/dp/B006T9B6R2/
* HDMI/USB Capture Card: https://www.amazon.com/dp/B088D3QPN5/
* HDMI Cable (3.3 Feet, or shorter): https://www.amazon.com/dp/B07FFS7RH1/
* USB-C to USB-A Adapter Cable (3 Feet, or shorter): https://www.amazon.com/dp/B01GGKYS6E/

![Hardware Setup](https://github.com/catid/kvm/raw/master/art/hw_setup.jpg "Hardware Setup") 

The software is compatible with this fanless heatsink case and will not cause it to overheat.

Recommended additional hardware:

The TinyPilot project has built a power/data splitter that is super useful when working with target devices that cannot supply full power to the Raspberry Pi.  This splitter requires two additional Micro USB cables.

* Power/Data Splitter: https://tinypilotkvm.com/product/tinypilot-power-connector
* 2x Micro USB to USB-A Adapter Cable (3 Feet, or shorter): https://www.amazon.com/dp/B01JPDTZXK


## OS Installation

Install the Raspbian OS on your Raspberry Pi 4 and configure its hostname.

Insert SD Card into the SD Card Reader, and insert the reader into your computer.

On Windows, download the Raspberry Pi Imager and use it to flash the SD card.

https://www.raspberrypi.org/software/

After flashing the SD card, while it is still plugged into your computer, open the mounted SD card folder and add two new files:

* One empty file called `ssh` with no extension.
* One file called `wpa_supplicant.conf` configured as described in this guide: https://raspberrytips.com/raspberry-pi-wifi-setup/

These modifications will enable SSH over WiFi for the Raspberry Pi without needing to attach a keyboard/monitor to the Pi.

This software currently requires that most of the system settings are the defaults, so avoiding unnecessary customization is recommended (at first).  For example the user name should be `pi` which is the default.


## Windows Setup

On Windows you will need to install the Bonjour Print Services for Windows to resolve the hostname `kvm.local`:

https://support.apple.com/kb/DL999?locale=en_US

You will also need a terminal with SSH.  I recommend Git Bash from https://gitforwindows.org/


## Software Installation

SSH into the Raspberry Pi at a terminal on your computer (over WiFi): `ssh pi@raspberrypi.local` with password `raspberry`.

Use `sudo raspiconf` on the Pi to change the hostname of the Raspberry Pi to `kvm`.  There is a guide here:
https://www.tomshardware.com/how-to/raspberry-pi-change-hostname

This may be a good time to change the SSH password on the device to a more secure password.

Build and install the KVM software in the SSH session:

```
cd ~
git clone git@github.com:catid/kvm.git
cd kvm

sudo apt install janus janus-dev cmake g++ libglib2.0-dev

sudo systemctl disable janus

mkdir build
cd build
cmake ..
make -j4

cd /home/pi/kvm/scripts/
sudo ./install.sh

sudo sync

sudo reboot now
```

When the Raspberry Pi reboots, it will be hosting the KVM services.  You can view the logs by connecting to `ssh pi@kvm.local` with password `raspberry`, and then entering `sudo journalctl -fu kvm_webrtc -n 10000`


## How To Use

Insert the HDMI/USB Capture Card into one of the two Blue Middle USB3 ports on the Raspberry Pi 4.  Connect the HDMI cable to the target device.

Insert the USB-C to USB-A Adapter Cable into the USB-C port on the Raspberry Pi 4.  Connect the USB-A cable to the target device.

Power on the target device.

Navigate to https://kvm.local/ to access the KVM web app.


## Credits

The most complete Pi KVM project is probably https://github.com/pikvm/pikvm/ which has a lot of additional features, such as supporting other video capture hardware and other versions of the Raspberry Pi.  The Pi KVM project has a wealth of information in the repo and has been in development for years.

The keyboard/mouse driver setup script is copied from the (MIT Licensed) TinyPilot software project at https://tinypilotkvm.com/

The author of TinyPilot is pushing towards commercializing using the Raspberry Pi for KVM.  I did not personally decide to commercialize this hardware solution because multiple products for IP-KVM already exist.  It does seem to be producing a decent income for the author of TinyPilot despite the competition, which is interesting.

Software by Christopher A. Taylor mrcatid@gmail.com

Please reach out if you need support or would like to collaborate on a project.
