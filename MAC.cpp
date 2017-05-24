#include "MAC.hpp"


MAC::MAC(char *mac_address) {
  strncpy(this->mac_address,mac_address,sizeof(mac_address));
  tx_channel_state = FREE;

  // generate random_mac
  this->mac_address = random_byte_generator();
  struct mq_attr attr_tx;
  attr_tx.mq_maxmsg = 10;
  attr_tx.mq_msgsize = MAX_BUF;

  struct mq_attr attr_rx;
  attr_rx.mq_maxmsg = 10;
  attr_rx.mq_msgsize = MAX_BUF;


  phy_tx_queue = mq_open("/loopback", O_WRONLY|O_CREAT, PMODE, &attr_tx);
  phy_rx_queue = mq_open("/loopback", O_RDONLY|O_CREAT, PMODE, &attr_rx);

  if(phy_tx_queue  == -1 || phy_rx_queue  == -1 ) {
    perror("Failed to open message queue");
    exit(0);
  }

  pthread_mutex_init(&tx_mutex, NULL);        // transmitter mutex
  pthread_create(&tx_process, NULL, MAC_tx_worker, (void *)this);
  pthread_mutex_init(&rx_mutex, NULL);        // receiver mutex
  pthread_create(&rx_process, NULL, MAC_rx_worker, (void *)this);
  pthread_exit(NULL);
}


MAC::~MAC() {
  stop_rx = true;
  stop_tx = true;
  mq_close(phy_tx_queue);
  mq_close(phy_rx_queue);
  mq_unlink("/loopback");
  //  mq_unlink("/phy2mac");
}


void *MAC_tx_worker(void *_arg) {
  MAC *mac = (MAC *) _arg;
  std::string message;// = "Hello";
  char *frame = new char[MAX_BUF];

  while(!mac->stop_tx) {
    if(mac->tx_channel_state == mac->FREE) {
      std::getline(std::cin,message);
      strncpy(frame,message.c_str(),message.length());
      int frame_len = CONTROL_FRAME_LEN + message.length();
      std::cout << "Frame Len: " << strlen(frame) << "\n";
      mac->create_frame(frame,strlen(frame),mac->DATA,mac->RTS);
      pthread_mutex_lock(&mac->tx_mutex);
      int status = mq_send(mac->phy_tx_queue, frame, frame_len, 0);
      if (status == -1) {
        perror("mq_send failure\n");
      }
      else {
        printf("mq_send successful\n");
      }
      pthread_mutex_unlock(&mac->tx_mutex);
    }
    //usleep(200);
  }
  pthread_exit(NULL);
}


void MAC::create_frame(char *&data, int data_len,ProtocolType newType,
                ProtocolSubtype newSubType) {
  char *temp_frame = new char[data_len+CONTROL_FRAME_LEN];
  FrameControl temp;
  temp.frame_protocol_type = newType;
  temp.frame_protocol_subtype = newSubType;
  char * temp_control_frame = getControlFrame(temp);
  memcpy(temp_frame,temp_control_frame, CONTROL_FRAME_LEN);
  memcpy(temp_frame+CONTROL_FRAME_LEN,data,data_len);
  if(newType==MANAGEMENT) {

  } else if(newType == CONTROL){

  } else if(newType == DATA) {


  }
  data = temp_frame;
}

char * MAC::getControlFrame(FrameControl temp){
  char *control_frame = new char[CONTROL_FRAME_LEN];
  memset(control_frame,0,sizeof(control_frame));
  char *offset = control_frame;
  int temp_frame_control = htons(((temp.frame_protocol_type << 12) | (temp.frame_protocol_subtype << 8)));
  char *temp_offset = (char *)&temp_frame_control;
  memcpy(offset,temp_offset,2);
  printf("Control Frame: %x\n",control_frame[0]);
  offset = offset+2;
  memcpy(offset,mac_address,6);
  offset = offset + 6;
  return control_frame;
}


char *MAC::getMACHeader(char *frame) {
  char * temp = new char[CONTROL_FRAME_LEN];
  memcpy(temp,frame,CONTROL_FRAME_LEN);
  return temp;
}

char *MAC::extractSourceMAC(char *header) {
  char *temp = new char[6];
  memset(temp,0,6);

  memcpy(temp,header+2,6);
  return temp;
}

char *MAC::getPayLoad(char *frame, int payload_len) {
  return frame+CONTROL_FRAME_LEN;
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

      printf("Message Received\n");
      mac->recv_header = mac->getMACHeader(buf);
      mac->recv_payload = mac->getPayLoad(buf,status);
      char *sourceMAC = mac->extractSourceMAC(mac->recv_header);
      if(strncmp(mac->mac_address,sourceMAC,6)!=0) {
          mac->tx_channel_state = mac->BUSY;
      }
      printf("%s\n", mac->recv_payload);
      printf("------------------------------------\n");
      memset(buf,0,MAX_BUF);
    }

  }
  pthread_exit(NULL);
}


char *random_byte_generator() {
	char *random_bytes = new char[6];
	time_t  currentTime= time(NULL);
	srand((unsigned) currentTime);
	for(int i = 0; i < 6; i++) {
		*(random_bytes+i) = rand()%256;
	}
	return random_bytes;
}
