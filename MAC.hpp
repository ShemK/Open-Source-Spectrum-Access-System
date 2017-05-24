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

typedef unsigned short uint16;

#define MAX_BUF 1500
#define CONTROL_FRAME_LEN 8

#define PMODE 0777


class MAC{
private:

public:
  MAC(char *mac_address);
  ~MAC();

  mqd_t phy_tx_queue;
  mqd_t phy_rx_queue;

  char *mac_address = new char[6];
  bool stop_rx = false;
  bool stop_tx = false;

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
};

void *MAC_tx_worker(void *_arg);
void *MAC_rx_worker(void *_arg);
char *random_byte_generator();
