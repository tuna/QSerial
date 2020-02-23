#ifndef SERIALPORTCH34X_H
#define SERIALPORTCH34X_H

#include "libusb.h"
#include "serialport.h"
#include <QAtomicInt>
#include <QThread>
#include <QTimer>

// reference:
// Linux kernel ch341.c
// Z4yx ch340.c

class SerialPortCH34X : public SerialPort {
  Q_OBJECT

public:
  static QList<SerialPort *> availablePorts(QObject *parent = nullptr);
  QString portName() override;
  void setBaudRate(qint32 baudRate) override;
  qint32 getBaudRate() override { return currentBaudRate; }
  void setDataBits(QSerialPort::DataBits dataBits) override;
  void setParity(QSerialPort::Parity parity) override;
  QSerialPort::Parity getParity() override { return currentParity; }
  void setStopBits(QSerialPort::StopBits stopBits) override;
  void setFlowControl(QSerialPort::FlowControl flowControl) override;
  bool open() override;
  bool isOpen() override;
  void close() override;

signals:
  void receivedData(QByteArray data);
  void breakChanged(bool set);

public slots:
  void sendData(const QByteArray &data) override;
  void triggerBreak(uint msecs) override;

private slots:
  void breakTimeout();

private:
  SerialPortCH34X(QObject *parent = nullptr, libusb_device *device = nullptr);
  ~SerialPortCH34X();

  void setLcr();
  void setHandshake(quint8 control);
  void setBreak(bool set);
  libusb_device *device;
  libusb_device_handle *handle;
  QThread *thread;
  QTimer *breakTimer;
  quint8 dataBits;
  quint8 parity;
  quint8 stopBits;

  QAtomicInt shouldStop;
};

#endif