#include <stdio.h>
#include <liquid/liquid.h>
#include <limits.h>
#include <net/if.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "SC_Cbsd.hpp"


// constructor
SC_Cbsd::SC_Cbsd(int argc, char** argv) {
  // Create TCP client to CORNET3D
  udp_server_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (udp_server_sock < 0) {
    printf("ERROR: Receiver Failed to Create Client Socket\n");
    exit(EXIT_FAILURE);
  }
  // Parameters for connecting to server
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(CBSD_IP);
  addr.sin_port = htons(CBSD_PORT);
  socklen_t clientlen = sizeof(addr);
  int bind_status = bind(udp_server_sock, (sockaddr *)&addr, clientlen);
  // Attempt to connect client socket to server

  if (bind_status) {
      printf("Binding to port failure.\n");
      exit(EXIT_FAILURE);
  }
  old_mod = 40;
  old_crc = 6;
  old_fec0 = 12;
  old_fec1 = 1;
  old_freq = 770e6;
  old_bandwidth = 1e6;
  old_gain = 20.0;


  ip = get_ip();
  printf("IP: %s\n",ip.c_str());

  mac = get_mac();
  printf("MAC: %s\n",mac.c_str());

  nodeID = std::hash<std::string>{}(mac);
  printf("Hash/Node ID: %d\n",nodeID);
  createParseSocket(9749);
}

// destructor
SC_Cbsd::~SC_Cbsd() {}

// setup feedback enables for each node
void SC_Cbsd::initialize_node_fb() {

    // enable all feedback types
    int fb_enables = INT_MAX;
    //int fb_enables = INT_MAX;
    for(int i=1; i<=sp.num_nodes; i++)
        set_node_parameter(i, CRTS_FB_EN, (void*) &fb_enables);

    double rx_stats_period = 1.0;
    double rx_stats_report_rate = 1.0;
    set_node_parameter(2, CRTS_RX_STATS, (void*) &rx_stats_period);
    set_node_parameter(2, CRTS_RX_STATS_FB, (void*) &rx_stats_report_rate);
}

// execute function
void SC_Cbsd::execute() {
    //Only send feedback to CORNET3D when feedback is received from a node,
    //not when execute is called due to a timeout
    //printf("Hello\n");
    if(sc_event == FEEDBACK)
    {
        switch (fb.fb_type) {
            case CRTS_TX_FREQ:
              {
                double temp_freq = 0;
                memcpy(&temp_freq,fb.arg,sizeof(double));
                printf("tx frequency update: %f for %d\n",temp_freq,fb.node);
                if(fb.node == 1) {
                  myparams.tx_freq = temp_freq;
                } else{
                  myparams.rx_freq = temp_freq;
                }

              }
              break;
            case CRTS_RX_FREQ:
              {
                double temp_freq = 0;
                memcpy(&temp_freq,fb.arg,sizeof(double));
                printf("rx frequency update: %f for %d\n",temp_freq,fb.node);
              }
              break;
            case CRTS_RX_STATS:
              {
                feedback_struct fs;
                printf("received stats update\n");
                struct ExtensibleCognitiveRadio::rx_statistics rx_stats =
                    *(struct ExtensibleCognitiveRadio::rx_statistics*) fb.arg;
                float per = rx_stats.per;
                float throughput = rx_stats.throughput;
                // forward feedback to CORNET 3D web server
                fs.type = 0;
                fs.node = fb.node;
                fs.frequency = per; // frequency is a temp for per
                fs.bandwidth = throughput; // temp for throughput
                send(udp_server_sock, (char*)&fs, sizeof(fs), 0);
                printf("node: %d PER: %f throughput: %f \n",fs.node, fs.frequency, fs.bandwidth);
                parseStats(fs);
              }
              break;

        }
    }
    // forward commands from CORNET 3D webserver to node
    fd_set fds;
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100;
    FD_ZERO(&fds);
    FD_SET(udp_server_sock, &fds);
    crts_signal_params params;
    if(select(udp_server_sock+1, &fds, NULL, NULL, &timeout))
    {
        int rlen = recv(udp_server_sock, &params, sizeof(params), 0);
        if(rlen > 0)
        {

            printf("params.node: %u\n", params.node);
            printf("params.mod: %u\n", params.mod);
            printf("params.crc: %u\n", params.crc);
            printf("params.fec0: %u\n", params.fec0);
            printf("params.fec1: %u\n", params.fec1);
            printf("params.freq: %f\n", params.freq);
            printf("params.bandwidth: %f\n", params.bandwidth);
            printf("params.gain: %f\n", params.gain);
            /*
            //CORNET3D backend sends a 9 when client disconnects
            //Call killall crts_controller with in turn shuts down all nodes
            if(params.type == 9)
            {
                printf("calling killall\n");
                system("killall crts_controller");
                exit(1);
            }

            if(params.mod >= 0 && params.mod != old_mod)
            {
                set_node_parameter(params.node, CRTS_TX_MOD, &params.mod);
                old_mod = params.mod;
            }

            if(params.crc >= 0 && params.crc != old_crc)
            {
                set_node_parameter(params.node, CRTS_TX_CRC, &params.crc);
                old_crc = params.crc;
            }

            if(params.fec0 >= 0 && params.fec0 != old_fec0)
            {
                set_node_parameter(params.node, CRTS_TX_FEC0, &params.fec0);
                old_fec0 = params.fec0;
            }

            if(params.fec1 >= 0 && params.fec1 != old_fec1)
            {
                set_node_parameter(params.node, CRTS_TX_FEC1, &params.fec1);
                old_fec1 = params.fec1;
            }
            */
            if(params.freq >= 0 && params.freq != old_freq)
            {
                set_node_parameter(params.node, CRTS_TX_FREQ, &params.freq);
                set_node_parameter(params.node == 1 ? 2 : 1, CRTS_RX_FREQ, &params.freq);
                old_freq = params.freq;
            }
            /*
            if(params.bandwidth >= 0 && params.bandwidth != old_bandwidth)
            {
                set_node_parameter(params.node, CRTS_TX_RATE, &params.bandwidth);
                set_node_parameter(params.node == 1 ? 2 : 1, CRTS_RX_RATE, &params.bandwidth);
                old_bandwidth = params.bandwidth;
            }

            if(params.gain >= 0 && params.gain != old_gain)
            {
                set_node_parameter(params.node, CRTS_TX_GAIN, &params.gain);
                old_gain = params.gain;
            }
            */
        }
    }

}


void SC_Cbsd::parseStats(feedback_struct fs){
  pmt::pmt_t info = pmt::make_dict();
  pmt::pmt_t key = pmt::string_to_symbol("nodeID");
  pmt::pmt_t value = pmt::mp(nodeID);
  info = pmt::dict_add(info,key,value);
  key = pmt::string_to_symbol("per");
  value = pmt::from_double(fs.frequency);
  info = pmt::dict_add(info,key,value);
  key = pmt::string_to_symbol("throughput");
  value = pmt::from_double(fs.bandwidth);
  info = pmt::dict_add(info,key,value);
  key = pmt::string_to_symbol("type");
  value = pmt::string_to_symbol("SU");
  info = pmt::dict_add(info,key,value);
  key = pmt::string_to_symbol("tx_freq");
  value = pmt::from_double(myparams.tx_freq);
  info = pmt::dict_add(info,key,value);
  key = pmt::string_to_symbol("rx_freq");
  value = pmt::from_double(myparams.rx_freq);
  info = pmt::dict_add(info,key,value);

  std::string serialized_pmt =  pmt::serialize_str(info);
  int len = serialized_pmt.length();
  int p = sendto(udp_client_sock, serialized_pmt.c_str(),len,0,(struct sockaddr *)&mailing_server_addr,sizeof(mailing_server_addr));
  if(p < 0){
    printf("Connection Failed\n");
  }

}

void SC_Cbsd::createParseSocket(int serverPort){
  udp_client_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  mailing_server_addr.sin_family = AF_INET;
  mailing_server_addr.sin_addr.s_addr = inet_addr("192.168.1.21");
  mailing_server_addr.sin_port = htons(serverPort);
}

std::string SC_Cbsd::exec(const char *cmd)
{
  std::array<char, 128> buffer;
  std::string result;
  std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
  if (!pipe)
  throw std::runtime_error("popen() failed!");
  while (!feof(pipe.get()))
  {
    if (fgets(buffer.data(), 128, pipe.get()) != NULL)
    result += buffer.data();
  }
  return result;
}

std::string SC_Cbsd::get_ip() {

  std::string interface_name = "eth0";
  std::string command =   "ifconfig " + interface_name + " | grep 'inet addr:' | cut -d: -f2 | awk '{print $1}'";
  return exec(command.c_str());
}


std::string SC_Cbsd::get_mac() {
  std::string interface_name = "eth0";
  std::string command = "ifconfig " + interface_name + " | grep HWaddr | awk '{print $5}'";
  return exec(command.c_str());
}
