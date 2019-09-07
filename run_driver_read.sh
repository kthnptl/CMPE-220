#!/bin/sh

#nautilus --browser /dev/ &

#check if there is any file with a word demo in it. Which indicates if USB device is insterted or not.
if find /dev/ -name "demo*" -print -quit | grep -q .
then

#if usb device is inserted then try to read from it.
echo "Press any key to read data... "
read key
sudo cat /dev/demo* #>> received.txt

echo "\n-------------------------------------------------------------------------------------" >> log.txt
echo "--------------------------- " $(date) "---------------------------" >> log.txt
echo "-------------------------------------------------------------------------------------\n" >> log.txt

#copy all the log messages with CMPE tag into log file
dmesg | grep -i CMPE >> log.txt

#open that log file
gedit log.txt &

fi

