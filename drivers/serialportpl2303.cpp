#include "serialportpl2303.h"

#include <QByteArray>
#include <QDebug>
#include <QMap>

enum pl2303_quirk {
  PL2303_QUIRK_UART_STATE_IDX0 = (1 << 0),
  PL2303_QUIRK_LEGACY = (1 << 1),
  PL2303_QUIRK_ENDPOINT_HACK = (1 << 2),
};
enum pl2303_type {
  TYPE_01,  /* Type 0 and 1 (difference unknown) */
  TYPE_HX,  /* HX version of the pl2303 chip */
  TYPE_HXN, /* HXN version of the pl2303 chip */
  TYPE_COUNT
};

#define VID_PROLIFIC 0x067b

#define VENDOR_WRITE_REQUEST_TYPE 0x40
#define VENDOR_WRITE_REQUEST 0x01
#define VENDOR_WRITE_NREQUEST 0x80

#define VENDOR_READ_REQUEST_TYPE 0xc0
#define VENDOR_READ_REQUEST 0x01
#define VENDOR_READ_NREQUEST 0x81

#define SET_LINE_REQUEST_TYPE 0x21
#define SET_LINE_REQUEST 0x20

#define GET_LINE_REQUEST_TYPE 0xa1
#define GET_LINE_REQUEST 0x21

#define SET_CONTROL_REQUEST_TYPE 0x21
#define SET_CONTROL_REQUEST 0x22
#define CONTROL_DTR 0x01
#define CONTROL_RTS 0x02

#define BREAK_REQUEST_TYPE 0x21
#define BREAK_REQUEST 0x23
#define BREAK_ON 0xffff
#define BREAK_OFF 0x0000

#define PL2303_READ_TYPE_HX_STATUS 0x8080

#define PL2303_HXN_RESET_REG 0x07
#define PL2303_HXN_RESET_UPSTREAM_PIPE 0x02
#define PL2303_HXN_RESET_DOWNSTREAM_PIPE 0x01

static QMap<quint16, quint8> pidAndQuirkOfProlific = {
    {0x2303, PL2303_QUIRK_ENDPOINT_HACK},
    {0x2304, 0},
    {0x23a3, 0},
    {0x23b3, 0},
    {0x23c3, 0},
    {0x23d3, 0},
    {0x23e3, 0},
    {0x23f3, 0},
    {0x04bb, 0},
    {0x1234, 0},
    {0xaaa0, 0},
    {0xaaa2, 0},
    {0xaaa8, 0},
    {0x0611, 0},
    {0x0612, 0},
    {0x0609, 0},
    {0x331a, 0},
    {0x0307, 0},
    {0xe1f1, 0},
};

SerialPortPL2303::SerialPortPL2303(QObject *parent, libusb_device *device)
    : SerialPort(parent) {
  libusb_ref_device(device);
  this->device = device;
  handle = nullptr;
  thread = nullptr;
  breakTimer = nullptr;
  memset(lineOptions, 0, sizeof lineOptions);
}

SerialPortPL2303::~SerialPortPL2303() { libusb_unref_device(device); }

QList<SerialPort *> SerialPortPL2303::availablePorts(QObject *parent) {
  QList<SerialPort *> result;
  libusb_device **list = nullptr;
  auto count = libusb_get_device_list(context, &list);
  for (auto i = 0; i < count; i++) {
    auto device = list[i];
    libusb_device_descriptor desc = {};
    auto rc = libusb_get_device_descriptor(device, &desc);
    if (rc == 0) {
      if (VID_PROLIFIC == desc.idVendor &&
          pidAndQuirkOfProlific.count(desc.idProduct)) {
        // found
        auto inst = new SerialPortPL2303(parent, device);
        quint8 type;
        if (desc.bDeviceClass == 0x02)
          type = TYPE_01; /* type 0 */
        else if (desc.bMaxPacketSize0 == 0x40)
          type = TYPE_HX;
        else if (desc.bDeviceClass == 0x00)
          type = TYPE_01; /* type 1 */
        else if (desc.bDeviceClass == 0xFF)
          type = TYPE_01; /* type 1 */
        inst->setType(type);
        inst->setQuirks(pidAndQuirkOfProlific[desc.idProduct] |
                        (type == TYPE_01 ? PL2303_QUIRK_LEGACY : 0));
        result.append(inst);
        break;
      }
    }
  }
  libusb_free_device_list(list, true);
  return result;
}

bool SerialPortPL2303::open() {
  auto rc = libusb_open(device, &handle);
  if (rc < 0)
    return false;
  if (libusb_kernel_driver_active(handle, 0) == 1) {
    rc = libusb_detach_kernel_driver(handle, 0);
    Q_ASSERT(rc >= 0);
  }
  rc = libusb_set_configuration(handle, 0);
  Q_ASSERT(rc >= 0);
  rc = libusb_claim_interface(handle, 0);
  Q_ASSERT(rc >= 0);

  libusb_config_descriptor *cfgDesc;
  rc = libusb_get_active_config_descriptor(device, &cfgDesc);
  Q_ASSERT(rc >= 0);

  dataEPOut = 0;
  dataEPIn = 0;

  Q_ASSERT(cfgDesc->bNumInterfaces > 0 &&
           cfgDesc->interface[0].num_altsetting > 0);
  auto &ifaceDesc = cfgDesc->interface[0].altsetting[0];
  for (int i = 0; i < ifaceDesc.bNumEndpoints; i++) {
    if (LIBUSB_TRANSFER_TYPE_BULK ==
        (ifaceDesc.endpoint[i].bmAttributes & 0x3)) {
      if ((ifaceDesc.endpoint[i].bEndpointAddress & 0x80))
        dataEPIn = ifaceDesc.endpoint[i].bEndpointAddress;
      else
        dataEPOut = ifaceDesc.endpoint[i].bEndpointAddress;
    }
  }
  qDebug() << "EP_IN" << dataEPIn << "EP_OUT" << dataEPOut;
  Q_ASSERT(dataEPOut && dataEPIn);

  if (type == TYPE_HX) {
    unsigned char buf[1];
    rc = libusb_control_transfer(handle, VENDOR_READ_REQUEST_TYPE,
                                 VENDOR_READ_REQUEST,
                                 PL2303_READ_TYPE_HX_STATUS, 0, buf, 1, 100);
    if (rc != 1)
      type = TYPE_HXN;
  }
  if (type != TYPE_HXN) {
    unsigned char buf[1];
    vendorRead(0x8484, buf);
    vendorWrite(0x0404, 0);
    vendorRead(0x8484, buf);
    vendorRead(0x8383, buf);
    vendorRead(0x8484, buf);
    vendorWrite(0x0404, 1);
    vendorRead(0x8484, buf);
    vendorRead(0x8383, buf);
    vendorWrite(0, 1);
    vendorWrite(1, 0);
    if ((quirks & PL2303_QUIRK_LEGACY))
      vendorWrite(2, 0x24);
    else
      vendorWrite(2, 0x44);
  }
  if (quirks & PL2303_QUIRK_LEGACY) {
    libusb_clear_halt(handle, dataEPIn);
    libusb_clear_halt(handle, dataEPOut);
  } else {
    /* reset upstream data pipes */
    if (type == TYPE_HXN) {
      vendorWrite(PL2303_HXN_RESET_REG, PL2303_HXN_RESET_UPSTREAM_PIPE |
                                            PL2303_HXN_RESET_DOWNSTREAM_PIPE);
    } else {
      vendorWrite(8, 0);
      vendorWrite(9, 0);
    }
  }
  qDebug() << "PL2303 type " << type << "quirks" << quirks;

  getLineOptions();
  lineOptions[6] = 8; // 8 data bits
  lineOptions[5] = 0; // no parity
  lineOptions[4] = 0; // 1 stop bit
  setBaudRate(9600);  // also set other line options
  setControlLines(0);

  shouldStop = 0;

  thread = QThread::create([this] {
    quint8 data[64] = {0};
    while (!shouldStop) {
      int len = 0;
      auto rc =
          libusb_bulk_transfer(handle, dataEPIn, data, sizeof(data), &len, 300);
      if (rc >= 0 || (rc == LIBUSB_ERROR_TIMEOUT && len > 0)) {
        emit this->receivedData(QByteArray((char *)data, len));
      }
    }
  });
  thread->start();
  return true;
}

void SerialPortPL2303::close() {
  if (!isOpen())
    return;
  setBreak(false);
  shouldStop = 1;
  thread->wait();
  libusb_close(handle);
  handle = nullptr;
}

QString SerialPortPL2303::portName() {
  return QString("PL2303 Bus %1 Addr %2")
      .arg(libusb_get_bus_number(device))
      .arg(libusb_get_device_address(device));
}

void SerialPortPL2303::triggerBreak(uint msecs) {
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

void SerialPortPL2303::breakTimeout() { setBreak(false); }

void SerialPortPL2303::setBreak(bool set) {
  auto rc =
      libusb_control_transfer(handle, BREAK_REQUEST_TYPE, BREAK_REQUEST,
                              set ? BREAK_ON : BREAK_OFF, 0, nullptr, 0, 100);
  if (rc < 0) {
    qWarning() << "setBreak" << rc;
  }
}

void SerialPortPL2303::setControlLines(quint8 control) {
  auto rc =
      libusb_control_transfer(handle, SET_CONTROL_REQUEST_TYPE,
                              SET_CONTROL_REQUEST, control, 0, nullptr, 0, 100);
  if (rc < 0) {
    qWarning() << "setControlLines" << rc;
  }
}

bool SerialPortPL2303::getLineOptions() {
  auto rc =
      libusb_control_transfer(handle, GET_LINE_REQUEST_TYPE, GET_LINE_REQUEST,
                              0, 0, lineOptions, 7, 100);
  if (rc != 7) {
    qWarning() << "getLineOptions" << rc;
  }
  return rc == 7;
}

bool SerialPortPL2303::setLineOptions() {
  auto rc =
      libusb_control_transfer(handle, SET_LINE_REQUEST_TYPE, SET_LINE_REQUEST,
                              0, 0, lineOptions, 7, 100);
  if (rc < 0) {
    qWarning() << "setLineOptions" << rc;
  }
  return rc >= 0;
}

void SerialPortPL2303::setParity(QSerialPort::Parity parity) {
  if (currentParity != parity) {
    switch (parity) {
    case QSerialPort::NoParity:
      this->lineOptions[5] = 0;
      break;
    case QSerialPort::EvenParity:
      this->lineOptions[5] = 2;
      break;
    case QSerialPort::OddParity:
      this->lineOptions[5] = 1;
      break;
    case QSerialPort::MarkParity:
      this->lineOptions[5] = 3;
      break;
    case QSerialPort::SpaceParity:
      this->lineOptions[5] = 4;
      break;
    default:
      break;
    }
    if (setLineOptions())
      currentParity = parity;
  }
}

void SerialPortPL2303::setDataBits(QSerialPort::DataBits dataBits) {
  if (currentDataBits != dataBits) {
    switch (dataBits) {
    case QSerialPort::Data5:
      this->lineOptions[6] = 5;
      break;
    case QSerialPort::Data6:
      this->lineOptions[6] = 6;
      break;
    case QSerialPort::Data7:
      this->lineOptions[6] = 7;
      break;
    case QSerialPort::Data8:
      this->lineOptions[6] = 8;
      break;
    default:
      break;
    }
    if (setLineOptions())
      currentDataBits = dataBits;
  }
}

void SerialPortPL2303::setStopBits(QSerialPort::StopBits stopBits) {
  if (currentStopBits != stopBits) {
    switch (stopBits) {
    case QSerialPort::OneStop:
      this->lineOptions[4] = 0;
      break;
    case QSerialPort::OneAndHalfStop:
      this->lineOptions[4] = 1;
      break;
    case QSerialPort::TwoStop:
      this->lineOptions[4] = 2;
      break;
    default:
      break;
    }
    if (setLineOptions())
      currentStopBits = stopBits;
  }
}

void SerialPortPL2303::setFlowControl(QSerialPort::FlowControl flowControl) {
  // not implemented yet
  Q_UNUSED(flowControl);
}

void SerialPortPL2303::setBaudRate(qint32 baudRate) {
  if (currentBaudRate != baudRate) {

    static const quint32 baud_sup[] = {
        75,      150,     300,     600,    1200,   1800,   2400,
        3600,    4800,    7200,    9600,   14400,  19200,  28800,
        38400,   57600,   115200,  230400, 460800, 614400, 921600,
        1228800, 2457600, 3000000, 6000000};
    int len = sizeof(baud_sup) / sizeof(baud_sup[0]);

    auto it = std::lower_bound(baud_sup, baud_sup + len, baudRate);
    if (it != baud_sup + len) {
      if ((quint32)baudRate != *it) {
        // TODO: arbitrary baudrate
      }
      baudRate = *it;
      qDebug() << "set Baudrate to" << baudRate;
      lineOptions[0] = baudRate & 0xff;
      lineOptions[1] = (baudRate >> 8) & 0xff;
      lineOptions[2] = (baudRate >> 16) & 0xff;
      lineOptions[3] = (baudRate >> 24) & 0xff;
      if (setLineOptions())
        currentBaudRate = baudRate;
    } else {
      qWarning() << "Baudrate exceeds" << baud_sup[len - 1];
    }
  }
}

static void pl2303_callback(libusb_transfer *transfer) {
  delete[] (unsigned char *)transfer->user_data;
  libusb_free_transfer(transfer);
}

void SerialPortPL2303::sendData(const QByteArray &data) {
  auto transfer = libusb_alloc_transfer(0);
  unsigned char *buffer = new unsigned char[data.length()];
  memcpy(buffer, data.data(), data.length());

  libusb_fill_bulk_transfer(transfer, handle, dataEPOut, buffer, data.length(),
                            pl2303_callback, buffer, 0);
  libusb_submit_transfer(transfer);
}

bool SerialPortPL2303::vendorRead(quint16 val, unsigned char buf[1]) {
  auto rc = libusb_control_transfer(handle, VENDOR_READ_REQUEST_TYPE,
                                    type == TYPE_HXN ? VENDOR_READ_NREQUEST
                                                     : VENDOR_READ_REQUEST,
                                    val, 0, buf, 1, 100);
  if (rc != 1) {
    qWarning() << "vendorRead" << rc;
  }
  return rc == 1;
}

bool SerialPortPL2303::vendorWrite(quint16 val, quint16 index) {
  auto rc = libusb_control_transfer(handle, VENDOR_WRITE_REQUEST_TYPE,
                                    type == TYPE_HXN ? VENDOR_WRITE_NREQUEST
                                                     : VENDOR_WRITE_REQUEST,
                                    val, index, nullptr, 0, 100);
  if (rc < 0) {
    qWarning() << "vendorWrite" << rc;
  }
  return rc >= 0;
}