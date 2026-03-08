/* test_filter_collimator.c – Unity tests for rfc_filter_hit_time and
 * rfc_collimator_hit_time.
 *
 * Standard test geometry
 * ----------------------
 *   Filter / collimator volume: r_min = 0.5 m, r_max = 1.5 m,
 *     y_min = -0.5 m, y_max = 0.5 m, full angular revolution.
 *   Collimator delta_angle = 0.1 rad  (≈ 62.8 channels in 2π).
 *
 * All test particles travel in the x-z plane with y = 0, vy = 0 and
 * use the +x radial direction (angle atan2(x,z) = π/2 ≈ 1.5708 rad).
 *
 * With a_min = -π and delta_angle = 0.1 the channel containing angle π/2
 * runs from  -π + 47×0.1 ≈ 1.5584 rad  to  -π + 48×0.1 ≈ 1.6584 rad
 * (channel 47), so π/2 ≈ 1.5708 lies comfortably in the interior – no
 * radial vane coincides with the +x axis.
 *
 * Collimator physics
 * ------------------
 * A radial collimator only transmits neutrons that appear to originate
 * from near r = 0.  Such rays keep the same angle as they travel outward
 * (vr > 0) and therefore never cross a channel boundary.  Rays with a
 * significant tangential component – corresponding to an apparent source
 * far from the axis – sweep through the angular channels and hit a vane.
 */

#include "unity.h"
#include "rfc-lib.h"
#include <math.h>

/* -- Helpers ---------------------------------------------------------------- */

static rfc_annulus_t make_full_annulus(double r_min, double r_max)
{
    rfc_annulus_t a;
    a.r2_min  = r_min * r_min;
    a.r2_max  = r_max * r_max;
    a.y_min   = -0.5;
    a.y_max   =  0.5;
    a.a_min   = -M_PI;
    a.a_range =  2.0 * M_PI;
    return a;
}

/* rfc_filter_hit_time does not dereference the material; zero-init is fine. */
static rfc_filter_t make_filter(double r_min, double r_max)
{
    rfc_filter_t f = {0};
    f.volume = make_full_annulus(r_min, r_max);
    return f;
}

static rfc_collimator_t make_collimator(double r_min, double r_max,
                                        double delta_angle)
{
    rfc_collimator_t c;
    c.volume      = make_full_annulus(r_min, r_max);
    c.delta_angle = delta_angle;
    return c;
}

static _class_particle make_p(double x, double y, double z,
                          double vx, double vy, double vz)
{
    _class_particle p;
    p.x = x; p.y = y; p.z = z;
    p.vx = vx; p.vy = vy; p.vz = vz;
    return p;
}

void setUp(void)    {}
void tearDown(void) {}

/* ==================================================================
 * rfc_filter_hit_time  (thin wrapper around rfc_annulus_classify)
 * ================================================================== */

/* Particle in the hollow (r < r_min), heading outward → INWARD.
 * Travels 0.4 m at 1000 m/s before reaching r_min = 0.5 m → t = 4×10⁻⁴ s. */
void test_filter_hollow_heading_out_is_inward(void)
{
    rfc_filter_t f = make_filter(0.5, 1.5);
    _class_particle   p = make_p(0.0, 0.0, 0.1, 0.0, 0.0, 1000.0);
    double t = 0.0;
    enum rfc_state pos = rfc_filter_hit_time(&f, &p, &t);
    TEST_ASSERT_EQUAL_INT(RFC_STATE_INWARD, pos);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 4e-4, t);
}

/* Particle inside the filter, heading outward → WITHIN.
 * Travels 0.75 m at 1000 m/s to r_max = 1.5 m → t = 7.5×10⁻⁴ s. */
void test_filter_inside_heading_out_is_within(void)
{
    rfc_filter_t f = make_filter(0.5, 1.5);
    _class_particle   p = make_p(0.0, 0.0, 0.75, 0.0, 0.0, 1000.0);
    double t = 0.0;
    enum rfc_state pos = rfc_filter_hit_time(&f, &p, &t);
    TEST_ASSERT_EQUAL_INT(RFC_STATE_WITHIN, pos);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 7.5e-4, t);
}

/* Particle outside (r > r_max), heading further away → OUTWARD. */
void test_filter_outside_heading_away_is_outward(void)
{
    rfc_filter_t f = make_filter(0.5, 1.5);
    _class_particle   p = make_p(2.0, 0.0, 0.0, 1000.0, 0.0, 0.0);
    double t = 0.0;
    enum rfc_state pos = rfc_filter_hit_time(&f, &p, &t);
    TEST_ASSERT_EQUAL_INT(RFC_STATE_OUTWARD, pos);
    TEST_ASSERT_TRUE(t > 1e30);
}

/* ==================================================================
 * rfc_collimator_hit_time
 * ================================================================== */

/* Radial ray from hollow: position (0.1, 0, 0), velocity (1000, 0, 0).
 * Angle stays at π/2 throughout; no channel boundary is ever crossed.
 * Expected: no vane hit (return 0). */
void test_collimator_radial_ray_from_hollow_passes(void)
{
    rfc_collimator_t c = make_collimator(0.5, 1.5, 0.1);
    _class_particle p = make_p(0.1, 0.0, 0.0, 1000.0, 0.0, 0.0);
    double t = 0.0;
    TEST_ASSERT_EQUAL_INT(0, rfc_collimator_hit_time(&c, &p, &t));
}

/* Tangential ray entering from hollow: position (0.1, 0, 0), velocity (0, 0, 1000).
 * The +z velocity is tangential at angle π/2.  As the particle moves into
 * the collimator, its angle sweeps downward toward 0 and it crosses a radial
 * vane shortly after entry.
 * Expected: vane hit (return 1). */
void test_collimator_tangential_ray_from_hollow_hits(void)
{
    rfc_collimator_t c = make_collimator(0.5, 1.5, 0.1);
    _class_particle p = make_p(0.1, 0.0, 0.0, 0.0, 0.0, 1000.0);
    double t = 0.0;
    int hit = rfc_collimator_hit_time(&c, &p, &t);
    TEST_ASSERT_EQUAL_INT(1, hit);
    TEST_ASSERT_TRUE(t > 0.0 && t < 1e30);
}

/* Ray completely missing the collimator (outside r_max, heading away).
 * Expected: return 0, t not updated to a finite value. */
void test_collimator_ray_misses_volume_returns_zero(void)
{
    rfc_collimator_t c = make_collimator(0.5, 1.5, 0.1);
    _class_particle p = make_p(2.0, 0.0, 0.0, 1000.0, 0.0, 0.0);
    double t = 0.0;
    TEST_ASSERT_EQUAL_INT(0, rfc_collimator_hit_time(&c, &p, &t));
}

/* Particle inside the collimator, moving radially outward (WITHIN):
 * position (0.8, 0, 0), velocity (1000, 0, 0).
 * Angle stays at π/2 → stays in channel 47 → no vane hit.
 * Expected: return 0. */
void test_collimator_within_radial_passes(void)
{
    rfc_collimator_t c = make_collimator(0.5, 1.5, 0.1);
    _class_particle p = make_p(0.8, 0.0, 0.0, 1000.0, 0.0, 0.0);
    double t = 0.0;
    TEST_ASSERT_EQUAL_INT(0, rfc_collimator_hit_time(&c, &p, &t));
}

/* Particle inside the collimator, moving tangentially (WITHIN):
 * position (0.8, 0, 0), velocity (0, 0, 1000).
 * Angle sweeps rapidly from π/2 toward 0; the first vane (at ≈ 1.5584 rad,
 * the lower boundary of channel 47) is crossed at t ≈ 9.9 µs.
 *
 * Note: this test exposes the a_start bug in the original code.  Without the
 * fix (using exit angle ≈ 0.56 rad instead of current angle π/2), the code
 * inspects channel 37 whose boundaries are outside r_max at the crossing
 * times, and erroneously returns 0 (PASS).  The corrected code uses
 * atan2(p->x, p->z) for WITHIN particles and correctly returns 1 (HIT). */
void test_collimator_within_tangential_hits(void)
{
    rfc_collimator_t c = make_collimator(0.5, 1.5, 0.1);
    _class_particle p = make_p(0.8, 0.0, 0.0, 0.0, 0.0, 1000.0);
    double t = 0.0;
    int hit = rfc_collimator_hit_time(&c, &p, &t);
    TEST_ASSERT_EQUAL_INT(1, hit);
    /* Vane crossing is at ≈ 9.9 µs, well before the exit at ≈ 1.27 ms. */
    TEST_ASSERT_TRUE(t > 0.0 && t < 1e-4);
}

/* Near-origin apparent source: position (0.25, 0, 0), velocity (1000, 0, 5).
 * The 0.5% tangential component corresponds to an apparent source offset of
 * only ≈ 0.001 m from the axis, far inside the acceptance radius
 * delta_angle × r_min = 0.1 × 0.5 = 0.05 m.
 * The angular drift across the collimator is ≈ 0.001 rad ≪ 0.1 rad.
 * Expected: no vane hit (return 0). */
void test_collimator_near_origin_source_passes(void)
{
    rfc_collimator_t c = make_collimator(0.5, 1.5, 0.1);
    _class_particle p = make_p(0.25, 0.0, 0.0, 1000.0, 0.0, 5.0);
    double t = 0.0;
    TEST_ASSERT_EQUAL_INT(0, rfc_collimator_hit_time(&c, &p, &t));
}

/* Far-from-origin apparent source: position (0.25, 0, 0), velocity (100, 0, 1000).
 * Tracing the ray back to where x = 0 gives an apparent origin ≈ 2.5 m away
 * in z; the large tangential component drives an angular sweep ≫ delta_angle
 * across the collimator.
 * Expected: vane hit (return 1). */
void test_collimator_far_origin_source_hits(void)
{
    rfc_collimator_t c = make_collimator(0.5, 1.5, 0.1);
    _class_particle p = make_p(0.25, 0.0, 0.0, 100.0, 0.0, 1000.0);
    double t = 0.0;
    int hit = rfc_collimator_hit_time(&c, &p, &t);
    TEST_ASSERT_EQUAL_INT(1, hit);
    TEST_ASSERT_TRUE(t > 0.0 && t < 1e30);
}

/* ==================================================================
 * main
 * ================================================================== */

int main(void)
{
    UNITY_BEGIN();

    /* rfc_filter_hit_time */
    RUN_TEST(test_filter_hollow_heading_out_is_inward);
    RUN_TEST(test_filter_inside_heading_out_is_within);
    RUN_TEST(test_filter_outside_heading_away_is_outward);

    /* rfc_collimator_hit_time */
    RUN_TEST(test_collimator_radial_ray_from_hollow_passes);
    RUN_TEST(test_collimator_tangential_ray_from_hollow_hits);
    RUN_TEST(test_collimator_ray_misses_volume_returns_zero);
    RUN_TEST(test_collimator_within_radial_passes);
    RUN_TEST(test_collimator_within_tangential_hits);
    RUN_TEST(test_collimator_near_origin_source_passes);
    RUN_TEST(test_collimator_far_origin_source_hits);

    return UNITY_END();
}
