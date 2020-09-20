#!/bin/sh
# This script configures a fresh install of Raspberry Pi OS for
# glitch-free execution of PiTrex software.
#
# Note that HDMI and Composite video output is disabled on start-up,
# until "sudo tvservice -p" is run manually (PiTrex software will be
# glitchy again).
#
# Kevin Koster.

echo "Adding configuration to config.txt"

#Edit config.txt:

echo \
'# Configure GPIO for PiTrex

#Inputs
gpio=0-5,16-24,26-29=ip
#Outputs
gpio=6-13,25=op
#No pull-up/down on RDY
gpio=24=np


#Use alternative Linux USB driver (no FIQ)
#Modes: "host", "peripheral" or "otg"
dtoverlay=dwc2,dr_mode=host
' >> /boot/config.txt

echo "Creating /opt/pitrex"

mkdir -p /opt/pitrex/bin /opt/pitrex/settings

sed -i '$a '' \ 
\
# set PATH so it includes PiTrex bin if it exists \
if [ \-d "/opt/pitrex/bin" ] ; then \
    PATH="/opt/pitrex/bin:$PATH" \
fi' ~/.profile

#Only parameter is "-configonly" so far.
if [ "$1" ]; then
 exit 0
fi

echo "Editing rc.local"

#Add commands to rc.local:

sed -i '/^exit 0$/i'' \
#Keep CPU frequency at maximum:\
echo \-n performance > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor\
\
#Disable HDMI/Composite video output:\
tvservice \-o' /etc/rc.local

echo "Enabling rc.local"

#Enable execution of rc.local on start-up with Systemd:

chmod a+x /etc/rc.local

echo \
'  [Unit]

   Description=Runs /etc/rc.local
   After=multi-user.target

   [Service]

   Type=simple
   ExecStart=/etc/rc.local

   [Install]

   WantedBy=rc-local.target
' > /etc/systemd/system/rc-local.service

echo \
'  [Unit]
   Description=Run service that runs /etc/rc.local
   Requires=multi-user.target
   After=multi-user.target
   AllowIsolate=yes
' > /etc/systemd/system/rc-local.target

if mkdir /etc/systemd/system/rc-local.target.wants; then
 ln -s /etc/systemd/system/rc-local.service /etc/systemd/system/rc-local.target.wants/rc-local.service
 systemctl daemon-reload
 systemctl set-default rc-local.target
 echo "Reboot now to enable new system configuration"
 exit 0
else
 echo "Failed to set up rc.local with Systemd"
 exit 1
fi
