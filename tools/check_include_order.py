#!/usr/bin/env python3
"""Enforce this project's include-ordering convention.

    Headers (.h/.hpp):  system/external includes go ABOVE project includes.
    Sources (.cpp/.cc): system/external includes go BELOW project includes.

`#include <...>` counts as system/external (this includes third-party
libraries such as <gtest/gtest.h>); `#include "..."` counts as project.

Why this is not a .clang-format rule: clang-format's IncludeCategories
priorities are global to a format run, so it cannot order includes one way in
headers and the other way in sources. .clang-format is set to
`IncludeBlocks: Preserve`, which sorts within a block but never moves an
include across blocks; the group order is enforced here instead.

Usage:
    check_include_order.py [path ...]      # defaults to the repo root

Exits 0 when clean, 1 on violations, 2 on bad usage.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

HEADER_SUFFIXES = {".h", ".hpp", ".hh", ".hxx"}
SOURCE_SUFFIXES = {".cpp", ".cc", ".cxx"}

# Directories that never contain first-party code.
EXCLUDED_DIR_NAMES = {".git", ".idea", "build", "_deps", "node_modules"}
EXCLUDED_DIR_PREFIXES = ("cmake-build",)

INCLUDE_RE = re.compile(r'^\s*#\s*include\s*([<"])')
COND_OPEN_RE = re.compile(r"^\s*#\s*(?:if|ifdef|ifndef)\b")
IFNDEF_RE = re.compile(r"^\s*#\s*ifndef\s+(\w+)")
DEFINE_RE = re.compile(r"^\s*#\s*define\s+(\w+)")
ENDIF_RE = re.compile(r"^\s*#\s*endif\b")

SYSTEM = "system"
PROJECT = "project"


def is_excluded(path: Path) -> bool:
    for part in path.parts:
        if part in EXCLUDED_DIR_NAMES or part.startswith(EXCLUDED_DIR_PREFIXES):
            return True
    return False


def iter_cxx_files(roots: list[Path]) -> list[Path]:
    wanted = HEADER_SUFFIXES | SOURCE_SUFFIXES
    found: set[Path] = set()
    for root in roots:
        if root.is_file():
            if root.suffix in wanted:
                found.add(root.resolve())
            continue
        for path in root.rglob("*"):
            if path.is_file() and path.suffix in wanted and not is_excluded(path):
                found.add(path.resolve())
    return sorted(found)


def next_meaningful_line(lines: list[str], start: int) -> str | None:
    """Return the next line that is not blank and not a // comment."""
    for line in lines[start:]:
        stripped = line.strip()
        if not stripped or stripped.startswith("//"):
            continue
        return line
    return None


def collect_includes(lines: list[str]) -> list[tuple[int, str]]:
    """Return (line_number, kind) for each top-level include.

    Includes nested inside #if/#ifdef blocks are skipped: platform-selection
    blocks (see network/tcp/src/Socket.cpp) are mutually exclusive branches
    rather than one ordered run, so the convention does not apply across them.

    An include guard (`#ifndef X` immediately followed by `#define X`) wraps
    the whole header and is deliberately not treated as a conditional --
    otherwise every header include would be skipped.
    """
    includes: list[tuple[int, str]] = []
    # Stack entries are True for a real conditional, False for an include guard.
    stack: list[bool] = []
    guard_consumed = False

    for index, line in enumerate(lines):
        if ENDIF_RE.match(line):
            if stack:
                stack.pop()
            continue

        if COND_OPEN_RE.match(line):
            is_guard = False
            ifndef_match = IFNDEF_RE.match(line)
            if ifndef_match and not guard_consumed and not any(stack) and not includes:
                following = next_meaningful_line(lines, index + 1)
                if following is not None:
                    define_match = DEFINE_RE.match(following)
                    if define_match and define_match.group(1) == ifndef_match.group(1):
                        is_guard = True
                        guard_consumed = True
            stack.append(not is_guard)
            continue

        include_match = INCLUDE_RE.match(line)
        if include_match and not any(stack):
            kind = SYSTEM if include_match.group(1) == "<" else PROJECT
            includes.append((index + 1, kind))

    return includes


def check_file(path: Path) -> list[str]:
    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except (OSError, UnicodeDecodeError) as exc:
        return [f"{path}: could not read file: {exc}"]

    includes = collect_includes(lines)
    if not includes:
        return []

    if path.suffix in HEADER_SUFFIXES:
        offending, expected_first, expected_second = SYSTEM, "system", "project"
    else:
        offending, expected_first, expected_second = PROJECT, "project", "system"

    # Walk the run once: after the first include of the *second* group appears,
    # no include of the *first* group may follow.
    problems: list[str] = []
    saw_second_group = False
    second_group_line = 0
    for line_number, kind in includes:
        if kind != offending:
            if not saw_second_group:
                saw_second_group = True
                second_group_line = line_number
        elif saw_second_group:
            problems.append(
                f"{path}:{line_number}: {expected_first} include appears after "
                f"{expected_second} include on line {second_group_line}; "
                f"in {path.suffix} files {expected_first} includes go "
                f"{'above' if path.suffix in HEADER_SUFFIXES else 'below'} {expected_second} includes"
            )
    return problems


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("paths", nargs="*", type=Path, help="files or directories to check (default: repo root)")
    args = parser.parse_args(argv)

    roots = args.paths or [Path(__file__).resolve().parent.parent]
    for root in roots:
        if not root.exists():
            print(f"error: no such file or directory: {root}", file=sys.stderr)
            return 2

    files = iter_cxx_files(roots)
    problems: list[str] = []
    for path in files:
        problems.extend(check_file(path))

    if problems:
        for problem in problems:
            print(problem, file=sys.stderr)
        print(f"\n{len(problems)} include-order violation(s) in {len(files)} file(s) checked.", file=sys.stderr)
        return 1

    print(f"include order OK ({len(files)} files checked)")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
