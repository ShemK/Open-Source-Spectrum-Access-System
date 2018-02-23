 #ifndef COGNITIVE_ENGINE_H
 #define COGNITIVE_ENGINE_H

#include <pthread.h>
#include <queue>
#include <string.h>
#include "Engine.hpp"
#include "ofdm_phy.hpp"
#include <map>
#include <unordered_map>
#include "BufferQ.h"
#include <liquid/liquid.h>

#define RED "\x1B[31m"
#define GRN "\x1B[32m"
#define YEL "\x1B[33m"
#define BLU "\x1B[34m"
#define MAG "\x1B[35m"
#define CYN "\x1B[36m"
#define WHT "\x1B[37m"
#define RESET "\x1B[0m"


class CognitiveEngine: public Engine{
public:
    CognitiveEngine(PhyLayer *PHY);
    ~CognitiveEngine();
    PhyLayer *PHY;
    void execute();

    int acceptable_nacks = 5;
    int num_rx_channels;

    bool links_created;


    enum RxLinkState{
        CALIBERATING,
        CALIBERATED,
        CALIBERATION_LOST
    };

    int max_points = 5;
    // currently based on the rx_channel
    // might want to make it based on the tx_node;
    struct RxChannel{ // NOTE toself: might need to convert these to be based on nodes rather than channels
        RxLinkState state;
        float sum_rssi;
        float sum_evm;
        float avg_rssi;
        float avg_evm;
        int no_of_points; // in milliseconds ???
        float last_rssi;
        float last_evm;
        int node_id;
        float cfo;
        int frame_num; // slippery slope to making this permanent
        int time_stamp;
    };
    
    // meant to replace or help with the above struct
    // information should be for a specific node
    struct NodeInfo{
        int node_id;
        int rx_channel_id;
        int expected_frame = 0;
        int prev_expected_frame = 0;
        int last_frame_received = 0;
        int last_frame_time = 0;
        int frame_errors = 0;
        int total_frames_received;
        double total_bits;
        double frame_rate;
    };

    RxChannel *rx_channels;
    std::unordered_map<int, NodeInfo> node_info;

    void create_rx_links(int num_channels);
    void pushInfo(ChannelInfo &new_info);


    ChannelInfo  pullInfo();
    void calculateStats(ChannelInfo &new_info);
    void caliberateFrequency(int channel, float cfo);
    unsigned char* modifyTxPacket(unsigned char *packet, unsigned int &packet_len,int node_id);
    unsigned char *getSharedInformation(unsigned char *packet, unsigned int & packet_len);
   
    pthread_mutex_t queueMutex; // mutex for the queue with information
    pthread_mutex_t stateMutex; // mutex for adjusting the internal state of the engine

    std::queue<ChannelInfo> info_queue;

    struct PayloadInfo{
        unsigned char *payload;
        int payload_len;
    };
     // hashmap for arq
    
    struct ReceiverPayLoad{
        int node_id;
        unsigned int frame_num = 0;
        std::unordered_map<int,PayloadInfo> retx_queue;
    };

    std::unordered_map<int,ReceiverPayLoad> retx_map;


    std::queue <NACK> nack_queue;
    pthread_mutex_t nackMutex;
    std::queue <NACK>nack_service_queue;
    pthread_mutex_t nackServiceMutex;
    

    void printSharedInfo(ExchangeInfo info);
    void printStatInfo(ChannelInfo info);
    Engine::NACK calculatePacketLoss(ChannelInfo &new_info);
    void storePayload(unsigned char *payload,unsigned int payload_len, int node_id,unsigned int &frame_num); // store payload in retransmission queue
    unsigned char * getPayload(unsigned int frame_num, unsigned int &payload_len,int node_id);
    int getNackServiceSize();
    int getNackSize();
    std::map<int,ExchangeInfo> myInfo;

    Engine::NACK popServiceNack();


    ExchangeInfo txInfo;
    Engine::NACK createNack(int node_id,int new_error);
    Engine::NACK popNack();
    Engine::NACK getNackFront();

    ofdmflexframegenprops_s arq_props;

    int max_tx_queue_size = 100;
private:
    
};

#endif