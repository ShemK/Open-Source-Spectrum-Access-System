#include "ArpRouter.hpp"

///g++ -o test_arp test_arp.cpp ArpRouter.cpp -ldnet -std=c++11

/*
**  Class to be utilized for future routing purposes 
**  Used to manipulate the ARP table
*/

ArpRouter::ArpRouter(){
  if ((arp = arp_open()) == NULL){
    perror("arp_open");
  }

}

ArpRouter::~ArpRouter(){
  arp_close(arp);
}

void ArpRouter::addToTable(const char *ipaddr, const char *hwaddr){

  if (addr_pton(ipaddr, &entry.arp_pa) < 0 || addr_pton(hwaddr, &entry.arp_ha) < 0) {
    perror("addr_pton");
  }

  if (arp_add(arp, &entry) < 0){
    perror("arp_add");
  } else{
    printf("%s added\n", addr_ntoa(&entry.arp_pa));
  }
}

void ArpRouter::getFromTable(const char *ipaddr){

  if (addr_pton(ipaddr, &entry.arp_pa) < 0){
    perror("addr_pton");
  }

  if (arp_get(arp, &entry) < 0){
    perror("arp_get");
  }

  print_arp(&entry, NULL);

}

void ArpRouter::show() {
  if (arp_loop(arp, print_arp, NULL) < 0){
    perror("arp_loop");
  }

}


void ArpRouter::deleteFromTable(const char *ipaddr){
  if (addr_pton(ipaddr, &entry.arp_pa) < 0){
    perror("addr_pton");
  }

  if (arp_delete(arp, &entry) < 0){
    perror("arp_delete");
  }

  printf("%s deleted\n", addr_ntoa(&entry.arp_pa));
}

int print_arp(const struct arp_entry *entry, void *arg){
  printf("%s at %s\n", addr_ntoa(&entry->arp_pa),
  addr_ntoa(&entry->arp_ha));
  return 0;
}
