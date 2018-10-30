#include "mainwindow.h"
#include "libusb.h"
#include "serialport.h"
#include <QDateTime>
#include <QDebug>
#include <QScrollBar>
#include <QSerialPortInfo>
#include <QTextCodec>
#include <QTimer>

#define INDEX_LINE_LF 0
#define INDEX_LINE_CRLF 1
#define INDEX_LINE_CR 2
#define INDEX_LINE_NONE 3

#define INDEX_SEND_UTF8 0
#define INDEX_SEND_BIG5 1
#define INDEX_SEND_GB18030 2
#define INDEX_SEND_SHIFTJIS 3
#define INDEX_SEND_HEX 4
#define INDEX_SEND_BASE64 5
#define INDEX_SEND_PERCENT 6

#define INDEX_RECV_UTF8 0
#define INDEX_RECV_BIG5 1
#define INDEX_RECV_GB18030 2
#define INDEX_RECV_SHIFTJIS 3
#define INDEX_RECV_HEX 4

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setupUi(this);

  ports = SerialPort::getAvailablePorts(this);
  serialPortComboBox->clear();
  for (auto port : ports) {
    serialPortComboBox->addItem(port->portName());
    connect(port, SIGNAL(receivedData(QByteArray)), this,
            SLOT(onDataReceived(QByteArray)));
    connect(port, SIGNAL(breakChanged(bool)), this, SLOT(onBreakChanged(bool)));
  }

  bytesRecv = 0;
  bytesSent = 0;

  timer = new QTimer(this);
  connect(timer, SIGNAL(timeout()), this, SLOT(onIdle()));
  timer->start(100);
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
  onOpen();
  if (!serialPort->isOpen()) {
    return;
  }

  auto text = inputPlainTextEdit->toPlainText();
  auto codec = QTextCodec::codecForName("UTF-8");
  auto textData = codec->fromUnicode(text);
  QByteArray data;
  bool isFirst;
  int currentByte;
  switch (sendParseAsComboBox->currentIndex()) {
  case INDEX_SEND_UTF8:
    // text utf-8
    data = textData;
    break;
  case INDEX_SEND_BIG5:
    // text big5
    codec = QTextCodec::codecForName("Big5");
    data = codec->fromUnicode(text);
    break;
  case INDEX_SEND_GB18030:
    // text gb18030
    codec = QTextCodec::codecForName("GB18030");
    data = codec->fromUnicode(text);
    break;
  case INDEX_SEND_SHIFTJIS:
    // text shift-jis
    codec = QTextCodec::codecForName("Shift-JIS");
    data = codec->fromUnicode(text);
    break;
  case INDEX_SEND_HEX:
    // hex
    isFirst = true;
    currentByte = 0;
    for (auto ch : textData) {
      if (ch == ' ' || ch == '\r' || ch == '\n') {
        continue;
      }
      if (ch == 'x' || ch == 'X') {
        if (isFirst == false && currentByte == 0) {
          // 0x
          isFirst = true;
          continue;
        } else {
          // fail
          break;
        }
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
  case INDEX_SEND_BASE64:
    // base64
    data = QByteArray::fromBase64(textData);
    break;
  case INDEX_SEND_PERCENT:
    // percent encoding
    data = QByteArray::fromPercentEncoding(textData);
    break;
  default:
    // never happens
    break;
  }

  if (sendParseAsComboBox->currentIndex() != INDEX_SEND_HEX) {
    // no line ending in hex mode
    switch (lineEndingComboBox->currentIndex()) {
    case INDEX_LINE_LF:
      // lf
      data.append('\n');
      break;
    case INDEX_LINE_CRLF:
      // cr lf
      data.append('\r');
      data.append('\n');
      break;
    case INDEX_LINE_CR:
      // cr
      data.append('\r');
      break;
    default:
      // none
      break;
    }
  }

  if (echoCheckBox->isChecked()) {
    if (sendShowTimeCheckBox->isChecked()) {
      text = QString("[%1] %2")
                 .arg(QDateTime::currentDateTime().toString(Qt::ISODate))
                 .arg(text);
    }
    appendText(text, Qt::green);
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
  QTextCodec *codec;
  switch (recvShowAsComboBox->currentIndex()) {
  case INDEX_RECV_UTF8:
    // text utf-8
    codec = QTextCodec::codecForName("UTF-8");
    text = codec->toUnicode(data);
    break;
  case INDEX_RECV_BIG5:
    // text big5
    codec = QTextCodec::codecForName("Big5");
    text = codec->toUnicode(data);
    break;
  case INDEX_RECV_GB18030:
    // text gb18030
    codec = QTextCodec::codecForName("GB18030");
    text = codec->toUnicode(data);
    break;
  case INDEX_RECV_SHIFTJIS:
    // text shift-jis
    codec = QTextCodec::codecForName("Shift-JIS");
    text = codec->toUnicode(data);
    break;
  case INDEX_RECV_HEX:
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
  appendText(text, Qt::red);
}

void MainWindow::appendText(QString text, QColor color) {
  auto cursor = textBrowser->textCursor();
  cursor.movePosition(QTextCursor::End);
  textBrowser->setTextCursor(cursor);
  textBrowser->setTextColor(color);
  textBrowser->insertPlainText(text);
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

void MainWindow::onIdle() {
  struct timeval tv = {};
  libusb_handle_events_timeout_completed(context, &tv, nullptr);
}

void MainWindow::onOpen() {
  auto serialPort = ports[serialPortComboBox->currentIndex()];
  if (!serialPort->isOpen()) {
    if (serialPort->open()) {
      serialPort->setBaudRate(baudRateComboBox->currentText().toInt());
      serialPort->setDataBits(
          (QSerialPort::DataBits)dataBitsComboBox->currentText().toInt());
      QSerialPort::Parity parity[] = {
          QSerialPort::NoParity, QSerialPort::EvenParity,
          QSerialPort::OddParity, QSerialPort::SpaceParity,
          QSerialPort::MarkParity};
      serialPort->setParity(parity[parityComboBox->currentIndex()]);
      serialPort->setStopBits(
          (QSerialPort::StopBits)(stopBitsComboBox->currentIndex() + 1));
      serialPort->setFlowControl(
          (QSerialPort::FlowControl)flowControlComboBox->currentIndex());

      statusBar()->showMessage(tr("%1 %2 %3 %4 %5 %6 Open")
                                   .arg(serialPort->portName())
                                   .arg(serialPort->getBaudRate())
                                   .arg(dataBitsComboBox->currentText())
                                   .arg(parityComboBox->currentText())
                                   .arg(stopBitsComboBox->currentText())
                                   .arg(flowControlComboBox->currentText()));
      serialPortComboBox->setEnabled(false);
      baudRateComboBox->setEnabled(false);
      dataBitsComboBox->setEnabled(false);
      parityComboBox->setEnabled(false);
      stopBitsComboBox->setEnabled(false);
      flowControlComboBox->setEnabled(false);
      sendButton->setText("Send");
      breakPushButton->setEnabled(true);
    } else {
      statusBar()->showMessage("Failed");
    }
  }
}

void MainWindow::onClose() {
  auto serialPort = ports[serialPortComboBox->currentIndex()];
  if (serialPort->isOpen()) {
    serialPort->close();
    statusBar()->showMessage("");
    serialPortComboBox->setEnabled(true);
    baudRateComboBox->setEnabled(true);
    dataBitsComboBox->setEnabled(true);
    parityComboBox->setEnabled(true);
    stopBitsComboBox->setEnabled(true);
    flowControlComboBox->setEnabled(true);
    sendButton->setText("Open and Send");
    breakPushButton->setEnabled(false);
  }
}

void MainWindow::onBreak() {
  auto serialPort = ports[serialPortComboBox->currentIndex()];
  uint time = breakDurationLineEdit->text().toInt();
  serialPort->triggerBreak(time);
}

void MainWindow::onBreakChanged(bool set) {
  if (set) {
    appendText("BREAK ON", Qt::blue);
  } else {
    appendText("BREAK OFF", Qt::blue);
  }
}

void MainWindow::onClear() { textBrowser->setPlainText(""); }