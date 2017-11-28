#!/bin/sh
clear
cd ~/Documents/CMPE220/CMPE-220
rm log.txt
make clean
sudo rmmod myUSBdrive
make
sudo insmod myUSBdrive.ko

