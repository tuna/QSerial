#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "drivers/serialport.h"
#include "ui_mainwindow.h"
#include <QMainWindow>
#include <QSerialPort>
#include <QWidget>
#include <QJsonArray>
#include <QSettings>

class MainWindow : public QMainWindow, private Ui::MainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  void sendBytes(const QByteArray &data);

protected:
   void resizeEvent(QResizeEvent *event);

private slots:
  void onReset();
  void onSend();
  void onBreak();
  void onClear();
  void onOpen();
  void onClose();
  void onToggleOpen();
  void onMutualTest();
  void onTabPageChanged(int index);
  void refreshStatistics();

  void onDataReceived(QByteArray data);
  void onIdle();
  void onBreakChanged(bool set);

private:
  QList<SerialPort *> ports;
  void appendText(QString text, QColor color);
  bool eventFilter(QObject *object, QEvent *event);
  void fitTerminal();
  void refreshOpenStatus();
  void saveSettings();
  void loadSettings();

  quint64 bytesRecv;
  quint64 bytesSent;
  QList<QPair<quint64, qint64>> recvRecord;
  QList<QPair<quint64, qint64>> sentRecord;
  QTimer *timer;
  QLabel *statisticsLabel;
  QIcon playIcon, stopIcon;
  bool isOpened;
  QSettings settings;
};

class JsInterface : public QObject
{
  Q_OBJECT
private:
  MainWindow* parentWindow;

public:
  JsInterface(MainWindow *_parent) : QObject(_parent) {
    parentWindow = _parent;
  }

  Q_INVOKABLE void sendBytes(const QJsonArray& data) const;
};

#endif
