#include <libconfig.h>
#include <iostream>

class ConfigReader{
public:
    ConfigReader();
    ConfigReader(const char *file);
    ~ConfigReader();
    
    double rx_rate;
    int node_id;
    double rx_freq;
    double tx_freq;
    double tx_rate;
    int controller;
    int channels;
    int get_status();
private:
    int status;

};
