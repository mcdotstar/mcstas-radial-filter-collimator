from pathlib import Path
from collections.abc import Callable


def compiled(method):
    """Decorator: skip test if no working C compiler is available."""
    from pytest import mark
    from mccode_antlr.compiler.check import simple_instr_compiles
    if simple_instr_compiles('cc'):
        return method

    @mark.skip(reason=f"A working C compiler is required for {method.__name__}")
    def skipped(*args, **kwargs):
        pass

    return skipped


def repo_registry():
    """Return a LocalRegistry pointing at the root of this git repository."""
    from git import Repo, InvalidGitRepositoryError
    from mccode_antlr.reader.registry import LocalRegistry
    try:
        repo = Repo('.', search_parent_directories=True)
        return LocalRegistry('radial_filter_collimator', repo.working_tree_dir)
    except InvalidGitRepositoryError as exc:
        raise RuntimeError(f"Could not locate repository root: {exc}") from exc
    


def write_init_switch(
        assembler, 
        param_name: str, 
        cases: dict, 
        decorator: Callable | None = None, 
        leadin: str | None = None
        ):
    """Write an INITIALIZE block that prints the value of *param_name* and matches it to *cases*."""
    from textwrap import dedent
    if decorator is None:

        decorator = lambda x: f'printf(\"{x}\\n\")'

    cases_str = "\n".join(
        f"case {value}:\n    {decorator(case)}; break;"
        for value, case in cases.items()
    )
    init_code = dedent(f"""
        {leadin or ""}
        printf("\\n{assembler.instrument.name}: ");
        switch ({param_name}) {{
        {cases_str}
        default:
            printf("Invalid {param_name} value: %d\\n", {param_name}); 
            exit(-1);
            break;
        }}
    """)
    assembler.initialize(init_code)



def acceptable_total_counts(dat, expected, tolerance=0.1):
    return abs(dat['I'].sum() - expected) <= tolerance * expected



# ---------------------------------------------------------------------------
# Compile + run helper
# ---------------------------------------------------------------------------
def time(method):
    """Decorator: print and return the time taken by a test."""
    import time as time_module

    def timed(*args, **kwargs):
        start = time_module.perf_counter()
        try:
            result = method(*args, **kwargs)
        finally:
            end = time_module.perf_counter()
            print(f"{method.__name__} took {end - start:.2f} seconds")
        return end-start, result

    return timed


@time
def timed_compile(sim, dir: str | Path | None = None):
    if dir is None:
        return sim.compile()
    return sim.compile(directory=dir)


@time
def timed_run(sim, params: dict, ncount: int = 1000, seed: int = 1):
    return sim.run(params, ncount=ncount, seed=seed)


@time
def timed_scan(sim, params: dict, ncount: int = 1000, seed: int = 1):
    return sim.scan(params, ncount=ncount, seed=seed)


def compile_and_scan(instr, parameters: dict, ncount: int, seed: int = 1, use_temp_dir=True, grid=False, **args):
    """Compile *instr* and run the specified scan"""
    from mccode_antlr.run import McStas

    sim = McStas(instr)
    # sim.source()
    compile_time, _ = timed_compile(sim, dir=None if use_temp_dir else Path('.'))
    run_time, output_results_list = timed_scan(sim, parameters, ncount=ncount, seed=seed)
    scan_result = [results for output, results in output_results_list]

    return {'compile': compile_time, 'run': run_time, 'scan_result': scan_result}



def compile_and_run(instr, ncount: int, parameters: dict | None = None, seed: int = 1, use_temp_dir=True) -> bytes:
    """Compile *instr* and run it with the given parameter string.

    Returns combined stdout+stderr bytes from the binary.
    A RuntimeError is raised by mccode_antlr on compilation or run failure.
    """
    from mccode_antlr.run import McStas

    sim = McStas(instr)
    compile_time, _ = timed_compile(sim, dir=None if use_temp_dir else Path('.'))
    run_time, (output, results) = timed_run(sim, parameters, ncount=ncount, seed=seed)
    
    return {'compile': compile_time, 'run': run_time, 'output': output, 'data': results}
