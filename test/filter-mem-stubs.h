/* filter-mem-stubs.h – Standalone stubs for McCode runtime helpers used by
 * filter-mem-lib (formerly fgm-lib).
 *
 * Include this (or force-include it via -include) BEFORE filter-mem-lib.h so
 * that the #ifndef MCSTAS guard in that header skips its conflicting
 * declarations and our macro/inline implementations take effect instead.
 *
 * filter-mem-lib uses the explicit-count calling convention:
 *   all_unset(6, emin, estep, lmin, lstep, kmin, kstep)
 * The pass-through macros below forward the arguments unchanged, so the
 * explicit count is passed directly to the variadic helper functions.
 *
 * Provides:
 *   UNSET            – the sentinel quiet-NaN used for "parameter not set"
 *   nans_match       – bitwise comparison of two NaN payloads
 *   is_unset / is_set / is_valid
 *   all_unset / all_set / any_unset / any_set – variadic macros (explicit count)
 *   SE2V, MS2AA, PI  – McCode physical constants
 */

#ifndef FILTER_MEM_STUBS_H
#define FILTER_MEM_STUBS_H

/* Tell filter-mem-lib.h that we are supplying the helper symbols ourselves. */
#ifndef MCSTAS
#  define MCSTAS
#endif

#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

/* ---- UNSET sentinel ---- */

/* The same NaN payload used by McCode ("notset" in ASCII = 0x6E6F74736574). */
#define UNSET (nan("0x6E6F74736574"))

/* ---- Single-value helpers ---- */

static inline int nans_match(double a, double b) {
    uint64_t ua, ub;
    memcpy(&ua, &a, sizeof ua);
    memcpy(&ub, &b, sizeof ub);
    return ua == ub;
}

static inline int is_unset(double v) {
    return isnan(v) && nans_match(v, UNSET);
}

static inline int is_valid(double v) {
    return !isnan(v);
}

static inline int is_set(double v) {
    return !is_unset(v);
}

/* ---- Variadic helper implementations ---- */

static inline int _fgm_all_unset(int n, ...) {
    va_list ap;
    va_start(ap, n);
    for (int i = 0; i < n; i++)
        if (!is_unset(va_arg(ap, double))) { va_end(ap); return 0; }
    va_end(ap);
    return 1;
}

static inline int _fgm_all_set(int n, ...) {
    va_list ap;
    va_start(ap, n);
    for (int i = 0; i < n; i++)
        if (!is_set(va_arg(ap, double))) { va_end(ap); return 0; }
    va_end(ap);
    return 1;
}

static inline int _fgm_any_unset(int n, ...) {
    va_list ap;
    va_start(ap, n);
    for (int i = 0; i < n; i++)
        if (is_unset(va_arg(ap, double))) { va_end(ap); return 1; }
    va_end(ap);
    return 0;
}

static inline int _fgm_any_set(int n, ...) {
    va_list ap;
    va_start(ap, n);
    for (int i = 0; i < n; i++)
        if (is_set(va_arg(ap, double))) { va_end(ap); return 1; }
    va_end(ap);
    return 0;
}

/* ---- Public variadic macros (pass-through; caller supplies count) ---- */

#define all_unset(...) _fgm_all_unset(__VA_ARGS__)
#define all_set(...)   _fgm_all_set(__VA_ARGS__)
#define any_unset(...) _fgm_any_unset(__VA_ARGS__)
#define any_set(...)   _fgm_any_set(__VA_ARGS__)

/* ---- McCode physical constants ---- */

#ifndef SE2V
#  define SE2V 437.393377   /* sqrt(meV) → m/s  (sqrt(2e/m_n), e in meV) */
#endif
#ifndef MS2AA
#  define MS2AA 3956.034    /* m/s → angstrom  (h/m_n in AA·m/s) */
#endif
#ifndef PI
#  define PI M_PI
#endif

#endif /* FILTER_MEM_STUBS_H */
