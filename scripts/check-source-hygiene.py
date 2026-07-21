#!/usr/bin/env python3

from pathlib import Path
import re
import sys

roots = (Path("apps"), Path("engine"), Path("tests"))
suffixes = {".c", ".cc", ".cpp", ".cxx", ".h", ".hpp"}

patterns = (
    re.compile(r"<<\s*'\n'\s*;"),
    re.compile(r'"[^"\n]*\n\s*";'),
)

errors: list[str] = []

for root in roots:
    for path in sorted(root.rglob("*")):
        if not path.is_file() or path.suffix not in suffixes:
            continue

        text = path.read_text(encoding="utf-8")

        for pattern in patterns:
            if pattern.search(text):
                errors.append(
                    f"{path}: broken multiline literal"
                )

if errors:
    print("\n".join(errors))
    sys.exit(1)
