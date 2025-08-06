#!/usr/bin/env python3
import csv
import os
import sys


def format_csv(input_file_path):
    """
    Adds headers to a CSV file, truncates the 'Message' column if it exceeds
    a defined limit, and removes the original file.
    """
    CELL_LIMIT = 31000
    TRUNCATION_MESSAGE = "... [Content Truncated]"
    csv.field_size_limit(sys.maxsize)
    headers = [
        "Name", "Description", "Severity", "Message", "Path", "Start line",
        "Start column", "End line", "End column"
    ]
    message_column_index = headers.index("Message")
    base, ext = os.path.splitext(input_file_path)
    output_file_path = f"{base}_formatted.csv"

    try:
        with open(input_file_path, "r", newline="") as infile, \
             open(output_file_path, "w", newline="") as outfile:

            reader = csv.reader(infile)
            writer = csv.writer(outfile)

            writer.writerow(headers)

            for row in reader:
                if len(row) > message_column_index:
                    if len(row[message_column_index]) > CELL_LIMIT:
                        original_message = row[message_column_index]
                        truncated_message = original_message[:CELL_LIMIT] + TRUNCATION_MESSAGE
                        row[message_column_index] = truncated_message

                writer.writerow(row)

        os.remove(input_file_path)
        print(f"Formatted CSV saved as {output_file_path}")
        print(f"Original file '{input_file_path}' has been removed.")

    except FileNotFoundError:
        print(f"Error: Input file not found at {input_file_path}")
        sys.exit(1)
    except Exception as e:
        print(f"An error occurred during CSV formatting: {e}")
        sys.exit(1)
