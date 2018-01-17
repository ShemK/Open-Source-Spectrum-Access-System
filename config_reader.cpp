#include "config_reader.hpp"

ConfigReader::ConfigReader(const char *file){
  config_t cfg, *cf;
  cf = &cfg;
  config_init(cf);
  node_id = -1;
    status = 0;

    if (!config_read_file(cf, file)) {
      fprintf(stderr, "%s:%d - %s\n",
              config_error_file(cf),
              config_error_line(cf),
              config_error_text(cf));
      config_destroy(cf);
      return ;
    }
    char temp[20] = "num_nodes";
    int num_nodes = 0;
    if(!config_lookup_int(cf,temp,&num_nodes)){
        // error getting the number of nodes;
        status = -1;
        return ;
    }

    // get controller info
    sprintf(temp,"controller");

    controller = 0;

    if(!config_lookup_bool(cf,temp,&controller)){
        // error getting controller information;
        status = -1;
        return ;
    }

    if(!controller){
        // get rx_bandwidth
        sprintf(temp,"rx_rate");
        if(!config_lookup_float(cf,temp,&rx_rate)){
            status = -1; 
            return;
        } 
        // get node id
        sprintf(temp,"node_id");
        if(!config_lookup_int(cf,temp,&node_id)){
            status = -1; 
            return;
        } 
        // get rx frequency
        sprintf(temp,"rx_freq");
        if(!config_lookup_float(cf,temp,&rx_freq)){
            status = -1; 
            return;
        } 

        sprintf(temp,"rx_channels");
        if(!config_lookup_int(cf,temp,&channels)){
            status = -1; 
            return;
        } 
        
        if(node_id <= channels){
            tx_rate = rx_rate/channels;
            if(node_id%2 == 0){
                rx_freq = rx_freq + (node_id/2)*tx_rate;
                tx_freq = rx_freq - (rx_rate/2 + tx_rate/2);
            } else{
                rx_freq = rx_freq - (((node_id + 1)/2) - 1)*tx_rate;
                tx_freq = rx_freq + (rx_rate/2 + tx_rate/2);
            }
        }
    } 
}

int ConfigReader::get_status(){
    return status;
}

ConfigReader::~ConfigReader(){

}
/*
int main(){
    ConfigReader x("Hello");
}
*/