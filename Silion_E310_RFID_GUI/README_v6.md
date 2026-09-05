# Silion E310 RFID GUI v6

Stability fixes:
- Serial worker no longer calls Qt widgets directly. All serial callbacks are delivered through PySide6 signals to the GUI thread.
- GUI shutdown cleanly stops the serial worker.
- Startup reader-information polling waits 1.4 seconds after connect and uses a 350 ms spacing between host commands.
- Single Poll, Multi Poll, EPC filtering, and one-row-per-EPC updating are retained.

Single/Multi Poll continue to use the existing STM32 `START`/`STOP` host protocol.
