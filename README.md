# ![Logo](https://github.com/catid/kvm/raw/master/art/logo_44.png "Logo") SkyJack
### IoT Crash Cart Adapter

This software runs on a Raspberry Pi 4 with an HDMI to USB adapter attached.
The USB C port is configured so that the Pi acts as a keyboard/mouse emulator, mass storage device, and ethernet adapter.
The ethernet adapter mode is used to configure the WiFi SSID and passphrase.

Once it is on the WiFi, the adapter becomes an IoT device reachable via web browser.
The browser displays the video from the target device, and keystrokes/mouse movements are relayed to the target.


## Building

```
    sudo apt install janus janus-dev cmake g++ libglib2.0-dev
```

## Set up USB gadgets

```
sudo dd if=/dev/zero of=~/usbdisk.img bs=1024 count=1024
sudo mkdosfs ~/usbdisk.img
```

```
echo "dtoverlay=dwc2" | sudo tee -a /boot/config.txt
echo "dwc2" | sudo tee -a /etc/modules
echo "libcomposite" | sudo tee -a /etc/modules
```

```
crontab -e
```

Add:

```
@reboot /home/pi/kvm/scripts/setup_gadget.sh
```
