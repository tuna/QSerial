#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "serialport.h"
#include "ui_mainwindow.h"
#include <QMainWindow>
#include <QSerialPort>
#include <QWidget>

class MainWindow : public QMainWindow, private Ui::MainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);

private slots:
  void onSend();
  void onDataReceived(QByteArray data);

private:
  QList<SerialPort *> ports;
};

#endif