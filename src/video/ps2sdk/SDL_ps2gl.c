/*
	SDL - Simple DirectMedia Layer
	Copyright (C) 1997, 1998, 1999, 2000  Sam Lantinga

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Library General Public
	License as published by the Free Software Foundation; either
	version 2 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Library General Public License for more details.

	You should have received a copy of the GNU Library General Public
	License along with this library; if not, write to the Free
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

	Gil Megidish
	gil@megidish.net

	Based on code by 

	Sam Lantinga
	slouken@libsdl.org
*/
#include "SDL_config.h"

#if SDL_VIDEO_OPENGL_PS2GL

#include "SDL.h"
#include "SDL_error.h"
#include "../SDL_sysvideo.h"
#include "SDL_ps2video.h"
#include "SDL_ps2gl.h"

#include <gif_registers.h>
#include <string.h>

/* P2GL is statically linked, there is no library to load */

/*
 * The GS is partitioned into pages of 8192 bytes. Frame buffers live in
 * fixed slots at the bottom of GS memory, textures go into slots above.
 * A page holds one 64x32 tile of 32-bit RGBA.
 */

/* size of one gs page, in bytes */
#define GS_PAGE_SIZE	8192

/* pixels per row of the display area, all ps2 modes are 640 wide */
#define DISPLAY_WIDTH	640

/* preferred pixel formats, values match ps2stuff's GS::tPSM enum */
#define PSM_CT32	0
#define PSM_PSMZ24	49

/* number of pages used by a 32bit frame buffer of the given height */
#define PAGES_FOR(height)	(((DISPLAY_WIDTH * (height) * 4) + GS_PAGE_SIZE - 1) / GS_PAGE_SIZE)

/* texture slots start above the depth buffer */
#define TEXTURE_BASE(pages_per_field)	(3 * (pages_per_field))

/* immediate mode geometry buffers passed to pglInit() */
#define IMM_BUFFER_VERTEXES	(32*1024)
#define IMM_BUFFER_QWORDS	1000

static int create_gs_memory(_THIS)
{
	int height;
	int pages;
	pgl_slot_handle_t slot0, slot1, zslot;

	/* one interlaced field: 224 lines on NTSC, 256 on PAL */
	height = _this->hidden->screen_h;
	if (height < 224 || height > 256) {
		SDL_SetError("P2GL: unsupported field height %d", height);
		return -1;
	}
	pages = PAGES_FOR(height);

	slot0 = pglAddGsMemSlot(0, pages, PSM_CT32);
	slot1 = pglAddGsMemSlot(pages, pages, PSM_CT32);
	zslot = pglAddGsMemSlot(2 * pages, pages, PSM_PSMZ24);
	pglLockGsMemSlot(slot0);
	pglLockGsMemSlot(slot1);
	pglLockGsMemSlot(zslot);

	_this->gl_data->frame_slot[0] = slot0;
	_this->gl_data->frame_slot[1] = slot1;
	_this->gl_data->depth_slot = zslot;

	/* double buffered display plus a depth buffer */
	_this->gl_data->frame_area[0] = pglCreateGsMemArea(DISPLAY_WIDTH, height, PSM_CT32);
	_this->gl_data->frame_area[1] = pglCreateGsMemArea(DISPLAY_WIDTH, height, PSM_CT32);
	_this->gl_data->depth_area = pglCreateGsMemArea(DISPLAY_WIDTH, height, PSM_PSMZ24);

	pglBindGsMemAreaToSlot(_this->gl_data->frame_area[0], _this->gl_data->frame_slot[0]);
	pglBindGsMemAreaToSlot(_this->gl_data->frame_area[1], _this->gl_data->frame_slot[1]);
	pglBindGsMemAreaToSlot(_this->gl_data->depth_area, _this->gl_data->depth_slot);

	/* draw into the new areas and display from them */
	pglSetDrawBuffers(PGL_INTERLACED,
		_this->gl_data->frame_area[0], _this->gl_data->frame_area[1],
		_this->gl_data->depth_area);
	pglSetDisplayBuffers(PGL_INTERLACED,
		_this->gl_data->frame_area[0], _this->gl_data->frame_area[1]);

	/* generic texture slots for user textures */
	pglAddGsMemSlot(TEXTURE_BASE(pages), 8, PSM_CT32);
	pglAddGsMemSlot(TEXTURE_BASE(pages) + 8, 8, PSM_CT32);
	pglAddGsMemSlot(TEXTURE_BASE(pages) + 16, 32, PSM_CT32);
	pglAddGsMemSlot(TEXTURE_BASE(pages) + 48, 32, PSM_CT32);
	pglAddGsMemSlot(TEXTURE_BASE(pages) + 80, 64, PSM_CT32);
	pglAddGsMemSlot(TEXTURE_BASE(pages) + 144, 64, PSM_CT32);

	return 0;
}

int PS2_GL_CreateContext(_THIS)
{
	/* reset the GIF; OSDSYS leaves PATH3 busy which makes the GIF
	   ignore our PATH1/PATH2 transfers */
	GIF_REG_CTRL = 1;

	if (!pglHasLibraryBeenInitted()) {
		printf("SDL: initializing P2GL\n");
		if (pglInit(IMM_BUFFER_VERTEXES, IMM_BUFFER_QWORDS) == 0) {
			SDL_SetError("Failed to initialize P2GL");
			return -1;
		}
	}

	if (!pglHasGsMemBeenInitted()) {
		if (create_gs_memory(_this) < 0) {
			return -1;
		}
	} else {
		/* gs memory already partitioned (possibly by us before),
		   just make sure our buffers are being displayed */
		pglSetDrawBuffers(PGL_INTERLACED,
			_this->gl_data->frame_area[0], _this->gl_data->frame_area[1],
			_this->gl_data->depth_area);
		pglSetDisplayBuffers(PGL_INTERLACED,
			_this->gl_data->frame_area[0], _this->gl_data->frame_area[1]);
	}

	_this->gl_data->gl_active = 1;
	return 0;
}

void PS2_GL_Shutdown(_THIS)
{
	_this->gl_data->gl_active = 0;
}

int PS2_GL_LoadLibrary(_THIS, const char *path)
{
	/* P2GL is linked statically, nothing to load */
	_this->gl_config.driver_loaded = 1;
	return 0;
}

void *PS2_GL_GetProcAddress(_THIS, const char *proc)
{
	void *addr = NULL;

	(void)_this;

	if (proc == NULL) {
		return NULL;
	}

	/* resolve the functions SDL itself uses at runtime */
#define SDL_PROC_UNUSED(ret,func,params)
#define SDL_PROC(ret,func,params) \
	if (!addr && SDL_strcmp(proc, #func) == 0) { \
		addr = (void *)(func); \
	}
#include "../SDL_glfuncs.h"
#undef SDL_PROC
#undef SDL_PROC_UNUSED

#define PGL_PROC(func) \
	if (!addr && SDL_strcmp(proc, #func) == 0) { \
		addr = (void *)(func); \
	}
	/* expose the pgl extension API as well */
	PGL_PROC(pglInit)
	PGL_PROC(pglFinish)
	PGL_PROC(pglWaitForVU1)
	PGL_PROC(pglWaitForVSync)
	PGL_PROC(pglSwapBuffers)
	PGL_PROC(pglHasLibraryBeenInitted)
	PGL_PROC(pglHasGsMemBeenInitted)
	PGL_PROC(pglPrintGsMemAllocation)
	PGL_PROC(pglAddGsMemSlot)
	PGL_PROC(pglLockGsMemSlot)
	PGL_PROC(pglUnlockGsMemSlot)
	PGL_PROC(pglRemoveAllGsMemSlots)
	PGL_PROC(pglCreateGsMemArea)
	PGL_PROC(pglDestroyGsMemArea)
	PGL_PROC(pglAllocGsMemArea)
	PGL_PROC(pglFreeGsMemArea)
	PGL_PROC(pglSetGsMemAreaWordAddr)
	PGL_PROC(pglBindGsMemAreaToSlot)
	PGL_PROC(pglUnbindGsMemArea)
	PGL_PROC(pglLockGsMemArea)
	PGL_PROC(pglUnlockGsMemArea)
	PGL_PROC(pglGsMemAreaIsAllocated)
	PGL_PROC(pglGetGsMemAreaWordAddr)
	PGL_PROC(pglSetDisplayBuffers)
	PGL_PROC(pglSetDrawBuffers)
	PGL_PROC(pglTextureFromGsMemArea)
	PGL_PROC(pglBindTextureToSlot)
	PGL_PROC(pglFreeTexture)
	PGL_PROC(pglNormalPointer)
	PGL_PROC(pglDrawIndexedArrays)
	PGL_PROC(pglBeginImmediateGeometry)
	PGL_PROC(pglEndImmediateGeometry)
	PGL_PROC(pglRenderImmediateGeometry)
	PGL_PROC(pglFinishRenderingImmediateGeometry)
	PGL_PROC(pglBeginGeometry)
	PGL_PROC(pglEndGeometry)
	PGL_PROC(pglRenderGeometry)
	PGL_PROC(pglFinishRenderingGeometry)
	PGL_PROC(pglSetRenderingFinishedCallback)
	PGL_PROC(pglEnable)
	PGL_PROC(pglDisable)
	PGL_PROC(pglSetInterlacingOffset)
	PGL_PROC(pglGetCurRendererName)
#undef PGL_PROC

	return addr;
}

/* Get attribute data from gl. */
int PS2_GL_GetAttribute(_THIS, SDL_GLattr attrib, int* value)
{
	int retval = 0;

	switch (attrib) {
		case SDL_GL_RED_SIZE:
		*value = 8;
		break;
		case SDL_GL_GREEN_SIZE:
		*value = 8;
		break;
		case SDL_GL_BLUE_SIZE:
		*value = 8;
		break;
		case SDL_GL_ALPHA_SIZE:
		*value = 8;
		break;
		case SDL_GL_DOUBLEBUFFER:
		*value = 1;
		break;
		case SDL_GL_DEPTH_SIZE:
		*value = 24;
		break;
		case SDL_GL_STENCIL_SIZE:
		case SDL_GL_ACCUM_RED_SIZE:
		case SDL_GL_ACCUM_GREEN_SIZE:
		case SDL_GL_ACCUM_BLUE_SIZE:
		case SDL_GL_ACCUM_ALPHA_SIZE:
		case SDL_GL_STEREO:
		*value = 0;
		break;
		default:
		retval = -1;
		break;
	}

	if (retval < 0) {
		SDL_SetError("OpenGL attribute is unsupported on this system");
	}
	return retval;
}

int PS2_GL_MakeCurrent(_THIS)
{
	/* single global context, always current */
	return 0;
}

void PS2_GL_SwapBuffers(_THIS)
{
	pglSwapBuffers();
}

/*
 * libps2gl doesn't implement the attribute stack, but SDL's
 * SDL_OPENGLBLIT path calls these through GetProcAddress.
 */
void glPushAttrib(GLbitfield mask)
{
	(void)mask;
}

void glPopAttrib(void)
{
}

void glPushClientAttrib(GLbitfield mask)
{
	(void)mask;
}

void glPopClientAttrib(void)
{
}

#endif /* SDL_VIDEO_OPENGL_PS2GL */
