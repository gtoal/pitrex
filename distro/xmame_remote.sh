#!/bin/sh
# Set up and switch to/from remote XMAME on PC via WiFi and XFree86

REMOTE_XMAME_VERSION=0.106
REMOTE_ROMDIR=/opt/pitrex/roms
SD=/mnt/mmcblk0p1
ROMDIR=$SD/roms
LOCAL_XMAME_VERSION=37b16

# If already set for remote XMAME, then switch back to local
# XMAME mode
if [ -e /tmp/flags/pc_connected ]
then
  if [ "`cat /tmp/flags/xmame.version`" == "$REMOTE_XMAME_VERSION" ]
  then
    echo -n "$LOCAL_XMAME_VERSION" > /tmp/flags/xmame.version
    sudo mkvmmenu.sh
    exit 0
  else
    echo -n "$REMOTE_XMAME_VERSION" > /tmp/flags/xmame.version
    sudo mkvmmenu.sh
    exit 0
  fi
else
  echo -n "$REMOTE_XMAME_VERSION" > /tmp/flags/xmame.version
  sudo mkvmmenu.sh
fi

tce-load -i avahi inetutils rsync &

# Start WiFi and wait for connection
if [ -f /tmp/flags/wifi ]
then
  while [ ! -s /tmp/flags/wifi ]; do sleep 1; done
else
  sudo wifiinit.sh
fi
PI_IP=`cat /tmp/flags/wifi`

# Load required extensions
# TODO: Include these in base? (avahi, rexec, rsync)
max_wait=15
until [ -e /usr/local/bin/rsync ]
do
  sleep 1
  max_wait=`expr $max_wait - 1`
  if [ $max_wait -eq 0 ]
  then
    echo "Time-out waiting for tce-load"
    exit 1
  fi
done

# Discover IP address of remote system
if [ -s $SD/remote_ip.txt ]
then
  # Read manually set remote IP
  PC_IP="`cat $SD/remote_ip.txt`"
else
  # Start mdns daemon
  sudo /usr/local/etc/init.d/avahi start

  # Find PC's IP address
  PC_IP="`avahi-resolve-host-name pitrex-remote.local`"
  PC_IP=${PC_IP#pitrex-remote.local	}
fi

if [ "$PC_IP" ]
then
  # Allow connections from PC to Pi's X server
  echo $PC_IP >> /etc/X0.hosts
  # Sync ROMs from Pi to PC
  rsync -rIq --exclude="$LOCAL_XMAME_VERSION/*" "$ROMDIR"/  rsync://$PC_IP:/roms
  echo $PC_IP > /tmp/flags/pc_connected
else
  exit 1
fi
