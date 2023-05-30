#!/bin/sh
## Rebuild XMAME VMMenu menu items
## Usage: zip2vmmenu.sh [menu data file] [roms directory]

MAINMENU="$1"
ROMDIR="$2"

rom=
romzip=
# Loop through all ZIP archives in roms directory
for romzip in "$ROMDIR"/*.zip "$ROMDIR"/*.ZIP
do
  # Check in case none upper/lower case
  [ -f "$romzip" ] || continue

  rommnu=
  last_rommnu=
  if [ -f "$romzip".mnu ]
  then
    # Skip zips with existing menu entries
    continue
  else
    # Find all menu entries matching ROM files and add them to game.zip.mnu
    mkdir -p /tmp/romzip
    rm -f /tmp/romzip/*
    unzip -qj "$romzip" -d /tmp/romzip
    for rom in /tmp/romzip/*
    do
      crc32="`crc32 \"$rom\"`"
      if rommnu="`grep -i \"^||${crc32%% *}||\" \"$MAINMENU\"`"
      then
        rommnu="${rommnu##*|| }"
        if [ "$rommnu" != "$last_rommnu" ] && ! grep -q "$rommnu" "$romzip".mnu 2>/dev/null
        then
          echo "$rommnu" >> "$romzip".mnu
          last_rommnu="$rommnu"
        fi
      else
        echo "No CRC match for ${rom#/tmp/romzip/} from $romzip" >&2
      fi
    done
    rm -f /tmp/romzip/*
  fi
done

cat "$ROMDIR"/*.zip.mnu
