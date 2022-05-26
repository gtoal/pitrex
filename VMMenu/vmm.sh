#!/bin/bash
PATH="/opt/pitrex/bin:$PATH"
case "$2" in
 "Linux" )
   echo "Running installed program: $1"
   case "$1" in
       "zblast" )
	   #pushd ../zblast
	   zblast
	   #popd
	   #renice -n 8 `pgrep vmmenu`
          ;;
       "vhyperoid" )
	   #pushd ../xhyperoid
	   vhyperoid
	   #popd
	   #renice -n 8 `pgrep vmmenu`
          ;;
       "gyrocks" )
	   #pushd ../gyrocks
	   gyrocks
	   #popd
	   #renice -n 8 `pgrep vmmenu`
          ;;
       "draft4" )
	   #pushd ../draft4
	   draft4
	   #popd
	   #renice -n 8 `pgrep vmmenu`
          ;;
       "vash" )
	   # vash - removed - crashes linux on exit.
          ;;
       * )
           echo "$1 not yet implemented"
   esac
  ;;

 "Vectrex" )
   echo "Command Line: vecxemul \"/opt/pitrex/roms/vectrex/$1\""
   # vecxemuld also available depending on the game
   vecxemul "/opt/pitrex/roms/vectrex/$1"
  ;;

 "Cinematronics" )
   echo "Command Line: cinemu \"$1\""
   # could pick out ../tailgunner/tailgunner.img instead of default
   cinemu "$1"
  ;;

 "CinEmu" )
   echo "Command Line: cinemu \"$1\""
   # could pick out ../tailgunner/tailgunner.img instead of default
   if [ "$1" == "tailgunner" ] ; then
     tailgunner
   else
     cinemu "$1"
   fi
   
  ;;

 "Atari" )
   echo "Running Atari emulator for: $1"
   case "$1" in
       "asteroids" )
	   asteroids_sbt
          ;;
       "battlezone" )
	   battlezone
          ;;

       "blackwidow" )
	   blackwidow
          ;;

       "deluxe" )
	   deluxe
          ;;

       "gravitar" )
	   gravitar
          ;;

       "lunar" )
	   lunar
          ;;

       "redbaron" )
	   redbaron
          ;;

       "spaceduel" )
	   spaceduel
          ;;

       "tempest" )
	   tempest
          ;;

       * )
           echo "$1 not yet implemented"
	   # sim $1
   esac
   ;;

 * )
   echo "Command Line: $1"
   # advmame $1 # "-video zvg"?
esac
