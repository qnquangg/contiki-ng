# Unicast Downward Traffic

## Structure

The project contains two main applications:

- **root.c**: Acts as the root or sender node. It:
  - Initializes a set of child IPv6 addresses.
  - Starts a DAG root (for RPL-based routing).
  - Sends unicast messages to children in a round-robin fashion.
  - Inserts a delay (`SEND_INTERVAL`) between transmissions with some additional jitter.
  - Logs the message sent and the corresponding child destination.

- **sink.c**: Acts as the sink or receiver node. It:
  - Listens on the specified UDP port.
  - Logs the reception of any incoming message along with the current timestamp.

## Files

- **root.c**

  - Initializes a list of child addresses following a pattern (e.g., `fd00::202:2:2:2` for the first child).
  - Configures the UDP connection on port `1903`.
  - Implements a function `send_unicast_to_children()` to send messages (each message contains a message count and a timestamp).
  - Cycles through the list of child addresses for each message sent.
  - Uses a periodic timer (with added jitter) to schedule transmissions.

- **sink.c**

  - Registers a UDP connection on port `1903`.
  - Implements a callback `udp_rx_callback()` to log incoming messages with the reception timestamp.
  - Uses a simple event-based process to wait for incoming messages.

- **unicast-traffic-downward.csc**
  - Simulation.
