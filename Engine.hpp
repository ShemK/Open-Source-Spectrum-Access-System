
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
        
    };

    virtual void create_rx_links(int num_channels) = 0;
    virtual void pushInfo(ChannelInfo &new_info) = 0;

    pthread_mutex_t queueMutex;

    std::queue<ChannelInfo> info_queue;

private:
    
};

#endif