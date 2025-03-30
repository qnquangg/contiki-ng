#!/usr/bin/env python3
import re
import sys

def time_str_to_ms(time_str):
    """
    Convert a time string in either "mm:ss.SSS" or "HH:mm:ss.SSS" format to total milliseconds.
    Example:
      "12:00.458"    -> hours=0, minutes=12, seconds=0, ms=458
      "1:02:18.558"  -> hours=1, minutes=02, seconds=18, ms=558
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
    # Updated regex patterns to match the provided log format.
    # Sender log sample: "12:00.458    ID:1    Send to multicast address: ff03::fc, data [0x00000042], packet_number #67"
    sender_pattern = re.compile(
        r"^(?P<time>\S+)\s+ID:1\s+Send to multicast address:\s+\S+,\s+data\s+\[(?P<data>0x[0-9a-fA-F]+)\],\s+packet_number\s+#(?P<packet_number>\d+)"
    )
    # Receiver log sample: "12:01.535    ID:11    In: [0x0000003f], TTL 255, total 20"
    receiver_pattern = re.compile(
        r"^(?P<time>\S+)\s+ID:(?P<receiver_id>\d+)\s+In:\s+\[(?P<data>0x[0-9a-fA-F]+)\],\s+TTL\s+\d+,\s+total\s+(?P<total_received>\d+)"
    )
    
    # Dictionaries for storing logs.
    # For sender logs, we use two dictionaries:
    #   1. sender_logs: keyed by packet_number (if needed later)
    #   2. sender_by_data: keyed by the data field (assumed to be unique for each packet)
    sender_logs = {}
    sender_by_data = {}
    total_message_sent = 0  # Will be updated to the largest packet_number found.

    # For each receiver node (IDs 2-21), store:
    #   - max_total_received: the largest total_received value observed
    #   - last_entry: the last matching entry (latest receive time) with send/receive times and computed latency
    #   - latencies: list of latencies (in ms) for that receiver.
    receiver_data = {rid: {'max_total_received': 0,
                           'last_entry': None,
                           'latencies': []} for rid in range(2, 22)}
    
    # Process the log file line by line.
    with open(log_filepath, "r") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue

            # Try to match a sender log line.
            sender_match = sender_pattern.match(line)
            if sender_match:
                time_str = sender_match.group("time")
                try:
                    send_time_ms = time_str_to_ms(time_str)
                except ValueError:
                    continue  # Skip lines with unexpected time format
                data_field = sender_match.group("data")
                packet_number = int(sender_match.group("packet_number"))
                # Record the sender log information.
                entry = {
                    'send_time_ms': send_time_ms,
                    'time_str': time_str,
                    'data': data_field,
                    'packet_number': packet_number
                }
                sender_logs[packet_number] = entry
                sender_by_data[data_field] = entry

                # Update the overall total messages sent (using the largest packet_number).
                if packet_number > total_message_sent:
                    total_message_sent = packet_number
                continue  # Next log line
            
            # Try to match a receiver log line.
            receiver_match = receiver_pattern.match(line)
            if receiver_match:
                time_str = receiver_match.group("time")
                try:
                    receive_time_ms = time_str_to_ms(time_str)
                except ValueError:
                    continue
                receiver_id = int(receiver_match.group("receiver_id"))
                data_field = receiver_match.group("data")
                total_received = int(receiver_match.group("total_received"))
                
                # Only process receiver IDs 2 through 21.
                if receiver_id < 2 or receiver_id > 21:
                    continue

                # Update the maximum total_received for this receiver.
                if total_received > receiver_data[receiver_id]['max_total_received']:
                    receiver_data[receiver_id]['max_total_received'] = total_received

                # Attempt to find the matching sender entry using the data field.
                sender_entry = sender_by_data.get(data_field)
                if sender_entry:
                    send_time_ms = sender_entry['send_time_ms']
                    # Calculate latency only if the sender's time is earlier than receiver's time.
                    if receive_time_ms >= send_time_ms:
                        latency_ms = receive_time_ms - send_time_ms
                        receiver_data[receiver_id]['latencies'].append(latency_ms)
                        # For output, keep the entry with the latest receive_time.
                        last_entry = receiver_data[receiver_id]['last_entry']
                        if (last_entry is None) or (receive_time_ms > last_entry['receive_time_ms']):
                            receiver_data[receiver_id]['last_entry'] = {
                                'send_time_ms': send_time_ms,
                                'receive_time_ms': receive_time_ms,
                                'latency_ms': latency_ms,
                                'send_time_str': sender_entry['time_str'],
                                'receive_time_str': time_str
                            }
                continue  # Next log line

    # Prepare the output lines.
    # Format: ID, total_message_received, PRR, receive_time, send_time, Latency
    output_lines = []
    header = "ID, total_message_received, PRR, receive_time, send_time, Latency"
    output_lines.append(header)
    
    for rid in range(2, 22):
        rdata = receiver_data[rid]
        total_recv = rdata['max_total_received']
        # Calculate PRR: (total_received / total_message_sent) * 100.
        prr = (total_recv / total_message_sent * 100) if total_message_sent else 0
        prr_str = f"{prr:.2f}%"
        
        # If we have a matching sender/receiver pair, format times as numbers.
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
        
        line = f"{rid}, {total_recv}, {prr_str}, {receive_time_out}, {send_time_out}, {latency_out}"
        output_lines.append(line)
    
    # Append final line with the total number of messages sent.
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
