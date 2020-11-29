#!/bin/sh
BASE=/sys/kernel/config/usb_gadget/kvm

# Create HID widget
sudo mkdir -p $BASE
# USB 2.0
echo "0x0200" | sudo tee -a $BASE/bcdUSB
# Device class = 3 Keyboard
echo "3" | sudo tee -a $BASE/bDeviceClass
# FIXME: Select unique ids
echo "0x413C" | sudo tee -a $BASE/idVendor
echo "0x2501" | sudo tee -a $BASE/idProduct
# Version
echo "0x0100" | sudo tee -a $BASE/bcdDevice

# Strings
sudo mkdir $BASE/strings/0x409
echo "0123456789" | sudo tee -a $BASE/strings/0x409/serialnumber
echo "catid" | sudo tee -a $BASE/strings/0x409/manufacturer
echo "SkyJack" | sudo tee -a $BASE/strings/0x409/product

HID=$BASE/functions/hid.usb0

# HID
sudo mkdir -p $HID
echo 1 | sudo tee -a $HID/protocol
echo 1 | sudo tee -a $HID/subclass
echo 8 | sudo tee -a $HID/report_length
echo "\\x05\\x01\\x09\\x06\\xa1\\x01\\x05\\x07\\x19\\xe0\\x29\\xe7\\x15\\x00\\x25\\x01\\x75\\x01\\x95\\x08\\x81\\x02\\x95\\x01\\x75\\x08\\x81\\x03\\x95\\x05\\x75\\x01\\x05\\x08\\x19\\x01\\x29\\x05\\x91\\x02\\x95\\x01\\x75\\x03\\x91\\x03\\x95\\x06\\x75\\x08\\x15\\x00\\x25\\x65\\x05\\x07\\x19\\x00\\x29\\x65\\x81\\x00\\xc0" | sudo tee -a $HID/report_desc

# Bus powered
echo "0x80" | sudo tee -a $BASE/configs/c.1/bmAttributes
# Power in mA
echo "500" | sudo tee -a $BASE/configs/c.1/MaxPower

# Config
sudo mkdir -p $BASE/configs/c.1/strings/0x409
echo "HID" | sudo tee -a $BASE/configs/c.1/strings/0x409/configuration 

# Symlinks
sudo ln -s $BASE/configs/c.1 $BASE/os_desc
sudo ln -s $HID $BASE/configs/c.1/
ls /sys/class/udc | sudo tee -a $BASE/UDC
