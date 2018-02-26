#include "CognitiveEngine.hpp"
#include <iostream>
#include <stdexcept>
CognitiveEngine::CognitiveEngine(PhyLayer *PHY){
    pthread_mutex_init(&queueMutex,NULL);
    pthread_mutex_init(&stateMutex,NULL);
    pthread_mutex_init(&nackMutex,NULL);
    pthread_mutex_init(&nackServiceMutex,NULL);
    links_created = false;
    this->PHY = PHY;
    ofdmflexframegenprops_init_default(&arq_props);
    arq_props.check = LIQUID_CRC_32;
    arq_props.fec0 = LIQUID_FEC_RS_M8;
    arq_props.fec1 = LIQUID_FEC_CONV_V27;
    arq_props.mod_scheme = LIQUID_MODEM_BPSK;
}


CognitiveEngine::~CognitiveEngine(){
    delete rx_channels;
    for(auto p = retx_map.begin();p != retx_map.end();p++){
        for(auto it = p->second.retx_queue.begin();it != p->second.retx_queue.end();it++){
            delete [] it->second.payload;
        }
    }

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
    } else{
        // place a counter that decreases when no packet is received from nodes
    }

    if(PHY->properties_changed){
        PHY->setARQOfdmProperties(arq_props);
    }
}

// create rx links based on the rx channels
// NOTE: might need to convert this to a hash map
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
    printf("Info Queue Len: %lu\n",info_queue.size());
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
    rx_channels[j].node_id = new_info.tx_node_id;
    if(rx_channels[j].no_of_points >= max_points){ // there is an interval at which these stats are calculated
        rx_channels[j].sum_evm = rx_channels[j].sum_evm - rx_channels[j].last_evm;
        rx_channels[j].sum_rssi = rx_channels[j].sum_rssi - rx_channels[j].last_rssi;
        if(rx_channels[j].no_of_points > 1){
            rx_channels[j].no_of_points--;
        } 
        rx_channels[j].last_rssi = new_info.rssi;
        rx_channels[j].last_evm = new_info.evm;
    } else{
        rx_channels[j].sum_evm = rx_channels[j].sum_evm + new_info.evm;
        rx_channels[j].sum_rssi = rx_channels[j].sum_rssi + new_info.rssi;
        rx_channels[j].no_of_points++;
        rx_channels[j].avg_rssi = rx_channels[j].sum_rssi/rx_channels[j].no_of_points;
        rx_channels[j].avg_evm = rx_channels[j].sum_evm/rx_channels[j].no_of_points;
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

// modifies the packet to add channel info for the node to which this packet will be sent
unsigned char* CognitiveEngine::modifyTxPacket(unsigned char *packet,unsigned int &packet_len, int node_id){
    unsigned char *new_packet = new unsigned char[packet_len + sizeof(ExchangeInfo)];
    std::cout << "----------Sending Info for node: " << node_id << "---------\n";
    memset(&txInfo,0,sizeof(txInfo));
    if(nack_queue.size() > 0){
        txInfo.infoNack = popNack();
        txInfo.sending_nack = true;
    } else{
                    
    }
    // Need to change this to hash maps
    //
    if(node_id == -1){
        txInfo.node_id = -1;
        memcpy(new_packet,&txInfo,sizeof(txInfo));
        memcpy(new_packet+sizeof(txInfo),packet,packet_len);
        packet_len = packet_len + sizeof(txInfo);
        return new_packet;
    } else{ // 
        for(int i = 0; i < num_rx_channels; i++){
            if(node_id == rx_channels[i].node_id){
                txInfo.avg_rssi = rx_channels[i].avg_rssi;
                txInfo.node_id = node_id;
                txInfo.avg_evm = rx_channels[i].avg_evm;
                memcpy(new_packet,&txInfo,sizeof(txInfo));
                memcpy(new_packet+sizeof(txInfo),packet,packet_len);
                packet_len = packet_len + sizeof(txInfo);
                return new_packet;// if node_id is found
            }
        }
        txInfo.node_id = -1;
        memcpy(new_packet,&txInfo,sizeof(txInfo));
        memcpy(new_packet+sizeof(txInfo),packet,packet_len);
        packet_len = packet_len + sizeof(txInfo);
        //printSharedInfo(txInfo);
        return new_packet;
    }

}

unsigned char * CognitiveEngine::getSharedInformation(unsigned char *packet, unsigned int &packet_len){
    packet_len = packet_len - sizeof(ExchangeInfo);
    ExchangeInfo *recvInfo = (ExchangeInfo *) packet;
    std::cout << "----------Received Info: -----------------" << "\n";
    printSharedInfo(*recvInfo);
    return packet+sizeof(ExchangeInfo);
}

void CognitiveEngine::printSharedInfo(ExchangeInfo info){
    std::cout << "node_id: " << info.node_id << "\n";
    std::cout << "rssi: " << info.avg_rssi << "\n";
    std::cout << "evm: " << info.avg_evm << "\n";
    std::cout << "retransmit?: " << info.retransmit<< "\n";
    std::cout << "nack?: " << info.sending_nack << "\n";
    if(info.sending_nack == 1){
        printf(CYN "Lost Expected Frame: %d\n" RESET,info.infoNack.expected_frame );
        printf(CYN "Errors Reported: %d\n" RESET,info.infoNack.errors );
        printf(CYN "Reported by: %d\n" RESET,info.infoNack.sender_id );
        printf(CYN "Reporting to: %d\n" RESET,info.infoNack.node_id );
        if(info.infoNack.node_id == PHY->get_node_id()){
            pthread_mutex_lock(&nackServiceMutex);
            nack_service_queue.push(info.infoNack);
            pthread_mutex_unlock(&nackServiceMutex);
        }
    }
}

void CognitiveEngine::printStatInfo(ChannelInfo info){
    std::cout << "node_id: " << info.tx_node_id << "\n";
    std::cout << "rssi: " << info.rssi<< "\n";
    std::cout << "evm: " << info.evm<< "\n";
    std::cout << "channel_id: " << info.channelId << "\n";
}

Engine::NACK CognitiveEngine::calculatePacketLoss(ChannelInfo &new_info){
    NACK new_nack;
    new_nack.errors = 0;
    if(new_info.payload_valid){
        try{
            int key = new_info.tx_node_id;
            if(!new_info.retransmit){
                
                int new_error = new_info.frame_num - node_info[key].expected_frame;
                node_info[key].frame_errors = node_info[key].frame_errors + new_error;
                node_info[key].total_frames_received++;
                node_info[key].total_bits = node_info[key].total_bits + new_info.payload_len*8;
            
            // reset calculations
            
                int diff_frame = new_info.frame_num - node_info[key].last_frame_received;
                if(diff_frame <= 0 || diff_frame > 2000){
                    node_info[key].last_frame_received = new_info.frame_num;
                    node_info[key].frame_errors = 0;
                    node_info[key].last_frame_time = new_info.time_stamp;
                    node_info[key].total_bits = 0;
                }

                float error_rate = ((float)node_info[key].frame_errors) / ((float)diff_frame);
                node_info[key].prev_expected_frame =  node_info[key].expected_frame;
                node_info[key].expected_frame = new_info.frame_num+1;


                int time_elapsed = new_info.time_stamp - node_info[key].last_frame_time;
                if(time_elapsed > 1){
                    double bit_rate = node_info[key].total_bits/((double)time_elapsed);
                    printf(BLU "BitRate: %f\n" RESET,bit_rate);
                }
                printf(BLU "PHY Errors: %d\n" RESET,node_info[key].frame_errors);
                printf(BLU "PHY Error Rate: %f\n" RESET,error_rate);
                return createNack(key,new_error);
            } else{
                node_info[key].frame_errors--;
            }

            
        }
        catch (const std::out_of_range& oor) {
            NodeInfo new_node;
            new_node.node_id = new_info.tx_node_id;
            new_node.last_frame_time = new_info.time_stamp;
            new_node.last_frame_received = new_info.frame_num;
            node_info.emplace(new_node.node_id,new_node);
        }
    }
    return new_nack;
}

Engine::NACK CognitiveEngine::createNack(int node_id,int new_error){
    NACK new_nack; // extra work -- need to find wa
    new_nack.node_id = node_id;
    new_nack.errors = 0;
    new_nack.sender_id = PHY->get_node_id();
    if(new_error > 0 && new_error < acceptable_nacks){     
        new_nack.expected_frame = node_info[node_id].prev_expected_frame;
        new_nack.errors = new_error; 
        new_nack.node_id = node_id;

        pthread_mutex_lock(&nackMutex);
        nack_queue.push(new_nack);
        pthread_mutex_unlock(&nackMutex); 
    } 
    return new_nack;
}

Engine::NACK CognitiveEngine::popNack(){
    pthread_mutex_lock(&nackMutex);
    NACK temp = nack_queue.front();
    nack_queue.pop();
    pthread_mutex_unlock(&nackMutex);
    return temp;
}

void CognitiveEngine::storePayload(unsigned char *payload, unsigned int payload_len,int node_id, unsigned int &frame_num){
    PayloadInfo new_load;
    new_load.payload_len = payload_len;
    new_load.payload = new unsigned char[payload_len];
    memcpy(new_load.payload,payload,payload_len);

    try{
        retx_map[node_id].node_id = node_id;
        frame_num = retx_map[node_id].frame_num;
        retx_map[node_id].frame_num++;
        retx_map[node_id].retx_queue.emplace(frame_num,new_load);
    
        if(retx_map[node_id].retx_queue.size() > max_tx_queue_size){
            delete [] retx_map[node_id].retx_queue[frame_num-max_tx_queue_size].payload;
            retx_map[node_id].retx_queue.erase(frame_num-max_tx_queue_size);
        }
    } catch (const std::out_of_range& oor) {
        ReceiverPayLoad rxLoad;
        retx_map.emplace(node_id,rxLoad);
    }

}


unsigned char * CognitiveEngine::getPayload(unsigned int frame_num, unsigned int &payload_len, int node_id){
    try{
        payload_len = retx_map[node_id].retx_queue.at(frame_num).payload_len;
        return retx_map[node_id].retx_queue.at(frame_num).payload;
    }
    catch (const std::out_of_range& oor) {
        payload_len = 0;
        return NULL;
    }
}

int CognitiveEngine::getNackServiceSize(){
    return nack_service_queue.size();
}


int CognitiveEngine::getNackSize(){
    return nack_queue.size();
}

Engine::NACK CognitiveEngine::getNackFront(){
    return nack_queue.front();
}

Engine::NACK CognitiveEngine::popServiceNack(){
    pthread_mutex_lock(&nackServiceMutex);
    NACK temp = nack_service_queue.front();
    nack_service_queue.pop();
    pthread_mutex_unlock(&nackServiceMutex);
    return temp;
}

