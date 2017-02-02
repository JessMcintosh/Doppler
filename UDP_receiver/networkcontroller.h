#ifndef RECEIVER_H
#define RECEIVER_H

#include <QVector>
#include <QQueue>
#include <QHostAddress>

class QUdpSocket;

class NetworkController : public QObject
{
    Q_OBJECT

public:
    NetworkController();
	QVector<double> T; 
	QVector<double> T_90; 
	QVector<double> ra; 
	QVector<double> rb; 
	QVector<double> cos_a; 
	QVector<double> sin_a; 
	QVector<double> cos_b; 
	QVector<double> sin_b; 
	QVector<double> cos_a_raw; 
	QVector<double> sin_a_raw; 
	QVector<double> cos_b_raw; 
	QVector<double> sin_b_raw; 
	QVector<double> m; 
	QVector<double> m_filtered; 
	QVector<double> x;
	QVector<double> sin_a_x_cos_b;
	QVector<double> cos_a_x_sin_b;
	QVector<double> cos_b_x_cos_a;
	QVector<double> sin_b_x_sin_a;
	QVector<double> I_par;
	QVector<double> Q_par;
	QVector<double> I_perp;
	QVector<double> Q_perp;
	QQueue<double> q_T;
	QQueue<double> q_ra;
	QQueue<double> q_rb;
	bool updateVectors = 1;

public slots:
    void processPendingDatagrams();
    void sendData(QByteArray data);

private:
    QUdpSocket *udpSocket;
    QHostAddress groupAddress;
	int packetCount = 0;
};

#endif
