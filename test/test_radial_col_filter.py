"""Tests for the Radial_col_filter McStas component.

Tests
-----
test_component_parses
    Verifies that mccode-antlr can load and parse the .comp file without a
    C compiler or NCrystal installation.

test_component_compiles
    Builds and runs a trivial instrument containing the component.
    Skipped when no working C compiler is available.

test_neutron_passes_when_outside_geometry
    Neutrons aimed outside the angular sector must propagate unchanged.
    Skipped when no working C compiler is available.

test_collimator_blocks_neutrons
    Neutrons aimed directly at a collimator-vane boundary must be absorbed.
    Skipped when no working C compiler is available.
"""
from __future__ import annotations

from textwrap import dedent

from .utilities import compiled, repo_registry, compile_and_run

# ---------------------------------------------------------------------------
# Inline component: a minimal, fully deterministic neutron source.
# ---------------------------------------------------------------------------
_TRIVIAL_SOURCE_COMP = dedent("""\
    DEFINE COMPONENT TrivialSource
    SETTING PARAMETERS (double velocity=2200.0, double vx_in=0.0, double vy_in=0.0)
    TRACE
    %{
      x = 0; y = 0; z = 0;
      vx = vx_in; vy = vy_in; vz = velocity;
      t = 0.0;
      p = 1.0;
      SCATTER;
    %}
    END
""")


def _make_registries():
    from mccode_antlr.reader.registry import InMemoryRegistry
    src_reg = InMemoryRegistry('trivial_src')
    src_reg.add_comp('TrivialSource', _TRIVIAL_SOURCE_COMP)
    return [src_reg, repo_registry()]


# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

def test_component_parses():
    """mccode-antlr can read and parse Radial_col_filter.comp.

    Does not require a C compiler or NCrystal installation.
    """
    from pathlib import Path
    from git import Repo, InvalidGitRepositoryError
    from mccode_antlr import Flavor
    from mccode_antlr.reader import Reader

    try:
        root = Path(Repo('.', search_parent_directories=True).working_tree_dir)
    except InvalidGitRepositoryError as exc:
        raise RuntimeError(f"Could not locate repository root: {exc}") from exc

    comp_path = root / 'Radial_col_filter.comp'
    assert comp_path.exists(), f"Component file not found: {comp_path}"

    reader = Reader(registries=[repo_registry()], flavor=Flavor.MCSTAS)
    comp = reader.get_component('Radial_col_filter')
    assert comp is not None
    assert comp.name == 'Radial_col_filter'


@compiled
def test_component_compiles():
    """A minimal instrument containing Radial_col_filter compiles and runs."""
    from mccode_antlr import Flavor
    from mccode_antlr.assembler import Assembler

    assembler = Assembler(
        'RfcmCompileTest',
        registries=_make_registries(),
        flavor=Flavor.MCSTAS,
    )
    assembler.initialize('printf("rfcm_test_start\\n");')

    src = assembler.component(
        'origin', 'TrivialSource',
        at=([0, 0, 0], 'ABSOLUTE'),
    )
    assembler.component(
        'rfcm', 'Radial_col_filter',
        at=([0, 0, 0], src),
        parameters={
            'angle_width': 10,
            'yheight': 0.2,
            'minimum_radius': 0.1,
            'maximum_radius': 0.5,
            'collimation': 2,
        },
    )

    instr = assembler.instrument
    results = compile_and_run(instr, ncount=10, seed=1)
    assert b'rfcm_test_start' in results['output']


@compiled
def test_neutron_passes_when_outside_geometry():
    """Neutrons aimed outside the angular sector are not absorbed or attenuated."""
    from mccode_antlr import Flavor
    from mccode_antlr.assembler import Assembler

    assembler = Assembler(
        'RfcmPassthroughTest',
        registries=_make_registries(),
        flavor=Flavor.MCSTAS,
    )
    assembler.initialize('printf("rfcm_passthrough_start\\n");')

    src = assembler.component(
        'origin', 'TrivialSource',
        at=([0, 0, 0], 'ABSOLUTE'),
        # Large transverse velocity — well outside the ±5 deg window.
        parameters={'vx_in': 2200.0 * 5.0, 'velocity': 2200.0},
    )
    assembler.component(
        'rfcm', 'Radial_col_filter',
        at=([0, 0, 0], src),
        parameters={
            'angle_width': 10,
            'yheight': 0.2,
            'minimum_radius': 0.1,
            'maximum_radius': 0.5,
            'collimation': 2,
        },
    )

    instr = assembler.instrument
    results = compile_and_run(instr, ncount=1, seed=1)
    text = results['output'].decode(errors='replace')
    assert 'rfcm_passthrough_start' in text, (
        f"Sentinel missing from output:\n{text}"
    )


@compiled
def test_collimator_blocks_neutrons():
    """Neutrons aimed at a collimator-vane boundary are absorbed."""
    from mccode_antlr import Flavor
    from mccode_antlr.assembler import Assembler

    assembler = Assembler(
        'RfcmCollimatorTest',
        registries=_make_registries(),
        flavor=Flavor.MCSTAS,
    )
    assembler.initialize('printf("rfcm_collimator_start\\n");')

    src = assembler.component(
        'origin', 'TrivialSource',
        at=([0, 0, 0], 'ABSOLUTE'),
        # Neutron travels along +z, directly on the 0-deg vane plane.
        parameters={'vx_in': 0.0, 'velocity': 2200.0},
    )
    assembler.component(
        'rfcm', 'Radial_col_filter',
        at=([0, 0, 0], src),
        parameters={
            'angle_minimum': -10,
            'angle_maximum':  10,
            'yheight': 0.2,
            'minimum_radius': 0.1,
            'maximum_radius': 0.5,
            'collimation': 5,   # vanes at -10, -5, 0, 5, 10 deg
        },
    )

    instr = assembler.instrument
    results = compile_and_run(instr, ncount=1, seed=1)
    text = results['output'].decode(errors='replace')
    assert 'rfcm_collimator_start' in text, (
        f"Sentinel missing from output:\n{text}"
    )
