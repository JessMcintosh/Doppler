#include <QtWidgets>
#include <QtNetwork>
#include <QDebug>
#include <QProcess>

#include "networkcontroller.h"

NetworkController::NetworkController()
{
	qDebug() << "initialising network controller";
    groupAddress = QHostAddress("localhost");

    udpSocket = new QUdpSocket(this);
	udpSocket->bind(QHostAddress::LocalHost, 45454);
	//udpSocket->connectToHost(groupAddress, 45454);

	QObject::connect(udpSocket, SIGNAL(readyRead()),
            this, SLOT(processPendingDatagrams()));
	T = QVector<double>(100000);
	T_90 = QVector<double>(100000);
	ra = QVector<double>(100000);
	rb = QVector<double>(100000);
	cos_a = QVector<double>(100000);
	sin_a = QVector<double>(100000);
	cos_b = QVector<double>(100000);
	sin_b = QVector<double>(100000);
	cos_a_raw = QVector<double>(100000);
	sin_a_raw = QVector<double>(100000);
	cos_b_raw = QVector<double>(100000);
	sin_b_raw = QVector<double>(100000);

	sin_a_x_cos_b = QVector<double>(100000);
	cos_a_x_sin_b = QVector<double>(100000);
	cos_b_x_cos_a = QVector<double>(100000);
	sin_b_x_sin_a = QVector<double>(100000);

	I_par 	= QVector<double>(100000);  
	Q_par   = QVector<double>(100000);
	I_perp  = QVector<double>(100000);
	Q_perp	= QVector<double>(100000);

	m = QVector<double>(100000);
	m_filtered = QVector<double>(100000);
	x = QVector<double>(100000);
	for (int i=0; i<100000; ++i)
	{
		q_T.enqueue(i);
		q_ra.enqueue(i);
		q_rb.enqueue(i);
		T[i] = i/5000.0 - 1; 
		T_90[i] = i/5000.0 - 1; 
		m[i] = i/5000.0 - 1; 
		ra[i] = i/5000.0 - 1; 
		rb[i] = i/5000.0 - 1; 
	}
	for (int i=0; i<100000; ++i)
	{
		m_filtered[i] = i/25000.0 - 1; 
		m[i] = i/25000.0 - 1; 
		x[i] = i;
		cos_a[i] = i;
		sin_a[i] = i;
		cos_b[i] = i;
		sin_b[i] = i;
	}
}

void NetworkController::processPendingDatagrams()
{
	//QByteArray data = udpSocket->readAll();
    while (udpSocket->hasPendingDatagrams()) {
		QByteArray datagram;
        datagram.resize(udpSocket->pendingDatagramSize());
        udpSocket->readDatagram(datagram.data(), datagram.size());
        //statusLabel->setText(tr("Received datagram: \"%1\"")
        //                     .arg(datagram.data()));
		
		QDataStream ds(datagram);
		ds.setByteOrder(QDataStream::LittleEndian);

		int packetSize = datagram.size() / (4 * 3);
		//qDebug() << "Packetsize: " << packetSize;


		int value_t;
		int value_r;

		for(int i = 0; i < packetSize; i++){
			packetCount++;
			ds >> value_t;
			ds >> value_r;

			if(updateVectors){

				//value_t /= 100.0;
				q_T.dequeue();
				q_T.enqueue(value_t);
				//T.pop_back();
				//T.push_front(value_t);
				value_r*=250;
				//value_r*=2;
				//ra.pop_back();
				//ra.push_front(value_r);
				q_ra.enqueue(value_r);
				q_ra.dequeue();
				//int value_m = value_t * value_r / 10000.0;
				//q_rb.enqueue(value_m);
				//q_rb.dequeue();
				
				//cos_a_raw.pop_back();
				//cos_a_raw.push_front(value_m);
				//T_90.pop_back();
				//T_90.push_front(T[3]); // 90 degree phase shift
				//value_m = T[3] * value_r / 5000.0;
				//sin_a_raw.pop_back();
				//sin_a_raw.push_front(value_m);

				//ds >> value_r;
				
				//value_r*=200;
				//rb.pop_back();
				//rb.push_front(value_r);
				//value_m = value_t * value_r / 5000.0;
				//cos_b_raw.pop_back();
				//cos_b_raw.push_front(value_m);
				//value_m = T[3] * value_r / 5000.0;
				//sin_b_raw.pop_back();
				//sin_b_raw.push_front(value_m);
			}
		}

		//ds >> value_t;
		//T.pop_back();
		//T.push_front(value_t);
		//ds >> value_r;
		//value_r*=10;
		//r0.pop_back();
		//r0.push_front(value_r);
		//int value_m = value_t * value_r / 5000.0;
		//m.pop_back();
		//m.push_front(value_m);
		////QString s_data = QString(datagram.data());
		////qDebug() << value;
		//T_90.pop_back();
		////T_90.push_front(T[1]);
		//T_90.push_front(T[3]);
		
		if(packetCount > 1000000){
			//QString s_data = QString(datagram.data());
			//qDebug() << s_data;
			//qDebug() << " FINISHED ";
		}
		//char value = (datagram.data())[0];
    }
}

void NetworkController::sendData(QByteArray data)
{
	//QByteArray data("foobar\r\n\0");
	udpSocket->write(data);
	//if( tcpSocket->waitForConnected() ) {
		//tcpSocket->write( data );
	//}
}
