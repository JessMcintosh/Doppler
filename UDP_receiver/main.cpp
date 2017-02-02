#include <QApplication>

#include "networkcontroller.h"
#include "networkgui.h"
#include "mainwindow.h"

class NetworkController;

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	NetworkController *controller = new NetworkController();
	MainWindow w(0, controller);
	w.show();
	return app.exec();
}
