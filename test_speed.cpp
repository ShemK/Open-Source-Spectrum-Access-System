#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <mqueue.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <arpa/inet.h>
#include <iomanip>
#include <zlib.h>
#include "tun.hpp"
#include <sys/ioctl.h>
#include <linux/if_tun.h>
#include <netinet/if_ether.h>
#include <queue>
#include <map>
#include <future>
#include <functional>
#include <cstring>
void *MAC_tx_worker(void *_arg);
#define PMODE 0777

#define MAX_BUF 2000
int main()
{

    mqd_t phy_tx_queue;
    mqd_t phy_rx_queue;

    struct mq_attr attr_tx;
    attr_tx.mq_maxmsg = 10;
    attr_tx.mq_msgsize = MAX_BUF;

    struct mq_attr attr_rx;
    attr_rx.mq_maxmsg = 10;
    attr_rx.mq_msgsize = MAX_BUF;

    phy_tx_queue = mq_open("/mac2phy", O_WRONLY | O_CREAT, PMODE, &attr_tx);
    phy_rx_queue = mq_open("/phy2mac", O_RDONLY | O_CREAT, PMODE, &attr_rx);
    int count = 0;
    while (count < 500)
    {
        std::string frame = "ksakfaksfkljsakfjkasjfkasf;lasl;fkoqpifwkqwu0fjpowqfpi[fwqop[qfwpwqfpwqkfoqwfowqfj;lfwqjilkqwfiljk...salk;kflas;lkfasl;sfkll;saflk;as;lfklasfl;saf;lkasf;lksalfkl;sakf";
        //std::cin >> frame;
        char buf[MAX_BUF];
        int status = mq_send(phy_tx_queue, frame.c_str(), frame.size(), 0);
        struct timespec timeout;
        timeout.tv_sec = time(NULL);
        timeout.tv_nsec = 1e3;

        status = mq_timedreceive(phy_rx_queue, buf, MAX_BUF, 0, &timeout);
       // usleep(100000);
        count++;
    }
}
