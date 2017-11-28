#!/bin/sh

#clear the terminal
clear

#navigate to the desired folder which has driver code
cd ~/Documents/CMPE220/CMPE-220

#delete the existing file
rm log.txt

#cleaing the make to start fresh
make clean

#Uninstall the existing driver if it is installed
sudo rmmod myUSBdrive

#make the driver and supporting files
make

#install the driver
sudo insmod myUSBdrive.ko

