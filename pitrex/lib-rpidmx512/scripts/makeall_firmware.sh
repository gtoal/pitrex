#!/bin/bash

DIR=../rpi_*

for f in $DIR
do
	echo -e "\e[32m[$f]\e[0m"
	if [ -d $f ]; then
		if [[ $f != *"circle"* ]]; then
			cd "$f"
			
			if [ -f Makefile ]; then
				make $1 $2 || exit
			fi
			
			cd -
		fi
	fi
done
