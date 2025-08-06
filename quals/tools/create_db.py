#!/usr/bin/env python3
import subprocess
import os
import shutil


def generate_build_sh(build_path, source_root):
    """Generate a build.sh script in the given path that builds RTEMS."""
    content = f"""#!/bin/bash
    cd {source_root} && \\
    ./waf clean && \\
    ./waf build
    exit $?
"""
    with open(build_path, 'w') as f:
        f.write(content)
        os.chmod(build_path, 0o755)
        print(f"build.sh generated at {build_path}")


def create_codeql_database(db_name, source_root, build_sh_path):
    # Remove existing DB
    db_path = os.path.join(source_root, db_name)
    if os.path.exists(db_name):
        print(f"Removing existing database at {db_path}")
        shutil.rmtree(db_path)

    # Run database creation
    cmd = [
        "codeql", "database", "create",
        str(db_name), "--db-cluster", "--language=python,c-cpp", "--command",
        str(build_sh_path), "--no-run-unnecessary-builds", "--source-root",
        str(source_root)
    ]
    print(f" Running CodeQL Analysis with:")
    print(f" Database name      : {db_name}")
    print(f" Languages          :Python,C,CPP")
    print(f" Codebase Path      : {source_root}")
    print(f" build.sh_path      : {build_sh_path}")
    print(f"Running CodeQL database creation in: {db_path}")
    print("Command:", " ".join(cmd))
    try:
        subprocess.run(cmd, check=True)
        print(f"CodeQL database created successfully at {db_path}")
    except subprocess.CalledProcessError as e:
        print(
            f"CodeQl database creation failed with return code {e.returncode}")
        exit(1)
