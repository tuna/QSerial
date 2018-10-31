#include "mutualtest.h"
#include <QDebug>
#include <QScrollBar>
#include <QThread>

struct Packet {
  quint32 len;
  quint32 direction;
  qint32 baudRate;
  QSerialPort::DataBits dataBits;
  QSerialPort::Parity parity;
  QSerialPort::StopBits stopBits;
  quint32 checksum;
};

MutualTest::MutualTest(QWidget *parent) : QDialog(parent) {
  setupUi(this);

  ports = SerialPort::getAvailablePorts(this);
  device1ComboBox->clear();
  device2ComboBox->clear();

  for (auto port : ports) {
    device1ComboBox->addItem(port->portName());
    connect(port, SIGNAL(receivedData(QByteArray)), this,
            SLOT(receivedData(QByteArray)));
  }
  for (auto port : ports) {
    device2ComboBox->addItem(port->portName());
  }

  connect(this, SIGNAL(appendText(QString)), this, SLOT(doAppendText(QString)));

  buffer.clear();
}

void MutualTest::onBegin() {
  thread = QThread::create([this] {
    doWork(0);
    doWork(1);
  });
  thread->start();
}

void MutualTest::doWork(int direction) {
  // direction=0 : 1 => 2
  // direction=1 : 2 => 1
  appendText("Begin test");
  auto device1 = ports[device1ComboBox->currentIndex()];
  auto device2 = ports[device2ComboBox->currentIndex()];
  auto from = direction ? device2 : device1;
  auto to = direction ? device1 : device2;
  if (!from->open()) {
    from->open();
  }
  if (!to->open()) {
    to->open();
  }

  if (from->isOpen() && to->isOpen()) {
    QList<qint32> allBaudRate = {9600, 115200};
    QList<QSerialPort::DataBits> allDataBits = {
        QSerialPort::Data5, QSerialPOrt::Data6, QSerialPort::Data7,
        QSerialPort::Data8};
    QList<QSerialPort::Parity> allParity = {
        QSerialPort::NoParity, QSerialPort::EvenParity, QSerialPort::OddParity,
        QSerialPort::SpaceParity, QSerialPort::MarkParity};
    QString parityName[] = {"N", "E", "O", "S", "M"};
    QList<QSerialPort::StopBits> allStopBits = {
        QSerialPort::OneStop,
        QSerialPort::TwoStop,
        QSerialPort::OneAndHalfStop,
    };
    char buffer[64] = {0};
    struct Packet *packet = (struct Packet *)buffer;
    for (auto baudRate : allBaudRate) {
      from->setBaudRate(baudRate);
      to->setBaudRate(baudRate);
      for (auto dataBits : allDataBits) {
        from->setDataBits(dataBits);
        to->setDataBits(dataBits);
        for (auto i = 0; i < allParity.size(); i++) {
          auto parity = allParity[i];
          from->setParity(parity);
          to->setParity(parity);
          for (auto stopBits : allStopBits) {
            from->setStopBits(stopBits);
            to->setStopBits(stopBits);

            packet->len = sizeof(struct Packet);
            packet->direction = direction;
            packet->baudRate = baudRate;
            packet->dataBits = dataBits;
            packet->parity = parity;
            packet->stopBits = stopBits;
            packet->checksum = packet->direction + packet->baudRate +
                               packet->dataBits + packet->parity +
                               packet->stopBits;

            from->sendData(QByteArray(buffer, packet->len));

            appendText(QString("%1 %3 %4%5%6")
                           .arg(direction ? "<-" : "->")
                           .arg(baudRate)
                           .arg(dataBits)
                           .arg(parityName[i])
                           .arg(stopBits));
            QThread::msleep(100);
          }
        }
      }
    }
  } else {
    appendText("Failed to open device");
  }
  if (from->isOpen()) {
    from->close();
  }
  if (to->isOpen()) {
    to->close();
  }
}

void MutualTest::doAppendText(QString text) {
  auto cursor = activityTextBrowser->textCursor();
  cursor.movePosition(QTextCursor::End);
  activityTextBrowser->setTextCursor(cursor);
  activityTextBrowser->insertPlainText(text + "\n");
  activityTextBrowser->verticalScrollBar()->setValue(
      activityTextBrowser->verticalScrollBar()->maximum());
}

void MutualTest::receivedData(QByteArray data) {
  buffer.append(data);
  struct Packet *packet = (struct Packet *)buffer.data();
  while (buffer.length() >= sizeof(Packet)) {

    if (packet->len == sizeof(Packet) &&
        packet->checksum == packet->direction + packet->baudRate +
                                packet->dataBits + packet->parity +
                                packet->stopBits &&
        (packet->direction == 0 || packet->direction == 1)) {
      // valid packet
      auto textBrowser =
          packet->direction ? device2TextBrowser : device1TextBrowser;

      QString parityName;
      switch (packet->parity) {
      case QSerialPort::NoParity:
        parityName = "N";
        break;
      case QSerialPort::EvenParity:
        parityName = "E";
        break;
      case QSerialPort::OddParity:
        parityName = "O";
        break;
      case QSerialPort::SpaceParity:
        parityName = "S";
        break;
      case QSerialPort::MarkParity:
        parityName = "M";
        break;
      default:
        parityName = "U";
        break;
      }

      auto text = QString("%1 %2 %3%4%5 OK\n")
                      .arg(packet->direction ? "<-" : "->")
                      .arg(packet->baudRate)
                      .arg(packet->dataBits)
                      .arg(parityName)
                      .arg(packet->stopBits);

      auto cursor = textBrowser->textCursor();
      cursor.movePosition(QTextCursor::End);
      textBrowser->setTextCursor(cursor);
      textBrowser->insertPlainText(text);
      textBrowser->verticalScrollBar()->setValue(
          textBrowser->verticalScrollBar()->maximum());

      buffer.remove(0, sizeof(Packet));
    } else {
      // bad packet
      buffer.remove(0, 1);
    }
  }
}