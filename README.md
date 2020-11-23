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
echo "dtoverlay=dwc2,dr_mode=peripheral" | sudo tee -a /boot/config.txt
echo "dwc2" | sudo tee -a /etc/modules
echo "libcomposite" | sudo tee -a /etc/modules

# Create multi-function widget
sudo mkdir -p /sys/kernel/config/usb_gadget/kvm
echo "0x1d6b" | sudo tee -a /sys/kernel/config/usb_gadget/kvm/idVendor
echo "0x0104" | sudo tee -a /sys/kernel/config/usb_gadget/kvm/idProduct
echo "0x0100" | sudo tee -a /sys/kernel/config/usb_gadget/kvm/bcdDevice
echo "0x0200" | sudo tee -a /sys/kernel/config/usb_gadget/kvm/bcdUSB

sudo mkdir -p /sys/kernel/config/usb_gadget/kvm/configs/c.1

# HID
sudo mkdir -p /sys/kernel/config/usb_gadget/kvm/functions/hid.usb0
echo 1 | sudo tee -a /sys/kernel/config/usb_gadget/kvm/functions/hid.usb0/protocol
echo 1 | sudo tee -a /sys/kernel/config/usb_gadget/kvm/functions/hid.usb0/subclass
echo 8 | sudo tee -a /sys/kernel/config/usb_gadget/kvm/functions/hid.usb0/report_length
echo "\\x05\\x01\\x09\\x06\\xa1\\x01\\x05\\x07\\x19\\xe0\\x29\\xe7\\x15\\x00\\x25\\x01\\x75\\x01\\x95\\x08\\x81\\x02\\x95\\x01\\x75\\x08\\x81\\x03\\x95\\x05\\x75\\x01\\x05\\x08\\x19\\x01\\x29\\x05\\x91\\x02\\x95\\x01\\x75\\x03\\x91\\x03\\x95\\x06\\x75\\x08\\x15\\x00\\x25\\x65\\x05\\x07\\x19\\x00\\x29\\x65\\x81\\x00\\xc0" | sudo tee -a /sys/kernel/config/usb_gadget/kvm/functions/hid.usb0/report_desc
sudo ln -s /sys/kernel/config/usb_gadget/kvm/functions/hid.usb0 /sys/kernel/config/usb_gadget/kvm/configs/c.1/

DISK_FILE=/home/pi/usbdisk.img
sudo mkdir -p /sys/kernel/config/usb_gadget/kvm/functions/mass_storage.usb0
echo 1 | sudo tee -a /sys/kernel/config/usb_gadget/kvm/functions/mass_storage.usb0/protocol/stall
echo 0 | sudo tee -a /sys/kernel/config/usb_gadget/kvm/functions/mass_storage.usb0/lun.0/cdrom
echo 0 | sudo tee -a /sys/kernel/config/usb_gadget/kvm/functions/mass_storage.usb0/lun.0/ro
echo 0 | sudo tee -a /sys/kernel/config/usb_gadget/kvm/functions/mass_storage.usb0/lun.0/nofua
echo $DISK_FILE | sudo tee -a /sys/kernel/config/usb_gadget/kvm/functions/mass_storage.usb0/lun.0/file
sudo ln -s /sys/kernel/config/usb_gadget/kvm/functions/mass_storage.usb0 /sys/kernel/config/usb_gadget/kvm/configs/c.1/
```
