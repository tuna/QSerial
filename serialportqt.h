#ifndef SERIALPORTQT_H
#define SERIALPORTQT_H

#include "serialport.h"

class SerialPortQt : public SerialPort {
  Q_OBJECT

public:
  SerialPortQt(QObject *parent = nullptr, QString portName = "");
  ~SerialPortQt();
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

public slots:
  void sendData(const QByteArray &data) override;

private slots:
  void handleReadyRead();

private:
  QSerialPort *port;
};

#endif