from mccode_antlr import Flavor
from mccode_antlr.assembler import Assembler
from textwrap import dedent

def make_test_instrument():
    """
    Build and extend the test instrument, Test_filters.instr,
    from the McCode repostiory, originally by Peter Willendrup
    """
    from textwrap import dedent
    from math import atan2, pi
    from mccode_antlr.common.expression import Expr, binary_expr, DataType
    from .utilities import repo_registry, write_init_switch
    r = Assembler("test_filters", registries=[repo_registry()], flavor=Flavor.MCSTAS)

    # Ensure the required data files are present in the registry
    # datafiles = ["Be.trm", "Be.laz"]
    # for reg in r.instrument.registries:
    #     for datafile in datafiles:
    #         if reg.known(datafile):
    #             reg.path(datafile)
        
    
    r.parameter("int filter=1")
    r.parameter('double Lmin=0.5')
    r.parameter('double Lmax=10')
    r.parameter('int nL=101')
    r.parameter('double zdepth=0.15')
    r.parameter('double T=80')
    r.parameter('double radius=1000.0') # Large radius to approximate a flat filter

    r.declare(dedent("""
        double lambda0; 
        double dlambda; 
        char NCcfg[128];
    """)
    )
    write_init_switch(r, "filter", {
        0: r"Running with Filter_gen(filename=\"Be.trm\", zdepth=%g,...)\\n",
        1: r"Running with NCrystal(cfg=, zdepth=%g,...)\\n",
        2: r"Running with PowderN(reflections=\"Be.laz\", zdepth=%g,...)\\n",
        3: r"Running with Radial_filter_collimator(zdepth=%g,...)\\n",
    }, leadin=dedent("""
        lambda0 = (Lmax+Lmin)/2.0;
        dlambda = (Lmax-Lmin)/2.0;
        sprintf(NCcfg,\"Be_sg194.ncmat;temp=%.2gK\",T);
    """),
    decorator = lambda x: f'MPI_MASTER( printf("{x}, zdepth"); )'
    )

    r.component('origin', 'Progress_bar', at=([0, 0, 0], 'ABSOLUTE'),)
    r.component('source_div', 'Source_div', parameters={
        'xwidth': 0.1, 
        'yheight': 0.1, 
        'focus_aw': 0.00001, 
        'focus_ah': 0.00001, 
        'lambda0': 'lambda0', 
        'dlambda': 'dlambda', 
        'flux': 1e14,
    }, at=([0, 0, 0], 'origin'))

    r.component('L_in', 'L_monitor', parameters={
        'nL': 'nL', 
        'xwidth': 0.1, 
        'yheight': 0.1, 
        'Lmin': 'Lmin',
        'Lmax': 'Lmax',
    }, at=([0, 0, 1e-3], 'source_div'))

    r.component('FilterA', 'Filter_gen', parameters={
        'filename': '"Be.trm"',
        'xwidth': 0.2,
        'yheight': 0.2,
        'thickness': 'zdepth',
    }, at=([0, 0, 1e-3], 'L_in'), when=Expr.parameter('filter').eq(0))

    r.component('FilterB', 'NCrystal_sample', parameters={
        'yheight': 0.2,
        'xwidth': 0.2,
        'zdepth': 'zdepth',
        'cfg': 'NCcfg',
    }, at=([0, 0, 1e-3], 'L_in'), when=Expr.parameter('filter').eq(1))

    r.component('FilterC', 'PowderN', parameters={
        'yheight': 0.2,
        'xwidth': 0.2,
        'zdepth': 'zdepth',
        'reflections': '"Be.laz"',
    }, at=([0, 0, 1e-3], 'L_in'), when=Expr.parameter('filter').eq(2))

    # Approximate a flat filter by using a large minimum radius
    # - set the width to the same as the other filters (0.2 m) and calculate the corresponding angle_width
    # - set the collimator blade spacing to the full width so there is only one "slit" and it covers the whole area
    # - set the thickness to zdepth as for the other filters
    # - position the filter so that its inner radius is at the same location as the other filters
    radius = Expr.parameter('radius')
    radius.data_type = DataType.float
    zdepth = Expr.parameter('zdepth')
    angle_width = (2 * 180 / pi) * binary_expr(atan2, 'atan2', Expr.float(0.2), 2*radius)  # angle_width = 2*atan(width/(2*radius))
    angle_width.data_type = DataType.float
    r.component('FilterE', 'Radial_filter_collimator', parameters={
        'yheight': 0.2,
        'filter_angle_width': angle_width,
        'minimum_radius': radius,
        'maximum_radius': radius + zdepth,
        'collimator_angle_width': 180.0, # much wider than the filter
        'collimation': 180.0,
        'cfg': 'NCcfg',
    }, at=([0, 0, 1e-3 - radius], 'L_in'), when=Expr.parameter('filter').eq(3))

    r.component('L_out', 'L_monitor', parameters={
        'nL': 'nL', 
        'xwidth': 0.1, 
        'yheight': 0.1, 
        'Lmin': 'Lmin', 
        'Lmax': 'Lmax',
    }, at=([0, 0, Expr.parameter('zdepth')+1e-3], 'FilterA'))

    return r.instrument


def test_filters_are_similar():
    from .utilities import compile_and_scan, acceptable_total_counts

    instr = make_test_instrument()
    with open("Test_filters.instr", "w") as f:
        instr.to_file(f)

    filters = {
        "Filter_gen": 2183.87,
        "NCrystal_sample": 1875.02,
        "PowderN": 1868.5,
        "Radial_filter_collimator": 1875.02, # it should be identical to the NCrystal_sample in this configuration
    }
    results = compile_and_scan(instr, {'filter': '0:3'}, ncount=1e6, seed=1, use_temp_dir=True)
    filternames = list(filters.keys())
    l_out = {filternames[i]: res['L_out'] for i, res in enumerate(results['scan_result'])}

    for name, expected in filters.items():
        assert acceptable_total_counts(l_out[name], expected, tolerance=1.0), (
            f"Total counts for {name} differ from expected by more than 10%: "
            f"{l_out[name]['I'].sum()} vs {expected}"
        )