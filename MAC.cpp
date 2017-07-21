#include "MAC.hpp"
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>

#define DEBUG 0
#if DEBUG == 1
#define dprintf(...) printf(__VA_ARGS__)
#else
#define dprintf(...) /*__VA_ARGS__*/
#endif

MAC::MAC()
{
  dprintf("Creating tun interface\n");
  // Create TUN interface
  dprintf("Creating tun interface\n");
  strcpy(tun_name, "mac_interface");

  sprintf(systemCMD, "sudo ip tuntap add dev %s mode tap", tun_name);
  system(systemCMD);
  dprintf("Bringing up tun interface\n");
  dprintf("Connecting to tun interface\n");
  sprintf(systemCMD, "sudo ip link set dev %s up", tun_name);
  system(systemCMD);

  memset(prev_packet, 0, MAX_BUF);
  // Get reference to TUN interface
  tunfd = tun_alloc(tun_name, IFF_TAP);

  struct mq_attr attr_tx;
  attr_tx.mq_maxmsg = 10;
  attr_tx.mq_msgsize = MAX_BUF;

  struct mq_attr attr_rx;
  attr_rx.mq_maxmsg = 10;
  attr_rx.mq_msgsize = MAX_BUF;

  phy_tx_queue = mq_open("/mac2phy", O_WRONLY | O_CREAT, PMODE, &attr_tx);
  phy_rx_queue = mq_open("/phy2mac", O_RDONLY | O_CREAT, PMODE, &attr_rx);

  //phy_tx_queue = mq_open("/loopback", O_WRONLY | O_CREAT, PMODE, &attr_tx);
  //phy_rx_queue = mq_open("/loopback", O_RDONLY | O_CREAT, PMODE, &attr_rx);
  if (phy_tx_queue == -1 || phy_rx_queue == -1)
  {
    perror("Failed to open message queue");
    exit(0);
  }

  // set any banned mac addresses
  set_banned_addresses();

  // initialize random seed
  srand(time(NULL));
  pthread_mutex_init(&tx_mutex, NULL); // transmitter mutex
  pthread_create(&tx_process, NULL, MAC_tx_worker, (void *)this);
  pthread_mutex_init(&rx_mutex, NULL); // receiver mutex
  pthread_create(&rx_process, NULL, MAC_rx_worker, (void *)this);
}

MAC::~MAC()
{
  printf("Closing MAC\n");
  stop_rx = true;
  stop_tx = true;
  mq_close(phy_tx_queue);
  mq_close(phy_rx_queue);
  //mq_unlink("/mac2phy");
  //mq_unlink("/phy2mac");
  // close the TUN interface file descriptor
  dprintf("destructor closing the TUN interface file descriptor\n");
  close(tunfd);

  dprintf("destructor bringing down TUN interface\n");
  sprintf(systemCMD, "sudo ip link set dev %s down", tun_name);
  system(systemCMD);

  dprintf("destructor deleting TUN interface\n");
  sprintf(systemCMD, "sudo ip tuntap del dev %s mode tap", tun_name);
  system(systemCMD);

  peerlist.clear();
}

void MAC::set_ip(const char *ip)
{
  sprintf(systemCMD, "sudo ifconfig %s %s netmask 255.255.0.0", tun_name, ip);
  system(systemCMD);
  sprintf(systemCMD, "sudo ifconfig %s txqueuelen %s", tun_name, "15000");
  system(systemCMD);
  std::string mac_str = exec("ifconfig mac_interface | grep HWaddr | awk '{print $5}'");
  for (uint idx = 0; idx < 6; ++idx)
  {
    mac_address[idx] = hex_digit(mac_str[3 * idx]) << 4;
    mac_address[idx] |= hex_digit(mac_str[1 + 3 * idx]);
  }
  tx_channel_state = FREE;
  frames_received = 0;
  memset(broadcast_address, 255, 6);
}

/*
//  Transmission worker thread
//  TODO:: Get signals on when to transmit or not
//  TODO:: Check with the receiver first before Transmission
//  TODO:: Get
*/
void *MAC_tx_worker(void *_arg)
{
  MAC *mac = (MAC *)_arg;
  std::string message = "";
  // char buffer[MAX_BUF];
  // int buffer_len = MAX_BUF;
  char *frame = new char[MAX_BUF];
  // int frame_num = 0;
  int nread = 0;
  while (!mac->stop_tx)
  {

    fd_set fds;
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 1000;

    FD_ZERO(&fds);
    FD_SET(mac->tunfd, &fds);
    if (nread == 0)
    {
      // check if anything is available on the TUN interface
      if (select(mac->tunfd + 1, &fds, NULL, NULL, &timeout) > 0)
      {
        //    printf("Reading Data from tun interface\n");
        // grab data from TUN interface
        nread += cread(mac->tunfd, (char *)(&frame[nread]), MAX_BUF);
        if (nread < 0)
        {
          dprintf(RED "Error reading from TUN interface\n" RESET);
          close(mac->tunfd);
          exit(EXIT_FAILURE);
        }
        else
        {
          /*
          dprintf("%d bytes ready for transmission\n", nread);
          for (int i = 0; i < nread; i++)
          {
            //dprintf("%x ", frame[i]);
          }
          dprintf("\n");
          */
          // dprintf(YEL "Packet Received from tun interface\n" RESET);
          uint16 ether_type = -1;
          memcpy(&ether_type, frame + 2, sizeof(ether_type));
          ether_type = htons(ether_type);
          MAC::IpSegment new_segment;
          char *peer_address = mac->extractDestinationMAC(frame);
          switch (ether_type)
          {
          case ETH_P_ARP:
            dprintf(YEL "ARP Packet\n" RESET);
            dprintf(YEL "Peer List Size: %lu\n" RESET, mac->peerlist.size());
            //new_segment.frame_num = mac->updatePeerTxStatistics(peer_address);
            memset(new_segment.segment, 0, nread);
            new_segment.size = nread;
            memcpy(new_segment.segment, frame, nread);
            new_segment.arp_packet = true;
            mac->ip_tx_queue.push(new_segment);
            memset(frame, 0, nread);
            break;
          case ETH_P_IP:
            if (!mac->isAddressBanned(peer_address))
            {
              new_segment.frame_num = mac->updatePeerTxStatistics(peer_address);
              printf(YEL "IP Packet with Frame Number: %d for Peer: ", new_segment.frame_num);
              for (int i = 0; i < 6; i++)
              {
                printf("%02x:", (unsigned char)peer_address[i]);
              }
              printf("\n" RESET);
              memset(new_segment.segment, 0, nread);
              new_segment.size = nread;
              memcpy(new_segment.segment, frame, nread);
              memset(frame, 0, nread);
              new_segment.arp_packet = false;
              mac->ip_tx_queue.push(new_segment);
            }
            break;
          default:
            dprintf(YEL "Unknown Packet\n" RESET);
            break;
          }
          nread = 0;
          delete[] peer_address;
        }
      }
      if (mac->ip_tx_queue.size() > 0)
      {
        if (mac->ip_tx_queue.front().arp_packet)
        {
          mac->transmit_frame(mac->ip_tx_queue.front().segment, mac->ip_tx_queue.front().size,
                              UDP_PACKET, mac->ip_tx_queue.front().frame_num);
        }
        else
        {
          mac->transmit_frame(mac->ip_tx_queue.front().segment, mac->ip_tx_queue.front().size,
                              mac->getProtocol(mac->ip_tx_queue.front().segment), mac->ip_tx_queue.front().frame_num);
        }
      }
    }
  }
  delete[] frame;
  pthread_exit(NULL);
}

void MAC::transmit_frame(char *segment, int segment_len, char ip_protocol, int &frame_num)
{
  char *frame = new char[MAX_BUF];
  memcpy(frame, segment, segment_len);
  // Check for invalid payload with unknown protocol
  if (ip_protocol != UDP_PACKET && ip_protocol != TCP_PACKET)
  {
    dprintf(RED "Unknown Protocol: %02x\n" RESET, ip_protocol);
    ip_tx_queue.pop();
    return;
  }
  if (tx_channel_state == FREE && segment_len > 0)
  {
    dprintf(YEL "\n----------------Transmitting-------------------\n" RESET);
    dprintf(CYN "Current TX Queue Length: %lu\n" RESET, ip_tx_queue.size());
    dprintf(RED "Current Retransmissions: %d\n" RESET, retransmissions);
    bool last_segment = false;
    if (last_frame_sent)
    {
      last_segment = true;
      new_transmission = false;
    }
    // check ip flags set
    //if (isLastsegment(frame) || ip_protocol == UDP_PACKET || segment_len < MTU)
    if (segment_len < MTU)
    {
      last_segment = true;
    }
    else
    {
      last_segment = false;
    }

    if (!last_frame_sent)
    {
      if (ip_tx_queue.size() > 3)
      {
        if (burst_packets < 1)
        {
          burst_packets = ip_tx_queue.size();
        }
      }
    }

    if (burst_packets > 0)
    {
      last_segment = false;
      burst_packets--;
    }
    else
    {
      burst_packets = 0;
    }

    int frame_len = segment_len + CONTROL_FRAME_LEN;

    create_frame(frame, segment_len, DATA, UNKNOWN);
    pthread_mutex_lock(&tx_mutex);

    unsigned int conv_frame_num = htonl(frame_num);
    memcpy(frame + FRAME_NUM_POS, &conv_frame_num, 4);
    if (tx_continuation > 5)
    {
      last_segment = true;
    }

    // FIXME:: Need to look into what state the MAC should be in
    // if only one segment is received
    if (last_segment)
    {
      memset(frame + 1, 1, 1);
      dprintf(YEL "Last Frame: %d\n" RESET, frame[1]);
      if (new_transmission)
      {
        start_time = time(NULL);
        dprintf(YEL "New Transmission - No more frames\n" RESET);
        myState = BACKING_OFF;
        backOff();
      }
      else
      {
        dprintf(YEL "Transmission Continuation - Last Frame\n" RESET);
        new_transmission = true;
      }
      tx_continuation = 0;
      myState = IDLE;
    }
    else
    {
      if (new_transmission)
      {
        tx_continuation = 0;
        start_time = time(NULL);
        dprintf(YEL "New Transmission - More frames to follow\n" RESET);
        myState = BACKING_OFF;
        backOff();
        myState = TRANSMITTING;
      }
      else
      {
        tx_continuation++;
        dprintf(YEL "Transmission Continuation - More frames to follow\n" RESET);
      }
      new_transmission = false;
    }
    addCRC(frame, frame_len);
    dprintf(YEL "Frame Len: %d\n" RESET, frame_len);
    struct timespec timeout;
    timeout.tv_sec = time(NULL) + 1;
    timeout.tv_nsec = 0;
    int status = mq_timedsend(phy_tx_queue, frame, frame_len, 0, &timeout);
    if (status == -1)
    {
      if (errno == ETIMEDOUT)
      {
        perror(RED "Message Queue Time Out\n" RESET);
      }
      if (!stop_tx)
      {
        perror(RED "mq_send failure\n" RESET);
      }
    }
    else
    {
      dprintf(CYN "mq_send successful with frame_num: %d\n" RESET, frame_num);
      //frame_num++;
      if (ip_tx_queue.front().arp_packet)
      {
        retransmissions = 1000;
      }
      if (last_segment)
      {
        last_frame_sent = true;
        usleep(1000);
        if (ack_received)
        {
          // dprintf(CYN "Last Frame Acknowledged: %d\n" RESET, frames_sent);
          frames_sent++;
          dprintf(CYN "Frames Sent: %d\n" RESET, frames_sent);
          ip_tx_queue.pop();
          last_frame_sent = false;
          ack_reception_failed = false;
          retransmissions = 0;
          ack_received = false;
        }
        else
        {
          if (ack_reception_failed)
          {
            retransmissions = 0;
            ip_tx_queue.pop();
            dprintf(RED "Packet dropped due to lack of ACK\n" RESET);
            ack_reception_failed = false;
          }
          else
          {
            dprintf(RED "ACK for last frame sent has not been received, message to be resent\n" RESET);
            retransmissions++;
          }
        }
        if (retransmissions > 2)
        {
          ack_reception_failed = true;
          retransmissions = 0;
        }
      }
      else
      {
        frames_sent++;
        dprintf(CYN "Frames Sent: %d\n" RESET, frames_sent);
        ip_tx_queue.pop();
      }
    }
    pthread_mutex_unlock(&tx_mutex);
  }
  else
  {
    dprintf(RED "TX Channel Busy: %d\n" RESET, tx_channel_state);
    dprintf(RED "TX MAC State: %d\n" RESET, myState);
  }
  // usleep(1000);
  delete[] frame;
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

  memcpy(data, temp_frame, data_len + CONTROL_FRAME_LEN);
  //extractFrameControl(data);
  delete[] temp_frame;
  delete[] temp_control_frame;
}
/*  Creates a control frame header for any payload to be transmitted
  **  TODO:: Need to improve on which type of information is needed in
  **         the control frame
  */

char *MAC::getControlFrame(FrameControl temp)
{
  char *control_frame = new char[CONTROL_FRAME_LEN];
  memset(control_frame, 0, CONTROL_FRAME_LEN);
  char *offset = control_frame;
  int temp_frame_control = htons(((temp.frame_protocol_type << 12) | (temp.frame_protocol_subtype << 8)));
  char *temp_offset = (char *)&temp_frame_control;
  memcpy(offset, temp_offset, 2); // TODO:: test by removing temp_offset. temp_offset provides a frame_protocol_type
  return control_frame;
}

MAC::FrameControl MAC::extractFrameControl(char *header)
{
  char temp[2];
  memcpy(temp, header, 2);
  MAC::FrameControl frame_control;
  memset(&frame_control, 0, sizeof(frame_control));

  uint16 temp_frame_control = (uint16)(temp[0] & 0x30) >> 4;
  memcpy(&frame_control.frame_protocol_type, &temp_frame_control, 2);
  temp_frame_control = (uint16)(temp[0] & 0x0F);
  memcpy(&frame_control.frame_protocol_subtype, &temp_frame_control, 2);
  return frame_control;
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

char *MAC::extractSourceMAC(char *payload)
{
  char *temp = new char[6];
  memset(temp, 0, 6);
  memcpy(temp, payload + TAP_EXTRA_LOAD + 6, 6);
  return temp;
}

char *MAC::extractDestinationMAC(char *payload)
{
  char *temp = new char[6];
  memset(temp, 0, 6);
  memcpy(temp, payload + TAP_EXTRA_LOAD, 6);
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
  //int frames_received = 0;
  MAC *mac = (MAC *)_arg;
  char buf[MAX_BUF];
  while (!mac->stop_rx)
  {
    //mac->tx_channel_state = mac->FREE;
    struct timespec timeout;
    timeout.tv_sec = time(NULL);
    timeout.tv_nsec = 1e3;

    int status = mq_timedreceive(mac->phy_rx_queue, buf, MAX_BUF, 0, &timeout);

    if (status == -1)
    {
      if (errno == ETIMEDOUT)
      {
        if (mac->tx_channel_state != mac->UNAVAILABLE)
        {
          mac->tx_channel_state = mac->FREE;
          // dprintf(GRN "Nothing Detected - Channel is free\n" RESET);
        }
      }
      else
      {
        if (!mac->stop_rx)
        {
          perror("Failed to read queue");
          exit(0);
        }
      }
    }
    else
    {
      dprintf("\n----------------Receiving-------------------\n");
      if (mac->isCorrectCRC(buf, status))
      {

        dprintf("Correct CRC Received for %d bytes\n", status);
        std::future<void> fut = std::async(std::launch::async, &MAC::analyzeReceivedFrame,
                                           mac, buf, status);
        dprintf("Asynchronous Analyze Thread Launched\n");
        //mac->analyzeReceivedFrame(buf, status);
        //pthread_mutex_unlock(&mac->rx_mutex);
      }
      else
      {
        dprintf("Wrong Packet due to wrong CRC\n");
      }
      memset(buf, 0, MAX_BUF);
    }
  }
  pthread_exit(NULL);
}

// Analyze any received frame to make decisions on the state of the channel
void MAC::analyzeReceivedFrame(char *buf, int buf_len)
{
  dprintf("Message Received\n");
  recv_header = getMACHeader(buf);
  recv_payload = getPayLoad(buf, buf_len);
  int recv_payload_len = buf_len - CONTROL_FRAME_LEN;
  char *sourceMAC = extractSourceMAC(recv_payload);
  char *destinationMAC = extractDestinationMAC(recv_payload);
  // double frame_error_rate = 0;

  int frame_num = buffToInteger(recv_header + FRAME_NUM_POS);
  dprintf(MAG "Packet Frame Received: %d\n" RESET, frame_num);
  MAC::FrameControl incomingFrameControl = extractFrameControl(recv_header);

  if (incomingFrameControl.frame_protocol_subtype == ACK)
  {
    tx_channel_state = FREE;
    dprintf(GRN "Channel free - ACK Received\n" RESET);
  }

  if (strncmp(mac_address, sourceMAC, 6) != 0)
  {
    pthread_mutex_lock(&rx_mutex);
    dprintf("Source MAC Address: ");
    for (int i = 0; i < 6; i++)
    {
      dprintf("%02x:", (unsigned char)sourceMAC[i]);
    }
    dprintf("\n");

    dprintf("Destination MAC Address: ");
    for (int i = 0; i < 6; i++)
    {
      dprintf("%02x:", (unsigned char)destinationMAC[i]);
    }
    dprintf("\n");

    if (!isLastAlienFrame(recv_header))
    {
      tx_channel_state = BUSY;
      dprintf("Channel Busy\n");
    }
    else
    {
      tx_channel_state = FREE;
      dprintf(GRN "Channel free - Last Frame Received\n" RESET);
    }

    pthread_mutex_unlock(&rx_mutex);

    if (memcmp(destinationMAC, mac_address, 6) == 0)
    {
      if (incomingFrameControl.frame_protocol_subtype == ACK)
      {
        printf(MAG "Received ACK For Last Frame Transmitted\n" RESET);
        ack_received = true;
      }
      if (incomingFrameControl.frame_protocol_type == DATA)
      {
        dprintf(GRN "Received Payload Sent to Me\n" RESET);
        if (memcmp(prev_packet, recv_payload, recv_payload_len) == 0)
        {
          printf(RED "Duplicate Packet Received and Discarded\n" RESET);
          printf(RED "Resending ACK\n" RESET);
          if (isLastAlienFrame(recv_header))
          {
            sendACK(recv_payload, frame_num);
          }
        }
        else
        {
          memcpy(prev_packet, recv_payload, recv_payload_len);

          uint16 ether_type = -1;
          memcpy(&ether_type, recv_payload + 2, sizeof(ether_type));
          ether_type = htons(ether_type);

          if (ether_type == ETH_P_IP)
          {
            printf("IP Packet Received : %d\n", ether_type);
            updatePeerRxStatistics(sourceMAC, frame_num, buf_len);
          }
          else if (ether_type == ETH_P_ARP)
          {
            printf("ARP Packet Received : %d\n", ether_type);
          }
          sendToIPLayer(recv_payload, recv_payload_len);
          if (isLastAlienFrame(recv_header))
          {
            sendACK(recv_payload, frame_num);
          }
          else
          {
            frames_before_last_frame++;
          }
        }

        //ack_received = false;
      }
    }

    if (memcmp(destinationMAC, broadcast_address, 6) == 0)
    {
      dprintf(GRN "This is a broadcast message\n" RESET);
      tx_channel_state = FREE;
      sendToIPLayer(recv_payload, recv_payload_len);
    }
  }
  else
  {
    tx_channel_state = FREE;
    if (memcmp(sourceMAC, mac_address, 6) == 0)
    {
      dprintf("I am transmitting\n");
    }

    if (incomingFrameControl.frame_protocol_subtype == ACK)
    {
      tx_channel_state = FREE;
      dprintf(GRN " This is the ACK that I transmitted\n" RESET);
    }
  }

  delete[] recv_header;
  delete[] sourceMAC;
  delete[] destinationMAC;
}

void MAC::sendToIPLayer(char *payload, int payload_len)
{
  int nwrite = 0;
  dprintf(GRN "Writing to TAP Interface\n" RESET);
  for (int i = 0; i < payload_len; i++)
  {
    nwrite = cwrite(tunfd, (char *)&payload[i], payload_len);
  }
  if (nwrite < 0)
  {
    perror(RED "Error writing to TAP interface\n" RESET);
  }
  dprintf(GRN "Done Writing to TAP interface\n" RESET);
}

/*
  // check if the tcp segment is the last segment
  */

bool MAC::isLastsegment(char *segment)
{
  unsigned char flag;
  memset(&flag, 0, 1);
  memcpy(&flag, segment + ETH_HEADER_LEN + IP_HEADER_LEN + IP_FLAG_POS, sizeof(char));
  dprintf(CYN "TCP Flags: %02x\n" RESET, flag);
  return (isBitSet(flag, 0) | isBitSet(flag, 1) | isBitSet(flag, 2) | isBitSet(flag, 3));
}

bool MAC::isLastAlienFrame(char *frame)
{
  if (frame[1] == 1)
  {
    return true;
  }
  else
  {
    return false;
  }
}

void MAC::backOff()
{
  cw = cw_min + rand() % (cw_max - cw_min);
  dprintf(GRN "Random CW: %d\n" RESET, cw);
  for (int i = 0; i < cw; i++)
  {
    if (tx_channel_state == BUSY)
    {
      if (i > 0)
      {
        i--;
        dprintf(RED "BACK OFF PAUSED: %d \n" RESET, i);
      }
    }
    else
    {
      dprintf(GRN "BACK OFF RESUMED: %d \n" RESET, i);
      usleep(SLOT_TIME);
    }
  }
  myState = IDLE;
}

// HACK:: Little Endian and Big Endian issues with CRC
void MAC::addCRC(char *frame, int &frame_len)
{
  unsigned long crc = 0; //crc32(0L,Z_NULL,0);
  crc = crc32(crc, (const unsigned char *)frame, frame_len);
  memcpy(frame + frame_len, &crc, 4);
  //  dprintf("CRC sent: %lu\n", crc);
  frame_len = frame_len + 4;
}

bool MAC::isCorrectCRC(char *buf, int buf_len)
{
  unsigned long crc = 0; //crc32(0L,Z_NULL,0);
  crc = crc32(crc, (const unsigned char *)buf, buf_len - 4);
  // printf("CRC Received: %lu\n",crc);
  unsigned long received_crc = htonl((unsigned long)buffToInteger(buf + buf_len - 4));
  //printf("CRC Received 2: %lu\n",received_crc);
  if (crc == received_crc)
  {
    return true;
  }
  else
  {
    return false;
  }
}

void MAC::set_banned_addresses()
{
  std::string mac_str = "01:00:5e:00:00:fb";
  for (uint idx = 0; idx < 6; ++idx)
  {
    mdns[idx] = hex_digit(mac_str[3 * idx]) << 4;
    mdns[idx] |= hex_digit(mac_str[1 + 3 * idx]);
  }
}

bool MAC::isAddressBanned(const char *add_check)
{
  if (memcmp(mdns, add_check, 3) == 0)
  {
    return true;
  }
  else
  {
    return false;
  }
}

char MAC::getProtocol(char *frame)
{
  return *(frame + ETH_HEADER_LEN + 9);
}

int MAC::updatePeerTxStatistics(char peer_address[6])
{
  int pos = getPeerPosition(peer_address);
  if (pos < 0)
  {
    // if peer does not exist
    MAC::Peer *new_peer = new Peer;
    memcpy(new_peer->mac_address, peer_address, 6);
    new_peer->frames_sent = 1;
    new_peer->frames_received = 0;
    new_peer->frame_errors = 0;
    peerlist.push_back(*new_peer);
    printf("New Peer Added\n");
    return new_peer->frames_sent;
  }
  else
  {
    // if peer exists;
    printf("Peer Exists\n");
    int temp = peerlist.at(pos).frames_sent++;
    printf("Frames Sent to one of peers = %d\n", temp);
    return temp;
  }
}

void MAC::updatePeerRxStatistics(char peer_address[6], int frame_num_received, int frame_len)
{
  dprintf(YEL "Peer List Size: %lu\n" RESET, peerlist.size());
  int pos = getPeerPosition(peer_address);
  if (pos < 0)
  {
    // if peer does not exist
    MAC::Peer *new_peer = new Peer;
    memcpy(new_peer->mac_address, peer_address, 6);
    new_peer->frames_sent = 0;
    new_peer->frames_received = frame_num_received + 1;
    new_peer->frame_errors = 0;
    peerlist.push_back(*new_peer);
    printf("New RX Peer Added\n");
  }
  else
  {
    // if peer exists;
    printf("RX Peer Exists\n");

    peerlist.at(pos).frame_errors = frame_num_received - peerlist.at(pos).frames_received;
    peerlist.at(pos).frames_received++;
    peerlist.at(pos).bit_error_rate = ((float)peerlist.at(pos).frame_errors) / ((float)frame_num_received);
    printf(MAG "Frame Num Received: %d\n" RESET, frame_num_received);
    printf("Frame Errors: %d\n", peerlist.at(pos).frame_errors);
    std::cout << std::fixed;
    std::cout << "Frame Error Rate: " << std::setprecision(5) << peerlist.at(pos).bit_error_rate << '\n';
    printf(MAG "Number of frames Received: %d\n" RESET, peerlist.at(pos).frames_received);
  }
}

int MAC::getPeerPosition(char peer_address[6])
{
  for (unsigned int i = 0; i < peerlist.size(); i++)
  {
    if (memcmp(peerlist[i].mac_address, peer_address, 6) == 0)
    {
      return i;
    }
  }
  return -1;
}

void MAC::sendACK(char *recv_payload, int frame_num)
{
  printf(GRN "Sending ACK\n" RESET);
  char *ack_frame = new char[50];
  memcpy(ack_frame, recv_payload, ETH_HEADER_LEN);
  char *sourceMAC = extractSourceMAC(recv_payload);
  char *destinationMAC = extractDestinationMAC(recv_payload);
  memcpy(ack_frame + TAP_EXTRA_LOAD, sourceMAC, 6);
  memcpy(ack_frame + TAP_EXTRA_LOAD + 6, destinationMAC, 6);
  int frame_len = TAP_EXTRA_LOAD + 6 + 6;
  create_frame(ack_frame, frame_len, MANAGEMENT, ACK);
  unsigned int conv_frame_num = htonl(frame_num);
  memcpy(ack_frame + FRAME_NUM_POS, &conv_frame_num, 4);
  frame_len = frame_len + CONTROL_FRAME_LEN;
  addCRC(ack_frame, frame_len);
  int status = mq_send(phy_tx_queue, ack_frame, frame_len, 0);

  if (status == -1)
  {
    if (!stop_tx)
    {
      perror(RED "mq_send failure\n" RESET);
    }
  }
  else
  {
    printf(GRN "ACK Packet Sent to PHY\n" RESET);
  }
  delete[] ack_frame;
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
int buffToInteger(char *buffer)
{
  // from https://stackoverflow.com/questions/34943835/convert-four-bytes-to-integer-using-c
  int a = static_cast<int>(static_cast<unsigned char>(buffer[0]) << 24 |
                           static_cast<unsigned char>(buffer[1]) << 16 |
                           static_cast<unsigned char>(buffer[2]) << 8 |
                           static_cast<unsigned char>(buffer[3]));
  return a;
}
//
bool isBitSet(unsigned char c, int n)
{
  return (1 & (c >> n));
}

std::string exec(const char *cmd)
{
  std::array<char, 128> buffer;
  std::string result;
  std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
  if (!pipe)
    throw std::runtime_error("popen() failed!");
  while (!feof(pipe.get()))
  {
    if (fgets(buffer.data(), 128, pipe.get()) != NULL)
      result += buffer.data();
  }
  return result;
}

unsigned char hex_digit(char ch)
{
  if (('0' <= ch) && (ch <= '9'))
  {
    ch -= '0';
  }
  else
  {
    if (('a' <= ch) && (ch <= 'f'))
    {
      ch += 10 - 'a';
    }
    else
    {
      if (('A' <= ch) && (ch <= 'F'))
      {
        ch += 10 - 'A';
      }
      else
      {
        ch = 16;
      }
    }
  }
  return ch;
}
