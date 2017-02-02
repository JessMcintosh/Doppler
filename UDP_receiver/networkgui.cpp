#include <QtWidgets>
#include <QtNetwork>
#include <QDebug>
#include <QProcess>

#include "networkgui.h"

NetworkGUI::NetworkGUI(QWidget *parent)
    : QDialog(parent)
{
	controller = new NetworkController();

    statusLabel = new QLabel(tr("Listening for multicasted messages"));
    quitButton = new QPushButton(tr("&Quit"));
    sendButton = new QPushButton(tr("&Send Foo"));
    startButton = new QPushButton(tr("&Setup"));
    calButton = new QPushButton(tr("&Cals 0"));

    connect(quitButton, SIGNAL(clicked()), this, SLOT(close()));
    connect(sendButton, SIGNAL(clicked()), this, SLOT(sendSomething()));
    connect(startButton, SIGNAL(clicked()), this, SLOT(sendStart()));
    connect(calButton, SIGNAL(clicked()), this, SLOT(sendCal()));

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(quitButton);
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(sendButton);
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(startButton);
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(calButton);
    buttonLayout->addStretch(1);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(statusLabel);
    mainLayout->addLayout(buttonLayout);
    setLayout(mainLayout);

    setWindowTitle(tr("TCP client"));
}

void NetworkGUI::sendSomething()
{
	QByteArray data("foobar\r\n");
	//QString data("foobar\r\n");
	controller->sendData(data);
	//if( tcpSocket->waitForConnected() ) {
		//tcpSocket->write( data );
	//}
}

void NetworkGUI::sendStart()
{
	QByteArray data("DXN 0 0\rDLYS 0 0\rSTS -1\rXXA 256\rDOF 1\rPRF 2000\rTXF 1 0 -1\rTXF 1 1 0\rTXF 1 2 0\rTXF 1 3 0\rTXF 1 4 0\rTXF 1 5 0\rTXF 1 6 0\rTXF 1 7 0\rTXF 1 8 0\rTXF 1 9 0\rTXF 1 10 0\rTXF 1 11 0\rTXF 1 12 0\rTXF 1 13 0\rTXF 1 14 0\rTXF 1 15 0\rTXF 1 16 0\rTXF 1 17 0\rTXF 1 18 0\rTXF 1 19 0\rTXF 1 20 0\rTXF 1 21 0\rTXF 1 22 0\rTXF 1 23 0\rTXF 1 24 0\rTXF 1 25 0\rTXF 1 26 0\rTXF 1 27 0\rTXF 1 28 0\rTXF 1 29 0\rTXF 1 30 0\rTXF 1 31 0\rTXF 1 32 0\rTXF 1 33 0\rTXF 1 34 0\rTXF 1 35 0\rTXF 1 36 0\rTXF 1 37 0\rTXF 1 38 0\rTXF 1 39 0\rTXF 1 40 0\rTXF 1 41 0\rTXF 1 42 0\rTXF 1 43 0\rTXF 1 44 0\rTXF 1 45 0\rTXF 1 46 0\rTXF 1 47 0\rTXF 1 48 0\rTXF 1 49 0\rTXF 1 50 0\rTXF 1 51 0\rTXF 1 52 0\rTXF 1 53 0\rTXF 1 54 0\rTXF 1 55 0\rTXF 1 56 0\rTXF 1 57 0\rTXF 1 58 0\rTXF 1 59 0\rTXF 1 60 0\rTXF 1 61 0\rTXF 1 62 0\rTXF 1 63 0\rTXF 1 64 0\rRXF 1 0 -1 0\rRXF 1 1 0 0\rRXF 1 2 0 0\rRXF 1 3 0 0\rRXF 1 4 0 0\rRXF 1 5 0 0\rRXF 1 6 0 0\rRXF 1 7 0 0\rRXF 1 8 0 0\rRXF 1 9 0 0\rRXF 1 10 0 0\rRXF 1 11 0 0\rRXF 1 12 0 0\rRXF 1 13 0 0\rRXF 1 14 0 0\rRXF 1 15 0 0\rRXF 1 16 0 0\rRXF 1 17 0 0\rRXF 1 18 0 0\rRXF 1 19 0 0\rRXF 1 20 0 0\rRXF 1 21 0 0\rRXF 1 22 0 0\rRXF 1 23 0 0\rRXF 1 24 0 0\rRXF 1 25 0 0\rRXF 1 26 0 0\rRXF 1 27 0 0\rRXF 1 28 0 0\rRXF 1 29 0 0\rRXF 1 30 0 0\rRXF 1 31 0 0\rRXF 1 32 0 0\rRXF 1 33 0 0\rRXF 1 34 0 0\rRXF 1 35 0 0\rRXF 1 36 0 0\rRXF 1 37 0 0\rRXF 1 38 0 0\rRXF 1 39 0 0\rRXF 1 40 0 0\rRXF 1 41 0 0\rRXF 1 42 0 0\rRXF 1 43 0 0\rRXF 1 44 0 0\rRXF 1 45 0 0\rRXF 1 46 0 0\rRXF 1 47 0 0\rRXF 1 48 0 0\rRXF 1 49 0 0\rRXF 1 50 0 0\rRXF 1 51 0 0\rRXF 1 52 0 0\rRXF 1 53 0 0\rRXF 1 54 0 0\rRXF 1 55 0 0\rRXF 1 56 0 0\rRXF 1 57 0 0\rRXF 1 58 0 0\rRXF 1 59 0 0\rRXF 1 60 0 0\rRXF 1 61 0 0\rRXF 1 62 0 0\rRXF 1 63 0 0\rRXF 1 64 0 0\rTXN 256 1\rRXN 256 1\rSWP 1 256\rDIS 0\rDISS 0\rENA 256\rENAS 1\rGAN 0 160\rGANS 0 160\rGAT 0 0 1000\rGATS 0 0 1000\rPAV 1 128 100\rPAW 1 128 80\rFRQS 0 4 1\rAWF 0 1\rAWFS 0 1\rAMP 0 13 0\rAMPS 0 13 0\rBUFF 0\rCALS 0\r");
	controller->sendData(data);
}

void NetworkGUI::sendCal()
{
	QByteArray data("CALS 0\r");
	controller->sendData(data);
}
