"""One complete, optional native receive-mode payload shared by build and install tools."""
NATIVE_BINARIES = ("dx_modes_decoder", "dx_wspr_decoder", "libgfortran.5.dylib", "libquadmath.0.dylib", "libgcc_s.1.1.dylib")
NATIVE_FILES = (*NATIVE_BINARIES, "COPYING.RUNTIME", "COPYING.LIB", "mode-runtime.json")


def mode_files(folder):
    present = [name for name in NATIVE_FILES if (folder / name).is_file()]
    if present and len(present) != len(NATIVE_FILES):
        raise ValueError("Incomplete additional-mode decoder payload")
    return NATIVE_FILES if present else ()
