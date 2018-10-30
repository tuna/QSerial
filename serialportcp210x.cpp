#include "serialportcp210x.h"
#include <QDebug>
#include <QVector>

#define CP210X_CTRL_IN                                                         \
  (LIBUSB_RECIPIENT_INTERFACE | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_IN)
#define CP210X_CTRL_OUT                                                        \
  (LIBUSB_RECIPIENT_INTERFACE | LIBUSB_REQUEST_TYPE_VENDOR |                   \
   LIBUSB_ENDPOINT_OUT)

#define CP210X_DATA_IN (0x1 | LIBUSB_ENDPOINT_IN)
#define CP210X_DATA_OUT (0x1 | LIBUSB_ENDPOINT_OUT)

#define CP210X_REQ_IFC_ENABLE 0x00
#define CP210X_REQ_SET_LINE_CTL 0x03
#define CP210X_REQ_GET_LINE_CTL 0x04
#define CP210X_REQ_SET_BREAK 0x05
#define CP210X_REQ_GET_COMM_STATUS 0x10
#define CP210X_REQ_GET_BAUDRATE 0x1D
#define CP210X_REQ_SET_BAUDRATE 0x1E

#define CP210X_LINE_CTL_PARITY_MASK 0x00f0
#define CP210X_LINE_CTL_PARITY_NONE 0x0000
#define CP210X_LINE_CTL_PARITY_ODD 0x0010
#define CP210X_LINE_CTL_PARITY_EVEN 0x0020
#define CP210X_LINE_CTL_PARITY_MARK 0x0040
#define CP210X_LINE_CTL_PARITY_SPACE 0x0080

#define TIMEOUT 300

struct Q_PACKED SerialStatusResponse {
  quint32 ulErrors;
  quint32 ulHoldReasons;
  quint32 ulAmountInInQueue;
  quint32 ulAmountInOutQueue;
  quint8 bEofReceived;
  quint8 bWaitForImmediate;
  quint8 bReserved;
};

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
  if (baudRate != currentBaudRate) {
    // SET_BAUDRATE
    auto rc = libusb_control_transfer(
        handle, CP210X_CTRL_OUT, CP210X_REQ_SET_BAUDRATE, 0, 0,
        (unsigned char *)&baudRate, sizeof(baudRate), TIMEOUT);
    Q_ASSERT(rc >= 0);

    // GET_BAUDRATE
    rc = libusb_control_transfer(
        handle, CP210X_CTRL_IN, CP210X_REQ_GET_BAUDRATE, 0, 0,
        (unsigned char *)&currentBaudRate, sizeof(currentBaudRate), TIMEOUT);
    Q_ASSERT(rc >= 0);
  }
}

void SerialPortCP210X::setParity(QSerialPort::Parity parity) {
  if (currentParity != parity) {
    uint16_t lineCtl;
    // GET_LINE_CTL
    auto rc = libusb_control_transfer(
        handle, CP210X_CTRL_IN, CP210X_REQ_GET_LINE_CTL, 0, 0,
        (unsigned char *)&lineCtl, sizeof(lineCtl), TIMEOUT);
    Q_ASSERT(rc >= 0);
    lineCtl &= ~CP210X_LINE_CTL_PARITY_MASK;
    switch (parity) {
    case QSerialPort::NoParity:
      lineCtl |= CP210X_LINE_CTL_PARITY_NONE;
      break;
    case QSerialPort::EvenParity:
      lineCtl |= CP210X_LINE_CTL_PARITY_EVEN;
      break;
    case QSerialPort::OddParity:
      lineCtl |= CP210X_LINE_CTL_PARITY_ODD;
      break;
    case QSerialPort::SpaceParity:
      lineCtl |= CP210X_LINE_CTL_PARITY_SPACE;
      break;
    case QSerialPort::MarkParity:
      lineCtl |= CP210X_LINE_CTL_PARITY_MARK;
      break;
    }
    // SET_LINE_CTL
    rc = libusb_control_transfer(
        handle, CP210X_CTRL_OUT, CP210X_REQ_SET_LINE_CTL, 0, 0,
        (unsigned char *)&lineCtl, sizeof(lineCtl), TIMEOUT);
    Q_ASSERT(rc >= 0);
    currentParity = parity;
  }
}

void SerialPortCP210X::setDataBits(QSerialPort::DataBits dataBits) {
  uint16_t lineCtl;
  // GET_LINE_CTL
  auto rc = libusb_control_transfer(
      handle, CP210X_CTRL_IN, CP210X_REQ_GET_LINE_CTL, 0, 0,
      (unsigned char *)&lineCtl, sizeof(lineCtl), TIMEOUT);
  Q_ASSERT(rc >= 0);
  lineCtl = (lineCtl & 0b0000000011111111) | (dataBits << 8);
  // SET_LINE_CTL
  rc = libusb_control_transfer(handle, CP210X_CTRL_OUT, CP210X_REQ_SET_LINE_CTL,
                               0, 0, (unsigned char *)&lineCtl, sizeof(lineCtl),
                               TIMEOUT);
  Q_ASSERT(rc >= 0);
}

void SerialPortCP210X::setStopBits(QSerialPort::StopBits stopBits) {
  uint16_t lineCtl;
  // GET_LINE_CTL
  auto rc = libusb_control_transfer(
      handle, CP210X_CTRL_IN, CP210X_REQ_GET_LINE_CTL, 0, 0,
      (unsigned char *)&lineCtl, sizeof(lineCtl), TIMEOUT);
  Q_ASSERT(rc >= 0);
  const uint16_t mapping[] = {65535, 0, 2, 1}; // One, Two, OneAndHalf
  lineCtl = (lineCtl & 0b1111111111110000) | mapping[stopBits];
  // SET_LINE_CTL
  rc = libusb_control_transfer(handle, CP210X_CTRL_OUT, CP210X_REQ_SET_LINE_CTL,
                               0, 0, (unsigned char *)&lineCtl, sizeof(lineCtl),
                               TIMEOUT);
  Q_ASSERT(rc >= 0);
}

void SerialPortCP210X::setFlowControl(QSerialPort::FlowControl flowControl) {
  // not implemented yet
  Q_UNUSED(flowControl);
}

bool SerialPortCP210X::open() {
  auto rc = libusb_open(device, &handle);
  if (rc >= 0) {
    if (libusb_kernel_driver_active(handle, 0) == 1) {
      rc = libusb_detach_kernel_driver(handle, 0);
      Q_ASSERT(rc >= 0);
    }
    rc = libusb_set_configuration(handle, 0);
    Q_ASSERT(rc >= 0);
    rc = libusb_claim_interface(handle, 0);
    Q_ASSERT(rc >= 0);

    // IFC_ENABLE
    auto rc =
        libusb_control_transfer(handle, CP210X_CTRL_OUT, CP210X_REQ_IFC_ENABLE,
                                1, 0, nullptr, 0, TIMEOUT);
    Q_ASSERT(rc >= 0);

    shouldStop = 0;

    thread = QThread::create([this] {
      quint8 data[64] = {0};
      SerialStatusResponse resp;
      bool breakOn = false;
      while (!shouldStop) {
        int len = 0;
        auto rc = libusb_bulk_transfer(handle, CP210X_DATA_IN, data,
                                       sizeof(data), &len, TIMEOUT);
        if (rc >= 0 || (rc == LIBUSB_ERROR_TIMEOUT && len > 0)) {
          emit this->receivedData(QByteArray((char *)data, len));
        }

        rc = libusb_control_transfer(handle, CP210X_CTRL_IN,
                                     CP210X_REQ_GET_COMM_STATUS, 0, 0,
                                     (quint8 *)&resp, sizeof(data), TIMEOUT);
        if (rc == sizeof(resp)) {
          if ((resp.ulErrors & 1) != breakOn) {
            // BREAK Changed
            breakOn = resp.ulErrors & 1;
            emit breakChanged(breakOn);
          }
        }
      }
    });
    thread->start();
  }
  return rc >= 0;
}

bool SerialPortCP210X::isOpen() { return handle != nullptr; }

void SerialPortCP210X::close() {
  shouldStop = 1;
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

  libusb_fill_bulk_transfer(transfer, handle, CP210X_DATA_OUT, buffer,
                            data.length(), cp210x_callback, buffer, 0);
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
    if (rc >= 0) {
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
  auto rc = libusb_control_transfer(
      handle, CP210X_CTRL_OUT, CP210X_REQ_SET_BREAK, 1, 0, nullptr, 0, TIMEOUT);
  Q_ASSERT(rc >= 0);

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
  auto rc = libusb_control_transfer(
      handle, CP210X_CTRL_OUT, CP210X_REQ_SET_BREAK, 0, 0, nullptr, 0, TIMEOUT);
  Q_ASSERT(rc >= 0);
}