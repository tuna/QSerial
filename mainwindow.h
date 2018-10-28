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
  void onReset();
  void onDataReceived(QByteArray data);
  void onIdle();
  void onOpen();
  void onClose();
  void onBreak();
  void onBreakChanged(bool set);
  void onClear();

private:
  QList<SerialPort *> ports;
  void appendText(QString text, QColor color);

  quint64 bytesRecv;
  quint64 bytesSent;
  QList<QPair<quint64, qint64>> recvRecord;
  QList<QPair<quint64, qint64>> sentRecord;
  QTimer *timer;
};

#endif