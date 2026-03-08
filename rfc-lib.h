#ifndef RFC_LIB_H
#define RFC_LIB_H

#include <math.h>

#ifndef WIN32
  #include "NCrystal/ncrystal.h"
#else
  #include "NCrystal\\ncrystal.h"
#endif
#include "stdio.h"
#include "stdlib.h"
#ifndef NCMCERR2
  /* consistent/convenient error reporting */
  #define NCMCERR2(compname, msg)                                                                                                                              \
  do {                                                                                                                                                       \
  fprintf (stderr, "\nNCrystal: %s: ERROR: %s\n\n", compname, msg);                                                                                        \
  exit (1);                                                                                                                                                \
  } while (0)
#endif

static int radial_filter_collimator_ncrystal_reported_version = 0;

#ifndef NCMCERR
/* more convenient form (only works in TRACE section, not in SHARE functions) */
#define NCMCERR(msg) NCMCERR2 (NAME_CURRENT_COMP, msg)
#endif


#ifndef MCSTAS
struct _struct_particle {
  double x;
  double y;
  double z;
  double vx;
  double vy;
  double vz;
};
typedef struct _struct_particle _class_particle;
#endif /* MCSTAS */

/**\brief A volume defined by two concentric cylinders (an annulus) around the y-axis, with optional bounds on y and angle around the y-axis.
The annulus is defined by
  - the inner and outer squared radius (distance from the y-axis),
  - the minimum and maximum y coordinate,
  - and the lower bound and range of angle of the volume around the y-axis.

Angles are measured in radians, with 0 corresponding to the positive z-axis, and increasing counterclockwise when looking down the positive y-axis.
The minimum angle plus range can unambiguously define the angular extent of the annulus, even if the range is greater than 2π.
*/
typedef struct {
  double r2_min; // inner squared radius (distance from y-axis) -- m^2
  double r2_max; // outer squared radius (distance from y-axis) -- m^2
  double y_min;  // minimum y coordinate -- m
  double y_max;  // maximum y coordinate -- m
  double a_min;  // lower bound angle of volume around y-axis -- rad
  double a_range; // angular range of volume around y-axis -- rad
} rfc_annulus_t;

typedef struct {
  int is_vacuum; // 1 if the material is vacuum, 0 otherwise
  double density_factor;
  double inv_density_factor;
  ncrystal_scatter_t scat;
  ncrystal_process_t proc_scat, proc_abs;
  int proc_scat_isoriented;
  int absmode;
} rfc_material_t;

typedef struct {
  rfc_material_t material;
  rfc_annulus_t volume;
} rfc_filter_t;

typedef struct {
  double delta_angle; // angular width of each collimator channel -- rad
  rfc_annulus_t volume; // the collimator volume
} rfc_collimator_t;

enum rfc_state {
  RFC_STATE_UNKNOWN, // Particle position is unknown
  RFC_STATE_FIXED,   // Particle is stopped, which should not be possible
  RFC_STATE_INWARD,  // Particle is outside the volume, but will enter it in the future
  RFC_STATE_WITHIN,  // Particle is within the volume, and will exit in the future
  RFC_STATE_OUTWARD, // Particle is outside the volume, and will never enter it in the future
};

int rfc_is_inside_annulus(const rfc_annulus_t * annulus, const _class_particle * p);
enum rfc_state rfc_annulus_classify (const rfc_annulus_t * annulus, const _class_particle * p, double * t);
double rfc_cylindrical_surface_intersection_time(double r2, const rfc_annulus_t * params, const _class_particle * p);
double rfc_radial_surface_intersection_time(double a_boundary, const rfc_annulus_t * params, const _class_particle * p);
double rfc_endcap_surface_intersection_time(double y_boundary, const rfc_annulus_t * params, const _class_particle * p);

enum rfc_state rfc_filter_hit_time(const rfc_filter_t * filter, const _class_particle * p, double * t);
enum rfc_state rfc_collimator_hit_time(const rfc_collimator_t * collimator, const _class_particle * p, double * t);
double rfc_filter_exit_time(const rfc_filter_t * filter, const _class_particle * p);


#endif // RFC_LIB_H