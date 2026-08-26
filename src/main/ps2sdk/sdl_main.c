#include <stdio.h>

#include "SDL_main.h"

/* Bring the graphics hardware up before anything else runs. Doing this
   here (rather than lazily inside SDL_VideoInit) is the ordering every
   PlayStation 2 title uses: GS/DMA first, then everything else. It also
   means no SIF/RPC traffic is required before the display is up, which
   matters when an ELF is booted directly by an emulator's loader. */
extern void PS2SDL_PreInitVideo(void);

#undef main
int main(int argc, char *argv[])
{
	PS2SDL_PreInitVideo();

	return SDL_main(argc, argv);
}
