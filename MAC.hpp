#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <mqueue.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <arpa/inet.h>
#include <iomanip>
#include <zlib.h>
#include "tun.hpp"
#include <sys/ioctl.h>
#include <linux/if_tun.h>
#include <netinet/if_ether.h>
#include <queue>
#include <map>
#include <future>
#include <functional>
#include <cstring>
#include <bitset>

#define RED "\x1B[31m"
#define GRN "\x1B[32m"
#define YEL "\x1B[33m"
#define BLU "\x1B[34m"
#define MAG "\x1B[35m"
#define CYN "\x1B[36m"
#define WHT "\x1B[37m"
#define RESET "\x1B[0m"

typedef unsigned short uint16;

#define MAX_BUF 2000 // maximum buffer tx frame
#define MTU 1500
#define CONTROL_FRAME_LEN 7
#define ETH_HEADER_LEN 18 //14 + 4 for ethernet
#define IP_HEADER_LEN 20
#define IP_FLAG_POS 13
#define PMODE 0777
#define LAST_FRAME_POS 1
#define FRAME_NUM_POS 3
#define ARQ_POS 2
#define DIFS_TIME 50
#define SLOT_TIME 1000

#define TCP_PACKET 0x06
#define UDP_PACKET 0x11
#define TAP_EXTRA_LOAD 4

class MAC
{
private:
public:
  MAC();
  ~MAC();

  mqd_t phy_tx_queue;
  mqd_t phy_rx_queue;

  int frames_received;
  int cw_min = 16;
  int cw_max = 256; //1024;
  int cw = 30;
  int back_off_time;
  bool collision_occured = false;
  char mac_address[6];
  bool stop_rx = false;
  bool stop_tx = false;

  int tunfd;
  char tun_name[20];
  char systemCMD[200];

  char broadcast_address[6];

  int start_time = 0;
  char *dest_address;
  uint16 seq_num;
  int checksum;
  char *recv_payload;
  char *recv_header;
  enum State
  {
    IDLE = 0,
    DIFS,
    SIFS,
    DATA_WAIT,
    CTS_WAIT,
    ACK_WAIT,
    BACKING_OFF,
    TRANSMITTING
  };

  enum Channel_state
  {
    BUSY = 0,
    FREE,
    UNAVAILABLE
  };

  enum ProtocolType
  {
    MANAGEMENT = 0x0000,
    CONTROL = 0x0001,
    DATA = 0x0002
  };

  enum ProtocolSubtype
  {
    ASSOC_REQ = 0x0000,
    BEACON = 0x0008,
    RTS = 0x000A,
    CTS = 0x000C,
    ACK = 0x000E,
    UNKNOWN = 0x000F
  };

  struct FrameControl
  {
    ProtocolType frame_protocol_type;
    ProtocolSubtype frame_protocol_subtype;
  };

  State myState;
  Channel_state tx_channel_state;
  FrameControl packet_FrameControl;

  pthread_t tx_process; // thread for transmission
  pthread_mutex_t tx_mutex;
  pthread_t rx_process; // thread for transmission
  pthread_mutex_t rx_mutex;

  void transmit_frame(char *segment, int segment_len, int ip_type, int &frame_num);

  char *getControlFrame(FrameControl temp);
  void create_frame(char *&data, int data_len, ProtocolType newType,
                    ProtocolSubtype newSubType);
  char *getMACHeader(char *frame);
  char *getPayLoad(char *frame, int payload_len);
  char *extractSourceMAC(char *header);
  char *extractDestinationMAC(char *recv_payload);
  void analyzeReceivedFrame(char *frame, int frame_len);
  bool isLastsegment(char *segment);
  bool isLastAlienFrame(char *frame);
  void backOff();
  void addCRC(char *frame, int &frame_len);
  bool isCorrectCRC(char *buf, int buf_len);
  void sendToIPLayer(char *payload, int payload_len);
  void set_ip(const char *ip);

  bool new_transmission = true;
  int frames_sent = 0;

  char mdns[6]; // to store mdns mac_address
  void set_banned_addresses();
  bool isAddressBanned(const char *add_check);
  char getProtocol(char *frame);
  struct IpSegment
  {
    int size;
    char segment[MAX_BUF];
    bool arp_packet = false;
    bool routing_packet = false;
    int frame_num = 0;
  };
  std::queue<IpSegment> ip_tx_queue;
  int tx_continuation = 0;

  struct Peer
  {
    char mac_address[6];
    int frames_sent;
    int frames_received;
    int frame_errors;
    double bit_error_rate;
  };


  std::vector<Peer> peerlist;
  int peer_number = 0;
  int getPeerPosition(char peer_address[6]);
  int updatePeerTxStatistics(char peer_address[6]);
  void updatePeerRxStatistics(char peer_address[6], int frame_num_received, int frame_len);
  //Peer new_peer;


  FrameControl extractFrameControl(char *header);
  void sendACK(char *recv_payload, int frame_num);
  int frames_before_last_frame = 0;
  bool last_frame_sent = false;
  bool ack_received = false;
  bool ack_reception_failed = false;
  int burst_packets = 0;
  int retransmissions = 0;
  char prev_packet[MAX_BUF];
  int prev_frame_num  = 0;

  void set_frames_sent(char * frame,int tx_continuation);
  void convert_bits_int(char * frame);
  char *extractSourceIP(char *payload, int pos);

  struct timespec last_tx;
};

void *MAC_tx_worker(void *_arg);
void *MAC_rx_worker(void *_arg);
char *random_byte_generator();
int buffToInteger(char *buffer);
bool isBitSet(unsigned char c, int n);
std::string exec(const char *cmd);
unsigned char hex_digit(char ch);
char* macAddr_toString(unsigned char* addr);
char* ipAddr_toString(unsigned char* addr);
