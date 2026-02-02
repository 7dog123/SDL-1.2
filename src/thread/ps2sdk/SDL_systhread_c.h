/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2004 Sam Lantinga

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

    Sam Lantinga
    slouken@libsdl.org
*/

#include <tamtypes.h>

/* PS2 per-thread control block — exposed so other compilation units
 * can inspect thread state (tid, semaphore, stack, SDL data, etc.)
 */
typedef struct PS2_Thread {
    s32 tid;        /* kernel thread id */
    int sema;       /* semaphore id used for join/wakeup */
    void *stack;    /* allocated stack pointer */
    void *sdl_data; /* SDL thread function data pointer */
    int joined;     /* set when SDL_SYS_WaitThread cleaned up */
    int exited;     /* set when thread_entry finishes */
} PS2_Thread;

/* System thread handle is a pointer to the PS2_Thread control block */
typedef struct PS2_Thread* SYS_ThreadHandle;
/*
#ifndef DISABLE_THREADS
#define DISABLE_THREADS
#endif
*/
