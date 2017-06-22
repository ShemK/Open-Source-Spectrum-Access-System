#include "MAC.hpp"
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <signal.h>

int sig_terminate = 0;
void terminate(int signum)
{
  printf("\nSending termination message to controller\n");
  sig_terminate = 1;
}

int main()
{

  // register signal handlers
  signal(SIGINT, terminate);
  signal(SIGQUIT, terminate);
  signal(SIGTERM, terminate);
  MAC *mac = new MAC();
  mac->set_ip("10.0.0.2");
  int buffer_length = 1500;
  char message[buffer_length];
  char recv_buffer[buffer_length];
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
  while (sig_terminate == 0)
  {
    std::getline(std::cin, input);
    packet_number = input.length();
    strncpy(message, input.c_str(), packet_number);
    //  printf("Hello %d\n",packet_number);
    int send_return = sendto(udp_client_sock, (char *)message, packet_number, 0,
                             (struct sockaddr *)&udp_server_addr, sizeof(udp_server_addr));
    //  printf("Sent %d\n",send_return);

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(udp_client_sock, &read_fds);
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 1000;

    int p = select(udp_client_sock + 1, &read_fds, NULL, NULL, &timeout);
    if (p > 0)
    {
      int recv_len = recvfrom(udp_client_sock, recv_buffer,  buffer_length, 0,
                          (struct sockaddr *)&udp_server_addr, &clientlen);
      for (int i = 0; i < recv_len; i++)
      {
        std::cout << recv_buffer[i];
      }
      printf("\n");
    }
  }
  delete mac;
  pthread_exit(NULL);
}
