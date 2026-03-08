#ifndef RFC_LIB_H
// Guard against trying to include the header _after_ McCode's %include directive has inserted this source
#include "rfc-lib.h"
#endif

double rfc_cylindrical_surface_intersection_time(const double r2, const rfc_annulus_t * params, const _class_particle * p) {
  const double v_v = p->vx * p->vx + p->vz * p->vz; // v dot v
  const double x_v = p->x * p->vx + p->z * p->vz;   // x dot v
  const double x_x = p->x * p->x + p->z * p->z;     // x dot x
  const double discriminant = x_v * x_v - v_v * (x_x - r2);
  if (discriminant < 0) {
    return INFINITY; // No intersection
  }
  const double part = sqrt(discriminant);
  /* Compensated (numerically stable) quadratic roots.
   *
   * The textbook form  ts[1] = (-x_v + part) / v_v  subtracts two nearly-equal
   * large numbers when the particle is close to the surface (x_x ≈ r2, so
   * part ≈ x_v).  Catastrophic cancellation can make ts[1] come out negative,
   * which then gets discarded, returning INFINITY instead of a tiny positive
   * exit time – causing a spurious ABSORB inside the while-loop.
   *
   * The equivalent forms below avoid cancellation:
   *   ts[1] = (r2 - x_x) / (x_v + part)  — numerator is small but exact
   *   ts[0] = (x_x - r2) / (part - x_v)  — stable for outside-→-inside case
   *
   * Both are non-negative whenever the particle is actually inside / outside
   * the cylinder respectively, so they cannot produce spurious negative roots. */
  double ts[2] = {(-x_v - part) / v_v, (-x_v + part) / v_v};
  if (x_v + part > 0) ts[1] = (r2 - x_x) / (x_v + part); /* exit  time (inside→out) */
  if (part - x_v > 0) ts[0] = (x_x - r2) / (part - x_v); /* entry time (outside→in) */

  // We want the smallest positive time when the particle is within the height of the cylinder
  double t_cylinder = INFINITY;

  // First check whether the particle will intersect the end caps of the cylinder before it intersects the infinite cylinder
  for (int i = 0; i < 2; ++i) {
    // Check angle boundaries for this intersection time
    const double angle = atan2(p->x + p->vx * ts[i], p->z + p->vz * ts[i]);
    if (angle < params->a_min || angle > params->a_min + params->a_range) {
      ts[i] = INFINITY; // This intersection is not valid, so ignore it
    }
    const double y = p->y + p->vy * ts[i];
    if (y < params->y_min || y > params->y_max) {
      ts[i] = INFINITY; // This intersection is not valid, so ignore it
    }
    if (0 < ts[i] && ts[i] < t_cylinder) {
      t_cylinder = ts[i];
    }
  }
  // Either a valid intersection time or INFINITY if no future intersection
  return t_cylinder;
}
double rfc_radial_surface_intersection_time(const double a_boundary, const rfc_annulus_t * params, const _class_particle * p) {
  // Half-plane through y-axis at angle a_boundary has normal vector (sin(a_boundary), 0, -cos(a_boundary))
  const double nx = cos(a_boundary);
  const double nz = -sin(a_boundary);
  const double d = p->x * nx + p->z * nz; // Distance from plane: d = x*nx + z*nz
  const double v_n = p->vx * nx + p->vz * nz; // Velocity toward plane: v_n = vx*nx + vz*nz
  const double t_boundary = -d / v_n;
  if (0 < t_boundary && t_boundary < INFINITY) {
    const double x = p->x + p->vx * t_boundary;
    const double y = p->y + p->vy * t_boundary;
    const double z = p->z + p->vz * t_boundary;
    /* s is the signed distance along the boundary half-ray from the origin.
     * The boundary direction is (sin(a), cos(a)); s < 0 means the
     * intersection is on the anti-ray (wrong side of origin). */
    const double s = x * sin(a_boundary) + z * cos(a_boundary);
    if (s < 0) return INFINITY;
    const double r2 = x * x + z * z;
    if (r2 >= params->r2_min && r2 <= params->r2_max && y >= params->y_min && y <= params->y_max) {
      return t_boundary; // Valid intersection with angle boundary
    }
  }
  return INFINITY; // No valid intersection with angle boundary
}
double rfc_endcap_surface_intersection_time(const double y_boundary, const rfc_annulus_t * params, const _class_particle * p) {
  const double t_boundary = (y_boundary - p->y) / p->vy;
  if (0 < t_boundary && t_boundary < INFINITY) {
    const double x = p->x + p->vx * t_boundary;
    const double z = p->z + p->vz * t_boundary;
    const double r2 = x * x + z * z;
    if (r2 >= params->r2_min && r2 <= params->r2_max) {
      const double angle = atan2(x, z);
      if (angle >= params->a_min && angle <= params->a_min + params->a_range) {
        return t_boundary; // Valid intersection with end cap
      }
    }
  }
  return INFINITY; // No valid intersection with end cap
}

int rfc_is_inside_annulus(const rfc_annulus_t * annulus, const _class_particle * p) {
  const double r2 = p->x * p->x + p->z * p->z;
  const double y = p->y;
  const double a = atan2(p->x, p->z); // angle around y-axis
  return (r2 >= annulus->r2_min && r2 <= annulus->r2_max && y >= annulus->y_min && y <= annulus->y_max && a >= annulus->a_min && a <= annulus->a_min + annulus->a_range);
}

enum rfc_state rfc_annulus_classify (const rfc_annulus_t * annulus, const _class_particle * p, double * t) {
  *t = INFINITY; // Default to INFINITY, will be updated with the minimum intersection time
  if (p->vx == 0 && p->vy == 0 && p->vz == 0) {
    return RFC_STATE_FIXED; // Particle is stopped, which should not be possible
  }
  double t_min = INFINITY;
  t_min = fmin(t_min, rfc_cylindrical_surface_intersection_time(annulus->r2_min, annulus, p));
  t_min = fmin(t_min, rfc_cylindrical_surface_intersection_time(annulus->r2_max, annulus, p));
  if (fabs(annulus->a_range) < 2*M_PI) {
    t_min = fmin(t_min, rfc_radial_surface_intersection_time(annulus->a_min, annulus, p));
    t_min = fmin(t_min, rfc_radial_surface_intersection_time(annulus->a_min + annulus->a_range, annulus, p));
  }
  t_min = fmin(t_min, rfc_endcap_surface_intersection_time(annulus->y_min, annulus, p));
  t_min = fmin(t_min, rfc_endcap_surface_intersection_time(annulus->y_max, annulus, p));

  *t = t_min; // Update the output parameter with the minimum intersection time

  if (rfc_is_inside_annulus(annulus, p)) {
    return RFC_STATE_WITHIN;
  }
  if (*t < INFINITY) {
    return RFC_STATE_INWARD; // Will enter the volume in the future
  }
  if (*t == INFINITY){
    return RFC_STATE_OUTWARD; // Will never enter the volume
  }
  return RFC_STATE_UNKNOWN; // Should not happen
}

enum rfc_state rfc_filter_hit_time(const rfc_filter_t * filter, const _class_particle * p, double * t) {
  return rfc_annulus_classify(&filter->volume, p, t);
}

double rfc_filter_exit_time(const rfc_filter_t * filter, const _class_particle * p) {
  double t;
  const enum rfc_state pos = rfc_annulus_classify(&filter->volume, p, &t);
  if (pos == RFC_STATE_WITHIN) {
    // The particle is currently within the filter volume *OR ON ITS SURFACE*, and we want to know when it will exit
    // But the classify function only allows _future_ exit times, which excludes t=0 as a valid solution
    // if the particle is exactly on the surface at t=0, so we need to check for that case separately
    if (t == INFINITY) {
      // The particle is currently on the surface and will not enter the volume in the future, so we need to check if it is currently within the volume or not
      return 0;
    }
    return t; // Time until the particle exits the filter volume
  }
  return INFINITY; // Particle is not within the filter volume, so no exit time
}

enum rfc_state rfc_collimator_hit_time(const rfc_collimator_t * collimator, const _class_particle * p, double * t) {
  const enum rfc_state type = rfc_annulus_classify(&collimator->volume, p, t);
  if (type != RFC_STATE_INWARD && type != RFC_STATE_WITHIN) {
    return RFC_STATE_OUTWARD; // No hit if the particle is not entering or within the collimator volume
  }
  // decide if/when the particle hits a collimator vane:
  // For WITHIN, use the current position; for INWARD, use the entry position.
  const double a_start = (type == RFC_STATE_WITHIN)
      ? atan2(p->x, p->z)
      : atan2(p->x + p->vx * *t, p->z + p->vz * *t);

  const double entry_channel = floor((a_start - collimator->volume.a_min) / collimator->delta_angle);
  if (collimator->volume.a_min + entry_channel * collimator->delta_angle == a_start) {
    printf("Warning: particle from x=(%f, %f, %f) v=(%f, %f, %f) is exactly on the boundary between two channels at time t=%g\n",
      p->x, p->y, p->z, p->vx, p->vy, p->vz, *t);
    // exactly on the boundary between two channels at time t, which is a vane -> hit
    return RFC_STATE_WITHIN;
  }
  // check if the particle hits a vane
  const double t_lower = rfc_radial_surface_intersection_time(collimator->volume.a_min + entry_channel * collimator->delta_angle, &collimator->volume, p);
  const double t_upper = rfc_radial_surface_intersection_time(collimator->volume.a_min + (entry_channel + 1) * collimator->delta_angle, &collimator->volume, p);
  if (t_lower < INFINITY || t_upper < INFINITY) {
    *t = (t_lower < t_upper) ? t_lower : t_upper; // Set t to the smaller of the two intersection times
    return RFC_STATE_INWARD; // hit a vane in the future
  }
  return RFC_STATE_OUTWARD; // No hit if the particle does not intersect any vanes
}