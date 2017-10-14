#include "rem.h"
#include "ui_rem.h"
#include <QToolButton>

Rem::Rem(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::Rem)
{
  ui->setupUi(this);

  pthread_create(&listener_process, NULL, listener, (void *)this);
}

Rem::~Rem()
{
  app_open = false;
  pthread_join(listener_process,NULL);
  known_nodes.clear();
  delete ui;
}

void Rem::initializeNodeInfo(){
  config_t cfg, *cf;
  cf = &cfg;
  config_init(cf);
  int nodeID;


  if (!config_read_file(cf, "nodeInfo.cfg")) {
      fprintf(stderr, "%s:%d - %s\n",
              config_error_file(cf),
              config_error_line(cf),
              config_error_text(cf));
      config_destroy(cf);
      return ;
    }
  char node[20];
  for(int i = 1; i <= node_num; i++){
      sprintf(node,"Node%d.nodeID",i);
      if(config_lookup_int(cf,node,&nodeID)){
          nodeInfo n;
          n.nodeID = nodeID;
          sprintf(node,"Node%d.latitude",i);
          const char *latitude = NULL;
          config_lookup_string(cf, node, &latitude);

          std::string lat_str(latitude);
          n.latitude = lat_str;
          sprintf(node,"Node%d.longitude",i);
          const char *longitude = NULL;
          config_lookup_string(cf, node, &longitude);
          std::string lon_str(longitude);
          n.longitude = lon_str;

          sprintf(node,"Node%d",i);
          std::string visualID(node);
          n.visualID = visualID;
          known_nodes.push_back(n);

    }
  }
  printf("Known Node Size: %lu\n",known_nodes.size());

  for(unsigned int i = 0; i < known_nodes.size(); i++){
    std::cout << known_nodes[i].nodeID << std::endl;
  }
}

void *listener(void *_arg){
  Rem *rem = (Rem *)_arg;
  rem->initializeNodeInfo();
  struct sockaddr_in udp_server_addr;
  struct sockaddr_in udp_client_addr;
  socklen_t addr_len = sizeof(udp_server_addr);
  memset(&udp_server_addr, 0, addr_len);
  udp_server_addr.sin_family = AF_INET;
  udp_server_addr.sin_addr.s_addr = inet_addr("0.0.0.0");
  udp_server_addr.sin_port = htons(4680);
  int udp_server_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  int status = bind(udp_server_sock, (sockaddr *)&udp_server_addr, addr_len);
  fd_set read_fds;
  int recv_buffer_len = 2000;
  char recv_buffer[recv_buffer_len];
  std::cout << "Here\n";
  if(status < 0){
      std::cout << "Binding Failure" << std::endl;
    }else {
      while(rem->app_open){
          FD_ZERO(&read_fds);
          FD_SET(udp_server_sock, &read_fds);
          struct timeval timeout;
          timeout.tv_sec = 0;
          timeout.tv_usec = 1000;
          if(select(udp_server_sock + 1, &read_fds, NULL, NULL, &timeout) > 0){
              int recv_len = recvfrom(udp_server_sock, recv_buffer, recv_buffer_len, 0,
                                      (struct sockaddr *)&udp_client_addr, &addr_len);
              if(recv_len > 0){
                  //pthread_mutex_lock(&rem->listener_mutex);
                  rem->analyzeInfo(recv_buffer,recv_buffer_len);
                 // pthread_mutex_lock(&rem->listener_mutex);
                }
            }
        }
    }

  close(udp_server_sock);
  pthread_exit(0);
}


void Rem::analyzeInfo(const char *recv_buffer, int recv_len){
  std::string received_string;
  if (recv_len > 0){
      for (int i = 0; i < recv_len; i++)
        {
          received_string.push_back(recv_buffer[i]);
        }
    }
  pmt::pmt_t received_dict = pmt::deserialize_str(received_string);
  pmt::pmt_t not_found = pmt::mp(0);
  pmt::pmt_t nodeID = pmt::dict_ref(received_dict, pmt::string_to_symbol("nodeID"), not_found);
  if(nodeID!=not_found){
      int nodeTemp = pmt::to_long(nodeID);
      int status = getNodePos(nodeTemp);
      if(status==-1){
          printf("new node: %d",nodeTemp);
          nodeInfo n;
          n.nodeID = nodeTemp;
          known_nodes.push_back(n);
        } else{
          // create a function to get associated button
          double occ = pmt::to_double(pmt::dict_ref(received_dict, pmt::string_to_symbol("occ"), not_found));
          double lowerFreq = pmt::to_double(pmt::dict_ref(received_dict, pmt::string_to_symbol("lowerFreq"), not_found));
          double bandwidth = pmt::to_double(pmt::dict_ref(received_dict, pmt::string_to_symbol("bandwidth"), not_found));
          organizeData(status,occ,lowerFreq,bandwidth);
        }
    } else{
      std::cout << "Missing Node ID" << std::endl;
    }
  std::cout << received_dict << std::endl;
}

bool Rem::channelSort(Rem::channelInfo x, Rem::channelInfo y){
  return (x.lowFrequency < y.lowFrequency);
}

void Rem::organizeData(int nodePos, double occ, double lowFreq,double bandwidth) {
  lowFreq = round(lowFreq/1e6)*1e6;
  int pos = getChannelPos(known_nodes[nodePos],lowFreq);
  if(pos == -1){

      channelInfo c;
      c.lowFrequency = lowFreq;
      c.occ = occ;
      known_nodes[nodePos].channels.push_back(c);
      using namespace std::placeholders;
      std::sort(known_nodes[nodePos].channels.begin(),known_nodes[nodePos].channels.end(),std::bind(&Rem::channelSort,this,_1,_2));
      std::cout << "New Channel: " << lowFreq << std::endl;
    } else{
      known_nodes[nodePos].current_channel = pos;
      known_nodes[nodePos].channels[pos].lowFrequency = lowFreq;
      known_nodes[nodePos].channels[pos].bandwidth = bandwidth;
      known_nodes[nodePos].channels[pos].highFrequency = lowFreq + bandwidth;
      known_nodes[nodePos].channels[pos].occ = occ;
      known_nodes[nodePos].channels[pos].occ_history.push_back(occ);
      if(known_nodes[nodePos].channels[pos].occ_history.size() > 20){
        known_nodes[nodePos].channels[pos].occ_history.pop_front();
      }
      QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest),
                                  Qt::LowEventPriority);
      QMetaObject::invokeMethod(this, "updateVisualNode", Q_ARG(int,nodePos));
      std::cout << "Node: " << nodePos << " " << known_nodes[nodePos].channels[pos].lowFrequency << std::endl;
      std::cout << "Node: " << nodePos << " " << known_nodes[nodePos].channels[pos].occ << std::endl;
    }
}


void Rem::updateVisualNode(int nodePos){


  QString below_threshold("background-color: green");
  QString above_threshold("background-color: red");

  QString visualID(known_nodes[nodePos].visualID.c_str());

  QToolButton *button = ui->centralWidget->findChild<QToolButton *>(visualID);

  bool detected = false;
  for(unsigned int i = 0; i < known_nodes[nodePos].channels.size(); i++){
    if(known_nodes[nodePos].channels[i].occ > occ_threshold){
        detected = true;
        for(unsigned int i = 0; i < known_nodes[nodePos].channels.size(); i++){
          std::cout << " | LF" << known_nodes[nodePos].channels[i].lowFrequency << " occ: " << known_nodes[nodePos].channels[i].occ;
        }
        std::cout << "\n";
        break;
    }
  }

  QCoreApplication::postEvent(button, new QEvent(QEvent::UpdateRequest),
                              Qt::LowEventPriority);

  if(detected){
      QMetaObject::invokeMethod(button, "setStyleSheet", Q_ARG(QString, above_threshold));
  } else{
      QMetaObject::invokeMethod(button, "setStyleSheet", Q_ARG(QString, below_threshold));
  }

}
// TODO: Efficient search needed
int Rem::getNodePos(int nodeID)
{
  for (unsigned int i = 0; i < known_nodes.size(); i++)
    {
      if (nodeID == known_nodes[i].nodeID)
        {
          return i;
        }
    }
  return -1;
}
// TODO: Efficient search needed
int Rem::getChannelPos(nodeInfo n,double lowFreq)
{
  for (unsigned int i = 0; i < n.channels.size(); i++)
    {
      if (lowFreq == n.channels[i].lowFrequency)
        {
          return i;
        }
    }
  return -1;
}

// TODO: Efficient search needed
int Rem::getNodePos(std::string visualID)
{
  for (unsigned int i = 0; i < known_nodes.size(); i++)
    {
      if (visualID == known_nodes[i].visualID)
        {
          return i;
        }
    }
  return -1;
}


void Rem::showMoreInfo(std::string visualID){
  int pos = getNodePos(visualID);
  SensorView s(pos, this);
  s.setModal(true);
  s.exec();
}

void Rem::on_Node6_clicked()
{
  showMoreInfo("Node6");
}
void Rem::on_Node11_clicked()
{
  showMoreInfo("Node11");
}


void Rem::on_Node12_clicked()
{
  showMoreInfo("Node12");
}

void Rem::on_Node10_clicked()
{
  showMoreInfo("Node10");
}

void Rem::on_Node8_clicked()
{
  showMoreInfo("Node8");
}

void Rem::on_Node5_clicked()
{
  showMoreInfo("Node5");
}

void Rem::on_Node3_clicked()
{
  showMoreInfo("Node3");
}

void Rem::on_Node2_clicked()
{
  showMoreInfo("Node2");
}

void Rem::on_Node1_clicked()
{
  showMoreInfo("Node1");
}

void Rem::on_Node4_clicked()
{
  showMoreInfo("Node4");
}

void Rem::on_Node7_clicked()
{
  showMoreInfo("Node7");
}

void Rem::on_Node9_clicked()
{
  showMoreInfo("Node9");
}
