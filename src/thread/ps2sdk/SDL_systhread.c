#include "SDL_config.h"

#ifdef SDL_THREAD_PS2SDK

/* Thread management routines for SDL */

#include "SDL_error.h"
#include "SDL_thread.h"
#include "../SDL_systhread.h"
#include "../SDL_thread_c.h"

#ifdef	DISABLE_THREADS

int SDL_SYS_CreateThread(SDL_Thread *thread, void *args)
{
    SDL_SetError("Threads are not supported on this platform");
    return -1;
}

void SDL_SYS_SetupThread(void)
{
    return;
}

Uint32 SDL_ThreadID(void)
{
    return 0;
}

void SDL_SYS_WaitThread(SDL_Thread *thread)
{
    return;
}

void SDL_SYS_KillThread(SDL_Thread *thread)
{
    return;
}

#else /* Threads enabled */

#include <kernel.h>
#include <stdio.h>
#include <malloc.h>

#define STACK_SIZE 16384
extern void *_gp;

int SDL_SYS_CreateThread(SDL_Thread *thread, void *args)
{
    ee_thread_t th_attr;
    s32 tid;

    printf("SDL_SYS_CreateThread\n");

    th_attr.func = (void *)SDL_RunThread;
    th_attr.stack = (void *)malloc(STACK_SIZE);
    if (th_attr.stack == NULL)
    {
        SDL_OutOfMemory();
        return -1;
    }

    th_attr.stack_size = STACK_SIZE;
    th_attr.gp_reg = (void *)&_gp;
    th_attr.initial_priority = 64;
    th_attr.option = 0;

    // CreateThread returns integer thread ID
    tid = CreateThread(&th_attr);
    if (tid < 0)
    {
        SDL_SetError("Not enough resources to create thread");
        free(th_attr.stack);
        return -1;
    }

    // Store the thread ID in SDL_Thread->handle (cast to match SDL type)
    thread->handle = (SYS_ThreadHandle)(intptr_t)tid;

    printf("SDL_SYS_CreateThread ends successfully\n");

    StartThread(tid, args); // Start the thread using the integer ID
    printf("Thread started\n");

    return 0;
}

void SDL_SYS_SetupThread(void)
{
    return;
}

Uint32 SDL_ThreadID(void)
{
    return (Uint32)GetThreadId();
}

void SDL_SYS_WaitThread(SDL_Thread *thread)
{
    // Not implemented: SDL on PS2SDK may need custom implementation
    printf("SDL_SYS_WaitThread called but not implemented!\n");
}

void SDL_SYS_KillThread(SDL_Thread *thread)
{
    s32 tid = (s32)(intptr_t)thread->handle;
    TerminateThread(tid);
}

#endif /* DISABLE_THREADS */

#endif /* SDL_THREAD_PS2SDK */
