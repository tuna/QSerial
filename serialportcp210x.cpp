#include "serialportcp210x.h"
#include <QDebug>
#include <QVector>

const unsigned short ESCAPE_CHAR = 0x1e;

SerialPortCP210X::SerialPortCP210X(QObject *parent, libusb_device *device)
    : SerialPort(parent) {
  libusb_ref_device(device);
  this->device = device;
  handle = nullptr;
  thread = nullptr;
  breakTimer = nullptr;
}

SerialPortCP210X::~SerialPortCP210X() { libusb_unref_device(device); }

QString SerialPortCP210X::portName() {
  return QString("CP210x Bus %1 Addr %2")
      .arg(libusb_get_bus_number(device))
      .arg(libusb_get_device_address(device));
}
void SerialPortCP210X::setBaudRate(qint32 baudRate) {
  // SET_BAUDRATE
  auto rc = libusb_control_transfer(handle, 0b01000001, 0x1E, 0, 0,
                                    (unsigned char *)&baudRate,
                                    sizeof(baudRate), 300);
  Q_ASSERT(rc == 0);
}
void SerialPortCP210X::setParity(QSerialPort::Parity parity) {
  uint16_t lineCtl;
  // GET_LINE_CTL
  auto rc =
      libusb_control_transfer(handle, 0b11000001, 0x04, 0, 0,
                              (unsigned char *)&lineCtl, sizeof(lineCtl), 300);
  Q_ASSERT(rc == 0);
  const uint16_t mapping[] = {0, 65535, 2,
                              1, 4,     3}; // No, Even, Odd, Space, Mark
  lineCtl = (lineCtl & 0b1111111100001111) | (mapping[parity] << 4);
  // SET_LINE_CTL
  rc = libusb_control_transfer(handle, 0b01000001, 0x03, 0, 0,
                               (unsigned char *)&lineCtl, sizeof(lineCtl), 300);
  Q_ASSERT(rc == 0);
}
void SerialPortCP210X::setDataBits(QSerialPort::DataBits dataBits) {
  uint16_t lineCtl;
  // GET_LINE_CTL
  auto rc =
      libusb_control_transfer(handle, 0b11000001, 0x04, 0, 0,
                              (unsigned char *)&lineCtl, sizeof(lineCtl), 300);
  Q_ASSERT(rc == 0);
  lineCtl = (lineCtl & 0b0000000011111111) | (dataBits << 8);
  // SET_LINE_CTL
  rc = libusb_control_transfer(handle, 0b01000001, 0x03, 0, 0,
                               (unsigned char *)&lineCtl, sizeof(lineCtl), 300);
  Q_ASSERT(rc == 0);
}
void SerialPortCP210X::setStopBits(QSerialPort::StopBits stopBits) {
  uint16_t lineCtl;
  // GET_LINE_CTL
  auto rc =
      libusb_control_transfer(handle, 0b11000001, 0x04, 0, 0,
                              (unsigned char *)&lineCtl, sizeof(lineCtl), 300);
  Q_ASSERT(rc == 0);
  const uint16_t mapping[] = {65535, 0, 2, 1}; // One, Two, OneAndHalf
  lineCtl = (lineCtl & 0b1111111111110000) | mapping[stopBits];
  // SET_LINE_CTL
  rc = libusb_control_transfer(handle, 0b01000001, 0x03, 0, 0,
                               (unsigned char *)&lineCtl, sizeof(lineCtl), 300);
  Q_ASSERT(rc == 0);
}
void SerialPortCP210X::setFlowControl(QSerialPort::FlowControl flowControl) {
  // not implemented yet
  Q_UNUSED(flowControl);
}
bool SerialPortCP210X::open() {
  auto rc = libusb_open(device, &handle);
  if (rc == 0) {
    rc = libusb_detach_kernel_driver(handle, 0);
    Q_ASSERT(rc == 0);
    rc = libusb_set_configuration(handle, 0);
    Q_ASSERT(rc == 0);
    rc = libusb_claim_interface(handle, 0);
    Q_ASSERT(rc == 0);

    // IFC_ENABLE
    auto rc = libusb_control_transfer(handle, 0b01000001, 0x00, 1, 0, nullptr,
                                      0, 300);
    Q_ASSERT(rc == 0);

    // FIXME: BREAK recv not working yet
    // EMBED_EVENTS
    rc = libusb_control_transfer(handle, 0b01000001, 0x15, ESCAPE_CHAR, 0,
                                 nullptr, 0, 300);
    Q_ASSERT(rc == 0);

    thread = QThread::create([this] {
      char data[64] = {0};
      int len = 0;
      while (true) {
        auto rc = libusb_bulk_transfer(handle, 0x81, (unsigned char *)data,
                                       sizeof(data), &len, 0);
        if (rc == 0) {
          emit this->receivedData(QByteArray(data, len));
        }
      }
    });
    thread->start();
  }
  return rc == 0;
}
bool SerialPortCP210X::isOpen() { return handle != nullptr; }
void SerialPortCP210X::close() {
  thread->terminate();
  thread->wait();
  libusb_close(handle);
  handle = nullptr;
}
void cp210x_callback(libusb_transfer *transfer) {
  delete (unsigned char *)transfer->user_data;
  libusb_free_transfer(transfer);
}

void SerialPortCP210X::sendData(const QByteArray &data) {
  auto transfer = libusb_alloc_transfer(0);
  unsigned char *buffer = new unsigned char[data.length()];
  memcpy(buffer, data.data(), data.length());

  libusb_fill_bulk_transfer(transfer, handle, 0x01, buffer, data.length(),
                            cp210x_callback, buffer, 0);
  libusb_submit_transfer(transfer);
}

QVector<QPair<quint16, quint16>> supportedCP210XDevices = {{0x10C4, 0xEA60},
                                                           {0x10C4, 0xEA70}};

QList<SerialPort *> SerialPortCP210X::availablePorts(QObject *parent) {
  QList<SerialPort *> result;
  libusb_device **list = nullptr;
  auto count = libusb_get_device_list(context, &list);
  for (auto i = 0; i < count; i++) {
    auto device = list[i];
    libusb_device_descriptor desc = {};
    auto rc = libusb_get_device_descriptor(device, &desc);
    if (rc == 0) {
      for (auto dev : supportedCP210XDevices) {
        if (dev.first == desc.idVendor && dev.second == desc.idProduct) {
          // found
          result.append(new SerialPortCP210X(parent, device));
          break;
        }
      }
    }
  }
  libusb_free_device_list(list, true);
  return result;
}

void SerialPortCP210X::triggerBreak(uint msecs) {
  // SET_BREAK
  auto rc =
      libusb_control_transfer(handle, 0b01000001, 0x05, 1, 0, nullptr, 0, 300);
  Q_ASSERT(rc == 0);

  if (breakTimer) {
    breakTimer->stop();
    delete breakTimer;
  }
  breakTimer = new QTimer(this);
  breakTimer->setSingleShot(true);
  connect(breakTimer, SIGNAL(timeout()), this, SLOT(breakTimeout()));
  breakTimer->start(msecs);
}

void SerialPortCP210X::breakTimeout() {
  // SET_BREAK
  auto rc =
      libusb_control_transfer(handle, 0b01000001, 0x05, 0, 0, nullptr, 0, 300);
  Q_ASSERT(rc == 0);
}