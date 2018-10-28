#ifndef SERIALPORTCP210X_H
#define SERIALPORTCP210X_H

#include "libusb.h"
#include "serialport.h"
#include <QAtomicInt>
#include <QThread>
#include <QTimer>

// reference:
// https://www.silabs.com/documents/public/application-notes/AN571.pdf

class SerialPortCP210X : public SerialPort {
  Q_OBJECT

public:
  static QList<SerialPort *> availablePorts(QObject *parent = nullptr);
  QString portName() override;
  void setBaudRate(qint32 baudRate) override;
  void setDataBits(QSerialPort::DataBits dataBits) override;
  void setParity(QSerialPort::Parity parity) override;
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
  SerialPortCP210X(QObject *parent = nullptr, libusb_device *device = nullptr);
  ~SerialPortCP210X();
  libusb_device *device;
  libusb_device_handle *handle;
  QThread *thread;
  QTimer *breakTimer;
  QAtomicInt shouldStop;
};

#endif