from mccode_antlr import Flavor
from mccode_antlr.assembler import Assembler


def make_test_instrument():
    from textwrap import dedent
    from mccode_antlr.common.expression import Expr
    from .utilities import repo_registry, write_init_switch
    r = Assembler("test_instrument", registries=[repo_registry()], flavor=Flavor.MCSTAS)
    
    r.parameter("int collimator=1")
    r.parameter('string powder="Na2Ca3Al2F14.laz"')

    def make_collimator_expr(value: int) -> Expr:
        return Expr.parameter('collimator').eq(value)
    
    write_init_switch(r, "collimator", {
        1: "Using Collimator_Radial",
        2: "Using Collimator_ROC",
        3: "Using Exact_radial_coll",
        4: "Using Radial_filter_collimator",
    })

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

    r.component('radial', 'Collimator_radial', at=([0, 0, 0], 'sample'), parameters={
        'nslit': 'ceil((130-2)/0.42)', 'radius': 0.324, 'length': 0.419-0.324, 'yheight': 0.09,
        'theta_min': -160, 'theta_max': 160, 'roc': 0,
    }).WHEN(make_collimator_expr(1))

    r.component('d20', 'Collimator_ROC', at=([0, 0, 0], 'sample'), parameters={
        'ROC_pitch': 0.42, 'ROC_ri': 0.324, 'ROC_ro': 0.419, 'ROC_h': 0.09, 'ROC_ttmin': -160,
        'ROC_ttmax': 160, 'ROC_sign': -1,
    }).WHEN(make_collimator_expr(2))

    r.component('exact', 'Exact_radial_coll', at=([0, 0, 0], 'sample'), parameters={
        'nslit': 'ceil(130-2/0.42)', 'radius': 0.324, 'length': 0.419-0.324, 'h_in': 0.09,
        'h_out': 0.09, 'theta_min': -160, 'theta_max': 160,
    }).WHEN(make_collimator_expr(3))

    r.component('filter', 'Radial_filter_collimator', at=([0, 0, 0], 'sample'), parameters={
        'cfg': '""',  # vacuum
        'angle_minimum': -160, 'angle_maximum': 160, 'yheight': 0.09, 
        'minimum_radius': 0.324, 'maximum_radius': 0.419, 'collimation': 0.42,
    }).WHEN(make_collimator_expr(4))

    r.component('banana_theta', 'Monitor_nD', at=([0, 0, 0], 'sample'), parameters={
        'options': '"banana, angle limits=[2 160], bins=1280"', 'radius': 1.5, 'yheight': 0.09,
        'filename': '"banana_theta.dat"',
    })
    return r.instrument



def test_collimators_are_similar():
    """Test that the three collimator components give similar results for a simple
    instrument geometry."""
    from .utilities import compile_and_scan, acceptable_total_counts

    instr = make_test_instrument()
    banana_theta = {}
    collimators = {
        "Collimator_Radial": 1.52649e-08, 
        "Collimator_ROC": 1.41509e-08, 
        "Exact_radial_coll": 1.581e-08,
        "Radial_filter_collimator": 1.42e-08,
    }
    res_dict = compile_and_scan(instr, dict(collimator="1,2,3,4"), ncount=2e6, seed=1)

    colnames = list(collimators.keys())
    banana_theta = {colnames[i]: v['banana_theta'] for i, v in enumerate(res_dict['scan_result'])}
    # totals = {k: sum(v['I']) for k, v in banana_theta.items()}

    for k, v in banana_theta.items():
        assert acceptable_total_counts(v, collimators[k]), (
            f"Total counts for {k} differ from expected by more than 10%: "
            f"{v['I'].sum()} vs {collimators[k]}"
        )


    # # Compare the three banana_theta histograms pairwise.
    # for i in range(1, 4):
    #     for j in range(i + 1, 4):
    #         assert allclose(banana_theta[i]['I'], banana_theta[j]['I'], rtol=0.1, atol=0.01)