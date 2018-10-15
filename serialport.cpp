#include "serialport.h"
#include "serialportcp210x.h"
#include "serialportdummy.h"
#include "serialportqt.h"

SerialPort::SerialPort(QObject *parent) : QObject(parent) {}

QList<SerialPort *> SerialPort::getAvailablePorts(QObject *parent) {
  QList<SerialPort *> result;
  result.append(SerialPortQt::availablePorts(parent));
  result.append(SerialPortDummy::availablePorts(parent));
  result.append(SerialPortCP210X::availablePorts(parent));
  return result;
}