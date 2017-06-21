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
  MAC *mac = new MAC();
  mac->set_ip("10.0.0.3");
  int recv_buffer_len= 1000;
  char recv_buffer[recv_buffer_len];


  struct sockaddr_in udp_server_addr;
  memset(&udp_server_addr, 0, sizeof(udp_server_addr));
  udp_server_addr.sin_family = AF_INET;
  udp_server_addr.sin_addr.s_addr = inet_addr("10.0.0.3"); //10.0.0.3
  udp_server_addr.sin_port = htons(5000);
  socklen_t clientlen = sizeof(udp_server_addr);
  int udp_server_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  struct sockaddr_in udp_client_addr;
  memset(&udp_client_addr, 0, sizeof(udp_client_addr));
  udp_client_addr.sin_family = AF_INET;

  // Bind CRTS server socket
  bind(udp_server_sock, (sockaddr *)&udp_server_addr, clientlen); //10.0.0.3

  int packet_number = 0;
  fd_set read_fds;

  while(sig_terminate == 0) {
    int p = 0;
    int recv_len = 0;
    FD_ZERO(&read_fds);
    FD_SET(udp_server_sock, &read_fds);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 1000;

    p = select(udp_server_sock + 1, &read_fds, NULL, NULL, &timeout);
    if(p > 0) {
      printf("TUN Interface received bytes\n");
      int recv_len = recvfrom(udp_server_sock, recv_buffer, recv_buffer_len, 0,
                            (struct sockaddr *)&udp_client_addr, &clientlen);
      if (recv_len > 0) {

      }
    }

  }
  delete mac;
  pthread_exit(NULL);
}
