#!/bin/sh
# VMMenu menu generator

XMAMEVERSIONS="37b16 0.106"
ROMDIR=/mnt/mmcblk0p1/roms
VMMENUINIDIR=/opt/pitrex/share/vmmenu
EXTENSIONDIR=/mnt/mmcblk0p1/tcz
VECROMDIR=/mnt/mmcblk0p1/vectrex

# Compare checksum of zip files in roms dir ("sum -s" is quickest).
# Regenerate menu item list if rom files added/removed/renamed
sum="`echo -n $ROMDIR/* $ROMDIR/*/* | sum -s`"
if [ ! -f $ROMDIR/.sum ] || [ "$sum" != "`cat $ROMDIR/.sum`" ]
then
  for xmameversion in $XMAMEVERSIONS
  do
    # xmame_${xmameversion}_menu_crcs_vector.txt must already exist
    mainmenu=$VMMENUINIDIR/xmame_${xmameversion}_menu_crcs_vector.txt
    if [ -f $mainmenu ]
    then
      # Check for version-specific roms directory, otherwise fall back to root
      if [ -d $ROMDIR/$xmameversion ]
      then
        zip2vmmenu.sh $mainmenu $ROMDIR/$xmameversion > $ROMDIR/$xmameversion/.ini
      else
        zip2vmmenu.sh $mainmenu $ROMDIR > $ROMDIR/$xmameversion.ini
      fi
      echo -n "$sum" > $ROMDIR/.sum
    else
      echo "Can't find menu data file: $mainmenu" >&2
    fi
  done
fi

# Again for Vectrex ROMs (symlinked to /opt/pitrex/roms/vectrex with 
# all file names lowercase)
mnulist=
sum="`echo -n /opt/pitrex/roms/vectrex/* | sum -s`"
if [ ! -f $VECROMDIR/.sum ] || [ "$sum" != "`cat $VECROMDIR/.sum`" ]
then
  rm -f /tmp/vecroms.missing
  for rom in /opt/pitrex/roms/vectrex/*
  do
    romname="${rom##*/}"
    if [ -f $VECROMDIR/"$romname.mnu" ]
    then
      mnulist="$mnulist $ext.mnu"
    else
      # If no menu item in vec rom dir, check for a default menu line
      # for this rom
      if [ -f "$VECROMDIR/defaults/$romname.mnu" ]
      then
        mnulist="$mnulist $VECROMDIR/defaults/$romname.mnu"
      else
        # If no default line, then add to list of auto-generated menu entries
        cartlist "$rom" >> /tmp/vecroms.missing
      fi
    fi
  done
  cat $mnulist /tmp/vecroms.missing > $VECROMDIR/.ini 2>/dev/null
  rm -f /tmp/vecroms.*
  echo -n "$sum" > $VECROMDIR/.sum
fi

# Again for PiTrex executable extensions
# list full info for checksum so that updates are also detected
mnulist=
sum="`ls -l --full-time $EXTENSIONDIR | sum -s`"
if [ ! -f $EXTENSIONDIR/.sum ] || [ "$sum" != "`cat $EXTENSIONDIR/.sum`" ]
then
  rm -f /tmp/extensions.missing
  for ext in $EXTENSIONDIR/*.tcz $EXTENSIONDIR/*.TCZ
  do
    [ -f "$ext" ] || continue
    if [ -f "$ext.mnu" ]
    then
      mnulist="$mnulist $ext.mnu"
    else
      # If no menu item in tcz dir, check for a default menu line
      # for this extension
      extname=${ext##*/}
      if [ -f "$EXTENSIONDIR/defaults/$extname.mnu" ]
      then
        mnulist="$mnulist $EXTENSIONDIR/defaults/$extname.mnu"
      else
        echo "Can't find menu item for extension: $extname" >&2
        extname="${extname%%.*}"
        [ -f "$ext" ] && echo "Linux|$extname|$extname|$extname" >> /tmp/extensions.missing
      fi
    fi
  done
  cat $mnulist /tmp/extensions.missing > $EXTENSIONDIR/.ini 2>/dev/null
  echo -n "$sum" > $EXTENSIONDIR/.sum
fi
      
# Squish all the lists together to make the final menu list
xmameversion="`cat /tmp/flags/xmame.version`"
if [ -d $ROMDIR/$xmameversion ]
then
  xmameini=$ROMDIR/$xmameversion/.ini
else
  xmameini=$ROMDIR/$xmameversion.ini
fi
cat $VMMENUINIDIR/builtin.ini \
    $EXTENSIONDIR/.ini \
    $VECROMDIR/.ini \
    $xmameini > $VMMENUINIDIR/vmmenu.ini
