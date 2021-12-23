#include "serialport.h"
#include "serialportch34x.h"
#include "serialportcp210x.h"
#include "serialportpl2303.h"
#include "serialportdummy.h"
#include "serialportqt.h"

SerialPort::SerialPort(QObject *parent) : QObject(parent) {
  currentBaudRate = QSerialPort::Baud115200;
  currentDataBits = QSerialPort::Data8;
  currentParity = QSerialPort::NoParity;
  currentStopBits = QSerialPort::OneStop;
  currentFlowControl = QSerialPort::NoFlowControl;
}

QList<SerialPort *> SerialPort::getAvailablePorts(QObject *parent) {
  QList<SerialPort *> result;
  result.append(SerialPortCP210X::availablePorts(parent));
  result.append(SerialPortCH34X::availablePorts(parent));
  result.append(SerialPortPL2303::availablePorts(parent));
  result.append(SerialPortQt::availablePorts(parent));
  result.append(SerialPortDummy::availablePorts(parent));
  return result;
}
