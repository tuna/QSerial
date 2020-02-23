#ifndef SERIALPORT_H
#define SERIALPORT_H

#include <QObject>
#include <QSerialPort>

class SerialPort : public QObject {
  Q_OBJECT

public:
  static QList<SerialPort *> getAvailablePorts(QObject *parent = nullptr);
  virtual QString portName() = 0;
  virtual void setBaudRate(qint32 baudRate) = 0;
  virtual qint32 getBaudRate() = 0;
  virtual void setDataBits(QSerialPort::DataBits dataBits) = 0;
  virtual void setParity(QSerialPort::Parity parity) = 0;
  virtual QSerialPort::Parity getParity() = 0;
  virtual void setStopBits(QSerialPort::StopBits stopBits) = 0;
  virtual void setFlowControl(QSerialPort::FlowControl flowControl) = 0;
  virtual bool open() = 0;
  virtual bool isOpen() = 0;
  virtual void close() = 0;

signals:
  void receivedData(QByteArray data);
  void breakChanged(bool set);

public slots:
  virtual void sendData(const QByteArray &data) = 0;
  virtual void triggerBreak(uint msecs) = 0;

protected:
  SerialPort(QObject *parent = nullptr);

  qint32 currentBaudRate;
  QSerialPort::DataBits currentDataBits;
  QSerialPort::Parity currentParity;
  QSerialPort::StopBits currentStopBits;
  QSerialPort::FlowControl currentFlowControl;
};

#endif