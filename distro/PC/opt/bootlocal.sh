#!/bin/sh

# Make roms directory
mkdir -m a+rwx -p /opt/pitrex/roms

# Start mDNS daemon
/usr/local/etc/init.d/avahi start

# Start rsync and rexec via inetd daemon
inetd /opt/inetd.conf

# Remember WiFi networks
ln -s /etc/sysconfig/tcedir/wifi.db /home/tc/wifi.db

# Set up default audio device with ALSA
alsactl -s init
