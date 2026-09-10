#!/usr/bin/env python3
"""Audit owned C files for blank lines inside function bodies."""
from pathlib import Path


def _owned_files() -> list[Path]:
    """
    Find owned C and header files.

    Parameters
    ----------
    None

    Returns
    -------
    list[pathlib.Path]
        Owned non-generated C and header paths.
    """
    paths = Path(".").glob("**/*")
    return sorted(path for path in paths if _is_owned(path))


def _is_owned(path: Path) -> bool:
    """
    Determine whether a path is in the audit scope.

    Parameters
    ----------
    path : pathlib.Path
        Candidate source path.

    Returns
    -------
    bool
        True when the path is an owned C or header file.
    """
    name = str(path)
    excluded = (
        "crypto/ed25519" in name or "build/" in name or ".pio/" in name
    )
    return path.suffix in {".c", ".h"} and not excluded


def _function_spans(lines: list[str]) -> list[tuple[int, int]]:
    """
    Find approximate brace spans for C functions.

    Parameters
    ----------
    lines : list[str]
        Source lines.

    Returns
    -------
    list[tuple[int, int]]
        One-based function start and end lines.
    """
    spans = []
    depth = 0
    start = None
    for number, line in enumerate(lines, 1):
        start = _start_function(line, depth, number, start)
        depth += line.count("{") - line.count("}")
        if start is not None and depth <= 0:
            spans.append((start, number))
            start = None
    return spans


def _start_function(
    line: str, depth: int, number: int, start: int | None
) -> int | None:
    """
    Detect a function-opening line.

    Parameters
    ----------
    line : str
        Current source line.
    depth : int
        Current brace depth.
    number : int
        Current one-based line number.
    start : int or None
        Existing function start.

    Returns
    -------
    int or None
        Updated function start.
    """
    brace = line.find("{")
    found = brace >= 0 and ")" in line[:brace]
    return number if start is None and depth == 0 and found else start


def audit(path: Path) -> list[int]:
    """
    Find blank lines inside C function spans.

    Parameters
    ----------
    path : pathlib.Path
        C or header source path.

    Returns
    -------
    list[int]
        One-based blank-line numbers.
    """
    lines = path.read_text(encoding="utf-8").splitlines()
    spans = _function_spans(lines)
    return [
        number
        for number, line in enumerate(lines, 1)
        if not line.strip() and _inside(number, spans)
    ]


def _inside(number: int, spans: list[tuple[int, int]]) -> bool:
    """
    Check whether a line lies inside a function span.

    Parameters
    ----------
    number : int
        One-based source line number.
    spans : list[tuple[int, int]]
        Function start and end lines.

    Returns
    -------
    bool
        True when the line is inside a function body.
    """
    return any(start < number < end for start, end in spans)


def main() -> int:
    """
    Audit all owned C and header files.

    Parameters
    ----------
    None

    Returns
    -------
    int
        Zero for a clean audit, otherwise one.
    """
    failures = _report_failures()
    return int(bool(failures))


def _report_failures() -> list[str]:
    """
    Print C audit failures.

    Parameters
    ----------
    None

    Returns
    -------
    list[str]
        Failure paths.
    """
    failures = [str(path) for path in _owned_files() if audit(path)]
    for path in failures:
        print(path)
    return failures


if __name__ == "__main__":
    raise SystemExit(main())
