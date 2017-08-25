/*
Modification from:
https://github.com/ardikars/CodeLab/blob/master/c/arp.c


*/

#include <dnet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

class ArpRouter{

public:
  ArpRouter();
  ~ArpRouter();
  void show();
  void addToTable(const char *ipaddr, const char *hwaddr);
  void getFromTable(const char *ipaddr); //TODO:: need to change to get MAC address
  void deleteFromTable(const char *ipaddr);

private:
  struct arp_entry entry;
  arp_t *arp;
};
int print_arp(const struct arp_entry *entry, void *arg);
