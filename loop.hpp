#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <mqueue.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <arpa/inet.h>
#include <iomanip>
#include <zlib.h>
#include <sys/ioctl.h>
#include <linux/if_tun.h>
#include <netinet/if_ether.h>
#include <queue>
#include <map>
#include <future>
#include <functional>
#include <cstring>
#include <bitset>
#include <complex>
#include <semaphore.h>
#include <sys/mman.h>
#include "BufferQ.h"

#define RED "\x1B[31m"
#define GRN "\x1B[32m"
#define YEL "\x1B[33m"
#define BLU "\x1B[34m"
#define MAG "\x1B[35m"
#define CYN "\x1B[36m"
#define WHT "\x1B[37m"
#define RESET "\x1B[0m"

typedef unsigned short uint16;

#define MAX_SHARED_BUFFER 50000
#define PMODE 0777

class Loop
{
private:
public:
  Loop();
  ~Loop();
  sem_t mutex;

  int fd_shm;

  struct shared_struct{
    int sample = 0;
    double frequency = 0;
    double sample_rate = 0;
    int num_samples = 0;
    std::complex<float> data[MAX_SHARED_BUFFER];
  };

  BufferQ<std::complex<float>> *theQ;//((int)100,1);

  shared_struct *shared;

  void transmit(std::vector<std::complex<float>> usrp_buffer,int buffer_len);
  int receive(std::vector<std::complex<float> > &recv);
  void manipulate_header(char *payload);
};
