#!/bin/sh

usb_en=453
usb_select=510

function set_gpio()
{
	local gpio_num=$1
	local gpio_val=$2
	local gpio_path="/sys/class/gpio/gpio${gpio_num}"

	if test -d ${gpio_path}; then
		echo "GPIO ${gpio_num} already exported" >> /tmp/ncm.log 2>&1
	else
		echo ${gpio_num} > /sys/class/gpio/export
	fi

	echo out > ${gpio_path}/direction
	sleep 0.1
	echo ${gpio_val} > ${gpio_path}/value
}

set_gpio ${usb_en} 0
sleep 0.5
set_gpio ${usb_select} 0
sleep 0.5

/etc/uhubon.sh device >> /tmp/ncm.log 2>&1
/etc/run_usb.sh probe ncm >> /tmp/ncm.log 2>&1
if test -e /mnt/system/burnd; then
  /etc/run_usb.sh probe acm >> /tmp/ncm.log 2>&1
fi
/etc/run_usb.sh start ncm >> /tmp/ncm.log 2>&1

sleep 0.5
ifconfig usb0 192.168.42.1

count=`ps | grep dnsmasq | grep -v grep | wc -l`
if [ ${count} -lt 1 ] ;then
  echo "/etc/init.d/S80dnsmasq start" >> /tmp/ncm.log 2>&1
  /etc/init.d/S80dnsmasq start >> /tmp/ncm.log 2>&1
fi

sleep 2
mkdir -p /lib/firmware
if test -e /mnt/system/burnd; then
  burnd &
  if test -e /lib/firmware/arduino.elf; then
    sleep 2
    echo stop  > /sys/class/remoteproc/remoteproc0/state
    echo start > /sys/class/remoteproc/remoteproc0/state
  fi
fi
