# DS/DSi wireless link protocol

The legacy raw NiFi backend now uses a versioned, bounds-checked packet format.
The native 3DS build also exposes a LAN backend using the same protocol model.

## Protocol v2

Every packet carries:

- `YOB2` magic and protocol version;
- host/session ID and sender ROM identifier;
- packet sequence and exact acknowledged sequence;
- declared payload length;
- fragment metadata and total transfer length;
- CRC-32 over the complete header and payload.

Packets with truncated payloads, invalid fragment ranges, unsupported versions,
or a bad checksum are rejected before command data is accessed. Host/client
identity strings use explicit lengths and capacity checks instead of
`strlen`/`strcpy` on received data.

## Loss and synchronization handling

- Critical handshakes and SRAM fragments require an exact-sequence ACK and are
  retried with a bounded timeout.
- Input history is retained in a 64-frame ring with exact frame tags.
- Missing inputs produce an explicit retransmission request.
- Duplicate and out-of-order input packets cannot overwrite a different frame
  that happens to share the same ring slot.
- A deterministic state hash is exchanged every 60 frames; mismatches are
  logged with the first detected frame.
- Initial SRAM transfer is length-checked and reliable.

## Native Nintendo 3DS LAN backend

The 3DSX build initializes the 3DS SOC service and uses non-blocking UDP on
port 35553. It supports broadcast room discovery, host/client confirmation,
link-cable and SGB multiplayer modes, fragmented initial SRAM transfer,
ACK/retry, missing-input requests, state hashes, and connection timeout.
Separate local and remote Game Boy instances are retained as in the DS design.

This is LAN communication, not Nintendo UDS. Both systems must be on the same
IPv4 network and the access point/firewall must permit UDP broadcast and port
35553. The `.nds` raw-802.11 NiFi transport cannot directly join a native 3DS
LAN room; a transport bridge would be a separate project.

## Remaining validation

The protocol and its portable encoder/decoder have automated regression tests,
but the following radio matrix still requires physical systems:

- DS to DS;
- DSi to DSi;
- Nintendo 3DS in DS mode to DS/DSi.

Native 3DS-to-3DS LAN play also requires a physical two-system validation pass.
Native 3DS-to-`.nds` interoperability remains deferred because the transports
are different even though their packet semantics are related.
