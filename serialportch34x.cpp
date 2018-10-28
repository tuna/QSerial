#include "serialportch34x.h"
#include <QDebug>
#include <QVector>

#define USB_CTRL_IN (LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_IN)
#define USB_CTRL_OUT (LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_OUT)

#define CH34X_REQ_READ_VERSION 0x5F
#define CH34X_REQ_WRITE_REG 0x9A
#define CH34X_REQ_READ_REG 0x9A
#define CH34X_REQ_SERIAL_INIT 0xA1
#define CH34X_REQ_MODEM_CTRL 0xA4

#define CH34X_REG_BREAK 0x05
#define CH34X_REG_LCR 0x18
#define CH34X_NBREAK_BITS 0x01

#define CH34X_LCR_ENABLE_RX 0x80
#define CH34X_LCR_ENABLE_TX 0x40
#define CH34X_LCR_MARK_SPACE 0x20
#define CH34X_LCR_PAR_EVEN 0x10
#define CH34X_LCR_ENABLE_PAR 0x08
#define CH34X_LCR_STOP_BITS_2 0x04
#define CH34X_LCR_CS8 0x03
#define CH34X_LCR_CS7 0x02
#define CH34X_LCR_CS6 0x01
#define CH34X_LCR_CS5 0x00

#define CH34X_BAUDBASE_FACTOR 1532620800
#define CH34X_BAUDBASE_DIVMAX 3

SerialPortCH34X::SerialPortCH34X(QObject *parent, libusb_device *device)
    : SerialPort(parent) {
  libusb_ref_device(device);
  this->device = device;
  handle = nullptr;
  thread = nullptr;
  breakTimer = nullptr;

  dataBits = CH34X_LCR_CS8;
  parity = 0;
  stopBits = 0;
}

SerialPortCH34X::~SerialPortCH34X() { libusb_unref_device(device); }

QString SerialPortCH34X::portName() {
  return QString("CH34x Bus %1 Addr %2")
      .arg(libusb_get_bus_number(device))
      .arg(libusb_get_device_address(device));
}
void SerialPortCH34X::setBaudRate(qint32 baudRate) {
  // Taken from Linux kernel ch341_set_baudrate_lcr
  uint factor = (CH34X_BAUDBASE_FACTOR / baudRate);
  uint divisor = CH34X_BAUDBASE_DIVMAX;
  while ((factor > 0xfff0) && divisor) {
    factor >>= 3;
    divisor--;
  }

  if (factor > 0xfff0)
    return; // invalid

  factor = 0x10000 - factor;
  quint16 a = (factor & 0xff00) | divisor;
  a |= (1 << 7);

  auto rc = libusb_control_transfer(handle, USB_CTRL_OUT, CH34X_REQ_WRITE_REG,
                                    0x1312, a, nullptr, 0, 300);
  Q_ASSERT(rc == 0);
}

void SerialPortCH34X::setLcr() {
  // Taken from Linux kernel ch341_set_baudrate_lcr
  quint8 lcr = CH34X_LCR_ENABLE_TX | CH34X_LCR_ENABLE_RX;
  lcr |= dataBits;
  lcr |= parity;
  lcr |= stopBits;
  auto rc = libusb_control_transfer(handle, USB_CTRL_OUT, CH34X_REQ_WRITE_REG,
                                    0x2518, lcr, nullptr, 0, 300);
  Q_ASSERT(rc == 0);
}

void SerialPortCH34X::setHandshake(quint8 control) {
  // DTR, RTS mode
  auto rc = libusb_control_transfer(handle, USB_CTRL_OUT, CH34X_REQ_MODEM_CTRL,
                                    ~control, 0, nullptr, 0, 300);
  Q_ASSERT(rc == 0);
}

void SerialPortCH34X::setParity(QSerialPort::Parity parity) {
  switch (parity) {
  case QSerialPort::NoParity:
    this->parity = 0;
    break;
  case QSerialPort::EvenParity:
    this->parity = CH34X_LCR_ENABLE_PAR | CH34X_LCR_PAR_EVEN;
    break;
  case QSerialPort::OddParity:
    this->parity = CH34X_LCR_ENABLE_PAR;
    break;
  case QSerialPort::MarkParity:
    this->parity = CH34X_LCR_ENABLE_PAR | CH34X_LCR_MARK_SPACE;
    break;
  case QSerialPort::SpaceParity:
    this->parity =
        CH34X_LCR_ENABLE_PAR | CH34X_LCR_MARK_SPACE | CH34X_LCR_PAR_EVEN;
    break;
  default:
    break;
  }
  setLcr();
}
void SerialPortCH34X::setDataBits(QSerialPort::DataBits dataBits) {
  switch (dataBits) {
  case QSerialPort::Data5:
    this->dataBits = CH34X_LCR_CS5;
    break;
  case QSerialPort::Data6:
    this->dataBits = CH34X_LCR_CS6;
    break;
  case QSerialPort::Data7:
    this->dataBits = CH34X_LCR_CS8;
    break;
  case QSerialPort::Data8:
    this->dataBits = CH34X_LCR_CS8;
    break;
  default:
    break;
  }
  setLcr();
}
void SerialPortCH34X::setStopBits(QSerialPort::StopBits stopBits) {
  switch (stopBits) {
  case QSerialPort::OneStop:
    this->stopBits = 0;
    break;
  case QSerialPort::TwoStop:
    this->stopBits = CH34X_LCR_STOP_BITS_2;
    break;
  default:
    break;
  }
  setLcr();
}
void SerialPortCH34X::setFlowControl(QSerialPort::FlowControl flowControl) {
  // not implemented yet
  Q_UNUSED(flowControl);
}
bool SerialPortCH34X::open() {
  auto rc = libusb_open(device, &handle);
  if (rc == 0) {
    rc = libusb_detach_kernel_driver(handle, 0);
    Q_ASSERT(rc == 0);
    rc = libusb_set_configuration(handle, 0);
    Q_ASSERT(rc == 0);
    rc = libusb_claim_interface(handle, 0);
    Q_ASSERT(rc == 0);

    unsigned char buffer[2];
    uint size = 2;

    auto rc = libusb_control_transfer(
        handle, USB_CTRL_IN, CH34X_REQ_READ_VERSION, 0, 0, buffer, size, 300);
    Q_ASSERT(rc == 0);
    qWarning() << "CH34x version" << (int)buffer[0];

    rc = libusb_control_transfer(handle, USB_CTRL_OUT, CH34X_REQ_SERIAL_INIT, 0,
                                 0, nullptr, 0, 300);
    Q_ASSERT(rc == 0);

    setBaudRate(9600);
    setLcr();
    setHandshake(0);

    shouldStop = 0;

    thread = QThread::create([this] {
      char data[64] = {0};
      int len = 0;
      while (!shouldStop) {
        auto rc = libusb_bulk_transfer(handle, 0x82, (unsigned char *)data,
                                       sizeof(data), &len, 100);
        if (rc == 0) {
          emit this->receivedData(QByteArray(data, len));
        }
      }
    });
    thread->start();
  }
  return rc == 0;
}
bool SerialPortCH34X::isOpen() { return handle != nullptr; }
void SerialPortCH34X::close() {
  shouldStop = 1;
  thread->wait();
  libusb_close(handle);
  handle = nullptr;
}
void ch34x_callback(libusb_transfer *transfer) {
  delete (unsigned char *)transfer->user_data;
  libusb_free_transfer(transfer);
}

void SerialPortCH34X::sendData(const QByteArray &data) {
  auto transfer = libusb_alloc_transfer(0);
  unsigned char *buffer = new unsigned char[data.length()];
  memcpy(buffer, data.data(), data.length());

  libusb_fill_bulk_transfer(transfer, handle, 0x02, buffer, data.length(),
                            ch34x_callback, buffer, 0);
  libusb_submit_transfer(transfer);
}

QVector<QPair<quint16, quint16>> supportedCH34XDevices = {
    {0x4348, 0x5523}, {0x1A86, 0x7523}, {0x1A86, 0x5523}};

QList<SerialPort *> SerialPortCH34X::availablePorts(QObject *parent) {
  QList<SerialPort *> result;
  libusb_device **list = nullptr;
  auto count = libusb_get_device_list(context, &list);
  for (auto i = 0; i < count; i++) {
    auto device = list[i];
    libusb_device_descriptor desc = {};
    auto rc = libusb_get_device_descriptor(device, &desc);
    if (rc == 0) {
      for (auto dev : supportedCH34XDevices) {
        if (dev.first == desc.idVendor && dev.second == desc.idProduct) {
          // found
          result.append(new SerialPortCH34X(parent, device));
          break;
        }
      }
    }
  }
  libusb_free_device_list(list, true);
  return result;
}

void SerialPortCH34X::triggerBreak(uint msecs) {
  setBreak(true);

  if (breakTimer) {
    breakTimer->stop();
    delete breakTimer;
  }
  breakTimer = new QTimer(this);
  breakTimer->setSingleShot(true);
  connect(breakTimer, SIGNAL(timeout()), this, SLOT(breakTimeout()));
  breakTimer->start(msecs);
}

void SerialPortCH34X::breakTimeout() { setBreak(false); }

void SerialPortCH34X::setBreak(bool set) {
  qWarning() << set;
  quint16 ch34x_break_reg = ((quint16)CH34X_REG_LCR << 8) | CH34X_REG_BREAK;
  quint8 break_reg[2];

  auto rc = libusb_control_transfer(handle, USB_CTRL_IN, CH34X_REQ_READ_REG,
                                    ch34x_break_reg, 0, break_reg,
                                    sizeof(break_reg), 300);
  Q_ASSERT(rc == 0);

  if (set) {
    break_reg[0] &= ~CH34X_NBREAK_BITS;
    break_reg[1] &= ~CH34X_LCR_ENABLE_TX;
  } else {
    break_reg[0] |= CH34X_NBREAK_BITS;
    break_reg[1] |= CH34X_LCR_ENABLE_TX;
  }

  rc = libusb_control_transfer(handle, USB_CTRL_OUT, CH34X_REQ_WRITE_REG,
                               ch34x_break_reg, *(quint16 *)break_reg, nullptr,
                               0, 300);
  Q_ASSERT(rc == 0);
}