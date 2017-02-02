
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include "qcustomplot.h" // the header file of QCustomPlot. Don't forget to add it to your project, if you use an IDE, so it gets compiled.
#include "networkcontroller.h"
#include "DspFilters/Dsp.h"
#include <iostream>

class NetworkController;

namespace Ui {
class MainWindow;
}

using namespace std;

class MainWindow : public QMainWindow
{
  Q_OBJECT
  
public:
  explicit MainWindow(QWidget *parent = 0, NetworkController *netc = 0);
  ~MainWindow();
  void setupDemo(int demoIndex);
  void setupPlot(QCustomPlot *customPlot);
  
private slots:
  void realtimeDataSlot();
  void redraw();
  void bracketDataSlot();
  void screenShot();
  void allScreenShots();

protected:
	void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;
  
private:
  Ui::MainWindow *ui;
  QString demoName;
  QTimer dataTimer;
  QCPItemTracer *itemDemoPhaseTracer;
  int currentDemoIndex;
  NetworkController *nc;
  int sampleRate = 100000;
  int xrange = 100000;
  int yrange = 5000;
  double QFactor = 0.3;
  int updatePlot[3] = {1, 1, 0};

  bool updatePlots = 1;
  bool lowPass = 1;
  bool highPass = 1;
};

#endif // MAINWINDOW_H
