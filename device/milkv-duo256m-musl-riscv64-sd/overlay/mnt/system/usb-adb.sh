#!/bin/sh

/etc/uhubon.sh device >> /tmp/adb.log 2>&1
/etc/run_usb.sh probe adb >> /tmp/adb.log 2>&1
/etc/run_usb.sh start adb >> /tmp/adb.log 2>&1

