#!/bin/sh

# Exit on first error.
set -e

# Echo commands to stdout.
set -x

# Treat undefined environment variables as errors.
set -u

openssl req -newkey rsa:2048 -x509 -sha256 -nodes -out /home/pi/kvm/scripts/cert.pem -keyout /home/pi/kvm/scripts/key.pem -days 365 -subj \'/C=US/ST=California/L=Irvine/O=Catid/OU=Catid/CN=kvm\'

python3 start_https.py
