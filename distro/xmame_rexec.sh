#!/bin/sh
$1 "DISPLAY=`cat /tmp/flags/wifi`:0 /opt/pitrex/bin/xmame.x11 $2 -rp $3 -skip_gameinfo -skip_warnings -skip_disclaimer -hwvec 2"
