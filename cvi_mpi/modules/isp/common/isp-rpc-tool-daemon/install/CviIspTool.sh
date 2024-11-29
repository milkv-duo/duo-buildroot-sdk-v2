#!/bin/sh

ARCH="64bit"
RPCBIND=$(pgrep -f rpcbind)
PWD=$(pwd)

if [ -z "$RPCBIND" ]; then

	if [ ! -f "/etc/netconfig" ]; then
		cp ./$ARCH/netconfig /etc/
	fi

	if [ ! -f "/etc/bindresvport.blacklist" ]; then
		cp ./$ARCH/bindresvport.blacklist /etc/
	fi

	export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PWD/$ARCH
	./$ARCH/rpcbind
	sleep 1
	touch /tmp/startIspRpc
	sync
fi

./isp_tool_daemon
