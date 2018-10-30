How to write a driver for a new serial device
===============

1. Look for a open source driver from Linux, BSD, etc.
2. Subclass from SerialPort and implement its functions.
3. Use builtin testing tool to check whether it really works.
