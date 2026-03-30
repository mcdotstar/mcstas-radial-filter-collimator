// Unusual guard needed for use in McCode runtime
#ifndef FILTER_GEN_MEM
#include "filter-mem-lib.h"
#endif

/* ---- Transmission memoisation cache ---- */

typedef struct {
    enum filter_mem_type type;
    double independent_min;
    double independent_step;
    int    independent_count;
    char * cfg;
} filter_mem_cache_key_t;

typedef struct {
    filter_mem_cache_key_t key;
    double * transmission;
} filter_mem_cache_entry_t;

typedef struct {
    filter_mem_cache_entry_t * entries;
    int count;
    int capacity;
} filter_mem_cache_t;

static filter_mem_cache_t _filter_mem_cache = {NULL, 0, 0};

static int _filter_mem_key_equal(const filter_mem_cache_key_t *a, const filter_mem_cache_key_t *b) {
    return a->type == b->type
        && a->independent_min   == b->independent_min
        && a->independent_step  == b->independent_step
        && a->independent_count == b->independent_count
        && strcmp(a->cfg ? a->cfg : "", b->cfg ? b->cfg : "") == 0;
}

static double * _filter_mem_cache_lookup(const filter_mem_cache_key_t *key) {
    for (int i = 0; i < _filter_mem_cache.count; ++i) {
        if (_filter_mem_key_equal(&_filter_mem_cache.entries[i].key, key))
            return _filter_mem_cache.entries[i].transmission;
    }
    return NULL;
}

static void _filter_mem_cache_insert(const filter_mem_cache_key_t *key, double *transmission) {
    if (_filter_mem_cache.count == _filter_mem_cache.capacity) {
        int new_cap = _filter_mem_cache.capacity ? _filter_mem_cache.capacity * 2 : 4;
        filter_mem_cache_entry_t *entries = (filter_mem_cache_entry_t *)
            realloc(_filter_mem_cache.entries, new_cap * sizeof(filter_mem_cache_entry_t));
        if (!entries) return;
        _filter_mem_cache.entries  = entries;
        _filter_mem_cache.capacity = new_cap;
    }
    filter_mem_cache_entry_t *entry = &_filter_mem_cache.entries[_filter_mem_cache.count++];
    entry->key            = *key;
    entry->key.cfg        = key->cfg ? strdup(key->cfg) : NULL;
    entry->transmission   = transmission;
}

void filter_mem_cache_clear(void) {
    for (int i = 0; i < _filter_mem_cache.count; ++i) {
        free(_filter_mem_cache.entries[i].key.cfg);
        free(_filter_mem_cache.entries[i].transmission);
    }
    free(_filter_mem_cache.entries);
    _filter_mem_cache.entries  = NULL;
    _filter_mem_cache.count    = 0;
    _filter_mem_cache.capacity = 0;
}

/* ---- End of cache ---- */

void filter_mem_which(char * which, double * minimum, double * maximum, double * delta, int * count, enum filter_mem_type *type){
  // Determine what the independe-variable parameters represent:
  if (!which || !strlen(which) || strstr(which, "L") || strstr(which, "wavelength")){
    *type = FILTER_MEM_TYPE_WAVELENGTH;
  } else if (strstr(which, "E") || strstr(which, "energy")){
    *type = FILTER_MEM_TYPE_ENERGY;
  } else if (strstr(which, "k") || strstr(which, "wavenumber")){
    *type = FILTER_MEM_TYPE_WAVENUMBER;
  }
  // And update their (missing) values as needed with reasonable defaults
  if (any_unset(3, *minimum, *maximum, *delta)){
    // ensure we have _an_ integer to use
    int dcount = *count ? *count : 1000;
    // defaults that are (hopefully) broad enough for all useful cases
    double dmin, dmax, ddel;
    dmin = (FILTER_MEM_TYPE_ENERGY == *type) ? 0.01 : (FILTER_MEM_TYPE_WAVENUMBER == *type) ? 1e-2 : 1e-3;
    dmax = (FILTER_MEM_TYPE_ENERGY == *type) ? 1e3 : (FILTER_MEM_TYPE_WAVENUMBER == *type) ? 1e3 : 1e2;
    ddel = (dmax - dmin) / dcount;
  
    if (all_unset(3, *minimum, *maximum, *delta)){
      *minimum = dmin;
      *maximum = dmax;
      *delta = ddel;
    } else if (all_unset(2, *minimum, *maximum)) {
      *minimum = dmin;
      *maximum = dmin + *count * *delta;
    } else if (all_unset(2, *minimum, *delta)) {
      *minimum = (*maximum - dcount * ddel) > 0 ? (*maximum - dcount * ddel) : dmin;
      *delta = (*maximum - *minimum) / dcount;
    } else if (all_unset(2, *delta, *maximum)) {
      *maximum = *minimum + dcount * ddel;
      *delta = ddel;
    } else if (is_unset(*minimum)) {
      *minimum = (*maximum - *delta * dcount) > 0 ? (*maximum - *delta * dcount) : dmin;
      *delta = (*maximum - *minimum) / dcount;
    } else if (is_unset(*maximum)) {
      *maximum = *minimum + *delta * dcount;
    } else if (is_unset(*delta)) {
      *delta = (*maximum - *minimum) / dcount;
    }
  }
  // Now that all of (minimum, maximum, delta) are defined, ensure our count value is valid
  *count = (int)((*maximum - *minimum) / *delta);
}


void filter_mem_options(char * opts, enum filter_mem_mode * mode, int * verbose){
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


static int _filter_mem_geometry_axis(double length, double * minimum, double * maximum){
  int min_unset = is_unset(*minimum);
  int max_unset = is_unset(*maximum);
  if (!min_unset && !max_unset) return 0;
  if (min_unset && max_unset){
    *maximum = length/2;
    *minimum = -(*maximum);
  } else if (min_unset){
    *minimum = *maximum - length;
  } else {
    *maximum = *minimum + length;
  }
  return is_unset(length);
}


int filter_mem_geometry(double width, double * wmin, double * wmax, double height, double * hmin, double * hmax){
  if (all_unset(6, width, *wmin, *wmax, height, *hmin, *hmax)){
    return 1;
  }
  int ret = 0;
  ret += 2*_filter_mem_geometry_axis(width, wmin, wmax);
  ret += 4*_filter_mem_geometry_axis(height, hmin, hmax);
  return ret;
}


static double * _filter_mem_transmission_compute(enum filter_mem_type type, double independent_min, double independent_step, int independent_count, const char * cfg){
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
    transmission[i] = exp(-numberdensity * (sigma_scatter + sigma_absorb));
  }

  return transmission;
}

double * filter_mem_transmission(enum filter_mem_type type, double independent_min, double independent_step, int independent_count, const char * cfg){
  filter_mem_cache_key_t key = {type, independent_min, independent_step, independent_count, (char *)cfg};
  double *cached = _filter_mem_cache_lookup(&key);
  if (cached) return cached;
  double *result = _filter_mem_transmission_compute(type, independent_min, independent_step, independent_count, cfg);
  if (result) _filter_mem_cache_insert(&key, result);
  return result;
}
