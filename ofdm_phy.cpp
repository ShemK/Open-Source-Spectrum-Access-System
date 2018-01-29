/*
//  This is an adaptation of the CRTS Code from https://github.com/ericps1/crts/
//
//
//
//
*/
#include <stdio.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <linux/if_tun.h>
#include <math.h>
#include <complex>
#include <liquid/liquid.h>
#include <pthread.h>
#include <iostream>
#include <fstream>
#include <errno.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sched.h>
#include <bitset>
#include <uhd/utils/msg.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/types/tune_request.hpp>
#include "ofdm_phy.hpp"
#include "timer.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctime>
#include <fftw3.h>

#define DEBUG 1
#if DEBUG == 1
#define dprintf(...) printf(__VA_ARGS__)
#else
#define dprintf(...) /*__VA_ARGS__*/
#endif

void uhd_msg_handler(uhd::msg::type_t type, const std::string &msg);
int PhyLayer::uhd_msg;

// Constructor
PhyLayer::PhyLayer()
{
  test_loop = new Loop();
  // set internal properties
  tx_params.numSubcarriers = 32;
  tx_params.cp_len = 16;
  tx_params.taper_len = 4;
  tx_params.subcarrierAlloc = (unsigned char *)malloc(32 * sizeof(char));
  tx_params.payload_sym_length = 288 * 8; // 256 * 8;
  tx_params.tx_freq = 460.0e6f;
  tx_params.tx_rate = 1e6;
  tx_params.tx_gain_soft = 0.0f;
  tx_params.tx_gain_uhd = 0.0f;

  rx_params.numSubcarriers = 32;
  rx_params.cp_len = 16;
  rx_params.taper_len = 4;
  rx_params.subcarrierAlloc = (unsigned char *)malloc(32 * sizeof(char));
  rx_params.rx_freq = 460.0e6f;
  rx_params.rx_rate = 500e3;
  rx_params.rx_gain_uhd = 0.0f;

  // use liquid default subcarrier allocation
  ofdmframe_init_default_sctype(32, tx_params.subcarrierAlloc);
  ofdmframe_init_default_sctype(32, rx_params.subcarrierAlloc);

  // determine how many subcarriers carry data
  numDataSubcarriers = 0;
  for (unsigned int i = 0; i < tx_params.numSubcarriers; i++)
  {
    if (tx_params.subcarrierAlloc[i] == OFDMFRAME_SCTYPE_DATA)
      numDataSubcarriers++;
  }
  update_tx_data_rate = true;

  // Initialize header to all zeros
  memset(tx_header, 0, sizeof(tx_header));

  // initialize frame generator setting
  ofdmflexframegenprops_init_default(&tx_params.fgprops);
  tx_params.fgprops.check = LIQUID_CRC_32;
  tx_params.fgprops.fec0 = LIQUID_FEC_HAMMING128;
  tx_params.fgprops.fec1 = LIQUID_FEC_NONE;
  tx_params.fgprops.mod_scheme = LIQUID_MODEM_QAM4;

  // copy tx_params to tx_params_updated and initialize update flags to 0
  tx_params_updated = tx_params;
  update_tx_flag = 0;
  update_usrp_tx = 0;
  recreate_fg = 0;

  // create frame generator
  fg = ofdmflexframegen_create(tx_params.numSubcarriers, tx_params.cp_len,
                               tx_params.taper_len, tx_params.subcarrierAlloc,
                               &tx_params.fgprops);

  // allocate memory for frame generator output (single OFDM symbol)
  fgbuffer_len = tx_params.numSubcarriers + tx_params.cp_len;
  fgbuffer =
      (std::complex<float> *)malloc(fgbuffer_len * sizeof(std::complex<float>));

  // create frame synchronizer
  fs = ofdmflexframesync_create(rx_params.numSubcarriers, rx_params.cp_len,
                                rx_params.taper_len, rx_params.subcarrierAlloc,
                                rxCallback, (void *)&threadInfo[0]);

  // initialize update flags to false
  update_tx_flag = false;
  update_usrp_tx = false;
  recreate_fg = false;
  reset_fg = false;

  update_rx_flag = false;
  update_usrp_rx = false;
  recreate_fs = false;
  reset_fs = false;
  // register UHD message handler
  uhd::msg::register_handler(&uhd_msg_handler);

  struct mq_attr attr_tx;
  attr_tx.mq_maxmsg = 10;
  attr_tx.mq_msgsize = MAX_BUF;

  struct mq_attr attr_rx;
  attr_rx.mq_maxmsg = 10;
  attr_rx.mq_msgsize = MAX_BUF;

  phy_tx_queue = mq_open("/mac2phy", O_RDONLY | O_CREAT, PMODE, &attr_tx);
  phy_rx_queue = mq_open("/phy2mac", O_WRONLY | O_CREAT, PMODE, &attr_rx);

  consumers = 4;

  

  resampler_factor = 4; // samples/symbol

  filter_delay = 8;      // filter delay
  beta = 0.01f; // filter excess bandwidth

  resetRxChannels();

  //resetResampler();
}

// Destructor
PhyLayer::~PhyLayer()
{

  delete h;
  delete g;
  dprintf("Stopping transceiver\n");
  stop_rx();
  stop_tx();

  mq_close(phy_tx_queue);
  mq_close(phy_rx_queue);
  mq_unlink("/mac2phy");
  mq_unlink("/phy2mac");
  // sleep so tx/rx threads are ready for signal
  usleep(1e4);

  // signal condition (tell rx worker to continue)
  dprintf("destructor signaling rx condition...\n");
  rx_thread_running = false;
  pthread_cond_signal(&rx_cond);

  dprintf("destructor joining rx thread...\n");
  void *rx_exit_status;
  pthread_join(rx_process, &rx_exit_status);

  // signal condition (tell tx worker to continue)
  dprintf("destructor signaling tx condition...\n");
  tx_thread_running = false;
  pthread_cond_signal(&tx_cond);

  dprintf("destructor joining tx thread...\n");
  void *tx_exit_status;
  pthread_join(tx_process, &tx_exit_status);

  // destroy rx threading objects
  dprintf("destructor destroying rx mutex...\n");
  pthread_mutex_destroy(&rx_mutex);
  pthread_mutex_destroy(&rx_params_mutex);
  dprintf("destructor destroying rx condition...\n");
  pthread_cond_destroy(&rx_cond);

  // destroy tx threading objects
  dprintf("destructor destroying tx mutex...\n");
  pthread_mutex_destroy(&tx_mutex);
  pthread_mutex_destroy(&tx_params_mutex);
  dprintf("destructor destroying tx condition...\n");
  pthread_cond_destroy(&tx_cond);

  // destroy framing objects
  dprintf("destructor destroying other objects...\n");
  ofdmflexframegen_destroy(fg);
  ofdmflexframesync_destroy(fs);

  for(int i = 0; i < consumers;i++){
    ofdmflexframesync_destroy(fsyncs[i]);
  }

  timer_destroy(tx_timer);

  // free memory for subcarrier allocation if necessary
  if (tx_params.subcarrierAlloc)
    free(tx_params.subcarrierAlloc);

  delete recvQueue;
  delete analysisThreads;
  delete threadInfo;
  delete fsyncs;

}

void uhd_msg_handler(uhd::msg::type_t type, const std::string &msg)
{
  
  if ((!strcmp(msg.c_str(), "O")) || (!strcmp(msg.c_str(), "D"))){
    PhyLayer::uhd_msg = 1;
    std::cout << msg << "\n";
  }
  else if (!strcmp(msg.c_str(), "U")){
    PhyLayer::uhd_msg = 2;
    std::cout << msg << "\n";
  }
}

void PhyLayer::set_uhd(uhd::usrp::multi_usrp::sptr &usrp_tx,
                       uhd::usrp::multi_usrp::sptr &usrp_rx, uhd::device_addr_t &dev_addr,
                       char *tx_sub_addr, char *rx_sub_addr)
{

  usrp_tx = uhd::usrp::multi_usrp::make(dev_addr);
  usrp_rx = uhd::usrp::multi_usrp::make(dev_addr);
  usrp_rx->set_rx_antenna("RX2", 0);
  usrp_tx->set_tx_antenna("TX/RX", 0);
  if (custom_uhd_set == true)
  {
    usrp_rx->set_rx_subdev_spec(uhd::usrp::subdev_spec_t(rx_sub_addr), 0);
    usrp_tx->set_tx_subdev_spec(uhd::usrp::subdev_spec_t(tx_sub_addr), 0);
  }
}

void PhyLayer::set_attributes()
{
  if (custom_uhd_set == false)
  {
    set_uhd(usrp_tx, usrp_rx, dev_addr, (char *)" ", (char *)" ");
  }

  // create and start rx thread
  dprintf("Starting rx thread...\n");
  rx_state = RX_STOPPED;    // receiver is not running initially
  rx_thread_running = true; // receiver thread IS running initially
  rx_worker_state = WORKER_HALTED;
  pthread_mutex_init(&rx_mutex, NULL);        // receiver mutex
  pthread_mutex_init(&rx_params_mutex, NULL); // receiver parameter mutexframe_num
  pthread_cond_init(&rx_cond, NULL);          // receiver condition
  pthread_create(&rx_process, NULL, PHY_rx_worker, (void *)this);

  // create and start tx thread
  frame_num = 0;
  tx_state = TX_STOPPED; // transmitter is not running initially
  tx_complete = false;
  tx_thread_running = true;                   // transmitter thread IS running initially
  tx_worker_state = WORKER_HALTED;            // transmitter worker is not yet ready for signal
  pthread_mutex_init(&tx_mutex, NULL);        // transmitter mutex
  pthread_mutex_init(&tx_params_mutex, NULL); // transmitter parameter mutex
  pthread_cond_init(&tx_cond, NULL);          // transmitter condition
  pthread_create(&tx_process, NULL, PHY_tx_worker, (void *)this);

  tx_timer = timer_create();

  // initialize default tx values
  dprintf("Initializing USRP settings...\n");
  set_tx_freq(tx_params.tx_freq);
  set_tx_rate(tx_params.tx_rate);
  set_tx_gain_soft(tx_params.tx_gain_soft);
  set_tx_gain_uhd(tx_params.tx_gain_uhd);
  set_rx_freq(rx_params.rx_freq);
  set_rx_rate(rx_params.rx_rate);
  set_rx_gain_uhd(rx_params.rx_gain_uhd);


  pthread_mutex_init(&tx_rx_mutex, NULL);
  dprintf("Finished creating PHY\n");
}

////////////////////////////////////////////////////////////////////////
// Transmit methods
////////////////////////////////////////////////////////////////////////

// start transmitter
void PhyLayer::start_tx()
{

  // Ensure that the tx_worker ends up in the correct state.
  // There are three possible states it may be in initially.
  pthread_mutex_lock(&tx_params_mutex);
  tx_state = TX_CONTINUOUS;
  bool wait = false;
  switch (tx_worker_state)
  {
  case (WORKER_HALTED):
    dprintf("Waiting for tx worker thread to be ready\n");
    wait = true;
    while (wait)
    {
      pthread_mutex_unlock(&tx_params_mutex);
      usleep(5e2);
      pthread_mutex_lock(&tx_params_mutex);
      wait = (tx_worker_state == WORKER_HALTED);
    }
  // fall through to signal 0.0430923 0.0185445
  case (WORKER_READY):
    dprintf("Signaling tx worker thread\n");
    pthread_cond_signal(&tx_cond);
    break;
  case (WORKER_RUNNING):
    break;
  }
  pthread_mutex_unlock(&tx_params_mutex);
}

// transmitter worker thread
void *PHY_tx_worker(void *_arg)
{
  // type cast input argument as PHY object
  PhyLayer *PHY = (PhyLayer *)_arg;

  // set up transmit buffer
  int buffer_len = MAX_BUF;
  char buffer[MAX_BUF];
  unsigned char *payload = new unsigned char[MAX_BUF];
  unsigned int payload_len;
  int nread;
  std::ofstream log("yo.txt");
  unsigned int count = 0;
  while (PHY->tx_thread_running)
  {
    // wait for signal to start
    dprintf("tx worker waiting for start condition\n");
    pthread_mutex_lock(&(PHY->tx_params_mutex));
    PHY->tx_worker_state = WORKER_READY;
    dprintf("Waiting\n");
    pthread_cond_wait(&(PHY->tx_cond), &(PHY->tx_params_mutex));
    PHY->tx_worker_state = WORKER_RUNNING;
    pthread_mutex_unlock(&(PHY->tx_params_mutex));
    dprintf("tx worker waiting for start condition\n");

    memset(buffer, 0, buffer_len);

    fd_set fds;
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 1000;

    // variables to keep track of number of frames and max tx time if needeed
    PHY->tx_frame_counter = 0;
    timer_tic(PHY->tx_timer);
    struct mq_attr attr_tx;
    // run transmitter
    bool tx_continue = true;
    while (tx_continue)
    {
      pthread_mutex_lock(&PHY->tx_params_mutex);
      if (PHY->update_tx_flag)
      {
        PHY->update_tx_params();
      }
      pthread_mutex_unlock(&PHY->tx_params_mutex);


      if(PHY->tx_params.tx_freq > PHY->rx_params.rx_freq){
        PHY->tx_side = 0x01;
        PHY->random_offset = -3*PHY->nco_offset;
      } else{
        PHY->tx_side = 0x00;
        PHY->random_offset = 3*PHY->nco_offset;
      }
      /////
      ///// TODO: Add reading from tuntap interface
      /////
      /////

      /*
      // getting information from the mac
      */
      struct timespec timeout;
      clock_gettime(CLOCK_REALTIME, &timeout);

      timeout.tv_nsec = timeout.tv_nsec + 5 * 1e6;
      if (timeout.tv_nsec > 1e9)
      {
        timeout.tv_sec = timeout.tv_sec + 1;
        timeout.tv_nsec = timeout.tv_nsec - 1e9;
      }

      int status = 0;
      if(PHY->random_data){
        status = 1000;
        memset(buffer,'a',status);
        
        PHY->tx_nco_offset = PHY->random_offset;
        buffer[status-1] = PHY->tx_side;
        usleep(1000000);
      } else{
        status = mq_timedreceive(PHY->phy_tx_queue, buffer, buffer_len, 0, &timeout);
        mq_getattr(PHY->phy_tx_queue,&attr_tx);
        PHY->tx_nco_offset = PHY->random_offset;
      }

      if (status == -1)
      {
        if (errno != ETIMEDOUT)
        {
          if (PHY->tx_state != TX_STOPPED)
          {
            perror("Failed to read from queue");
            exit(0);
          }
        }
      } 
      else
      {
        printf("Current Queue Len: %lu \n",attr_tx.mq_curmsgs);
        char control = buffer[status-1];
        dprintf("MAC Control: %x\n",control);
        if(control == 0x03){
          dprintf("Control Frame\n");
          //PHY->route_resend = true;
        } else if(control== PHY->tx_side){

        } else{
          //PHY->changeTxChannel();
        }
        payload_len = status-1;
        //  strncpy((char*)payload, buffer, payload_len);
        memcpy(payload, (unsigned char *)buffer, payload_len);
        PHY->tx_frame_counter++;

        pthread_mutex_lock(&PHY->tx_params_mutex);
        if (PHY->update_tx_flag)
        {
          PHY->update_tx_params();
        }
        pthread_mutex_unlock(&PHY->tx_params_mutex);
    
        PHY->transmit_frame(PhyLayer::DATA, payload, payload_len);
        if(PHY->route_resend){
          //PHY->changeTxChannel();
          PHY->transmit_frame(PhyLayer::DATA, payload, payload_len);
          PHY->route_resend = false;
          printf("Current tx_freq: %f\n", PHY->get_tx_freq());
        }
        memset(buffer, 0, buffer_len);
      }
      // change state to stopped once all frames have been transmitted
      // or max transmission time has passed when in burst mode
      pthread_mutex_lock(&PHY->tx_params_mutex);
      if ((PHY->tx_state == TX_BURST) &&
          ((PHY->tx_frame_counter >= PHY->num_tx_frames) ||
           (timer_toc(PHY->tx_timer) * 1.0e3 > PHY->max_tx_time_ms)))
      {
        PHY->tx_state = TX_STOPPED;
      }

      // check if tx has been stopped
      if (PHY->tx_state == TX_STOPPED)
      {
        dprintf("tx worker halting\n");
        tx_continue = false;
        PHY->tx_worker_state = WORKER_HALTED;
      }
      pthread_mutex_unlock(&PHY->tx_params_mutex);
      count++;
    } // while tx_running

    dprintf("tx_worker finished running\n");
  } // while tx_thread_running
  dprintf("tx_worker exiting thread\n");
  pthread_exit(NULL);
}

void PhyLayer::transmit_frame(unsigned int frame_type,
                              unsigned char *_payload,
                              unsigned int _payload_len)
{

  pthread_mutex_lock(&tx_rx_mutex);
  transmitting = true;
  pthread_mutex_unlock(&tx_rx_mutex);
  // vector buffer to send data to device
  std::vector<std::complex<float>> usrp_buffer(fgbuffer_len);
  memset(&usrp_buffer.front(),0,fgbuffer_len);
  if(tx_params.tx_freq > rx_params.rx_freq){
    tx_header[3] = 0x00;
    tx_nco_offset = -3*nco_offset;
  } else{
    tx_header[3] = 0x01; 
    tx_nco_offset = 3*nco_offset;
  }

  pthread_mutex_lock(&tx_params_mutex);
  float tx_gain_soft_lin = powf(10.0f, resampler_factor*tx_params.tx_gain_soft / 20.0f);
  tx_header[0] = ((frame_num >> 8) & 0x3f);
  tx_header[0] |= (frame_type << 6);
  tx_header[1] = (frame_num)&0xff;
  frame_num++;
  pthread_mutex_unlock(&tx_params_mutex);

  // set up the metadta flags
  metadata_tx.start_of_burst = true; // never SOB when continuous
  metadata_tx.end_of_burst = false;  //
  metadata_tx.has_time_spec = false; // set to false to send immediately
  // TODO: flush buffers

  pthread_mutex_lock(&tx_mutex);
  struct timeval ts;
  gettimeofday(&ts, NULL);
  // assemble frame
  ofdmflexframegen_assemble(fg, tx_header, _payload, _payload_len);
  // dprintf("Transmitting Frame!!!!\n");
  dprintf("-----------------Transmitting--------------\n");
  dprintf("Frame_num transmitted: %d with %d bytes at %lus and %luus \n",
         frame_num - 1, _payload_len, ts.tv_sec, ts.tv_usec);
  //dprintf("FG BUFFER LEN: %d\n",fgbuffer_len);
  // generate a single OFDM frame
  bool last_symbol = false;
  unsigned int i;

  num_symbols = fgbuffer_len;
  num_samples = resampler_factor * num_symbols;
  
  nco_crcf q = nco_crcf_create(LIQUID_NCO);
  nco_crcf_set_frequency(q, 2*M_PI*tx_nco_offset/tx_params.tx_rate);
  dprintf("offset: %f \n",tx_nco_offset);
  bool header = true;
  // send initial zeros for the decimation filter filter_delay
  usrp_tx->get_device()->send(&usrp_buffer.front(), filter_delay,
                                metadata_tx, uhd::io_type_t::COMPLEX_FLOAT32,
                                uhd::device::SEND_MODE_FULL_BUFF);
  ofdmflexframegen_print(fg);
  int ofdm_symbols = 0;
  /*
  float fc=0.44f;         // filter cutoff frequency
  float ft=0.02f;         // filter transition
  float As=100.0f;         // stop-band attenuation [dB]
  float mu=0.0f;          // fractional timing offset

    // estimate required filter length and generate filter
  unsigned int h_len = estimate_req_filter_len(ft,As);
  float hf[h_len];
  printf("Filter Length: %d\n",h_len);
  //liquid_firdes_kaiser(h_len,fc,As,mu,hf);
  //firfilt_crcf tx_filt = firfilt_crcf_create(hf,h_len); 
  */
  while (!last_symbol)
  {

    // generate symbol
    last_symbol = ofdmflexframegen_write(fg, fgbuffer, fgbuffer_len);
    
    // copy symbol and apply gain
    for (i = 0; i < fgbuffer_len; i++)
    {
      usrp_buffer[i] = fgbuffer[i] * tx_gain_soft_lin;
    }
    //firfilt_crcf_execute_block(tx_filt, &usrp_buffer[0], usrp_buffer.size(), &usrp_buffer[0]);
    //ofdmflexframesync_execute(fs, &usrp_buffer[0], usrp_buffer.size());
    /*
    if (last_symbol)
    {
      for (; i < num_symbols; i++)
      {
        usrp_buffer.push_back(0);
      }
      num_symbols = fgbuffer_len + 2 * filter_delay;
      num_samples = resampler_factor * num_symbols;
    }

    std::complex<float> z[num_samples];

    firinterp_crcf_execute_block(interp, &usrp_buffer[0], usrp_buffer.size(), z);

    usrp_buffer.resize(num_samples);

    nco_crcf_mix_block_up(q, z, z, usrp_buffer.size());
    
    memcpy(&usrp_buffer[0], z, usrp_buffer.size() * sizeof(std::complex<float>));
    */
    // send samples to the device
    usrp_tx->get_device()->send(&usrp_buffer.front(), usrp_buffer.size(),
                                metadata_tx, uhd::io_type_t::COMPLEX_FLOAT32,
                                uhd::device::SEND_MODE_FULL_BUFF);
    if (loop)
    {
      test_loop->transmit(usrp_buffer, usrp_buffer.size());
    }
    
    //metadata_tx.start_of_burst = false; // never SOB when continuou
    
    if(!last_symbol){
      //usrp_buffer.resize(fgbuffer_len);
    }
    ofdm_symbols++;
  } 
  //firfilt_crcf_destroy(tx_filt);
  printf("OFDM SYMBOLS SENT: %d\n",ofdm_symbols);
  /*
  if(loop){
    memset(&usrp_buffer[0],0,100*sizeof(std::complex<float>));
    test_loop->transmit(usrp_buffer, 100);
  }
  */
  // while loop
  // send a few extra samples to the device
  // NOTE: this seems necessary to preserve last OFDM symbol in
  //       frame from corruption
  /*
  usrp_tx->get_device()->send(&usrp_buffer.front(), usrp_buffer.size(),
                              metadata_tx, uhd::io_type_t::COMPLEX_FLOAT32,
                              uhd::device::SEND_MODE_FULL_BUFF);
  
  memset(&usrp_buffer.front(),0,fgbuffer_len);

  usrp_tx->get_device()->send(&usrp_buffer.front(), filter_delay,
                                metadata_tx, uhd::io_type_t::COMPLEX_FLOAT32,
                                uhd::device::SEND_MODE_FULL_BUFF);
  */                              
  nco_crcf_destroy(q);
  // send a mini EOB packet
  metadata_tx.end_of_burst = true;
  
  usrp_tx->get_device()->send("", 0, metadata_tx,
                              uhd::io_type_t::COMPLEX_FLOAT32,
                              uhd::device::SEND_MODE_FULL_BUFF);
  
  pthread_mutex_unlock(&tx_mutex);

  pthread_mutex_lock(&tx_rx_mutex);
  transmitting = false;
  pthread_mutex_unlock(&tx_rx_mutex);
}
// start transmitter
void PhyLayer::start_tx_burst(unsigned int _num_tx_frames,
                              float _max_tx_time_ms)
{

  // Ensure that the tx_worker ends up in the correct state.
  // There are three possible states it may be in initially.
  pthread_mutex_lock(&tx_params_mutex);
  tx_state = TX_BURST;
  num_tx_frames = _num_tx_frames;
  max_tx_time_ms = _max_tx_time_ms;
  bool wait = false;
  switch (tx_worker_state)
  {
  case (WORKER_HALTED):
    dprintf("Waiting for tx worker thread to be ready\n");
    wait = true;
    while (wait)
    {
      pthread_mutex_unlock(&tx_params_mutex);
      usleep(5e2);
      pthread_mutex_lock(&tx_params_mutex);
      wait = (tx_worker_state == WORKER_HALTED);
    }
  // fall through to signal
  case (WORKER_READY):
    dprintf("Signaling tx worker thread\n");
    pthread_cond_signal(&tx_cond);
    break;
  case (WORKER_RUNNING):
    break;
  }
  pthread_mutex_unlock(&tx_params_mutex);
}

// stop transmitter
void PhyLayer::stop_tx()
{
  pthread_mutex_lock(&tx_params_mutex);
  tx_state = TX_STOPPED;
  pthread_mutex_unlock(&tx_params_mutex);
}

// reset transmitter objects and buffers
void PhyLayer::reset_tx()
{
  pthread_mutex_lock(&tx_params_mutex);
  reset_fg = true;
  pthread_mutex_unlock(&tx_params_mutex);
}

// set transmitter frequency
void PhyLayer::set_tx_freq(double _tx_freq)
{
  pthread_mutex_lock(&tx_params_mutex);
  tx_params_updated.tx_freq = _tx_freq;
  tx_params_updated.tx_dsp_freq = 0.0;
  update_tx_flag = true;
  update_usrp_tx = true;
  pthread_mutex_unlock(&tx_params_mutex);
}

// set transmitter frequency
void PhyLayer::set_tx_freq(double _tx_freq, double _dsp_freq)
{
  pthread_mutex_lock(&tx_params_mutex);
  tx_params_updated.tx_freq = _tx_freq;
  tx_params_updated.tx_dsp_freq = _dsp_freq;
  update_tx_flag = true;
  update_usrp_tx = true;
  pthread_mutex_unlock(&tx_params_mutex);
}

// get transmitter state
int PhyLayer::get_tx_state()
{
  pthread_mutex_lock(&tx_params_mutex);
  int s = tx_state;
  pthread_mutex_unlock(&tx_params_mutex);
  return s;
}

// get transmitter frequency
double PhyLayer::get_tx_freq()
{
  pthread_mutex_lock(&tx_params_mutex);
  double f = tx_params.tx_freq + tx_params.tx_dsp_freq;
  pthread_mutex_unlock(&tx_params_mutex);
  return f;
}

// get transmitter LO frequency
double PhyLayer::get_tx_lo_freq()
{
  pthread_mutex_lock(&tx_params_mutex);
  double f = tx_params.tx_freq;
  pthread_mutex_unlock(&tx_params_mutex);
  return f;
}

// get transmitter dsp frequency
double PhyLayer::get_tx_dsp_freq()
{
  pthread_mutex_lock(&tx_params_mutex);
  double f = tx_params.tx_dsp_freq;
  pthread_mutex_unlock(&tx_params_mutex);
  return f;
}

// set transmitter sample rate
void PhyLayer::set_tx_rate(double _tx_rate)
{
  pthread_mutex_lock(&tx_params_mutex);
  tx_params_updated.tx_rate = _tx_rate;
  update_tx_flag = true;
  update_usrp_tx = true;
  pthread_mutex_unlock(&tx_params_mutex);
}

// get transmitter sample rate
double PhyLayer::get_tx_rate()
{
  pthread_mutex_lock(&tx_params_mutex);
  double r = tx_params.tx_rate;
  pthread_mutex_unlock(&tx_params_mutex);
  return r;
}

// set transmitter software gain
void PhyLayer::set_tx_gain_soft(double _tx_gain_soft)
{
  pthread_mutex_lock(&tx_params_mutex);
  tx_params_updated.tx_gain_soft = _tx_gain_soft;
  update_tx_flag = true;
  pthread_mutex_unlock(&tx_params_mutex);
}

// get transmitter software gain
double PhyLayer::get_tx_gain_soft()
{
  pthread_mutex_lock(&tx_params_mutex);
  double g = tx_params_updated.tx_gain_soft;
  pthread_mutex_unlock(&tx_params_mutex);
  return g;
}

// set transmitter hardware (UHD) gain
void PhyLayer::set_tx_gain_uhd(double _tx_gain_uhd)
{
  pthread_mutex_lock(&tx_params_mutex);
  tx_params_updated.tx_gain_uhd = _tx_gain_uhd;
  update_tx_flag = true;
  update_usrp_tx = true;
  pthread_mutex_unlock(&tx_params_mutex);
}

// get transmitter hardware (UHD) gain
double PhyLayer::get_tx_gain_uhd()
{
  pthread_mutex_lock(&tx_params_mutex);
  double g = tx_params.tx_gain_uhd;
  pthread_mutex_unlock(&tx_params_mutex);
  return g;
}

// set modulation scheme
void PhyLayer::set_tx_modulation(int mod_scheme)
{
  pthread_mutex_lock(&tx_params_mutex);
  tx_params_updated.fgprops.mod_scheme = mod_scheme;
  update_tx_flag = true;
  pthread_mutex_unlock(&tx_params_mutex);
}

// get modulation scheme
int PhyLayer::get_tx_modulation()
{
  pthread_mutex_lock(&tx_params_mutex);
  int m = tx_params.fgprops.mod_scheme;
  pthread_mutex_unlock(&tx_params_mutex);
  return m;
}

// set CRC scheme
void PhyLayer::set_tx_crc(int crc_scheme)
{
  pthread_mutex_lock(&tx_params_mutex);
  tx_params_updated.fgprops.check = crc_scheme;
  update_tx_flag = true;
  pthread_mutex_unlock(&tx_params_mutex);
}

// get CRC scheme
int PhyLayer::get_tx_crc()
{
  pthread_mutex_lock(&tx_params_mutex);
  int c = tx_params.fgprops.check;
  pthread_mutex_unlock(&tx_params_mutex);
  return c;
}

// set FEC0
void PhyLayer::set_tx_fec0(int fec_scheme)
{
  pthread_mutex_lock(&tx_params_mutex);
  tx_params_updated.fgprops.fec0 = fec_scheme;
  update_tx_flag = true;
  pthread_mutex_unlock(&tx_params_mutex);
}

// get FEC0
int PhyLayer::get_tx_fec0() { return tx_params.fgprops.fec0; }

// set FEC1
void PhyLayer::set_tx_fec1(int fec_scheme)
{
  pthread_mutex_lock(&tx_params_mutex);
  tx_params_updated.fgprops.fec1 = fec_scheme;
  update_tx_flag = true;
  pthread_mutex_unlock(&tx_params_mutex);
}

// get FEC1
int PhyLayer::get_tx_fec1() { return tx_params.fgprops.fec1; }

// set number of subcarriers
void PhyLayer::set_tx_subcarriers(
    unsigned int _numSubcarriers)
{
  pthread_mutex_lock(&tx_params_mutex);
  tx_params_updated.numSubcarriers = _numSubcarriers;
  update_tx_flag = true;
  recreate_fg = true;
  pthread_mutex_unlock(&tx_params_mutex);
}

// get number of subcarriers
unsigned int PhyLayer::get_tx_subcarriers()
{
  pthread_mutex_lock(&tx_params_mutex);
  unsigned int n = tx_params.numSubcarriers;
  pthread_mutex_unlock(&tx_params_mutex);
  return n;
}
void PhyLayer::set_tx_subcarrier_alloc(char *_subcarrierAlloc)
{

  pthread_mutex_lock(&tx_params_mutex);
  tx_params_updated.subcarrierAlloc =
      (unsigned char *)realloc((void *)tx_params_updated.subcarrierAlloc,
                               tx_params_updated.numSubcarriers);
  if (_subcarrierAlloc != NULL)
  {
    memcpy(tx_params_updated.subcarrierAlloc, _subcarrierAlloc,
           tx_params_updated.numSubcarriers);
  }
  else
  {
    ofdmframe_init_default_sctype(tx_params_updated.numSubcarriers,
                                  tx_params_updated.subcarrierAlloc);
  }

  // calculate the number of data subcarriers
  numDataSubcarriers = 0;
  for (unsigned int i = 0; i < tx_params_updated.numSubcarriers; i++)
  {
    if (tx_params_updated.subcarrierAlloc[i] == OFDMFRAME_SCTYPE_DATA)
      numDataSubcarriers++;
  }

  update_tx_flag = true;
  recreate_fg = true;
  pthread_mutex_unlock(&tx_params_mutex);
}

// get subcarrier allocation
void PhyLayer::get_tx_subcarrier_alloc(char *subcarrierAlloc)
{
  pthread_mutex_lock(&tx_params_mutex);
  memcpy(subcarrierAlloc, tx_params.subcarrierAlloc, tx_params.numSubcarriers);
  pthread_mutex_unlock(&tx_params_mutex);
}

// set cp_len
void PhyLayer::set_tx_cp_len(unsigned int _cp_len)
{
  pthread_mutex_lock(&tx_params_mutex);
  tx_params_updated.cp_len = _cp_len;
  update_tx_flag = true;
  recreate_fg = true;
  pthread_mutex_unlock(&tx_params_mutex);
}

// get cp_len
unsigned int PhyLayer::get_tx_cp_len()
{
  pthread_mutex_lock(&tx_params_mutex);
  unsigned int c = tx_params.cp_len;
  pthread_mutex_unlock(&tx_params_mutex);
  return c;
}

// set taper_len
void PhyLayer::set_tx_taper_len(unsigned int _taper_len)
{
  pthread_mutex_lock(&tx_params_mutex);
  tx_params_updated.taper_len = _taper_len;
  update_tx_flag = true;
  recreate_fg = true;
  pthread_mutex_unlock(&tx_params_mutex);
}

// get taper_len
unsigned int PhyLayer::get_tx_taper_len()
{
  pthread_mutex_lock(&tx_params_mutex);
  unsigned int t = tx_params.taper_len;
  pthread_mutex_unlock(&tx_params_mutex);
  return t;
}

// set control info (must have length 6)
void PhyLayer::set_tx_control_info(
    unsigned char *_control_info)
{
  pthread_mutex_lock(&tx_params_mutex);
  memcpy(&tx_header[2], _control_info, 6 * sizeof(unsigned char));
  pthread_mutex_unlock(&tx_params_mutex);
}

// get control info
void PhyLayer::get_tx_control_info(
    unsigned char *_control_info)
{
  pthread_mutex_lock(&tx_params_mutex);
  memcpy(_control_info, &tx_header[2], 6 * sizeof(unsigned char));
  pthread_mutex_unlock(&tx_params_mutex);
}

// set tx payload length
void PhyLayer::set_tx_payload_sym_len(unsigned int len)
{
  pthread_mutex_lock(&tx_params_mutex);
  tx_params_updated.payload_sym_length = len;
  update_tx_flag = true;
  pthread_mutex_unlock(&tx_params_mutex);
}

// update the actual parameters being used by the transmitter
void PhyLayer::update_tx_params()
{

  // copy all the new parameters
  tx_params = tx_params_updated;

  // update the USRP
  if (update_usrp_tx)
  {
    pthread_mutex_lock(&tx_mutex);
    usrp_tx->set_tx_gain(tx_params.tx_gain_uhd);
    usrp_tx->set_tx_rate(tx_params.tx_rate);

    uhd::tune_request_t tune;
    tune.rf_freq_policy = uhd::tune_request_t::POLICY_MANUAL;
    tune.dsp_freq_policy = uhd::tune_request_t::POLICY_MANUAL;
    tune.rf_freq = tx_params.tx_freq;
    tune.dsp_freq = tx_params.tx_dsp_freq;
    printf("Hopping to new freq: %f\n",tune.rf_freq );
    usrp_tx->set_tx_freq(tune);
    update_usrp_tx = false;
    pthread_mutex_unlock(&tx_mutex);
  }

  // recreate the frame generator only if necessary
  if (recreate_fg)
  {

    pthread_mutex_lock(&tx_mutex);
    ofdmflexframegen_destroy(fg);
    fg = ofdmflexframegen_create(tx_params.numSubcarriers, tx_params.cp_len,
                                 tx_params.taper_len, tx_params.subcarrierAlloc,
                                 &tx_params.fgprops);
    pthread_mutex_unlock(&tx_mutex);
    recreate_fg = false;
  }

  if (reset_fg)
  {
    ofdmflexframegen_reset(fg);
    reset_fg = false;
  }

  ofdmflexframegen_setprops(fg, &tx_params.fgprops);

  // make sure the frame generator buffer is appropriately sized
  fgbuffer_len = tx_params.numSubcarriers + tx_params.cp_len;
  fgbuffer = (std::complex<float> *)realloc(
      (void *)fgbuffer, fgbuffer_len * sizeof(std::complex<float>));

  // reset flag
  update_tx_flag = false;

  // set update data rate flag
  update_tx_data_rate = true;
}

/////////////////////////////////////////////////////////////////////
// Receiver methods
/////////////////////////////////////////////////////////////////////

// update the actual parameters being used by the receiver
void PhyLayer::update_rx_params()
{

  // update the USRP
  if (update_usrp_rx)
  {
    usrp_rx->set_rx_gain(rx_params.rx_gain_uhd);
    usrp_rx->set_rx_rate(rx_params.rx_rate);
    //usrp_rx->set_master_clock_rate(4*rx_params.rx_rate);
    uhd::tune_request_t tune1(rx_params.rx_freq);
    tune1.rf_freq_policy = uhd::tune_request_t::POLICY_MANUAL;
    tune1.dsp_freq_policy = uhd::tune_request_t::POLICY_MANUAL;
    tune1.rf_freq = rx_params.rx_freq;
    tune1.dsp_freq = rx_params.rx_dsp_freq;

    usrp_rx->set_rx_freq(tune1, 0);
    /*
    uhd::tune_request_t tune2(rx_params.rx_freq+10e6);
    tune2.rf_freq_policy = uhd::tune_request_t::POLICY_MANUAL;
    tune2.dsp_freq_policy = uhd::tune_request_t::POLICY_MANUAL;
    tune2.rf_freq = rx_params.rx_freq+10e6;
    tune2.dsp_freq = rx_params.rx_dsp_freq;

    usrp_rx->set_rx_freq(tune2, 1);
*/
    update_usrp_rx = false;
  }

  // recreate the frame synchronizer only if necessary
  if (recreate_fs)
  {
    ofdmflexframesync_destroy(fs);
    fs = ofdmflexframesync_create(
        rx_params.numSubcarriers, rx_params.cp_len, rx_params.taper_len,
        rx_params.subcarrierAlloc, rxCallback, (void *)&threadInfo[0]);
    threadInfo[0].fs_thread = &fs;
    //setRxChannels(consumers);
    
    for(int i = 0; i < consumers;i++){
      ofdmflexframesync_destroy(fsyncs[i]);
      fsyncs[i] = ofdmflexframesync_create(rx_params.numSubcarriers, rx_params.cp_len,
                      rx_params.taper_len, rx_params.subcarrierAlloc,
                      rxCallback, (void *)&threadInfo[i]);
      threadInfo[i].fs_thread = &fsyncs[i];
    }
    recreate_fs = false;
  }

  // reset the frame synchronizer only if necessary
  if (reset_fs)
  {
    ofdmflexframesync_reset(fs);
    reset_fs = false;
  }

  // reset flag
  update_rx_flag = false;
}

// receiver worker thread
void *PHY_rx_worker(void *_arg)
{
  // type cast input argument as PHY object
  PhyLayer *PHY = (PhyLayer *)_arg;

  // set up receive buffers
  PHY->rx_buffer_len =
       PHY->usrp_rx->get_device()->get_max_recv_samps_per_packet();
  PHY->rx_buffer = (std::complex<float> *)malloc(PHY->rx_buffer_len *
                                                 sizeof(std::complex<float>));

  std::complex<float> *buffer_F = (std::complex<float> *)malloc(PHY->rx_buffer_len *
                                                                sizeof(std::complex<float>));

  fftplan fft = fft_create_plan(PHY->rx_buffer_len, reinterpret_cast<liquid_float_complex *>(PHY->rx_buffer),
                                reinterpret_cast<liquid_float_complex *>(buffer_F),
                                LIQUID_FFT_FORWARD, 0);
  //PHY->usrp_rx->set_master_clock_rate(PHY->rx_params.rx_rate*4);
  while (PHY->rx_thread_running)
  {
    // wait for signal to start
    pthread_mutex_lock(&(PHY->rx_params_mutex));
    PHY->rx_worker_state = WORKER_READY;
    pthread_cond_wait(&(PHY->rx_cond), &(PHY->rx_params_mutex));
    dprintf("rx worker received start condition\n");
    PHY->rx_worker_state = WORKER_RUNNING;
    pthread_mutex_unlock(&(PHY->rx_params_mutex));

    // condition given; check state: run or exit
    if (!PHY->rx_thread_running)
    {
      dprintf("rx_worker finished\n");
      break;
    }

    // start the rx stream from the USRP
    pthread_mutex_lock(&PHY->rx_mutex);
    dprintf("rx worker beginning usrp streaming\n");
    pthread_mutex_unlock(&PHY->rx_mutex);

    uhd::stream_args_t stream_args("fc32");
    std::vector<size_t> channel_nums;
    channel_nums.push_back(0);
    // channel_nums.push_back(1);
    stream_args.channels = channel_nums;
    uhd::rx_streamer::sptr rx_stream = PHY->usrp_rx->get_rx_stream(stream_args);
    rx_stream->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
    bool rx_continue = true;

    const size_t samps_per_buff = PHY->rx_buffer_len;//rx_stream->get_max_num_samps();
    std::vector<std::vector<std::complex<float>>> buffs(
        PHY->usrp_rx->get_rx_num_channels(), std::vector<std::complex<float>>(samps_per_buff));
    
    //create a vector of pointers to point to each of the channel buffers
    std::vector<std::complex<float> *> buff_ptrs;
    for (size_t i = 0; i < buffs.size(); i++)
      buff_ptrs.push_back(&buffs[i].front());
    
    std::vector<std::complex<float>> recv(50000);
    std::complex<float> recv_array[50000];
    // run receiver
    while (rx_continue)
    {
      //    dprintf("Reading \n");
      // grab data from USRP and push through the frame synchronizer
      int num_rx_samps = 0;
      pthread_mutex_lock(&(PHY->rx_mutex));

      if (PHY->loop)
      {
        num_rx_samps = PHY->test_loop->receive(recv);
        if (num_rx_samps > 0)
        {
          memcpy(recv_array, &recv[0], num_rx_samps * sizeof(std::complex<float>));
          //std::cout << "Adding to Queue\n";
          PHY->recvQueue->enqueue(recv_array,num_rx_samps);
          //std::cout << "Done Adding to Queue\n";
        }
      }
      else
      {
        //std::cout << "Here1\n";
        num_rx_samps = rx_stream->recv(
            buff_ptrs, PHY->rx_buffer_len, PHY->metadata_rx);
        
        memcpy(PHY->rx_buffer, buff_ptrs[0], PHY->rx_buffer_len * sizeof(std::complex<float>));
        memcpy(recv_array, buff_ptrs[0], PHY->rx_buffer_len * sizeof(std::complex<float>));

        if(num_rx_samps > 0){
          //std::cout << "Power: " << sum_power <<"\n";
          PHY->recvQueue->enqueue(recv_array,num_rx_samps);
        }

      }

      pthread_mutex_unlock(&(PHY->rx_mutex));
      // update parameters if necessary
      pthread_mutex_lock(&PHY->rx_mutex);
      pthread_mutex_lock(&PHY->rx_params_mutex);
      if (PHY->update_rx_flag)
        PHY->update_rx_params();
      pthread_mutex_unlock(&PHY->rx_mutex);
      pthread_mutex_unlock(&PHY->rx_params_mutex);

      // we need to tightly control the state of the worker thread
      // to protect against issues with abrupt starts and stops
      if (PHY->rx_state == RX_STOPPED)
      {
        dprintf("rx worker halting\n");
        rx_continue = false;
        PHY->rx_worker_state = WORKER_HALTED;
      }
    } // while rx running
    PHY->recvQueue->stopConsumers();
    dprintf("rx_worker finished running\n");
    pthread_mutex_lock(&PHY->rx_mutex);
    rx_stream->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
    pthread_mutex_unlock(&PHY->rx_mutex);

  } // while rx thread is running
  
  free(PHY->rx_buffer);
  free(buffer_F);
  dprintf("rx_worker exiting thread\n");
  pthread_exit(NULL);
}


void *analysis(void *_arg){
  PhyLayer::ThreadInfo *threadInfo = (PhyLayer::ThreadInfo * )_arg;

  PhyLayer *PHY = threadInfo->PHY;
  int consumer = threadInfo->consumer;

  std::cout << "Consumer " << consumer << "- Offset: " <<  threadInfo->nco_offset << "\n";
  nco_crcf q = nco_crcf_create(LIQUID_NCO);
  float rx_rate = PHY->rx_params.rx_rate;
  float nco_offset = threadInfo->nco_offset;
  threadInfo->new_info = false;
  nco_crcf_set_frequency(q, 2*M_PI*nco_offset/PHY->rx_params.rx_rate);
  bool rx_continue = true;
  int n = 1;
  std::complex<float> *x = new std::complex<float> [n];
  while(rx_continue){
    int num_rx_samps = 0;
    std::complex<float> *recv_array = PHY->recvQueue->dequeue(consumer,num_rx_samps);
    if(num_rx_samps == -1){
      break;
    }
    if(num_rx_samps!=n){
      delete[]x;
      n = num_rx_samps;
      x = new std::complex<float> [n];
    }
    

    memcpy(x, &recv_array[0], num_rx_samps * sizeof(std::complex<float>));
    
    if(threadInfo->new_info){
      nco_offset = threadInfo->nco_offset;
      nco_crcf_destroy(q);
      q = nco_crcf_create(LIQUID_NCO);
      nco_crcf_set_frequency(q, 2*M_PI*nco_offset/rx_rate);
      std::cout << "Consumer " << consumer << "- Offset: " <<  threadInfo->nco_offset << "\n";
    }
    
    if(rx_rate!=PHY->rx_params.rx_rate){
      rx_rate = PHY->rx_params.rx_rate;
      nco_crcf_destroy(q);
      q = nco_crcf_create(LIQUID_NCO);
      nco_crcf_set_frequency(q, 2*M_PI*nco_offset/rx_rate);
      std::cout << "Consumer " << consumer << "- Offset: " <<  threadInfo->nco_offset << "\n";
    }

    nco_crcf_mix_block_down(q, x, x, num_rx_samps);
    int recv_symbols = num_rx_samps / PHY->resampler_factor;
    firdecim_crcf_execute_block(PHY->decim[consumer], x, recv_symbols, x);
    ofdmflexframesync_execute(PHY->fsyncs[consumer], x, recv_symbols);
    float rssi = ofdmflexframesync_get_rssi(PHY->fsyncs[consumer]);
    if(rssi > -30){
      //printf(GRN"RSSI: %f\n" RESET ,rssi);
    }
    
    if(threadInfo->packet_found){
      threadInfo->packet_found = false;
      //nco_crcf_destroy(q);
      //q = nco_crcf_create(LIQUID_NCO);
      //nco_crcf_set_frequency(q, 2*M_PI*nco_offset/rx_rate);
    }
  }
  delete[]x;
  nco_crcf_destroy(q);
  firdecim_crcf_destroy(PHY->decim[consumer]);

}

// function to handle frames received by the PHY object
int rxCallback(unsigned char *_header, int _header_valid,
               unsigned char *_payload, unsigned int _payload_len,
               int _payload_valid, framesyncstats_s _stats, void *_userdata)
{
  // typecast user argument as PHY object
  PhyLayer::ThreadInfo *threadInfo = (PhyLayer::ThreadInfo * )_userdata;
  PhyLayer *PHY = threadInfo->PHY;
  threadInfo->packet_found = true;
  int consumer = threadInfo->consumer;
  float freq_offset = threadInfo->nco_offset;
  float rssi = ofdmflexframesync_get_rssi(*threadInfo->fs_thread);
  // typecast user argument as PHY object
  //PhyLayer *PHY = (PhyLayer *)_userdata;
  dprintf("\n---------------------Received at thread %d with offset %f and rssi %f---------------\n",consumer,freq_offset,rssi);
  unsigned int frame_num = ((_header[0] & 0x3F) << 8 | _header[1]);
  dprintf("Frame_num received: %d and %x\n", frame_num,_header[3]);
  struct timeval ts;
  gettimeofday(&ts, NULL);
  dprintf("Received %d bytes at %lus and %luus\n", _payload_len, ts.tv_sec, ts.tv_usec);
  if (_header_valid == 1)
  {
    if (_payload_valid == 1)
    {
      timespec timeout;
      timeout.tv_sec = time(NULL);
      timeout.tv_nsec = 100;
      char mac_load[_payload_len+1];
      mac_load[0] = _header[3];
      memcpy(&mac_load[1],(char *)_payload, _payload_len);
      int status = mq_timedsend(PHY->phy_rx_queue, (char *)mac_load, _payload_len+1, 0, &timeout);
      if (status == -1)
      {
        perror("Queue is full\n");
        dprintf("mq_send failure\n");
      }
      else
      {
        dprintf(GRN "mq_send to mac successful\n" RESET);
      }
    }
    else
    {
      dprintf( RED "Invalid Payload\n" RESET);
    }
  }
  else
  {
     dprintf(YEL "Invalid Header\n" RESET);
  }

  return 0;
}
// set receiver frequency
void PhyLayer::set_rx_freq(double _rx_freq)
{
  pthread_mutex_lock(&rx_params_mutex);
  rx_params.rx_freq = _rx_freq;
  rx_params.rx_dsp_freq = 0.0;
  update_rx_flag = true;
  update_usrp_rx = true;
  pthread_mutex_unlock(&rx_params_mutex);
}

// set receiver frequency
void PhyLayer::set_rx_freq(double _rx_freq, double _dsp_freq)
{
  pthread_mutex_lock(&rx_params_mutex);
  rx_params.rx_freq = _rx_freq;
  rx_params.rx_dsp_freq = _dsp_freq;
  update_rx_flag = true;
  update_usrp_rx = true;
  pthread_mutex_unlock(&rx_params_mutex);
}

// get receiver state
int PhyLayer::get_rx_state()
{
  pthread_mutex_lock(&rx_params_mutex);
  int s = rx_state;
  pthread_mutex_unlock(&rx_params_mutex);
  return s;
}

// get receiver state
int PhyLayer::get_rx_worker_state()
{
  pthread_mutex_lock(&rx_params_mutex);
  int s = rx_worker_state;
  pthread_mutex_unlock(&rx_params_mutex);
  return s;
}

// get receiver LO frequency
double PhyLayer::get_rx_lo_freq()
{
  pthread_mutex_lock(&rx_params_mutex);
  double f = rx_params.rx_freq;
  pthread_mutex_unlock(&rx_params_mutex);
  return f;
}

// get receiver dsp frequency
double PhyLayer::get_rx_dsp_freq()
{
  pthread_mutex_lock(&rx_params_mutex);
  double f = rx_params.rx_dsp_freq;
  pthread_mutex_unlock(&rx_params_mutex);
  return f;
}

// get receiver center frequency
double PhyLayer::get_rx_freq()
{
  pthread_mutex_lock(&rx_params_mutex);
  double f = rx_params.rx_freq - rx_params.rx_dsp_freq;
  pthread_mutex_unlock(&rx_params_mutex);
  return f;
}

// set receiver sample rate
void PhyLayer::set_rx_rate(double _rx_rate)
{
  pthread_mutex_lock(&rx_params_mutex);
  rx_params.rx_rate = _rx_rate;
  update_rx_flag = true;
  update_usrp_rx = true;
  pthread_mutex_unlock(&rx_params_mutex);
}

// get receiver sample rate
double PhyLayer::get_rx_rate()
{
  pthread_mutex_lock(&rx_params_mutex);
  double r = rx_params.rx_rate;
  pthread_mutex_unlock(&rx_params_mutex);
  return r;
}

// set receiver hardware (UHD) gain
void PhyLayer::set_rx_gain_uhd(double _rx_gain_uhd)
{
  pthread_mutex_lock(&rx_params_mutex);
  rx_params.rx_gain_uhd = _rx_gain_uhd;
  update_rx_flag = true;
  update_usrp_rx = true;
  pthread_mutex_unlock(&rx_params_mutex);
}

// get receiver hardware (UHD) gain
double PhyLayer::get_rx_gain_uhd()
{
  pthread_mutex_lock(&rx_params_mutex);
  double g = rx_params.rx_gain_uhd;
  pthread_mutex_unlock(&rx_params_mutex);
  return g;
}

// set receiver antenna
void PhyLayer::set_rx_antenna(char *_rx_antenna)
{
  usrp_rx->set_rx_antenna(_rx_antenna);
}

// set number of subcarriers
void PhyLayer::set_rx_subcarriers(
    unsigned int _numSubcarriers)
{
  pthread_mutex_lock(&rx_params_mutex);
  rx_params.numSubcarriers = _numSubcarriers;
  update_rx_flag = true;
  recreate_fs = true;
  pthread_mutex_unlock(&rx_params_mutex);
}

// get number of subcarriers
unsigned int PhyLayer::get_rx_subcarriers()
{
  pthread_mutex_lock(&rx_params_mutex);
  unsigned int n = rx_params.numSubcarriers;
  pthread_mutex_unlock(&rx_params_mutex);
  return n;
}

// set subcarrier allocation
void PhyLayer::set_rx_subcarrier_alloc(char *_subcarrierAlloc)
{

  pthread_mutex_lock(&rx_params_mutex);
  rx_params.subcarrierAlloc =
      (unsigned char *)realloc((void *)rx_params.subcarrierAlloc,
                               rx_params.numSubcarriers);
  if (_subcarrierAlloc)
  {
    memcpy(rx_params.subcarrierAlloc, _subcarrierAlloc,
           rx_params.numSubcarriers);
  }
  else
  {
    ofdmframe_init_default_sctype(rx_params.numSubcarriers,
                                  rx_params.subcarrierAlloc);
  }

  update_rx_flag = true;
  recreate_fs = true;
  pthread_mutex_unlock(&rx_params_mutex);
}

// get subcarrier allocation
void PhyLayer::get_rx_subcarrier_alloc(char *subcarrierAlloc)
{
  pthread_mutex_lock(&rx_params_mutex);
  memcpy(subcarrierAlloc, rx_params.subcarrierAlloc, rx_params.numSubcarriers);
  pthread_mutex_unlock(&rx_params_mutex);
}

// set cp_len
void PhyLayer::set_rx_cp_len(unsigned int _cp_len)
{
  pthread_mutex_lock(&rx_params_mutex);
  rx_params.cp_len = _cp_len;
  pthread_mutex_unlock(&rx_params_mutex);
}

// get cp_len
unsigned int PhyLayer::get_rx_cp_len()
{
  pthread_mutex_lock(&rx_params_mutex);
  unsigned int c = rx_params.cp_len;
  pthread_mutex_unlock(&rx_params_mutex);
  return c;
}

// set taper_len
void PhyLayer::set_rx_taper_len(unsigned int _taper_len)
{
  pthread_mutex_lock(&rx_params_mutex);
  rx_params.taper_len = _taper_len;
  pthread_mutex_unlock(&rx_params_mutex);
}

// get taper_len
unsigned int PhyLayer::get_rx_taper_len()
{
  pthread_mutex_lock(&rx_params_mutex);
  unsigned int t = rx_params.taper_len;
  pthread_mutex_unlock(&rx_params_mutex);
  return t;
}

// start receiver
void PhyLayer::start_rx()
{

  // Ensure that the rx_worker ends up in the correct state.
  // There are three possible states it may be in initially.
  pthread_mutex_lock(&rx_params_mutex);
  rx_state = RX_CONTINUOUS;
  bool wait = false;
  switch (rx_worker_state)
  {
  case (WORKER_HALTED):
    dprintf("Waiting for rx worker thread to be ready\n");
    wait = true;
    while (wait)
    {
      pthread_mutex_unlock(&rx_params_mutex);
      usleep(5e2);
      pthread_mutex_lock(&rx_params_mutex);
      wait = (rx_worker_state == WORKER_HALTED);
    }
  // fall through to signal
  case (WORKER_READY):
    dprintf("Signaling rx worker thread\n");
    pthread_cond_signal(&rx_cond);
    break;
  case (WORKER_RUNNING):
    break;
  }
  pthread_mutex_unlock(&rx_params_mutex);
}

// stop receiver
void PhyLayer::stop_rx()
{
  pthread_mutex_lock(&rx_params_mutex);
  rx_state = RX_STOPPED;
  pthread_mutex_unlock(&rx_params_mutex);
}

// set the nco_offset for a thread
void PhyLayer::set_nco_offset(int consumer,float nco_offset){
  threadInfo[consumer].nco_offset = nco_offset;
  threadInfo[consumer].new_info = true;
}

void PhyLayer::changeTxChannel(){
  // sleep and wait for any prior tx to finish
  usleep(20000);
  if(tx_side == 0x00){
    dprintf("Changing to Upper Channels\n");
    set_tx_freq(tx_params.tx_freq + 2*tx_params.tx_rate);
    random_offset = -3*nco_offset;
    tx_nco_offset = random_offset;
    tx_side = 0x01;
  } else{
    dprintf("Changing to Lower Channels\n");
    set_tx_freq(tx_params.tx_freq - 2*tx_params.tx_rate);
    random_offset = 3*nco_offset;
    tx_nco_offset = random_offset;
    tx_side = 0x00;
  }

  pthread_mutex_lock(&tx_params_mutex);
  if (update_tx_flag)
  {
    update_tx_params();
  }
  pthread_mutex_unlock(&tx_params_mutex);
  // sleep and wait for the usrp to change frequency
  usleep(20000);
}

void PhyLayer::resetResampler(){

  h_len = 2 * resampler_factor * filter_delay + 1;
  num_symbols = fgbuffer_len + 2 * filter_delay;

  num_samples = resampler_factor * num_symbols;
  delete h;
  delete g;
  h = new float[h_len];
  g = new float[h_len];

  liquid_firdes_rrcos(resampler_factor, filter_delay, beta, 0.3f, h);

  for (int i = 0; i < h_len; i++)
    g[i] = h[h_len - i - 1];

  interp = firinterp_crcf_create(resampler_factor, h, h_len);
  delete decim;
  decim = new firdecim_crcf[consumers];
  for(int i = 0; i < consumers;i++){
    decim[i] = firdecim_crcf_create(resampler_factor, g, h_len);
  }
  
}
void PhyLayer::setRxChannels(double rx_rate,int channels){

  if(channels > 0){
    this->consumers = channels;
    resampler_factor = channels;
    if(channels%2 == 0){
      nco_offset = (rx_rate/2)/channels;
    } else{
      nco_offset = (rx_rate/2)/(channels+1);
    }
    resetRxChannels();
  }
}

void PhyLayer::resetRxChannels(){
  pthread_mutex_lock(&rx_mutex);
  delete recvQueue;
  usleep(1000); // needed just in case all threads do not stop
  delete analysisThreads;
  delete threadInfo;
  delete fsyncs;

  resetResampler();

  int q_len = 500;
  recvQueue = new BufferQ<std::complex<float>>(q_len,consumers);
  analysisThreads = new pthread_t[consumers];
  threadInfo = new ThreadInfo[consumers];
  fsyncs = new ofdmflexframesync[consumers];
  int count = 0;
  for(int i = 0; i < consumers;i++){
    fsyncs[i] = ofdmflexframesync_create(rx_params.numSubcarriers, rx_params.cp_len,
                                rx_params.taper_len, rx_params.subcarrierAlloc,
                                rxCallback, (void *)&threadInfo[i]);
    threadInfo[i].fs_thread = &fsyncs[i];
    threadInfo[i].PHY = this;
    threadInfo[i].consumer = i;
    if(consumers%2 == 0){
      if(i%2 == 0){
        count++;
        threadInfo[i].nco_offset = (2*count-1)*nco_offset;
      } else{
        threadInfo[i].nco_offset = -1*(2*count-1)*nco_offset;
      }
    } else{
      if(i == 0){
        threadInfo[i].nco_offset = 0;
      } else{
        if(i%2 == 0){
          threadInfo[i].nco_offset = 2*count*nco_offset;
        } else{
          count++;
          threadInfo[i].nco_offset = -2*count*nco_offset;
        }
      }
    }

    //threadInfo[i].nco_offset = 2*nco_offset;
    int status = pthread_create(&analysisThreads[i],NULL,analysis,(void *) &threadInfo[i]);
    if(status < 0){
      perror("Failed to create threads");
    }
  }
  pthread_mutex_unlock(&rx_mutex);
}
