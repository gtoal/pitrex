#!/bin/sh
# "-b" for background.

if [ ! -f /tmp/flags/wifi ] && [ -f /mnt/mmcblk0p1/wpa_supplicant.conf ]
then
  touch /tmp/flags/wifi
  tce-load -i firmware-rpi-wifi wireless-KERNEL wireless_tools wpa_supplicant haveged
  # Speed up enthropy collection for /dev/random (otherwise takes 1min after Pi Zero boots up)
  sudo haveged
  sudo cp /mnt/mmcblk0p1/wpa_supplicant.conf /etc/
  wait=`expr 10 + \`date +%s\``
  until iwconfig | grep -q wlan0
  do
    if [ `date +%s` -ge $wait ]
    then
      echo wireless interface not detected
      exit 1
    else
      sleep 1
    fi
  done
  sudo wpa_supplicant -Dnl80211 -iwlan0 -c/etc/wpa_supplicant.conf -B
  if [ "$1" = "-b" ]
  then
    expr "`sudo udhcpc -i wlan0 2>&1`" : '.* \([0-9]*\.[0-9]*\.[0-9]*\.[0-9]*\) obtained' > /tmp/flags/wifi &
  else
    # Time out if not connected after 20s if not running DHCP client in the background
    {
      sleep 20
      if [ ! -s /tmp/flags/wifi ]
      then
        sudo killall udhcpc
        rm /tmp/flags/wifi
      fi
    } &
    expr "`sudo udhcpc -i wlan0 2>&1`" : '.* \([0-9]*\.[0-9]*\.[0-9]*\.[0-9]*\) obtained' > /tmp/flags/wifi
  fi
fi
