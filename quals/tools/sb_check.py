#!/usr/bin/env python3
import os
import subprocess


def is_codeql_in_path():
    """Check if CodeQL is in PATH and accessible."""
    try:
        result = subprocess.run(["codeql", "--version"],
                                check=True,
                                capture_output=True,
                                text=True)
        output = result.stdout.strip()
        first_line = output.splitlines()[0]
        version = first_line.split()[-1].strip(".")
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        return False
