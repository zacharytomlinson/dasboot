# Firmware Expectations (Bootloader Contract)

This document describes what the main application firmware must do (and must **not** do) to work with the bootloader design:

- Bootloader verifies candidate images using **ATECC608A stored public key slot 13**.
- Bootloader uses **W25Q** as an **A/B staging + rollback** store.
- Bootloader is responsible for keeping rollback possible (both slots usable).

## Non‑Negotiable Assumptions

- The application **must not self-program internal flash** for updates. It must stage updates into W25Q.
- The application must not modify ATECC configuration/keys at runtime (production devices are provisioned/locked).
- The application must “confirm” a successful boot so the bootloader can mark the update as committed.

## Flash Layout & Fuses (ATmega4808)

This bootloader relies on the ATmega4808 “flash sections” behavior: after reset, the CPU uses the **start of APPCODE** as the interrupt vector base. With a 4 KiB bootloader, this means vectors begin at `0x1000` (BOOTEND * 256).

Expected layout:

- `0x0000..0x0FFF`: BOOT section (bootloader)
- `0x1000..0x10FF`: APPCODE vector trampoline (installed by bootloader build)
- `0x1100..`: application vectors + code (`APP_START_ADDR` in `src/boot_contract.h`)

Fuse expectation:

- `bootend` (aka `fuse8`) = `0x10` (16 * 256 bytes = 4096 bytes)

Example (port varies by OS/host):

```bash
avrdude -p m4808 -c serialupdi -P /dev/ttyACM0 -U bootend:w:0x10:m
```

Application build expectation:

- Link application to start at `APP_START_ADDR` (default `0x1100`), e.g. `-Wl,--section-start=.text=0x1100`.
- Do **not** enable “compact vector table” (`CPUINT.CTRLA.CVT` must remain `0`), since the trampoline assumes 4-byte `jmp` vectors.

## W25Q A/B Slot Model

- Two slots: **A** and **B**. Exactly one is “confirmed/current”.
- Slot sizing and base addresses must match the bootloader build-time constants (see `src/boot_contract.h`), notably `W25_SLOT_SIZE`, `W25_SLOT_A_BASE`, and `W25_SLOT_B_BASE`.
- Default layout: slot A base `0x000000`, slot B base `0x010000` (64 KiB each).
- Each slot contains:
  - a small **slot header** at the slot base address
  - the raw firmware image bytes immediately following the header
- The firmware image is verified as:
  1) `SHA-256(image bytes)` computed by ATECC SHA engine (streamed)
  2) `ECDSA verify` of that digest using **ATECC608A stored public key slot 13** and a 64-byte raw signature `R||S`

Recommended slot header fields (written in little-endian unless noted):
- `magic` (32-bit)
- `header_version` (8-bit)
- `state` (8-bit): `EMPTY=0xFF`, `STAGING=0x7F`, `READY=0x3F`, `CONFIRMED=0x1F` (bit-clearing progression)
- `reserved0` (16-bit, reserved; set to 0)
- `image_len` (32-bit, raw bytes)
- `image_version` or `sequence` (32-bit monotonic)
- `sig_rs[64]` (raw `R(32)||S(32)`, each 32-byte **big-endian**)
- `header_crc16` (16-bit) over header bytes excluding the CRC itself, computed **as if `state == READY`** (use the shared CRC implementation in `src/crc16.h`)

Commit rule: the header should be written such that it can be made valid **only at the end** (e.g., write `state=READY` last, or write `magic` last).

Note on `state` vs CRC: because `header_crc16` is computed as if `state==READY`, firmware may later clear bits (e.g., `READY->CONFIRMED`) without needing to rewrite the CRC field (which could require 0→1 bit sets and is not generally safe in flash).

## EEPROM Handoff Flags (Minimal)

The application writes a small EEPROM struct that the bootloader reads at reset:
- `pending_slot` (0=A, 1=B, 0xFF=none)
- `confirmed_slot` (0/1)
- `attempts` (uint8)
- `flags`:
  - `APPLY_PENDING`: app requests bootloader to apply `pending_slot`
  - `WAIT_CONFIRM`: bootloader has applied an update and is waiting for the app to confirm
- `eeprom_crc16` (use the shared CRC implementation in `src/crc16.h`)

CRC init: this project uses `crcval=0x0000` as the initial value (matches the running firmware).

Recommended EEPROM layout (to avoid collisions with other settings):

- `0x0000..0x0005`: `boot_state_t` (bootloader handoff struct, 6 bytes)
- `0x0010..0x0013`: `last_confirmed_sequence` (little-endian `uint32_t`, firmware-owned)

The bootloader may ignore `image_len` in EEPROM and treat W25 slot headers as the source of truth.

## Application Update Responsibilities

When the server indicates an update is available:
1) Choose the **inactive** slot (the one not equal to `confirmed_slot`).
2) Erase required W25 blocks/sectors for that slot (see W25Q notes below).
3) Stream firmware bytes into W25 at `slot_base + header_size`, exactly `image_len` bytes.
4) Obtain/provide the signature `sig_rs[64]` for the digest of the image (raw `R||S`, big-endian integers).
5) Write the slot header and commit it (`state=READY`).
6) Write EEPROM:
   - set `pending_slot` to the staged slot
   - set `flags |= APPLY_PENDING`
   - reset `attempts=0`
   - write CRC
7) Reset the MCU.

On successful startup (after the bootloader has applied the update), the application must:
- Clear `WAIT_CONFIRM`
- Set `confirmed_slot = pending_slot`, then clear `pending_slot = 0xFF`
- Update EEPROM CRC

## W25Q128JVP Erase/Write Notes (SPI)

This repo assumes a **W25Q128JVP-compatible** flash and uses **64 KiB slots**. The current application staging path erases only the 4 KiB sectors needed for the slot header plus image bytes.

Recommended staging sequence for a slot:

1) **Erase** the required slot region:
   - `WREN` (0x06)
   - `Sector Erase 4KB` (0x20) for each 4 KiB sector covering `slot_base..slot_base + sizeof(slot_header_t) + image_len`
   - Poll `RDSR1` (0x05) `WIP` bit until cleared.

2) **Program** the image bytes at `slot_base + sizeof(slot_header_t)`:
   - For each chunk (max 256 bytes, must not cross a 256-byte page boundary):
     - `WREN` (0x06)
     - `Page Program` (0x02) + 24-bit address + data
     - Poll `RDSR1` (0x05) `WIP` until cleared.
   - Optional but recommended: read back (`READ` 0x03) and verify incrementally.

3) **Commit** the slot header last:
   - Write all header fields with `state=STAGING` (or not-READY).
   - Finally rewrite the header with `state=READY` and a valid `header_crc16` (so power-loss can’t produce a “READY” header by accident).

Whole-slot `Block Erase 64KB` (0xD8) is still compatible with the slot alignment, but it is not required by the current application staging path. Keep slot layout constants aligned with the bootloader (`W25_SLOT_*`).

## Board Control Pins

Current board assumptions shared with the application firmware:

- W25Q CS: `PD2`.
- W25Q `/HOLD`/`/RESET`: `PD1`, driven high before flash access.
- W25Q `/WP`: `PD3`, driven high before flash access.
- STAT LED: `PD0`.

The bootloader initializes these pins before probing W25Q so the flash and shared SPI bus start from a sane idle state.

## Common Pain Points (Firmware Side)

- **Link address**: the application (including its vector table) must be linked to start at `APP_START_ADDR` (default `0x1100`). Nothing in the app should be linked to `0x0000` or `0x1000` (bootloader owns `0x0000..0x10FF` including the trampoline).
- **No compact vectors**: keep `CPUINT.CTRLA.CVT = 0`. The bootloader’s trampoline assumes 4-byte `jmp` vectors.
- **SPI program boundaries**: W25Q `Page Program` is max **256 bytes** and must not cross a 256-byte page boundary. Always `WREN` before each program/erase and poll `RDSR1` `WIP` after.
- **Address mode**: stay in 3-byte addressing (default). If your firmware ever enables 4-byte address mode, ensure it exits back to 3-byte mode before reset/hand-off.
- **Power-loss commit discipline**: never create a “READY” slot header until the entire image is written and verified; flip `state=READY` (and CRC) as the final write.
- **Signature semantics**: signature must verify `SHA-256(image_bytes[0..image_len-1])` exactly; store signature as raw `R||S` (32-byte big-endian each).
- **EEPROM state writes**: update EEPROM only on state transitions (to reduce wear) and always write the whole struct + CRC; avoid concurrent/background writes during an update attempt.
- **Re-init peripherals**: don’t assume reset defaults. Bootloader configures SPI/TWI/clock; firmware should explicitly initialize what it uses.

## Bootloader Responsibilities (For Reference)

- If `pending_slot` is set and slot header is `READY`, verify signature via ATECC `Verify(Stored)` using slot 13 and program internal flash.
- Maintain rollback: if `pending_confirm` remains set across boots or `attempts` exceeds a limit, re-flash internal from `confirmed_slot`.
- Ensure at least one valid slot remains `CONFIRMED` (bootloader may copy the running internal image into W25 when needed to re-establish A/B redundancy).

## Failure/Recovery Expectations

- The application should treat EEPROM flags as authoritative and avoid rewriting them concurrently with a bootloader update attempt.
- Power loss during staging must leave either:
  - the previous `CONFIRMED` slot intact, or
  - a header state that does not appear `READY` to the bootloader.
