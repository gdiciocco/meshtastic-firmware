#!/usr/bin/env python3

import argparse
import difflib
import re
import sys
from pathlib import Path


SOURCE_EXTENSIONS = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx"}
IDENTIFIER = re.compile(r"[A-Za-z0-9_]")


def find_calls(source: str) -> list[tuple[int, int, list[str]]]:
    calls = []
    index = 0
    state = "code"

    while index < len(source):
        char = source[index]
        following = source[index + 1] if index + 1 < len(source) else ""

        if state == "line_comment":
            if char == "\n":
                state = "code"
            index += 1
            continue

        if state == "block_comment":
            if char == "*" and following == "/":
                state = "code"
                index += 2
            else:
                index += 1
            continue

        if state in {"string", "character"}:
            if char == "\\":
                index += 2
            elif (state == "string" and char == '"') or (state == "character" and char == "'"):
                state = "code"
                index += 1
            else:
                index += 1
            continue

        if char == "/" and following == "/":
            state = "line_comment"
            index += 2
            continue
        if char == "/" and following == "*":
            state = "block_comment"
            index += 2
            continue
        if char == '"':
            state = "string"
            index += 1
            continue
        if char == "'":
            state = "character"
            index += 1
            continue

        if source.startswith("strlcpy", index):
            before = source[index - 1] if index else ""
            after_name = index + len("strlcpy")
            after = source[after_name] if after_name < len(source) else ""
            if (before and IDENTIFIER.match(before)) or (after and IDENTIFIER.match(after)):
                index += 1
                continue

            open_paren = after_name
            while open_paren < len(source) and source[open_paren].isspace():
                open_paren += 1
            if open_paren < len(source) and source[open_paren] == "(":
                end, arguments = parse_arguments(source, open_paren)
                calls.append((index, end, arguments))
                index = end
                continue

        index += 1

    return calls


def parse_arguments(source: str, open_paren: int) -> tuple[int, list[str]]:
    arguments = []
    argument_start = open_paren + 1
    paren_depth = 1
    bracket_depth = 0
    brace_depth = 0
    index = argument_start
    state = "code"

    while index < len(source):
        char = source[index]
        following = source[index + 1] if index + 1 < len(source) else ""

        if state == "line_comment":
            if char == "\n":
                state = "code"
            index += 1
            continue
        if state == "block_comment":
            if char == "*" and following == "/":
                state = "code"
                index += 2
            else:
                index += 1
            continue
        if state in {"string", "character"}:
            if char == "\\":
                index += 2
            elif (state == "string" and char == '"') or (state == "character" and char == "'"):
                state = "code"
                index += 1
            else:
                index += 1
            continue

        if char == "/" and following == "/":
            state = "line_comment"
            index += 2
            continue
        if char == "/" and following == "*":
            state = "block_comment"
            index += 2
            continue
        if char == '"':
            state = "string"
            index += 1
            continue
        if char == "'":
            state = "character"
            index += 1
            continue

        if char == "(":
            paren_depth += 1
        elif char == ")":
            paren_depth -= 1
            if paren_depth == 0:
                arguments.append(source[argument_start:index].strip())
                if len(arguments) != 3 or any(not argument for argument in arguments):
                    raise ValueError(f"expected 3 arguments at offset {open_paren}")
                return index + 1, arguments
        elif char == "[":
            bracket_depth += 1
        elif char == "]":
            bracket_depth -= 1
        elif char == "{":
            brace_depth += 1
        elif char == "}":
            brace_depth -= 1
        elif char == "," and paren_depth == 1 and bracket_depth == 0 and brace_depth == 0:
            arguments.append(source[argument_start:index].strip())
            argument_start = index + 1

        index += 1

    raise ValueError(f"unterminated strlcpy call at offset {open_paren}")


def add_cstdio_include(source: str) -> str:
    if re.search(r"^\s*#\s*include\s*<cstdio>\s*$", source, re.MULTILINE):
        return source

    lines = source.splitlines(keepends=True)
    first_include = next(
        (index for index, line in enumerate(lines) if re.match(r"^\s*#\s*include\b", line)),
        None,
    )
    if first_include is None:
        raise ValueError("cannot add <cstdio>: no include block found")

    insert_at = first_include
    while insert_at < len(lines) and re.match(r"^\s*#\s*include\b", lines[insert_at]):
        insert_at += 1

    newline = "\r\n" if lines[first_include].endswith("\r\n") else "\n"
    lines.insert(insert_at, f"#include <cstdio>{newline}")
    return "".join(lines)


def transform(source: str) -> tuple[str, int]:
    calls = find_calls(source)
    transformed = source

    for start, end, arguments in reversed(calls):
        destination, source_value, size = arguments
        replacement = f'static_cast<size_t>(std::snprintf({destination}, {size}, "%s", {source_value}))'
        transformed = transformed[:start] + replacement + transformed[end:]

    if calls:
        transformed = add_cstdio_include(transformed)

    return transformed, len(calls)


def source_files(paths: list[Path]) -> list[Path]:
    files = []
    for path in paths:
        if path.is_file() and path.suffix.lower() in SOURCE_EXTENSIONS:
            files.append(path)
        elif path.is_dir():
            files.extend(
                candidate
                for candidate in path.rglob("*")
                if candidate.is_file() and candidate.suffix.lower() in SOURCE_EXTENSIONS
            )
        else:
            raise ValueError(f"path does not exist or is not a supported source file: {path}")
    return sorted(set(files))


def read_source(path: Path) -> str:
    with path.open("r", encoding="utf-8", newline="") as source_file:
        return source_file.read()


def write_source(path: Path, source: str) -> None:
    with path.open("w", encoding="utf-8", newline="") as source_file:
        source_file.write(source)


def main() -> int:
    parser = argparse.ArgumentParser(description="Replace non-portable strlcpy calls with std::snprintf.")
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--check", action="store_true", help="report occurrences and exit 1 when replacements are needed")
    mode.add_argument("--dry-run", action="store_true", help="print the proposed patch without changing files")
    parser.add_argument("paths", nargs="*", type=Path, default=[Path("src")], help="source files or directories")
    args = parser.parse_args()

    try:
        pending = []
        for path in source_files(args.paths):
            original = read_source(path)
            updated, count = transform(original)
            if count:
                pending.append((path, original, updated, count))
    except (OSError, UnicodeError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2

    total = sum(item[3] for item in pending)
    if args.check:
        for path, _, _, count in pending:
            print(f"{path}: {count} strlcpy call(s)")
        return 1 if total else 0

    if args.dry_run:
        for path, original, updated, _ in pending:
            sys.stdout.writelines(
                difflib.unified_diff(
                    original.splitlines(keepends=True),
                    updated.splitlines(keepends=True),
                    fromfile=str(path),
                    tofile=str(path),
                )
            )
    else:
        for path, _, updated, _ in pending:
            write_source(path, updated)

    action = "would replace" if args.dry_run else "replaced"
    print(f"{action} {total} strlcpy call(s) in {len(pending)} file(s)", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
