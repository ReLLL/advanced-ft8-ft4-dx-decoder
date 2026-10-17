"""One complete, optional native receive-mode payload shared by build and install tools."""
BASE_BINARIES = ("dx_modes_decoder", "dx_wspr_decoder", "libgfortran.5.dylib", "libquadmath.0.dylib", "libgcc_s.1.1.dylib")
NATIVE_BINARIES = (*BASE_BINARIES, "dx_js8_decoder")
NATIVE_FILES = (*BASE_BINARIES, "COPYING.RUNTIME", "COPYING.LIB", "mode-runtime.json")
JS8_FILES = ("dx_js8_decoder", "COPYING.JS8", "COPYING.QT", "js8-runtime.json")


def mode_files(folder):
    present = [name for name in NATIVE_FILES if (folder / name).is_file()]
    if present and len(present) != len(NATIVE_FILES):
        raise ValueError("Incomplete additional-mode decoder payload")
    js8 = [name for name in JS8_FILES if (folder / name).is_file()]
    if js8 and len(js8) != len(JS8_FILES):
        raise ValueError("Incomplete JS8 decoder payload")
    return (NATIVE_FILES if present else ()) + (JS8_FILES if js8 else ())
