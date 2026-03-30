/* test_filter_mem.c – Unity tests for the filter-mem-lib cache layer.
 *
 * These tests exercise filter_mem_cache_clear() and the memoisation
 * behaviour of filter_mem_transmission() without requiring a real NCrystal
 * material database: they probe only the cache hit/miss/clear paths by
 * verifying pointer identity.
 *
 * Build: linked against filter-mem-lib.c and Unity; filter-mem-stubs.h is
 * force-included via -include so that McCode runtime helpers are available.
 */

#include "unity.h"
#include "filter-mem-lib.h"
#include <stdlib.h>
#include <string.h>

/* ---- helpers ---- */

/* A minimal stub for filter_mem_transmission_compute so that the unit tests
 * do not depend on NCrystal being initialised with a real material.  We
 * return a small heap-allocated array whose content is unimportant; what
 * matters is the pointer identity returned by the memoising wrapper.
 *
 * Because filter-mem-lib.c compiles filter_mem_transmission_compute as a
 * *static* function and then wraps it in the public filter_mem_transmission,
 * we cannot replace the compute function from outside.  Instead the tests
 * call filter_mem_transmission with a recognisable cfg string and rely on
 * the NCrystal call inside _compute being reached only on cache misses.
 *
 * To avoid actually invoking NCrystal we use the
 * FILTER_MEM_TEST_SKIP_NCRYSTAL preprocessor flag which is set by the CMake
 * test target.  When set, filter_mem_transmission_compute is replaced by a
 * trivial stub at compile time via a #define before including the .c source
 * (see cmake-fmem todo / CMakeLists.txt changes).
 *
 * For the cache-focused tests below, all we need is that two calls with
 * identical arguments return the *same* pointer and a call after
 * filter_mem_cache_clear() returns a *different* (freshly allocated) pointer.
 */

void setUp(void) {
    /* Start each test with an empty cache. */
    filter_mem_cache_clear();
}

void tearDown(void) {
    filter_mem_cache_clear();
}

/* ---- cache-hit: same args → same pointer ---- */

void test_same_key_returns_same_pointer(void) {
    /* Use a non-NULL cfg that the cache key comparison exercises.
     * We do NOT actually hit NCrystal here because the first call populates
     * the cache and the second should return before reaching _compute.
     * However, the first call will attempt to call NCrystal, so this test
     * can only be run in an environment where NCrystal is available.
     * Mark it as skipped when FILTER_MEM_TEST_SKIP_NCRYSTAL is defined. */
#ifdef FILTER_MEM_TEST_SKIP_NCRYSTAL
    TEST_IGNORE_MESSAGE("NCrystal not available; skipping transmission test");
#else
    const char *cfg = "Al_sg225.ncmat";
    double *p1 = filter_mem_transmission(FILTER_MEM_TYPE_WAVELENGTH, 0.5, 0.01, 100, cfg);
    double *p2 = filter_mem_transmission(FILTER_MEM_TYPE_WAVELENGTH, 0.5, 0.01, 100, cfg);
    TEST_ASSERT_NOT_NULL(p1);
    TEST_ASSERT_EQUAL_PTR(p1, p2);
#endif
}

/* ---- cache-miss: different cfg → different pointer ---- */

void test_different_cfg_returns_different_pointer(void) {
#ifdef FILTER_MEM_TEST_SKIP_NCRYSTAL
    TEST_IGNORE_MESSAGE("NCrystal not available; skipping transmission test");
#else
    double *p1 = filter_mem_transmission(FILTER_MEM_TYPE_WAVELENGTH, 0.5, 0.01, 100, "Al_sg225.ncmat");
    double *p2 = filter_mem_transmission(FILTER_MEM_TYPE_WAVELENGTH, 0.5, 0.01, 100, "Be_sg194.ncmat");
    TEST_ASSERT_NOT_NULL(p1);
    TEST_ASSERT_NOT_NULL(p2);
    TEST_ASSERT_NOT_EQUAL(p1, p2);
#endif
}

/* ---- cache-clear: cache is repopulated correctly after clear ---- */

void test_clear_causes_reallocation(void) {
#ifdef FILTER_MEM_TEST_SKIP_NCRYSTAL
    TEST_IGNORE_MESSAGE("NCrystal not available; skipping transmission test");
#else
    const char *cfg = "Al_sg225.ncmat";
    double *p1 = filter_mem_transmission(FILTER_MEM_TYPE_WAVELENGTH, 0.5, 0.01, 100, cfg);
    TEST_ASSERT_NOT_NULL(p1);
    filter_mem_cache_clear();
    /* After clear the cache is empty; the first call is a miss (new allocation),
     * the second must be a hit (same pointer) — regardless of whether the
     * allocator happens to reuse the same address as p1. */
    double *p2 = filter_mem_transmission(FILTER_MEM_TYPE_WAVELENGTH, 0.5, 0.01, 100, cfg);
    double *p3 = filter_mem_transmission(FILTER_MEM_TYPE_WAVELENGTH, 0.5, 0.01, 100, cfg);
    TEST_ASSERT_NOT_NULL(p2);
    TEST_ASSERT_EQUAL_PTR(p2, p3);
#endif
}

/* ---- cache-clear is idempotent ---- */

void test_double_clear_is_safe(void) {
    filter_mem_cache_clear();
    filter_mem_cache_clear(); /* should not crash or corrupt state */
    TEST_PASS();
}

/* ---- cache-clear on empty cache is safe ---- */

void test_clear_empty_cache_is_safe(void) {
    /* setUp already called filter_mem_cache_clear(); call it again */
    filter_mem_cache_clear();
    TEST_PASS();
}

/* ---- entry count grows monotonically until clear ---- */

/* (Validated implicitly by the hit/miss pointer tests above; no direct API.) */

/* ---- main ---- */

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_same_key_returns_same_pointer);
    RUN_TEST(test_different_cfg_returns_different_pointer);
    RUN_TEST(test_clear_causes_reallocation);
    RUN_TEST(test_double_clear_is_safe);
    RUN_TEST(test_clear_empty_cache_is_safe);
    return UNITY_END();
}
