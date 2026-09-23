/* psx_vita_perf.h — Vita-only throughput counters for the hardware perf loop.
 *
 * The Vita build cannot be profiled with a debugger, and the generated code's
 * cost is spread over per-instruction helpers, so the only way to attribute the
 * host cycles each guest instruction costs is to count how often each helper
 * runs. Each counter costs one add per helper call (~0.5 host instructions per
 * guest instruction, well under a percent) and is reported by main.cpp's
 * [xg-phase] rate line.
 *
 * The whole body is inside `#ifdef __vita__`, so a non-Vita build expands this
 * header to nothing at all — no declarations, no definitions, not even the
 * extern "C" braces, which keeps the preprocessed output of C++ TUs that
 * include it byte-identical to the unguarded tree. */
#ifndef PSXRECOMP_PSX_VITA_PERF_H
#define PSXRECOMP_PSX_VITA_PERF_H

#ifdef __vita__

#ifdef __cplusplus
extern "C" {
#endif

extern unsigned long long g_xg_vita_blocks_run;      /* AOT blocks entered (cpu_state.h wrapper) */
extern unsigned long long g_xg_vita_svc_calls;       /* psx_devices_service_to_now calls */
extern unsigned long long g_xg_vita_irq_checks;      /* psx_check_interrupts_at calls */
extern unsigned long long g_xg_vita_icache_fetches;  /* psx_icache_fetch calls */

#ifdef __cplusplus
}
#endif

#endif /* __vita__ */

#endif /* PSXRECOMP_PSX_VITA_PERF_H */
