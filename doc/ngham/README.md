# Task 1 — NGHam Protocol in Python (PyNGHam)

## Overview

Prior to implementing NGHam in C for the MSP430, this task builds a working understanding
of the protocol itself, independent of embedded constraints. It uses the existing
[pyngham](https://github.com/mgm8/pyngham) Python library — developed by **Gabriel Mariano
Marcelino** as a Python port of the original NGHam library written in C by **Jon Petter
Skagmo (LA3JPA)** — to encode sample telemetry data and observe the protocol's behavior
end-to-end.

NGHam (Next Generation Ham Radio Protocol) is a lightweight communication protocol developed
for amateur radio and CubeSat missions where data must be transmitted reliably over noisy RF 
links. It packages application data into structured frames containing synchronization fields,
error detection, and Reed-Solomon forward error correction, allowing receivers to recover from
a limited number of transmission errors without requiring retransmission. Because satellite 
downlinks often experience signal fading, interference, and high bit error rates, NGHam provides
a robust and bandwidth-efficient method for exchanging telemetry, telecommands, and other mission
data between spacecraft and ground stations. This task uses the Python implementation of NGHam
(pyngham) to understand the protocol's operation before implementing it in C for the MSP430 platform.

**Tools used:** Python 3, the `pyngham` library (`PyNGHam`, `PyNGHamSPP`,
`PyNGHamExtension` classes), and Python's built-in `struct` module for byte-level packing.

**What this script demonstrates**, in order: building a realistic telemetry payload,
a basic encode/decode round trip, Reed-Solomon recovering intentionally corrupted bytes
(and correctly failing when too many errors are introduced), byte-by-byte streaming decode
(the realistic firmware model), SPP packets for onboard computer ↔ radio module
communication, and extension packets for bundling multiple telemetry fields into one frame.

Python was used instead of C at this stage purely for iteration speed — no rebuild/reflash
cycle, immediate printed output at every stage. This is not a from-scratch NGHam
implementation; it is deliberately a library-driven exploration to build protocol intuition
before writing an original C implementation in Task 3.

---

## 1. Payload Construction and Basic Encoding

```python
payload_bytes = struct.pack(">H B I I H H H H H H I I",
    0xCC2A,   # device id
    0x01,     # hardware version
    0x00010203, # firmware version v1.2.3
    2880000,  # time counter in ms
    3,        # reset counter
    3300,     # mcu voltage in mV
    45,       # mcu current in mA
    299,      # mcu temperature in K
    5012,     # radio voltage in mV
    312,      # radio current in mA
    127,      # tx packet count
    58        # rx packet count
)
# convert to list because pyngham wants a list
payload = list(payload_bytes)
print("original payload:", payload)
print("payload length:", len(payload), "bytes")

# encode the payload into a ngham packet
packet = ngham.encode(payload)
print("\nencoded packet:", packet)
print("encoded packet length:", len(packet), "bytes")
```

The field list is taken from **Table 3.3 of the TTC 2.0 documentation**, packed big-endian
(`>` prefix), so this mirrors an actual telemetry frame rather than arbitrary data.
`pyngham` expects a plain list of integers, not raw `bytes`, hence the `list(...)`
conversion. `ngham.encode()` then wraps it with NGHam's full frame structure — preamble,
sync word, size tag, header, CRC, padding, and Reed-Solomon parity.

**Result:**

```
original payload: [204, 42, 1, 0, 1, 2, 3, 0, 43, 242, 0, 0, 3, 12, 228, 0, 45, 1, 43, 19, 148, 1, 56, 0, 0, 0, 127, 0, 0, 0, 58]
payload length: 31 bytes

encoded packet: [170, 170, 170, 170, 93, 230, 42, 126, 77, 218, 87, 226, 132, 36, 193, 154, 12, 114, 191, 142, 7, 97, 173, 167, 180, 74, 42, 90, 186, 124, 231, 33, 54, 190, 6, 10, 16, 241, 247, 148, 205, 234, 139, 188, 37, 29, 129, 52, 26, 225, 121, 28, 89, 39, 91, 79, 110, 141, 156, 181, 46, 251, 152, 101, 69, 126, 124, 20, 33, 227, 17, 41, 155, 213, 86, 21, 88, 108, 205, 184, 160, 254, 85, 179, 139, 169, 142, 82, 154, 11]
encoded packet length: 90 bytes
```

The payload grows from 31 bytes to 90 bytes after encoding — that overhead is the
preamble/sync/size-tag/CRC/padding/RS-parity NGHam adds around the raw data.

---

## 2. Encode → Decode Round Trip

```python
decoded, errors, error_positions = ngham.decode(packet)
print("\ndecoded payload:", decoded)
print("number of errors:", errors)
print("error positions:", error_positions)

if decoded == payload:
    print("success - decoded payload matches original")
else:
    print("something went wrong")
```

The baseline sanity check: encode, immediately decode, confirm the result matches. If this
fails, nothing downstream is worth testing.

**Result:**
```
decoded payload: [204, 42, 1, 0, 1, 2, 3, 0, 43, 242, 0, 0, 3, 12, 228, 0, 45, 1, 43, 19, 148, 1, 56, 0, 0, 0, 127, 0, 0, 0, 58]
number of errors: 0
error positions: []
success - decoded payload matches original
```
Zero errors, empty error-position list — a clean round trip with no corruption introduced.

---

## 3. Reed-Solomon Error Correction (Within Limit)

```python
corrupted_packet = list(packet)
corrupted_packet[12] = 0xFF  # mess up byte 12
corrupted_packet[18] = 0x00  # mess up byte 18
corrupted_packet[25] = 0xAB  # mess up byte 25
print("\ncorrupted 3 bytes in the packet")

decoded2, errors2, error_positions2 = ngham.decode(corrupted_packet)
print("decoded payload:", decoded2)
print("number of errors corrected:", errors2)
print("error positions:", error_positions2)

if decoded2 == payload:
    print("success - reed solomon fixed the errors")
else:
    print("could not fix errors")
```

NGHam's RS(47,31) code (smallest size class) can correct up to **8 corrupted bytes** per
block. Three bytes were manually set to garbage values here, well within that limit.

**Result:**

```
corrupted 3 bytes in the packet
decoded payload: [204, 42, 1, 0, 1, 2, 3, 0, 43, 242, 0, 0, 3, 12, 228, 0, 45, 1, 43, 19, 148, 1, 56, 0, 0, 0, 127, 0, 0, 0, 58]
number of errors corrected: 3
error positions: [190, 177, 183]
success - reed solomon fixed the errors
```

The decoded payload matches the original exactly, with `errors2` confirming 3 corrections —
proof the RS layer is doing real work, not just passing clean data through.

---

## 4. Reed-Solomon Failure Case and Streaming Decode

```python
over_limit_packet = list(packet)
over_limit_packet[11] ^= 0xFF
over_limit_packet[13] ^= 0xFF
over_limit_packet[15] ^= 0xFF
over_limit_packet[17] ^= 0xFF
over_limit_packet[19] ^= 0xFF
over_limit_packet[21] ^= 0xFF
over_limit_packet[23] ^= 0xFF
over_limit_packet[25] ^= 0xFF
over_limit_packet[27] ^= 0xFF
print("\ncorrupted 9 bytes - more than the rs limit of 8")

decoded3, errors3, error_positions3 = ngham.decode(over_limit_packet)
print("decoded payload:", decoded3)
print("errors:", errors3)

if len(decoded3) == 0:
    print("packet was discarded because too many errors")

# stream decode - feed bytes one at a time like the firmware does
# note: skip first 8 bytes (preamble + sync word) because the radio
# hardware already removes those before the mcu sees the data
stream = PyNGHam()
for i, byte in enumerate(packet[8:]):
    result, errs, positions = stream.decode_byte(byte)
    if result:
        print("\nstream decode finished")
        print("recovered payload:", result)
        if result == payload:
            print("success - stream decode matches original")
        break
```

9 corrupted bytes — one past RS(47,31)'s correction limit of 8 — should make the decoder
give up rather than return corrupted data. The second block demonstrates the realistic
firmware model: bytes arrive one at a time from a UART interrupt, not as a complete array,
so `decode_byte()` is fed one byte at a time and holds internal state across calls.

**Result:**

```
corrupted 9 bytes - more than the rs limit of 8
decoded payload: []
errors: -1
packet was discarded because too many errors

stream decode finished
recovered payload: [204, 42, 1, 0, 1, 2, 3, 0, 43, 242, 0, 0, 3, 12, 228, 0, 45, 1, 43, 19, 148, 1, 56, 0, 0, 0, 127, 0, 0, 0, 58]
success - stream decode matches original
```

The over-limit packet correctly returns an empty payload (`errors: -1`) instead of silently
returning garbage. The streaming decoder, fed one byte at a time, still recovers the exact
original payload once enough bytes have arrived.

---

## 5. SPP Packets — Radio Module Communication

```python
spp = PyNGHamSPP()

# rx packet - radio received something from satellite, tells computer
rx_pkt = spp.encode_rx_pkt(-120, -85, 0, 0, payload)
rx_decoded = spp.decode(rx_pkt)
print("\nspp rx packet encoded:", rx_pkt)
print("spp rx decoded type:", rx_decoded['type'], "(0 = rx)")
print("spp rx decoded rssi:", rx_decoded['rssi'], "dBm")
print("spp rx decoded noise floor:", rx_decoded['noise_floor'], "dBm")
print("spp rx decoded payload:", rx_decoded['payload'])

# tx packet - computer tells radio to transmit a command to the satellite
telecommand = list(struct.pack(">B I", 24, 1))
tx_pkt = spp.encode_tx_pkt(0, telecommand)
tx_decoded = spp.decode(tx_pkt)
print("\nspp tx packet encoded:", tx_pkt)
print("spp tx decoded type:", tx_decoded['type'], "(1 = tx)")
print("spp tx decoded payload:", tx_decoded['payload'])

# command packet - computer sends text config command to radio hardware
cmd_pkt = spp.encode_cmd_pkt(list("FREQ 145900000".encode()))
cmd_decoded = spp.decode(cmd_pkt)
print("\nspp cmd packet encoded:", cmd_pkt)
print("spp cmd decoded type:", cmd_decoded['type'], "(3 = command)")
print("spp cmd decoded command:", bytes(cmd_decoded['payload']).decode())

# local packet - radio sends its own status report to computer
local_pkt = spp.encode_local_pkt(0, list(b"radio ok"))
local_decoded = spp.decode(local_pkt)
print("\nspp local packet encoded:", local_pkt)
print("spp local decoded type:", local_decoded['type'], "(2 = local)")
print("spp local decoded message:", bytes(local_decoded['payload']).decode())
```

SPP (Serial Peripheral Protocol) is how FloripaSat-2's radio module talks to the onboard
computer over UART — separate from the over-the-air NGHam frame. Four types tested: **RX**
(received data + signal strength, from a reset telecommand at parameter id 24), **TX**
(a telecommand to send), **command** (plain-text radio config — here, setting frequency to
145.9 MHz per TTC 2.0 Chapter 5 Step 4), and **local** (radio self-status).

**Result:**

```
spp rx packet encoded: [36, 34, 12, 0, 39, 106, 106, 222, 85, 80, 115, 0, 0, 204, 42, 1, 0, 1, 2, 3, 0, 43, 242, 0, 0, 3, 12, 228, 0, 45, 1, 43, 19, 148, 1, 56, 0, 0, 0, 127, 0, 0, 0, 58]
spp rx decoded type: 0 (0 = rx)
spp rx decoded rssi: -85 dBm
spp rx decoded noise floor: -120 dBm
spp rx decoded payload: [204, 42, 1, 0, 1, 2, 3, 0, 43, 242, 0, 0, 3, 12, 228, 0, 45, 1, 43, 19, 148, 1, 56, 0, 0, 0, 127, 0, 0, 0, 58]

spp tx packet encoded: [36, 165, 232, 1, 6, 0, 24, 0, 0, 0, 1]
spp tx decoded type: 1 (1 = tx)
spp tx decoded payload: [24, 0, 0, 0, 1]

spp cmd packet encoded: [36, 210, 9, 3, 14, 70, 82, 69, 81, 32, 49, 52, 53, 57, 48, 48, 48, 48, 48]
spp cmd decoded type: 3 (3 = command)
spp cmd decoded command: FREQ 145900000

spp local packet encoded: [36, 45, 26, 2, 9, 0, 114, 97, 100, 105, 111, 32, 111, 107]
spp local decoded type: 2 (2 = local)
spp local decoded message: radio ok
```

All four types round-trip correctly, including the RSSI/noise-floor values (`-85 dBm` /
`-120 dBm`), the decoded `FREQ 145900000` command string, and the `radio ok` status message.

---

## 6. Extension Packets and Full NGHam Wrap

```python
ext = PyNGHamExtension()
ext_payload = list()

# id extension - who is transmitting (satellite callsign)
ext_payload = ext.append_id_pkt(ext_payload, ext.encode_callsign("PY0EGO", 1), 127)

# status extension - telemetry stats from Table 3.3
# rssi and noise must be unsigned bytes: -85 = 171, -120 = 136
ext_payload = ext.append_stat_pkt(ext_payload,
    1,    # hw version
    1,    # serial number
    258,  # sw version
    2880, # uptime in seconds
    33,   # voltage in decivolts (33 = 3.3V)
    26,   # temperature celsius
    171,  # rssi -85dBm as unsigned byte (256-85=171)
    136,  # noise -120dBm as unsigned byte (256-120=136)
    58,   # rx ok count
    3,    # rx corrected count
    1,    # rx failed count
    127   # tx count
)

# toh extension - time of hour in microseconds
ext_payload = ext.append_toh_pkt(ext_payload, 900000000, 1)

# decode id packet - works correctly through normal decode()
raw_ext = ext.decode(ext_payload)
id_pkt = raw_ext[0]
print("\nid decoded - callsign:", id_pkt['call_ssid'][0])
print("id decoded - ssid:", id_pkt['call_ssid'][1])
print("id decoded - sequence:", id_pkt['sequence'])

# decode stat packet - call internal method directly due to library bug
stat_data = ext_payload[13:35]
stat = ext._decode_stat_pkt(stat_data)
print("\nstat decoded - hw version:", stat['hw_ver'])
print("stat decoded - uptime:", stat['uptime_s'], "seconds")
print("stat decoded - voltage:", stat['voltage'] / 10, "V")
print("stat decoded - temperature:", stat['temp'], "C")
print("stat decoded - tx count:", stat['cntr_tx'])
print("stat decoded - rx ok:", stat['cntr_rx_ok'])
signal_dbm = stat['signal'] - 256 if stat['signal'] > 127 else stat['signal']
noise_dbm = stat['noise'] - 256 if stat['noise'] > 127 else stat['noise']
print("stat decoded - rssi:", signal_dbm, "dBm")
print("stat decoded - noise floor:", noise_dbm, "dBm")

# decode toh packet - manual decode due to library bug
toh_data = ext_payload[37:]
toh_us = (toh_data[0] << 24) | (toh_data[1] << 16) | (toh_data[2] << 8) | toh_data[3]
toh_val = toh_data[4]
print("\ntoh decoded -", toh_us, "microseconds")
print("toh decoded -", toh_us / 1000000, "seconds into the hour")
print("toh decoded - valid:", toh_val)

# wrap the whole extension bundle in a normal ngham frame
ngham_ext = PyNGHam()
ext_packet = ngham_ext.encode(ext_payload)
print("\nextension ngham packet:", ext_packet)
print("extension ngham packet length:", len(ext_packet), "bytes")

ext_dec, ext_err, _ = ngham_ext.decode(ext_packet)
print("ngham decode errors:", ext_err)
if ext_dec == ext_payload:
    print("success - extension payload survived ngham encode and decode")
```

Extension packets stack multiple sub-packets (ID / status / time-of-hour) into one payload
before NGHam encoding, instead of sending one frame per field. The ID packet decodes
correctly through the library's standard `.decode()` call — but a known bug in this version
of the library means the stat and TOH sub-packets don't parse correctly through that same
call, so those two are decoded with a manual workaround: calling the internal
`_decode_stat_pkt()` method directly on a manually sliced byte range, and unpacking the TOH
timestamp by hand with bit shifts. RSSI/noise values are stored as unsigned bytes (`-85` →
`171`), converted back to signed dBm after decode.

**Result:**

```
extension payload after id packet: [1, 9, 80, 89, 48, 69, 71, 79, 32, 1, 127]
extension payload after stat packet: [1, 9, 80, 89, 48, 69, 71, 79, 32, 1, 127, 2, 22, 0, 1, 0, 1, 1, 2, 0, 0, 11, 64, 33, 26, 171, 136, 0, 58, 0, 3, 0, 1, 0, 127]
extension payload after toh packet: [1, 9, 80, 89, 48, 69, 71, 79, 32, 1, 127, 2, 22, 0, 1, 0, 1, 1, 2, 0, 0, 11, 64, 33, 26, 171, 136, 0, 58, 0, 3, 0, 1, 0, 127, 5, 5, 53, 164, 233, 0, 1]

id decoded - callsign: PY0EGO
id decoded - ssid: 1
id decoded - sequence: 1

stat decoded - hw version: 1
stat decoded - uptime: 2880 seconds
stat decoded - voltage: 3.3 V
stat decoded - temperature: 26 C
stat decoded - tx count: 127
stat decoded - rx ok: 58
stat decoded - rssi: -85 dBm
stat decoded - noise floor: -120 dBm

toh decoded - 900000000 microseconds
toh decoded - 900.0 seconds into the hour
toh decoded - valid: 1

extension ngham packet: [170, 170, 170, 170, 93, 230, 42, 126, 77, 218, 87, 237, 73, 7, 144, 195, 61, 53, 251, 193, 12, 146, 210, 165, 161, 70, 207, 90, 150, 124, 206, 50, 162, 180, 126, 43, 10, 90, 0, 148, 247, 234, 178, 254, 145, 29, 254, 49, 31, 212, 221, 245, 89, 38, 118, 172, 110, 141, 156, 181, 46, 251, 152, 101, 69, 126, 124, 20, 33, 227, 17, 41, 155, 213, 98, 239, 77, 50, 21, 139, 160, 26, 187, 5, 141, 113, 216, 219, 34, 23]
extension ngham packet length: 90 bytes
ngham decode errors: 0
success - extension payload survived ngham encode and decode
```

Callsign, status snapshot (voltage, temperature, RSSI, tx/rx counts), and time-of-hour all
decode correctly despite the library bug, and the full bundle survives a normal NGHam
encode/decode cycle.

---

## Key Takeaways

- NGHam cleanly separates two concerns: the outer layer (preamble, sync word, size tag, RS
  parity) handles RF-noise resilience; the inner payload carries arbitrary data.
- Reed-Solomon has a hard mathematical correction limit, not unlimited "extra safety" —
  relevant later for flagging when a downlink pass is too noisy to recover.
- Streaming decode (`decode_byte`) is the realistic firmware model and was carried directly
  into the C implementation in Task 3.
- This task relied entirely on an existing, published NGHam implementation rather than
  reimplementing the protocol — appropriate for building understanding before writing an
  original C version.

## References

- [pyngham](https://github.com/mgm8/pyngham) — the Python NGHam library used throughout
  this task, by Gabriel Mariano Marcelino.
- [pyngham documentation](https://mgm8.github.io/pyngham/overview.html) — primary reference
  for library usage and NGHam packet structure.
- Marcelino, G. M. (2023). *PyNGHam: A Python library of the NGHam protocol.* Journal of
  Open Source Software, 8(81), 4915. https://doi.org/10.21105/joss.04915
- Original NGHam protocol and C implementation by Jon Petter Skagmo (LA3JPA); background at
  [mgm8/ngham](https://github.com/mgm8/ngham).
