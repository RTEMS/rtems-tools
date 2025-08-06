#!/usr/bin/env python3

import csv
import fnmatch


def load_exclusion_patterns(exclusion_file):
    """Loads exclusion patterns from a text file, one pattern per line."""

    patterns = []
    try:
        with open(exclusion_file, 'r', encoding='utf-8') as input_file:
            for line in input_file:
                pattern = line.strip()
                if pattern:
                    patterns.append(pattern)
    except FileNotFoundError:
        print(f"Error: Exclusion file '{exclusion_file}' not found.")
    return patterns


def exclude_path(input_file, output_file, path_column_name,
                 exclusion_patterns_list):
    """
    Excludes paths from a CSV file based on glob patterns.

    Args:
        input_file (str): Path to input CSV file.
        output_file (str): Path to output CSV file.
        path_column_name (str): Name of the column containing paths.
        exclusion_patterns_list (list): A list of glob patterns to exclude.

    """
    excluded_count = 0
    excluded_path_detail = {}

    try:
        with open(input_file, 'r', encoding='utf-8', newline='') as infile:
            reader = csv.DictReader(infile)
            fieldnames = reader.fieldnames

            if path_column_name not in fieldnames:
                print(
                    f"Error: Path Column '{path_column_name}' not found in CSV header."
                )
                return

            all_rows = list(reader)

        with open(output_file, 'w', newline='', encoding='utf-8') as outfile:
            writer = csv.DictWriter(outfile, fieldnames=fieldnames)
            writer.writeheader()

            for row in all_rows:
                path_in_csv = row.get(path_column_name, '')
                should_exclude = False
                matched_pattern = None

                for pattern in exclusion_patterns_list:
                    if fnmatch.fnmatch(path_in_csv, pattern):
                        should_exclude = True
                        matched_pattern = pattern
                        break

                if not should_exclude:
                    writer.writerow(row)
                else:
                    excluded_count += 1
                    if matched_pattern not in excluded_path_detail:
                        excluded_path_detail[matched_pattern] = []
                    excluded_path_detail[matched_pattern].append(path_in_csv)

    except FileNotFoundError as e:
        print(f"Error: A file was not found: {e}")
        return
    except Exception as e:
        print(f"An unexpected error occurred during file processing: {e}")
        return

    print(
        f"Filtered data written to '{output_file}'. Total {excluded_count} rows were excluded.\n"
    )

    if excluded_path_detail:
        print("--- Details of Excluded Paths ---")
        # Corrected variable name from 'patterns' to 'pattern_key' for clarity
        for pattern_key, paths in excluded_path_detail.items():
            print(f"Pattern '{pattern_key}':")  # Corrected variable name here

            path_counts = {}
            for p in paths:
                path_counts[p] = path_counts.get(p, 0) + 1

            for p, count in path_counts.items():
                print(
                    f"  - {p} (Excluded {count} time{'s' if count > 1 else ''})"
                )
            print(
                f"  (Total unique paths for this pattern: {len(path_counts)})")
        print("-------------------------------")
    else:
        print("No paths were excluded based on the provided patterns.")
