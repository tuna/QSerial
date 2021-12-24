#include "drivers/libusb.h"
#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[]) {
  auto rc = libusb_init(&context);
  Q_ASSERT(rc >= 0);

  QCoreApplication::setOrganizationName("TUNA");
  QCoreApplication::setOrganizationDomain("tuna.tsinghua.edu.cn");
  QCoreApplication::setApplicationName("QSerial");

  QApplication app(argc, argv);

  MainWindow mw;
  mw.show();

  return app.exec();
}
