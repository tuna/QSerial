#ifndef SERIALPORTDUMMY_H
#define SERIALPORTDUMMY_H

#include "serialport.h"

class SerialPortDummy : public SerialPort {
  Q_OBJECT

public:
  static QList<SerialPort *> availablePorts(QObject *parent = nullptr) {
    return QList<SerialPort *>{new SerialPortDummy(parent)};
  }
  QString portName() override { return "dummy"; }
  void setBaudRate(qint32) override {}
  void setDataBits(QSerialPort::DataBits) override {}
  void setParity(QSerialPort::Parity) override {}
  void setStopBits(QSerialPort::StopBits) override {}
  void setFlowControl(QSerialPort::FlowControl) override {}
  bool open() override { return isOpening = true; }
  bool isOpen() override { return isOpening; }
  void close() override { isOpening = false; }

signals:
  void receivedData(QByteArray data);

public slots:
  void sendData(const QByteArray &data) override { emit receivedData(data); }

private:
  SerialPortDummy(QObject *parent = nullptr) : SerialPort(parent) {
    isOpening = false;
  }
  ~SerialPortDummy() {}

  bool isOpening;
};

#endif