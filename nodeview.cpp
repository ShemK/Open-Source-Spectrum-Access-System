#include "nodeview.h"
#include "ui_nodeview.h"
#include <QTableWidgetItem>
NodeView::NodeView(int pos,QWidget *parent) :
  QDialog(parent),
  ui(new Ui::NodeView)
{

  ui->setupUi(this);
  std::string title_str = "Node" + std::to_string(pos+1);
  QString title(title_str.c_str());
  this->setWindowTitle(title);
  rem = (Rem *) parent;
  this->pos = pos;
  ui->tableWidget->setColumnCount(3);
  pthread_create(&listener_process, NULL, node_listener, (void *)this);
}

NodeView::~NodeView()
{
  //rem = NULL;
  pthread_mutex_lock(&listener_mutex);
  app_open = false;
  pthread_mutex_unlock(&listener_mutex);
  std::cout << "Deleted\n";
  pthread_join(listener_process,NULL);
  delete ui;
}

void *node_listener(void *_arg){
  NodeView *node = (NodeView *)_arg;

  while(node->app_open){
    usleep(1000);
    pthread_mutex_lock(&node->listener_mutex);
    QCoreApplication::postEvent(node, new QEvent(QEvent::UpdateRequest),
                                Qt::LowEventPriority);
    QMetaObject::invokeMethod(node, "updateView");
    pthread_mutex_unlock(&node->listener_mutex);

  }
  pthread_exit(0);
}

void NodeView::updateView(){

  QStringList tableHeader;
  tableHeader<<"Low Frequency"<<"High Frequency"<<"Occupancy Statistic";
  ui->tableWidget->setHorizontalHeaderLabels(tableHeader);
  unsigned int current_channel = rem->known_nodes[pos].current_channel;
  std::vector<Rem::channelInfo> channels;
  for(unsigned int i = 0; i< rem->known_nodes[pos].channels.size();i++){
      channels.push_back(rem->known_nodes[pos].channels[i]);
  }
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

}

void NodeView::on_NodeView_destroyed()
{
  //std::cout << "destroyed\n";
  app_open = false;
}

void NodeView::on_NodeView_rejected()
{
  //std::cout << "rejected\n";
  app_open = false;
}
