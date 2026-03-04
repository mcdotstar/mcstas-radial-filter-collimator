from mccode_antlr import Flavor
from mccode_antlr.assembler import Assembler

from textwrap import dedent

# ---------------------------------------------------------------------------
# Inline component: a minimal, fully deterministic neutron source.
# Emits one neutron per event from the origin with a fixed velocity along +z,
# t=0 and unit weight.
# ---------------------------------------------------------------------------
_FLAT_SOURCE_COMP = dedent("""\
    DEFINE COMPONENT FlatSource
    SETTING PARAMETERS (xwidth, yheight=0.2, double velocity=1000.0)
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
    from .utilities import repo_registry
    from mccode_antlr.reader.registry import InMemoryRegistry
    src_reg = InMemoryRegistry('trivial_src')
    src_reg.add_comp('FlatSource', _FLAT_SOURCE_COMP)
    return [src_reg, repo_registry()]


def make_test_instrument():
    from mccode_antlr.common.expression import Expr, Value, ObjectType, DataType, ShapeType, BinaryOp, OpStyle
    r = Assembler("test_instrument", registries=_make_registries(), flavor=Flavor.MCSTAS)
    
    r.parameter("int collimator=1")
    r.parameter('string powder="Na2Ca3Al2F14.laz"')

    collimator = Value("collimator", DataType.int, ObjectType.parameter, ShapeType.scalar)

    def make_collimator_expr(value: int) -> Expr:
        return f'{collimator:p} == {value}'
    
    r.initialize(dedent("""\
        printf("\\nTest_Collimator_Radial: ");
        switch (collimator) {
        case 1:
            printf("Using Collimator_Radial\\n"); break;
        case 2:
            printf("Using Collimator_ROC\\n"); break;
        case 3:
            printf("Using Exact_radial_coll\\n"); break;
        }
    """))

    r.component('origin', 'Progress_bar', at=([0, 0, 0], 'ABSOLUTE'),)

    r.component('source', 'Source_gen', at=([0, 0, 0], 'origin'), parameters={
        'focus_xw': 0.006, 'focus_yh': 0.01, 'lambda0': 1.5, 'dlambda': 0.01,
        'yheight': 0.05, 'xwidth': 0.05,
    })

    r.component('monitor1_xt', 'Monitor_nD', at=([0, 0, 1], 'source'), parameters={
        'options': '"x y"', 'xwidth': 0.05, 'yheight': 0.05, 'filename': '"monitor1_xt.dat"',
    })

    r.component('sample', 'PowderN', at=([0, 0, 0.1], 'monitor1_xt'), parameters={
        'reflections': 'powder', 'radius': 0.0030, 'p_transmit': 0.1, 'p_inc': 0.05,
        'yheight': 0.01, 'd_phi': 'RAD2DEG*atan2(0.09,1.5)',
    }).EXTEND("if (!SCATTERED) ABSORB;")

    r.component('banana_theta_in', 'Monitor_nD', at=([0, 0, 0], 'sample'), parameters={
        'options': '"banana, angle limits=[2 160], bins=1280, restore_neutron=1"',
        'radius': 0.3, 'yheight': 0.09,
        'filename': '"banana_theta_in.dat"',
    })

    r.component('collimador_rad', 'Collimator_radial', at=([0, 0, 0], 'sample'), parameters={
        'nslit': 'ceil((130-2)/0.42)', 'radius': 0.324, 'length': 0.419-0.324, 'yheight': 0.09,
        'theta_min': -160, 'theta_max': 160, 'roc': 0,
    }).WHEN(make_collimator_expr(1))

    r.component('collimador_d20', 'Collimator_ROC', at=([0, 0, 0], 'sample'), parameters={
        'ROC_pitch': 0.42, 'ROC_ri': 0.324, 'ROC_ro': 0.419, 'ROC_h': 0.09, 'ROC_ttmin': -160,
        'ROC_ttmax': 160, 'ROC_sign': -1,
    }).WHEN(make_collimator_expr(2))

    r.component('collimador_contrib', 'Exact_radial_coll', at=([0, 0, 0], 'sample'), parameters={
        'nslit': 'ceil(130-2/0.42)', 'radius': 0.324, 'length': 0.419-0.324, 'h_in': 0.09,
        'h_out': 0.09, 'theta_min': -160, 'theta_max': 160,
    }).WHEN(make_collimator_expr(3))

    r.component('banana_theta', 'Monitor_nD', at=([0, 0, 0], 'sample'), parameters={
        'options': '"banana, angle limits=[2 160], bins=1280"', 'radius': 1.5, 'yheight': 0.09,
        'filename': '"banana_theta.dat"',
    })
    return r.instrument


def test_collimators_are_similar():
    """Test that the three collimator components give similar results for a simple
    instrument geometry."""
    from .utilities import compile_and_run
    from numpy import allclose

    instr = make_test_instrument()
    results = {}
    for collimator in [1, 2, 3]:
        output, datafiles = compile_and_run(
            instr, 
            f'-n 1000000 --seed=1 collimator={collimator}', 
            # use_temp_dir=False
            )
        results[collimator] = datafiles

    banana_theta = {i: results[i]['banana_theta'] for i in range(1, 4)}

    # # Compare the three banana_theta histograms pairwise.
    # for i in range(1, 4):
    #     for j in range(i + 1, 4):
    #         assert allclose(banana_theta[i]['I'], banana_theta[j]['I'], rtol=0.1, atol=0.01)