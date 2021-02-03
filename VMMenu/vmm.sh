#!/bin/bash

echo $1
echo $2

case "$2" in
 "Linux" )
   echo "Running installed program: $1"
   $1
  ;;

 "Vectrex" )
   echo "Vectrex Game Selected"
   echo "Command Line: vecx \"$1\""
   # /opt/pitrex/bin/advmess vectrex -cart "$1"
   vecx "$1"
  ;;

 "CinEmu" )
   echo "Running Cinematronics emulator for: $1"
   cinemu "$1"
  ;;

 * )
    echo "Command Line: /opt/pitrex/bin/advmame $1"
            /opt/pitrex/bin/advmame $1 # "-video zvg"?
esac
