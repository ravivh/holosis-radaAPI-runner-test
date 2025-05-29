#! /bin/sh

echo spidev > /sys/bus/spi/devices/spi1.0/driver_override
echo spi1.0 > /sys/bus/spi/drivers/spidev/bind

busybox devmem 0x303840d0 32 0x3