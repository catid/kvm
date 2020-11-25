#!/bin/sh
sudo modprobe dwc2
sudo modprobe libcomposite
sudo modprobe g_hid

BASE=/sys/kernel/config/usb_gadget/kvm

# Create HID widget
sudo mkdir -p $BASE
echo "0x1d6b" | sudo tee -a $BASE/idVendor
echo "0x0104" | sudo tee -a $BASE/idProduct
echo "0x0100" | sudo tee -a $BASE/bcdDevice
echo "0x0200" | sudo tee -a $BASE/bcdUSB

# Strings
mkdir -p $BASE/strings/0x409
echo "fedcba9876543210" | sudo tee -a $BASE/strings/0x409/serialnumber
echo "catid" | sudo tee -a $BASE/strings/0x409/manufacturer
echo "SkyJack" | sudo tee -a $BASE/strings/0x409/product

HID=$BASE/functions/hid.usb0

# HID
sudo mkdir -p $HID
echo 1 | sudo tee -a $HID/protocol
echo 1 | sudo tee -a $HID/subclass
echo 8 | sudo tee -a $HID/report_length
echo "\\x05\\x01\\x09\\x06\\xa1\\x01\\x05\\x07\\x19\\xe0\\x29\\xe7\\x15\\x00\\x25\\x01\\x75\\x01\\x95\\x08\\x81\\x02\\x95\\x01\\x75\\x08\\x81\\x03\\x95\\x05\\x75\\x01\\x05\\x08\\x19\\x01\\x29\\x05\\x91\\x02\\x95\\x01\\x75\\x03\\x91\\x03\\x95\\x06\\x75\\x08\\x15\\x00\\x25\\x65\\x05\\x07\\x19\\x00\\x29\\x65\\x81\\x00\\xc0" | sudo tee -a $HID/report_desc

# Config
sudo mkdir -p $BASE/configs/c.1/strings/0x409
sudo ln -s $HID $BASE/configs/c.1/

echo "Config 1: HID" | sudo tee -a $BASE/configs/c.1/strings/0x409/configuration 
echo 250 | sudo tee -a $BASE/configs/c.1/MaxPower 

# UDC
ls /sys/class/udc | sudo tee -a $BASE/UDC
