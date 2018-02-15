#include "CognitiveEngine.hpp"


CognitiveEngine::CognitiveEngine(PhyLayer *PHY){
    pthread_mutex_init(&queueMutex,NULL);
    pthread_mutex_init(&stateMutex,NULL);
    links_created = false;
    this->PHY = PHY;
    
}


CognitiveEngine::~CognitiveEngine(){
    delete rx_channels;
}

void CognitiveEngine::execute(){
    
    if(info_queue.size() > 0 && links_created){ // mutex not needed since it is just reading
        ChannelInfo new_info = pullInfo();
        calculateStats(new_info);
        pthread_mutex_lock(&stateMutex);
        RxLinkState current_state = rx_channels[new_info.channelId].state;
        pthread_mutex_unlock(&stateMutex);
        switch (current_state) {
            case CALIBERATING:
                caliberateFrequency(new_info.channelId,new_info.cfo);
                break;
            case CALIBERATED:
                break;
            case CALIBERATION_LOST:
                break;
            default:
                break;
        }
    }
}

void CognitiveEngine::create_rx_links(int num_channels){
    pthread_mutex_lock(&stateMutex);
    if(num_channels < 0){
        //perror("Channels less than 0");
        //exit(0);
    }
    if(links_created){
        delete [] rx_channels;
    }
    rx_channels = new RxChannel[num_channels];
    memset(rx_channels,0,num_channels*sizeof(RxChannel));
    num_rx_channels = num_channels;
    links_created = true;
    pthread_mutex_unlock(&stateMutex);
}

// phy utilizes this function to push information to the queue
void CognitiveEngine::pushInfo(ChannelInfo &new_info){
    pthread_mutex_lock(&queueMutex);
    info_queue.push(new_info);
    pthread_mutex_unlock(&queueMutex);
}

// get information from queue of information from the phy
CognitiveEngine::ChannelInfo CognitiveEngine::pullInfo(){
    // very inefficient -- might need to do this in the main loop
    ChannelInfo new_info;
    pthread_mutex_lock(&queueMutex);
    new_info = info_queue.front();
    info_queue.pop();
    pthread_mutex_unlock(&queueMutex);
    return new_info;
}


// calculate the statistics from the phy
void CognitiveEngine::calculateStats(ChannelInfo &new_info){
    pthread_mutex_lock(&stateMutex);
    int j = new_info.channelId;
    
    if(rx_channels[j].no_of_points >= max_points){
        rx_channels[j].sum_evm = rx_channels[j].sum_evm - rx_channels[j].last_evm;
        rx_channels[j].sum_rssi = rx_channels[j].sum_rssi - rx_channels[j].last_rssi;
        if(rx_channels[j].no_of_points > 1){
            rx_channels[j].no_of_points--;
        } 
    } else{
        rx_channels[j].sum_evm = rx_channels[j].sum_evm + new_info.evm;
        rx_channels[j].sum_rssi = rx_channels[j].sum_rssi + new_info.rssi;
        rx_channels[j].no_of_points++;
        rx_channels[j].avg_rssi = rx_channels[j].sum_rssi/rx_channels[j].no_of_points;
        rx_channels[j].avg_evm = rx_channels[j].sum_rssi/rx_channels[j].no_of_points;
    }
    pthread_mutex_unlock(&stateMutex);
}


void CognitiveEngine::caliberateFrequency(int channel, float cfo){
    float offset = 0.0f;
    if(cfo > 0.001){
        offset = 0.05e3;
    } else if(cfo < -0.001){
        offset = -0.05e3;
    }

    if(cfo > 0.01){
        offset = 1e3;
    } else if(cfo < -0.01){
        offset = -1e3;
    }
    PHY->adjustRxFreq(offset,channel);
}