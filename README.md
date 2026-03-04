# Radial_filter_collimator

A [McStas](https://mcstas.org) component that combines a **radial collimator** and an
**NCrystal filter** in a single annular wedge geometry.  Originally developed for the
[BIFROST](https://ess.eu/instruments/bifrost) instrument at the
European Spallation Source.

---

## Description

`Radial_filter_collimator` models a wedge-shaped volume (defined by an inner/outer radius
and an angular range in the *x*–*z* plane) that acts simultaneously as:

* **Filter** — neutrons passing through the annular region interact with an
  [NCrystal](https://github.com/mctools/ncrystal) material (default: beryllium at 77 K).
  Scattering and absorption are handled via the full NCrystal physics backend.
* **Collimator** — radial vanes (absorbing blades) are placed at regular angular intervals
  inside the same volume.  Neutrons that cross a vane boundary are absorbed.

Neutrons that miss both the filter volume and the collimator pass through unaffected.

### Geometry

```
         ang_max
          /
 fil_max /___
        |    |  ← filter annulus (NCrystal material)
 fil_min|    |
        |    |
         \___\
          \
         ang_min

  collimation blades spaced every `collimation` degrees inside the same wedge
```

---

## Parameters

| Parameter | Unit | Default | Description |
|-----------|------|---------|-------------|
| `cfg` | str | `Be_sg194.ncmat;temp=77K` | NCrystal material configuration string |
| `absorptionmode` | 0\|1\|2 | `1` | 0 = no absorption; 1 = intensity reduction; 2 = terminate track |
| `multscat` | 0\|1 | `1` | Enable (1) or disable (0) multiple scattering |
| `angle_minimum` | deg | UNSET | Lower angular bound in the *x*–*z* plane |
| `angle_maximum` | deg | UNSET | Upper angular bound in the *x*–*z* plane |
| `angle_width` | deg | `0` | Total angular width; sets `±angle_width/2` when min/max are unset |
| `y_minimum` | m | UNSET | Lower *y* bound |
| `y_maximum` | m | UNSET | Upper *y* bound |
| `yheight` | m | `0` | Height; sets `±yheight/2` when min/max are unset |
| `collimator_minimum_radius` | m | UNSET | Inner collimator radius (falls back to `minimum_radius`) |
| `collimator_maximum_radius` | m | UNSET | Outer collimator radius (falls back to `maximum_radius`) |
| `filter_minimum_radius` | m | UNSET | Inner filter radius (falls back to `minimum_radius`) |
| `filter_maximum_radius` | m | UNSET | Outer filter radius (falls back to `maximum_radius`) |
| `minimum_radius` | m | `0` | Shared inner radius fallback |
| `maximum_radius` | m | `0` | Shared outer radius fallback |
| `collimation` | deg | `0` | Angular spacing between collimator vanes |

---

## Requirements

* **McStas ≥ 3.x** (the component uses the `DEPENDENCY "@NCRYSTALFLAGS@"` mechanism)
* **NCrystal** — the `ncrystal-config` executable must be on `PATH` when McStas resolves
  the component's compile-time flags
* **mccode-antlr** (Python) — for programmatic instrument construction and for the tests

Install the Python tooling:

```bash
pip install mccode-antlr
```

---

## Using the component with mccode-antlr

The snippet below fetches `Radial_filter_collimator.comp` directly from this GitHub
repository (no local clone required) and builds a minimal test instrument.

```python
from mccode_antlr import Flavor
from mccode_antlr.assembler import Assembler
from mccode_antlr.reader.registry import GitHubRegistry

# Fetch the component from GitHub automatically via pooch.
# The `pooch-registry.txt` file in the repo root is kept up to date by CI.
rfc_registry = GitHubRegistry(
    name='radial_filter_collimator',
    url='https://github.com/mcdotstar/mcstas-radial-filter-collimator',
    version='main',
    filename='pooch-registry.txt',
)

assembler = Assembler(
    'MyInstrument',
    registries=[rfc_registry],
    flavor=Flavor.MCSTAS,
)

# Instrument parameter: neutron wavelength in Å
assembler.parameter('double wavelength=2.0')

# Source → filter/collimator → monitor
source = assembler.component(
    'source', 'Source_simple',
    at=([0, 0, 0], 'ABSOLUTE'),
    parameters={
        'radius': 0.05,
        'dist': 1.0,
        'focus_xw': 0.05,
        'focus_yh': 0.10,
        'lambda0': 'wavelength',
        'dlambda': 0.1,
    },
)

assembler.component(
    'filter_collimator', 'Radial_filter_collimator',
    at=([0, 0, 1.0], 'ABSOLUTE'),
    parameters={
        'cfg': 'Be_sg194.ncmat;temp=77K',   # beryllium filter at 77 K
        'angle_width': 10,                   # ±5° angular acceptance
        'yheight': 0.10,
        'minimum_radius': 0.05,
        'maximum_radius': 0.30,
        'collimation': 2,                    # 2° vane spacing
        'absorptionmode': 1,
        'multscat': 1,
    },
)

assembler.component(
    'detector', 'PSD_monitor',
    at=([0, 0, 1.5], 'ABSOLUTE'),
    parameters={
        'nx': 128, 'ny': 128,
        'xwidth': 0.10, 'yheight': 0.10,
        'filename': '"psd.dat"',
    },
)

instr = assembler.instrument
print(instr)   # inspect the assembled McStas instrument object
```

> **Tip:** pin `version` to a specific tag (e.g. `'v0.1.0'`) for reproducible builds.

---

## Running the tests

The test suite uses `pytest` and `mccode-antlr`.  A working C compiler is required for
the compilation tests; the parse test runs without one.

```bash
pip install -r requirements.txt
pytest test/ -v
```

The four test cases are:

| Test | Requires compiler | Description |
|------|:-----------------:|-------------|
| `test_component_parses` | no | DSL syntax is valid and all sections are parseable |
| `test_component_compiles` | yes | minimal instrument compiles and runs without error |
| `test_neutron_passes_when_outside_geometry` | yes | neutron outside angular sector exits unchanged |
| `test_collimator_blocks_neutrons` | yes | neutron on a vane boundary is absorbed |

---

## Pre-commit hook

A `mcfmt` pre-commit hook is provided that checks `.comp` files for parse errors and
C-code-block formatting issues before every commit:

```bash
cp hooks/pre-commit .git/hooks/pre-commit
chmod +x .git/hooks/pre-commit
```

The hook runs `mcfmt --check --clang-format` on every staged `*.comp` file and blocks
the commit if any issues are found.  To fix formatting in place:

```bash
mcfmt -i --clang-format Radial_filter_collimator.comp
```

---

## Registry file

The `pooch-registry.txt` file in the repository root is regenerated automatically by the
`register.yml` GitHub Actions workflow on every push to `main`.  You do not need to
update it manually.

---

## License

See [LICENSE](LICENSE) (if present) or contact the authors.

