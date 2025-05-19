#!/usr/bin/env python3
import re
import sys

def time_str_to_ms(time_str):
    """
    Convert a time string in either "mm:ss.SSS" or "HH:mm:ss.SSS" format to total milliseconds.
    Example: "53:18.558" => 0h 53m 18.558s, "1:02:18.558" => 1h 02m 18.558s.
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
    # Regular expression for sender logs.
    # Matches lines like:
    # "04:39.767  ID:21 Send to node ID:17, address is fd00::211:11:11:11 and packet_number #55"
    sender_pattern = re.compile(
        r"^(?P<time>\S+)\s+ID:22\s+Send to node ID:(?P<receiver_id>\d+),\s+address is\s+(?P<ipv6_address>\S+)\s+and packet_number #(?P<packet_number>\d+)"

    )
    
    # Regular expression for receiver logs.
    # Matches lines like:
    # "04:39.768    ID:17 Received from Sink with message: 'Send from sink with packet_number 55', total_received 51"
    # or similar variations.
    receiver_pattern = re.compile(
        r"^(?P<time>\S+)\s+ID:(?P<receiver_id>\d+)\s+Received from (?:Sink|node \S+)\s+with message: 'Send from (?:Sink|sink) with packet_number (?P<packet_number>\d+)',\s+total_received\s+(?P<total_received>\d+)",
        re.IGNORECASE
    )
    
    # Dictionary to store sender log entries.
    # Key: (receiver_id, packet_number), Value: dict with send_time (ms) and original time string.
    sender_logs = {}
    total_message_sent = 0  # Will be updated to the largest packet_number found in sender logs.
    
    # Dictionary to store receiver data for nodes 1-20.
    # For each receiver, we store:
    #   - max_total_received: the largest total_received value seen
    #   - last_entry: latest matching entry (with send/receive times and calculated latency)
    #   - latencies: list of individual latency values (ms) for further analysis if needed.
    receiver_data = {}
    for rid in range(1, 21):
        receiver_data[rid] = {
            'max_total_received': 0,
            'last_entry': None,
            'latencies': []
        }
    
    # Process the log file.
    with open(log_filepath, "r") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue

            # Check if this is a sender log entry.
            sender_match = sender_pattern.match(line)
            if sender_match:
                time_str = sender_match.group("time")
                try:
                    send_time_ms = time_str_to_ms(time_str)
                except ValueError:
                    continue  # Skip if the time format is not as expected.
                receiver_id = int(sender_match.group("receiver_id"))
                packet_number = int(sender_match.group("packet_number"))
                # Record the sender log entry.
                sender_logs[(receiver_id, packet_number)] = {
                    'send_time_ms': send_time_ms,
                    'time_str': time_str
                }
                # Update the total message sent if this packet number is larger.
                if packet_number > total_message_sent:
                    total_message_sent = packet_number
                continue  # Move to the next line.
            
            # Check if this is a receiver log entry.
            receiver_match = receiver_pattern.match(line)
            if receiver_match:
                time_str = receiver_match.group("time")
                try:
                    receive_time_ms = time_str_to_ms(time_str)
                except ValueError:
                    continue
                receiver_id = int(receiver_match.group("receiver_id"))
                # We only process receiver IDs 1-20.
                if receiver_id < 1 or receiver_id > 20:
                    continue
                packet_number = int(receiver_match.group("packet_number"))
                total_received = int(receiver_match.group("total_received"))
                
                # Update the maximum total_received value for this receiver.
                if total_received > receiver_data[receiver_id]['max_total_received']:
                    receiver_data[receiver_id]['max_total_received'] = total_received
                
                # Calculate latency if a matching sender entry exists.
                key = (receiver_id, packet_number)
                if key in sender_logs:
                    send_entry = sender_logs[key]
                    send_time_ms = send_entry['send_time_ms']
                    latency_ms = receive_time_ms - send_time_ms
                    receiver_data[receiver_id]['latencies'].append(latency_ms)
                    
                    # Save the latest (largest receive time) entry for output.
                    last_entry = receiver_data[receiver_id]['last_entry']
                    if (last_entry is None) or (receive_time_ms > last_entry['receive_time_ms']):
                        receiver_data[receiver_id]['last_entry'] = {
                            'send_time_ms': send_time_ms,
                            'receive_time_ms': receive_time_ms,
                            'latency_ms': latency_ms,
                            'send_time_str': send_entry['time_str'],
                            'receive_time_str': time_str
                        }
                # If no matching sender log is found, we skip latency calculation.
                continue

    # Prepare output in the requested comma-separated format.
    output_lines = []
    header = "ID, total_message_received, PRR, receive_time, send_time, Latency"
    output_lines.append(header)
    
    for rid in range(1, 21):
        rdata = receiver_data[rid]
        total_received = rdata['max_total_received']
        # Compute PRR = (total_received / total_message_sent) * 100.
        prr = (total_received / total_message_sent * 100) if total_message_sent else 0
        prr_str = f"{prr:.2f}%"
        
        if rdata['last_entry'] is not None:
            send_time_ms = rdata['last_entry']['send_time_ms']
            receive_time_ms = rdata['last_entry']['receive_time_ms']
            latency_ms = rdata['last_entry']['latency_ms']
            send_time_out = f"{send_time_ms}"
            receive_time_out = f"{receive_time_ms}"
            latency_out = f"{latency_ms:.2f} ms"
        else:
            send_time_out = "N/A"
            receive_time_out = "N/A"
            latency_out = "N/A"
        
        line = f"{rid}, {total_received}, {prr_str}, {receive_time_out}, {send_time_out}, {latency_out}"
        output_lines.append(line)
    
    # Add final line indicating total messages sent.
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
