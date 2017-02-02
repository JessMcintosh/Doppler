#ifndef RECEIVERGUI_H
#define RECEIVERGUI_H

#include <QDialog>
#include <QHostAddress>
#include "networkcontroller.h"

class QLabel;
class QPushButton;
class NetworkController;

class NetworkGUI : public QDialog
{
    Q_OBJECT

public:
    NetworkGUI(QWidget *parent = 0);
private slots:
    void sendSomething();
    void sendStart();
    void sendCal();

private:
    QLabel *statusLabel;
    QPushButton *quitButton;
    QPushButton *sendButton;
    QPushButton *startButton;
    QPushButton *calButton;
	NetworkController *controller;
};

#endif
