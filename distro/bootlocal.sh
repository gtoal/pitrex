#!/bin/sh

RUN="vmmenu|Linux|VMMenu"
# Read kernel command line options
for cmd in `cat /proc/cmdline`
do
  case $cmd in
   *=*)
         case $cmd in
            run*) RUN=${cmd#*=} ;;
         esac
   ;;
   *)
         case $cmd in
            startwifi) WIFI=1 ;;
            startbluetooth) BLUETOOTH=1 ;;
            debug) DEBUG=1 ;;
         esac
   ;;
  esac
done

if [ "$DEBUG" ]
then
  #Start debugging services
  sudo -u tc tce-load -i inetutils-servers
  inetd /opt/inetd.conf
else
  tvservice -o
fi

# Link Vectrex ROMs into single directory (note: duplicate names overwritten)
find /mnt/mmcblk0p1/vectrex -type f \! -ipath '*/bios*' \! -name '*.mnu' \! -iname '*.nib' \! -iname '*.sav' -exec ln -sf '{}' /opt/pitrex/roms/vectrex/ +

# Link to extracted ROMs for non-XMAME emulators
find /mnt/mmcblk0p1/roms -maxdepth 1 -type d \! -name roms -exec ln -sf '{}' /opt/pitrex/roms/ +

# Extract ZIP files for non-XMAME emulators
for romzip in /mnt/mmcblk0p1/roms/*.zip /mnt/mmcblk0p1/roms/*.ZIP
do
  [ -f "$romzip" ] || continue
  romzip="${romzip%.zip}"
  [ -d "/opt/pitrex/roms/${romzip##*/}" ] && continue
  unzip -n -d "/opt/pitrex/roms/${romzip##*/}" "$romzip".zip
done

# Convert all Vectrex ROM filenames to lowercase to match default menu entries
for upper in /opt/pitrex/roms/vectrex/*[A-Z]*
do
  mv "$upper" "`echo \"$upper\" | /usr/bin/tr '[:upper:]' '[:lower:]'`"
done

# Create flags directory
mkdir -m a+rwx /tmp/flags
[ -d /mnt/mmcblk0p1/flags ] && cp -r /mnt/mmcblk0p1/flags /tmp/

# Copy permanent list of allowed hosts, if present
[ -f /mnt/mmcblk0p1/X0.hosts ] && cp -f /mnt/mmcblk0p1/X0.hosts /etc/X0.hosts

# Copy VMMenu config, if present
[ -f /mnt/mmcblk0p1/vmmenu.cfg ] && cp /mnt/mmcblk0p1/vmmenu.cfg /opt/pitrex/share/vmmenu/

# Set default XMAME version:
echo -n 37b16 > /tmp/flags/xmame.version
# Copy XMAME configuration and game states:
cp -rf /mnt/mmcblk0p1/roms/37b16/.xmame* /home/tc/
chown -R tc:staff /home/tc/.xmame*

# Start WiFi
[ "$WIFI" ] && /usr/bin/wifiinit.sh -b

# Start Bluetooth
if [ "$BLUETOOTH" ]
then
  /usr/bin/btinit.sh
  /usr/bin/btpair.sh -a
fi

chmod a+rw /tmp/flags/*

# To start serial terminal /w console
# Add the following to the cmdline.txt console=serial0,115200
# Reference https://www.raspberrypi.org/documentation/configuration/uart.md for UART configuration
# Uncomment the next line
#  This isn't working anyway
#/usr/sbin/startserialtty &

# Start openssh daemon
#/usr/local/etc/init.d/openssh start

# Start menu first if set on kernel command line
if [ "$RUN" = "vmmenu|Linux|VMMenu" ]
then
  sudo -u tc tce-load -i `cat /mnt/mmcblk0p1/tcz/VMMenu.tcz.dep`
  sudo -u tc tce-load -i /mnt/mmcblk0p1/tcz/VMMenu.tcz
  /usr/bin/mkvmmenu.sh
  SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy /opt/pitrex/bin/vmmenu &
fi

# Start any other program specified on the command line with
# the entry: run=[executable]|[platform]|[extension]
if [ "$RUN" ] && [ ! "$RUN" = "vmmenu|Linux|VMMenu" ]
then
  sudo -u tc /usr/bin/vmm_pitrexcore.sh `echo "$RUN" | tr '|' ' '`
fi
