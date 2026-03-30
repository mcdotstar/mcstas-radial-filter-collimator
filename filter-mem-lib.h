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
#include "string.h"

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
  FILTER_MEM_TYPE_UNKNOWN,    ///< Provided distribution has unknown independent axis
  FILTER_MEM_TYPE_ENERGY,     ///< Provided distribution is in energy, meV
  FILTER_MEM_TYPE_WAVELENGTH, ///< Provided distribution is in wavelength, angstrom
  FILTER_MEM_TYPE_WAVENUMBER, ///< Provided distribution is in wavenumber, 1/angstrom
};
enum filter_mem_mode {
  FILTER_MEM_MODE_SET, ///< Replace the neutron weight with the distribution value
  FILTER_MEM_MODE_ADD, ///< Add the distribution value to the neutron weight
  FILTER_MEM_MODE_MUL, ///< Multiply the neutron weight by the distribution value
};

/** \brief Determine the type and fill in any missing axis bounds for a Filter_mem component.
 *
 * Identifies what physical quantity the independent axis represents from the
 * \p which string, then fills in any UNSET values among \p minimum, \p maximum,
 * and \p delta using the count and sensible per-type defaults.  Always updates
 * \p count to equal `(maximum - minimum) / delta` on return.
 *
 * \param[in]     which    String containing one of "E"/"energy", "L"/"wavelength",
 *                         or "k"/"wavenumber".  NULL or empty defaults to wavelength.
 * \param[in,out] minimum  Minimum value of the independent axis; filled from defaults if UNSET.
 * \param[in,out] maximum  Maximum value of the independent axis; filled from defaults if UNSET.
 * \param[in,out] delta    Step size between points; derived from the other values if UNSET.
 * \param[in,out] count    Number of steps; used as a hint when deriving missing values,
 *                         and always recalculated as `(maximum - minimum) / delta` on return.
 * \param[in,out] type     Set to the enumerated type matching \p which; unchanged if \p which
 *                         does not match any recognised string.
 */
void filter_mem_which(char * which, double * minimum, double * maximum, double * delta, int * count, enum filter_mem_type *type);

/** \brief Set the operating mode and verbosity for a Filter_mem component.
 *
 * Scans the \p opts string for recognised keywords and updates \p mode and
 * \p verbose accordingly.  Both output parameters are left unchanged if no
 * matching keyword is found in \p opts.
 *
 * \param[in]     opts     String optionally containing "set", "add", or "multiply"
 *                         (first match wins) and/or "verbose".  NULL or empty is a no-op.
 * \param[in,out] mode     Updated to SET, ADD, or MUL when the corresponding keyword
 *                         is present; otherwise unchanged.
 * \param[in,out] verbose  Set to 1 when "verbose" is present in \p opts; otherwise unchanged.
 */
void filter_mem_options(char * opts, enum filter_mem_mode * mode, int * verbose);

/** \brief Resolve the spatial bounds of a Filter_mem component.
 *
 * For each axis, any single UNSET value among (length, min, max) is derived
 * from the other two.  If both min and max are already set, length is ignored.
 *
 * \param width   Width of the filter along x; used to derive \p wmin and/or \p wmax if either is UNSET.
 * \param[in,out] wmin  Minimum x bound; derived as `wmax - width` if UNSET.
 * \param[in,out] wmax  Maximum x bound; derived as `wmin + width` if UNSET.
 * \param height  Height of the filter along y; used to derive \p hmin and/or \p hmax if either is UNSET.
 * \param[in,out] hmin  Minimum y bound; derived as `hmax - height` if UNSET.
 * \param[in,out] hmax  Maximum y bound; derived as `hmin + height` if UNSET.
 *
 * \returns
 *   - 0 if both axes were successfully resolved (all bounds are now finite).
 *   - 1 if all six inputs are UNSET (the component has no spatial extent defined).
 *   - 2 if the x-axis could not be resolved (either \p wmin/\p wmax UNSET and \p width UNSET).
 *   - 4 if the y-axis could not be resolved (either \p hmin/\p hmax UNSET and \p height UNSET).
 *   - 6 if neither axis could be resolved.
 */
int filter_mem_geometry(double width, double * wmin, double * wmax, double height, double * hmin, double * hmax);

/** \brief Compute an array of per-unit-cm neutron transmission values using NCrystal.
 *
 * Each element \c i gives the probability that a neutron survives 1 cm of the
 * specified material: \c exp(-N * (sigma_scatter + sigma_absorb)), where \c N is
 * the material number density (atoms/ų) from NCrystal.
 *
 * Results are memoised: a second call with identical arguments returns the same
 * pointer without recomputing.  The cache is freed by filter_mem_cache_clear().
 *
 * \param type              Physical quantity the independent axis represents.
 * \param independent_min   First point on the independent axis.
 * \param independent_step  Spacing between consecutive points.
 * \param independent_count Number of points; determines the length of the returned array.
 * \param cfg               NCrystal configuration string identifying the material.
 * \returns Heap-allocated array of \p independent_count transmission values,
 *          owned by the internal cache.  Returns NULL on allocation failure.
 */
double * filter_mem_transmission(enum filter_mem_type type, double independent_min, double independent_step, int independent_count, const char * cfg);

/** \brief Free all entries held by the transmission cache.
 *
 * Releases every cached transmission array and resets the singleton cache.
 * Safe to call multiple times or when the cache is empty.
 * Only affects distributions computed internally by filter_mem_transmission;
 * user-supplied arrays are never stored in the cache.
 */
void filter_mem_cache_clear(void);

#endif
