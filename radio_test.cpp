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
#include "ofdm_phy.hpp"
#include "radio.hpp"
#include "config_reader.hpp"

#define DEBUG 0
#if DEBUG == 1
#define dprintf(...) printf(__VA_ARGS__)
#else
#define dprintf(...) /*__VA_ARGS__*/
#endif

int sig_terminate;
bool start_msg_received = false;

void initialize_node_parameters(struct node_parameters *np);
void Initialize_PHY(struct node_parameters *np, void *PHY_p,
                    int argc, char **argv);
char *highPassAlloc(struct node_parameters *np, int type = 0);
char *lowPassAlloc(struct node_parameters *np);
void terminate(int signum)
{
  printf("\nTerminating all Threads\n");
  sig_terminate = 1;
}

int main(int argc, char **argv)
{

  // register signal handlers
  signal(SIGINT, terminate);
  signal(SIGQUIT, terminate);
  signal(SIGTERM, terminate);

  // pointer to PHY which may or may not be used
  PhyLayer *PHY = NULL;
  // Create node parameters struct and the scenario parameters struct
  // and read info from controller
  struct node_parameters np;
  memset(&np, 0, sizeof(np));
  initialize_node_parameters(&np);
  //struct scenario_parameters sp;
  PHY = new PhyLayer;
  //PHY->custom_uhd_set=true;
  //PHY->set_uhd(PHY->usrp_tx,PHY->usrp_rx,PHY->dev_addr,(char*)"A:0",(char*)"A:0");
  PHY->set_attributes();

  // set the USRP's timer to 0
  uhd::time_spec_t t0(0, 0, 1e6);

  PHY->usrp_rx->set_time_now(t0, 0);

  ConfigReader myConfig("network.cfg");
  if(myConfig.get_status() != -1){
    np.rx_freq = myConfig.rx_freq;
    np.rx_rate = myConfig.rx_rate;
    PHY->setRxChannels(np.rx_rate,myConfig.channels);
    printf("RX FREQ: %f\n",np.rx_freq);
    np.tx_freq = myConfig.tx_freq;
    printf("TX FREQ: %f\n",np.tx_freq);
    np.tx_rate = myConfig.tx_rate;
  }
  Initialize_PHY(&np, (void *)PHY, argc, argv);

  // initialize sig_terminate flag and check return from socket call
  sig_terminate = 0;

  // Start PHY
  dprintf("Starting PHY object...\n");
  PHY->start_rx();
  PHY->start_tx();

  // send burst of packets
  while (sig_terminate == 0)
  {
  }

  printf("Preparing to terminate transmission\n");

  delete PHY;
  printf("Done!\n");
}

void initialize_node_parameters(struct node_parameters *np)
{
  // initial USRP
 // strncpy(np->my_ip, "10.0.0.2", sizeof(np->my_ip));
 // strncpy(np->target_ip, "10.0.0.3", sizeof(np->target_ip));
  // initial USRP settings
  np->rx_freq = 458e6;
  np->rx_rate = 4e6;
  np->rx_gain = 20.0;

  np->tx_freq = 462.5e6;
  np->tx_rate = 4e6;
  np->tx_gain = 80.0;

  // initial liquid OFDM settings
  np->tx_gain_soft = -4;
  np->tx_modulation = LIQUID_MODEM_QAM32;
  np->tx_crc = LIQUID_CRC_32;
  np->tx_fec0 = LIQUID_FEC_RS_M8;
  np->tx_fec1 = LIQUID_FEC_RS_M8;
  np->tx_cp_len = 32;
  np->rx_cp_len = 32;
  np->tx_taper_len = 4;

  int subcarriers = 512;
  np->tx_subcarriers = subcarriers;
  np->tx_guard_subcarriers = 4;
  np->tx_central_nulls = 6;
  np->tx_pilot_freq = 4;

  np->rx_subcarriers = subcarriers;
  np->rx_guard_subcarriers = 4;
  np->rx_central_nulls = 6;
  np->rx_pilot_freq = 4;
}

void Initialize_PHY(struct node_parameters *np, void *PHY_p,
                    int argc, char **argv)
{


  char *alloc = highPassAlloc(np);
  PhyLayer *PHY = (PhyLayer *)PHY_p;
  //  PHY->set_ip(np->my_ip);
  PHY->set_rx_freq(np->rx_freq);
  PHY->set_rx_rate(np->rx_rate);
  PHY->set_rx_gain_uhd(np->rx_gain);
  PHY->set_rx_subcarriers(np->rx_subcarriers);
  
  PHY->set_rx_cp_len(np->rx_cp_len);
  PHY->set_rx_taper_len(np->rx_taper_len);
  PHY->set_tx_freq(np->tx_freq);
  PHY->set_tx_rate(np->tx_rate);
  PHY->set_tx_gain_soft(np->tx_gain_soft);
  PHY->set_tx_gain_uhd(np->tx_gain);
  PHY->set_tx_subcarriers(np->tx_subcarriers);

  PHY->set_tx_cp_len(np->tx_cp_len);
  PHY->set_tx_taper_len(np->tx_taper_len);
  PHY->set_tx_modulation(np->tx_modulation);
  PHY->set_tx_crc(np->tx_crc);
  PHY->set_tx_fec0(np->tx_fec0);
  PHY->set_tx_fec1(np->tx_fec1);


  if(np->tx_freq >= np->rx_freq ){
    PHY->set_tx_subcarrier_alloc(highPassAlloc(np));
    PHY->set_rx_subcarrier_alloc(highPassAlloc(np,1));
  } else{
    PHY->set_tx_subcarrier_alloc(highPassAlloc(np,1));
    PHY->set_rx_subcarrier_alloc(highPassAlloc(np));    
  }

  //PHY->set_rx_subcarrier_alloc(alloc);
  //PHY->set_tx_subcarrier_alloc(alloc);
}

// type == 0 -> high pass
// otherwise low pass
char *highPassAlloc(struct node_parameters *np, int type){
  char *alloc = new char[np->tx_subcarriers];
  memset(alloc,OFDMFRAME_SCTYPE_NULL,np->tx_subcarriers);

  
  int count = np->tx_subcarriers/2;
  int less_nulls = 16;
  int more_nulls = 16;
  int q = more_nulls;
  int r = less_nulls; 
  
  if(type != 0){
    r = more_nulls;
    q = less_nulls;
  }
  // LOWER BAND
  int s = 0;
  for(int i = 1; i <= (np->tx_subcarriers - np->tx_subcarriers/q)/2;i++){
      alloc[np->tx_subcarriers-s-i] = OFDMFRAME_SCTYPE_DATA;
  }
  // HIGHER BAND
  for(int i = 0; i < (np->tx_subcarriers - np->tx_subcarriers/r)/2;i++){
      alloc[s+i] = OFDMFRAME_SCTYPE_DATA;
  }

  // insert pilots
  int no_p = np->tx_subcarriers/4;
  int p = np->tx_subcarriers/(2*no_p);
  for(int i = 0; i < no_p/2; i++){
    alloc[i*p] = OFDMFRAME_SCTYPE_PILOT;
    alloc[np->tx_subcarriers-1-i*p] = OFDMFRAME_SCTYPE_PILOT;
  }  
  
  // insert DC NULLS
  count = np->tx_subcarriers/32;
  for(int i = 0; i < count; i++){
    alloc[i] = OFDMFRAME_SCTYPE_NULL;
    alloc[np->tx_subcarriers-1-i] = OFDMFRAME_SCTYPE_NULL;
  }

  return alloc;
}
// NOTE: Issues with the usrp creating out of band interference