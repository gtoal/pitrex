#!/bin/bash
# This script configures a fresh install of Raspberry Pi OS for
# glitch-free execution of PiTrex software.
#
# Note that HDMI and Composite video output is disabled on start-up,
# until "sudo tvservice -p" is run manually (PiTrex software will be
# glitchy again).
#
# This script must be run under sudo.
#
# Kevin Koster.
expand_tilde()
{
    case "$1" in
    (\~)        echo "$HOME";;
    (\~/*)      echo "$HOME/${1#\~/}";;
    (\~[^/]*/*) local user=$(eval echo ${1%%/*})
                echo "$user/${1#*/}";;
    (\~[^/]*)   eval echo ${1};;
    (*)         echo "$1";;
    esac
}

confirm() {
  local _prompt _default _response
 
  if [ "$1" ]; then _prompt="$1"; else _prompt="Are you sure"; fi
  _prompt="$_prompt [y/n] ?"
 
  # Loop forever until the user enters a valid response (Y/N or Yes/No).
  while true; do
    read -r -p "$_prompt " _response
    case "$_response" in
      [Yy][Ee][Ss]|[Yy]) # Yes or Y (case-insensitive).
          echo "y"
	  return
        ;;
      [Nn][Oo]|[Nn])  # No or N.
          echo "n"
	  return
        ;;
      *) # Anything else (including a blank) is invalid.
        ;;
    esac
  done
}

if [ "`whoami`" != "root" ] ; then
    echo This script must be run using sudo.
    exit 1
fi

if [ "$SUDO_USER" == "" ] ; then
    echo This script must be run using sudo, not as root itself.
    exit 1
fi

echo "Adding configuration to config.txt"

#Edit config.txt:
L1="`grep ^gpio=0-5,16-24,26-29=ip$ /boot/config.txt`"
L2="`grep ^gpio=6-13,25=op$ /boot/config.txt`"
L3="`grep ^gpio=24=np$ /boot/config.txt`"
L4="`grep ^dtoverlay=dwc2,dr_mode=host$ /boot/config.txt`"
L5="`grep ^dtoverlay= /boot/config.txt`"
if [ "$L1" == "" ] || [ "$L2" == "" ] || [ "$L3" == "" ] ; then
    echo "[all]" >> /boot/config.txt   # just in case ...
    echo "# Configure GPIO for PiTrex" >> /boot/config.txt
fi
if [ "$L1" == "" ] ; then
    echo "" >> /boot/config.txt
    echo "#Inputs" >> /boot/config.txt
    echo "gpio=0-5,16-24,26-29=ip" >> /boot/config.txt
    echo "added: gpio=0-5,16-24,26-29=ip"
fi
if [ "$L2" == "" ] ; then
    echo "" >> /boot/config.txt
    echo "#Outputs"  >> /boot/config.txt
    echo "gpio=6-13,25=op" >> /boot/config.txt
    echo "added: gpio=6-13,25=op"
fi
if [ "$L3" == "" ] ; then
    echo "" >> /boot/config.txt
    echo "#No pull-up/down on RDY" >> /boot/config.txt
    echo "gpio=24=np" >> /boot/config.txt
    echo "added: gpio=24=np"
fi

if [ "$L4" == "" ] ; then
    if [ "$L5" != "" ] ; then
	echo "WARNING: check /boot/config.txt for a potential clash between existing:"
	echo -n "    "; grep ^dtoverlay= /boot/config.txt
	echo "       and the addition:"
	echo "    dtoverlay=dwc2,dr_mode=host"
    fi
    echo "" >> /boot/config.txt
    echo "#Use alternative Linux USB driver (no FIQ)" >> /boot/config.txt
    echo "#Modes: \"host\", \"peripheral\" or \"otg\"" >> /boot/config.txt
    echo "dtoverlay=dwc2,dr_mode=host" >> /boot/config.txt
    echo "added: dtoverlay=dwc2,dr_mode=host"
fi

echo "Creating /opt/pitrex folders"

mkdir -p /opt/pitrex/bin /opt/pitrex/settings /opt/pitrex/roms /opt/pitrex/ini
chown $SUDO_USER /opt/pitrex/bin /opt/pitrex/settings /opt/pitrex/roms /opt/pitrex/ini

# NOTE: by default this just goes into ROOT's .profile, we need a little
# extra effort to put it in the calling user's .profile as well...
# Might be easier to just add it globally in /etc/rc.local?
P1="`fgrep \"PATH=\\\"/opt/pitrex/bin:\" ~root/.profile`"
if [ "$P1" == "" ] ; then
    echo "adding /opt/pitrex/bin to path in ~root/.profile"
cat <<EOF >> ~root/.profile

# set PATH so it includes PiTrex bin if it exists
if [ -d "/opt/pitrex/bin" ] ; then
    PATH="/opt/pitrex/bin:\$PATH"
fi
EOF
else
    echo "/opt/pitrex/bin already in path in ~root/.profile"
fi

PRO=$(expand_tilde ~$SUDO_USER/.profile)
P2="`fgrep \"PATH=\\\"/opt/pitrex/bin:\" $PRO`"
if [ "$P2" == "" ] ; then
    echo "adding /opt/pitrex/bin to path in $PRO"
cat <<EOF >> $PRO

# set PATH so it includes PiTrex bin if it exists
if [ -d "/opt/pitrex/bin" ] ; then
    PATH="/opt/pitrex/bin:\$PATH"
fi
EOF
else
    echo "/opt/pitrex/bin already in path in $PRO"
fi

#Only parameter is "-configonly" so far.
if [ "$1" ]; then
 exit 0
fi

echo "Editing rc.local"

#Add commands to rc.local:

R1="`fgrep \"#Keep CPU frequency at maximum:\" /etc/rc.local`"
if [ "$R1" != "" ] ; then
    echo "WARNING: rc.local configuration appears to have been done already."
fi

echo "Do you want to disable HDMI at boot time?"
YN=$(confirm "Yes or No?")

if [ "$YN" == "y" ] ; then
sed -i '/^exit 0$/i'' \
#Disable HDMI/Composite video output:\
tvservice \-o' /etc/rc.local
echo "adding: tvservice -o to /etc/rc.local"
fi

if [ "$YN" == "n" ] ; then
sed -i '/^exit 0$/i'' \
#Disable HDMI/Composite video output:\
# tvservice \-o' /etc/rc.local
fi

echo "Do you want to set performance to maximum at boot time?"
YN=$(confirm "Yes or No?")

if [ "$YN" == "y" ] ; then
sed -i '/^exit 0$/i'' \
#Keep CPU frequency at maximum:\
echo \-n performance > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor' /etc/rc.local
echo "adding: performance mode to /etc/rc.local"
fi

if [ "$YN" == "n" ] ; then
sed -i '/^exit 0$/i'' \
#Keep CPU frequency at maximum:\
# echo \-n performance > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor' /etc/rc.local
fi

echo "Enabling rc.local"

#Enable execution of rc.local on start-up with Systemd:

chmod a+x /etc/rc.local

# update if it exists already
echo "Updating /etc/systemd/system/rc-local.service"
cat > /etc/systemd/system/rc-local.service <<EOF
[Unit]

Description=Runs /etc/rc.local
After=multi-user.target

[Service]

Type=simple
ExecStart=/etc/rc.local

[Install]

WantedBy=rc-local.target
EOF

# update if it exists already
echo "Updating /etc/systemd/system/rc-local.target"
cat > /etc/systemd/system/rc-local.target <<EOF
[Unit]
Description=Run service that runs /etc/rc.local
Requires=multi-user.target
After=multi-user.target
AllowIsolate=yes
EOF

if [ -d /etc/systemd/system/rc-local.target.wants ] ; then
    echo "WARNING: /etc/systemd/system/rc-local.target.wants already exists"
else
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
fi

exit 0
