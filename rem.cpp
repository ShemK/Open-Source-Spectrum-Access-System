#include "rem.h"
#include "ui_rem.h"
#include <QToolButton>
//FREQUENCY( A1:A600,B1:B8)
Rem::Rem(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::Rem)
{
  sem_init(&graphic_mutex,0,1);
  node_phores = new sem_t[node_num+1];
  for(int i = 0; i < node_num+1; i++){
      sem_init(&node_phores[i],0,0);
    }

  ui->setupUi(this);
  pthread_create(&listener_process, NULL, listener, (void *)this);
  experimentRunning = false;
}

Rem::~Rem()
{
  app_open = false;
  pthread_join(listener_process,NULL);
  known_nodes.clear();
  sem_close(&graphic_mutex);
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
          //n.latitude = lat_str;
          sprintf(node,"Node%d.longitude",i);
          const char *longitude = NULL;
          config_lookup_string(cf, node, &longitude);
          std::string lon_str(longitude);
          //n.longitude = lon_str;

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
  //delete rem;
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
  pmt::pmt_t not_found = pmt::mp(-1);
  pmt::pmt_t type = pmt::dict_ref(received_dict, pmt::string_to_symbol("type"), not_found);
  if(type!=not_found){
      pmt::pmt_t nodeID = pmt::dict_ref(received_dict, pmt::string_to_symbol("nodeID"), not_found);
      std::string type_str = pmt::symbol_to_string(type);
      //std::cout << "Type : " << type_str.c_str() << std::endl;

      if(strcmp(type_str.c_str(),"SENSOR") == 0){
          //std::cout << "SENSOR INFO\n";
          if(nodeID!=not_found){
              int nodeTemp = pmt::to_long(nodeID);
              int status = getNodePos(nodeTemp);
              if(status==-1){
                  printf("new node: %d",nodeTemp);
                  nodeInfo n;
                  n.nodeID = nodeTemp;
                  n.type = SENSOR;
                  known_nodes.push_back(n);
                } else{
                  known_nodes.at(status).type = SENSOR;
                  // create a function to get associated button
                  double occ = pmt::to_double(pmt::dict_ref(received_dict, pmt::string_to_symbol("occ"), not_found));
                  double lowerFreq = pmt::to_double(pmt::dict_ref(received_dict, pmt::string_to_symbol("lowerFreq"), not_found));
                  double bandwidth = pmt::to_double(pmt::dict_ref(received_dict, pmt::string_to_symbol("bandwidth"), not_found));
                  organizeData(status,occ,lowerFreq,bandwidth);
                  if(known_nodes[status].log_channels.size() > 0){
                    if(experimentRunning){
                      std::vector<double> &v = known_nodes[status].log_channels;
                      if(std::find(v.begin(), v.end(), lowerFreq) != v.end()) {
                         //std::cout << "Writng: " << lowerFreq << "at" << known_nodes.at(status).nodeID << std::endl;
                        std::string filename = currentExperiment + "/SENSORS/statistics_" + std::to_string(known_nodes.at(status).nodeID) + "_" + std::to_string(lowerFreq) + ".csv";
                        std::fstream writer;
                        writer.open (filename.c_str(), std::fstream::out | std::fstream::app | std::fstream::ate);
                        if(writer.tellp()==0){
                            writer <<"time,occ\n";
                        }
                        if(writer.tellp() > 0){
                          writer << currentTime() << "," << occ <<  "\n";
                        }
                      }
                    } else{
                      known_nodes[status].log_channels.clear();
                    }
                  }
                  //if(known_nodes[status].channels[pos].lowFrequency
                }

            } else{
              //std::cout << "Missing Node ID" << std::endl;
            }
        } else if(strcmp(type_str.c_str(),"SU") == 0){
          if(nodeID!=not_found){
              //std::cout << "SU INFO\n";
              short unsigned int nodeTemp = pmt::to_long(nodeID);
              int status = getNodePos(nodeTemp);
              if(status==-1){
                  printf("new node: %d",nodeTemp);
                  nodeInfo n;
                  n.nodeID = nodeTemp;
                  n.type = SU;
                  known_nodes.push_back(n);

                } else{
                  if(known_nodes.at(status).type!=SU){
                      int sval;
                      known_nodes.at(status).type = SU;
                      sem_getvalue(&graphic_mutex, &sval);
                      //std::cout << "Graphics Sem Value: " << sval << "\n";
                      if(sval >= 1){
                          sval = sem_trywait(&graphic_mutex);
                          if(sval >=0){
                              QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest),
                                                          Qt::LowEventPriority);
                              QMetaObject::invokeMethod(this, "updateVisualNode", Q_ARG(int,status));
                            }

                        }
                    }

                  pmt::pmt_t thru = pmt::dict_ref(received_dict, pmt::string_to_symbol("throughput"), not_found);
                  if(thru!=not_found){
                      double tx_freq = pmt::to_double(pmt::dict_ref(received_dict, pmt::string_to_symbol("tx_freq"), not_found));
                      double rx_freq = pmt::to_double(pmt::dict_ref(received_dict, pmt::string_to_symbol("rx_freq"), not_found));
                      if(tx_freq != known_nodes.at(status).tx_info.tx_freq){
                        known_nodes.at(status).tx_info.new_change = true;
                      }
                      known_nodes.at(status).tx_info.tx_freq = tx_freq;
                      known_nodes.at(status).tx_info.rx_freq = rx_freq;

                      performanceStats stats;
                      stats.bitrate = pmt::to_double(pmt::dict_ref(received_dict, pmt::string_to_symbol("throughput"), not_found));
                      stats.per = pmt::to_double(pmt::dict_ref(received_dict, pmt::string_to_symbol("per"), not_found));
                      if (known_nodes.at(status).tx_info.stats.size() > 10){
                          known_nodes.at(status).tx_info.stats.pop();
                        }
                      known_nodes.at(status).tx_info.stats.push(stats);
                      known_nodes.at(status).tx_info.state = "GRANT ACCEPTED";

                      if(experimentRunning){
                        std::string filename = currentExperiment + "/SU/perf_" + std::to_string(known_nodes.at(status).nodeID) + ".csv";
                        std::fstream writer;
                        writer.open (filename.c_str(), std::fstream::out | std::fstream::app | std::fstream::ate);
                        if(writer.tellp() == 0){
                          writer << "time,bitrate(bps),per\n";
                        }
                        if(writer.tellp() > 0){
                          writer << currentTime() << "," << stats.bitrate << ","<< stats.per <<  "\n";
                        }

                        if(known_nodes.at(status).tx_info.new_change){
                          writer.close();
                          filename = currentExperiment + "/SU/attributes_" + std::to_string(known_nodes.at(status).nodeID) + ".csv";
                          writer.open (filename.c_str(), std::fstream::out | std::fstream::app | std::fstream::ate);
                          if(writer.tellp() == 0){
                            writer << "time,tx_freq,rx_freq\n";
                          }
                          if(writer.tellp() > 0){
                            writer << currentTime() << "," << known_nodes.at(status).tx_info.tx_freq << ","<< known_nodes.at(status).tx_info.rx_freq  <<  "\n";
                          }
                          known_nodes.at(status).tx_info.new_change = false;
                        }
                      }
                    }





                  pmt::pmt_t su_state =   pmt::dict_ref(received_dict, pmt::string_to_symbol("state"), not_found);

                  if(su_state!=not_found){

                      pmt::pmt_t state_pmt = pmt::dict_ref(received_dict, pmt::string_to_symbol("state"), not_found);
                      if(state_pmt!=not_found){
                          known_nodes.at(status).tx_info.state = pmt::symbol_to_string(state_pmt);
                        }


                    }
                    /*
                  pmt::pmt_t group = pmt::dict_ref(received_dict, pmt::string_to_symbol("group"), not_found);

                  if (group!=not_found){
                      std::vector<short unsigned int> value_vector =  pmt::u16vector_elements (group);
                      known_nodes.at(status).tx_info.group = value_vector;
                      //std::cout << "Group Found\n";
                      //pmt::pmt_t values = pmt::dict_values (group);

                    }
                    updateGroupee(nodeTemp,status);
                    */
                  int sval;
                  sem_getvalue(&graphic_mutex, &sval);
                  //std::cout << "Graphics Sem Value: " << sval << "\n";
                  if(sval >= 1){
                      sval = sem_trywait(&graphic_mutex);
                      if(sval >= 0){
                          QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest),
                                                      Qt::LowEventPriority);
                          QMetaObject::invokeMethod(this, "updateVisualNode", Q_ARG(int,status));
                        }

                   }

                }
            } else{
              std::cout << "Missing Node ID" << std::endl;
            }

        } else if(strcmp(type_str.c_str(),"SAS") == 0){

          analyzeSASInfo(received_dict);
        } else if(strcmp(type_str.c_str(),"PU") == 0){
          std::cout << "##############################################################################\n";
          analyzePuInfo(received_dict,nodeID);
        }

    }

  //std::cout << received_dict << std::endl;
}


void Rem::updateGroupee(short unsigned int nodeTemp, int status){
  std::cout << "Updating Groupee\n";
  std::vector<short unsigned int> value_vector = known_nodes.at(status).tx_info.group;
  for(unsigned int i  = 0; i < value_vector.size(); i++){
      if (nodeTemp!=value_vector[i]){
          int pos = getNodePos(value_vector[i]);
          std::cout << "Groupee Pos: "<< pos <<"\n";
          if(pos > 0){
              if(known_nodes.at(pos).type!=SU){
                  known_nodes.at(pos).type = SU;
                  int sval;
                  sem_getvalue(&graphic_mutex, &sval);
                  std::cout << "Graphics Sem Value: " << sval << "\n";
                  if(sval >= 1){
                      sval = sem_trywait(&graphic_mutex);
                      if(sval >= 0){
                          QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest),
                                                      Qt::LowEventPriority);
                          QMetaObject::invokeMethod(this, "updateVisualNode", Q_ARG(int,pos));
                        }

                    }

                  //updateVisualNode(pos);

                }

              known_nodes.at(pos).tx_info = known_nodes.at(status).tx_info;
            }
        }
    }
}



void Rem::analyzePuInfo(pmt::pmt_t & received_dict, pmt::pmt_t nodeID){
  std::cout << "PU INFO\n";
  pmt::pmt_t not_found = pmt::mp(-1);
  if(nodeID!=not_found){
      int nodeTemp = pmt::to_long(nodeID);
      int status = getNodePos(nodeTemp);
      if(status==-1){
          printf("new node: %d",nodeTemp);
          nodeInfo n;
          n.nodeID = nodeTemp;
          n.type = PU;
          known_nodes.push_back(n);
          status = getNodePos(nodeTemp);
        }
          known_nodes.at(status).type = PU;
          // create a function to get associated button
          pmt::pmt_t pu_status = pmt::dict_ref(received_dict, pmt::string_to_symbol("status"), not_found);
          std::string status_str = pmt::symbol_to_string(pu_status);
          if(strcmp(status_str.c_str(),"Starting") == 0){
              std::cout << "Starting Experiment\n";
              createResultFolder();
              double tx_rate = pmt::to_double(pmt::dict_ref(received_dict, pmt::string_to_symbol("tx_rate"), not_found));
              double tx_freq = pmt::to_double(pmt::dict_ref(received_dict, pmt::string_to_symbol("tx_freq"), not_found));
              long int pu_time = currentTime();
              //double pu_time= pmt::to_double(pmt::dict_ref(received_dict, pmt::string_to_symbol("time"), not_found));
              double run_time = pmt::to_double(pmt::dict_ref(received_dict, pmt::string_to_symbol("run_time"), not_found));

              known_nodes.at(status).state = ACTIVE;
              numPUs++;
              std::string filename = currentExperiment + "/attributes.csv";
              //std::cout << filename << std::endl;
              std::fstream writer;
              writer.open (filename.c_str(), std::fstream::out | std::fstream::app | std::fstream::ate);
              //updatePUResults(writer,std::string("tx_freq"),false,false);
              //std::cout << received_dict << std::endl;
              //std::cout << writer.tellp() << "\n";
              updatePUResults(writer,std::to_string(pu_time),true,false);
              updatePUResults(writer,std::to_string(tx_freq),false,false);
              updatePUResults(writer,std::to_string(tx_rate),false,false);
              updatePUResults(writer,std::to_string(run_time),false,false);
              updatePUResults(writer,std::to_string(nodeTemp),false,true);

          }
          else if(strcmp(status_str.c_str(),"Stop") == 0){
              std::cout << "Stopping Experiment\n";
              known_nodes.at(status).state = INACTIVE;
              if(numPUs > 0){
                 numPUs--;
               }
              if(numPUs == 0){
                experimentRunning = false;
              }

          } else if(strcmp(status_str.c_str(),"update") == 0){
              pmt::pmt_t latitude = pmt::dict_ref(received_dict, pmt::string_to_symbol("latitude"), not_found);
              if(latitude!=not_found){
                known_nodes.at(status).latitude = pmt::to_double(latitude);
                known_nodes.at(status).longitude = pmt::to_double(pmt::dict_ref(received_dict, pmt::string_to_symbol("longitude"), not_found));
                std::cout << "Location set to: (" << known_nodes.at(status).latitude << ", " << known_nodes.at(status).longitude << ")\n";
                if(experimentRunning){
                  std::string filename = currentExperiment + "/location_" + std::to_string(known_nodes.at(status).nodeID) + ".csv";
                  std::fstream writer;
                  writer.open (filename.c_str(), std::fstream::out | std::fstream::app | std::fstream::ate);
                  if(writer.tellp() == 0){
                    writer << "time,latitude,longitude\n";
                  }
                  if(writer.tellp() > 0){
                    writer << currentTime() << "," << known_nodes.at(status).latitude << "," << known_nodes.at(status).longitude << "\n";
                  }
                }
              }
          }

    } else{
      std::cout << "Missing NodeID\n";
    }
}

void Rem::analyzeSASInfo(pmt::pmt_t & received_dict){
    pmt::pmt_t not_found = pmt::mp(-1);
    pmt::pmt_t sas_status = pmt::dict_ref(received_dict, pmt::string_to_symbol("status"), not_found);
    std::string status_str = pmt::symbol_to_string(sas_status);
    if(strcmp(status_str.c_str(),"pu_location") == 0){
      std::cout << "SAS INFO\n";
//      std::cout << received_dict << "\n";
      double lat = pmt::to_double(pmt::dict_ref(received_dict, pmt::string_to_symbol("latitude"), not_found));
      double lon = pmt::to_double(pmt::dict_ref(received_dict, pmt::string_to_symbol("longitude"), not_found));
      double freq = pmt::to_double(pmt::dict_ref(received_dict, pmt::string_to_symbol("frequency"), not_found));
      std::cout << "Freq " << freq << std::endl;
      double dist = searchNearestPU(lat,lon);


      std::string filename = currentExperiment + "/SAS/location.csv";
      //std::cout << filename << std::endl;
      std::fstream writer;
      writer.open (filename.c_str(),std::fstream::app);
      updatePUResults(writer,std::to_string(currentTime()),true,false);
      updatePUResults(writer,std::to_string(dist),false,false);


      pmt::pmt_t sensors = pmt::dict_ref(received_dict, pmt::string_to_symbol("sensors"), not_found);
      std::vector<short unsigned int> sensor_vector =  pmt::u16vector_elements (sensors);
       std::vector<double> distance_vector = pmt::f64vector_elements(pmt::dict_ref(received_dict, pmt::string_to_symbol("distances"), not_found));
       std::cout << "===============================================================\n";
      for(unsigned int i  = 0; i < sensor_vector.size(); i++){
        int j = getNodePos(sensor_vector[i]);
        if(j!=-1){
            std::cout << "Sensor " << sensor_vector[i] << " calculated distance: " << distance_vector[i] << "\n";
            dist = searchNearestPU(known_nodes[j].latitude,known_nodes[j].longitude);
            updatePUResults(writer,std::to_string(std::abs(dist-distance_vector[i])),false,false);
          }
        }
      std::cout << "===============================================================\n";

    } else if(strcmp(status_str.c_str(),"sensor_location") == 0){
         //std::cout << "SAS INFO\n";
        double lat = pmt::to_double(pmt::dict_ref(received_dict, pmt::string_to_symbol("latitude"), not_found));
        double lon = pmt::to_double(pmt::dict_ref(received_dict, pmt::string_to_symbol("longitude"), not_found));
        int sensor_id = pmt::to_long(pmt::dict_ref(received_dict, pmt::string_to_symbol("sensor_id"), not_found));
        int i = getNodePos(sensor_id);
        if(i!=-1){
          //std::cout << "Updating Location for Sensor " << sensor_id << std::endl;
          known_nodes[i].latitude = lat;
          known_nodes[i].longitude = lon;
        }

    }else if(strcmp(status_str.c_str(),"potential_pu") == 0){
      //std::cout << "##############" << received_dict << std::endl;
      std::vector<double> frequencies = pmt::f64vector_elements(pmt::dict_ref(received_dict, pmt::string_to_symbol("frequencies"), not_found));
      int sensor_id = pmt::to_long(pmt::dict_ref(received_dict, pmt::string_to_symbol("sensor_id"), not_found));
      int fccId = pmt::to_long(pmt::dict_ref(received_dict, pmt::string_to_symbol("fccId"), not_found));
      if(experimentRunning){
        std::string filename = currentExperiment + "/SAS/potential_pu.csv";
        std::fstream writer;
        writer.open (filename.c_str(), std::fstream::out | std::fstream::app | std::fstream::ate);
        if(writer.tellp() == 0){
          writer << "time,sensor_id,fccId,frequencies\n";
        }

        int i = getNodePos(sensor_id);

        if(writer.tellp() > 0){
          writer << currentTime() << "," <<  sensor_id << "," << fccId;
          for(unsigned int k = 0; k < frequencies.size();k++){
            frequencies[k] = std::floor(frequencies[k]);
            writer << "," << frequencies[k];
            writer << "\n";

            if(i!=-1){
               std::vector<double>::iterator it = std::find(known_nodes[i].log_channels.begin(), known_nodes[i].log_channels.end(), frequencies[k]);
              if( it == known_nodes[i].log_channels.end()) {
                known_nodes[i].log_channels.push_back(frequencies[k]);
              }
            }
          }
        }
      }
    }
}


double Rem::searchNearestPU(double latitude,double longitude){
  for (unsigned int i = 0; i < known_nodes.size(); i++)
  {
      if(known_nodes.at(i).type == PU){
          double lat = known_nodes.at(i).latitude;
          double lon = known_nodes.at(i).longitude;
        //std::cout << std::setprecision(12) << known_nodes.at(i).latitude << std::endl;
        // std::cout << std::setprecision(12) << latitude << std::endl;
        //std::cout << known_nodes.at(i).longitude << std::endl;
        double dist = 131332796.6*(acos(cos(lat)*cos(lon)*cos(latitude)*cos(longitude) + cos(lat)*sin(lon)*cos(latitude)*sin(longitude) + sin(lat)*sin(latitude))/360)*0.3048;
        std::cout << "Distance: " << dist << "\n ";//<< lat << "," << lon << " & " << latitude << "," << longitude <<  "\n";
        return dist;
      }
  }
  return 0;
}

bool Rem::channelSort(Rem::channelInfo x, Rem::channelInfo y){
  return (x.lowFrequency < y.lowFrequency);
}

void Rem::organizeData(int nodePos, double occ, double lowFreq,double bandwidth) {
  lowFreq = round(lowFreq/1e6)*1e6;
  int pos = getChannelPos(known_nodes[nodePos],lowFreq);

  int sval;
  sem_getvalue(&node_phores[nodePos], &sval);
  //std::cout << "Semaphore Value for " << nodePos << " " << sval << "\n";
  bool info_update = true;
  if(sval > 0 && nodePos !=-1){
      sem_trywait(&node_phores[nodePos]);
      if(sval > 0){

          info_update = false;
        }
    }

  if(info_update){
      if(pos == -1){

          channelInfo c;
          c.lowFrequency = lowFreq;
          c.occ = occ;
          known_nodes[nodePos].channels.push_back(c);
          using namespace std::placeholders;
          std::sort(known_nodes[nodePos].channels.begin(),known_nodes[nodePos].channels.end(),std::bind(&Rem::channelSort,this,_1,_2));
          //std::cout << "New Channel: " << lowFreq << std::endl;
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
          sem_getvalue(&graphic_mutex, &sval);
          //std::cout << "Graphics Sem Value: " << sval << "\n";
          if(sval >= 1){
              sem_wait(&graphic_mutex);
              QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest),
                                          Qt::HighEventPriority);
              QMetaObject::invokeMethod(this, "updateVisualNode", Q_ARG(int,nodePos));
             updateVisualNode(nodePos);
            }

          //std::cout << "Node: " << nodePos << " " << known_nodes[nodePos].channels[pos].lowFrequency << std::endl;
          //std::cout << "Node: " << nodePos << " " << known_nodes[nodePos].channels[pos].occ << std::endl;

        }
      sem_post(&node_phores[nodePos]);
    }


}


void Rem::updateVisualNode(int nodePos){

  if (known_nodes[nodePos].type == SENSOR){
      QString below_threshold("background-color: rgb(0,255,0)");
      QString above_threshold("background-color: red");

      QString visualID(known_nodes[nodePos].visualID.c_str());

      QToolButton *button = ui->centralWidget->findChild<QToolButton *>(visualID);

      bool detected = false;
      for(unsigned int i = 0; i < known_nodes[nodePos].channels.size(); i++){
          if(known_nodes[nodePos].channels[i].occ > occ_threshold){
              detected = true;
              /* for(unsigned int i = 0; i < known_nodes[nodePos].channels.size(); i++){
                  std::cout << " | LF" << known_nodes[nodePos].channels[i].lowFrequency << " occ: " << known_nodes[nodePos].channels[i].occ;
                }
              std::cout << "\n"; */
              break;
            }
        }

      if(detected){
          if(strcmp(known_nodes[nodePos].node_color.c_str(),"red") != 0){
              QCoreApplication::postEvent(button, new QEvent(QEvent::UpdateRequest),
                                          Qt::HighEventPriority);
              QMetaObject::invokeMethod(button, "setStyleSheet", Q_ARG(QString, above_threshold));
              known_nodes[nodePos].node_color = "red";
              //printf("Updating to red for node %d\n",nodePos);
            }

        } else{
          if(strcmp(known_nodes[nodePos].node_color.c_str(),"green") != 0){
              QCoreApplication::postEvent(button, new QEvent(QEvent::UpdateRequest),
                                          Qt::HighEventPriority);
              QMetaObject::invokeMethod(button, "setStyleSheet", Q_ARG(QString, below_threshold));
              known_nodes[nodePos].node_color = "green";
              //printf("Updating to green for node %d\n",nodePos);
            }
        }

    } else if (known_nodes[nodePos].type == SU){
      QString su_color("background-color: blue");
      std::string new_color  = "";
      //std::cout << "Current Status: " << known_nodes.at(nodePos).tx_info.state.c_str() <<"\n";
      if (strcmp(known_nodes.at(nodePos).tx_info.state.c_str(),"GRANT ACCEPTED")==0){
          QString temp_color("background-color: rgb(0,0,255)");
          su_color = temp_color;
          new_color = "blue";
        } else{
          QString temp_color("background-color: rgb(255,255,0)");
          su_color = temp_color;
          new_color = "yellow";
        }

      if(strcmp(new_color.c_str(), known_nodes[nodePos].node_color.c_str()) !=0){
          QString visualID(known_nodes[nodePos].visualID.c_str());

          QToolButton *button = ui->centralWidget->findChild<QToolButton *>(visualID);

          QCoreApplication::postEvent(button, new QEvent(QEvent::UpdateRequest),
                                      Qt::HighEventPriority);
          QMetaObject::invokeMethod(button, "setStyleSheet", Q_ARG(QString, su_color));

          known_nodes[nodePos].node_color = new_color;

        }


    }

    sem_post(&graphic_mutex);

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
  if(known_nodes.at(pos).type == SENSOR){
      SensorView s(pos, this);
      s.setModal(true);
      s.exec();
    } else if (known_nodes.at(pos).type == SU){
      SuView s(pos, this);
      s.setModal(true);
      s.exec();
    }

}

void Rem::createResultFolder(){
  if(!experimentRunning){
    std::chrono::system_clock::time_point today = std::chrono::system_clock::now();
    time_t tt = std::chrono::system_clock::to_time_t ( today );
    std::string current_time(ctime(&tt));
    for(unsigned int i = 0; i < current_time.size(); i++){
      if(current_time[i] == ' '){
        current_time[i] = '-';
      }
    }
    currentExperiment = "results/Experiment-" + current_time;
    currentExperiment.pop_back();
    std::string command("mkdir ");
    command = command + currentExperiment;
    system(command.c_str());
    // Setting up SAS Folder
    std::string folder = command + "/SAS";
    system(folder.c_str());
    std::string filename = currentExperiment + "/SAS/location.csv";
    std::fstream writer;
    writer.open (filename.c_str(), std::fstream::out | std::fstream::app | std::fstream::ate);
    writer << "time,Location Error, Sensor 1, Sensor 2, Sensor 3";
    writer.close();
    // Setting up PU attributes
    filename = currentExperiment + "/attributes.csv";
    writer.open (filename.c_str(), std::fstream::out | std::fstream::app);
    writer << "time,tx_freq,tx_freq,run_time,nodeID";
    writer.close();

    //SU folder
    folder = command + "/SU";
    system(folder.c_str());

    // Sensor folder
    folder = command + "/SENSORS";
    system(folder.c_str());
    //filename = currentExperiment + "/pu_location.csv";
    experimentRunning = true;
  }
}

void Rem::updatePUResults(std::fstream & out,std::string input, bool new_row,bool close){
  if(!new_row){
    out << "," << input;
  } else{
    out << "\n" << input;
  }
  if(close){
    //out << "\n";
    out.close();
  }

}

long int Rem::currentTime(){
  time_t time_check = time(NULL);
  return static_cast<long int>(time_check);
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
