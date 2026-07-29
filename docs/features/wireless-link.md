# DS/DSi wireless link protocol

The legacy raw NiFi backend now uses a versioned, bounds-checked packet format.
This work applies to the `.nds` build. Native 3DS networking remains a separate
future backend.

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

## Remaining validation

The protocol and its portable encoder/decoder have automated regression tests,
but the following radio matrix still requires physical systems:

- DS to DS;
- DSi to DSi;
- Nintendo 3DS in DS mode to DS/DSi.

The native 3DS build does not expose this raw NiFi backend. A 3DS UDS or LAN
implementation, including any bridge to `.nds` peers, remains deferred.
