# Address Map

| Range | Target |
|---|---|
| 0x00000000–0x000FFFFF | Main memory |
| 0x10000000–0x100000FF | DMA registers |
| 0x20000000–0x200000FF | Accelerator registers |
| 0x30000000–0x300000FF | Interrupt controller |

# DMA Register map

| Use 32-bit MMIO registers, even if the modeled address type is 64-bit |

Offset        Register
| 0x00 |      | CONTROL |
| 0x04 |      | STATUS  |
| 0x08 |      | SOURCE_LO |
| 0x0C |      | SOURCE_HI |
| 0x10 |      | DESTINATION_LO |
| 0x14 |      | DESTINATION_HI |
| 0x18 |      | LENGTH_BYTES |
| 0x1C |      | BURST_BYTES |
