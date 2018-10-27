#include "serialportqt.h"
#include <QDebug>
#include <QSerialPortInfo>

SerialPortQt::SerialPortQt(QObject *parent, QString portName)
    : SerialPort(parent) {
  port = new QSerialPort(this);
  port->setPortName(portName);
  connect(port, SIGNAL(readyRead()), this, SLOT(handleReadyRead()));
  breakTimer = nullptr;
}

SerialPortQt::~SerialPortQt() { delete port; }

QString SerialPortQt::portName() { return port->portName(); }
void SerialPortQt::setBaudRate(qint32 baudRate) { port->setBaudRate(baudRate); }
void SerialPortQt::setParity(QSerialPort::Parity parity) {
  port->setParity(parity);
}
void SerialPortQt::setDataBits(QSerialPort::DataBits dataBits) {
  port->setDataBits(dataBits);
}
void SerialPortQt::setStopBits(QSerialPort::StopBits stopBits) {
  port->setStopBits(stopBits);
}
void SerialPortQt::setFlowControl(QSerialPort::FlowControl flowControl) {
  port->setFlowControl(flowControl);
}
bool SerialPortQt::open() { return port->open(QIODevice::ReadWrite); }
bool SerialPortQt::isOpen() { return port->isOpen(); }
void SerialPortQt::close() { return port->close(); }
void SerialPortQt::sendData(const QByteArray &data) { port->write(data); }

void SerialPortQt::handleReadyRead() {
  while (port->bytesAvailable()) {
    auto data = port->readAll();
    emit receivedData(data);
  }
}
QList<SerialPort *> SerialPortQt::availablePorts(QObject *parent) {
  QList<SerialPort *> result;
  auto ports = QSerialPortInfo::availablePorts();
  for (auto port : ports) {
    result.append(new SerialPortQt(parent, port.portName()));
  }
  return result;
}

void SerialPortQt::triggerBreak(uint msecs) {
  port->setBreakEnabled(true);
  qWarning() << "Enabled";
  if (breakTimer) {
    breakTimer->stop();
    delete breakTimer;
  }
  breakTimer = new QTimer(this);
  breakTimer->setSingleShot(true);
  connect(breakTimer, SIGNAL(timeout()), this, SLOT(breakTimeout()));
  breakTimer->start(msecs);
}

void SerialPortQt::breakTimeout() {
  port->setBreakEnabled(false);
  qWarning() << "Disabled";
}