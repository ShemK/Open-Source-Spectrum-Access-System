 #ifndef COGNITIVE_ENGINE_H
 #define COGNITIVE_ENGINE_H

#include <pthread.h>
#include <queue>
#include <string.h>
#include "Engine.hpp"
#include "ofdm_phy.hpp"

class CognitiveEngine: public Engine{
public:
    CognitiveEngine(PhyLayer *PHY);
    ~CognitiveEngine();
    PhyLayer *PHY;
    void execute();

    int num_rx_channels;

    bool links_created;


    enum RxLinkState{
        CALIBERATING,
        CALIBERATED,
        CALIBERATION_LOST
    };

    int max_points = 5;
    struct RxChannel{
        RxLinkState state;
        float sum_rssi;
        float sum_evm;
        float avg_rssi;
        float avg_evm;
        int no_of_points; // in milliseconds
        float last_rssi;
        float last_evm;
        
        float cfo;
    };
    

    RxChannel *rx_channels;

    void create_rx_links(int num_channels);
    void pushInfo(ChannelInfo &new_info);


    ChannelInfo  pullInfo();
    void calculateStats(ChannelInfo &new_info);
    void caliberateFrequency(int channel, float cfo);

    pthread_mutex_t queueMutex; // mutex for the queue with information
    pthread_mutex_t stateMutex; // mutex for adjusting the internal state of the engine

    std::queue<ChannelInfo> info_queue;

private:
    
};

#endif