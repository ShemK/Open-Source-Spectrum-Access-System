#include "MAC.hpp"

MAC::MAC(char *mac_address)
{
  strncpy(this->mac_address, mac_address, sizeof(mac_address));
  tx_channel_state = FREE;
  frames_received = 0;
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


/*
//  Transmission worker thread
//  TODO:: Get signals on when to transmit or not
//  TODO:: Check with the receiver first before Transmission
//  TODO:: Get
*/
void *MAC_tx_worker(void *_arg)
{
  
  int frames_sent = 0;
  MAC *mac = (MAC *)_arg;
  std::string message = "";
  char *frame = new char[MAX_BUF];
  int frame_num = 0;
  bool new_transmission = true;
  mac->start_time = time(NULL);
  while (!mac->stop_tx)
  {
    if (mac->tx_channel_state == mac->FREE)
    {
      int tmp_len = MTU;
      message = "";
      bool last_segment = false;
      //std::getline(std::cin, message);
      if(frame_num%10 == 0){
        for(int i = 0; i < tmp_len ; i++){
          message = message + "1";
        }
      } else{
        for(int i = 0; i < tmp_len ; i++){
          message = message + "0";
        }
      }
      strncpy(frame, message.c_str(), message.length());
      int segment_len = message.length();
      // check ip flags set
      if(mac->isLastsegment(frame)) {
        last_segment = true;
      } else{
        last_segment = false;
      }

      int frame_len = segment_len + CONTROL_FRAME_LEN;

      mac->create_frame(frame, strlen(frame), mac->DATA, mac->RTS);
      pthread_mutex_lock(&mac->tx_mutex);
   
     
      unsigned int conv_frame_num = htonl(frame_num);
      memcpy(frame+8,&conv_frame_num,4);
      
      // FIXME:: Need to look into what state the MAC should be in
      // if only one segment is received
      if(last_segment){
        memset(frame + 1, 1, 1);
        printf("Last Frame: %d\n", frame[1]);
        if(new_transmission){
          //printf("New Transmission1\n");
          mac->myState = mac->BACKING_OFF;
          mac->backOff();
         // mac->myState = mac->TRANSMITTING;
        } else{
          //printf("Transmission Continuation1\n");
          new_transmission = true;
          mac->myState = mac->IDLE;
        }
      } else{
        if(new_transmission){
        //  printf("New Transmission2\n");
          mac->myState = mac->BACKING_OFF;
          mac->backOff();
          mac->myState = mac->TRANSMITTING;
        }else {
          //printf("Transmission Continuation2\n");
        }
        new_transmission = false;
      }
      mac->addCRC(frame,frame_len);
      std::cout << "Frame Len: " << frame_len << "\n";
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
      memset(frame, 0, MAX_BUF);
    }
    else
    {
      //printf("TX Channel Busy\n");
    }
   // usleep(1000);
  }
  pthread_exit(NULL);
}

/*
//  Combines the control_frame(header) with the payload
//  TODO:: Type of frames to be used
*/

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
  memcpy(data,temp_frame,data_len + CONTROL_FRAME_LEN);
  //data = temp_frame;
}
/*  Creates a control frame header for any payload to be transmitted
**  TODO:: Need to improve on which type of information is needed in
**         the control frame
*/

char *MAC::getControlFrame(FrameControl temp)
{
  char *control_frame = new char[CONTROL_FRAME_LEN];
  memset(control_frame, 0, sizeof(control_frame));
  char *offset = control_frame;
  int temp_frame_control = htons(((temp.frame_protocol_type << 12) | (temp.frame_protocol_subtype << 8)));
  char *temp_offset = (char *)&temp_frame_control;
  memcpy(offset, temp_offset, 2); // TODO:: test by removing temp_offset. temp_offset provides a frame_protocol_type
  //printf("Control Frame: %x\n", control_frame[0]);
  offset = offset + 2;
  memcpy(offset, mac_address, 6); // add mac address to the control_frame
  offset = offset + 6;

  return control_frame;
}

// extracts control_frame(mac header) from any given payload
// control_frame size will always remain constant as specified by CONTROL_FRAME_LEN
char *MAC::getMACHeader(char *frame)
{
  char *temp = new char[CONTROL_FRAME_LEN];
  memcpy(temp, frame, CONTROL_FRAME_LEN);
  return temp;
}

// Assumes that the MAC address of the source of the payload is 2 bytes
// from the start of the header
// TODO:: need to create a variable for the position of the mac header

char *MAC::extractSourceMAC(char *header)
{
  char *temp = new char[6];
  memset(temp, 0, 6);

  memcpy(temp, header + 2, 6);
  return temp;
}
//
//  Extracts payload at position right after the control frame length
//
char *MAC::getPayLoad(char *frame, int payload_len)
{
  return frame + CONTROL_FRAME_LEN;
}


// The worker for the receiver thread
void *MAC_rx_worker(void *_arg)
{
  int frames_received = 0;
  MAC *mac = (MAC *)_arg;
  char buf[MAX_BUF];
  while (!mac->stop_rx)
  {
    //mac->tx_channel_state = mac->FREE;
    struct timespec timeout;
    timeout.tv_sec = time(NULL)+1;
    timeout.tv_nsec = 0;

    int status = mq_timedreceive(mac->phy_rx_queue, buf, MAX_BUF, 0, &timeout);

    if (status == -1)
    {
      if (errno == ETIMEDOUT)
      {
        if (mac->tx_channel_state != mac->UNAVAILABLE)
        {
          if(mac->myState == mac->IDLE){
            mac->tx_channel_state = mac->FREE;
          }
          //printf("Channel Free\n");
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
      if(mac->isCorrectCRC(buf,status)){
     // pthread_mutex_lock(&mac->rx_mutex);
        mac->analyzeReceivedFrame(buf,status);
     // pthread_mutex_unlock(&mac->rx_mutex);
      } else{
        printf("Wrong Packet due to wrong CRC\n");
      }
      memset(buf, 0, MAX_BUF);
    }

  }
  pthread_exit(NULL);
}

// Analyze any received frame to make decisions on the state of the channel
void MAC::analyzeReceivedFrame(char *buf, int buf_len){
  printf("Message Received\n");
  recv_header = getMACHeader(buf);
  recv_payload = getPayLoad(buf, buf_len);
  char *sourceMAC = extractSourceMAC(recv_header);
  printf("Source MAC Address: ");
  double frame_error_rate = 0;
  for (int i = 0; i < 6; i++)
  {
    printf("%02x:", (unsigned char)sourceMAC[i]);
  }
  printf("\n");

  if (strncmp(mac_address, sourceMAC, 6) != 0)
  {
    if(!isLastAlienFrame(recv_header)){
      tx_channel_state = BUSY;
      printf("Channel Busy %u\n", strncmp(mac_address, sourceMAC, 6));
    } else{
      tx_channel_state = FREE;
      printf("Channel free %u\n", strncmp(mac_address, sourceMAC, 6));
    }
  }
  else
  {
    if(myState == TRANSMITTING) {
      tx_channel_state = FREE;
    }
    int frame_num = buffToInteger(recv_header+8);
    frames_received++;
    frame_error_rate = (frame_num+1 - (float)frames_received)/((float)frame_num+1);
    int time_dif = (time(NULL) - start_time);
    int bitrate = 0;
    if(time_dif > 0) {
      bitrate = (buf_len*8)*frames_received/time_dif;
    }
    if(isCorrectCRC(buf,buf_len)){
      printf("CRC Check: Passed\n");
    } else{
      printf("CRC Check: Failed\n");
    }
    printf("Frame_num received: %d\n", frame_num);
    printf("Frames received: %d\n",frames_received);
    std::cout << std::fixed;
    std::cout << "Frame Error Rate: " << 
            std::setprecision(5) << frame_error_rate << '\n';
    printf("Bitrate: %d\n",bitrate);
  //  printf("%s\n", recv_payload);
    printf("------------------------------------\n");
  }
}


/*
// check if the tcp segment is the last segment
*/

bool MAC::isLastsegment(char *segment) {
  unsigned char flag;
  memcpy(&flag,segment+IP_HEADER_LEN+IP_FLAG_POS,1);
  return (isBitSet(flag,0) | isBitSet(flag,1)
            | isBitSet(flag,2) |isBitSet(flag,3));
}

bool MAC::isLastAlienFrame(char *frame){
  if(frame[1] == 1){
    return true;
  } else{
    return false;
  }
}

void MAC::backOff(){
  for(int i = 0; i < cw; i++){
    if(tx_channel_state==BUSY){
      if(i > 0) {
        i--;
     //   printf("BACK OFF PAUSED \n");
      }
    } else{
     // printf("BACK OFF RESUMED \n");
      usleep(SLOT_TIME);
    }
  }
  myState = IDLE;
}

// HACK:: Little Endian and Big Endian issues with CRC
void MAC::addCRC(char *frame, int &frame_len) {
  unsigned long crc = crc32(0L,Z_NULL,0);
  crc = crc32(crc,(const unsigned char *)frame,frame_len);
  memcpy(frame+frame_len,&crc,4);
  printf("CRC sent: %lu\n",crc);
  frame_len = frame_len+4;
}


bool MAC::isCorrectCRC(char *buf, int buf_len){
  unsigned long crc = crc32(0L,Z_NULL,0);
  crc = crc32(crc,(const unsigned char *)buf,buf_len-4);
 // printf("CRC Received: %lu\n",crc);
  unsigned long received_crc = htonl((unsigned long) buffToInteger(buf+buf_len-4));
  //printf("CRC Received 2: %lu\n",received_crc);
  if(crc==received_crc){
    return true;
  } else{
    return false;
  }
}
//
// Generate random_bytes for the MAC. Probability of using the same value = (1/256)^6
//
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

//
//  converts the received interger values in the frame
//
int buffToInteger(char * buffer)
{
  // from https://stackoverflow.com/questions/34943835/convert-four-bytes-to-integer-using-c
    int a = static_cast<int>(static_cast<unsigned char>(buffer[0]) << 24 |
        static_cast<unsigned char>(buffer[1]) << 16 |
        static_cast<unsigned char>(buffer[2]) << 8 |
        static_cast<unsigned char>(buffer[3]));
    return a;
}
//
bool isBitSet (unsigned char c, int n) {
  return (1 & (c >> n));
}
