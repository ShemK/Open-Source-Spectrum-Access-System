#include "MAC.hpp"
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <signal.h>

#define DEBUG 0
#if DEBUG == 1
#define dprintf(...) printf(__VA_ARGS__)
#else
#define dprintf(...) /*__VA_ARGS__*/
#endif

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
  std::string ip = "10.0.0.1";
  mac->set_ip(ip.c_str());
  int recv_buffer_len= 1500;
  char recv_buffer[recv_buffer_len];
  char message[recv_buffer_len];

  struct sockaddr_in udp_server_addr;
  memset(&udp_server_addr, 0, sizeof(udp_server_addr));
  udp_server_addr.sin_family = AF_INET;
  udp_server_addr.sin_addr.s_addr = inet_addr(ip.c_str()); 
  udp_server_addr.sin_port = htons(5000);
  socklen_t clientlen = sizeof(udp_server_addr);
  int udp_server_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  struct sockaddr_in udp_client_addr;
  memset(&udp_client_addr, 0, sizeof(udp_client_addr));
  udp_client_addr.sin_family = AF_INET;

  // Bind CRTS server socket
  bind(udp_server_sock, (sockaddr *)&udp_server_addr, clientlen); 

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
        dprintf("Received packet containing %i bytes:\n",recv_len);
        char client_ip[20];
        strncpy(client_ip,inet_ntoa(udp_client_addr.sin_addr),20);
        printf("Message Received from %s: %s",client_ip,recv_buffer);
        for(int i = 0; i < recv_len; i++) {
          //std::cout << recv_buffer[i];
        }
        std::string reply = "Message received";
        strncpy(message,reply.c_str(),reply.length());
        int message_length = reply.length();
        sendto(udp_server_sock,(char *)message, message_length, 0,
                  (struct sockaddr *)&udp_client_addr, sizeof(udp_client_addr));
        printf("\n");
      }
    }

  }
  delete mac;
  pthread_exit(NULL);
}
