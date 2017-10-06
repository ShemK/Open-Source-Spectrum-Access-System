#include "CentralRemConnector.hpp"
#include <signal.h>
#include <netinet/in.h>
#include <linux/if_tun.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/socket.h>
//g++ -o test test.cpp CentralRemConnector.cpp decision_maker.cc InformationParser.cpp -lpqxx -lgnuradio-pmt -std=c++11

int sig_terminate = 0;

void terminate(int signum)
{
    printf("\nCtr+D Pressed - Exiting\n");
    sig_terminate = 1;
}

int main()
{
    // register signal handlers
    signal(SIGINT, terminate);
    signal(SIGQUIT, terminate);
    signal(SIGTERM, terminate);

    CentralRemConnector db_connector("rem", "wireless", "wireless", "127.0.0.1");
    db_connector.connect();

    struct sockaddr_in udp_server_addr;
    struct sockaddr_in udp_client_addr;
    socklen_t addr_len = sizeof(udp_server_addr);
    memset(&udp_server_addr, 0, addr_len);
    udp_server_addr.sin_family = AF_INET;
    udp_server_addr.sin_addr.s_addr = inet_addr("0.0.0.0");
    udp_server_addr.sin_port = htons(6000);
    int udp_server_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    int status = bind(udp_server_sock, (sockaddr *)&udp_server_addr, addr_len);
    printf("Status: %d\n",status);
    if(status != -1){
      int recv_buffer_len = 2000;
      char recv_buffer[recv_buffer_len];
      fd_set read_fds;
      while (sig_terminate == 0)
      {
          FD_ZERO(&read_fds);
          FD_SET(udp_server_sock, &read_fds);
          struct timeval timeout;
          timeout.tv_sec = 0;
          timeout.tv_usec = 1000;
          if (select(udp_server_sock + 1, &read_fds, NULL, NULL, &timeout) > 0)
          {
              int recv_len = recvfrom(udp_server_sock, recv_buffer, recv_buffer_len, 0,
                                      (struct sockaddr *)&udp_client_addr, &addr_len);
              db_connector.analyze((const char *)recv_buffer, recv_len);
          }
      }
    close(udp_server_sock);
  } else{
    printf("Failed to bind to Port\n");
  }


}
