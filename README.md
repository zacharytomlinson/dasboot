# Firmware (ATmega4808)

This folder contains the AVR bring-up code for an ATmega4808 bootloader that will eventually:
- use a W25Qxx 128 Mbit SPI flash as a staging/rollback area
- verify firmware signatures using an ATECC608A secure element (I2C)
- self-program the host application image

## Build

Requirements: `avr-gcc`, `avr-objcopy`, and a build tool (`ninja` recommended).

### CLion (Recommended)

This repo ships `CMakePresets.json` with AVR presets. In CLion, select the `avr-ninja` (or `avr-make`) preset/profile so the IDE configures the project with the AVR toolchain.

From the CLI, you can also use:

```bash
cmake --preset avr-ninja
cmake --build --preset avr-ninja
```

### Manual

```bash
cmake -S . -B build-avr-ninja -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/avr-gcc-toolchain.cmake \
  -DMCU=atmega4808 -DF_CPU=20000000UL
cmake --build build-avr-ninja
```

Outputs:
- `build-avr-ninja/dasboot` (ELF)
- `build-avr-ninja/dasboot.hex` and `build-avr-ninja/dasboot.bin`

## Pin Mapping

Defaults live in `src/board.h` and are meant to match your schematic.

- SPI0 pins (current board): `PA4=MOSI`, `PA5=MISO`, `PA6=SCK`, `PA7=SS`
- W25Qxx: `/HOLD`/`/RESET` on `PD1`, CS on `PD2`, `/WP` on `PD3`
- STAT LED: `PD0`
- ATECC608A: I2C via TWI0 (adjust `TWI0_PORTMUX_ROUTE_SEL` in `src/board.h`)

Current bring-up expects the ATECC to be provisioned/locked with the **firmware verification public key stored in ATECC slot 13**. If W25Q probing, slot validation, flash programming, or verification fails, the STAT LED emits the corresponding fatal blink code.

## ATECC Provisioning (Optional)

There is an optional one-time provisioning firmware target (`atecc_provision`) that mirrors the legacy Arduino flow (write config, lock config, generate slot0 keypair, lock data+slot0). **Locking is permanent.**

- Build: `cmake --build build-avr-ninja --target atecc_provision`
- Flash `build-avr-ninja/atecc_provision.hex` (same UPDI method as the bootloader)
- Enable actual provisioning with: `-DATECC_PROVISION_ENABLE=1` (otherwise it will only blink error code **4** on unprovisioned devices)

Note: this legacy provisioning does **not** write the firmware verification public key into slot 13; it only mirrors the previous “generate per-device keypair in slot 0” behavior. Current factory provisioning is handled by the application repo flasher/provisioner flow.

## Runtime Behavior

On reset, DasBoot probes W25Q, reads the EEPROM handoff state, and either boots the app, applies a pending slot, or restores the confirmed slot after repeated unconfirmed boots.

- `APPLY_PENDING`: verify the pending W25Q slot, program internal flash from it, set `WAIT_CONFIRM`, then jump to the app.
- `WAIT_CONFIRM`: increment attempts and jump to the app. After 3 unconfirmed attempts, restore the confirmed slot and clear pending state.
- No pending flags: jump directly to the app through the trampoline.

Candidate verification streams `SHA-256(image bytes)` through the ATECC SHA engine and verifies the raw 64-byte ECDSA `R||S` signature with ATECC `Verify(Stored)` using slot 13.

Fatal STAT LED blink codes currently used by `src/main.c`:

- `3`: W25Q probe failed.
- `4`: pending slot header could not be read.
- `5`: pending slot verification or programming failed and rollback path could not recover cleanly.
- `6`: confirmed-slot restore/programming failed.
- `7`: confirmed slot header could not be read for rollback.

## Flashing (UPDI)

Example (adjust the serial port and programmer):

```bash
cmake -S . -B build-avr-ninja -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/avr-gcc-toolchain.cmake \
  -DAVRDUDE_PORT=/dev/ttyACM0 -DAVRDUDE_PROGRAMMER=serialupdi
cmake --build build-avr-ninja --target flash
```

Note: placing the image in the bootloader section and configuring fuses will be handled once the final memory map is decided.
