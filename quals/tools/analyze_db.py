#!/usr/bin/env python3
import subprocess
from pathlib import Path


def find_codeql_path():
    """Find codeql path if exists"""
    try:
        result = subprocess.run(["which", "codeql"],
                                check=True,
                                capture_output=True)
        codeql_path = result.stdout.decode().strip()
        print(f"Using CodeQL at: {codeql_path}")
        return Path(codeql_path)
    except (subprocess.CalledProcessError, FileNotFoundError):
        print("CodeQL not found in PATH.")
        exit(1)


def run_analysis(db_path, output_path, query_file, ram, threads, csv_format):

    codeql_path = find_codeql_path().resolve()
    command = [
        str(codeql_path), "database", "analyze",
        str(db_path),
        str(query_file), "--format=csv", "--output",
        str(output_path), "--ram",
        str(ram), "--threads",
        str(threads), "--csv-location-format", csv_format
    ]

    print(f" Running CodeQL Analysis with:")
    print(f" Database           : {db_path}")
    print(f" Query              : {query_file}")
    print(f" Output             : {output_path}")
    print(f" RAM                : {ram} MB")
    print(f" Threads            : {threads}")
    print(f" CSV Format         : {csv_format}")
    print("  Command        :", " ".join(command))

    try:
        subprocess.run(command, check=True)
        print(f"Analysis complete.Results saved to {output_path}")
        return True
    except subprocess.CalledProcessError as e:
        print(f"Analysis failed with exit code {e.returncode}")
        print("--- CodeQL Error Output ---")
        return False
