#include <iostream>
#include <string>
#include <pmt/pmt.h>
#include <vector>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace pmt;

class InformationParser{

private:
  int udp_client_sock;
  struct sockaddr_in udp_server_addr;
public:
  InformationParser();
  ~InformationParser();
  void sendData(pmt::pmt_t info);
};
