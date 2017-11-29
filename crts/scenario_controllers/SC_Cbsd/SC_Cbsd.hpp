#ifndef _SC_CBSD_
#define _SC_CBSD_
#include "scenario_controller.hpp"
#include "CORNET_3D.hpp"
#include <string>
#include <functional>

#define CBSD_PORT 5681
#define CBSD_IP "0.0.0.0"

using namespace pmt;

class SC_Cbsd : public ScenarioController {

private:
  // internal members used by this CE
  int TCP_CORNET_Tutorial;

  //store previous values so we don't make unnecessary updates to the radios
  int old_mod;
  int old_crc;
  int old_fec0;
  int old_fec1;
  double old_freq;
  double old_bandwidth;
  double old_gain;
  int udp_server_sock;
  void parseStats(feedback_struct fs);
  void createParseSocket(int serverPort);
  pmt::pmt_t instruction;
  int udp_client_sock;
  struct sockaddr_in mailing_server_addr;

  std::string exec(const char *cmd);

  std::string get_ip();

  std::string get_mac();
  std::string ip;
  std::string mac;

  struct NodeParams{
    double rx_freq;
    double tx_freq;
    double bandwidth;
  };

  NodeParams myparams;

  unsigned short nodeID;
public:
  SC_Cbsd(int argc, char **argv);
  ~SC_Cbsd();
  virtual void execute();
  virtual void initialize_node_fb();
};

#endif
