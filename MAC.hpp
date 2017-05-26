#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <mqueue.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <arpa/inet.h>
#include <iomanip>

typedef unsigned short uint16;

#define MAX_BUF 2000 // maximum buffer tx frame
#define MTU 1500
#define CONTROL_FRAME_LEN 12
#define IP_HEADER_LEN 0 //20
#define IP_FLAG_POS 0 //13
#define PMODE 0777
#define DIFS_TIME 50
#define SLOT_TIME 1000


class MAC{
private:

public:
  MAC(char *mac_address);
  ~MAC();

  mqd_t phy_tx_queue;
  mqd_t phy_rx_queue;

  int frames_received;
  int cw_min = 16;
  int cw_max = 1024;
  int cw = cw_min;
  int back_off_time;
  bool collision_occured = false;
  char *mac_address = new char[6];
  bool stop_rx = false;
  bool stop_tx = false;

  int start_time = 0;
  char *dest_address;
  uint16 seq_num;
  int checksum;
  char *recv_payload;
  char *recv_header;
  enum State{
    IDLE = 0,
    DIFS,
    SIFS,
    DATA_WAIT,
    CTS_WAIT,
    ACK_WAIT,
    BACKING_OFF
  };

  enum Channel_state{
    BUSY = 0,
    FREE,
    UNAVAILABLE
  };

  enum ProtocolType{
    MANAGEMENT = 0x0000,
    CONTROL = 0x0001,
    DATA = 0x0002
  };

  enum ProtocolSubtype{
    ASSOC_REQ = 0x0000,
    BEACON = 0x0008,
    RTS = 0x000A,
    CTS = 0x000C
  };


  struct FrameControl{
    ProtocolType frame_protocol_type;
    ProtocolSubtype frame_protocol_subtype;
  };


  State myState;
  Channel_state tx_channel_state;
  FrameControl packet_FrameControl;


  pthread_t tx_process;            // thread for transmission
  pthread_mutex_t tx_mutex;
  pthread_t rx_process;            // thread for transmission
  pthread_mutex_t rx_mutex;


  char *getControlFrame(FrameControl temp);
  void create_frame(char *&data, int data_len,ProtocolType newType,
                  ProtocolSubtype newSubType);
  char *getMACHeader(char *frame);
  char *getPayLoad(char *frame, int payload_len);
  char *extractSourceMAC(char *header);

  void analyzeReceivedFrame(char *frame, int frame_len);
  bool isLastsegment(char *segment);
  bool isLastAlienFrame(char *frame);
  void backOff();

};

void *MAC_tx_worker(void *_arg);
void *MAC_rx_worker(void *_arg);
char *random_byte_generator();
int buffToInteger(char * buffer);
bool isBitSet (unsigned char c, int n);