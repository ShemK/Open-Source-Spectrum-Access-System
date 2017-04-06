#include <stdlib.h>
#include <stdio.h>
#include <net/if.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <linux/if_tun.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>
#include <string>
#include <uhd/utils/msg.hpp>
#include <uhd/types/time_spec.hpp>
#include <iostream>
#include <fstream>
#include <errno.h>
#include <signal.h>
#include <random>
#include "timer.h"
#include "crts.hpp"
#include "extensible_cognitive_radio.hpp"
#include "tun.hpp"

#define DEBUG 0
#if DEBUG == 1
#define dprintf(...) printf(__VA_ARGS__)
#else
#define dprintf(...) /*__VA_ARGS__*/
#endif

int sig_terminate;
bool start_msg_received = false;
time_t start_time_s = 0;
time_t stop_time_s = 0;
std::ofstream log_rx_fstream;
std::ofstream log_tx_fstream;
float rx_stats_fb_period = 1e4;
timer rx_stat_fb_timer;
long int bytes_sent;
long int bytes_received;
void initialize_node_parameters(struct node_parameters *np);

void initialize_node_parameters(struct node_parameters *np) {
  // general node parameters
  np->node_type = "cognitive radio";
  np->cognitive_radio_type = 1;
  np->server_ip = "192.168.1.11";

  // network parameters
  np->crts_ip = "10.0.0.2";
  np->target_ip = "10.0.0.3";
  np->net_traffic_type = NET_TRAFFIC_STREAM;
  np->net_mean_throughput = 2e6;

  // cognitive engine parameters
  np->cognitive_engine = "CE_Template";
  np->ce_timeout_ms = 200.0;

  // log/report settings
  np->print_rx_frame_metrics = 1;
  np->log_phy_rx = 1;
  np->log_phy_tx = 1;
  np->log_net_rx = 1;
  np->log_net_tx = 1;
  np->generate_octave_logs = 1;

  // initial USRP settings
  np->rx_freq = 862.5e6;
  np->rx_rate = 2e6;
  np->rx_gain = 15.0;
  np->tx_freq = 857.5e6;
  np->tx_rate = 2e6;
  np->tx_gain = 15.0;

  // initial liquid OFDM settings
  np->tx_gain_soft = -12;
  np->tx_modulation = 25;
  np->tx_crc = 6;
  np->tx_fec0 = 11;
  np->tx_fec1 = 1;
  np->tx_cp_len = 16;
  np->rx_cp_len = 16;
  np->tx_taper_len = 4;


  np->tx_subcarriers = 32;
  np->tx_subcarrier_alloc_method = 2;
  np->tx_guard_subcarriers = 4;
  np->tx_central_nulls = 6;
  np->tx_pilot_freq = 4;

  np->rx_subcarriers = 32;
  np->rx_subcarrier_alloc_method = 2;
  np->rx_guard_subcarriers = 4;
  np->rx_central_nulls = 6;
  np->rx_pilot_freq = 4;
}

void Initialize_CR(struct node_parameters *np, void *ECR_p,
                   int argc, char **argv) {

  // initialize ECR parameters if applicable
  if (np->cognitive_radio_type == EXTENSIBLE_COGNITIVE_RADIO) {
    ExtensibleCognitiveRadio *ECR = (ExtensibleCognitiveRadio *)ECR_p;

    // append relative locations for log files
    char phy_rx_log_file_name[255];
    strcpy(phy_rx_log_file_name, "./logs/bin/");
    strcat(phy_rx_log_file_name, np->phy_rx_log_file);
    strcat(phy_rx_log_file_name, ".log");

    char phy_tx_log_file_name[255];
    strcpy(phy_tx_log_file_name, "./logs/bin/");
    strcat(phy_tx_log_file_name, np->phy_tx_log_file);
    strcat(phy_tx_log_file_name, ".log");

    // set cognitive radio parameters
    ECR->set_ip(np->crts_ip); /*sets ip address of the virtual interface*/
  //  ECR->print_metrics_flag = np->print_rx_frame_metrics;
    ECR->log_phy_rx_flag = np->log_phy_rx;
    ECR->log_phy_tx_flag = np->log_phy_tx;
    ECR->set_ce_timeout_ms(np->ce_timeout_ms);
    strcpy(ECR->phy_rx_log_file, phy_rx_log_file_name);
    strcpy(ECR->phy_tx_log_file, phy_tx_log_file_name);
    ECR->set_rx_freq(np->rx_freq);
    ECR->set_rx_rate(np->rx_rate);
    ECR->set_rx_gain_uhd(np->rx_gain);
    ECR->set_rx_subcarriers(np->rx_subcarriers);
    ECR->set_rx_cp_len(np->rx_cp_len);
    ECR->set_rx_taper_len(np->rx_taper_len);
    ECR->set_tx_freq(np->tx_freq);
    ECR->set_tx_rate(np->tx_rate);
    ECR->set_tx_gain_soft(np->tx_gain_soft);
    ECR->set_tx_gain_uhd(np->tx_gain);
    ECR->set_tx_subcarriers(np->tx_subcarriers);
    ECR->set_tx_cp_len(np->tx_cp_len);
    ECR->set_tx_taper_len(np->tx_taper_len);
    ECR->set_tx_modulation(np->tx_modulation);
    ECR->set_tx_crc(np->tx_crc);
    ECR->set_tx_fec0(np->tx_fec0);
    ECR->set_tx_fec1(np->tx_fec1);
    ECR->set_rx_stat_tracking(false, 0.0);
    ECR->set_ce(np->cognitive_engine, argc, argv);
    ECR->reset_log_files();

    // copy subcarrier allocations if other than liquid-dsp default
    if (np->tx_subcarrier_alloc_method == CUSTOM_SUBCARRIER_ALLOC ||
        np->tx_subcarrier_alloc_method == STANDARD_SUBCARRIER_ALLOC) {
      ECR->set_tx_subcarrier_alloc(np->tx_subcarrier_alloc);
    } else {
      ECR->set_tx_subcarrier_alloc(NULL);
    }
    if (np->rx_subcarrier_alloc_method == CUSTOM_SUBCARRIER_ALLOC ||
        np->rx_subcarrier_alloc_method == STANDARD_SUBCARRIER_ALLOC) {
      ECR->set_rx_subcarrier_alloc(np->rx_subcarrier_alloc);
    } else {
      ECR->set_rx_subcarrier_alloc(NULL);
    }
  }

}



void help_CRTS_CR() {
  printf("CRTS_CR -- Start a cognitive radio node. Only needs to be run "
         "explicitly when using CRTS_controller with -m option.\n");
  printf("        -- This program must be run from the main CRTS directory.\n");
  printf(" -h : Help.\n");
  printf(" -a : IP Address of node running CRTS_controller.\n");
}

int main(int argc, char **argv) {
 // std::ifstream read("Hello.txt");
  std::ifstream readT("Message.txt");
  std::ofstream writeR("Receive.txt");
  // register signal handlers
  signal(SIGINT, terminate);
  signal(SIGQUIT, terminate);
  signal(SIGTERM, terminate);

  // Port to be used by CRTS server and client
  int port = CRTS_CR_PORT;

  // pointer to ECR which may or may not be used
  ExtensibleCognitiveRadio *ECR = NULL;

  // Create node parameters struct and the scenario parameters struct
  // and read info from controller
  struct node_parameters np;
  memset(&np, 0, sizeof(np));
  struct scenario_parameters sp;

    // Create and start the ECR or python CR so that they are in a ready
    // state when the experiment begins
    if(np.cognitive_radio_type == EXTENSIBLE_COGNITIVE_RADIO)
    {
        dprintf("Creating ECR object...\n");
        ECR = new ExtensibleCognitiveRadio;

        // set the USRP's timer to 0
        uhd::time_spec_t t0(0, 0, 1e6);
        ECR->usrp_rx->set_time_now(t0, 0);


        rx_stat_fb_timer = timer_create();

        int argc = 0;
        char ** argv = NULL;
        dprintf("Converting ce_args to argc argv format\n");
        str2argcargv(np.ce_args, np.cognitive_engine, argc, argv);
        dprintf("Initializing CR\n");
        initialize_node_parameters(&np);
        Initialize_CR(&np, (void *)ECR, argc, argv);
        freeargcargv(argc, argv);
    }


  // Define address structure for CRTS socket server used to receive network
  // traffic
  struct sockaddr_in crts_server_addr;
  memset(&crts_server_addr, 0, sizeof(crts_server_addr));
  crts_server_addr.sin_family = AF_INET;
  // Only receive packets addressed to the crts_ip
  crts_server_addr.sin_addr.s_addr = inet_addr(np.crts_ip);
  crts_server_addr.sin_port = htons(port);
  socklen_t clientlen = sizeof(crts_server_addr);
  int crts_server_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  printf("Creating Sockets\n");
  system("route -n");
  // Define address structure for CRTS socket client used to send network
  // traffic
  struct sockaddr_in crts_client_addr;
  memset(&crts_client_addr, 0, sizeof(crts_client_addr));
  crts_client_addr.sin_family = AF_INET;
  crts_client_addr.sin_addr.s_addr = inet_addr(np.target_ip);
  crts_client_addr.sin_port = htons(port);
  int crts_client_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  // Bind CRTS server socket
  bind(crts_server_sock, (sockaddr *)&crts_server_addr, clientlen);

  // Define a buffer for receiving and a temporary message for sending
  int recv_buffer_len = 8192 * 2;
  char recv_buffer[recv_buffer_len];

  // Define parameters and message for sending
  int packet_counter = 0;
  unsigned char packet_num_prs[CRTS_CR_PACKET_NUM_LEN]; // pseudo-random sequence used
                                                  // to modify packet number
  unsigned char message[CRTS_CR_PACKET_LEN];
  srand(12);
  msequence ms = msequence_create_default(CRTS_CR_PACKET_SR_LEN);

  // define bit mask applied to packet number
  for (int i = 0; i < CRTS_CR_PACKET_NUM_LEN; i++)
    packet_num_prs[i] = msequence_generate_symbol(ms,8); //rand() & 0xff;

  char c;
  // create a defined payload generated pseudo-randomly
  for (int i = CRTS_CR_PACKET_NUM_LEN; i < CRTS_CR_PACKET_LEN; i++){
    readT.get(c);
   // message[i] = msequence_generate_symbol(ms,8);//(rand() & 0xff);
    message[i] = c;
  }

  // initialize sig_terminate flag and check return from socket call
  sig_terminate = 0;
  if (crts_client_sock < 0) {
    printf("CRTS failed to create client socket\n");
    sig_terminate = 1;
  }
  if (crts_server_sock < 0) {
    printf("CRTS failed to create server socket\n");
    sig_terminate = 1;
  }

  t_step = 8.0 * (float)CRTS_CR_PACKET_LEN / np.net_mean_throughput;
  float send_time_delta = 0;
  struct timeval send_time;
  fd_set read_fds;
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 1;

  // Poisson RV generator
  std::default_random_engine rand_generator;
  std::poisson_distribution<int> poisson_generator(1e6);
  int poisson_rv;

  bytes_sent = 0;
  bytes_received = 0;

  // Wait for the start-time before beginning the scenario
  struct timeval tv;
  time_t time_s;
  while (1) {
    receive_command_from_controller(&tcp_controller, &sp, &np, ECR, &fb_enables, &t_step);
    gettimeofday(&tv, NULL);
    time_s = tv.tv_sec;
    if ((time_s >= (time_t) start_time_s) && start_msg_received)
      break;
    if (sig_terminate)
      break;
    usleep(5e2);
  }

  if (np.cognitive_radio_type == EXTENSIBLE_COGNITIVE_RADIO) {
    // Start ECR
    dprintf("Starting ECR object...\n");
    ECR->start_rx();
    ECR->start_tx();
    ECR->start_ce();
  }

  bool send_flag = true;

  // main loop: receives control, sends feedback, and generates/receives network traffic
  while ((time_s < stop_time_s) && (!sig_terminate)) {

    if (send_flag) {
      // send burst of packets
      for (int i = 0; i < np.net_burst_length; i++) {

        // update packet number
        packet_counter++;
        for (int i = 0; i < CRTS_CR_PACKET_NUM_LEN; i++)
          message[i] =
              ((packet_counter >> (8 * (CRTS_CR_PACKET_NUM_LEN - i - 1))) & 0xff) ^
              packet_num_prs[i];


        // send UDP packet via CR
        dprintf("CRTS sending packet %i\n", packet_counter);
        int send_return = 0;
        sendto(crts_client_sock, (char *)message, sizeof(message), 0,
                (struct sockaddr *)&crts_client_addr, sizeof(crts_client_addr));
        if (send_return < 0)
          printf("Failed to send message\n");
        else
          bytes_sent += send_return;

        ECR->inc_tx_queued_bytes(send_return+32);

      }
    }

    // read all available data from the UDP socket
    int recv_len = 0;
    FD_ZERO(&read_fds);
    FD_SET(crts_server_sock, &read_fds);
    while (select(crts_server_sock + 1, &read_fds, NULL, NULL, &timeout) > 0) {
      recv_len = recvfrom(crts_server_sock, recv_buffer, recv_buffer_len, 0,
                          (struct sockaddr *)&crts_server_addr, &clientlen);

      // determine packet number
      int rx_packet_num = 0;
      for (int i = 0; i < CRTS_CR_PACKET_NUM_LEN; i++)
        rx_packet_num +=
            (((unsigned char)recv_buffer[i]) ^ packet_num_prs[i])
            << 8 * (CRTS_CR_PACKET_NUM_LEN - i - 1);

      // print out/log details of received messages
      if (recv_len > 0) {
        // TODO: Say what address message was received from.
        // (It's in CRTS_server_addr)
        dprintf("CRTS received packet %i containing %i bytes:\n", rx_packet_num,
                recv_len);
        bytes_received += recv_len;

      	for(int i = 0; i < recv_len; i++) {
      	  writeR << recv_buffer[i];
      //    std::cout << recv_buffer[i];
      	}

      }

      FD_ZERO(&read_fds);
      FD_SET(crts_server_sock, &read_fds);
    }

  }

  // close all network connections
  close(crts_client_sock);
  close(crts_server_sock);


  // clean up ECR/python process
  if (np.cognitive_radio_type == EXTENSIBLE_COGNITIVE_RADIO) {
    delete ECR;
  }
  printf(
      "CRTS: Reached termination. Sending termination message to controller\n");

  writeR.close();
}
