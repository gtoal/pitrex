#!/bin/ash
#Initialise USB audio device with ALSA

tce-load -i alsa
rm -f /tmp/flags/btaudio
#Copy custom ALSA configuration, if present
if [ -f /mnt/mmcblk0p1/asound_usb.conf ]
then
  sudo cp -f /mnt/mmcblk0p1/asound_usb.conf /etc/asound.conf
else
  sudo rm -f /etc/asound.conf
fi
