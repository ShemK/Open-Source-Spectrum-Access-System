#ifndef REM_H
#define REM_H

#include <QMainWindow>
#include <QString>
#include <string.h>
#include <vector>
#include <pmt/pmt.h>
#include <iostream>
#include <iomanip>
#include <math.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/socket.h>
#include <unistd.h>
#include <libconfig.h>
#include <stdlib.h>
#include <algorithm>
#include <list>
#include <queue>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <chrono>
#include <ctime>
#include <fstream>
#include "nodeview.h"
#include "sensorview.h"
#include "suview.h"
#include <algorithm>

namespace Ui {
  class Rem;
}

void *listener(void *_arg);



class Rem : public QMainWindow
{
  Q_OBJECT
public:
  explicit Rem(QWidget *parent = 0);
  ~Rem();
  struct channelInfo{
    double lowFrequency;
    double highFrequency;
    double bandwidth;
    double occ;
    bool log = false;
    std::list <double> occ_history;
  };

  struct performanceStats{
    int currentTime;
    double per;
    double bitrate;
  };
  struct transmissionInfo{
    std::string state = "";
    double tx_freq = 0;
    double rx_freq = 0;
    std::queue <performanceStats> stats;
    std::vector<short unsigned int> group;
    bool new_change = false;
  };

  enum nodeState{
    ACTIVE = 0,
    INACTIVE
  };

  enum nodeType{
    SENSOR = 0,
    SAS,
    SU,
    PU
  };

  struct nodeInfo{
    int nodeID;
    double latitude;
    double longitude;
    std::vector<channelInfo> channels;
    std::string visualID;
    unsigned int current_channel = 0;
    nodeState state;
    double current_occ;
    double previous_occ;
    nodeType type;
    transmissionInfo tx_info;
    std::string node_color = "";
    std::vector<double> log_channels;
  };

  struct sensor{
    unsigned int current_channel = 0;
    nodeState state;
    double current_occ;
    double previous_occ;
  };

  pthread_t listener_process;
  pthread_mutex_t listener_mutex;
  friend void *listener(void *);
  bool app_open = true;

  double occ_threshold = 0.1;

  int node_num = 12;

  void initializeNodeInfo();

  void analyzeInfo(const char *recv_buffer, int recv_len);
  void organizeData(int nodeID, double occ, double lowFreq, double bandwidth);
  int getChannelPos(nodeInfo n,double lowFreq);

  void analyzePuInfo(pmt::pmt_t & received_dict, pmt::pmt_t nodeID);

  void analyzeSASInfo(pmt::pmt_t & received_dict);

  double searchNearestPU(double latitude,double longitude);
  long int currentTime();
  std::vector<nodeInfo> known_nodes;
  int getNodePos(int nodeID);

  void showMoreInfo(std::string visualID);

  int getNodePos(std::string visualID);

  bool channelSort(Rem::channelInfo x, Rem::channelInfo y);


  sem_t *node_phores;

  void updateGroupee(short unsigned int nodeTemp, int status);

  sem_t graphic_mutex;

  void createResultFolder();

  bool experimentRunning;

  std::string currentExperiment;

  int numPUs = 0;

  void updatePUResults(std::fstream & out,std::string input, bool new_row = true,bool close = true);

  //bool suAttributeChange(int att,pos);

  enum su_att{
    FREQ_CHANGE,
    MOD_CHANGE,
    RATE_CHANGE
  };


  public slots:
  void updateVisualNode(int nodePos);

  private slots:

  void on_Node12_clicked();
  void on_Node6_clicked();

  void on_Node11_clicked();

  void on_Node10_clicked();

  void on_Node8_clicked();

  void on_Node5_clicked();

  void on_Node3_clicked();

  void on_Node2_clicked();

  void on_Node1_clicked();

  void on_Node4_clicked();

  void on_Node7_clicked();

  void on_Node9_clicked();


private:
  Ui::Rem *ui;
};

#endif // REM_H
