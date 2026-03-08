/* test_annulus.c – Unity tests for rfc-lib annulus intersection and
 * classification functions.
 *
 * Geometry convention used throughout:
 *   Angles measured as atan2(x, z) from the +z axis.
 *   Angular range stored as (a_min, a_range); the sector covers
 *   [a_min, a_min + a_range].
 *   A full-revolution annulus uses a_range = 2*M_PI (or larger). */

#include "unity.h"
#include "rfc-lib.h"
#include <math.h>

/* ---- helpers ---- */

/* Full-revolution annulus – only radial and y bounds matter. */
static rfc_annulus_t make_annulus(double r_min, double r_max,
                                  double y_min, double y_max) {
    rfc_annulus_t a;
    a.r2_min  = r_min * r_min;
    a.r2_max  = r_max * r_max;
    a.y_min   = y_min;
    a.y_max   = y_max;
    a.a_min   = -M_PI;
    a.a_range = 2 * M_PI;
    return a;
}

static _class_particle make_particle(double x, double y, double z,
                                double vx, double vy, double vz) {
    _class_particle p;
    p.x = x; p.y = y; p.z = z;
    p.vx = vx; p.vy = vy; p.vz = vz;
    return p;
}

void setUp(void)    {}
void tearDown(void) {}

/* ==================================================================
 * rfc_is_inside_annulus
 * ================================================================== */

void test_inside_annulus_point_clearly_inside(void) {
    rfc_annulus_t ann = make_annulus(1000.0, 1000.15, -0.5, 0.5);
    _class_particle p = make_particle(0.0, 0.0, 1000.05, 0.0, 0.0, 1.0);
    TEST_ASSERT_EQUAL_INT(1, rfc_is_inside_annulus(&ann, &p));
}

void test_inside_annulus_point_in_hollow(void) {
    rfc_annulus_t ann = make_annulus(1000.0, 1000.15, -0.5, 0.5);
    _class_particle p = make_particle(0.0, 0.0, 999.999, 0.0, 0.0, 1.0);
    TEST_ASSERT_EQUAL_INT(0, rfc_is_inside_annulus(&ann, &p));
}

void test_inside_annulus_point_outside_r_max(void) {
    rfc_annulus_t ann = make_annulus(1000.0, 1000.15, -0.5, 0.5);
    _class_particle p = make_particle(0.0, 0.0, 1100.0, 0.0, 0.0, 1.0);
    TEST_ASSERT_EQUAL_INT(0, rfc_is_inside_annulus(&ann, &p));
}

void test_inside_annulus_point_outside_y_range(void) {
    rfc_annulus_t ann = make_annulus(1000.0, 1000.15, -0.5, 0.5);
    _class_particle p = make_particle(0.0, 1.0, 1000.05, 0.0, 0.0, 1.0);
    TEST_ASSERT_EQUAL_INT(0, rfc_is_inside_annulus(&ann, &p));
}

void test_inside_annulus_point_outside_angular_sector(void) {
    rfc_annulus_t ann = make_annulus(1000.0, 1000.15, -0.5, 0.5);
    ann.a_min   = -0.1;
    ann.a_range =  0.2; /* sector covers [-0.1, 0.1] rad */
    /* x=500, z=866 ≈ angle 30° = 0.524 rad, outside sector */
    _class_particle p = make_particle(500.0, 0.0, 866.0, 0.0, 0.0, 1.0);
    TEST_ASSERT_EQUAL_INT(0, rfc_is_inside_annulus(&ann, &p));
}

/* ==================================================================
 * rfc_annulus_classify – return value
 * ================================================================== */

void test_classify_fixed_particle(void) {
    rfc_annulus_t ann = make_annulus(1000.0, 1000.15, -0.5, 0.5);
    _class_particle p = make_particle(0.0, 0.0, 999.999, 0.0, 0.0, 0.0);
    double t;
    enum rfc_state pos = rfc_annulus_classify(&ann, &p, &t);
    TEST_ASSERT_EQUAL_INT(RFC_STATE_FIXED, pos);
}

void test_classify_within_heading_outward(void) {
    rfc_annulus_t ann = make_annulus(1000.0, 1000.15, -0.5, 0.5);
    _class_particle p = make_particle(0.0, 0.0, 1000.05, 0.0, 0.0, 1000.0);
    double t;
    enum rfc_state pos = rfc_annulus_classify(&ann, &p, &t);
    TEST_ASSERT_EQUAL_INT(RFC_STATE_WITHIN, pos);
}

void test_classify_inward_from_hollow(void) {
    rfc_annulus_t ann = make_annulus(1000.0, 1000.15, -0.5, 0.5);
    _class_particle p = make_particle(0.0, 0.0, 999.999, 0.0, 0.0, 1000.0);
    double t;
    enum rfc_state pos = rfc_annulus_classify(&ann, &p, &t);
    TEST_ASSERT_EQUAL_INT(RFC_STATE_INWARD, pos);
}

void test_classify_inward_from_outside_r_max(void) {
    rfc_annulus_t ann = make_annulus(1000.0, 1000.15, -0.5, 0.5);
    _class_particle p = make_particle(0.0, 0.0, 1100.0, 0.0, 0.0, -1000.0);
    double t;
    enum rfc_state pos = rfc_annulus_classify(&ann, &p, &t);
    TEST_ASSERT_EQUAL_INT(RFC_STATE_INWARD, pos);
}

void test_classify_outward_never_enters(void) {
    rfc_annulus_t ann = make_annulus(1000.0, 1000.15, -0.5, 0.5);
    /* Particle at r > r_max heading away from origin (+z direction). */
    _class_particle p = make_particle(0.0, 0.0, 1100.0, 0.0, 0.0, 1000.0);
    double t;
    enum rfc_state pos = rfc_annulus_classify(&ann, &p, &t);
    TEST_ASSERT_EQUAL_INT(RFC_STATE_OUTWARD, pos);
}

/* ==================================================================
 * rfc_annulus_classify – intersection times
 * ================================================================== */

/* Bug-report scenario A: neutron at (0, 0, 999.999) heading in +z with
 * speed 1000 m/s.  Filter r_min=1000 m, r_max=1000.15 m.
 * Expected: INWARD, t = (1000 - 999.999)/1000 = 1e-6 s. */
void test_classify_scenario_a_entry_time(void) {
    rfc_annulus_t ann = make_annulus(1000.0, 1000.15, -0.5, 0.5);
    _class_particle p = make_particle(0.0, 0.0, 999.999, 0.0, 0.0, 1000.0);
    double t;
    enum rfc_state pos = rfc_annulus_classify(&ann, &p, &t);
    TEST_ASSERT_EQUAL_INT(RFC_STATE_INWARD, pos);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 1e-6, t);
}

/* Bug-report scenario B: back-scattered neutron inside filter at
 * (-0.002041, -0.036377, 1000.035359) with velocity
 * (42.118979, -360.481083, -948.723229).
 * Expected: WITHIN, t ≈ 3.727e-5 s (exits hollow at r_min). */
void test_classify_scenario_b_exit_time(void) {
    rfc_annulus_t ann = make_annulus(1000.0, 1000.15, -0.5, 0.5);
    _class_particle p = make_particle(-0.002041, -0.036377, 1000.035359,
                                  42.118979, -360.481083, -948.723229);
    double t;
    enum rfc_state pos = rfc_annulus_classify(&ann, &p, &t);
    TEST_ASSERT_EQUAL_INT(RFC_STATE_WITHIN, pos);
    TEST_ASSERT_DOUBLE_WITHIN(1e-7, 3.727e-5, t);
}

/* Simple WITHIN: particle at (0, 0, 1000.05) heading outward at 1000 m/s.
 * t_exit = (1000.15 - 1000.05)/1000 = 1e-4 s. */
void test_classify_within_exit_time(void) {
    rfc_annulus_t ann = make_annulus(1000.0, 1000.15, -0.5, 0.5);
    _class_particle p = make_particle(0.0, 0.0, 1000.05, 0.0, 0.0, 1000.0);
    double t;
    enum rfc_state pos = rfc_annulus_classify(&ann, &p, &t);
    TEST_ASSERT_EQUAL_INT(RFC_STATE_WITHIN, pos);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 1e-4, t);
}

/* Simple INWARD from outside r_max: particle at (0, 0, 1100) heading in
 * -z at 1000 m/s.  t_entry = (1100 - 1000.15)/1000 = 9.985e-2 s. */
void test_classify_inward_entry_time_from_outside(void) {
    rfc_annulus_t ann = make_annulus(1000.0, 1000.15, -0.5, 0.5);
    _class_particle p = make_particle(0.0, 0.0, 1100.0, 0.0, 0.0, -1000.0);
    double t;
    enum rfc_state pos = rfc_annulus_classify(&ann, &p, &t);
    TEST_ASSERT_EQUAL_INT(RFC_STATE_INWARD, pos);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 9.985e-2, t);
}

/* ==================================================================
 * rfc_cylindrical_surface_intersection_time
 * ================================================================== */

void test_cyl_particle_inside_heading_outward(void) {
    rfc_annulus_t ann = make_annulus(1000.0, 1000.15, -0.5, 0.5);
    _class_particle p = make_particle(0.0, 0.0, 1000.05, 0.0, 0.0, 1000.0);
    /* should hit r_max = 1000.15 at t = 0.1/1000 = 1e-4 s */
    double t = rfc_cylindrical_surface_intersection_time(ann.r2_max, &ann, &p);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 1e-4, t);
}

void test_cyl_particle_in_hollow_heading_outward(void) {
    rfc_annulus_t ann = make_annulus(1000.0, 1000.15, -0.5, 0.5);
    _class_particle p = make_particle(0.0, 0.0, 999.999, 0.0, 0.0, 1000.0);
    /* should hit r_min = 1000 at t = 0.001/1000 = 1e-6 s */
    double t = rfc_cylindrical_surface_intersection_time(ann.r2_min, &ann, &p);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 1e-6, t);
}

void test_cyl_parallel_path_returns_infinity(void) {
    rfc_annulus_t ann = make_annulus(1000.0, 1000.15, -0.5, 0.5);
    /* Particle moving purely in y – never changes r. */
    _class_particle p = make_particle(0.0, 0.0, 1000.05, 0.0, 1000.0, 0.0);
    double t = rfc_cylindrical_surface_intersection_time(ann.r2_max, &ann, &p);
    TEST_ASSERT_TRUE(t > 1e30);
}

/* ==================================================================
 * rfc_endcap_surface_intersection_time
 * ================================================================== */

void test_endcap_heading_up_hits_y_max(void) {
    rfc_annulus_t ann = make_annulus(1000.0, 1000.15, -0.5, 0.5);
    /* Particle inside at y=-0.3, vy=+1000 → t = (0.5-(-0.3))/1000 = 8e-4 s */
    _class_particle p = make_particle(0.0, -0.3, 1000.05, 0.0, 1000.0, 0.0);
    double t = rfc_endcap_surface_intersection_time(ann.y_max, &ann, &p);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 8e-4, t);
}

void test_endcap_heading_down_hits_y_min(void) {
    rfc_annulus_t ann = make_annulus(1000.0, 1000.15, -0.5, 0.5);
    /* Particle inside at y=0.3, vy=-1000 → t = (-0.5-0.3)/(-1000) = 8e-4 s */
    _class_particle p = make_particle(0.0, 0.3, 1000.05, 0.0, -1000.0, 0.0);
    double t = rfc_endcap_surface_intersection_time(ann.y_min, &ann, &p);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 8e-4, t);
}

void test_endcap_zero_vy_returns_infinity(void) {
    rfc_annulus_t ann = make_annulus(1000.0, 1000.15, -0.5, 0.5);
    _class_particle p = make_particle(0.0, 0.0, 1000.05, 0.0, 0.0, 1000.0);
    double t_top = rfc_endcap_surface_intersection_time(ann.y_max, &ann, &p);
    double t_bot = rfc_endcap_surface_intersection_time(ann.y_min, &ann, &p);
    TEST_ASSERT_TRUE(t_top > 1e30);
    TEST_ASSERT_TRUE(t_bot > 1e30);
}

/* ==================================================================
 * rfc_radial_surface_intersection_time
 * ================================================================== */

/* Particle at (x=0, y=0, z=100) heading in +x at 1000 m/s.
 * Annulus r=[50,150].  Boundary at a=pi/4 has direction (sin(pi/4), cos(pi/4)).
 * Plane normal: (cos(pi/4), -sin(pi/4)).
 * d = 0*cos(pi/4) + 100*(-sin(pi/4)) = -70.71.  v_n = 1000*cos(pi/4) = 707.1.
 * t = 70.71/707.1 = 0.1 s.  Intersection: (100, 100), r=141.4 in [50,150]. s>0. */
void test_radial_correct_half_returns_positive_t(void) {
    rfc_annulus_t ann = make_annulus(50.0, 150.0, -100.0, 100.0);
    ann.a_min   = -0.1;
    ann.a_range =  M_PI / 4.0 + 0.1; /* a_max = pi/4 */
    double a_max = ann.a_min + ann.a_range; /* pi/4 */
    _class_particle p = make_particle(0.0, 0.0, 100.0, 1000.0, 0.0, 0.0);
    double t = rfc_radial_surface_intersection_time(a_max, &ann, &p);
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, 0.1, t);
}

/* Particle heading toward the OPPOSITE half of the boundary plane.
 * When vz < 0 (heading back through the origin), the line-plane intersection
 * lies on the anti-ray (s < 0).  The function must return INFINITY, not a
 * spurious positive time.
 *
 * This pins the correct behaviour for the half-plane vs full-plane issue
 * identified in the .comp file.  If this test fails, rfc_radial_surface_
 * intersection_time needs the same s>=0 guard added to it. */
/* Particle at (150, 0, -50) heading in -x at 1000 m/s.
 * Same annulus r=[50,150] as the correct-half test, same a_boundary=pi/4.
 * The full-plane intersection is at t=0.2 s, (x=-50, z=-50), r=70.7 in [50,150].
 * But atan2(-50,-50) = -3*pi/4 ≠ pi/4: this is on the ANTI-ray (s=-70.7 < 0).
 * Without the s>=0 guard the function returns 0.2 (BUG); correct answer is INFINITY. */
void test_radial_anti_half_returns_infinity(void) {
    rfc_annulus_t ann = make_annulus(50.0, 150.0, -100.0, 100.0);
    ann.a_min   = -0.1;
    ann.a_range =  M_PI / 4.0 + 0.1; /* a_max = pi/4 */
    double a_boundary = ann.a_min + ann.a_range; /* pi/4 */
    /* Heading toward the anti-half: intersection at (-50,-50), angle=-3*pi/4. */
    _class_particle p = make_particle(150.0, 0.0, -50.0, -1000.0, 0.0, 0.0);
    double t = rfc_radial_surface_intersection_time(a_boundary, &ann, &p);
    TEST_ASSERT_TRUE(t > 1e30); /* must be INFINITY, not 0.2 */
}

/* ==================================================================
 * rfc_cylindrical_surface_intersection_time – near-surface cancellation
 * ==================================================================
 *
 * When a particle is extremely close to the cylinder surface (delta ≈ 1e-12 m)
 * the textbook quadratic form  ts = (-x_v + sqrt(disc)) / v_v  subtracts two
 * nearly-equal large numbers.  On 64-bit hardware this catastrophic cancellation
 * can produce a negative ts, which the function then discards, returning INFINITY.
 * The compensated form  ts = (r2 - x_x) / (x_v + sqrt(disc))  uses the small
 * numerator (r2 - x_x) directly and stays non-negative for inside-→-out crossings.
 */

/* Particle 1e-9 m inside r_max, heading radially outward.
 * True exit time = 1e-9 / 500 = 2e-12 s.
 * The standard formula loses precision here; the compensated form must return
 * a positive time within 10% of the true value. */
void test_cyl_near_rmax_inside_exits_soon(void) {
    const double r_max = 1000.15;
    const double delta = 1e-9; /* metres inside the surface */
    rfc_annulus_t ann = make_annulus(999.0, r_max, -1.0, 1.0);
    /* Place particle just inside r_max, moving purely radially outward */
    _class_particle p = make_particle(0.0, 0.0, r_max - delta, 0.0, 0.0, 500.0);
    double t = rfc_cylindrical_surface_intersection_time(ann.r2_max, &ann, &p);
    const double t_true = delta / 500.0; /* 2e-12 s */
    TEST_ASSERT_FALSE_MESSAGE(t > 1e30, "Expected a finite exit time, got INFINITY (cancellation bug)");
    TEST_ASSERT_DOUBLE_WITHIN(0.1 * t_true, t_true, t);
}

/* Particle 1e-9 m outside r_min (in hollow), heading radially outward into filter.
 * True entry time ≈ 1e-9 / 500 = 2e-12 s. */
void test_cyl_near_rmin_outside_enters_soon(void) {
    const double r_min = 1000.0;
    const double delta = 1e-9;
    rfc_annulus_t ann = make_annulus(r_min, 1000.15, -1.0, 1.0);
    _class_particle p = make_particle(0.0, 0.0, r_min - delta, 0.0, 0.0, 500.0);
    double t = rfc_cylindrical_surface_intersection_time(ann.r2_min, &ann, &p);
    const double t_true = delta / 500.0;
    TEST_ASSERT_FALSE_MESSAGE(t > 1e30, "Expected a finite entry time, got INFINITY (cancellation bug)");
    TEST_ASSERT_DOUBLE_WITHIN(0.1 * t_true, t_true, t);
}

/* ==================================================================
 * main
 * ================================================================== */

int main(void) {
    UNITY_BEGIN();

    /* rfc_is_inside_annulus */
    RUN_TEST(test_inside_annulus_point_clearly_inside);
    RUN_TEST(test_inside_annulus_point_in_hollow);
    RUN_TEST(test_inside_annulus_point_outside_r_max);
    RUN_TEST(test_inside_annulus_point_outside_y_range);
    RUN_TEST(test_inside_annulus_point_outside_angular_sector);

    /* rfc_annulus_classify – return value */
    RUN_TEST(test_classify_fixed_particle);
    RUN_TEST(test_classify_within_heading_outward);
    RUN_TEST(test_classify_inward_from_hollow);
    RUN_TEST(test_classify_inward_from_outside_r_max);
    RUN_TEST(test_classify_outward_never_enters);

    /* rfc_annulus_classify – intersection times */
    RUN_TEST(test_classify_scenario_a_entry_time);
    RUN_TEST(test_classify_scenario_b_exit_time);
    RUN_TEST(test_classify_within_exit_time);
    RUN_TEST(test_classify_inward_entry_time_from_outside);

    /* rfc_cylindrical_surface_intersection_time */
    RUN_TEST(test_cyl_particle_inside_heading_outward);
    RUN_TEST(test_cyl_particle_in_hollow_heading_outward);
    RUN_TEST(test_cyl_parallel_path_returns_infinity);
    RUN_TEST(test_cyl_near_rmax_inside_exits_soon);
    RUN_TEST(test_cyl_near_rmin_outside_enters_soon);

    /* rfc_endcap_surface_intersection_time */
    RUN_TEST(test_endcap_heading_up_hits_y_max);
    RUN_TEST(test_endcap_heading_down_hits_y_min);
    RUN_TEST(test_endcap_zero_vy_returns_infinity);

    /* rfc_radial_surface_intersection_time */
    RUN_TEST(test_radial_correct_half_returns_positive_t);
    RUN_TEST(test_radial_anti_half_returns_infinity);

    return UNITY_END();
}
