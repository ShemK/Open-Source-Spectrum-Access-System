#include "MAC.hpp"


MAC::MAC(char *mac_address) {
  strncpy(this->mac_address,mac_address,sizeof(mac_address));
  tx_channel_state = FREE;

  struct mq_attr attr_tx;
  attr_tx.mq_maxmsg = 10;
  attr_tx.mq_msgsize = MAX_BUF;

  struct mq_attr attr_rx;
  attr_rx.mq_maxmsg = 10;
  attr_rx.mq_msgsize = MAX_BUF;


  phy_tx_queue = mq_open("/mac2phy", O_WRONLY|O_CREAT, PMODE, &attr_tx);
  phy_rx_queue = mq_open("/phy2mac", O_RDONLY|O_CREAT, PMODE, &attr_rx);

  if(phy_tx_queue  == -1 || phy_rx_queue  == -1 ) {
    perror("Failed to open message queue");
    exit(0);
  }

  pthread_mutex_init(&tx_mutex, NULL);        // transmitter mutex
  pthread_create(&tx_process, NULL, MAC_tx_worker, (void *)this);
  pthread_mutex_init(&rx_mutex, NULL);        // receiver mutex
//  pthread_create(&rx_process, NULL, MAC_rx_worker, (void *)this);
  pthread_exit(NULL);
}


MAC::~MAC() {
  stop_rx = true;
  stop_tx = true;
  mq_close(phy_tx_queue);
  mq_close(phy_rx_queue);
  mq_unlink("/loopback");
  //mq_unlink("/phy2mac");
}


void *MAC_tx_worker(void *_arg) {
  MAC *mac = (MAC *) _arg;
  std::string message;
  char *max_message = new char[MAX_BUF];
  while(!mac->stop_tx) {
    if(mac->tx_channel_state == mac->FREE) {
      std::getline(std::cin,message);
      strncpy(max_message,message.c_str(),sizeof(message));

      pthread_mutex_lock(&mac->tx_mutex);

      int status = mq_send(mac->phy_tx_queue, message.c_str(), strlen(message.c_str())+1, 0);
      if (status == -1) {
        perror("mq_send failure\n");
      }
      else {
        printf("mq_send successful\n");
      }
      pthread_mutex_unlock(&mac->tx_mutex);
    }
  }
  pthread_exit(NULL);
}


void *MAC_rx_worker(void *_arg) {
  MAC *mac = (MAC *) _arg;
  char buf[MAX_BUF];
  while(!mac->stop_rx) {
    timespec timeout;
    timeout.tv_sec = 0;
    timeout.tv_nsec = 100;
    int status = mq_timedreceive(mac->phy_rx_queue, buf, MAX_BUF, 0,&timeout);
    if(status == -1) {
      if(errno == ETIMEDOUT) {
        mac->myState = mac->IDLE;
        if(mac->tx_channel_state != mac->UNAVAILABLE) {
          mac->tx_channel_state = mac->FREE;
        }
      }
      else{
        perror("Failed to read queue");
        exit(0);
      }
    }
    else {
      mac->tx_channel_state = mac->BUSY;
      printf("Message Received\n");
      printf("%s\n", buf);
      printf("------------------------------------\n");
    }
  }
  pthread_exit(NULL);
}
