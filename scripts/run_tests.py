#!/usr/bin/env python3
"""Run PlatformIO native unit test suite."""
import shutil
import subprocess
import sys
from pathlib import Path


def _pio_bin() -> str:
    """
    Locate PlatformIO executable.

    Parameters
    ----------
    None

    Returns
    -------
    str
        Path to platformio binary.
    """
    custom = Path.home() / ".platformio/penv/bin/platformio"
    if custom.is_file():
        return str(custom)
    return shutil.which("pio") or "platformio"


def main() -> int:
    """
    Execute native tests.

    Parameters
    ----------
    None

    Returns
    -------
    int
        Return code from test execution.
    """
    cmd = [_pio_bin(), "test", "-e", "native"]
    return subprocess.run(cmd).returncode


if __name__ == "__main__":
    sys.exit(main())
