#ifndef MUTUALTEST_H
#define MUTUALTEST_H

#include "serialport.h"
#include "ui_mutualtest.h"
#include <QDialog>
#include <QSerialPort>
#include <QWidget>

class MutualTest : public QDialog, private Ui::MutualTest {
  Q_OBJECT

public:
  explicit MutualTest(QWidget *parent = nullptr);

signals:
  void appendText(QString text);

private slots:
  void onBegin();
  void doAppendText(QString text);
  void receivedData(QByteArray data);

private:
  void doWork(int direction);

  QList<SerialPort *> ports;
  QThread *thread;
  QByteArray buffer;
};

#endif