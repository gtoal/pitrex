#!/bin/sh
## Build PiTrexCore extensions.
## Run in PiTrex source root directory after compiling all software.
## Extensions are placed in "distro/tcz".
## Any loaded PiTrex extensions will have their symlinks in /opt/pitrex deleted.

tce-load -i compiletc squashfs-tools binutils findutils coreutils
mkdir -p distro/tcz/build
# This will wipe any old extensions already in the destination directory.
sudo rm -rf distro/tcz/build/* distro/tcz/*.*

ext=
ext_name=
build_dir=
file=
dir=
target=

## Generate extensions for programs with existing [extension].tcz.list files
for ext in `find -name '*.tcz.list' ! -path './distro/tcz/*'`
do
  ext_name="${ext##*/}"
  ext_name="${ext_name%.list}"
  build_dir="${ext%/*}"
  mkdir "distro/tcz/build/$ext_name"
  xprog=
  if [ -f "$build_dir/$ext_name.dep" ]
  then
    grep -q xf86-libX11.tcz "$build_dir/$ext_name.dep" && xprog=1
    cp -P "$build_dir/$ext_name.dep" distro/tcz/
  fi
  # Loop through all files listed in [extension].tcz.list and add to build directory
  for file in `cat "$ext"`
  do
    dir="${file#/}"
    dir="${file%/*}"
    mkdir -p "distro/tcz/build/$ext_name/$dir"
    # Files to be added from below top level of program directory are symlinked.
    # Symlink to a symlink if you want to include a symlink in the extension.
    if [ -L "$build_dir/${file##*/}" ]
    then
      target=`ls -l "$build_dir/${file##*/}"`
      cp -Pp "$build_dir/${target##* -> }" "distro/tcz/build/$ext_name/${file#/}"
    else
      cp -Pp "$build_dir/${file##*/}" "distro/tcz/build/$ext_name/${file#/}"
    fi
    # Strip any binaries (note that this doesn't detect kernel modules)
    strip "distro/tcz/build/$ext_name/${file#/}" 2>/dev/null
    # SUID any PiTrex executables so they run with root perms to access GPIO
    if [ "${file%/*}" = "/opt/pitrex/bin" -a -z "$xprog" ]
    then
      chmod u+s "distro/tcz/build/$ext_name/${file#/}"
    fi
  done
  # Make the extension, with all files owned by root
  mksquashfs "distro/tcz/build/$ext_name" "distro/tcz/$ext_name" -all-root
  # Generate and copy other extension files (only .dep is strictly required,
  # if present)
  md5sum "distro/tcz/$ext_name" > "distro/tcz/$ext_name".md5.txt
  cp "$ext" distro/tcz/
done

## Make extensions for any program that has a Makefile.raspbian makefile with an install
## rule.

# Wipe out the /opt/pitrex directory
sudo rm -r /opt/pitrex

failed=
# Loop through every Makefile.raspbian with an install rule that's one level deep
for ext in `grep -l install: */Makefile.raspbian`
do
  ext_name="${ext%/*}"
  # Create basic /opt/pitrex tree
  sudo mkdir -p /opt/pitrex/bin /opt/pitrex/man /opt/pitrex/share
  # Install the program and check it completed OK
  if ! sudo make -C "$ext_name" -f Makefile.raspbian install
  then
   failed="$ext_name, $failed"
   sudo rm -r /opt/pitrex
   continue
  fi
  # Move all the newly installed files into the distro/tcz/build/[extension] directory
  sudo mkdir -p /opt/pitrex distro/tcz/build/"$ext_name".tcz/opt
  sudo mv /opt/pitrex distro/tcz/build/"$ext_name".tcz/opt/pitrex
  # If there's an XF86Config file for this program, include it too
  if [ -f "$ext_name/XF86Config" ]
  then
    mkdir -p "distro/tcz/build/$ext_name.tcz/usr/local/X11R6/etc/X11/$ext_name"
    cp -P "$ext_name/XF86Config" "distro/tcz/build/$ext_name.tcz/usr/local/X11R6/etc/X11/$ext_name/XF86Config"
    sudo chown root:root "distro/tcz/build/$ext_name.tcz/usr/local/X11R6/etc/X11/$ext_name/XF86Config"
  fi
  # Check that the directory has been moved successfully
  if cd distro/tcz/build/"$ext_name".tcz
  then
    # Generate the list of files in the extension
     # Add executables to list and strip them
    for file in `/usr/local/bin/find opt -executable ! -type d`
    do
      echo /"$file" >> ../../"$ext_name".tcz.list
      sudo strip "$file" 2>/dev/null
    done
     # Add other files to list
    /usr/local/bin/find opt ! -type d ! -executable | sed "s/^/\//g" >> ../../"$ext_name".tcz.list
    cd ../../../..
  else
   echo "Failed to create extension directory for $ext_name"
   exit 1
  fi
  # Make the extension
  mksquashfs "distro/tcz/build/$ext_name".tcz "distro/tcz/$ext_name".tcz
  # Make the checksum file
  md5sum "distro/tcz/$ext_name".tcz > "distro/tcz/$ext_name".tcz.md5.txt

  # Copy the .dep file over, if present
  if [ -f "$ext_name/$ext_name.tcz.dep" ]
  then
    cp -P "$ext_name/$ext_name.tcz.dep" distro/tcz/
  fi
done

# List any programs with makefile install rules that failed to install, and therefore
# didn't get an extension generated.
if [ "$failed" ]
then
  echo "-- NOTE: Install failed for: $failed --"
fi
