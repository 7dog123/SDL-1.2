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

#ifndef _SDL_PS2GL_H_
#define _SDL_PS2GL_H_

#if SDL_VIDEO_OPENGL_PS2GL

#include <GL/gl.h>
#include <GL/ps2gl.h>

#include "../SDL_sysvideo.h"

/* Hidden "this" pointer for the video functions */
#undef _THIS
#define _THIS	SDL_VideoDevice *_this

struct SDL_PrivateGLData {
	int gl_active;			/* context is up and running */

	/* gs memory slots */
	pgl_slot_handle_t frame_slot[2];
	pgl_slot_handle_t depth_slot;

	/* gs memory areas */
	pgl_area_handle_t frame_area[2];
	pgl_area_handle_t depth_area;
};

/* Context management */
extern int PS2_GL_CreateContext(_THIS);
extern void PS2_GL_Shutdown(_THIS);

/* OpenGL driver functions */
extern int PS2_GL_LoadLibrary(_THIS, const char *path);
extern void *PS2_GL_GetProcAddress(_THIS, const char *proc);
extern int PS2_GL_GetAttribute(_THIS, SDL_GLattr attrib, int *value);
extern int PS2_GL_MakeCurrent(_THIS);
extern void PS2_GL_SwapBuffers(_THIS);

#endif /* SDL_VIDEO_OPENGL_PS2GL */

#endif /* _SDL_PS2GL_H_ */
