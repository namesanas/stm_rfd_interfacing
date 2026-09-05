# Silion E310 RFID GUI v5

Added:
- EPC tag filter with contains/exact matching.
- Single Poll: starts inventory and automatically stops after the first received tag, with a configurable timeout.
- Multi Poll: continuous inventory using the existing START/STOP host protocol.
- Existing one-row-per-EPC updating remains unchanged.

Note:
Single/Multi Poll are implemented at the GUI host-interface level using the existing ASCII `START`/`STOP` commands. They do not yet expose the E310 native 0x21/0x22 commands directly.
