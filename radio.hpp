
struct node_parameters {
 

  char my_ip[100];
  char target_ip[100];
  double rx_freq;
  double rx_rate;
  double rx_gain;
  double tx_freq;
  double tx_rate;
  double tx_gain;

  // liquid OFDM settings
  int rx_subcarriers;
  int rx_cp_len;
  int rx_taper_len;
  int rx_subcarrier_alloc_method;
  int rx_guard_subcarriers;
  int rx_central_nulls;
  int rx_pilot_freq;
  char rx_subcarrier_alloc[2048];

  double tx_gain_soft;
  int tx_subcarriers;
  int tx_cp_len;
  int tx_taper_len;
  int tx_modulation;
  int tx_crc;
  int tx_fec0;
  int tx_fec1;
  int tx_subcarrier_alloc_method;
  int tx_guard_subcarriers;
  int tx_central_nulls;
  int tx_pilot_freq;
  char tx_subcarrier_alloc[2048];

};
