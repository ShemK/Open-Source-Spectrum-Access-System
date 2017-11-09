#include "sensorview.h"
#include "ui_sensorview.h"
#include <QtCharts>

using namespace QtCharts;

SensorView::SensorView(int pos,QWidget *parent) :
  QDialog(parent),
  ui(new Ui::SensorView)
{
  sem_init(&graphic_mutex,0,1);
  ui->setupUi(this);
  std::string title_str = "Node" + std::to_string(pos+1);
  QString title(title_str.c_str());
  this->setWindowTitle(title);
  rem = (Rem *) parent;
  this->pos = pos;
  ui->tableWidget->setColumnCount(3);


  pthread_create(&listener_process, NULL, sensor_listener, (void *)this);
}

SensorView::~SensorView()
{
  //pthread_mutex_lock(&listener_mutex);
  app_open = false;
  //pthread_mutex_unlock(&listener_mutex);
  std::cout << "Deleted\n";
  pthread_join(listener_process,NULL);
  sem_close(&graphic_mutex);
  delete ui->myGrid;
  delete ui;
}

void *sensor_listener(void *_arg){
  SensorView *node = (SensorView *)_arg;

  while(node->app_open){
      //usleep(100000);
      sem_wait(&node->graphic_mutex);
      sem_wait(&node->rem->node_phores[node->pos]);
      QCoreApplication::postEvent(node, new QEvent(QEvent::UpdateRequest),
                                  Qt::HighEventPriority);
      QMetaObject::invokeMethod(node, "updateView");


    }
  pthread_exit(0);
}

void SensorView::updateView(){

  QStringList tableHeader;
  tableHeader<<"Low Frequency"<<"High Frequency"<<"Occupancy Metric";
  ui->tableWidget->setHorizontalHeaderLabels(tableHeader);
  unsigned int current_channel = rem->known_nodes[pos].current_channel;
  std::vector<Rem::channelInfo> channels;



  for(unsigned int i = 0; i< rem->known_nodes[pos].channels.size();i++){
      if(rem->known_nodes[pos].channels[i].lowFrequency <= 3650e6){
          channels.push_back(rem->known_nodes[pos].channels[i]);
        }

    }



  QLineSeries *series = new QLineSeries();
  for(unsigned int i = 0; i < channels.size(); i++){
      //series->append(i, bitrate[i]);
      *series << QPointF(channels[i].lowFrequency/1e9,channels[i].occ);
    }

  QtCharts::QChart *chart = new QtCharts::QChart();
  //chart->createDefaultAxes();
  chart->legend()->hide();
  chart->setTitle("Spectrum Chart");

  QValueAxis *axisX = new QValueAxis;
  axisX->setTickCount(11);
  //axisX->setLinePenColor(series->pen().color());
  QString xTitle("Frequency (GHz)");
  axisX->setTitleText(xTitle);
  chart->addAxis(axisX, Qt::AlignBottom);

  chart->addSeries(series);

  QValueAxis *axisY = new QValueAxis;
  axisY->setLinePenColor(series->pen().color());
  axisY->setMin(0);
  axisY->setRange(0, 0.35);
  QString yTitle("Occupancy Metric");
  axisY->setTitleText(yTitle);



  QChartView *chartView = new QChartView(chart);
  chartView->setRenderHint(QPainter::Antialiasing);

  chart->addAxis(axisY, Qt::AlignLeft);

  series->attachAxis(axisX);
  series->attachAxis(axisY);

  if((int)channels.size() > ui->tableWidget->rowCount()){
      for(int i = 0; i < (int)channels.size()-ui->tableWidget->rowCount();i++){
          ui->tableWidget->insertRow(ui->tableWidget->rowCount()+i);
        }
    }

  for(unsigned int i = 0; i < channels.size();i++){
      std::string temp_str= std::to_string(channels[i].lowFrequency);
      QString temp(temp_str.c_str());
      ui->tableWidget->setItem(i,0,new QTableWidgetItem(temp));
      temp_str= std::to_string(channels[i].highFrequency);
      QString high_temp(temp_str.c_str());
      ui->tableWidget->setItem(i,1,new QTableWidgetItem(high_temp));
      temp_str= std::to_string(channels[i].occ);
      QString occ(temp_str.c_str());
      QTableWidgetItem *occ_widget = new QTableWidgetItem(occ);
      ui->tableWidget->setItem(i,2,occ_widget);
      if(occ_widget!=NULL){
          if(i == current_channel){
              occ_widget->setBackground(Qt::lightGray);
            }
          if(channels[i].occ > rem->occ_threshold){
              occ_widget->setBackground(Qt::red);
            } else if(channels[i].occ > rem->occ_threshold/2){
              occ_widget->setBackground(Qt::yellow);
            }
        }
    }
  ui->tableWidget->setColumnWidth(0,150);
  ui->tableWidget->setColumnWidth(1,150);
  ui->tableWidget->setColumnWidth(2,150);
  // Need to release memory
  // current method not working
  QLayoutItem *temp_item =  ui->myGrid->itemAtPosition(0,0);
  ui->myGrid->removeItem(temp_item);
  delete temp_item;

  ui->myGrid->addWidget(chartView,0,0);
  sem_post(&graphic_mutex);
  //delete series;

}

void SensorView::updateChat(){

}


void SensorView::on_SensorView_destroyed()
{
  app_open = false;
}

void SensorView::on_buttonBox_rejected()
{
  app_open = false;
}

