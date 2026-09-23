/* psx_vita_attr.h — Vita-only per-frame host-time attribution (perf probe).
 *
 * The Vita build cannot be profiled, so the only way to split the host cycles
 * each guest instruction costs is to bracket the coarse regions the guest
 * passes through and accumulate host time per window:
 *
 *   frame    vblank-to-vblank wall time (the denominator)
 *   guest    frame - present: everything the guest drives (generated code,
 *            dirty-RAM interpreter, timing model, device service, rasterizer,
 *            MDEC)
 *   raster   gp0_execute_command() — one bracket per completed GP0 command,
 *            the entry point of gp0_exec_* → gpu_sw_renderer (never per pixel)
 *   mdec     execute_decode() — one bracket per MDEC decode command
 *   present  the vblank frontend body (pacing + scanout + present), the same
 *            region the rate line reports as vblank_body
 *
 * raster and mdec are nested inside guest; main.cpp prints
 * other = guest - raster - mdec, i.e. pure CPU emulation + timing + devices.
 *
 * Accumulators are raw SDL performance-counter ticks (microseconds on Vita);
 * main.cpp converts with SDL_GetPerformanceFrequency(). The whole body is
 * inside #ifdef __vita__, so a non-Vita build expands this header to nothing
 * and every bracket site compiles out. */
#ifndef PSXRECOMP_PSX_VITA_ATTR_H
#define PSXRECOMP_PSX_VITA_ATTR_H

#ifdef __vita__

#ifdef __cplusplus
extern "C" {
#endif

extern unsigned long long g_xg_attr_frame_ticks;    /* vblank-to-vblank */
extern unsigned long long g_xg_attr_present_ticks;  /* inside the vblank body */
extern unsigned long long g_xg_attr_raster_ticks;   /* gp0_execute_command */
extern unsigned long long g_xg_attr_mdec_ticks;     /* execute_decode */
extern unsigned long long g_xg_attr_raster_cmds;    /* brackets entered */
extern unsigned long long g_xg_attr_mdec_decodes;   /* brackets entered */

/* SDL_GetPerformanceCounter(); one syscall read, so it is only used at the
 * coarse boundaries above, never per guest instruction or per pixel. */
unsigned long long xg_attr_now(void);

#ifdef __cplusplus
}
#endif

#endif /* __vita__ */

#endif /* PSXRECOMP_PSX_VITA_ATTR_H */
