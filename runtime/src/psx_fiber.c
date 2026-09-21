/*
 * psx_fiber.c — Win32-Fiber / POSIX-ucontext backends for psx_fiber.h.
 */
#include "psx_fiber.h"

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

psx_fiber_t psx_fiber_convert_thread(void)
{
    /* Attempt the conversion unconditionally and check the error code on
     * failure. The 0x1E00000000000000 TEB sentinel for "not yet a fiber"
     * is MSVC-internal and not guaranteed by MinGW's GetCurrentFiber()
     * implementation — reading it before ConvertThreadToFiber is called
     * can return an unspecified TEB value that isn't the sentinel, causing
     * the sentinel check to silently skip ConvertThreadToFiber, leaving
     * the thread as a non-fiber, and crashing the first SwitchToFiber. */
    void* fib = ConvertThreadToFiberEx(NULL, FIBER_FLAG_FLOAT_SWITCH);
    if (!fib && GetLastError() == ERROR_ALREADY_FIBER)
        fib = GetCurrentFiber();
    return (psx_fiber_t)fib;
}

psx_fiber_t psx_fiber_current(void)      { return (psx_fiber_t)GetCurrentFiber(); }

psx_fiber_t psx_fiber_create(size_t stack_size, psx_fiber_entry entry, void* arg)
{
    return (psx_fiber_t)CreateFiberEx(0, (SIZE_T)stack_size,
                                    FIBER_FLAG_FLOAT_SWITCH,
                                    (LPFIBER_START_ROUTINE)entry, arg);
}

void psx_fiber_switch(psx_fiber_t target) { SwitchToFiber((LPVOID)target); }
void psx_fiber_destroy(psx_fiber_t fiber) { if (fiber) DeleteFiber((LPVOID)fiber); }


#elif defined(__vita__)
/*
 * ARMv7 backend (PS Vita, newlib): no ucontext.h, so the context switch is a
 * hand-written arm/thumb assembly routine that saves/restores the AAPCS
 * callee-saved state: r4-r11, sp, lr and the VFP callee-saved d8-d15. Vita
 * user code always runs in thumb state, so stored code pointers carry the
 * thumb bit and the switch returns through `bx lr`. Semantics match the
 * ucontext backend: cooperative, one fiber running at a time, all switches
 * explicit.
 */

#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

typedef struct psx_fiber_impl {
    uint32_t regs[8];   /* r4-r11 */
    uint32_t sp;
    uint32_t lr;
    psx_fiber_entry entry;     /* offset 40: asm first-entry branch target */
    void*           arg;       /* offset 44: asm first-entry r0 */
    uint32_t        pending;   /* offset 48: created, never switched to */
    uint32_t        vfp[16];   /* offset 52: d8-d15 */
    psx_fiber_entry user_entry;   /* entry/arg the caller asked create() for */
    void*           user_arg;
    void*           stack;     /* NULL for the thread-fiber */
} psx_fiber_impl;

static _Thread_local psx_fiber_impl* s_current = NULL;

/* The naked asm below hardcodes these offsets. */
_Static_assert(offsetof(psx_fiber_impl, sp) == 32, "asm offset");
_Static_assert(offsetof(psx_fiber_impl, lr) == 36, "asm offset");
_Static_assert(offsetof(psx_fiber_impl, entry) == 40, "asm offset");
_Static_assert(offsetof(psx_fiber_impl, arg) == 44, "asm offset");
_Static_assert(offsetof(psx_fiber_impl, pending) == 48, "asm offset");
_Static_assert(offsetof(psx_fiber_impl, vfp) == 52, "asm offset");

/* r0 = save ctx, r1 = restore ctx. Callee-saved state plus sp/lr; the
 * d8-d15 pair keeps FPU-using guest code (GTE/GPU math) correct across a
 * switch regardless of the toolchain's float ABI. A never-entered fiber
 * branches to its entry instead of a saved lr, with r0 = arg. */
__attribute__((naked)) static void psx_fiber_asm_switch(
    psx_fiber_impl *from, psx_fiber_impl *to)
{
    __asm__ volatile(
        "stm r0, {r4-r11}\n"
        "mov r2, sp\n"
        "str r2, [r0, #32]\n"
        "str lr, [r0, #36]\n"
        "add r2, r0, #52\n"
        "vstm r2, {d8-d15}\n"
        "ldm r1, {r4-r11}\n"
        "ldr r2, [r1, #32]\n"
        "mov sp, r2\n"
        "ldr lr, [r1, #36]\n"
        "add r2, r1, #52\n"
        "vldm r2, {d8-d15}\n"
        "ldr r2, [r1, #48]\n"
        "cmp r2, #0\n"
        "beq 1f\n"
        "movs r0, #0\n"
        "str r0, [r1, #48]\n"
        "ldr r0, [r1, #44]\n"
        "ldr r2, [r1, #40]\n"
        "bx r2\n"
        "1:\n"
        "bx lr\n"
    );
}

static void psx_fiber_entry_thunk(void* arg)
{
    psx_fiber_impl* f = (psx_fiber_impl*)arg;
    f->user_entry(f->user_arg);
    /* The BIOS thread entry never returns normally (it switches back to its
     * scheduler target, or trap_crashes). Reaching here is a bug. */
    abort();
}

psx_fiber_t psx_fiber_convert_thread(void)
{
    if (!s_current) {
        psx_fiber_impl* f = (psx_fiber_impl*)calloc(1, sizeof(*f));
        if (!f) return NULL;
        f->stack = NULL;       /* runs on the real thread stack */
        s_current = f;
    }
    return (psx_fiber_t)s_current;
}

psx_fiber_t psx_fiber_current(void) { return (psx_fiber_t)s_current; }

psx_fiber_t psx_fiber_create(size_t stack_size, psx_fiber_entry entry, void* arg)
{
    if (stack_size < 16384u) stack_size = 16384u;
    psx_fiber_impl* f = (psx_fiber_impl*)calloc(1, sizeof(*f));
    if (!f) return NULL;
    f->stack = malloc(stack_size);
    if (!f->stack) { free(f); return NULL; }
    f->user_entry = entry;
    f->user_arg   = arg;
    f->entry = psx_fiber_entry_thunk;
    f->arg   = f;
    f->pending = 1u;
    /* First switch branches to the trampoline with r0 = f (the thunk's arg)
     * and sp inside the fresh stack, 8-byte aligned per AAPCS. */
    uintptr_t top = (uintptr_t)f->stack + stack_size;
    top &= ~(uintptr_t)7u;
    f->sp = (uint32_t)top;
    f->lr = 0u;   /* trampoline never returns; abort() if the entry does */
    return (psx_fiber_t)f;
}

void psx_fiber_switch(psx_fiber_t target)
{
    psx_fiber_impl* to   = (psx_fiber_impl*)target;
    psx_fiber_impl* from = s_current;
    if (!to || !from) abort();
    if (to == from) return;
    s_current = to;
    psx_fiber_asm_switch(from, to);
    /* Resumed: s_current was set back to `from` by whoever switched here. */
}

void psx_fiber_destroy(psx_fiber_t fiber)
{
    psx_fiber_impl* f = (psx_fiber_impl*)fiber;
    if (!f) return;
    free(f->stack);
    free(f);
}


#else /* POSIX: ucontext */

#ifndef _XOPEN_SOURCE
#  define _XOPEN_SOURCE 700
#endif
#ifdef __APPLE__
#  define _DARWIN_C_SOURCE 1
#endif

#include <ucontext.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <signal.h>    /* SIGSTKSZ — minimum fiber stack floor */

/* glibc >= 2.34 no longer exposes SIGSTKSZ as a compile-time constant under
 * _XOPEN_SOURCE; provide a portable fallback (used only as a stack floor). */
#ifndef SIGSTKSZ
#  define SIGSTKSZ 16384
#endif

#if defined(__clang__) || defined(__GNUC__)
#  pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

typedef struct psx_fiber_impl {
    ucontext_t      ctx;
    void*           stack;    /* NULL for the thread-fiber */
    psx_fiber_entry entry;
    void*           arg;
} psx_fiber_impl;

static _Thread_local psx_fiber_impl* s_current = NULL;

/* makecontext only passes ints; split the fiber pointer across two. */
static void psx_fiber_trampoline(unsigned int hi, unsigned int lo)
{
    uintptr_t p = ((uintptr_t)hi << 32) | (uintptr_t)lo;
    psx_fiber_impl* f = (psx_fiber_impl*)p;
    f->entry(f->arg);
    /* The BIOS thread entry never returns normally (it switches back to its
     * scheduler target, or trap_crashes). Reaching here is a bug. */
    abort();
}

psx_fiber_t psx_fiber_convert_thread(void)
{
    if (!s_current) {
        psx_fiber_impl* f = (psx_fiber_impl*)calloc(1, sizeof(*f));
        if (!f) return NULL;
        f->stack = NULL;       /* runs on the real thread stack */
        s_current = f;
    }
    return (psx_fiber_t)s_current;
}

psx_fiber_t psx_fiber_current(void) { return (psx_fiber_t)s_current; }

psx_fiber_t psx_fiber_create(size_t stack_size, psx_fiber_entry entry, void* arg)
{
    if (stack_size < SIGSTKSZ) stack_size = SIGSTKSZ;
    psx_fiber_impl* f = (psx_fiber_impl*)calloc(1, sizeof(*f));
    if (!f) return NULL;
    f->stack = malloc(stack_size);
    if (!f->stack) { free(f); return NULL; }
    f->entry = entry;
    f->arg   = arg;
    if (getcontext(&f->ctx) != 0) { free(f->stack); free(f); return NULL; }
    f->ctx.uc_stack.ss_sp   = f->stack;
    f->ctx.uc_stack.ss_size = stack_size;
    f->ctx.uc_link          = NULL;   /* entry never returns; see trampoline */
    uintptr_t p = (uintptr_t)f;
    makecontext(&f->ctx, (void (*)(void))psx_fiber_trampoline, 2,
                (unsigned int)(p >> 32), (unsigned int)(p & 0xFFFFFFFFu));
    return (psx_fiber_t)f;
}

void psx_fiber_switch(psx_fiber_t target)
{
    psx_fiber_impl* to   = (psx_fiber_impl*)target;
    psx_fiber_impl* from = s_current;
    if (!to || !from) abort();
    if (to == from) return;
    s_current = to;
    if (swapcontext(&from->ctx, &to->ctx) != 0) abort();
    /* Resumed: s_current was set back to `from` by whoever switched here. */
}

void psx_fiber_destroy(psx_fiber_t fiber)
{
    psx_fiber_impl* f = (psx_fiber_impl*)fiber;
    if (!f) return;
    free(f->stack);
    free(f);
}

#endif /* _WIN32 */
