#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "drivers/serialport.h"
#include "ui_mainwindow.h"
#include <QMainWindow>
#include <QSerialPort>
#include <QWidget>

class MainWindow : public QMainWindow, private Ui::MainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);

protected:
   void resizeEvent(QResizeEvent *event);

private slots:
  void onReset();
  void onSend();
  void onBreak();
  void onClear();
  void onOpen();
  void onClose();
  void onMutualTest();
  void onToggleTerminal();

  void onDataReceived(QByteArray data);
  void onIdle();
  void onBreakChanged(bool set);

private:
  QList<SerialPort *> ports;
  void appendText(QString text, QColor color);
  bool eventFilter(QObject *object, QEvent *event);
  void fitTerminal();

  quint64 bytesRecv;
  quint64 bytesSent;
  QList<QPair<quint64, qint64>> recvRecord;
  QList<QPair<quint64, qint64>> sentRecord;
  QTimer *timer;
  bool terminalShowing;
};

#endif
