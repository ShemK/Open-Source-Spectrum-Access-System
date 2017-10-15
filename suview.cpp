#include "suview.h"
#include "ui_suview.h"

#include <QtCharts>


SuView::SuView(int pos, QWidget *parent) :
  QDialog(parent),
  ui(new Ui::SuView)
{
  ui->setupUi(this);
  rem = (Rem *) parent;
  this->pos = pos;
  pthread_create(&listener_process, NULL, su_listener, (void *)this);
}

SuView::~SuView()
{
  pthread_mutex_lock(&listener_mutex);
  app_open = false;
  pthread_mutex_unlock(&listener_mutex);
  std::cout << "Deleted\n";
  pthread_join(listener_process,NULL);
  delete ui;
}


void *su_listener(void *_arg){
  SensorView *node = (SensorView *)_arg;

  while(node->app_open){
    usleep(1000000);

    QCoreApplication::postEvent(node, new QEvent(QEvent::UpdateRequest),
                                Qt::LowEventPriority);
    QMetaObject::invokeMethod(node, "updateView");


  }
  pthread_exit(0);
}


void SuView::updateView(){


  Rem::performanceStats currentStat;
  bool newValue = false;
  pthread_mutex_lock(&listener_mutex);
  if (rem->known_nodes[pos].tx_info.stats.size() > 0){
    while(rem->known_nodes[pos].tx_info.stats.size() > 0){
        currentStat = rem->known_nodes[pos].tx_info.stats.front();
        rem->known_nodes[pos].tx_info.stats.pop();
        bitrate.push_back(currentStat.bitrate);
    }

    newValue = true;
  }
  pthread_mutex_unlock(&listener_mutex);
  if(newValue){
      QLineSeries *series = new QLineSeries();
      for(unsigned int i = 0; i < bitrate.size(); i++){
          //series->append(i, bitrate[i]);
          *series << QPointF(i,bitrate[i]);
        }



      QtCharts::QChart *chart = new QtCharts::QChart();
      //chart->createDefaultAxes();
      chart->legend()->hide();
      chart->setTitle("BitRate Chart");


      QValueAxis *axisX = new QValueAxis;
      axisX->setTickCount(10);

      chart->addAxis(axisX, Qt::AlignBottom);

      chart->addSeries(series);

      QValueAxis *axisY = new QValueAxis;
      axisY->setLinePenColor(series->pen().color());
      axisY->setMin(0);
      //series->attachAxis(axisX);




      QChartView *chartView = new QChartView(chart);
      chartView->setRenderHint(QPainter::Antialiasing);

      chart->addAxis(axisY, Qt::AlignLeft);

      series->attachAxis(axisX);
      series->attachAxis(axisY);

      ui->myGrid->addWidget(chartView,0,0);

    }

}

void SuView::on_SuView_destroyed()
{
  app_open = false;
}
