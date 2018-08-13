#!/bin/bash

if [ "${1}" = "" ]
then
  echo "Usage: ./program.sh <image.bin>"
  exit 1
fi

echo "# jlink programming script" > .prog.jlink
echo "device atsamr21e18" >> .prog.jlink
echo "speed 400" >> .prog.jlink
echo "loadbin ${1},0 " >> .prog.jlink
echo "exit" >> .prog.jlink

./JLinkExe -if swd .prog.jlink
