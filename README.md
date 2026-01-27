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
cmake -S . -B build-fw -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/avr-gcc-toolchain.cmake \
  -DMCU=atmega4808 -DF_CPU=20000000UL
cmake --build build-fw
```

Outputs:
- `build-fw/dasboot` (ELF)
- `build-fw/dasboot.hex` and `build-fw/dasboot.bin`

## Pin Mapping

Defaults live in `src/board.h` and are meant to match your schematic.

- SPI0 pins (current board): `PA4=MOSI`, `PA5=MISO`, `PA6=SCK`, `PA7=SS`
- W25Qxx: CS on `PD2`
- STAT LED: `PD0`
- ATECC608A: I2C via TWI0 (adjust `TWI0_PORTMUX_ROUTE_SEL` in `src/board.h`)

Current bring-up expects the ATECC to be provisioned/locked with the **firmware verification public key stored in slot 8**. If it is not, the firmware blinks error code **4**.

## ATECC Provisioning (Optional)

There is an optional one-time provisioning firmware target (`atecc_provision`) that mirrors the legacy Arduino flow (write config, lock config, generate slot0 keypair, lock data+slot0). **Locking is permanent.**

- Build: `cmake --build build-fw --target atecc_provision`
- Flash `build-fw/atecc_provision.hex` (same UPDI method as the bootloader)
- Enable actual provisioning with: `-DATECC_PROVISION_ENABLE=1` (otherwise it will only blink error code **4** on unprovisioned devices)

Note: this legacy provisioning does **not** write the firmware verification public key into slot 8; it only mirrors the previous “generate per-device keypair in slot 0” behavior.

## Flashing (UPDI)

Example (adjust the serial port and programmer):

```bash
cmake -S . -B build-fw -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/avr-gcc-toolchain.cmake \
  -DAVRDUDE_PORT=/dev/ttyACM0 -DAVRDUDE_PROGRAMMER=serialupdi
cmake --build build-fw --target flash
```

Note: placing the image in the bootloader section and configuring fuses will be handled once the final memory map is decided.
