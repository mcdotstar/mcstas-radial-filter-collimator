/* test_fgm.c – Unity tests for filter-mem-lib pure-C functions.
 *
 * Covers: filter_mem_options, filter_mem_geometry_axis, filter_mem_geometry.
 *
 * fgm-stubs.h is force-included by the CMake target (via -include) so that
 * all McCode runtime helpers are available when compiling both this file and
 * filter-mem-lib.c.  It also defines MCSTAS, which suppresses the conflicting
 * function-prototype block in filter-mem-lib.h. */

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

/* All six axis parameters unset → function returns without touching outputs. */
void test_options_all_unset_returns_early(void)
{
    double omin = -99.0, ostep = -99.0;
    enum filter_mem_mode mode = FILTER_MEM_MODE_SET;
    enum filter_mem_type type = FILTER_MEM_TYPE_UNKNOWN;
    int verbose = 0;

    filter_mem_options(U,U, U,U, U,U, &omin, &ostep, NULL, &mode, &type, &verbose);

    TEST_ASSERT_EQUAL_DOUBLE(-99.0, omin);
    TEST_ASSERT_EQUAL_DOUBLE(-99.0, ostep);
    TEST_ASSERT_EQUAL_INT(FILTER_MEM_TYPE_UNKNOWN, type);
}

/* Only emin+estep set → energy axis selected. */
void test_options_energy_axis_selected(void)
{
    double omin = 0.0, ostep = 0.0;
    enum filter_mem_mode mode = FILTER_MEM_MODE_SET;
    enum filter_mem_type type = FILTER_MEM_TYPE_UNKNOWN;
    int verbose = 0;

    filter_mem_options(1.0, 0.5,  U,U,  U,U,  &omin, &ostep, NULL, &mode, &type, &verbose);

    TEST_ASSERT_EQUAL_INT(FILTER_MEM_TYPE_ENERGY, type);
    TEST_ASSERT_EQUAL_DOUBLE(1.0, omin);
    TEST_ASSERT_EQUAL_DOUBLE(0.5, ostep);
}

/* Only lmin+lstep set → wavelength axis selected. */
void test_options_wavelength_axis_selected(void)
{
    double omin = 0.0, ostep = 0.0;
    enum filter_mem_mode mode = FILTER_MEM_MODE_SET;
    enum filter_mem_type type = FILTER_MEM_TYPE_UNKNOWN;
    int verbose = 0;

    filter_mem_options(U,U,  0.5, 0.1,  U,U,  &omin, &ostep, NULL, &mode, &type, &verbose);

    TEST_ASSERT_EQUAL_INT(FILTER_MEM_TYPE_WAVELENGTH, type);
    TEST_ASSERT_EQUAL_DOUBLE(0.5, omin);
    TEST_ASSERT_EQUAL_DOUBLE(0.1, ostep);
}

/* Only kmin+kstep set → wavenumber axis selected. */
void test_options_wavenumber_axis_selected(void)
{
    double omin = 0.0, ostep = 0.0;
    enum filter_mem_mode mode = FILTER_MEM_MODE_SET;
    enum filter_mem_type type = FILTER_MEM_TYPE_UNKNOWN;
    int verbose = 0;

    filter_mem_options(U,U,  U,U,  2.0, 0.2,  &omin, &ostep, NULL, &mode, &type, &verbose);

    TEST_ASSERT_EQUAL_INT(FILTER_MEM_TYPE_WAVENUMBER, type);
    TEST_ASSERT_EQUAL_DOUBLE(2.0, omin);
    TEST_ASSERT_EQUAL_DOUBLE(0.2, ostep);
}

/* Two axes set simultaneously → ambiguous, function returns without modifying
 * outputs (energy+wavelength both set). */
void test_options_two_axes_set_returns_early(void)
{
    double omin = -99.0, ostep = -99.0;
    enum filter_mem_mode mode = FILTER_MEM_MODE_SET;
    enum filter_mem_type type = FILTER_MEM_TYPE_UNKNOWN;
    int verbose = 0;

    filter_mem_options(1.0, 0.5,  0.5, 0.1,  U,U,  &omin, &ostep, NULL, &mode, &type, &verbose);

    TEST_ASSERT_EQUAL_DOUBLE(-99.0, omin);
    TEST_ASSERT_EQUAL_DOUBLE(-99.0, ostep);
    TEST_ASSERT_EQUAL_INT(FILTER_MEM_TYPE_UNKNOWN, type);
}

/* Only emin set, estep unset → incomplete axis → returns early. */
void test_options_partial_axis_returns_early(void)
{
    double omin = -99.0, ostep = -99.0;
    enum filter_mem_mode mode = FILTER_MEM_MODE_SET;
    enum filter_mem_type type = FILTER_MEM_TYPE_UNKNOWN;
    int verbose = 0;

    filter_mem_options(1.0, U,  U,U,  U,U,  &omin, &ostep, NULL, &mode, &type, &verbose);

    TEST_ASSERT_EQUAL_DOUBLE(-99.0, omin);
    TEST_ASSERT_EQUAL_INT(FILTER_MEM_TYPE_UNKNOWN, type);
}

/* opts="set" → mode becomes FILTER_MEM_MODE_SET (energy axis used to get past early return). */
void test_options_mode_set(void)
{
    double omin = 0.0, ostep = 0.0;
    enum filter_mem_mode mode = FILTER_MEM_MODE_ADD;
    enum filter_mem_type type = FILTER_MEM_TYPE_UNKNOWN;
    int verbose = 0;
    char opts[] = "set";

    filter_mem_options(1.0, 0.5,  U,U,  U,U,  &omin, &ostep, opts, &mode, &type, &verbose);

    TEST_ASSERT_EQUAL_INT(FILTER_MEM_MODE_SET, mode);
}

/* opts="add" → mode becomes FILTER_MEM_MODE_ADD. */
void test_options_mode_add(void)
{
    double omin = 0.0, ostep = 0.0;
    enum filter_mem_mode mode = FILTER_MEM_MODE_SET;
    enum filter_mem_type type = FILTER_MEM_TYPE_UNKNOWN;
    int verbose = 0;
    char opts[] = "add";

    filter_mem_options(1.0, 0.5,  U,U,  U,U,  &omin, &ostep, opts, &mode, &type, &verbose);

    TEST_ASSERT_EQUAL_INT(FILTER_MEM_MODE_ADD, mode);
}

/* opts="multiply" → mode becomes FILTER_MEM_MODE_MUL. */
void test_options_mode_multiply(void)
{
    double omin = 0.0, ostep = 0.0;
    enum filter_mem_mode mode = FILTER_MEM_MODE_SET;
    enum filter_mem_type type = FILTER_MEM_TYPE_UNKNOWN;
    int verbose = 0;
    char opts[] = "multiply";

    filter_mem_options(1.0, 0.5,  U,U,  U,U,  &omin, &ostep, opts, &mode, &type, &verbose);

    TEST_ASSERT_EQUAL_INT(FILTER_MEM_MODE_MUL, mode);
}

/* opts="verbose" → verbose flag set to 1. */
void test_options_verbose_flag(void)
{
    double omin = 0.0, ostep = 0.0;
    enum filter_mem_mode mode = FILTER_MEM_MODE_SET;
    enum filter_mem_type type = FILTER_MEM_TYPE_UNKNOWN;
    int verbose = 0;
    char opts[] = "verbose";

    filter_mem_options(1.0, 0.5,  U,U,  U,U,  &omin, &ostep, opts, &mode, &type, &verbose);

    TEST_ASSERT_EQUAL_INT(1, verbose);
}

/* opts="" (empty string) → mode and verbose unchanged. */
void test_options_empty_opts_no_change(void)
{
    double omin = 0.0, ostep = 0.0;
    enum filter_mem_mode mode = FILTER_MEM_MODE_SET;
    enum filter_mem_type type = FILTER_MEM_TYPE_UNKNOWN;
    int verbose = 0;
    char opts[] = "";

    filter_mem_options(1.0, 0.5,  U,U,  U,U,  &omin, &ostep, opts, &mode, &type, &verbose);

    TEST_ASSERT_EQUAL_INT(FILTER_MEM_TYPE_ENERGY, type);
    TEST_ASSERT_EQUAL_INT(FILTER_MEM_MODE_SET, mode);
    TEST_ASSERT_EQUAL_INT(0, verbose);
}

/* opts can carry both a mode keyword and "verbose" in one string. */
void test_options_multiply_and_verbose(void)
{
    double omin = 0.0, ostep = 0.0;
    enum filter_mem_mode mode = FILTER_MEM_MODE_SET;
    enum filter_mem_type type = FILTER_MEM_TYPE_UNKNOWN;
    int verbose = 0;
    char opts[] = "multiply verbose";

    filter_mem_options(U,U,  0.5, 0.1,  U,U,  &omin, &ostep, opts, &mode, &type, &verbose);

    TEST_ASSERT_EQUAL_INT(FILTER_MEM_TYPE_WAVELENGTH, type);
    TEST_ASSERT_EQUAL_INT(FILTER_MEM_MODE_MUL, mode);
    TEST_ASSERT_EQUAL_INT(1, verbose);
}

/* ==================================================================
 * filter_mem_geometry_axis
 * ================================================================== */

/* Both min and max unset → derived symmetrically from length. */
void test_geometry_axis_both_unset_derives_symmetric(void)
{
    double mn = U, mx = U;
    filter_mem_geometry_axis(1.0, &mn, &mx);
    TEST_ASSERT_EQUAL_DOUBLE( 0.5, mx);
    TEST_ASSERT_EQUAL_DOUBLE(-0.5, mn);
}

/* min unset, max known → min = max - length. */
void test_geometry_axis_min_unset_derives_from_max(void)
{
    double mn = U, mx = 1.0;
    filter_mem_geometry_axis(0.4, &mn, &mx);
    TEST_ASSERT_EQUAL_DOUBLE(1.0, mx);
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, 0.6, mn);
}

/* max unset, min known → max = min + length. */
void test_geometry_axis_max_unset_derives_from_min(void)
{
    double mn = -0.2, mx = U;
    filter_mem_geometry_axis(0.6, &mn, &mx);
    TEST_ASSERT_EQUAL_DOUBLE(-0.2, mn);
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, 0.4, mx);
}

/* Both min and max already set → function is a no-op. */
void test_geometry_axis_both_set_unchanged(void)
{
    double mn = -0.3, mx = 0.7;
    filter_mem_geometry_axis(99.0, &mn, &mx);
    TEST_ASSERT_EQUAL_DOUBLE(-0.3, mn);
    TEST_ASSERT_EQUAL_DOUBLE( 0.7, mx);
}

/* ==================================================================
 * filter_mem_geometry
 * ================================================================== */

/* All six inputs unset → returns 0 (no geometry). */
void test_geometry_all_unset_returns_zero(void)
{
    double wmin = U, wmax = U, hmin = U, hmax = U;
    int r = filter_mem_geometry(U, &wmin, &wmax, U, &hmin, &hmax);
    TEST_ASSERT_EQUAL_INT(0, r);
}

/* Width dimension entirely unset, height partially specified → returns -1. */
void test_geometry_width_all_unset_returns_minus_one(void)
{
    double wmin = U, wmax = U, hmin = -0.5, hmax = U;
    int r = filter_mem_geometry(U, &wmin, &wmax, 1.0, &hmin, &hmax);
    TEST_ASSERT_EQUAL_INT(-1, r);
}

/* Height dimension entirely unset, width partially specified → returns -1. */
void test_geometry_height_all_unset_returns_minus_one(void)
{
    double wmin = U, wmax = U, hmin = U, hmax = U;
    int r = filter_mem_geometry(0.4, &wmin, &wmax, U, &hmin, &hmax);
    TEST_ASSERT_EQUAL_INT(-1, r);
}

/* Both dimensions specified by length → returns 1 and derives all bounds. */
void test_geometry_by_length_derives_bounds(void)
{
    double wmin = U, wmax = U, hmin = U, hmax = U;
    int r = filter_mem_geometry(0.4, &wmin, &wmax, 0.6, &hmin, &hmax);
    TEST_ASSERT_EQUAL_INT(1, r);
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, -0.2, wmin);
    TEST_ASSERT_DOUBLE_WITHIN(1e-12,  0.2, wmax);
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, -0.3, hmin);
    TEST_ASSERT_DOUBLE_WITHIN(1e-12,  0.3, hmax);
}

/* Width by explicit bounds (wmin/wmax set, width unset),
 * height by length → both resolved, returns 1. */
void test_geometry_width_by_bounds_height_by_length(void)
{
    double wmin = -0.1, wmax = 0.3, hmin = U, hmax = U;
    int r = filter_mem_geometry(U, &wmin, &wmax, 0.8, &hmin, &hmax);
    TEST_ASSERT_EQUAL_INT(1, r);
    TEST_ASSERT_EQUAL_DOUBLE(-0.1, wmin);
    TEST_ASSERT_EQUAL_DOUBLE( 0.3, wmax);
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, -0.4, hmin);
    TEST_ASSERT_DOUBLE_WITHIN(1e-12,  0.4, hmax);
}

/* Width with only wmin set and length, height fully explicit → returns 1. */
void test_geometry_width_derives_max_from_min_and_length(void)
{
    double wmin = 0.1, wmax = U, hmin = -0.5, hmax = 0.5;
    int r = filter_mem_geometry(0.6, &wmin, &wmax, U, &hmin, &hmax);
    TEST_ASSERT_EQUAL_INT(1, r);
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, 0.7, wmax);
    TEST_ASSERT_EQUAL_DOUBLE(-0.5, hmin);
    TEST_ASSERT_EQUAL_DOUBLE( 0.5, hmax);
}

/* ==================================================================
 * main
 * ================================================================== */

int main(void)
{
    UNITY_BEGIN();

    /* filter_mem_options */
    RUN_TEST(test_options_all_unset_returns_early);
    RUN_TEST(test_options_energy_axis_selected);
    RUN_TEST(test_options_wavelength_axis_selected);
    RUN_TEST(test_options_wavenumber_axis_selected);
    RUN_TEST(test_options_two_axes_set_returns_early);
    RUN_TEST(test_options_partial_axis_returns_early);
    RUN_TEST(test_options_mode_set);
    RUN_TEST(test_options_mode_add);
    RUN_TEST(test_options_mode_multiply);
    RUN_TEST(test_options_verbose_flag);
    RUN_TEST(test_options_empty_opts_no_change);
    RUN_TEST(test_options_multiply_and_verbose);

    /* filter_mem_geometry_axis */
    RUN_TEST(test_geometry_axis_both_unset_derives_symmetric);
    RUN_TEST(test_geometry_axis_min_unset_derives_from_max);
    RUN_TEST(test_geometry_axis_max_unset_derives_from_min);
    RUN_TEST(test_geometry_axis_both_set_unchanged);

    /* filter_mem_geometry */
    RUN_TEST(test_geometry_all_unset_returns_zero);
    RUN_TEST(test_geometry_width_all_unset_returns_minus_one);
    RUN_TEST(test_geometry_height_all_unset_returns_minus_one);
    RUN_TEST(test_geometry_by_length_derives_bounds);
    RUN_TEST(test_geometry_width_by_bounds_height_by_length);
    RUN_TEST(test_geometry_width_derives_max_from_min_and_length);

    return UNITY_END();
}
