When you start bench testing, the most useful artifacts to share if something
misbehaves are:

- The EEPROM handoff bytes (raw or decoded fields), and
- The 1st 96 bytes of the selected W25 slot (header) + its CRC/state.
