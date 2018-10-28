#include "libusb.h"
#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[]) {
  auto rc = libusb_init(&context);
  Q_ASSERT(rc == 0);

  QApplication app(argc, argv);

  MainWindow mw;
  mw.show();

  return app.exec();
}
