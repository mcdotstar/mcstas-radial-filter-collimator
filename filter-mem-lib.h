#ifndef FILTER_GEN_MEM
#define FILTER_GEN_MEM
#include <math.h>
#ifndef WIN32
  #include "NCrystal/ncrystal.h"
#else
  #include "NCrystal\\ncrystal.h"
#endif
#include "stdio.h"
#include "stdlib.h"

#ifndef MCSTAS
#define UNSET nan("0x6E6F74736574")
int nans_match(double, double);
int is_unset(double);
int is_valid(double);
int is_set(double);
int all_unset(int n, ...);
int all_set(int n, ...);
int any_unset(int n, ...);
int any_set(int n, ...);
#endif /* MCSTAS */

enum filter_mem_type {
  FILTER_MEM_TYPE_UNKNOWN,    // Provided distribution has unknown independent axis
  FILTER_MEM_TYPE_ENERGY,     // Provided distribution is in energy, meV
  FILTER_MEM_TYPE_WAVELENGTH, // Provided wavelength is in wavelength, angstrom
  FILTER_MEM_TYPE_WAVENUMBER, // Provided wavelength is in wavenumber, 1/angstrom
};
enum filter_mem_mode {
  FILTER_MEM_MODE_SET,
  FILTER_MEM_MODE_ADD,
  FILTER_MEM_MODE_MUL,
};

void filter_mem_options(double emin, double estep, double lmin, double lstep, double kmin, double kstep, double * omin, double * ostep, char * opts, enum filter_mem_mode * mode, enum filter_mem_type * type, int * verbose);

void filter_mem_geometry_axis(double, double*, double*);

int filter_mem_geometry(double width, double * wmin, double * wmax, double height, double * hmin, double * hmax);

/** \brief Construct an array of transmission values for use with Filter_mem using NCrystal
 *
 * \param type One of the enumerated type values indicating what the independent variable 'x' represents
 * \param independent_min The minimum value along the independent variable axis
 * \param independent_step The distance between calculated transmission points along the independent variable axis
 * \param independent_count The number of points to calculate along the independent variable axis
 * \param cfg The NCrystal configuration string used to specify the material
 * \param thickness_cm Filter thickness in cm
 * */
double * filter_mem_transmission(enum filter_mem_type type, double independent_min, double independent_step, int independent_count, const char * cfg, double thickness_cm);

#endif
