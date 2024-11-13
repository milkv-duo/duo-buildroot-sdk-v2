/*
 * isp rpc tool daemon
 */

#include <unistd.h>
#include "cvi_ispd.h"
#include "cvi_bin.h"

#define WEBSOCKET_PORT	5566

int main(int argc, char *argv[])
{
	// argc = argc;
	// argv = argv;

	printf("cvi isp rpc tool daemon start...\n");

	isp_daemon_init(WEBSOCKET_PORT);

	while (1) {
		sleep(1);
	}

	return 0;
}

