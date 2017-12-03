#!/bin/sh

#nautilus --browser /dev/ &

#check if there is any file with a word demo in it. Which indicates if USB device is insterted or not.
if find /dev/ -name "demo*" -print -quit | grep -q .
then

#try to copy a text file in that file
echo "Press any key to send data... "
read key
sudo cp /home/shivam/Documents/linux/MAINTAINERS /dev/demo*
sudo cp /home/shivam/Documents/linux/MAINTAINERS /dev/demo*
sudo cp /home/shivam/Documents/linux/MAINTAINERS /dev/demo*

echo "\n-------------------------------------------------------------------------------------" >> log.txt
echo "--------------------------- " $(date) "---------------------------" >> log.txt
echo "-------------------------------------------------------------------------------------\n" >> log.txt

#copy all the log messages with CMPE tag into log file
dmesg | grep -i CMPE >> log.txt

#open that log file
gedit log.txt &

fi

