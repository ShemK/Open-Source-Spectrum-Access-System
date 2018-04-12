#include "Engine.hpp"
#include <iostream>

Engine::Engine() {}
Engine::~Engine() {}

// used to modify any payload to be transmitted
unsigned char* Engine::modifyTxPacket(unsigned char *packet,unsigned int &packet_len, int node_id){
    std::cout << "-------------------asuifas;f-------------------\n";
    unsigned char *new_packet = new unsigned char[packet_len];
    // modify packet;
    memcpy(new_packet,packet,packet_len);
    return new_packet;
}

// remove any header information that might have been added to the received payload
unsigned char * Engine::getSharedInformation(unsigned char *packet, unsigned int &packet_len){
    return packet;
}
