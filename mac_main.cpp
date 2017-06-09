#include "MAC.hpp"
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <signal.h>

int sig_terminate = 0;
void terminate(int signum) {
  printf("\nSending termination message to controller\n");
  sig_terminate = 1;
}

int main() {

  // register signal handlers
  signal(SIGINT, terminate);
  signal(SIGQUIT, terminate);
  signal(SIGTERM, terminate);
  MAC *mac = new MAC((char*)"AAAAAA");
  int buffer_length = 1000;
  char message[buffer_length];
  std::string input;
  struct sockaddr_in udp_server_addr;
  memset(&udp_server_addr, 0, sizeof(udp_server_addr));
  udp_server_addr.sin_family = AF_INET;
  udp_server_addr.sin_addr.s_addr = inet_addr("10.0.0.3"); //10.0.0.3
  udp_server_addr.sin_port = htons(5000);

  struct sockaddr_in udp_client_addr;
  memset(&udp_client_addr, 0, sizeof(udp_client_addr));
  udp_client_addr.sin_family = AF_INET;
  udp_client_addr.sin_addr.s_addr = inet_addr("10.0.0.2"); //10.0.0.2
  udp_client_addr.sin_port = htons(8000);
  socklen_t clientlen = sizeof(udp_client_addr);


  int udp_client_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  int packet_number = 0;
  while(sig_terminate == 0) {
    std::getline(std::cin,input);
    packet_number = input.length();
    strncpy(message,input.c_str(),packet_number);
  //  printf("Hello %d\n",packet_number);
    int send_return = sendto(udp_client_sock, (char *)message, packet_number, 0,
    (struct sockaddr *)&udp_server_addr, sizeof(udp_server_addr));
  //  printf("Sent %d\n",send_return);
  }
  delete mac;
  pthread_exit(NULL);
}
