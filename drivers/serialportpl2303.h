#ifndef SERIALPORTPL2303_H
#define SERIALPORTPL2303_H

#include "libusb.h"
#include "serialport.h"
#include <QAtomicInt>
#include <QThread>
#include <QTimer>

// reference:
// Linux kernel pl2303.c

class SerialPortPL2303 : public SerialPort {
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
  bool isOpen() override { return handle != nullptr; }
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
  SerialPortPL2303(QObject *parent = nullptr, libusb_device *device = nullptr);
  ~SerialPortPL2303();

  bool vendorRead(quint16 val, unsigned char buf[1]);
  bool vendorWrite(quint16 val, quint16 index);
  bool getLineOptions();
  bool setLineOptions();
  void setControlLines(quint8 control);
  void setBreak(bool set);
  void setQuirks(quint8 _quirks) { quirks = _quirks; }
  void setType(quint8 _type) { type = _type; }
  libusb_device *device;
  libusb_device_handle *handle;
  QThread *thread;
  QTimer *breakTimer;
  quint8 lineOptions[7];
  quint8 quirks;
  quint8 type;
  quint8 dataEPIn, dataEPOut;

  QAtomicInt shouldStop;
};

#endif