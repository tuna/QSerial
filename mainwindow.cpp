#include "mainwindow.h"
#include <QSerialPortInfo>
#include "serialport.h"
#include <QTextCodec>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setupUi(this);

  ports = SerialPort::getAvailablePorts(this);
  serialPortComboBox->clear();
  for (auto port : ports) {
    serialPortComboBox->addItem(port->portName());
    connect(port, SIGNAL(receivedData(QByteArray)), this, SLOT(onDataReceived(QByteArray)));
  }
}

void MainWindow::onSend() {
  auto serialPort = ports[serialPortComboBox->currentIndex()];
  serialPort->setBaudRate(baudRateComboBox->currentText().toInt());
  serialPort->setDataBits(
      (QSerialPort::DataBits)dataBitsComboBox->currentText().toInt());
  QSerialPort::Parity parity[] = {
      QSerialPort::NoParity, QSerialPort::EvenParity, QSerialPort::OddParity,
      QSerialPort::SpaceParity, QSerialPort::MarkParity};
  serialPort->setParity(parity[parityComboBox->currentIndex()]);
  serialPort->setStopBits(
      (QSerialPort::StopBits)(stopBitsComboBox->currentIndex() + 1));
  serialPort->setFlowControl(
      (QSerialPort::FlowControl)flowControlComboBox->currentIndex());
  if (!serialPort->isOpen()) {
    if (serialPort->open()) {
      statusBar()->showMessage(tr("%1 %2 Open").arg(serialPort->portName()).arg(baudRateComboBox->currentText().toInt()));
    } else {
      statusBar()->showMessage("Failed");
    }
  }
  auto codec = QTextCodec::codecForName("UTF-8");
  auto text = inputPlainTextEdit->toPlainText();
  serialPort->sendData(codec->fromUnicode(text + "\n"));
}
  
void MainWindow::onDataReceived(QByteArray data) {
  qWarning() << "MainWindow" << data;
  auto codec = QTextCodec::codecForName("UTF-8");
  textBrowser->setPlainText(textBrowser->toPlainText() + codec->toUnicode(data));
}