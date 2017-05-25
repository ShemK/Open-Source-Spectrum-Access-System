#include "MAC.hpp"

MAC::MAC(char *mac_address)
{ 
  strncpy(this->mac_address, mac_address, sizeof(mac_address));
  tx_channel_state = FREE;
  // generate random_mac
  this->mac_address = random_byte_generator();
  printf("My MAC Address: ");
  for (int i = 0; i < 6; i++)
  {
    printf("%02x:", (unsigned char)this->mac_address[i]);
  }
  printf("\n");
  struct mq_attr attr_tx;
  attr_tx.mq_maxmsg = 10;
  attr_tx.mq_msgsize = MAX_BUF;

  struct mq_attr attr_rx;
  attr_rx.mq_maxmsg = 10;
  attr_rx.mq_msgsize = MAX_BUF;

  phy_tx_queue = mq_open("/mac2phy", O_WRONLY | O_CREAT, PMODE, &attr_tx);
  phy_rx_queue = mq_open("/phy2mac", O_RDONLY | O_CREAT, PMODE, &attr_rx);

  if (phy_tx_queue == -1 || phy_rx_queue == -1)
  {
    perror("Failed to open message queue");
    exit(0);
  }

  pthread_mutex_init(&tx_mutex, NULL); // transmitter mutex
  pthread_create(&tx_process, NULL, MAC_tx_worker, (void *)this);
  pthread_mutex_init(&rx_mutex, NULL); // receiver mutex
  pthread_create(&rx_process, NULL, MAC_rx_worker, (void *)this);
  pthread_exit(NULL);
}

MAC::~MAC()
{
  stop_rx = true;
  stop_tx = true;
  mq_close(phy_tx_queue);
  mq_close(phy_rx_queue);
  mq_unlink("/mac2phy");
  mq_unlink("/phy2mac");
}

void *MAC_tx_worker(void *_arg)
{
  int frames_sent = 0;
  MAC *mac = (MAC *)_arg;
  std::string message = "Hello";
  char *frame = new char[MAX_BUF];
  int frame_num = 0;
  while (!mac->stop_tx)
  {
    if (mac->tx_channel_state == mac->FREE)
    {
      //std::getline(std::cin, message);
      strncpy(frame, message.c_str(), message.length());
      int frame_len = message.length() + CONTROL_FRAME_LEN;
   
      mac->create_frame(frame, strlen(frame), mac->DATA, mac->RTS);
      pthread_mutex_lock(&mac->tx_mutex);
      memset(frame + 1, frame_num, 1);
      unsigned int conv_frame_num = htonl(frame_num);
      memcpy(frame+8,&conv_frame_num,4);
         std::cout << "Frame Len: " << strlen(frame) << "\n";
      int status = mq_send(mac->phy_tx_queue, frame, frame_len, 5);
      if (status == -1)
      {
        perror("mq_send failure\n");
      }
      else
      {
        printf("mq_send successful with frame_num: %d\n", frame_num );
        frame_num++;
        frames_sent++;
        printf("Frames Sent: %d\n",frames_sent);
      }
      pthread_mutex_unlock(&mac->tx_mutex);
      memset(frame, 0, frame_len);
    }
    else
    {
      printf("Channel Busy\n");
    }
    //usleep(200);
  }
  pthread_exit(NULL);
}

void MAC::create_frame(char *&data, int data_len, ProtocolType newType,
                       ProtocolSubtype newSubType)
{
  char *temp_frame = new char[data_len + CONTROL_FRAME_LEN];
  FrameControl temp;
  temp.frame_protocol_type = newType;
  temp.frame_protocol_subtype = newSubType;
  char *temp_control_frame = getControlFrame(temp);
  memcpy(temp_frame, temp_control_frame, CONTROL_FRAME_LEN);
  memcpy(temp_frame + CONTROL_FRAME_LEN, data, data_len);
  if (newType == MANAGEMENT)
  {
  }
  else if (newType == CONTROL)
  {
  }
  else if (newType == DATA)
  {
  }
  data = temp_frame;
}

char *MAC::getControlFrame(FrameControl temp)
{
  char *control_frame = new char[CONTROL_FRAME_LEN];
  memset(control_frame, 0, sizeof(control_frame));
  char *offset = control_frame;
  int temp_frame_control = htons(((temp.frame_protocol_type << 12) | (temp.frame_protocol_subtype << 8)));
  char *temp_offset = (char *)&temp_frame_control;
  memcpy(offset, temp_offset, 2);
  printf("Control Frame: %x\n", control_frame[0]);
  offset = offset + 2;
  memcpy(offset, mac_address, 6);
  offset = offset + 6;

  return control_frame;
}

char *MAC::getMACHeader(char *frame)
{
  char *temp = new char[CONTROL_FRAME_LEN];
  memcpy(temp, frame, CONTROL_FRAME_LEN);
  return temp;
}

char *MAC::extractSourceMAC(char *header)
{
  char *temp = new char[6];
  memset(temp, 0, 6);

  memcpy(temp, header + 2, 6);
  return temp;
}

char *MAC::getPayLoad(char *frame, int payload_len)
{
  return frame + CONTROL_FRAME_LEN;
}

void *MAC_rx_worker(void *_arg)
{
  int frames_received = 0;
  MAC *mac = (MAC *)_arg;
  char buf[MAX_BUF];
  while (!mac->stop_rx)
  {
    timespec timeout;
    timeout.tv_sec = 0;
    timeout.tv_nsec = 500;
    int status = mq_timedreceive(mac->phy_rx_queue, buf, MAX_BUF, 0, &timeout);
    if (status == -1)
    {
      if (errno == ETIMEDOUT)
      {
        mac->myState = mac->IDLE;
        if (mac->tx_channel_state != mac->UNAVAILABLE)
        {
          mac->tx_channel_state = mac->FREE;
          // printf("Channel Free\n");
        }
      }
      else
      {
        perror("Failed to read queue");
        exit(0);
      }
    }
    else
    {

      printf("Message Received\n");
      mac->recv_header = mac->getMACHeader(buf);
      mac->recv_payload = mac->getPayLoad(buf, status);
      char *sourceMAC = mac->extractSourceMAC(mac->recv_header);
      printf("Source MAC Address: ");

      for (int i = 0; i < 6; i++)
      {
        printf("%02x:", (unsigned char)sourceMAC[i]);
      }
      printf("\n");

      if (strncmp(mac->mac_address, sourceMAC, 6) != 0)
      {
        mac->tx_channel_state = mac->BUSY;
        printf("Channel Busy %u\n", strncmp(mac->mac_address, sourceMAC, 6));
      }
      else
      {
        int frame_num = buffToInteger(mac->recv_header+8);
        frames_received++;
        printf("Frame_num received: %d\n", frame_num);
        printf("Frames received: %d\n",frames_received);
        printf("%s\n", mac->recv_payload);
        printf("------------------------------------\n");
      }

      memset(buf, 0, MAX_BUF);
    }
  }
  pthread_exit(NULL);
}

char *random_byte_generator()
{
  char *random_bytes = new char[6];
  time_t currentTime = time(NULL);
  srand((unsigned)currentTime);
  for (int i = 0; i < 6; i++)
  {
    *(random_bytes + i) = rand() % 256;
  }
  return random_bytes;
}

int buffToInteger(char * buffer)
{
  // from https://stackoverflow.com/questions/34943835/convert-four-bytes-to-integer-using-c
    int a = static_cast<int>(static_cast<unsigned char>(buffer[0]) << 24 |
        static_cast<unsigned char>(buffer[1]) << 16 | 
        static_cast<unsigned char>(buffer[2]) << 8 | 
        static_cast<unsigned char>(buffer[3]));
    return a;
}