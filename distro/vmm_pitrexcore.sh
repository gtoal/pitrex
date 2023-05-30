#!/bin/sh
# Usage: vmm.sh [executable] [platform] [extension]

PATH="/opt/pitrex/bin:$PATH"
SD=/mnt/mmcblk0p1
EXTENSIONDIR=$SD/tcz
ROMDIR=$SD/roms
XF86CONFIGDIR=$SD/xf86config
XMAMEVERSION=`cat /tmp/flags/xmame.version`
LOCAL_XMAME_VERSION=37b16
SETTINGSDIR=$SD/tcz/settings
SETTINGS=
SETTINGSCKSUM=
REMOTE_ROMDIR=/opt/pitrex/roms
REMOTE_PASSWORD=vectrex
[ -e /tmp/flags/pc_connected ] && PC_IP=`cat /tmp/flags/pc_connected`
REMOTE_REXEC_CMD="rexec -n -h $PC_IP -u tc -p $REMOTE_PASSWORD"
REMOTE_XMAME_VERSION=0.106
CMD="$1"
EXT="$3"
SAVE_XMAME_CFG=

case "$2" in
 "Vectrex" | "Linux" )
  if [ "$2" = "Vectrex" ]
  then
    if [ -f /tmp/flags/vecx_direct ]
    then
      CMD="sudo vecx /opt/pitrex/roms/vectrex/$1"
      EXT=vecx.direct
    else
      CMD="sudo vecxemul /opt/pitrex/roms/vectrex/$1"
      EXT=vecx.emulated
    fi
  fi
  # Load extension and dependencies if this is the first run,
  # unless the executable is in the base.
  if [ "${EXT%%_*}" != "base" ] && [ ! -f "/usr/local/tcz.installed/$EXT" ]
  then
    DEPS="`cat \"$EXTENSIONDIR/$EXT.tcz.dep\" 2>/dev/null`"
    [ "$DEPS" ] && tce-load -i $DEPS
    tce-load -i "$EXTENSIONDIR/$EXT.tcz"
  fi
  # Check whether this extension needs the X server
  if [ "$DEPS" ] && echo "$DEPS" | grep -q xf86-libX11.tcz
  then
    # Load Xvectrex if not already loaded
    if [ ! -f /usr/local/tcz.installed/xvectrex ]
    then
      DEPS="`cat \"$EXTENSIONDIR/xvectrex.tcz.dep\" 2>/dev/null`"
      [ "$DEPS" ] && tce-load -i $DEPS
      tce-load -i "$EXTENSIONDIR/xvectrex.tcz"
    fi
    # Check for XF86Config files by extension name
    if [ -e /usr/local/X11R6/etc/X11/usercfg/"$EXT".cfg ]
    then
      XF86Config=usercfg/"$EXT".cfg
    else
      if [ -e /usr/local/X11R6/etc/X11/"$EXT"/XF86Config ]
      then
        XF86Config="$EXT"/XF86Config
      else
        XF86Config=XF86Config
      fi
    fi
    # Copy settings to RAM
    SETTINGS=/opt/pitrex/settings/X_generic
    if [ -f $SETTINGSDIR/X_generic ]
    then
      cp -f $SETTINGSDIR/X_generic $SETTINGS
      SETTINGSCKSUM="`sum -s $SETTINGS`"
    fi
    # Xinit will start X server and close it when program terminates
    xinit /opt/pitrex/bin/$CMD -- /opt/pitrex/bin/XFree86 -nolisten tcp -xf86config "$XF86Config"
  else
    # Non-X programs (PiTrex s/w must be SUID root):
    # Copy settings to RAM
    if [ ! -e /opt/pitrex/settings/${CMD%% *} -a -f $SETTINGSDIR/${CMD%% *} ]
    then
      SETTINGS=/opt/pitrex/settings/${CMD%% *}
      cp -f $SETTINGSDIR/${CMD%% *} $SETTINGS
    else
      if [ ! -e /opt/pitrex/settings/default -a -f $SETTINGSDIR/default ]
      then
        SETTINGS=/opt/pitrex/settings/default
        cp -f $SETTINGSDIR/default $SETTINGS
      fi
    fi
    [ "$SETTINGS" ] && SETTINGSCKSUM="`sum -s $SETTINGS`"
    echo "Running: $CMD"
    # Using exec to prevent unexplained long pause before VMMenu resumes after program exits sometimes.
    # However this means that settings aren't saved unless closed with "shut down".
    exec $CMD
  fi
 ;;

 "Cinematronics" | \
 "Atari" | \
 "Sega" | \
 "Other" | \
 "Vectorbeam" | \
 "Midway" | \
 "Centuri" | \
 "Cinematronics / GCE" )
   # Check for XF86Config files by game name
   if [ -e "$ROMDIR/$XMAMEVERSION/$CMD".cfg ]
   then
    XF86Config="vexmame/$XMAMEVERSION/$CMD".cfg
   else
     if [ -e "$ROMDIR/$CMD".cfg ]
     then
       XF86Config="vexmame/$CMD".cfg
     else
       if [ -e /usr/local/X11R6/etc/X11/"vexmame_${XMAMEVERSION}_defaults/$CMD".XF86Config ]
       then
         # Symlink to common default settings (eg. XF86Config_analogue)
         XF86Config="vexmame_${XMAMEVERSION}_defaults/$CMD".XF86Config
       else
         # Overall default
         XF86Config=vexmame_${XMAMEVERSION}_defaults/XF86Config
       fi
     fi
   fi
   # Load Xvectrex if not already loaded
   if [ ! -f /usr/local/tcz.installed/xvectrex ]
   then
     DEPS="`cat \"$EXTENSIONDIR/xvectrex.tcz.dep\" 2>/dev/null`"
     [ "$DEPS" ] && tce-load -i $DEPS
     tce-load -i "$EXTENSIONDIR/xvectrex.tcz"
   fi
   # Copy settings to RAM
   SETTINGS=/opt/pitrex/settings/X_generic
   if [ -f $SETTINGSDIR/X_generic ]
   then
     cp -f $SETTINGSDIR/X_generic $SETTINGS
     SETTINGSCKSUM="`sum -s $SETTINGS`"
   fi
   # Check whether running XMAME locally or remotely
   if [ "$XMAMEVERSION" = "$LOCAL_XMAME_VERSION" ]
   then
    # Set XMAME-version-specific ROM path
    [ -f "$ROMDIR/$XMAMEVERSION/$EXT.zip" ] && ROMDIR="$ROMDIR/$XMAMEVERSION"
    # Load VeXMAME extensions:
    if [ ! -f /usr/local/tcz.installed/vexmame ]
    then
     DEPS="`cat \"$EXTENSIONDIR/vexmame.tcz.dep\" 2>/dev/null`"
     [ "$DEPS" ] && tce-load -i $DEPS xf86-xinit
     tce-load -i "$EXTENSIONDIR/vexmame.tcz"
    fi
     # Audio set-up
     AUDIOARGS=
     if [ -e /tmp/flags/btaudio ]
     then
       [ -e /tmp/flags/rbuf ] && RBUF="`cat /tmp/flags/rbuf`" || RBUF=150
       [ -e /tmp/flags/pbuf ] && PBUF="`cat /tmp/flags/pbuf`" || PBUF=50
       if [ ! -e /usr/local/tce.installed/sox ]
       then
         tce-load -i sox
         sudo modprobe snd-aloop index=2 pcm_substreams=1
       fi
       AUDIODEV=hw:2,1,0 rec -q --buffer $RBUF -r 8000 -c 1 -p | AUDIODEV=bluealsa play --buffer $PBUF -r 8000 -c 1 -p -q &
       AUDIOARGS="-audiodevice hw:2,0,0 -samplefreq 8000 -nostereo"
     fi
     xinit /opt/pitrex/bin/xmame.x11.$LOCAL_XMAME_VERSION $CMD $AUDIOARGS -cyclone -drz80 -rp "$ROMDIR" -skip_disclaimer -skip_warnings \
     -- /opt/pitrex/bin/XFree86 -nolisten tcp -xf86config "$XF86Config"
     [ -e /tmp/flags/btaudio ] && killall rec play
     SAVE_XMAME_CFG="$LOCAL_XMAME_VERSION"
   else
     # -- Remote XVectrex --
     if [ ! -f /usr/local/tcz.installed/vexmame_pc_cfg ]
     then
       tce-load -i xf86-xinit
       tce-load -i "$EXTENSIONDIR/vexmame_pc_cfg.tcz"
     fi
     # Set XMAME-version-specific ROM path
     [ -f "$ROMDIR/$XMAMEVERSION/$EXT.zip" ] && REMOTE_ROMDIR="$REMOTE_ROMDIR/$XMAMEVERSION"
     # Connect with PC and sync ROMs if not done already
     if [ ! -e /tmp/flags/pc_connected ]
     then
       xmame_remote.sh || exit 1
     fi
     # Start X server and run XMAME remotely on PC using rexec
     xinit /usr/bin/xmame_rexec.sh "$REMOTE_REXEC_CMD" "$CMD" "$REMOTE_ROMDIR" -- /opt/pitrex/bin/XFree86 -xf86config "$XF86Config"
     SAVE_XMAME_CFG="$REMOTE_XMAME_VERSION"
   fi
 ;;
esac

# Save any configuration changes to SD card then disable write access again:
# Note: XMAME high score saving requires hiscore.dat
if [ "$SAVE_XMAME_CFG" ] || [ -f "$SETTINGS" -a "$SETTINGSCKSUM" != "`sum -s $SETTINGS`" ]
then
  echo "Saving settings"
  # Enable write access to SD card
  sudo mount -o remount -o rw $SD
  # Copy settings
  cp -f "$SETTINGS" $SETTINGSDIR/
  if [ "$SAVE_XMAME_CFG" ]
  then
    if [ "$SAVE_XMAME_CFG" = "$LOCAL_XMAME_VERSION" ]
    then
      cp -rf /home/tc/.xmame* /mnt/mmcblk0p1/roms/"$SAVE_XMAME_CFG"/
    else
      rsync -rIq rsync://$PC_IP:/roms/"$SAVE_XMAME_CFG"/.xmame "$ROMDIR/$SAVE_XMAME_CFG"
    fi
  fi
  # Disable write access to SD card
  sudo mount -o remount -o ro $SD
fi
