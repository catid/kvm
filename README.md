# ![Logo](https://github.com/catid/kvm/raw/master/art/logo.png "Logo") SkyJack
### IoT Crash Cart Adapter

This software runs on a Raspberry Pi 4 with an HDMI to USB adapter attached.
The USB C port is configured so that the Pi acts as a keyboard/mouse emulator, mass storage device, and ethernet adapter.
The ethernet adapter mode is used to configure the WiFi SSID and passphrase.

Once it is on the WiFi, the adapter becomes an IoT device reachable via web browser.
The browser displays the video from the target device, and keystrokes/mouse movements are relayed to the target.
