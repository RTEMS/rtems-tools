#!/usr/bin/env python3
import os
import shutil


def delete_codeql_db(db_path):
    """Deletes a CodeQL database directory if it exists."""
    db_path = os.path.abspath(os.path.expanduser(db_path))

    if os.path.exists(db_path):
        if os.path.isdir(db_path):
            print(f"Deleting CodeQl database at: {db_path}")
            shutil.rmtree(db_path)
            print("Database deleted successfully.")
        else:
            print(f"Error: {db_path} exists but is not a directory.")
    else:
        print(f"No CodeQL database found at: {db_path}")
