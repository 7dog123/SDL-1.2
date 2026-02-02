#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <iopheap.h>
#include <iopcontrol.h>
#include <libcdvd.h>
#include <smod.h>
#include <stdio.h>

#include "SDL_main.h"

int SDL_HasMMX()
{
	return 0;
}

#undef main
int main(int argc, char *argv[])
{
	/* Initialize RPC so we can query IOP modules */
	SifInitRpc(0);
	/* Make sure early printf()s are visible immediately */
	setbuf(stdout, NULL);

#ifdef PS2SDL_ENABLE_MTAP
	smod_mod_info_t info;
	/* smod_get_mod_by_name() returns 0 when the module is found */
	if (smod_get_mod_by_name("sio2man", &info) == 0) {
		printf("PS2SDL: sio2man detected, resetting IOP\n");
		fflush(stdout);

		/* Cleanly shut down IOP services before resetting */
		sceCdInit(SCECdEXIT);
		SifExitIopHeap();
		SifLoadFileExit();
		SifExitRpc();

		/* Reset IOP and wait for it to come back up */
		SifIopReset(NULL, 0);
		while (!SifIopSync()) { /* wait */ };

		/* Reinitialize RPC after reset */
		SifInitRpc(0);
	}
#endif

	return SDL_main(argc, argv);
}
