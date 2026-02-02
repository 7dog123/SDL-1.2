#include "SDL_config.h"

#ifdef SDL_THREAD_PS2SDK

/* Thread management routines for SDL for PS2SDK
 *
 * Implement a lightweight thread wrapper that:
 *  - allocates a stack and a small per-thread control block
 *  - creates a semaphore to signal thread exit
 *  - starts the kernel thread, which runs a wrapper that calls
 *    SDL_RunThread(data) and signals the semaphore on exit
 *  - allows WaitThread to block on the semaphore and cleanup
 */

#include "SDL_error.h"
#include "SDL_thread.h"
#include "../SDL_systhread.h"
#include "../SDL_thread_c.h"

#include <kernel.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>

#define STACK_SIZE 16384
extern void *_gp;

/* Per-thread control block defined in the header so other compilation
 * units may inspect thread state. See SDL_systhread_c.h for the layout.
 */
/* Thread entry wrapper - runs in the newly created kernel thread */
static void thread_entry(void *arg)
{
    PS2_Thread *pt = (PS2_Thread *)arg;

    if (!pt) {
        /* Nothing we can do */
        ExitThread();
    }

    /* Do any thread setup required by SDL */
    SDL_SYS_SetupThread();

    /* Call the SDL thread function */
    SDL_RunThread(pt->sdl_data);

    /* Mark exited and signal that the thread is exiting so joiners can continue */
    pt->exited = 1;
    if (pt->sema >= 0) {
        if (SignalSema(pt->sema) < 0) {
            /* best effort: log error but continue exiting */
            SDL_SetError("Failed to signal thread semaphore on exit");
        }
    }

    /* Exit the thread */
    ExitThread();
}

int SDL_SYS_CreateThread(SDL_Thread *thread, void *args)
{
    ee_thread_t th_attr;
    s32 tid;
    PS2_Thread *pt;
    ee_sema_t sem_attr;
    int start_ret;

    if (!thread) {
        SDL_SetError("NULL thread pointer passed to SDL_SYS_CreateThread");
        return -1;
    }

    pt = (PS2_Thread *)malloc(sizeof(*pt));
    if (pt == NULL) {
        SDL_OutOfMemory();
        return -1;
    }
    memset(pt, 0, sizeof(*pt));
    pt->sema = -1;
    pt->joined = 0;
    pt->exited = 0;

    pt->stack = malloc(STACK_SIZE);
    if (pt->stack == NULL) {
        SDL_OutOfMemory();
        free(pt);
        return -1;
    }

    /* Create a semaphore that starts at 0 so WaitThread can wait for it */
    sem_attr.init_count = 0;
    sem_attr.max_count  = 1;
    sem_attr.option     = 0;
    sem_attr.attr       = 0;
    pt->sema = CreateSema(&sem_attr);
    if (pt->sema < 0) {
        SDL_SetError("Failed to create semaphore for thread");
        free(pt->stack);
        free(pt);
        return -1;
    }

    /* Save SDL data pointer for the wrapper */
    pt->sdl_data = args;

    /* Prepare thread attributes and create kernel thread */
    th_attr.func = (void *)thread_entry;
    th_attr.stack = pt->stack;
    th_attr.stack_size = STACK_SIZE;
    th_attr.gp_reg = (void *)&_gp;
    th_attr.initial_priority = 64;
    th_attr.option = 0;

    tid = CreateThread(&th_attr);
    if (tid < 0) {
        SDL_SetError("Not enough resources to create thread");
        if (pt->sema >= 0) DeleteSema(pt->sema);
        free(pt->stack);
        free(pt);
        return -1;
    }

    pt->tid = tid;

    /* Store pointer to PS2_Thread as the SDL handle */
    thread->handle = (SYS_ThreadHandle)pt;
    thread->threadid = (Uint32)tid;

    /* Start the thread, passing our per-thread control block */
    start_ret = StartThread(tid, pt);
    if (start_ret < 0) {
        /* Attempt to clean up the kernel thread (best effort) */
        TerminateThread(tid);
        if (pt->sema >= 0) DeleteSema(pt->sema);
        free(pt->stack);
        free(pt);
        thread->handle = NULL;
        SDL_SetError("Failed to start thread");
        return -1;
    }

    return 0;
}

void SDL_SYS_SetupThread(void)
{
    /* Nothing special needed for PS2 at thread start for now */
}

Uint32 SDL_ThreadID(void)
{
    return (Uint32)GetThreadId();
}

void SDL_SYS_WaitThread(SDL_Thread *thread)
{
    PS2_Thread *pt = (PS2_Thread *)thread->handle;

    if (pt == NULL) {
        return;
    }

    /* Prevent double-joins */
    if (pt->joined) {
        return;
    }

    /* Wait until the thread signals its semaphore (i.e., exits) */
    if (pt->sema >= 0) {
        if (WaitSema(pt->sema) < 0) {
            SDL_SetError("Error waiting for thread semaphore");
        }
        if (DeleteSema(pt->sema) < 0) {
            SDL_SetError("Failed to delete thread semaphore");
        }
        pt->sema = -1;
    }

    pt->joined = 1;

    /* Free thread stack and control block */
    if (pt->stack) {
        free(pt->stack);
        pt->stack = NULL;
    }

    free(pt);
    thread->handle = NULL;
}

void SDL_SYS_KillThread(SDL_Thread *thread)
{
    PS2_Thread *pt = (PS2_Thread *)thread->handle;

    if (pt == NULL) {
        return;
    }

    /* Terminate the thread (best effort) */
    if (TerminateThread(pt->tid) < 0) {
        SDL_SetError("Failed to terminate thread");
    }

    /* Signal the semaphore so waiters wake up */
    if (pt->sema >= 0) {
        if (SignalSema(pt->sema) < 0) {
            SDL_SetError("Failed to signal thread semaphore in kill");
        }
    }
}

#endif /* SDL_THREAD_PS2SDK */
