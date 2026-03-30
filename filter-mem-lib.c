// Unusual guard needed for use in McCode runtime
#ifndef FILTER_GEN_MEM
#include "filter-mem-lib.h"
#endif

void filter_mem_options(double emin, double estep, double lmin, double lstep, double kmin, double kstep, double * omin, double * ostep, char * opts, enum filter_mem_mode * mode, enum filter_mem_type * type, int * verbose){
  if (all_unset(6, emin, estep, lmin, lstep, kmin, kstep)){
    fprintf(stderr, "No values set: energy=%f, %f, wavelength=%f, %f, wavenumber=%f, %f\n", emin, estep, lmin, lstep, kmin, kstep);
    return;
  }
  int energy = all_set(2, emin, estep);
  int wavelength = all_set(2, lmin, lstep);
  int wavenumber = all_set(2, kmin, kstep);
  if (energy + wavelength + wavenumber != 1){
    fprintf(stderr, "Wrong number of values set: energy=%d, wavelength=%d, wavenumber%d\n", energy, wavelength, wavenumber);
    return;
  }
  if (energy){
    *type = FILTER_MEM_TYPE_ENERGY;
    *omin = emin;
    *ostep = estep;
  } else if (wavelength){
    *type = FILTER_MEM_TYPE_WAVELENGTH;
    *omin = lmin;
    *ostep = lstep;
    fprintf(stderr, "Set type=%d, omin=%f, ostep=%f\n", *type, *omin, *ostep);
  } else if (wavenumber){
    *type = FILTER_MEM_TYPE_WAVENUMBER;
    *omin = kmin;
    *ostep = kstep;
  }  

  if (!opts || !strlen(opts)) return;

  if (strstr(opts, "set")){
    *mode = FILTER_MEM_MODE_SET;
  } else if (strstr(opts, "add")){
    *mode = FILTER_MEM_MODE_ADD;
  } else if (strstr(opts, "multiply")){
    *mode = FILTER_MEM_MODE_MUL;
  }

  if (strstr(opts, "verbose")){
    *verbose = 1;
  }

}


void filter_mem_geometry_axis(double length, double * minimum, double * maximum){
  if (all_unset(2, *minimum, *maximum)){
    *maximum = length/2;
    *minimum = -(*maximum);
  } else if (is_unset(*minimum)){
    *minimum = *maximum - length;
  } else if (is_unset(*maximum)){
    *maximum = *minimum + length;
  }
}


int filter_mem_geometry(double width, double * wmin, double * wmax, double height, double * hmin, double * hmax){
  if (all_unset(6, width, *wmin, *wmax, height, *hmin, *hmax)){
    return 0;
  }
  if (all_unset(3, width, *wmin, *wmax) ||  all_unset(3, height, *hmin, *hmax)){
    return -1;
  }
  filter_mem_geometry_axis(width, wmin, wmax);
  filter_mem_geometry_axis(height, hmin, hmax);
  return 1;
}


double * filter_mem_transmission(enum filter_mem_type type, double independent_min, double independent_step, int independent_count, const char * cfg, const double thickness_cm){
  // We need the number density of the configuration-specified material
  ncrystal_info_t info = ncrystal_create_info (cfg);
  double numberdensity = ncrystal_info_getnumberdensity (info);
  ncrystal_unref (&info);

  // Setup scattering & absorption processes
  ncrystal_process_t proc_scat = ncrystal_cast_scat2proc (ncrystal_create_scatter (cfg));
  ncrystal_process_t proc_abs = ncrystal_cast_abs2proc (ncrystal_create_absorption (cfg));
  if (!ncrystal_isnonoriented(proc_scat) || !ncrystal_isnonoriented(proc_abs)) {
    fprintf(stderr, "Only non-oriented cross sections supported for calculating transmissions\n");
    exit(1);
  }

  double * transmission = (double *) malloc(independent_count * sizeof(double));
  if (!transmission) return transmission;

  // The cross-section is wavelength|wavenumber|energy dependent.
  // NCrystal uses kinetic-energies in eV -- but calculates this most-naturaly from wavelength in angstrom
  // So first convert everything to wavelength (maintaining specified point-order)
  // Calculate and store the wavlength|wavenumber|energy points:
  for (int i=0; i<independent_count; ++i){
    transmission[i] = independent_min + (double) i * independent_step;
  }

  if (type == FILTER_MEM_TYPE_ENERGY){
    // Convert from energy in meV to wavenumber
    double conversion = SE2V * MS2AA; // sqrt(E)[sqrt(meV)] to v[m/s] to k[1/angstrom]
    for (int i=0; i<independent_count; ++i){
      transmission[i] = sqrt(transmission[i]) * conversion;
    }
    type = FILTER_MEM_TYPE_WAVENUMBER;
  }
  if (type == FILTER_MEM_TYPE_WAVENUMBER){
    // Convert from wavenumber to wavelength
    for (int i=0; i<independent_count; ++i){
      transmission[i] = transmission[i] ? 2 * PI / transmission[i] : INFINITY;
    }
    type = FILTER_MEM_TYPE_WAVELENGTH;
  }

  if (type != FILTER_MEM_TYPE_WAVELENGTH){
    fprintf(stderr, "Expected conversion of independent variable to wavelength!\n");
    exit(1);
  }
  // Now actually calculate the per-point cross section and 1 cm tramsission
  // The transmission is exp(-numberdensity * crosssection * filterlength)
  // And is the combination of the scattering cross-section and the absorption cross-section.
  for (int i=0; i<independent_count; ++i){
    //
    double ekin = ncrystal_wl2ekin(transmission[i]);
    double sigma_scatter, sigma_absorb;
    ncrystal_crosssection_nonoriented(proc_scat, ekin, &sigma_scatter);
    ncrystal_crosssection_nonoriented(proc_abs, ekin, &sigma_absorb);
    // [N] = atom/angstrom^3
    // [sigma] = barn/atom = 10^-28 m^2/atom
    // [N*sigma] = 1/cm
    transmission[i] = exp(-numberdensity * (sigma_scatter + sigma_absorb) * thickness_cm);
  }

  return transmission;
}
