"""Use relative Fortran input paths so runtime diagnostics never embed a developer's home."""
import os
from pathlib import Path
import subprocess
import sys


def relative(value):
    return os.path.relpath(value) if Path(value).is_absolute() else value


arguments = []
for argument in sys.argv[2:]:
    if argument.startswith(("-I", "-J")) and len(argument) > 2:
        arguments.append(argument[:2] + relative(argument[2:]))
    elif not argument.startswith("-") and Path(argument).suffix.lower() in (".f", ".f90", ".for", ".f95"):
        arguments.append(relative(argument))
    else:
        arguments.append(argument)
raise SystemExit(subprocess.run([sys.argv[1], *arguments]).returncode)
