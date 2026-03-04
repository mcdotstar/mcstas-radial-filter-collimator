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



# ---------------------------------------------------------------------------
# Compile + run helper
# ---------------------------------------------------------------------------

def compile_and_run(instr, parameters: str, verbose=False, use_temp_dir=True) -> bytes:
    """Compile *instr* and run it with the given parameter string.

    Returns combined stdout+stderr bytes from the binary.
    A RuntimeError is raised by mccode_antlr on compilation or run failure.
    """
    from pathlib import Path
    from tempfile import TemporaryDirectory, mkdtemp
    from mccode_antlr import Flavor
    from mccode_antlr.run import mccode_compile, mccode_run_compiled

    if use_temp_dir:
        with TemporaryDirectory() as build_dir:
            binary, target = mccode_compile(instr, Path(build_dir), flavor=Flavor.MCSTAS, dump_source=True)
            out_dir = Path(build_dir) / 'output'
            output, dats = mccode_run_compiled(
                binary, target, out_dir, parameters, capture=True,
            )
    else:
        binary, target = mccode_compile(instr, Path('.'), flavor=Flavor.MCSTAS, dump_source=True)
        out_dir = Path(mkdtemp(dir=Path('.'), prefix='mccode_run_')) # this creates the directory
        # but McCode refuses to write to an exising output folder (other than the working directory)
        out_dir.rmdir()  # so remove it again before running
        output, dats = mccode_run_compiled(
            binary, target, out_dir, parameters, capture=True,
        )

    if verbose:
        print(f"Output from {binary}:\n{output.decode(errors='replace')}")
        print(f"Data files generated in {out_dir}:\n{list(out_dir.glob('*.dat'))}")
        print(f'{dats=}')
    
    return output, dats
