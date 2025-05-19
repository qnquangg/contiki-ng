#!/usr/bin/env python3
import re
import sys

def parse_power_log(file_path):
    """
    Parses the power consumption log file to extract node ID and power consumption.

    Args:
        file_path (str): The path to the log file.

    Returns:
        list: A list of tuples, where each tuple contains (node_id, power_consumption_percentage_string).
              Returns an empty list if the file cannot be read or no data is found.
    """
    results = []
    try:
        with open(file_path, 'r') as f:
            # Regex to find lines like "Contiki_X ON ... Y %"
            # It captures the node number (X) and the percentage (Y)
            regex = r"Contiki_(\d+) ON .* (\d+\.\d{2}) %"
            for line in f:
                match = re.search(regex, line)
                if match:
                    node_id = match.group(1)
                    power_consumption = match.group(2) + "%"
                    results.append((node_id, power_consumption))
    except FileNotFoundError:
        print(f"Error: File not found at {file_path}", file=sys.stderr)
        return []
    except Exception as e:
        print(f"An error occurred: {e}", file=sys.stderr)
        return []
    return results

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python parse_power_consumption.py <logfile>", file=sys.stderr)
        sys.exit(1)

    log_file = sys.argv[1]
    parsed_data = parse_power_log(log_file)

    if parsed_data:
        # Print CSV header
        print("ID,PowerConsumption")
        for node_id, power in parsed_data:
            print(f"{node_id},{power}")