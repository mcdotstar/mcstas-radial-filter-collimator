"""Tests for the Filter_sample McStas component.

Tests
-----
test_component_parses
    Verifies mccode-antlr can parse Filter_sample.comp without a C compiler
    or NCrystal installation.

test_box_compiles
    Builds and runs a trivial instrument with a box-shaped Filter_sample.
    Skipped when no working C compiler is available.

test_cylinder_compiles
    Builds and runs a trivial instrument with a cylinder-shaped Filter_sample.
    Skipped when no working C compiler is available.
"""
from __future__ import annotations

from textwrap import dedent

from .utilities import compiled, repo_registry, compile_and_run

_TRIVIAL_SOURCE_COMP = dedent("""\
    DEFINE COMPONENT TrivialSource
    SETTING PARAMETERS (double velocity=2200.0)
    TRACE
    %{
      x = 0; y = 0; z = 0;
      vx = 0; vy = 0; vz = velocity;
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


def test_component_parses():
    """mccode-antlr can read and parse Filter_sample.comp without a compiler."""
    from pathlib import Path
    from git import Repo, InvalidGitRepositoryError
    from mccode_antlr import Flavor
    from mccode_antlr.reader import Reader

    try:
        root = Path(Repo('.', search_parent_directories=True).working_tree_dir)
    except InvalidGitRepositoryError as exc:
        raise RuntimeError(f"Could not locate repository root: {exc}") from exc

    comp_path = root / 'Filter_sample.comp'
    assert comp_path.exists(), f"Component file not found: {comp_path}"

    reader = Reader(registries=[repo_registry()], flavor=Flavor.MCSTAS)
    comp = reader.get_component('Filter_sample')
    assert comp is not None
    assert comp.name == 'Filter_sample'


@compiled
def test_box_compiles():
    """A minimal instrument with a box-shaped Filter_sample compiles and runs."""
    from mccode_antlr import Flavor
    from mccode_antlr.assembler import Assembler

    assembler = Assembler(
        'FilterSampleBoxTest',
        registries=_make_registries(),
        flavor=Flavor.MCSTAS,
    )
    assembler.initialize('printf("fs_box_start\\n");')

    src = assembler.component(
        'origin', 'TrivialSource',
        at=([0, 0, 0], 'ABSOLUTE'),
    )
    assembler.component(
        'sample', 'Filter_sample',
        at=([0, 0, 0.5], src),
        parameters={
            'xwidth': 0.1,
            'yheight': 0.1,
            'zdepth': 0.15,
        },
    )

    instr = assembler.instrument
    results = compile_and_run(instr, ncount=10, seed=1)
    assert b'fs_box_start' in results['output']


@compiled
def test_cylinder_compiles():
    """A minimal instrument with a cylinder-shaped Filter_sample compiles and runs."""
    from mccode_antlr import Flavor
    from mccode_antlr.assembler import Assembler

    assembler = Assembler(
        'FilterSampleCylTest',
        registries=_make_registries(),
        flavor=Flavor.MCSTAS,
    )
    assembler.initialize('printf("fs_cyl_start\\n");')

    src = assembler.component(
        'origin', 'TrivialSource',
        at=([0, 0, 0], 'ABSOLUTE'),
    )
    assembler.component(
        'sample', 'Filter_sample',
        at=([0, 0, 0.5], src),
        parameters={
            'radius': 0.05,
            'yheight': 0.1,
        },
    )

    instr = assembler.instrument
    results = compile_and_run(instr, ncount=10, seed=1)
    assert b'fs_cyl_start' in results['output']
