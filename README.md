# ![Logo](https://github.com/catid/kvm/raw/master/art/logo_44.png "Logo") SkyJack
### IP KVM / IoT Crash Cart Adapter using Raspberry Pi

This software runs on a Raspberry Pi 4 with an HDMI/USB adapter attached.

The USB-C port is configured so that the Pi acts as a keyboard/mouse emulator.

By connecting your Raspberry Pi 4 to the HDMI and USB ports of a target device, you can use a web browser to access the target device remotely, as if you were sitting at the computer with a keyboard and mouse attached to it.  You will be able to access the BIOS and boot menus of the device remotely.


## Features

Project features:

* The browser displays video from the target device.
* Keystrokes and mouse movements are relayed to the target device.
* WebRTC video and data channels are used to ensure low-latency.
* All data sent over the network is encrypted using TLS.


## Hardware Required

Amazon List (Containing these items): https://www.amazon.com/hz/wishlist/ls/25IKRAT6M9NGW

Required hardware:

* Raspberry Pi 4 (The 1 GB RAM model should work, untested): https://www.amazon.com/dp/B07TC2BK1X/
* Protective Case: https://www.amazon.com/dp/B07X5Y81C6/
* SD Card: https://www.amazon.com/dp/B073JYC4XM/
* SD Card Reader: https://www.amazon.com/dp/B006T9B6R2/
* HDMI/USB Capture Card: https://www.amazon.com/dp/B088D3QPN5/
* HDMI Cable (3.3 Feet): https://www.amazon.com/dp/B07FFS7RH1/
* USB-C to USB-A Adapter Cable (3 Feet): https://www.amazon.com/dp/B01GGKYS6E/

Recommended additional hardware:

The TinyPilot project has built a power/data splitter that is super useful when working with target devices that cannot supply full power to the Raspberry Pi.  This splitter requires two additional Micro USB cables.

* Power/Data Splitter: https://tinypilotkvm.com/product/tinypilot-power-connector
* 2x Micro USB to USB-A Adapter Cable (3 Feet): https://www.amazon.com/dp/B01JPDTZXK


## Hardware Setup

You will need a WiFi network, a laptop/desktop computer, and a target device with USB-A and HDMI ports.

Follow the included paper instructions to place the Raspberry Pi inside the protective case.

Insert the HDMI/USB Capture Card into one of the two Blue Middle USB3 ports on the Raspberry Pi 4.

Insert the USB-C to USB-A Adapter Cable into the USB-C port on the Raspberry Pi 4.


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


## Windows mDNS Installation

On Windows you will need to install the Bonjour Print Services for Windows to resolve the hostname `raspberrypi.local`:

https://support.apple.com/kb/DL999?locale=en_US

You will also need a terminal with SSH.  I recommend Git Bash from https://gitforwindows.org/


## Software Installation

SSH into the Raspberry Pi at a terminal on your computer (over WiFi): `ssh pi@raspberrypi.local` with password `raspberry`.

Use `sudo raspiconf` on the Pi to change the hostname of the Raspberry Pi to `kvm`.  There is a guide here:
https://www.tomshardware.com/how-to/raspberry-pi-change-hostname

This may be a good time to change the SSH password on the device to a more secure password.

Now build and install the KVM software:

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
```

Now to install the KVM software run:

```
cd /home/pi/kvm/scripts/
sudo ./install.sh
```


## Credits

The keyboard/mouse driver setup script is copied from the (MIT Licensed) TinyPilot software project at https://tinypilotkvm.com/

The author of TinyPilot is pushing towards commercializing using the Raspberry Pi for KVM.  I did not personally decide to commercialize this hardware solution because multiple products for IP-KVM already exist.  It does seem to be producing a decent income for the author of TinyPilot, which is interesting.

Software by Christopher A. Taylor mrcatid@gmail.com

Please reach out if you need support or would like to collaborate on a project.
