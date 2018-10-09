#include "mainwindow.h"
#include <QSerialPortInfo>
#include <QTextCodec>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setupUi(this);

  serialPort = nullptr;

  auto ports = QSerialPortInfo::availablePorts();
  serialPortComboBox->clear();
  for (auto port : ports) {
    serialPortComboBox->addItem(port.portName());
  }
}

void MainWindow::onSend() {
  if (serialPort == nullptr) {
    serialPort = new QSerialPort(this);
    serialPort->setPortName(serialPortComboBox->currentText());
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
    if (serialPort->open(QIODevice::ReadWrite)) {
      statusBar()->showMessage(tr("%1 %2").arg(serialPort->portName()).arg(serialPort->baudRate()));
    } else {
      statusBar()->showMessage("Serial port failed");
      serialPort = nullptr;
    }
  }
  auto codec = QTextCodec::codecForName("UTF-8");
  auto text = inputPlainTextEdit->toPlainText();
  serialPort->write(codec->fromUnicode(text));
}