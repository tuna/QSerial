#include "mainwindow.h"
#include "serialport.h"
#include <QDebug>
#include <QScrollBar>
#include <QSerialPortInfo>
#include <QTextCodec>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setupUi(this);

  ports = SerialPort::getAvailablePorts(this);
  serialPortComboBox->clear();
  for (auto port : ports) {
    serialPortComboBox->addItem(port->portName());
    connect(port, SIGNAL(receivedData(QByteArray)), this,
            SLOT(onDataReceived(QByteArray)));
  }
}

inline int fromHex(char ch) {
  if ('0' <= ch && ch <= '9') {
    return ch - '0';
  } else if ('a' <= ch && ch <= 'f') {
    return ch - 'a' + 10;
  } else if ('A' <= ch && ch <= 'F') {
    return ch - 'A' + 10;
  } else {
    return -1;
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
      statusBar()->showMessage(
          tr("%1 %2 Open")
              .arg(serialPort->portName())
              .arg(baudRateComboBox->currentText().toInt()));
    } else {
      statusBar()->showMessage("Failed");
    }
  }
  if (serialPort->isOpen()) {
    QString text = inputPlainTextEdit->toPlainText();
    auto codec = QTextCodec::codecForName("UTF-8");
    QByteArray data;
    bool isFirst;
    int currentByte;
    switch (sendParseAsComboBox->currentIndex()) {
    case 0:
      // text
      data = codec->fromUnicode(text);
      break;
    case 1:
      // hex
      isFirst = true;
      currentByte = 0;
      for (auto ch : codec->fromUnicode(text)) {
        if (ch == ' ' || ch == '\r' || ch == '\n') {
          continue;
        }
        int cur = fromHex(ch);
        if (cur == -1) {
          statusBar()->showMessage("Invalid hex");
          return;
        }
        if (isFirst) {
          currentByte = cur;
          isFirst = false;
        } else {
          currentByte = (currentByte << 4) + cur;
          data.append(currentByte);
          isFirst = true;
        }
      }
      if (!isFirst) {
        statusBar()->showMessage("Invalid hex");
        return;
      }
      break;
    default:
      // never happens
      break;
    }
    data.append('\n');
    if (echoCheckBox->isChecked()) {
      textBrowser->setPlainText(textBrowser->toPlainText() + text);
      textBrowser->verticalScrollBar()->setValue(
          textBrowser->verticalScrollBar()->maximum());
    }
    serialPort->sendData(data);
  }
}

inline char toHex(int value) {
  if (0 <= value && value <= 9) {
    return '0' + value;
  } else if (10 <= value && value <= 15) {
    return 'A' + value - 10;
  } else {
    return '?';
  }
}

void MainWindow::onDataReceived(QByteArray data) {
  QString text;
  auto codec = QTextCodec::codecForName("UTF-8");
  switch (recvShowAsComboBox->currentIndex()) {
  case 0:
    // text
    text = codec->toUnicode(data);
    break;
  case 1:
    // hex
    for (auto byte : data) {
      text += toHex((byte & 0xF0) >> 4);
      text += toHex(byte & 0x0F);
      text += ' ';
    }
    break;
  default:
    // never happens
    break;
  }
  textBrowser->setPlainText(textBrowser->toPlainText() + text);
  textBrowser->verticalScrollBar()->setValue(
      textBrowser->verticalScrollBar()->maximum());
}