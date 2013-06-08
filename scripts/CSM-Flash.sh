#!/bin/sh

if test -e "$1"; then
    echo "calling CSM bootloader..."
    if test ! -d /sys/class/gpio/gpio17; then echo 17 > /sys/class/gpio/export; fi
    if test ! -d /sys/class/gpio/gpio24; then echo 24 > /sys/class/gpio/export; fi
    echo out > /sys/class/gpio/gpio17/direction
    echo out > /sys/class/gpio/gpio24/direction
    echo 0 > /sys/class/gpio/gpio17/value
    echo 0 > /sys/class/gpio/gpio24/value
    sleep 1
    echo 1 > /sys/class/gpio/gpio17/value
    sleep 1
    echo 1 > /sys/class/gpio/gpio24/value

    echo "Programming CSM"
    avrdude -p atmega324p -P /dev/ttyAMA0 -b 38400 -c avr109 -U flash:w:$1
else
    echo "Error! No hex file given, or hex file could not found."
fi
