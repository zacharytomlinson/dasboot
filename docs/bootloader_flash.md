# Bootloader Flashing & Factory Programming

This repo uses an ATmega4808 flash layout with a 4 KiB bootloader and a 256 B APPCODE vector trampoline:

- BOOT: `0x0000..0x0FFF` (bootloader)
- Trampoline: `0x1000..0x10FF` (interrupt vectors that jump to bootloader reset)
- App: `0x1100..` (application vectors + code)

## Build Outputs

Bootloader build produces:

- `build-fw*/dasboot.hex`: bootloader + trampoline (includes records at `0x0000` and `0x1000`)

Quick check that the trampoline is present:

```bash
rg '^:..1000' build-fw/dasboot.hex
```

## One-Time Fuse Programming

Set BOOT section size to 4 KiB:

- `bootend` (aka `fuse8`) = `0x10` (16 * 256 bytes = 4096 bytes)

Example (port depends on host OS):

```bash
avrdude -p m4808 -c serialupdi -P /dev/ttyACM0 -U bootend:w:0x10:m
```

## Flash Programming Sequence

Recommended “chip erase then program everything” flow:

1) Program bootloader (includes trampoline):

```bash
avrdude -p m4808 -c serialupdi -P /dev/ttyACM0 -U flash:w:build-fw/dasboot.hex:i
```

2) Program application (must be linked to start at `0x1100`):

```bash
avrdude -p m4808 -c serialupdi -P /dev/ttyACM0 -U flash:w:firmware.hex:i
```

3) (Optional) Set lockbits to protect the boot section (choose values per your security policy/datasheet):

```bash
# Example only; fill in your chosen lock value:
avrdude -p m4808 -c serialupdi -P /dev/ttyACM0 -U lock:w:0x00:m
```

## Verification

- Confirm your app is linked correctly: `avr-objdump -h firmware.elf | rg '\\.text'` should show `.text` VMA at `0x1100`.
- Confirm fuses and lockbits:

```bash
avrdude -p m4808 -c serialupdi -P /dev/ttyACM0 -U bootend:r:-:h -U lock:r:-:h
```

## Common Pitfalls

- Programming “app only” after a chip erase will remove the bootloader/trampoline.
- The application must keep `CPUINT.CTRLA.CVT = 0` (no compact vectors); the trampoline assumes 4-byte `jmp` vectors.
