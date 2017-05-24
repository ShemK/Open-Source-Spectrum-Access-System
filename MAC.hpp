#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <mqueue.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

typedef unsigned short uint16;

#define MAX_BUF 1500

#define PMODE 0777


class MAC{
private:
  char *mac_address = new char[20];
public:
  MAC(char *mac_address);
  ~MAC();

  mqd_t phy_tx_queue;
  mqd_t phy_rx_queue;


  bool stop_rx = false;
  bool stop_tx = false;

  char *dest_address;
  uint16 seq_num;
  int checksum;

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

  enum Protocol_version{
    MANAGEMENT = 0x0000,
    CONTROL = 0x0001,
    DATA = 0x0010
  };

  enum Protocol_subtype{
    BEACON = 0x0001,
    RTS = 0x000A,
    CTS = 0x000C
  };


  struct Frame_control{
    Protocol_version frame_protocol;
    Protocol_subtype frame_subtype;
  };


  State myState;
  Channel_state tx_channel_state;
  Frame_control packet_frame_control;


  pthread_t tx_process;            // thread for transmission
  pthread_mutex_t tx_mutex;
  pthread_t rx_process;            // thread for transmission
  pthread_mutex_t rx_mutex;
};

void *MAC_tx_worker(void *_arg);
void *MAC_rx_worker(void *_arg);
