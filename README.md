QSerial
====================

The missing cross platform serial port utility with batteries included.

Following devices are supported, even without kernel-mode driver:

- Silicon Labs CP210x
- WCH CH34x
- Prolific PL2303x

Implemented features:

- Basic serial port settings (baud rate, data bits, parity, stop bits and flow control)
- Send plain text, hex encoded binary, base64 encoded binary and percent encoded text.
- Send BREAK condition.
- Show received data as UTF-8, Big5, GB18030, Shift-JIS or hex.
- Speed meter.
