#include "mutualtest.h"

MutualTest::MutualTest(QWidget *parent) : QDialog(parent) {
  setupUi(this);

  ports = SerialPort::getAvailablePorts(this);
  device1ComboBox->clear();
  device2ComboBox->clear();
  for (auto port : ports) {
    device1ComboBox->addItem(port->portName());
    device2ComboBox->addItem(port->portName());
  }
}