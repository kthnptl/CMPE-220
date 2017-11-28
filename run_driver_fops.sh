#!/bin/sh
#nautilus --browser /dev/ &

if find /dev/ -name "demo*" -print -quit | grep -q .
then

sudo cat /dev/demo*
#sudo echo 'a' >> /dev/demo*
sudo cp myUSBdrive.c /dev/demo*
echo "\n-------------------------------------------------------------------------------------" >> log.txt
echo "--------------------------- " $(date) $(-rfc-2822) "---------------------------" >> log.txt
echo "-------------------------------------------------------------------------------------\n" >> log.txt
dmesg | grep -i CMPE >> log.txt
gedit log.txt &

fi

