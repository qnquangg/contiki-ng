#!/usr/bin/env python3
import re
import sys

def time_str_to_ms(time_str):
    """
    Convert a time string in either "mm:ss.SSS" or "HH:mm:ss.SSS" format to total milliseconds.
    """
    parts = time_str.split(':')
    if len(parts) == 3:
        # Format: HH:mm:ss.SSS
        hours = int(parts[0])
        minutes = int(parts[1])
        sec_part = parts[2]
    elif len(parts) == 2:
        # Format: mm:ss.SSS (assume hours = 0)
        hours = 0
        minutes = int(parts[0])
        sec_part = parts[1]
    else:
        raise ValueError(f"Unexpected time format: {time_str}")
    
    if '.' in sec_part:
        seconds_str, ms_str = sec_part.split('.')
    else:
        seconds_str = sec_part
        ms_str = "0"
    seconds = int(seconds_str)
    milliseconds = int(ms_str)
    total_ms = ((hours * 3600) + (minutes * 60) + seconds) * 1000 + milliseconds
    return total_ms

def main(log_filepath):
    # Regular expressions to match sender and receiver log entries.
    sender_pattern = re.compile(
        r"^(?P<time>\S+)\s+ID:1\s+Send to node ID:(?P<receiver_id>\d+),\s+address is\s+(?P<ipv6_address>\S+)\s+and packet_number #(?P<packet_number>\d+)"
    )
    receiver_pattern = re.compile(
        r"^(?P<time>\S+)\s+ID:(?P<receiver_id>\d+)\s+Received from Root with message: 'Send from Root \(ID:1\) with packet_number (?P<packet_number>\d+)',\s+total_received\s+(?P<total_received>\d+)"
    )
    
    # Dictionaries to store the extracted information.
    # For sender logs, key = (receiver_id, packet_number) and value = dict with send time in ms and original string.
    sender_logs = {}
    total_message_sent = 0  # We'll use the largest packet_number found in sender logs.
    
    # For receiver logs, we will store per receiver node:
    #   - max_total_received: largest total_received value seen
    #   - last_entry: the last matching entry (with send/receive times and calculated latency)
    #   - latencies: list of latencies (if needed for further analysis)
    receiver_data = {}
    for rid in range(2, 22):
        receiver_data[rid] = {
            'max_total_received': 0,
            'last_entry': None,  # Will store a dict with keys: send_time_ms, receive_time_ms, latency_ms, send_time_str, receive_time_str
            'latencies': []
        }
    
    # Process the log file.
    with open(log_filepath, "r") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            # Check for sender log
            sender_match = sender_pattern.match(line)
            if sender_match:
                time_str = sender_match.group("time")
                try:
                    send_time_ms = time_str_to_ms(time_str)
                except ValueError:
                    continue  # Skip lines with unexpected time format
                receiver_id = int(sender_match.group("receiver_id"))
                packet_number = int(sender_match.group("packet_number"))
                # Record the sender log entry
                sender_logs[(receiver_id, packet_number)] = {
                    'send_time_ms': send_time_ms,
                    'time_str': time_str
                }
                # Update the overall total messages sent by sender based on packet number.
                if packet_number > total_message_sent:
                    total_message_sent = packet_number
                continue  # Next line
            
            # Check for receiver log
            receiver_match = receiver_pattern.match(line)
            if receiver_match:
                time_str = receiver_match.group("time")
                try:
                    receive_time_ms = time_str_to_ms(time_str)
                except ValueError:
                    continue
                receiver_id = int(receiver_match.group("receiver_id"))
                packet_number = int(receiver_match.group("packet_number"))
                total_received = int(receiver_match.group("total_received"))
                
                # Only consider receiver IDs 2-21.
                if receiver_id < 2 or receiver_id > 21:
                    continue
                
                # Update maximum total_received for this receiver.
                if total_received > receiver_data[receiver_id]['max_total_received']:
                    receiver_data[receiver_id]['max_total_received'] = total_received
                
                # Try to get the corresponding sender log for latency calculation.
                key = (receiver_id, packet_number)
                if key in sender_logs:
                    send_entry = sender_logs[key]
                    send_time_ms = send_entry['send_time_ms']
                    latency_ms = receive_time_ms - send_time_ms
                    # Save latency for later average if needed.
                    receiver_data[receiver_id]['latencies'].append(latency_ms)
                    # For output, we keep the last (i.e., latest receive_time) entry.
                    last_entry = receiver_data[receiver_id]['last_entry']
                    if (last_entry is None) or (receive_time_ms > last_entry['receive_time_ms']):
                        receiver_data[receiver_id]['last_entry'] = {
                            'send_time_ms': send_time_ms,
                            'receive_time_ms': receive_time_ms,
                            'latency_ms': latency_ms,
                            'send_time_str': send_entry['time_str'],
                            'receive_time_str': time_str
                        }
                # If no matching sender log, we might record the receiver info without latency.
                continue

    # Prepare output.
    # The output format is:
    # ID, total_message_received, PRR, receive_time, send_time, Latency
    # where receive_time and send_time are numerical representations (ms since start) and latency is in ms.
    output_lines = []
    header = "ID, total_message_received, PRR, receive_time, send_time, Latency"
    output_lines.append(header)
    
    for rid in range(2, 22):
        rdata = receiver_data[rid]
        total_received = rdata['max_total_received']
        # Calculate PRR. If total_message_sent is zero, then PRR is 0.
        if total_message_sent:
            prr = (total_received / total_message_sent) * 100
        else:
            prr = 0
        prr_str = f"{prr:.2f}%"
        
        if rdata['last_entry'] is not None:
            send_time_ms = rdata['last_entry']['send_time_ms']
            receive_time_ms = rdata['last_entry']['receive_time_ms']
            latency_ms = rdata['last_entry']['latency_ms']
            # Format times as integers and latency with two decimal places.
            send_time_out = f"{send_time_ms}"
            receive_time_out = f"{receive_time_ms}"
            latency_out = f"{latency_ms:.2f} ms"
        else:
            send_time_out = "N/A"
            receive_time_out = "N/A"
            latency_out = "N/A"
        
        line = f"{rid}, {total_received}, {prr_str}, {receive_time_out}, {send_time_out}, {latency_out}"
        output_lines.append(line)
    
    # Add final line for total messages sent.
    output_lines.append(f"total_message_sent = {total_message_sent}")
    
    # Print the output.
    for l in output_lines:
        print(l)

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python analyze_log.py <log_filepath>")
        sys.exit(1)
    log_filepath = sys.argv[1]
    main(log_filepath)
