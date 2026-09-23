#ifndef PSXRECOMP_PSX_ICACHE_H
#define PSXRECOMP_PSX_ICACHE_H

#include "cpu_state.h"
#include "psx_vita_perf.h"

#ifdef __cplusplus
extern "C" {
#endif

extern uint32_t g_psx_icache_tv[1024];
extern int g_psx_icache_active;
extern int g_ls_replay_active;
void psx_icache_reset(void);
void psx_icache_fetch(CPUState *cpu, uint32_t addr);
void psx_icache_fetch_miss(CPUState *cpu, uint32_t addr);

/* Keep the interpreter's steady-state tag hit inside its translation unit.
 * Misses use the shared slow path, preserving exact cache evolution/timing. */
static inline void psx_icache_fetch_interp(CPUState *cpu, uint32_t addr) {
#ifdef PSX_ENABLE_BLOCK_CYCLES
    if (g_ls_replay_active) return;
    if (g_psx_icache_active < 0) {
#if defined(PSX_TIGHT_INLINE_ACTIVE)
        /* The cascade below macro-maps psx_icache_fetch onto this function, so
         * call the miss routine directly (same code) to avoid self-recursion. */
        psx_icache_fetch_miss(cpu, addr);
#else
        psx_icache_fetch(cpu, addr);
#endif
        return;
    }
    if (!g_psx_icache_active) return;
    uint32_t idx = (addr & 0xFFCu) >> 2;
    if (g_psx_icache_tv[idx] == addr) return;
    psx_icache_fetch_miss(cpu, addr);
#else
    (void)cpu;
    (void)addr;
#endif
}

#if defined(PSX_TIGHT_INLINE_ACTIVE)
/* Cascade ON: route every generated `psx_icache_fetch(cpu, addr)` call through
 * the inlined steady-state tag-hit path above (one extern bl per fetch
 * otherwise, ~1 per 3 guest instructions). Equivalent for a Vita release
 * build: lockstep replay and the icache shadow-diff state this path cannot
 * model are armed only by the dev diff channel, which PSX_NO_DEBUG_TOOLS
 * builds do not have. psx_icache.c undefines the name before exporting the
 * real symbol, and the fetch counter moves into this macro so the reported
 * rate keeps counting generated-code fetches. */
#define psx_icache_fetch(cpu_, addr_) \
    (++g_xg_vita_icache_fetches, psx_icache_fetch_interp((cpu_), (addr_)))
#endif

#ifdef __cplusplus
}
#endif

#endif
