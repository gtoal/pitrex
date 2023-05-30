#!/bin/sh
# Run with sudo at log-in
# TODO: Manual IP address settings instead of DHCP

ETHIF=
WIFIIF=
if=
oldif=
SELECTION=

clear
echo '
       **    **    **    **    **    **    **   *
      *                                         *
      *
                                                *
      *                                         *
      *
      
                    PiTrex Remote               *
      *                                         *
      *
      
                                                *
      *                                         *
      *  **    **    **    **    **    **    **'

# Automatically start WiFi if "startwifi" command is on the kernel command line
grep -q startwifi /proc/cmdline && SELECTION=w

while true
do

[ -z "$SELECTION" ] && read -p 'Configure (w)ifi, (e)thernet, change audio (v)olume, or (s)hutdown? ' SELECTION
case "$SELECTION" in
    V | \
    v) alsamixer
       SELECTION=
       continue
    ;;

    W | \
    w) # Load wireless extensions
       [ -z "$WIFIIF" ] && sudo -u tc tce-load -i `cat /etc/sysconfig/tcedir/optional/wifi.list`
       # Run WiFi config scipt - auto connect to last network if remembered
       [ -s /etc/sysconfig/tcedir/wifi.db ] && wifi.sh -a || wifi.sh
       # Find name of WiFi interface in use
       WIFIIF=`expr "\`iwconfig 2>/dev/null\`" : '\([a-z]*[0-9]*\)[[:space:]]*IEEE 802.11[[:space:]]*ESSID:"'`
       # Disable previously used interface if successful
       [ "$if" -a "$WIFIIF" ] && ifconfig $if down 2>&1 >/dev/null
       if=$WIFIIF
    ;;

    E | \
    e) 
       read -p 'Interface Number (press Enter for 0)? ' IFNO
       [ -z "$IFNO" ] && IFNO=0
       ETHIF=eth$IFNO
       # Activate, and disable last interface if successful
       if udhcpc -i eth$IFNO -n && [ "$if" -a "$if" != "$ETHIF" ]
       then
        ifconfig $if down 2>&1 >/dev/null
       fi
       if=$ETHIF
    ;;
    S | \
    s) poweroff
       exit
    ;;
    *) SELECTION=
       continue
    ;;
esac
# Check whether an IP address was obtained
ip=`expr "\`ifconfig $if 2>/dev/null\`" : '.*inet addr:\([0-9]*\.[0-9]*\.[0-9]*\.[0-9]*\)'`
[ "$ip" ] && echo -e "Network Interface $if Enabled\nIP Address is: $ip" || if=$oldif
oldif=$if
SELECTION=

done
