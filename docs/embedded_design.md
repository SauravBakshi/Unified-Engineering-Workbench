# Embedded integration design (simulation only)
Host PC communicates with a microcontroller over USB CDC/UART, with an FPGA development board over a board-specific USB-UART/JTAG bridge, and with a dedicated USB-JTAG probe through its vendor-supported host driver. The JTAG probe is a debug/programming transport, not a general-purpose compute board; it does not replace an MCU or FPGA.

| Link | Signal / pin assumption | Voltage / power | Ownership and safety |
|---|---|---|---|
| Host–MCU | USB D+/D−, GND; board-specific UART pins | USB 5 V input; MCU GPIO commonly 3.3 V (verify exact board) | Host owns USB; MCU firmware owns UART telemetry |
| Host–FPGA | USB-UART bridge TX/RX/GND; FPGA I/O bank pins | Board USB input; bank may be 1.8/2.5/3.3 V | FPGA bank voltage must match I/O standard; never assume 3.3 V |
| Host–JTAG probe | USB host to probe; TCK, TMS, TDI, TDO, GND, optional nRESET | Target reference voltage sensed by probe; target powered separately unless explicitly supported | Probe drives TCK/TMS/TDI; target drives TDO; shared ground required |

Use a common ground for signal links, verify connector pinout from the exact board manuals, check target reference voltage, and use level translators where logic domains differ. Never connect 5 V signals directly to 3.3 V/1.8 V inputs without explicit tolerance. Avoid powering a target from two sources. The included adapters only simulate discovery, capabilities, connect/disconnect and telemetry; no physical device is accessed.
