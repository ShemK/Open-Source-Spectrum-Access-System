#ifndef REM_H
#define REM_H

#include <QMainWindow>
#include <QString>
#include <string.h>
#include <vector>
#include <pmt/pmt.h>
#include <iostream>
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
#include "nodeview.h"
#include "sensorview.h"
#include "suview.h"


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
    std::string latitude;
    std::string longitude;
    std::vector<channelInfo> channels;
    std::string visualID;
    unsigned int current_channel = 0;
    nodeState state;
    double current_occ;
    double previous_occ;
    nodeType type;
    transmissionInfo tx_info;
    std::string node_color = "";
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

  double occ_threshold = 0.01;

  int node_num = 12;

  void initializeNodeInfo();

  void analyzeInfo(const char *recv_buffer, int recv_len);
  void organizeData(int nodeID, double occ, double lowFreq, double bandwidth);
  int getChannelPos(nodeInfo n,double lowFreq);

  std::vector<nodeInfo> known_nodes;
  int getNodePos(int nodeID);

  void showMoreInfo(std::string visualID);

  int getNodePos(std::string visualID);

  bool channelSort(Rem::channelInfo x, Rem::channelInfo y);


  sem_t *node_phores;

  void updateGroupee(short unsigned int nodeTemp, int status);

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
