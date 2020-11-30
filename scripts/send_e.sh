#!/bin/sh
echo -ne "\0\0\x8\0\0\0\0\0" | sudo tee -a /dev/hidg0
echo -ne "\0\0\0\0\0\0\0\0" | sudo tee -a /dev/hidg0

