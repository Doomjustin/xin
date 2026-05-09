#!/usr/bin/env python3
"""
Sort C++20 import statements and format C++ files with clang-format.
Module imports are grouped by priority and sorted alphabetically within groups.
Then applies clang-format for consistent formatting.
"""

import re
import subprocess
import sys
from pathlib import Path
from typing import List, Tuple


def categorize_import(import_stmt: str) -> Tuple[int, str]:
    """
    Categorize an import statement and return (priority, import_name).

    Priority levels (lower = earlier):
    1. Standard library (std, std.compat, etc.)
    2. Third-party libraries (fmt, glaze, etc.)
    3. Project modules (xin.*, ocean.*, etc.)
    """
    # Extract module name from "import <name>;"
    match = re.match(r"import\s+([a-zA-Z0-9_.]+)\s*;", import_stmt.strip())
    if not match:
        return (999, import_stmt)  # Unknown format, keep at end

    module_name = match.group(1)

    # Priority 1: Standard library modules
    if module_name.startswith("std"):
        return (1, import_stmt)

    # Priority 2: Third-party libraries (fmt, glaze, asio, etc.)
    third_party = {
        "fmt",
        "glaze",
        "asio",
        "catch2",
        "spdlog",
        "magic_enum",
        "ctre",
        "gsl",
    }
    if any(module_name.startswith(lib) for lib in third_party):
        return (2, import_stmt)

    # Priority 3: Project modules (xin.*, ocean.*, etc.)
    if module_name.startswith(("xin", "ocean")):
        return (3, import_stmt)

    # Default: other modules
    return (4, import_stmt)


def sort_cpp_imports(file_path: Path) -> bool:
    """
    Sort import statements in a C++ file.
    Collects all import statements, sorts them, and reinserts as a block.
    Limits blank lines after imports to maximum 2 lines.

    Returns True if file was modified, False otherwise.
    """
    content = file_path.read_text(encoding="utf-8")
    lines = content.splitlines(keepends=True)

    # Find all imports
    imports: List[str] = []
    import_indices: List[int] = []

    for i, line in enumerate(lines):
        stripped = line.strip()
        if stripped.startswith("import ") and stripped.endswith(";"):
            imports.append(line)
            import_indices.append(i)

    if not imports:
        return False  # No imports found

    # Check if already sorted
    original_imports = imports.copy()
    sorted_imports = sort_import_group(imports)

    if original_imports == sorted_imports:
        return False  # Already in correct order

    # Remove all import lines from the file
    result_lines = [line for i, line in enumerate(lines) if i not in import_indices]

    # Find where to reinsert sorted imports (position of first import in original)
    first_import_line = import_indices[0]

    # Calculate insertion point in result_lines
    # Count how many lines before first_import_line are kept
    insert_pos = sum(1 for i in range(first_import_line) if i not in import_indices)

    # Insert sorted imports with blank lines between them
    insert_block = []
    for imp in sorted_imports:
        insert_block.append(imp)

    result_lines[insert_pos:insert_pos] = insert_block

    # Clean up: limit blank lines after imports to maximum 2
    # Find the first non-blank line after imports
    last_import_pos = insert_pos + len(insert_block) - 1
    blank_count = 0
    first_code_pos = None

    for i in range(last_import_pos + 1, len(result_lines)):
        if result_lines[i].strip() == "":
            blank_count += 1
        else:
            first_code_pos = i
            break

    # Remove excess blank lines (keep only max 2)
    if first_code_pos is not None and blank_count > 2:
        # Delete excess blank lines
        excess = blank_count - 2
        del result_lines[last_import_pos + 1 : last_import_pos + 1 + excess]

    new_content = "".join(result_lines)
    if new_content != content:
        file_path.write_text(new_content, encoding="utf-8")
        return True

    return False


def sort_import_group(imports: List[str]) -> List[str]:
    """
    Sort import statements by priority with blank line separators between groups.
    Groups:
    1. Standard library (std, std.compat, etc.)
    2. Third-party libraries
    3. Project modules
    4. Other modules
    """
    # Categorize each import
    categorized = [categorize_import(imp) for imp in imports]

    # Sort by priority first, then by module name
    categorized.sort(key=lambda x: (x[0], extract_module_name(x[1])))

    # Build result with blank lines between priority groups
    result = []
    current_priority = None

    for priority, imp in categorized:
        # Add blank line when priority changes
        if current_priority is not None and priority != current_priority:
            result.append("\n")
        result.append(imp)
        current_priority = priority

    return result


def extract_module_name(import_stmt: str) -> str:
    """Extract module name from import statement for sorting."""
    match = re.match(r"import\s+([a-zA-Z0-9_.]+)", import_stmt.strip())
    return match.group(1) if match else import_stmt


def main():
    if len(sys.argv) < 2:
        print("Usage: sort_imports.py <file.cpp> [file2.cpp ...]")
        sys.exit(1)

    processed_files = []
    for file_arg in sys.argv[1:]:
        file_path = Path(file_arg)
        if not file_path.exists():
            print(f"Warning: {file_path} not found", file=sys.stderr)
            continue

        # Step 1: Apply clang-format first
        try:
            result = subprocess.run(
                ["clang-format", "-i", str(file_path)],
                capture_output=True,
                text=True,
                timeout=10,
            )
            if result.returncode == 0:
                print(f"Formatted: {file_path}")
            else:
                print(
                    f"Warning: clang-format failed for {file_path}: {result.stderr}",
                    file=sys.stderr,
                )
        except FileNotFoundError:
            print("Warning: clang-format not found in PATH", file=sys.stderr)
        except subprocess.TimeoutExpired:
            print(f"Warning: clang-format timed out for {file_path}", file=sys.stderr)

        # Step 2: Sort imports after formatting
        if sort_cpp_imports(file_path):
            processed_files.append(str(file_path))
            print(f"Sorted imports: {file_path}")

    if processed_files:
        print(f"\nProcessed {len(processed_files)} file(s)")
        return 0
    else:
        print("No import changes needed")
        return 0


if __name__ == "__main__":
    sys.exit(main())
