#!/bin/sh

#nautilus --browser /dev/ &

#check if there is any file with a word demo in it. Which indicates if USB device is insterted or not.
if find /dev/ -name "demo*" -print -quit | grep -q .
then

#if usb device is inserted then try to read from it.
#sudo cat /dev/demo* #>> received.txt

#try to write a character in it
#sudo echo 'a' >> /dev/demo*

#try to copy a text file in that file
sudo cp ~/Downloads/resumes\ and\ CL/resume.pdf /dev/demo*

echo "\n-------------------------------------------------------------------------------------" >> log.txt
echo "--------------------------- " $(date) "---------------------------" >> log.txt
echo "-------------------------------------------------------------------------------------\n" >> log.txt

#copy all the log messages with CMPE tag into log file
dmesg | grep -i CMPE >> log.txt

#open that log file
gedit log.txt &

fi

