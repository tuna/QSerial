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

private slots:

private:
  QList<SerialPort *> ports;
};

#endif