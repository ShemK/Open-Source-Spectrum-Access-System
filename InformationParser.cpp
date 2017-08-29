#include "InformationParser.hpp"

InformationParser::InformationParser() {
  udp_client_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  udp_server_addr.sin_family = AF_INET;
  udp_server_addr.sin_addr.s_addr = inet_addr("128.173.39.76");
  udp_server_addr.sin_port = htons(4680);
}

InformationParser::~InformationParser(){
  close(udp_client_sock);
}

void InformationParser::sendData(pmt::pmt_t info){
  std::string serialized_pmt =  pmt::serialize_str(info);
  int len = serialized_pmt.length();
  int p = sendto(udp_client_sock, serialized_pmt.c_str(),len,0,(struct sockaddr *)&udp_server_addr,sizeof(udp_server_addr));
  if(p < 0){
    printf("Connection Failed\n");
  }
}
