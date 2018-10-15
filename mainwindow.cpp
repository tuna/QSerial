#include "mainwindow.h"
#include "serialport.h"
#include <QDateTime>
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

  bytesRecv = 0;
  bytesSent = 0;
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

inline QString toHumanRate(quint64 rate) {
  if (rate < 1024) {
    return QString("%1 B/s").arg(rate);
  } else if (1024 <= rate && rate < 1024 * 1024) {
    return QString("%1 KiB/s").arg(1.0 * rate / 1024, 0, 'f', 2);
  } else if (1024 * 1024 <= rate && rate < 1024 * 1024 * 1024) {
    return QString("%1 MiB/s").arg(1.0 * rate / 1024 / 1024, 0, 'f', 2);
  } else {
    return "really??";
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
    auto text = inputPlainTextEdit->toPlainText();
    auto codec = QTextCodec::codecForName("UTF-8");
    auto textData = codec->fromUnicode(text);
    QByteArray data;
    bool isFirst;
    int currentByte;
    switch (sendParseAsComboBox->currentIndex()) {
    case 0:
      // text
      data = textData;
      break;
    case 1:
      // hex
      isFirst = true;
      currentByte = 0;
      for (auto ch : textData) {
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
    case 2:
      // base64
      data = QByteArray::fromBase64(textData);
      break;
    default:
      // never happens
      break;
    }

    switch (lineEndingComboBox->currentIndex()) {
    case 0:
      // lf
      data.append('\n');
      break;
    case 1:
      // cr lf
      data.append('\r');
      data.append('\n');
      break;
    case 2:
      // cr
      data.append('\r');
      break;
    default:
      // none
      break;
    }

    if (echoCheckBox->isChecked()) {
      if (sendShowTimeCheckBox->isChecked()) {
        text = QString("[%1] %2")
                   .arg(QDateTime::currentDateTime().toString(Qt::ISODate))
                   .arg(text);
      }
      appendText(text, "green");
    }
    serialPort->sendData(data);

    bytesSent += data.length();
    bytesSentLabel->setText(QString("%1").arg(bytesSent));
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    sentRecord.push_back(QPair<quint64, qint64>(data.length(), currentTime));
    while (sentRecord.first().second < currentTime - 1000) {
      sentRecord.pop_front();
    }
    quint64 speed = 0;
    for (auto pair : sentRecord) {
      speed += pair.first;
    }
    sentSpeedLabel->setText(toHumanRate(speed));
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
  bytesRecv += data.length();
  bytesRecvLabel->setText(QString("%1").arg(bytesRecv));
  qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
  recvRecord.push_back(QPair<quint64, qint64>(data.length(), currentTime));
  while (recvRecord.first().second < currentTime - 1000) {
    recvRecord.pop_front();
  }
  quint64 speed = 0;
  for (auto pair : recvRecord) {
    speed += pair.first;
  }
  recvSpeedLabel->setText(toHumanRate(speed));

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
  if (recvShowTimeCheckBox->isChecked()) {
    text = QString("[%1] %2")
               .arg(QDateTime::currentDateTime().toString(Qt::ISODate))
               .arg(text);
  }
  appendText(text, "red");
}

void MainWindow::appendText(QString text, QString color) {
  text = QString("<font color=\"%1\">%2</font>")
             .arg(color)
             .arg(text.toHtmlEscaped());
  textBrowser->setHtml(textBrowser->toHtml() + text);
  textBrowser->verticalScrollBar()->setValue(
      textBrowser->verticalScrollBar()->maximum());
}

void MainWindow::onReset() {
  bytesRecv = 0;
  bytesRecvLabel->setText("0");
  bytesSent = 0;
  bytesSentLabel->setText("0");
  recvRecord.clear();
  recvSpeedLabel->setText("0");
  sentRecord.clear();
  sentSpeedLabel->setText("0");
}