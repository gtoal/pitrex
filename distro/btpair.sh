#!/bin/ash
# Connect to devices in /mnt/mmcblk0p1/btdev_ files.
# Run as root.

AUDIO=0
CONT=0
TIMEOUT=45 # seconds

until [ -z "$1" ]
do
  case "$1" in
  -a) AUDIO=1;;
  -c) CONT=1;;
  *) echo "Unknown option: $1"; exit 1;;
  esac
  shift
done

# Connect to bluetooth device with mac or ID listed in file
if [ $AUDIO -gt 0 ] && [ -f "/mnt/mmcblk0p1/btdev_audio.txt" ]
then
        #Copy custom ALSA configuration, if present
        if [ -f /mnt/mmcblk0p1/asound_bt.conf ]
        then
          cp -f /mnt/mmcblk0p1/asound_bt.conf /etc/asound.conf
        else
          cp -f /etc/asound_bt.conf /etc/asound.conf
        fi
	echo Searching for bluetooth speakers...
        wait=`expr $TIMEOUT + \`date +%s\``
	dev=
	while [ -z "$dev" ]
	do
          if [ `date +%s` -ge $wait ]
          then
            echo pairing timeout
            exit 1
          else
	    sleep 1
	    dev="`bluetoothctl devices | grep -f /mnt/mmcblk0p1/btdev_audio.txt`"
          fi
	done
	mac=`expr "$dev" : 'Device \(..:..:..:..:..:..\) '`
	bluetoothctl pair $mac
	bluetoothctl connect $mac
	sed -i "s/defaults.bluealsa.device \".*\"/defaults.bluealsa.device \"$mac\"/" /etc/asound.conf
	touch /tmp/flags/btaudio
fi

if [ $CONT -gt 0 ] && [ -f "/mnt/mmcblk0p1/btdev_controller.txt" ]
then
	modprobe joydev
	echo Searching for bluetooth controllers...
        wait=`expr $TIMEOUT + \`date +%s\``
	dev=
	while [ -z "$dev" ]
	do
          if [ `date +%s` -ge $wait ]
          then
            echo pairing timeout
            exit 1
          else
	    sleep 1
	    dev="`bluetoothctl devices | grep -f /mnt/mmcblk0p1/btdev_controller.txt`"
          fi
	done
	mac=`expr "$dev" : 'Device \(..:..:..:..:..:..\) '`
	bluetoothctl pair $mac
	bluetoothctl connect $mac
fi
