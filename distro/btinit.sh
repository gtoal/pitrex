#!/bin/ash
# Init Bluetooth.
# Run as root.

if [ ! -f /tmp/flags/bluetooth ]
then
  sudo -u tc tce-load -i alsa bluez-alsa
  modprobe hci_uart
  /usr/local/etc/init.d/bluez start
  bluealsa -S &

  # firmware upload command sometimes times out on the first attempt
  # Maximum three tries
  wait=`expr 25 + \`date +%s\``
  until hciattach -t 10 /dev/ttyAMA0 bcm43xx 921600 noflow
  do
    if [ `date +%s` -gt $wait ]
    then
      echo hciattach timeout
      exit 1
    else
      echo retrying hciattach
    fi
  done
  touch /tmp/flags/bluetooth
  sleep 2
else
  echo "Bluetooth already initialised"
fi

bluetoothctl power on
bluetoothctl scan on &
