#ifndef SERIALPORTQT_H
#define SERIALPORTQT_H

#include "serialport.h"
#include <QTimer>

class SerialPortQt : public SerialPort {
  Q_OBJECT

public:
  static QList<SerialPort *> availablePorts(QObject *parent = nullptr);
  QString portName() override;
  void setBaudRate(qint32 baudRate) override;
  qint32 getBaudRate() override;
  void setDataBits(QSerialPort::DataBits dataBits) override;
  void setParity(QSerialPort::Parity parity) override;
  void setStopBits(QSerialPort::StopBits stopBits) override;
  void setFlowControl(QSerialPort::FlowControl flowControl) override;
  bool open() override;
  bool isOpen() override;
  void close() override;

signals:
  void receivedData(QByteArray data);
  void breakChanged(bool set); // not supported

public slots:
  void sendData(const QByteArray &data) override;
  void triggerBreak(uint msecs) override;

private slots:
  void handleReadyRead();
  void breakTimeout();

private:
  SerialPortQt(QObject *parent = nullptr, QString portName = "");
  ~SerialPortQt();
  QSerialPort *port;
  QTimer *breakTimer;
};

#endif