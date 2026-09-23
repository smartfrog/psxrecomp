/* psx_tight_inline.h — Vita-only forced inlining for the generated-code hot
 * accessors. The recompiled game/overlay code compiles into single giant
 * functions (100 KB+ per shard), so GCC's inlining heuristics decline every
 * non-trivial helper there at ANY -O level (caller-growth budget exceeded
 * thousands of times over) — each guest instruction keeps a real `bl`.
 *
 * ENABLED ONLY when the build defines PSX_VITA_TIGHT_INLINE (CMake option
 * PSX_VITA_TIGHT_INLINE, default OFF). The cascade was measured on hardware
 * as +14.8 MB SELF and slower than the shipped baseline, so it is an A/B lever,
 * not a default: see vita/PERF-THROUGHPUT.md "measured and rejected".
 *
 * Every other target sees an empty macro and must remain byte-identical to the
 * unguarded code. */
#ifndef PSX_TIGHT_INLINE_H
#define PSX_TIGHT_INLINE_H

#if defined(__vita__) && defined(PSX_VITA_TIGHT_INLINE) &&               \
    !defined(PSX_OVERLAY_DLL_BUILD) && (defined(__GNUC__) || defined(__clang__))
#define PSX_TIGHT_INLINE __attribute__((always_inline))
#define PSX_TIGHT_INLINE_ACTIVE 1
#else
#define PSX_TIGHT_INLINE
#endif

#endif /* PSX_TIGHT_INLINE_H */
