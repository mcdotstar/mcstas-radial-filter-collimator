/* test_fgm.c – Unity tests for filter-mem-lib pure-C functions.
 *
 * Covers: filter_mem_options, filter_mem_which, filter_mem_geometry.
 *
 * filter-mem-stubs.h is force-included by the CMake target (via -include) so
 * that all McCode runtime helpers are available when compiling both this file
 * and filter-mem-lib.c.  It also defines MCSTAS, which suppresses the
 * conflicting function-prototype block in filter-mem-lib.h. */

#include "unity.h"
#include "filter-mem-lib.h"
#include <math.h>

void setUp(void)    {}
void tearDown(void) {}

/* ==================================================================
 * Helpers
 * ================================================================== */

/* Unset doubles used to initialise output parameters. */
#define U UNSET

/* ==================================================================
 * filter_mem_options
 * ================================================================== */

/* opts=NULL → no-op; mode and verbose unchanged. */
void test_options_null_opts_no_change(void)
{
    enum filter_mem_mode mode = FILTER_MEM_MODE_ADD;
    int verbose = 0;
    filter_mem_options(NULL, &mode, &verbose);
    TEST_ASSERT_EQUAL_INT(FILTER_MEM_MODE_ADD, mode);
    TEST_ASSERT_EQUAL_INT(0, verbose);
}

/* opts="" → no-op. */
void test_options_empty_opts_no_change(void)
{
    enum filter_mem_mode mode = FILTER_MEM_MODE_ADD;
    int verbose = 0;
    char opts[] = "";
    filter_mem_options(opts, &mode, &verbose);
    TEST_ASSERT_EQUAL_INT(FILTER_MEM_MODE_ADD, mode);
    TEST_ASSERT_EQUAL_INT(0, verbose);
}

/* opts="set" → mode becomes SET. */
void test_options_mode_set(void)
{
    enum filter_mem_mode mode = FILTER_MEM_MODE_ADD;
    int verbose = 0;
    char opts[] = "set";
    filter_mem_options(opts, &mode, &verbose);
    TEST_ASSERT_EQUAL_INT(FILTER_MEM_MODE_SET, mode);
}

/* opts="add" → mode becomes ADD. */
void test_options_mode_add(void)
{
    enum filter_mem_mode mode = FILTER_MEM_MODE_SET;
    int verbose = 0;
    char opts[] = "add";
    filter_mem_options(opts, &mode, &verbose);
    TEST_ASSERT_EQUAL_INT(FILTER_MEM_MODE_ADD, mode);
}

/* opts="multiply" → mode becomes MUL. */
void test_options_mode_multiply(void)
{
    enum filter_mem_mode mode = FILTER_MEM_MODE_SET;
    int verbose = 0;
    char opts[] = "multiply";
    filter_mem_options(opts, &mode, &verbose);
    TEST_ASSERT_EQUAL_INT(FILTER_MEM_MODE_MUL, mode);
}

/* opts="verbose" → verbose flag set to 1, mode unchanged. */
void test_options_verbose_flag(void)
{
    enum filter_mem_mode mode = FILTER_MEM_MODE_MUL;
    int verbose = 0;
    char opts[] = "verbose";
    filter_mem_options(opts, &mode, &verbose);
    TEST_ASSERT_EQUAL_INT(1, verbose);
    TEST_ASSERT_EQUAL_INT(FILTER_MEM_MODE_MUL, mode);
}

/* opts can carry both a mode keyword and "verbose". */
void test_options_multiply_and_verbose(void)
{
    enum filter_mem_mode mode = FILTER_MEM_MODE_SET;
    int verbose = 0;
    char opts[] = "multiply verbose";
    filter_mem_options(opts, &mode, &verbose);
    TEST_ASSERT_EQUAL_INT(FILTER_MEM_MODE_MUL, mode);
    TEST_ASSERT_EQUAL_INT(1, verbose);
}

/* ==================================================================
 * filter_mem_which
 * ================================================================== */

/* NULL which → defaults to WAVELENGTH; all-UNSET bounds filled with defaults. */
void test_which_null_defaults_to_wavelength(void)
{
    double mn = U, mx = U, dl = U;
    int cnt = 0;
    enum filter_mem_type type = FILTER_MEM_TYPE_UNKNOWN;
    filter_mem_which(NULL, &mn, &mx, &dl, &cnt, &type);
    TEST_ASSERT_EQUAL_INT(FILTER_MEM_TYPE_WAVELENGTH, type);
    TEST_ASSERT_FALSE(isnan(mn));
    TEST_ASSERT_FALSE(isnan(mx));
    TEST_ASSERT_FALSE(isnan(dl));
    TEST_ASSERT_GREATER_THAN_INT(0, cnt);
}

/* Empty string which → WAVELENGTH (same as NULL). */
void test_which_empty_string_defaults_to_wavelength(void)
{
    double mn = U, mx = U, dl = U;
    int cnt = 0;
    enum filter_mem_type type = FILTER_MEM_TYPE_UNKNOWN;
    char which[] = "";
    filter_mem_which(which, &mn, &mx, &dl, &cnt, &type);
    TEST_ASSERT_EQUAL_INT(FILTER_MEM_TYPE_WAVELENGTH, type);
}

/* "E" → ENERGY type. */
void test_which_E_selects_energy(void)
{
    double mn = U, mx = U, dl = U;
    int cnt = 0;
    enum filter_mem_type type = FILTER_MEM_TYPE_UNKNOWN;
    char which[] = "E";
    filter_mem_which(which, &mn, &mx, &dl, &cnt, &type);
    TEST_ASSERT_EQUAL_INT(FILTER_MEM_TYPE_ENERGY, type);
}

/* "energy" → ENERGY type. */
void test_which_energy_string_selects_energy(void)
{
    double mn = U, mx = U, dl = U;
    int cnt = 0;
    enum filter_mem_type type = FILTER_MEM_TYPE_UNKNOWN;
    char which[] = "energy";
    filter_mem_which(which, &mn, &mx, &dl, &cnt, &type);
    TEST_ASSERT_EQUAL_INT(FILTER_MEM_TYPE_ENERGY, type);
}

/* "k" → WAVENUMBER type. */
void test_which_k_selects_wavenumber(void)
{
    double mn = U, mx = U, dl = U;
    int cnt = 0;
    enum filter_mem_type type = FILTER_MEM_TYPE_UNKNOWN;
    char which[] = "k";
    filter_mem_which(which, &mn, &mx, &dl, &cnt, &type);
    TEST_ASSERT_EQUAL_INT(FILTER_MEM_TYPE_WAVENUMBER, type);
}

/* All params already set → count recalculated as (max-min)/delta. */
void test_which_all_set_recalculates_count(void)
{
    double mn = 1.0, mx = 3.0, dl = 0.5;
    int cnt = 0;
    enum filter_mem_type type = FILTER_MEM_TYPE_UNKNOWN;
    char which[] = "wavelength";
    filter_mem_which(which, &mn, &mx, &dl, &cnt, &type);
    TEST_ASSERT_EQUAL_INT(FILTER_MEM_TYPE_WAVELENGTH, type);
    TEST_ASSERT_EQUAL_INT(4, cnt);   /* (3.0 - 1.0) / 0.5 = 4 */
    TEST_ASSERT_EQUAL_DOUBLE(1.0, mn);
    TEST_ASSERT_EQUAL_DOUBLE(3.0, mx);
    TEST_ASSERT_EQUAL_DOUBLE(0.5, dl);
}

/* Only delta UNSET, min/max/count all set → delta derived as (max-min)/count. */
void test_which_derives_delta_from_min_max_count(void)
{
    double mn = 0.5, mx = 5.5, dl = U;
    int cnt = 10;
    enum filter_mem_type type = FILTER_MEM_TYPE_UNKNOWN;
    char which[] = "L";
    filter_mem_which(which, &mn, &mx, &dl, &cnt, &type);
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, 0.5, dl);   /* (5.5 - 0.5) / 10 = 0.5 */
    TEST_ASSERT_EQUAL_DOUBLE(0.5, mn);
    TEST_ASSERT_EQUAL_DOUBLE(5.5, mx);
}

/* ==================================================================
 * filter_mem_geometry
 * ================================================================== */

/* All six inputs UNSET → returns 1 (no geometry defined). */
void test_geometry_all_unset_returns_one(void)
{
    double wmin = U, wmax = U, hmin = U, hmax = U;
    int r = filter_mem_geometry(U, &wmin, &wmax, U, &hmin, &hmax);
    TEST_ASSERT_EQUAL_INT(1, r);
}

/* Width dimension entirely unresolvable (all x values UNSET), height partially
 * specified (length + one bound) → returns 2 (x-axis problem). */
void test_geometry_width_unresolvable_returns_2(void)
{
    double wmin = U, wmax = U, hmin = -0.5, hmax = U;
    int r = filter_mem_geometry(U, &wmin, &wmax, 1.0, &hmin, &hmax);
    TEST_ASSERT_EQUAL_INT(2, r);
}

/* Height dimension entirely unresolvable (all y values UNSET), width
 * specified by length → returns 4 (y-axis problem). */
void test_geometry_height_unresolvable_returns_4(void)
{
    double wmin = U, wmax = U, hmin = U, hmax = U;
    int r = filter_mem_geometry(0.4, &wmin, &wmax, U, &hmin, &hmax);
    TEST_ASSERT_EQUAL_INT(4, r);
}

/* Both dimensions specified by length → returns 0 and derives all bounds. */
void test_geometry_by_length_derives_bounds(void)
{
    double wmin = U, wmax = U, hmin = U, hmax = U;
    int r = filter_mem_geometry(0.4, &wmin, &wmax, 0.6, &hmin, &hmax);
    TEST_ASSERT_EQUAL_INT(0, r);
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, -0.2, wmin);
    TEST_ASSERT_DOUBLE_WITHIN(1e-12,  0.2, wmax);
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, -0.3, hmin);
    TEST_ASSERT_DOUBLE_WITHIN(1e-12,  0.3, hmax);
}

/* Width by explicit bounds, height by length → returns 0. */
void test_geometry_width_by_bounds_height_by_length(void)
{
    double wmin = -0.1, wmax = 0.3, hmin = U, hmax = U;
    int r = filter_mem_geometry(U, &wmin, &wmax, 0.8, &hmin, &hmax);
    TEST_ASSERT_EQUAL_INT(0, r);
    TEST_ASSERT_EQUAL_DOUBLE(-0.1, wmin);
    TEST_ASSERT_EQUAL_DOUBLE( 0.3, wmax);
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, -0.4, hmin);
    TEST_ASSERT_DOUBLE_WITHIN(1e-12,  0.4, hmax);
}

/* Width derives wmax from wmin + length, height fully explicit → returns 0. */
void test_geometry_width_derives_max_from_min_and_length(void)
{
    double wmin = 0.1, wmax = U, hmin = -0.5, hmax = 0.5;
    int r = filter_mem_geometry(0.6, &wmin, &wmax, U, &hmin, &hmax);
    TEST_ASSERT_EQUAL_INT(0, r);
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, 0.7, wmax);
    TEST_ASSERT_EQUAL_DOUBLE(-0.5, hmin);
    TEST_ASSERT_EQUAL_DOUBLE( 0.5, hmax);
}

/* All four bounds already set explicitly (length ignored) → returns 0, no change. */
void test_geometry_all_bounds_set_unchanged(void)
{
    double wmin = -0.1, wmax = 0.1, hmin = -0.2, hmax = 0.2;
    int r = filter_mem_geometry(99.0, &wmin, &wmax, 99.0, &hmin, &hmax);
    TEST_ASSERT_EQUAL_INT(0, r);
    TEST_ASSERT_EQUAL_DOUBLE(-0.1, wmin);
    TEST_ASSERT_EQUAL_DOUBLE( 0.1, wmax);
    TEST_ASSERT_EQUAL_DOUBLE(-0.2, hmin);
    TEST_ASSERT_EQUAL_DOUBLE( 0.2, hmax);
}

/* ==================================================================
 * main
 * ================================================================== */

int main(void)
{
    UNITY_BEGIN();

    /* filter_mem_options */
    RUN_TEST(test_options_null_opts_no_change);
    RUN_TEST(test_options_empty_opts_no_change);
    RUN_TEST(test_options_mode_set);
    RUN_TEST(test_options_mode_add);
    RUN_TEST(test_options_mode_multiply);
    RUN_TEST(test_options_verbose_flag);
    RUN_TEST(test_options_multiply_and_verbose);

    /* filter_mem_which */
    RUN_TEST(test_which_null_defaults_to_wavelength);
    RUN_TEST(test_which_empty_string_defaults_to_wavelength);
    RUN_TEST(test_which_E_selects_energy);
    RUN_TEST(test_which_energy_string_selects_energy);
    RUN_TEST(test_which_k_selects_wavenumber);
    RUN_TEST(test_which_all_set_recalculates_count);
    RUN_TEST(test_which_derives_delta_from_min_max_count);

    /* filter_mem_geometry */
    RUN_TEST(test_geometry_all_unset_returns_one);
    RUN_TEST(test_geometry_width_unresolvable_returns_2);
    RUN_TEST(test_geometry_height_unresolvable_returns_4);
    RUN_TEST(test_geometry_by_length_derives_bounds);
    RUN_TEST(test_geometry_width_by_bounds_height_by_length);
    RUN_TEST(test_geometry_width_derives_max_from_min_and_length);
    RUN_TEST(test_geometry_all_bounds_set_unchanged);

    return UNITY_END();
}
