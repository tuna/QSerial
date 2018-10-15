#include "serialportqt.h"
#include <QDebug>

SerialPortQt::SerialPortQt(QObject *parent, QString portName)
    : SerialPort(parent) {
  port = new QSerialPort(this);
  port->setPortName(portName);
  connect(port, SIGNAL(readyRead()), this, SLOT(handleReadyRead()));
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
  while(port->bytesAvailable()) {
    auto data = port->readAll();
    qWarning() << "SerailPortQt" << data;
    emit receivedData(data);
  }
}