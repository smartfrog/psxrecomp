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

/* ── AOT-routing instrumentation (vita-aot-routing) ──────────────────────
 * One counter per decision point on the path from a game-text PC to a
 * compiled block, so a single hardware run names the gate that rejects:
 * text-image guard state → range validation reason → dirty-dispatch entry →
 * identity gate → interrupt/vsync sub-gates → compiled execution. All are
 * plain increments on cold-ish paths; the per-window deltas are printed by
 * main.cpp's [xg-route] line (same cadence as the rate line). */

/* Text-image guard (memory.c). */
extern unsigned long long g_xg_route_guard_arms;       /* register_text_image accepted */
extern unsigned int       g_xg_route_guard_crc;        /* CRC32 of the registered image */
extern unsigned int       g_xg_route_guard_lo;         /* registered physical lo */
extern unsigned int       g_xg_route_guard_hi;         /* registered physical hi */
extern unsigned long long g_xg_route_baseline_clears;  /* clear_image_baseline calls */
extern unsigned int       g_xg_route_baseline_frame;   /* frame at the (first) clear */
extern unsigned int       g_xg_route_baseline_modified;/* modified pages at clear */
extern unsigned int       g_xg_route_baseline_diverged;/* diverged pages at clear */
extern unsigned long long g_xg_route_refcheck_bytes;   /* one-shot ref-vs-live bytes compared */
extern unsigned long long g_xg_route_refcheck_bad;     /* ... that differed */
extern unsigned int       g_xg_route_refcheck_first;   /* first differing offset + 1 (0 = none) */
extern unsigned int       g_xg_route_refcheck_live;    /* live byte at the first difference */
extern unsigned int       g_xg_route_refcheck_ref;     /* reference byte at the first difference */

/* Range validation (memory.c dirty_ram_text_native_ok_ranges_from). */
extern unsigned long long g_xg_route_range_calls;      /* calls (all are in-text by construction) */
extern unsigned long long g_xg_route_range_pass;       /* returned 1 */
extern unsigned long long g_xg_route_range_noref;      /* rejected: no reference image */
extern unsigned long long g_xg_route_range_bounds;     /* rejected: range outside the image */
extern unsigned long long g_xg_route_range_memcmp;     /* rejected: live bytes != reference */
extern unsigned int       g_xg_route_bounds_lo;        /* last out-of-bounds range lo */
extern unsigned int       g_xg_route_bounds_len;       /* last out-of-bounds range len */

/* Dirty-dispatch AOT attempt (dirty_ram_interp.c). */
extern unsigned long long g_xg_route_text_ok;          /* native_ok passed at a dirty entry */
extern unsigned long long g_xg_route_text_blocked;     /* native_ok failed at a dirty entry */
extern unsigned long long g_xg_route_text_aot;         /* psx_dispatch_game_compiled returned 1 */
extern unsigned long long g_xg_route_text_miss;        /* ... returned 0 after native_ok passed */
extern unsigned int       g_xg_route_last_blocked_addr;/* last rejected game-text PC */
extern unsigned int       g_xg_route_last_aot_addr;    /* last PC that ran compiled code */

/* Identity gate (game_identity.c). */
extern unsigned long long g_xg_route_id_bind_calls;    /* psx_game_identity_bind_static calls */
extern unsigned long long g_xg_route_id_bind_ok;       /* ... that bound the static identity */
extern unsigned long long g_xg_route_id_gate_calls;    /* psx_game_identity_gate calls */
extern unsigned long long g_xg_route_id_gate_ok;       /* ... that passed */

/* Sub-gates inside psx_dispatch_game_compiled (reached only past identity+ranges). */
extern unsigned long long g_xg_route_irq_entry_calls;  /* psx_check_interrupts_dispatch_entry calls */
extern unsigned long long g_xg_route_irq_entry_taken;  /* ... that took an interrupt instead */
extern unsigned long long g_xg_route_vsync_try_calls;  /* psx_vsync_query_hle_try calls */
extern unsigned long long g_xg_route_vsync_try_handled;/* ... that handled the query */

/* Static overlay AOT (overlay_loader.c overlay_static_dispatch). */
extern unsigned long long g_xg_route_ovl_static_tries; /* generated overlay dispatch attempts */
extern unsigned long long g_xg_route_ovl_static_hits;  /* ... that ran generated overlay code */

#ifdef __cplusplus
}
#endif

#endif /* __vita__ */

#endif /* PSXRECOMP_PSX_VITA_PERF_H */
