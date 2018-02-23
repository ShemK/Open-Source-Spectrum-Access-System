
 #ifndef ENGINE_H
 #define ENGINE_H

#include <pthread.h>
#include <queue>
#include <string.h>

class Engine{
public:
    Engine();
    ~Engine();
    virtual void execute() = 0;


    enum InformationFlag{
        PACKET_FLAG, // flag when a packet is received
    };
    struct ChannelInfo{
        int channelId;
        InformationFlag info_flag;
        float rssi;
        float evm;
        float cfo; // carrier frequency offset
        int tx_node_id;
        bool payload_valid;
        int frame_num;
        int time_stamp;
        int payload_len;
        bool retransmit = false;
    };



    struct NACK{
        int expected_frame;
        int errors = 0;
        int node_id;
        int sender_id;
    };

    // lazy way of creating a modular header for information
    // but can easily scale if more information is to be shared
    // will need to change the variables to one single array of chars
    struct ExchangeInfo{
        int node_id;
        float avg_rssi;
        float avg_evm;
        bool sending_nack = false;
        bool retransmit = false;
        NACK infoNack;
    };

    virtual void create_rx_links(int num_channels) = 0;
    virtual void pushInfo(ChannelInfo &new_info) = 0;
    virtual unsigned char* modifyTxPacket(unsigned char *packet, unsigned int &packet_len,int node_id);
    virtual unsigned char *getSharedInformation(unsigned char *packet, unsigned int & packet_len);
    virtual void storePayload(unsigned char *payload,unsigned int payload_len, int node_id,unsigned int &frame_num) = 0;
    virtual unsigned char * getPayload(unsigned int frame_num, unsigned int &payload_len, int node_id) = 0; 
    virtual int getNackServiceSize() = 0;
    virtual int getNackSize() = 0;
    virtual NACK popServiceNack() = 0;
    virtual NACK calculatePacketLoss(ChannelInfo &new_info) = 0;
    virtual NACK getNackFront() = 0;
    pthread_mutex_t queueMutex;

    std::queue<ChannelInfo> info_queue;

private:
    
};

#endif