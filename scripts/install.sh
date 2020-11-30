#!/usr/bin/env bash

# Exit on first error.
set -e

# Echo commands to stdout.
set -x

# Treat undefined environment variables as errors.
set -u

echo "This script installs KVM services on a Raspberry Pi"

if [ "$EUID" -ne 0 ];
  then echo "Please run as root"
  exit
fi

echo "Installing /boot/config.txt change: Enables firmware for dwc2 to get loaded"
echo "dtoverlay=dwc2" >> /boot/config.txt

echo "Installing /etc/modules change: Enables dwc2 module on boot"
echo "dwc2" >> /etc/modules

echo "Installing kvm_gadget, kvm_https, kvm_webrtc systemd services"
cp kvm_gadget.service /etc/systemd/system/
cp kvm_https.service /etc/systemd/system/
cp kvm_webrtc.service /etc/systemd/system/

echo "Enabling kvm_gadget, kvm_https, kvm_webrtc systemd services"
systemctl enable kvm_gadget
systemctl enable kvm_https
systemctl enable kvm_webrtc

echo "Syncing filesystem"
sync

echo "Success!  Power cycle the Raspberry Pi and navigate to https://kvm.local/ to begin using the KVM"
