# stm_rfd_interfacing
Project-: stm32f429 + JRD-100 RFID prototype
Development timeline-

Day 1-
STM32F429 project setup and MCU-specific configurationn\

Day 2- 
GPIO and peripheral driver development
Day 3-
USART driver implementation and UART testing
Day 4-
Document JRD-100 communication protocol using available documentation and existing implementation
Day 5-
Design JRD-100 driver architecture and API; define structures, commands, responses and error handling
Day 6-
Implement JRD-100 TX frame generation, command handling and checksum calculation
Day 7-
Implement JRD-100 RX handling, frame parser, checksum verification and timeout handling
Day 8-
Implement and test basic JRD-100 commands
Day 9-
Implement RFID inventory functionality and EPC/tag response parsing
Day 10-
Implement required tag operations such as tag read/write and reader configuration
Day 11-
Firmware integration and robustness testing; handle communication errors, timeouts and repeated inventory operations
Day 12-
Implement communication interface between STM32 and host/UI
Day 13-
Develop basic UI for reader status, inventory and tag information
Day 14-
Integrate complete system and perform hardware testing/debugging
Day 15-
Final testing, bug fixing, documentation and demonstration preparation
