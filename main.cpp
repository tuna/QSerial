#include "drivers/libusb.h"
#include "mainwindow.h"
#include <QApplication>

libusb_context *context = nullptr;

int main(int argc, char *argv[]) {
  auto rc = libusb_init(&context);
  Q_ASSERT(rc >= 0);
  Q_ASSERT(context != nullptr);

  QCoreApplication::setOrganizationName("TUNA");
  QCoreApplication::setOrganizationDomain("tuna.tsinghua.edu.cn");
  QCoreApplication::setApplicationName("QSerial");

  QApplication app(argc, argv);

  MainWindow mw;
  mw.show();

  return app.exec();
}
